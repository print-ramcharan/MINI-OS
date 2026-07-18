#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "syscall.h"
#include "pmm.h"
#include "kheap.h"
#include "ramfs.h"
#include "scheduler.h"
#include "editor.h"
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

static void parse_args(const char *cmd, char *arg0, char *arg1, char *arg2) {
  int i = 0;
  int j = 0;
  // Read first token (arg0)
  while (cmd[i] && cmd[i] != ' ' && j < 31) {
    arg0[j++] = cmd[i++];
  }
  arg0[j] = '\0';
  
  // Skip spaces
  while (cmd[i] == ' ') i++;
  
  // Read second token (arg1)
  j = 0;
  while (cmd[i] && cmd[i] != ' ' && j < 31) {
    arg1[j++] = cmd[i++];
  }
  arg1[j] = '\0';

  // Skip spaces
  while (cmd[i] == ' ') i++;
  
  // Read third token/rest of string (arg2)
  j = 0;
  while (cmd[i] && j < 255) {
    arg2[j++] = cmd[i++];
  }
  arg2[j] = '\0';
}

#define CMD_BUFFER_SIZE 64
static char cmd_buf[CMD_BUFFER_SIZE];
static int cmd_len = 0;

void execute_command(const char *cmd) {
  char arg0[32];
  char arg1[32];
  char arg2[256];
  parse_args(cmd, arg0, arg1, arg2);

  if (strcmp(arg0, "help") == 0) {
    print("Available commands:\n");
    print("  help                 - Show this help message\n");
    print("  clear                - Clear the screen\n");
    print("  ticks                - Print current system ticks\n");
    print("  free                 - Display physical and heap memory statistics\n");
    print("  cpuinfo              - Show CPU register and system descriptor statistics\n");
    print("  ls                   - List files in RAM File System\n");
    print("  touch <file>         - Create an empty file\n");
    print("  write <file> <text>  - Write text content to a file\n");
    print("  cat <file>           - Display the content of a file\n");
    print("  rm <file>            - Delete a file\n");
    print("  cp <src> <dest>      - Copy a file\n");
    print("  mv <src> <dest>      - Move/Rename a file\n");
    print("  edit <file>          - Edit a file interactively\n");
    print("  snake                - Play VGA text mode snake game\n");
    print("  ps                   - List active processes\n");
    print("  about                - Show operating system details\n");
    print("  exit                 - Exit the shell process\n");
  } else if (strcmp(arg0, "clear") == 0) {
    terminal_initialize();
  } else if (strcmp(arg0, "ticks") == 0) {
    print("System uptime: ");
    print_dec(get_tick());
    print(" ticks\n");
  } else if (strcmp(arg0, "ps") == 0) {
    print("Active kernel tasks:\n");
    scheduler_print_processes();
  } else if (strcmp(arg0, "free") == 0) {
    uint32_t max_blocks = pmm_get_max_blocks();
    uint32_t used_blocks = pmm_get_used_blocks();
    uint32_t free_blocks = max_blocks - used_blocks;
    print("Physical Memory (PMM):\n");
    print("  Total pages: "); print_dec(max_blocks); print(" ("); print_dec(max_blocks * 4); print(" KB)\n");
    print("  Used pages:  "); print_dec(used_blocks); print(" ("); print_dec(used_blocks * 4); print(" KB)\n");
    print("  Free pages:  "); print_dec(free_blocks); print(" ("); print_dec(free_blocks * 4); print(" KB)\n\n");

    uint32_t heap_tot = 0, heap_usd = 0, heap_fre = 0;
    kheap_get_stats(&heap_tot, &heap_usd, &heap_fre);
    print("Kernel Dynamic Heap:\n");
    print("  Total size: "); print_dec(heap_tot); print(" bytes\n");
    print("  Used size:  "); print_dec(heap_usd); print(" bytes\n");
    print("  Free size:  "); print_dec(heap_fre); print(" bytes\n");
  } else if (strcmp(arg0, "cpuinfo") == 0) {
    uint32_t cr0, cr3, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    uint16_t gdt_limit;
    uint32_t gdt_base;
    uint8_t gdt_ptr[6];
    asm volatile("sgdt %0" : "=m"(gdt_ptr));
    gdt_limit = *(uint16_t*)gdt_ptr;
    gdt_base = *(uint32_t*)(gdt_ptr + 2);

    uint16_t idt_limit;
    uint32_t idt_base;
    uint8_t idt_ptr[6];
    asm volatile("sidt %0" : "=m"(idt_ptr));
    idt_limit = *(uint16_t*)idt_ptr;
    idt_base = *(uint32_t*)(idt_ptr + 2);

    print("CPU Registers & Control Status:\n");
    print("  CR0 (Control Register 0):  "); print_hex(cr0); print("\n");
    print("  CR3 (Page Directory Base): "); print_hex(cr3); print("\n");
    print("  CR4 (Extended Features):   "); print_hex(cr4); print("\n\n");
    print("Global Descriptor Table (GDT):\n");
    print("  GDT Base:  "); print_hex(gdt_base); print("\n");
    print("  GDT Limit: "); print_hex(gdt_limit); print("\n\n");
    print("Interrupt Descriptor Table (IDT):\n");
    print("  IDT Base:  "); print_hex(idt_base); print("\n");
    print("  IDT Limit: "); print_hex(idt_limit); print("\n");
  } else if (strcmp(arg0, "ls") == 0) {
    print("Files in RamFS:\n");
    ramfs_list();
  } else if (strcmp(arg0, "touch") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: touch <filename>\n");
    } else {
      int res = ramfs_create(arg1);
      if (res == -1) print("Error: File already exists\n");
      else if (res == -2) print("Error: File system full\n");
      else print("File created.\n");
    }
  } else if (strcmp(arg0, "rm") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: rm <filename>\n");
    } else {
      int res = ramfs_delete(arg1);
      if (res == -1) print("Error: File not found\n");
      else print("File deleted.\n");
    }
  } else if (strcmp(arg0, "cp") == 0) {
    if (arg1[0] == '\0' || arg2[0] == '\0') {
      print("Usage: cp <source> <dest>\n");
    } else {
      int res = ramfs_copy(arg1, arg2);
      if (res == -1) print("Error: Source file not found\n");
      else if (res == -2) print("Error: File system full\n");
      else if (res == -3) print("Error: Destination already exists\n");
      else print("File copied.\n");
    }
  } else if (strcmp(arg0, "mv") == 0) {
    if (arg1[0] == '\0' || arg2[0] == '\0') {
      print("Usage: mv <source> <dest>\n");
    } else {
      int res = ramfs_rename(arg1, arg2);
      if (res == -1) print("Error: Source file not found\n");
      else if (res == -3) print("Error: Destination already exists\n");
      else print("File moved.\n");
    }
  } else if (strcmp(arg0, "edit") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: edit <filename>\n");
    } else {
      int file_len = 0;
      while (arg1[file_len] && file_len < 31) {
        editor_target_file[file_len] = arg1[file_len];
        file_len++;
      }
      editor_target_file[file_len] = '\0';
      process_t *child = create_process(editor_task);
      while (child->state != PROCESS_DEAD) {
        shell_sleep(5);
      }
      terminal_initialize();
      print("Welcome back to the MINI OS Shell!\n\n");
    }
  } else if (strcmp(arg0, "snake") == 0) {
    extern void snake_game_task(void);
    process_t *child = create_process(snake_game_task);
    while (child->state != PROCESS_DEAD) {
      shell_sleep(5);
    }
    terminal_initialize();
    print("Welcome back to the MINI OS Shell!\n\n");
  } else if (strcmp(arg0, "write") == 0) {
    if (arg1[0] == '\0' || arg2[0] == '\0') {
      print("Usage: write <filename> <content>\n");
    } else {
      int res = ramfs_write(arg1, arg2);
      if (res == -1) print("Error: File not found\n");
      else print("File written.\n");
    }
  } else if (strcmp(arg0, "cat") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: cat <filename>\n");
    } else {
      const char *content = ramfs_read(arg1);
      if (!content) print("Error: File not found\n");
      else {
        print(content);
        print("\n");
      }
    }
  } else if (strcmp(arg0, "about") == 0) {
    print("MINI OS KERNEL v1.1 - Command Shell\n");
  } else if (strcmp(arg0, "exit") == 0) {
    print("Exiting shell. Goodbye!\n");
    shell_exit();
  } else if (strcmp(arg0, "") == 0) {
    // Empty command, do nothing
  } else {
    print("Unknown command: ");
    print(arg0);
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
