#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "acl.h"

#include "fs.h"

#if WITH_FS
static size_t fs_read_line(File file, char *buffer, size_t max_len) {
  size_t pos = 0;
  while (pos < (max_len - 1)) {
    char c;
    size_t bytes_read = fs_read_data(file, &c, 1);

    if (bytes_read == 0) {
      // EOF reached
      break;
    }

    if (c == '\n') {
      break;
    }

    buffer[pos++] = c;
  }

  buffer[pos] = '\0';
  return pos;
}
#endif

void acl_load(struct access_control_list *acl, const char *file_path) {
#if WITH_FS
  acl->file_path = file_path;
  acl->user_count = 0;

  File file = fs_open(file_path, FS_O_RDONLY);

  char line[USER_MAX_LENGTH];
  while (true) {
    size_t len = fs_read_line(file, line, sizeof(line));
    if (len == 0) {
      break;
    }

    if (len > 0 && line[len - 1] == '\r') {
      line[len - 1] = '\0';
    }

    if (acl->user_count < MAX_USERS) {
      strncpy(acl->users[acl->user_count], line, USER_MAX_LENGTH - 1);
      acl->users[acl->user_count][USER_MAX_LENGTH - 1] = '\0';
      acl->user_count++;
    } else {
      fprintf(stderr, "[ACL] Maximum user limit reached.\n");
      break;
    }
  }

  fs_close(file);
#endif
}

void acl_save(struct access_control_list *acl) {
#if WITH_FS
  File file = fs_open(acl->file_path, FS_O_WRONLY | FS_O_CREAT | FS_O_TRUNC);
#ifdef __PICO_BUILD__
  if (!file.handle) {
#else
  if (file.fd < 0) {
#endif
    fprintf(stderr, "[ACL] Failed to open file '%s' for writing.\n",
            acl->file_path);
    return;
  }

  for (size_t i = 0; i < acl->user_count; i++) {
    size_t len = strnlen(acl->users[i], USER_MAX_LENGTH);

    if (len > 0) {
      if (fs_write_data(file, acl->users[i], len) != len) {
        fprintf(stderr, "[acl_save] Write error at user %zu\n", i);
        break;
      }
    }

    if (fs_write_data(file, "\n", 1) != 1) {
      fprintf(stderr, "[acl_save] Failed to write newline for user %zu\n", i);
      break;
    }
  }

  fs_close(file);
#endif
}

bool acl_has_user(struct access_control_list *acl, const char *user) {
  for (size_t i = 0; i < acl->user_count; i++) {
    /*
     * we may need additional logic here because of the way the previous rfid
     * reader handled bytes that started with 0 (hex notation) in the UID
     * */
    if (strcmp(acl->users[i], user) == 0) {
      return true;
    }
  }
  return false;
}

#define rotl1(x) (((x) << 1) | ((x) >> 31))
char sorted_users[MAX_USERS][USER_MAX_LENGTH];

int partition(char users[][USER_MAX_LENGTH], int low, int high) {
  char pivot[USER_MAX_LENGTH];
  strncpy(pivot, users[high], USER_MAX_LENGTH);

  int i = low - 1;
  for (int j = low; j < high; j++) {
    if (strcmp(users[j], pivot) <= 0) {
      i++;
      char temp[USER_MAX_LENGTH];
      strncpy(temp, users[i], USER_MAX_LENGTH);
      strncpy(users[i], users[j], USER_MAX_LENGTH);
      strncpy(users[j], temp, USER_MAX_LENGTH);
    }
  }

  char temp[USER_MAX_LENGTH];
  strncpy(temp, users[i + 1], USER_MAX_LENGTH);
  strncpy(users[i + 1], users[high], USER_MAX_LENGTH);
  strncpy(users[high], temp, USER_MAX_LENGTH);

  return i + 1;
}

void quick_sort_users(char users[][USER_MAX_LENGTH], int low, int high) {
  if (low < high) {
    int pivot_index = partition(users, low, high);
    quick_sort_users(users, low, pivot_index - 1);
    quick_sort_users(users, pivot_index + 1, high);
  }
}

void quick_sort_users_in_place(struct access_control_list *acl) {
  if (!acl || acl->user_count <= 1) {
    return;
  }
  quick_sort_users(acl->users, 0, acl->user_count - 1);
}

uint32_t acl_hash(struct access_control_list *acl) {
  quick_sort_users_in_place(acl);

  uint32_t hash = 5381;
  for (size_t i = 0; i < acl->user_count; i++) {
    const char *str = acl->users[i];
    while (*str) {
      hash = ((hash << 5) + hash) + (unsigned char)(*str);
      str++;
    }
    hash = ((hash << 5) + hash) + 0xFE;
  }
  return hash;
}

static uint32_t hash_string(const char *str) {
  uint32_t hash = 5381;
  while (*str) {
    hash = ((hash << 5) + hash) + (unsigned char)(*str);
    str++;
  }
  return hash;
}

void acl_print(struct access_control_list *acl) {
  printf("[ACL] Contents of Access Control List:\n");
  for (size_t i = 0; i < acl->user_count; i++) {
    printf("  User %zu: %s\n", i + 1, acl->users[i]);
  }
  printf("[ACL] Total Users: %zu\n", acl->user_count);
}

void acl_append_user(struct access_control_list *acl, const char *user) {
  if (acl->user_count >= MAX_USERS) {
    fprintf(stderr, "[ACL] Cannot append user. Maximum user limit reached.\n");
    return;
  }

  if (acl_has_user(acl, user)) {
    printf("[ACL] User '%s' already exists in the list.\n", user);
    return;
  }

  strncpy(acl->users[acl->user_count], user, USER_MAX_LENGTH - 1);
  acl->users[acl->user_count][USER_MAX_LENGTH - 1] = '\0';
  acl->user_count++;
}

void acl_remove_user(struct access_control_list *acl, const char *user) {
  for (size_t i = 0; i < acl->user_count; i++) {
    if (strcmp(acl->users[i], user) == 0) {
      // Shift everything after i up one
      for (size_t j = i; j < acl->user_count - 1; j++) {
        strncpy(acl->users[j], acl->users[j + 1], USER_MAX_LENGTH);
      }
      acl->user_count--;
      printf("[ACL] User '%s' removed successfully.\n", user);
      return;
    }
  }
  printf("[ACL] User '%s' not found in the list.\n", user);
}
