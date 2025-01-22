#include <hardware/flash.h>
#include <hardware/sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"

#if WITH_FS
#include <lfs.h>

extern const struct lfs_config lfs_pico_flash_config;
static lfs_t lfs;

void fs_init(void) {
	int err = lfs_mount(&lfs, &lfs_pico_flash_config);
	if (err) {
		lfs_format(&lfs, &lfs_pico_flash_config);
		lfs_mount(&lfs, &lfs_pico_flash_config);
	}
}

void fs_unmount(void) {
	lfs_unmount(&lfs);
}

File fs_open(const char *path, int flags) {
	File file = {0};
	/*lfs_file_open(&lfs, (lfs_file_t *)file.handle, path, flags);*/
	return file;
}

int fs_close(File file) {
	return lfs_file_close(&lfs, (lfs_file_t *)file.handle);
}

static size_t fs_read_acl(void *ptr, size_t size) {
	lfs_file_t f;

	int err = lfs_file_open(&lfs, &f, "acl", LFS_O_RDONLY);
	if (err < 0) {
		printf("[fs_read_acl] 'acl' file not found. Creating a new "
		       "file.\n");

		err = lfs_file_open(&lfs, &f, "acl",
			LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
		if (err < 0) {
			printf("[fs_read_acl] Failed to create 'acl' file, "
			       "err=%d\n",
				err);
			return 0;
		}

		lfs_file_close(&lfs, &f);
		return 0;
	}

	lfs_ssize_t br = lfs_file_read(&lfs, &f, ptr, size);
	if (br < 0) {
		printf("[fs_read_acl] Error reading from 'acl', err=%ld\n",
			(long)br);
		lfs_file_close(&lfs, &f);
		return 0;
	}

	lfs_file_close(&lfs, &f);

	return (size_t)br;
}

size_t fs_read_data(File file, void *ptr, size_t size) {
	char buffer[128];
	size_t bytes_read = fs_read_acl(buffer, sizeof(buffer));
	if (bytes_read > 0) {
		printf("Read %zu bytes from 'acl': %s\n", bytes_read, buffer);
	} else {
		printf("Failed to read from 'acl'.\n");
	}

	return (size_t)bytes_read;
}

size_t fs_write_data(File file, const void *ptr, size_t size) {
	lfs_file_t f;
	lfs_file_open(
		&lfs, &f, "acl", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
	lfs_ssize_t bw =
		lfs_file_write(&lfs, (lfs_file_t *)file.handle, ptr, size);
	lfs_file_close(&lfs, &f);

	if (bw < 0) {
		printf("[fs_write_data] Error writing, err=%ld\n", (long)bw);
		return 0;
	}
	return (size_t)bw;
}

void fs_print_contents(void) {
	lfs_dir_t dir;
	int err = lfs_dir_open(&lfs, &dir, "/");
	if (err < 0) {
		printf("[fs_print_contents] Error opening root dir: %d\n", err);
		return;
	}

	while (true) {
		struct lfs_info info;
		int res = lfs_dir_read(&lfs, &dir, &info);
		if (res < 0) {
			printf("[fs_print_contents] Error reading dir entry: "
			       "%d\n",
				res);
			break;
		}
		if (res == 0) {
			break;
		}

		if (strcmp(info.name, ".") == 0
			|| strcmp(info.name, "..") == 0) {
			continue;
		}

		if (info.type == LFS_TYPE_DIR) {
			printf("[Dir ] %s\n", info.name);
		} else if (info.type == LFS_TYPE_REG) {
			printf("[File] %s (size=%lu)\n", info.name,
				(unsigned long)info.size);
		} else {
			printf("[??? ] %s\n", info.name);
		}
	}

	lfs_dir_close(&lfs, &dir);
}
#endif
