#include "editor.h"
#include "vga.h"
#include "syscall.h"
#include "ramfs.h"
#include <stdint.h>

char editor_target_file[32];

static void draw_ui(void) {
  terminal_initialize();
  
  // Header
  terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
  for (int i = 0; i < 80; i++) {
    terminal_putentryat(' ', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), i, 0);
  }
  
  // Print header title
  print(" MiniOS Text Editor: ");
  print(editor_target_file);
  
  // Footer at row 24
  for (int i = 0; i < 80; i++) {
    terminal_putentryat(' ', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), i, 24);
  }
  // Print footer text
  // Row 24 cursor placement
  extern size_t terminal_row;
  extern size_t terminal_column;
  terminal_row = 24;
  terminal_column = 0;
  print(" [ESC] Exit  |  [F1] Save & Exit");
  
  // Reset text color for body
  terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
  terminal_row = 2; // start typing at row 2
  terminal_column = 0;
}

static void shell_sleep(uint32_t ticks) {
  asm volatile("mov $2, %%eax; mov %0, %%ebx; int $0x80"
               :
               : "r"(ticks)
               : "eax", "ebx");
}

static char shell_read_char(void) {
  uint32_t val;
  asm volatile("mov $5, %%eax; int $0x80; mov %%eax, %0"
               : "=r"(val)
               :
               : "eax");
  return (char)val;
}

static void editor_exit(void) {
  asm volatile("mov $3, %%eax; int $0x80" ::: "eax");
}

#define EDITOR_BUFFER_SIZE 256
static char editor_buf[EDITOR_BUFFER_SIZE];
static int editor_len = 0;

void editor_task(void) {
  draw_ui();
  
  editor_len = 0;
  
  // Try loading file content if it exists
  const char *existing_content = ramfs_read(editor_target_file);
  if (existing_content) {
    int i = 0;
    while (existing_content[i] && i < EDITOR_BUFFER_SIZE - 1) {
      char c = existing_content[i++];
      editor_buf[editor_len++] = c;
      terminal_putchar(c);
    }
  }

  while (1) {
    char c = shell_read_char();
    if (c != 0) {
      if (c == 27) { // ESC
        terminal_initialize();
        editor_exit();
      } else if (c == 19) { // F1 (Save)
        editor_buf[editor_len] = '\0';
        // If file doesn't exist, create it
        if (!ramfs_read(editor_target_file)) {
          ramfs_create(editor_target_file);
        }
        ramfs_write(editor_target_file, editor_buf);
        
        // Show saving message
        terminal_initialize();
        print("\nSaving file ");
        print(editor_target_file);
        print("...\n");
        shell_sleep(30);
        terminal_initialize();
        editor_exit();
      } else if (c == '\b') {
        if (editor_len > 0) {
          editor_len--;
          editor_buf[editor_len] = '\0';
          terminal_putchar('\b');
        }
      } else {
        if (editor_len < EDITOR_BUFFER_SIZE - 1) {
          editor_buf[editor_len++] = c;
          terminal_putchar(c);
        }
      }
    }
    shell_sleep(2); // Yield CPU
  }
}
