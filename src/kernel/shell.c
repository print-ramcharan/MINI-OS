#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "syscall.h"
#include <stdint.h>



static void shell_sleep(uint32_t ticks) {
  asm volatile("mov $2, %%eax; mov %0, %%ebx; int $0x80"
               :
               : "r"(ticks)
               : "eax", "ebx");
}

static void shell_exit(void) {
  asm volatile("mov $3, %%eax; int $0x80" ::: "eax");
}

static char shell_read_char(void) {
  uint32_t val;
  asm volatile("mov $5, %%eax; int $0x80; mov %%eax, %0"
               : "=r"(val)
               :
               : "eax");
  return (char)val;
}

static int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

#define CMD_BUFFER_SIZE 64
static char cmd_buf[CMD_BUFFER_SIZE];
static int cmd_len = 0;

void execute_command(const char *cmd) {
  if (strcmp(cmd, "help") == 0) {
    print("Available commands:\n");
    print("  help   - Show this help message\n");
    print("  clear  - Clear the screen\n");
    print("  ticks  - Print current system ticks\n");
    print("  about  - Show operating system details\n");
    print("  exit   - Exit the shell process\n");
  } else if (strcmp(cmd, "clear") == 0) {
    terminal_initialize();
  } else if (strcmp(cmd, "ticks") == 0) {
    print("System uptime: ");
    print_dec(get_tick());
    print(" ticks\n");
  } else if (strcmp(cmd, "about") == 0) {
    print("MINI OS KERNEL v1.1 - Command Shell\n");
  } else if (strcmp(cmd, "exit") == 0) {
    print("Exiting shell. Goodbye!\n");
    shell_exit();
  } else if (strcmp(cmd, "") == 0) {
    // Empty command, do nothing
  } else {
    print("Unknown command: ");
    print(cmd);
    print("\nType 'help' for a list of commands.\n");
  }
}

void shell_task(void) {
  print("Welcome to the MINI OS Shell!\n");
  print("Type 'help' to see available commands.\n\n");
  print("minios> ");

  cmd_len = 0;

  while (1) {
    char c = shell_read_char();
    if (c != 0) {
      if (c == '\n') {
        print("\n");
        cmd_buf[cmd_len] = '\0';
        execute_command(cmd_buf);
        cmd_len = 0;
        print("minios> ");
      } else if (c == '\b') {
        if (cmd_len > 0) {
          cmd_len--;
          print("\b");
        }
      } else {
        if (cmd_len < CMD_BUFFER_SIZE - 1) {
          cmd_buf[cmd_len++] = c;
          // Echo is already handled in keyboard driver, but print("\b") works
          // because it erases. Let's make sure we don't double print.
          // Wait, the keyboard driver prints the character.
          // So we don't need to print(c) here!
        }
      }
    }
    shell_sleep(2); // Sleep for 40ms to yield CPU
  }
}
