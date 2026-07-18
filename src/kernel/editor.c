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

void editor_task(void) {
  draw_ui();
  
  // Basic loop that yields for now
  while (1) {
    asm volatile("mov $4, %%eax; int $0x80" ::: "eax"); // Yield
  }
}
