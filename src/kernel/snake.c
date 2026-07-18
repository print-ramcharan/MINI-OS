#include "snake.h"
#include "vga.h"
#include "syscall.h"
#include "timer.h"
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

static void spawn_food(void) {
  uint32_t ticks = get_tick();
  
  // Calculate raw random coords
  int rx = BOARD_MIN_X + (int)(ticks % (BOARD_MAX_X - BOARD_MIN_X + 1));
  int ry = BOARD_MIN_Y + (int)((ticks / 7) % (BOARD_MAX_Y - BOARD_MIN_Y + 1));

  // Check if overlaps with body
  for (int i = 0; i < game.length; i++) {
    if (game.body[i].x == rx && game.body[i].y == ry) {
      rx++;
      if (rx > BOARD_MAX_X) {
        rx = BOARD_MIN_X;
        ry++;
        if (ry > BOARD_MAX_Y) ry = BOARD_MIN_Y;
      }
    }
  }

  game.food.x = rx;
  game.food.y = ry;

  // Draw food (red '*')
  terminal_putentryat('*', vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK), game.food.x, game.food.y);
}

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

  // Draw snake head
  terminal_putentryat('@', vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), game.body[0].x, game.body[0].y);
  // Draw body segments
  for (int i = 1; i < game.length; i++) {
    terminal_putentryat('o', vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK), game.body[i].x, game.body[i].y);
  }

  // Place food
  spawn_food();
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
  // Calculate new head
  coord_t new_head = game.body[0];
  switch (game.dir) {
    case DIR_UP:    new_head.y--; break;
    case DIR_DOWN:  new_head.y++; break;
    case DIR_LEFT:  new_head.x--; break;
    case DIR_RIGHT: new_head.x++; break;
  }

  // 1. Check Wall Collisions
  if (new_head.x < BOARD_MIN_X || new_head.x > BOARD_MAX_X ||
      new_head.y < BOARD_MIN_Y || new_head.y > BOARD_MAX_Y) {
    game.game_over = 1;
    return;
  }

  // 2. Check Self Collisions
  for (int i = 0; i < game.length; i++) {
    if (game.body[i].x == new_head.x && game.body[i].y == new_head.y) {
      game.game_over = 1;
      return;
    }
  }

  // 3. Check Food Collision
  int eats_food = (new_head.x == game.food.x && new_head.y == game.food.y);

  if (eats_food) {
    game.score += 10;
    if (game.length < MAX_SNAKE_LEN) {
      game.length++;
    }
    // Shift body segments
    for (int i = game.length - 1; i > 0; i--) {
      game.body[i] = game.body[i - 1];
    }
    game.body[0] = new_head;
    spawn_food();
  } else {
    // Clear tail from screen
    coord_t tail = game.body[game.length - 1];
    terminal_putentryat(' ', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), tail.x, tail.y);

    // Shift body
    for (int i = game.length - 1; i > 0; i--) {
      game.body[i] = game.body[i - 1];
    }
    game.body[0] = new_head;
  }

  // Draw body segments
  for (int i = 1; i < game.length; i++) {
    terminal_putentryat('o', vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK), game.body[i].x, game.body[i].y);
  }
  // Draw head
  terminal_putentryat('@', vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), game.body[0].x, game.body[0].y);

  // Draw score
  extern size_t terminal_row;
  extern size_t terminal_column;
  terminal_row = 1;
  terminal_column = 5;
  terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
  print("Score: ");
  print_dec(game.score);
}

void snake_game_task(void) {
  draw_board();
  init_game();

  while (1) {
    if (game.game_over) {
      // Draw Game Over Box
      terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
      extern size_t terminal_row;
      extern size_t terminal_column;
      terminal_row = 11;
      terminal_column = 25;
      print("*****************************");
      terminal_row = 12;
      terminal_column = 25;
      print("*         GAME OVER!        *");
      terminal_row = 13;
      terminal_column = 25;
      print("*  Press [Enter] to exit    *");
      terminal_row = 14;
      terminal_column = 25;
      print("*****************************");

      // Wait for exit key
      while (1) {
        char key = shell_read_char();
        if (key == '\n' || key == 27) {
          terminal_initialize();
          snake_exit();
        }
        shell_sleep(5);
      }
    }

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
