#include "snake.h"
#include "vga.h"
#include "syscall.h"
#include <stdint.h>

static void draw_board(void) {
  terminal_initialize();

  // Top header title
  extern size_t terminal_row;
  extern size_t terminal_column;
  terminal_row = 1;
  terminal_column = 25;
  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
  print("=== MINI-OS KERNEL SNAKE ===");

  // Draw borders
  terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY));
  
  // Top and bottom borders
  for (int x = BOARD_MIN_X - 1; x <= BOARD_MAX_X + 1; x++) {
    terminal_putentryat('#', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY), x, BOARD_MIN_Y - 1);
    terminal_putentryat('#', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY), x, BOARD_MAX_Y + 1);
  }

  // Left and right borders
  for (int y = BOARD_MIN_Y; y <= BOARD_MAX_Y; y++) {
    terminal_putentryat('#', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY), BOARD_MIN_X - 1, y);
    terminal_putentryat('#', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY), BOARD_MAX_X + 1, y);
  }

  // Footer instructions
  terminal_row = 24;
  terminal_column = 15;
  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
  print("Controls: W/A/S/D or Arrow keys | Esc to Quit");
}

static snake_game_t game;

static void init_game(void) {
  game.length = 3;
  game.dir = DIR_RIGHT;
  game.score = 0;
  game.game_over = 0;

  // Middle of screen
  game.body[0].x = 40;
  game.body[0].y = 12;
  game.body[1].x = 39;
  game.body[1].y = 12;
  game.body[2].x = 38;
  game.body[2].y = 12;

  // Place food at static position initially
  game.food.x = 25;
  game.food.y = 10;

  // Draw snake head
  terminal_putentryat('@', vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), game.body[0].x, game.body[0].y);
  // Draw body segments
  for (int i = 1; i < game.length; i++) {
    terminal_putentryat('o', vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK), game.body[i].x, game.body[i].y);
  }
}

void snake_game_task(void) {
  draw_board();
  init_game();

  while (1) {
    asm volatile("mov $4, %%eax; int $0x80" ::: "eax"); // Yield
  }
}
