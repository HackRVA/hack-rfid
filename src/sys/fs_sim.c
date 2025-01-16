

#include "fs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void fs_init(void) {}

void fs_unmount(void) {}

File fs_open(const char *path, int flags) {
  File file = {.fd = -1};

  int fd = open(path, flags, 0666);
  if (fd < 0) {
    perror("[fs_open] open failed");
    return file;
  }
  file.fd = fd;
  return file;
}

int fs_close(File file) {
  if (file.fd >= 0) {
    if (close(file.fd) < 0) {
      perror("[fs_close] close failed");
      return -1;
    }
  }
  return 0;
}

size_t fs_read_data(File file, void *ptr, size_t size) {
  if (file.fd < 0) {
    fprintf(stderr, "[fs_read_data] Invalid fd\n");
    return 0;
  }
  ssize_t br = read(file.fd, ptr, size);
  if (br < 0) {
    perror("[fs_read_data] read failed");
    return 0;
  }
  return (size_t)br;
}

size_t fs_write_data(File file, const void *ptr, size_t size) {
  if (file.fd < 0) {
    fprintf(stderr, "[fs_write_data] Invalid fd\n");
    return 0;
  }
  ssize_t bw = write(file.fd, ptr, size);
  if (bw < 0) {
    perror("[fs_write_data] write failed");
    return 0;
  }
  return (size_t)bw;
}
void fs_print_contents(void) {}
