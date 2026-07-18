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

static void snake_exit(void) {
  asm volatile("mov $3, %%eax; int $0x80" ::: "eax");
}

static void move_snake(void) {
  // Clear tail from screen
  coord_t tail = game.body[game.length - 1];
  terminal_putentryat(' ', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), tail.x, tail.y);

  // Shift body segments
  for (int i = game.length - 1; i > 0; i--) {
    game.body[i] = game.body[i - 1];
  }

  // Calculate new head
  coord_t new_head = game.body[0];
  switch (game.dir) {
    case DIR_UP:    new_head.y--; break;
    case DIR_DOWN:  new_head.y++; break;
    case DIR_LEFT:  new_head.x--; break;
    case DIR_RIGHT: new_head.x++; break;
  }
  
  // Set new head
  game.body[0] = new_head;

  // Draw body segments
  for (int i = 1; i < game.length; i++) {
    terminal_putentryat('o', vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK), game.body[i].x, game.body[i].y);
  }
  // Draw head
  terminal_putentryat('@', vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), game.body[0].x, game.body[0].y);
}

void snake_game_task(void) {
  draw_board();
  init_game();

  while (1) {
    char c = shell_read_char();
    if (c != 0) {
      if (c == 27) { // ESC to Quit
        terminal_initialize();
        snake_exit();
      } else if (c == 'w' || c == 'W') {
        if (game.dir != DIR_DOWN) game.dir = DIR_UP;
      } else if (c == 's' || c == 'S') {
        if (game.dir != DIR_UP) game.dir = DIR_DOWN;
      } else if (c == 'a' || c == 'A') {
        if (game.dir != DIR_RIGHT) game.dir = DIR_LEFT;
      } else if (c == 'd' || c == 'D') {
        if (game.dir != DIR_LEFT) game.dir = DIR_RIGHT;
      }
    }
    move_snake();
    shell_sleep(10); // Loop speed control (200ms)
  }
}
