#ifndef RAMFS_H
#define RAMFS_H

#include <stdint.h>
#include <stddef.h>

#define MAX_FILES 16
#define MAX_FILENAME_LEN 32
#define MAX_FILE_SIZE 256
#define MAX_PROCESS_OPEN_FILES 8

typedef struct {
  char name[MAX_FILENAME_LEN];
  char content[MAX_FILE_SIZE];
  uint32_t size;
  uint8_t used;
} ramfs_file_t;

typedef struct {
  char filename[MAX_FILENAME_LEN];
  uint32_t offset;
  uint8_t used;
} open_file_t;

void ramfs_init(void);
int ramfs_create(const char *name);
int ramfs_write(const char *name, const char *content);
const char *ramfs_read(const char *name);
void ramfs_list(void);
int ramfs_delete(const char *name);
int ramfs_copy(const char *src, const char *dest);
int ramfs_rename(const char *src, const char *dest);

#endif
