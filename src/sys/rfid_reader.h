#ifndef RFID_READER_H
#define RFID_READER_H

#include <stdint.h>

struct rfid_reader {
  uint8_t uid_len;
  uint8_t key_a[6];
};

void rfid_reader_init(struct rfid_reader *reader);
int rfid_reader_wait_for_card(struct rfid_reader *reader, int timeout_ms);
int rfid_reader_read(struct rfid_reader *reader, char *uid);

#endif // RFID_READER_H
