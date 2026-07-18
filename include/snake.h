#ifndef SNAKE_H
#define SNAKE_H

#define MAX_SNAKE_LEN 100
#define BOARD_MIN_X 10
#define BOARD_MAX_X 70
#define BOARD_MIN_Y 3
#define BOARD_MAX_Y 22

typedef enum {
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
} direction_t;

typedef struct {
  int x;
  int y;
} coord_t;

typedef struct {
  coord_t body[MAX_SNAKE_LEN];
  int length;
  direction_t dir;
  coord_t food;
  int score;
  int game_over;
} snake_game_t;

void snake_game_task(void);

#endif
