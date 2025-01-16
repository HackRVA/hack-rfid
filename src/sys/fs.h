
#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#define WITH_FS 0
#ifdef __PICO_BUILD__
/*#include <lfs.h>*/

#else // Linux
#include <fcntl.h>

#endif

#ifdef __PICO_BUILD__
typedef struct {
#if WITH_FS
  lfs_file_t *handle;
#endif
} File;

#else
typedef struct {
  int fd;
} File;
#endif

void fs_init(void);
void fs_unmount(void);
File fs_open(const char *path, int flags);
int fs_close(File file);
size_t fs_read_data(File file, void *ptr, size_t size);
size_t fs_write_data(File file, const void *ptr, size_t size);
void fs_print_contents(void);

#endif // FS_H
