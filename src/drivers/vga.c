#include "vga.h"
#include "ramfs.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t *terminal_buffer;

int redirect_active = 0;
char redirect_filename[32] = "";
int redirect_append = 0;

void terminal_initialize(void) {
  terminal_row = 0;
  terminal_column = 0;
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  terminal_buffer = (uint16_t *)0xB8000;
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
  }
}

void terminal_setcolor(uint8_t color) { terminal_color = color; }

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
  const size_t index = y * VGA_WIDTH + x;
  terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
  if (c == '\n') {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT)
      terminal_row =
          0; // NOTE: Simple wrap around, scrolling comes later if needed
    return;
  }

  if (c == '\b') {
    if (terminal_column > 0) {
      terminal_column--;
      terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    }
    return;
  }

  terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
  if (++terminal_column == VGA_WIDTH) {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT)
      terminal_row = 0;
  }
}

void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++)
    terminal_putchar(data[i]);
}

size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len])
    len++;
  return len;
}

void terminal_writestring(const char *data) {
  terminal_write(data, strlen(data));
}

void print(const char *str) {
  if (redirect_active && redirect_filename[0] != '\0') {
    char new_content[256];
    const char *old = ramfs_read(redirect_filename);
    int len = 0;
    if (old) {
      while (old[len] && len < 255) {
        new_content[len] = old[len];
        len++;
      }
    }
    int k = 0;
    while (str[k] && len < 255) {
      new_content[len++] = str[k++];
    }
    new_content[len] = '\0';
    ramfs_write(redirect_filename, new_content);
  } else {
    terminal_writestring(str);
  }
}

void print_hex(uint32_t val) {
  print("0x");
  if (val == 0) {
    print("0");
    return;
  }
  char buffer[10];
  int i = 0;
  while (val > 0) {
    uint8_t rem = val % 16;
    if (rem < 10)
      buffer[i++] = '0' + rem;
    else
      buffer[i++] = 'A' + (rem - 10);
    val /= 16;
  }
  // Reverse
  for (int j = 0; j < i / 2; j++) {
    char temp = buffer[j];
    buffer[j] = buffer[i - 1 - j];
    buffer[i - 1 - j] = temp;
  }
  buffer[i] = '\0';
  print(buffer);
}

void print_dec(uint32_t val) {
  // Routed via print() to support shell output redirection automatically
  if (val == 0) {
    print("0");
    return;
  }
  char buffer[12];
  int i = 0;
  while (val > 0) {
    buffer[i++] = '0' + (val % 10);
    val /= 10;
  }
  for (int j = 0; j < i / 2; j++) {
    char temp = buffer[j];
    buffer[j] = buffer[i - 1 - j];
    buffer[i - 1 - j] = temp;
  }
  buffer[i] = '\0';
  print(buffer);
}

static int vga_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int vga_set_theme(const char *name) {
  uint8_t color;
  if (vga_strcmp(name, "matrix") == 0) {
    color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
  } else if (vga_strcmp(name, "cyber") == 0) {
    color = vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE);
  } else if (vga_strcmp(name, "amber") == 0) {
    color = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
  } else if (vga_strcmp(name, "ocean") == 0) {
    color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
  } else if (vga_strcmp(name, "monochrome") == 0) {
    color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
  } else if (vga_strcmp(name, "default") == 0) {
    color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  } else {
    return -1;
  }

  terminal_setcolor(color);
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      char c = (char)(terminal_buffer[index] & 0xFF);
      terminal_buffer[index] = vga_entry(c, color);
    }
  }
  return 0;
}
