#include "mfrc522.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

static inline void cs_select(MFRC522_t *dev) { gpio_put(dev->cs_pin, 0); }

static inline void cs_deselect(MFRC522_t *dev) { gpio_put(dev->cs_pin, 1); }

/**
 * Write a single byte to a MFRC522 register.
 */
static void MFRC522_write_register(MFRC522_t *dev, uint8_t reg, uint8_t val) {
  // Data format:
  //   Address byte: 0xxxxxx0 (reg << 1), bit0=0 for write
  //   Data byte: val
  uint8_t addr_byte = ((reg << 1) & 0x7E);
  uint8_t outBuf[2] = {addr_byte, val};

  cs_select(dev);
  spi_write_blocking(dev->spi_port, outBuf, 2);
  cs_deselect(dev);
}

/**
 * Read a single byte from a MFRC522 register.
 */
static uint8_t MFRC522_read_register(MFRC522_t *dev, uint8_t reg) {
  // Data format:
  //   Address byte: 1xxxxxx0 (reg << 1 | 0x80), bit0=0 for read
  uint8_t addr_byte = ((reg << 1) & 0x7E) | 0x80;

  uint8_t outBuf[2] = {addr_byte, 0x00};
  uint8_t inBuf[2] = {0};

  cs_select(dev);
  spi_write_read_blocking(dev->spi_port, outBuf, inBuf, 2);
  cs_deselect(dev);

  // The second byte returned is the register content
  return inBuf[1];
}

/**
 * Set specific bits (mask) in a register.
 */
static void MFRC522_set_bits(MFRC522_t *dev, uint8_t reg, uint8_t mask) {
  uint8_t tmp = MFRC522_read_register(dev, reg);
  MFRC522_write_register(dev, reg, tmp | mask);
}

/**
 * Clear specific bits (mask) in a register.
 */
static void MFRC522_clear_bits(MFRC522_t *dev, uint8_t reg, uint8_t mask) {
  uint8_t tmp = MFRC522_read_register(dev, reg);
  MFRC522_write_register(dev, reg, tmp & (~mask));
}

/**
 * Calculate CRC of a data buffer.
 */
static void MFRC522_calculate_crc(MFRC522_t *dev, const uint8_t *data,
                                  size_t length, uint8_t *result) {
  MFRC522_clear_bits(dev, DivIrqReg, 0x04);  // Clear the CRC interrupt
  MFRC522_set_bits(dev, FIFOLevelReg, 0x80); // Clear FIFO pointer

  // Write data to FIFO
  for (size_t i = 0; i < length; i++) {
    MFRC522_write_register(dev, FIFODataReg, data[i]);
  }
  // Start CRC calculation
  MFRC522_write_register(dev, CommandReg, PCD_CALCCRC);

  // Poll for CRC completion
  uint16_t i = 0xFF;
  uint8_t n;
  do {
    n = MFRC522_read_register(dev, DivIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x04)); // bit 2: CRCIRq

  // Read the result
  result[0] = MFRC522_read_register(dev, CRCResultRegL);
  result[1] = MFRC522_read_register(dev, CRCResultRegH);
}

/**
 * Print a helpful message about the bits in ErrorReg.
 */
static void print_mfrc522_error_bits(uint8_t errorVal) {
  // According to the datasheet, ErrorReg bits are:
  //
  // Bit 7: WrErr (Error during write)
  // Bit 6: TempErr
  // Bit 5: Reserved
  // Bit 4: BufferOvfl (FIFO buffer overflow)
  // Bit 3: CollErr (Collision)
  // Bit 2: CRCErr
  // Bit 1: ParityErr
  // Bit 0: ProtocolErr
  printf("  ErrorReg=0x%02X\n", errorVal);
  if (errorVal & 0x80) {
    printf("    WrErr: Error during write.\n");
  }
  if (errorVal & 0x40) {
    printf("    TempErr: Temperature error.\n");
  }
  if (errorVal & 0x10) {
    printf("    BufferOvfl: FIFO buffer overflow.\n");
  }
  if (errorVal & 0x08) {
    printf("    CollErr: Collision.\n");
  }
  if (errorVal & 0x04) {
    printf("    CRCErr: CRC error.\n");
  }
  if (errorVal & 0x02) {
    printf("    ParityErr: Parity error.\n");
  }
  if (errorVal & 0x01) {
    printf("    ProtocolErr: Protocol error.\n");
  }
}

/**
 * Internal function to send/receive data to the card via the FIFO.
 * Similar to Python `_tocard()`
 */
static uint8_t MFRC522_to_card(MFRC522_t *dev, uint8_t cmd,
                               const uint8_t *sendData, size_t sendLen,
                               uint8_t *backData, size_t *backLen,
                               uint8_t *validBits) {
  uint8_t status = MFRC522_ERR;
  uint8_t irqEn = 0;
  uint8_t waitIRq = 0;
  uint8_t n, lastBits;
  size_t i;

  if (cmd == PCD_AUTHENT) {
    irqEn = 0x12;   // IRQ for Auth
    waitIRq = 0x10; // IdleIRq
  } else if (cmd == PCD_TRANSCEIVE) {
    irqEn = 0x77;   // Tx & Rx IRQs
    waitIRq = 0x30; // RxIRq and IdleIRq
  }

  MFRC522_write_register(dev, CommIEnReg, irqEn | 0x80); // enable interrupts
  MFRC522_clear_bits(dev, CommIrqReg, 0x80);             // clear IRQ bits
  MFRC522_set_bits(dev, FIFOLevelReg, 0x80);             // flush FIFO

  // Idle
  MFRC522_write_register(dev, CommandReg, PCD_IDLE);

  // Write to FIFO
  for (i = 0; i < sendLen; i++) {
    MFRC522_write_register(dev, FIFODataReg, sendData[i]);
  }

  // Execute command
  MFRC522_write_register(dev, CommandReg, cmd);
  if (cmd == PCD_TRANSCEIVE) {
    // StartSend = set BitFramingReg bit7
    MFRC522_set_bits(dev, BitFramingReg, 0x80);
  }

  // Wait for the command to complete (or timeout)
  uint16_t loopCount = 2000; // Rough wait
  do {
    n = MFRC522_read_register(dev, CommIrqReg);
    loopCount--;
  } while (loopCount && !(n & 0x01) && !(n & waitIRq));

  // Stop sending in case of Transceive
  MFRC522_clear_bits(dev, BitFramingReg, 0x80);

  if (loopCount == 0) {
    // We timed out waiting
    printf("[ERR] MFRC522_to_card() timed out.\n");
    return MFRC522_ERR;
  }

  // Check for errors
  uint8_t errorVal = MFRC522_read_register(dev, ErrorReg);
  if (!(errorVal & 0x1B)) {
    status = MFRC522_OK;
    if (n & irqEn & 0x01) {
      status = MFRC522_NOTAGERR;
      /*printf("[WARN] No tag error (interrupt says no tag?).\n");*/
    }
    if (cmd == PCD_TRANSCEIVE) {
      // Number of bytes in FIFO
      uint8_t fifoLevel = MFRC522_read_register(dev, FIFOLevelReg);
      lastBits = MFRC522_read_register(dev, ControlReg) & 0x07;
      if (validBits) {
        if (lastBits) {
          *validBits = (fifoLevel - 1) * 8 + lastBits;
        } else {
          *validBits = fifoLevel * 8;
        }
      }

      if (fifoLevel > *backLen) {
        // If there's more data than the buffer can hold, limit it
        fifoLevel = *backLen;
      }
      // Read the FIFO
      for (i = 0; i < fifoLevel; i++) {
        backData[i] = MFRC522_read_register(dev, FIFODataReg);
      }
      *backLen = fifoLevel;
    }
  } else {
    // We found an error, print out the bits
    status = MFRC522_ERR;
    printf("[ERR] MFRC522_to_card() => ErrorReg reported errors:\n");
    print_mfrc522_error_bits(errorVal);
  }

  return status;
}

/**
 * MFRC522_init
 * Initializes the MFRC522 reader (SPI config, reset pin, etc.)
 */
void MFRC522_init(MFRC522_t *dev, spi_inst_t *spi_port, uint cs_pin,
                  uint rst_pin, uint sck_pin, uint mosi_pin, uint miso_pin,
                  uint baudrate) {
  dev->spi_port = spi_port;
  dev->cs_pin = cs_pin;
  dev->rst_pin = rst_pin;
  dev->baudrate = baudrate;

  // Initialize chosen SPI port at given baud rate
  spi_init(dev->spi_port, dev->baudrate);

  // Make sure we set SPI to Mode 0, MSB first (Arduino default)
  spi_set_format(dev->spi_port, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  // Set the SPI pins
  gpio_set_function(sck_pin, GPIO_FUNC_SPI);
  gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
  gpio_set_function(miso_pin, GPIO_FUNC_SPI);

  // Chip select
  gpio_init(dev->cs_pin);
  gpio_set_dir(dev->cs_pin, GPIO_OUT);
  gpio_put(dev->cs_pin, 1);

  // Reset pin
  gpio_init(dev->rst_pin);
  gpio_set_dir(dev->rst_pin, GPIO_OUT);
  gpio_put(dev->rst_pin, 0); // Start as low

  sleep_ms(50);
  gpio_put(dev->rst_pin, 1); // Release reset
  sleep_ms(50);

  MFRC522_wake(dev);
  uint8_t version = MFRC522_read_register(dev, VersionReg);
  printf("MFRC522 Version: 0x%02X\n", version);
}

void MFRC522_wake(MFRC522_t *dev) {
  // Some default configuration (like the python init)
  MFRC522_reset(dev);

  // Timer: TPrescalerReg – prescaler value for the internal timer
  // TModeReg: TAuto=1; f(Timer) = 6.78MHz/TPreScaler
  MFRC522_write_register(dev, TModeReg, 0x8D);
  MFRC522_write_register(dev, TPrescalerReg, 0x3E);
  // Timer reload
  MFRC522_write_register(dev, TReloadRegL, 30);
  MFRC522_write_register(dev, TReloadRegH, 0);

  // 100%ASK
  MFRC522_write_register(dev, TxASKReg, 0x40);
  MFRC522_write_register(dev, ModeReg, 0x3D); // CRC initial value 0x6363

  // Turn on the antenna
  MFRC522_antenna_on(dev, true);
}

/**
 * Reset command
 */
void MFRC522_reset(MFRC522_t *dev) {
  MFRC522_write_register(dev, CommandReg, PCD_RESETPHASE);
}

/**
 * Turn on/off the antenna driver pins
 */
void MFRC522_antenna_on(MFRC522_t *dev, bool on) {
  uint8_t temp = MFRC522_read_register(dev, TxControlReg);
  if (on) {
    if (!(temp & 0x03)) {
      MFRC522_set_bits(dev, TxControlReg, 0x03);
    }
  } else {
    MFRC522_clear_bits(dev, TxControlReg, 0x03);
  }
}

/**
 * Request a tag (REQA or REQIDL).
 */
uint8_t MFRC522_request(MFRC522_t *dev, uint8_t mode, uint8_t *outBits) {
  // Setup bit framing
  MFRC522_write_register(dev, BitFramingReg, 0x07);

  uint8_t backData[16];
  size_t backLen = sizeof(backData);
  uint8_t validBits = 0;

  uint8_t cmdBuffer[1] = {mode};
  uint8_t status = MFRC522_to_card(dev, PCD_TRANSCEIVE, cmdBuffer, 1, backData,
                                   &backLen, &validBits);
  if ((status != MFRC522_OK) || (validBits != 0x10)) {
    /*printf("[ERR] MFRC522_request() failed. validBits=0x%02X\n", validBits);*/
    status = MFRC522_ERR;
  }
  if (outBits) {
    *outBits = validBits;
  }
  return status;
}

/**
 * Anti-collision for a given anticollision command (0x93, 0x95, 0x97).
 * `serialOut` must have space for 5 bytes if successful.
 */
uint8_t MFRC522_anticoll(MFRC522_t *dev, uint8_t antiCollVal,
                         uint8_t *serialOut) {
  uint8_t serChk = 0;
  uint8_t cmdBuffer[2] = {antiCollVal, 0x20};

  // Clear the BitFramingReg
  MFRC522_write_register(dev, BitFramingReg, 0x00);

  // The response should be 5 bytes: 4 uid bytes + 1 BCC
  uint8_t backData[10] = {0};
  size_t backLen = sizeof(backData);
  uint8_t validBits = 0;

  uint8_t status = MFRC522_to_card(dev, PCD_TRANSCEIVE, cmdBuffer, 2, backData,
                                   &backLen, &validBits);

  if (status == MFRC522_OK) {
    if (backLen == 5) {
      // XOR check
      for (int i = 0; i < 4; i++) {
        serChk ^= backData[i];
      }
      if (serChk != backData[4]) {
        printf(
            "[ERR] anticoll: XOR check failed. (serChk=0x%02X, BCC=0x%02X)\n",
            serChk, backData[4]);
        status = MFRC522_ERR;
      } else {
        // Copy out the full 5 bytes
        memcpy(serialOut, backData, 5);
      }
    } else {
      printf("[ERR] anticoll: Expected 5 bytes, got %zu\n", backLen);
      status = MFRC522_ERR;
    }
  }
  return status;
}

/**
 * MFRC522_select_tag
 * Uses the 'Select' command to finalize the UID selection on the MFRC522
 */
uint8_t MFRC522_select_tag(MFRC522_t *dev, const uint8_t *uid,
                           uint8_t uid_size) {
  // uid_size typically 5 for classic tags (4 + BCC)
  // Construct command buffer [ anticoll cmd(0x93/95/97), 0x70, uid, CRC16 ]
  uint8_t cmdBuffer[10];
  cmdBuffer[0] =
      PICC_ANTICOLL1; // Typically 0x93 if we are selecting from anticoll1
  cmdBuffer[1] = 0x70;

  // Copy in the UID (4 or 5 bytes?), ignoring the final BCC if present
  uint8_t copyLen = (uid_size > 5) ? 5 : uid_size;
  for (int i = 0; i < copyLen; i++) {
    cmdBuffer[2 + i] = uid[i];
  }
  size_t dataLen = 2 + copyLen;

  // Append CRC
  uint8_t crcResult[2];
  MFRC522_calculate_crc(dev, cmdBuffer, dataLen, crcResult);
  cmdBuffer[dataLen++] = crcResult[0];
  cmdBuffer[dataLen++] = crcResult[1];

  // Prepare response
  uint8_t backData[3];
  size_t backLen = sizeof(backData);
  uint8_t validBits = 0;

  uint8_t status = MFRC522_to_card(dev, PCD_TRANSCEIVE, cmdBuffer, dataLen,
                                   backData, &backLen, &validBits);
  if ((status == MFRC522_OK) && (backLen == 3) && (validBits == 0x18)) {
    return MFRC522_OK;
  }
  printf("[ERR] select_tag: Could not select UID. status=%d, backLen=%zu, "
         "validBits=0x%02X\n",
         status, backLen, validBits);
  return MFRC522_ERR;
}
/**
 * Auth with a specific block address, key A or B
 */
uint8_t MFRC522_auth(MFRC522_t *dev, uint8_t authMode, uint8_t blockAddr,
                     const uint8_t *sectorKey, const uint8_t *uid) {
  // Typically you pass 6-byte key, plus first 4 bytes of the UID
  // Build command
  uint8_t packet[12];
  packet[0] = authMode;
  packet[1] = blockAddr;
  for (int i = 0; i < 6; i++) {
    packet[2 + i] = sectorKey[i];
  }
  // Use only first 4 bytes of UID for authentication
  for (int i = 0; i < 4; i++) {
    packet[8 + i] = uid[i];
  }

  // Perform the command
  uint8_t backData[4];
  size_t backLen = sizeof(backData);
  uint8_t validBits = 0;

  uint8_t status = MFRC522_to_card(dev, PCD_AUTHENT, packet, sizeof(packet),
                                   backData, &backLen, &validBits);
  // Status is either OK or ERR
  return status;
}

/**
 * Stop encryption (stop_crypto1)
 */
void MFRC522_stop_crypto1(MFRC522_t *dev) {
  MFRC522_clear_bits(dev, Status2Reg, 0x08);
}

/**
 * Read a block (16 bytes) from card into recvData.
 */
uint8_t MFRC522_read(MFRC522_t *dev, uint8_t blockAddr, uint8_t *recvData) {
  uint8_t cmdBuffer[4];
  cmdBuffer[0] = 0x30; // MIFARE Read command
  cmdBuffer[1] = blockAddr;
  // Append CRC
  MFRC522_calculate_crc(dev, cmdBuffer, 2, &cmdBuffer[2]);

  uint8_t backData[18];
  size_t backLen = sizeof(backData);
  uint8_t validBits = 0;

  uint8_t status = MFRC522_to_card(dev, PCD_TRANSCEIVE, cmdBuffer, 4, backData,
                                   &backLen, &validBits);
  if ((status == MFRC522_OK) && (backLen == 16)) {
    // Copy to user buffer
    memcpy(recvData, backData, 16);
  } else {
    status = MFRC522_ERR;
  }
  return status;
}

/**
 * Write 16 bytes to a block.
 */
uint8_t MFRC522_write(MFRC522_t *dev, uint8_t blockAddr,
                      const uint8_t *writeData) {
  // Step 1: send the MIFARE Write command [ 0xA0, blockAddr, CRC ]
  uint8_t cmdBuffer[4];
  cmdBuffer[0] = 0xA0;
  cmdBuffer[1] = blockAddr;
  MFRC522_calculate_crc(dev, cmdBuffer, 2, &cmdBuffer[2]);

  uint8_t backData[2];
  size_t backLen = sizeof(backData);
  uint8_t validBits = 0;

  uint8_t status = MFRC522_to_card(dev, PCD_TRANSCEIVE, cmdBuffer, 4, backData,
                                   &backLen, &validBits);

  if ((status != MFRC522_OK) || (backLen != 1) ||
      ((backData[0] & 0x0F) != 0x0A)) {
    return MFRC522_ERR;
  }

  // Step 2: send the 16 data bytes + 2 CRC
  uint8_t buf[18];
  memcpy(buf, writeData, 16);
  MFRC522_calculate_crc(dev, buf, 16, &buf[16]);

  backLen = sizeof(backData);
  status = MFRC522_to_card(dev, PCD_TRANSCEIVE, buf, 18, backData, &backLen,
                           &validBits);
  if ((status != MFRC522_OK) || (backLen != 1) ||
      ((backData[0] & 0x0F) != 0x0A)) {
    return MFRC522_ERR;
  }
  return MFRC522_OK;
}

/**
 * read_sector_block
 * Auth, then read 16 bytes from sector/block into outData.
 */
uint8_t MFRC522_read_sector_block(MFRC522_t *dev, const uint8_t *uid,
                                  uint8_t sector, uint8_t block,
                                  const uint8_t *keyA, const uint8_t *keyB,
                                  uint8_t *outData) {
  uint8_t absoluteBlock = sector * 4 + (block % 4);
  if (absoluteBlock > 63) {
    return MFRC522_ERR;
  }

  uint8_t status = MFRC522_ERR;
  if (keyA != NULL) {
    // Auth with key A
    status = MFRC522_auth(dev, PICC_AUTHENT1A, absoluteBlock, keyA, uid);
  } else if (keyB != NULL) {
    // Auth with key B
    status = MFRC522_auth(dev, PICC_AUTHENT1B, absoluteBlock, keyB, uid);
  }
  if (status != MFRC522_OK) {
    return MFRC522_ERR;
  }
  // Read the block
  return MFRC522_read(dev, absoluteBlock, outData);
}

/**
 * write_sector_block
 * Auth, then write 16 bytes to a sector/block.
 */
uint8_t MFRC522_write_sector_block(MFRC522_t *dev, const uint8_t *uid,
                                   uint8_t sector, uint8_t block,
                                   const uint8_t *data16, const uint8_t *keyA,
                                   const uint8_t *keyB) {
  uint8_t absoluteBlock = sector * 4 + (block % 4);
  if (absoluteBlock > 63) {
    return MFRC522_ERR;
  }
  // Must be exactly 16 bytes
  if (!data16) {
    return MFRC522_ERR;
  }

  uint8_t status = MFRC522_ERR;
  if (keyA != NULL) {
    // Auth with key A
    status = MFRC522_auth(dev, PICC_AUTHENT1A, absoluteBlock, keyA, uid);
  } else if (keyB != NULL) {
    // Auth with key B
    status = MFRC522_auth(dev, PICC_AUTHENT1B, absoluteBlock, keyB, uid);
  }
  if (status != MFRC522_OK) {
    return MFRC522_ERR;
  }

  return MFRC522_write(dev, absoluteBlock, data16);
}
