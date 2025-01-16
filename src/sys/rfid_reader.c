#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "device/mfrc522.h"
#include "rfid_reader.h"

#define MFRC522_SPI_PORT spi0
#define PIN_MISO 4
#define PIN_MOSI 3
#define PIN_SCK 2
#define PIN_CS 1
#define PIN_RST 0

MFRC522_t rfid;

void rfid_reader_init(struct rfid_reader *reader) {
  if (!reader) {
    fprintf(stderr, "Error: rfid_reader pointer is NULL\n");
    return;
  }

  MFRC522_init(&rfid, MFRC522_SPI_PORT, PIN_CS, PIN_RST, PIN_SCK, PIN_MOSI,
               PIN_MISO, 1000 * 1000);

  memset(reader->key_a, 0xFF, sizeof(reader->key_a));

  printf("RFID reader initialized.\n");
}

int rfid_reader_wait_for_card(struct rfid_reader *reader, int timeout_ms) {
  if (!reader) {
    fprintf(stderr, "Error: rfid_reader pointer is NULL\n");
    return -1;
  }

  uint8_t status;
  uint8_t outBits;
  int elapsed_time = 0;

  while (elapsed_time < timeout_ms) {
    MFRC522_wake(&rfid);
    status = MFRC522_request(&rfid, PICC_REQIDL, &outBits);

    if (status == MFRC522_OK) {
      /*printf("Card detected!\n");*/
      return 0;
    }

    sleep_ms(100);
    elapsed_time += 100;
  }

  /*printf("No card detected within timeout.\n");*/
  return -1;
}

int rfid_reader_read(struct rfid_reader *reader, char *uid) {
  if (!reader || !uid) {
    fprintf(stderr, "Error: Invalid arguments to rfid_reader_read\n");
    return -1;
  }

  uint8_t status;
  uint8_t serial[5] = {0};

  status = MFRC522_anticoll(&rfid, PICC_ANTICOLL1, serial);
  if (status == MFRC522_OK) {
    printf("UID: ");
    for (int i = 0; i < 5; i++) {
      printf("%02X ", serial[i]);
    }
    printf("\n");

    for (int i = 0; i < 5; i++) {
      sprintf(&uid[i * 2], "%02x", serial[i]);
    }
    uid[10] = '\0';

    reader->uid_len = 5;

    MFRC522_stop_crypto1(&rfid);
    return 0;
  }

  fprintf(stderr, "Error reading card.\n");
  return -1;
}
