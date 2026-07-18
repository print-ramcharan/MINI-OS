#include "ramfs.h"
#include "vga.h"

static ramfs_file_t files[MAX_FILES];

static int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static void strcpy(char *dst, const char *src) {
  while ((*dst++ = *src++))
    ;
}

void ramfs_init(void) {
  for (int i = 0; i < MAX_FILES; i++) {
    files[i].used = 0;
    files[i].name[0] = '\0';
    files[i].content[0] = '\0';
    files[i].size = 0;
  }
}

int ramfs_create(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && strcmp(files[i].name, name) == 0) {
      return -1;
    }
  }
  for (int i = 0; i < MAX_FILES; i++) {
    if (!files[i].used) {
      strcpy(files[i].name, name);
      files[i].used = 1;
      files[i].content[0] = '\0';
      files[i].size = 0;
      return 0;
    }
  }
  return -2;
}

int ramfs_write(const char *name, const char *content) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && strcmp(files[i].name, name) == 0) {
      int len = 0;
      while (content[len] && len < MAX_FILE_SIZE - 1) {
        files[i].content[len] = content[len];
        len++;
      }
      files[i].content[len] = '\0';
      files[i].size = len;
      return 0;
    }
  }
  return -1;
}

const char *ramfs_read(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && strcmp(files[i].name, name) == 0) {
      return files[i].content;
    }
  }
  return (void*)0;
}

void ramfs_list(void) {
  int count = 0;
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used) {
      print("  ");
      print(files[i].name);
      print(" \t[");
      print_dec(files[i].size);
      print(" bytes]\n");
      count++;
    }
  }
  if (count == 0) {
    print("  (empty directory)\n");
  }
}

int ramfs_delete(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && strcmp(files[i].name, name) == 0) {
      files[i].used = 0;
      files[i].name[0] = '\0';
      files[i].content[0] = '\0';
      files[i].size = 0;
      return 0;
    }
  }
  return -1; // Not found
}

int ramfs_copy(const char *src, const char *dest) {
  int src_idx = -1;
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && strcmp(files[i].name, src) == 0) {
      src_idx = i;
      break;
    }
  }
  if (src_idx == -1) return -1; // Source not found

  // Create destination
  int res = ramfs_create(dest);
  if (res == -1) return -3; // Destination exists
  if (res == -2) return -2; // Full

  // Write contents
  return ramfs_write(dest, files[src_idx].content);
}
