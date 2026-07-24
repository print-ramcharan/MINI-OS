#include "keyboard.h"
#include "idt.h"
#include "vga.h"

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

unsigned char kbdus[128] = {
    0,   27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', ' ',  'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   19,  0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0};

#define KEYBOARD_BUFFER_SIZE 128
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t keyboard_buffer_head = 0;
static uint32_t keyboard_buffer_tail = 0;

static void keyboard_buffer_push(char c) {
  uint32_t next = (keyboard_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
  if (next != keyboard_buffer_tail) {
    keyboard_buffer[keyboard_buffer_head] = c;
    keyboard_buffer_head = next;
  }
}

char keyboard_read_char(void) {
  if (keyboard_buffer_head == keyboard_buffer_tail) {
    return 0; // Empty
  }
  char c = keyboard_buffer[keyboard_buffer_tail];
  keyboard_buffer_tail = (keyboard_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
  return c;
}

static void keyboard_callback(registers_t *regs) {
  (void)regs;
  uint8_t scancode = inb(0x60);
  if (scancode & 0x80) {
    // key release
  } else {
    // key press
    char c = kbdus[scancode];
    if (c != 0) {
      if (c >= 32 || c == '\n' || c == '\b') {
        terminal_putchar(c);
      }
      keyboard_buffer_push(c);
    }
  }
}

void init_keyboard() { register_interrupt_handler(33, keyboard_callback); }
