
#ifndef MFRC522_H
#define MFRC522_H

#include "hardware/spi.h"
#include <stdbool.h>
#include <stdint.h>

#define MFRC522_OK 0
#define MFRC522_NOTAGERR 1
#define MFRC522_ERR 2

#define PCD_IDLE 0x00
#define PCD_AUTHENT 0x0E
#define PCD_RECEIVE 0x08
#define PCD_TRANSMIT 0x04
#define PCD_TRANSCEIVE 0x0C
#define PCD_RESETPHASE 0x0F
#define PCD_CALCCRC 0x03

#define PICC_REQIDL 0x26
#define PICC_REQALL 0x52
#define PICC_ANTICOLL1 0x93
#define PICC_ANTICOLL2 0x95
#define PICC_ANTICOLL3 0x97
#define PICC_AUTHENT1A 0x60
#define PICC_AUTHENT1B 0x61

#define CommandReg 0x01
#define CommIEnReg 0x02
#define CommIrqReg 0x04
#define ErrorReg 0x06
#define Status1Reg 0x07
#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define ControlReg 0x0C
#define BitFramingReg 0x0D
#define CollReg 0x0E
#define ModeReg 0x11
#define TxModeReg 0x12
#define RxModeReg 0x13
#define TxControlReg 0x14
#define TxASKReg 0x15
#define TModeReg 0x2A
#define TPrescalerReg 0x2B
#define TReloadRegH 0x2C
#define TReloadRegL 0x2D
#define VersionReg 0x37
#define CRCResultRegL 0x21
#define CRCResultRegH 0x22

#define DivIrqReg        0x05
#define Status2Reg       0x08

typedef struct {
  spi_inst_t *spi_port;
  uint cs_pin;
  uint rst_pin;
  uint baudrate;
} MFRC522_t;

void MFRC522_init(MFRC522_t *dev, spi_inst_t *spi_port, uint cs_pin,
                  uint rst_pin, uint sck_pin, uint mosi_pin, uint miso_pin,
                  uint baudrate);

void MFRC522_wake(MFRC522_t *dev);
void MFRC522_reset(MFRC522_t *dev);

void MFRC522_antenna_on(MFRC522_t *dev, bool on);

uint8_t MFRC522_request(MFRC522_t *dev, uint8_t mode, uint8_t *outBits);

uint8_t MFRC522_anticoll(MFRC522_t *dev, uint8_t antiCollVal,
                         uint8_t *serialOut);

uint8_t MFRC522_select_tag(MFRC522_t *dev, const uint8_t *uid,
                           uint8_t uid_size);

uint8_t MFRC522_auth(MFRC522_t *dev, uint8_t authMode, uint8_t blockAddr,
                     const uint8_t *sectorKey, const uint8_t *uid);

uint8_t MFRC522_read(MFRC522_t *dev, uint8_t blockAddr, uint8_t *recvData);

uint8_t MFRC522_write(MFRC522_t *dev, uint8_t blockAddr,
                      const uint8_t *writeData);

void MFRC522_stop_crypto1(MFRC522_t *dev);

uint8_t MFRC522_read_sector_block(MFRC522_t *dev, const uint8_t *uid,
                                  uint8_t sector, uint8_t block,
                                  const uint8_t *keyA, const uint8_t *keyB,
                                  uint8_t *outData);

uint8_t MFRC522_write_sector_block(MFRC522_t *dev, const uint8_t *uid,
                                   uint8_t sector, uint8_t block,
                                   const uint8_t *data16, const uint8_t *keyA,
                                   const uint8_t *keyB);

#endif
