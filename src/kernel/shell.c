#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "syscall.h"
#include "pmm.h"
#include "kheap.h"
#include "ramfs.h"
#include "scheduler.h"
#include "editor.h"
#include "env.h"
#include "top.h"
#include <stdint.h>

void execute_command(const char *cmd);

static void shell_sleep(uint32_t ticks) {
  asm volatile("mov $2, %%eax; mov %0, %%ebx; int $0x80"
               :
               : "r"(ticks)
               : "eax", "ebx");
}

static void shell_exit(void) {
  asm volatile("mov $3, %%eax; int $0x80" ::: "eax");
}

static int shell_open(const char *filename) {
  int val;
  asm volatile("int $0x80" : "=a"(val) : "a"(6), "b"(filename));
  return val;
}

static int shell_close(int fd) {
  int val;
  asm volatile("int $0x80" : "=a"(val) : "a"(9), "b"(fd));
  return val;
}

static int shell_read(int fd, char *buf, uint32_t size) {
  int val;
  asm volatile("int $0x80" : "=a"(val) : "a"(7), "b"(fd), "c"(buf), "d"(size));
  return val;
}

static int shell_write_file(int fd, const char *buf, uint32_t size) {
  int val;
  asm volatile("int $0x80" : "=a"(val) : "a"(8), "b"(fd), "c"(buf), "d"(size));
  return val;
}

static int shell_delete(const char *filename) {
  int val;
  asm volatile("int $0x80" : "=a"(val) : "a"(10), "b"(filename));
  return val;
}

static int shell_set_priority(uint32_t pid, uint32_t priority) {
  int val;
  asm volatile("int $0x80" : "=a"(val) : "a"(11), "b"(pid), "c"(priority));
  return val;
}

static int atoi(const char *str) {
  int res = 0;
  for (int i = 0; str[i] != '\0'; ++i) {
    if (str[i] >= '0' && str[i] <= '9') {
      res = res * 10 + (str[i] - '0');
    } else {
      break;
    }
  }
  return res;
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

static void parse_redirect(const char *cmd, char *clean_cmd, char *target_file, int *append, int *active) {
  *active = 0;
  *append = 0;
  target_file[0] = '\0';
  
  int i = 0;
  int split_idx = -1;
  int is_double = 0;
  
  while (cmd[i]) {
    if (cmd[i] == '>') {
      if (cmd[i+1] == '>') {
        split_idx = i;
        is_double = 1;
        break;
      } else {
        split_idx = i;
        is_double = 0;
        break;
      }
    }
    i++;
  }
  
  if (split_idx != -1) {
    int k = 0;
    for (k = 0; k < split_idx; k++) {
      clean_cmd[k] = cmd[k];
    }
    while (k > 0 && clean_cmd[k-1] == ' ') {
      k--;
    }
    clean_cmd[k] = '\0';
    
    int f_start = split_idx + (is_double ? 2 : 1);
    while (cmd[f_start] == ' ') {
      f_start++;
    }
    int f_idx = 0;
    while (cmd[f_start] && cmd[f_start] != ' ' && f_idx < 31) {
      target_file[f_idx++] = cmd[f_start++];
    }
    target_file[f_idx] = '\0';
    
    if (target_file[0] != '\0') {
      *active = 1;
      *append = is_double;
    }
  } else {
    int k = 0;
    while (cmd[k]) {
      clean_cmd[k] = cmd[k];
      k++;
    }
    clean_cmd[k] = '\0';
  }
}

static int get_line(const char *buffer, int start, char *line, int max_len) {
  int i = start;
  int j = 0;
  while (buffer[i] && buffer[i] != '\n' && j < max_len - 1) {
    line[j++] = buffer[i++];
  }
  line[j] = '\0';
  if (buffer[i] == '\n') {
    i++;
  }
  return i;
}

static int script_recursion_depth = 0;

int shell_run_script(const char *filename) {
  if (script_recursion_depth >= MAX_SCRIPT_RECURSION) {
    print("Error: Max script recursion depth reached\n");
    return -2;
  }
  const char *content = ramfs_read(filename);
  if (!content) {
    return -1;
  }
  script_recursion_depth++;
  
  int offset = 0;
  char line[MAX_SCRIPT_LINE_LEN];
  while (content[offset]) {
    int next_offset = get_line(content, offset, line, MAX_SCRIPT_LINE_LEN);
    if (next_offset == offset) {
      break;
    }
    offset = next_offset;
    execute_command(line);
  }
  
  script_recursion_depth--;
  return 0;
}

void execute_command(const char *cmd) {
  char clean_cmd[64];
  char target_file[32];
  int append = 0;
  int active = 0;
  parse_redirect(cmd, clean_cmd, target_file, &append, &active);

  char arg0[32];
  char arg1[32];
  char arg2[256];
  parse_args(clean_cmd, arg0, arg1, arg2);

  if (active) {
    if (!append) {
      ramfs_create(target_file);
      ramfs_write(target_file, "");
    } else {
      ramfs_create(target_file);
    }
    vga_enable_redirect(target_file, append);
  }

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
    print("  kill <pid>           - Kill a process by PID\n");
    print("  nice <pid> <pri>     - Set process scheduling priority (1-20)\n");
    print("  uptime               - Show formatted system uptime\n");
    print("  top                  - Live system monitor & activity viewer\n");
    print("  env                  - List environment variables\n");
    print("  export <K> <V>       - Set an environment variable\n");
    print("  echo <text|$VAR>     - Print text or variable value\n");
    print("  theme <name>         - Change VGA color theme\n");
    print("  stat <file>          - Display file statistics\n");
    print("  memmap               - Print kernel memory map\n");
    print("  calc <a> <b>         - Run arithmetic calculator\n");
    print("  test_redirect        - Run shell output redirection self-tests\n");
    print("  about                - Show operating system details\n");
    print("  exit                 - Exit the shell process\n");
  } else if (strcmp(arg0, "clear") == 0) {
    terminal_initialize();
  } else if (strcmp(arg0, "ticks") == 0) {
    print("System uptime: ");
    print_dec(get_tick());
    print(" ticks\n");
  } else if (strcmp(arg0, "uptime") == 0) {
    timer_print_uptime();
  } else if (strcmp(arg0, "env") == 0) {
    print("Environment Variables:\n");
    env_list();
  } else if (strcmp(arg0, "export") == 0) {
    if (arg1[0] == '\0' || arg2[0] == '\0') {
      print("Usage: export <KEY> <VALUE>\n");
    } else {
      int res = env_set(arg1, arg2);
      if (res == 0) {
        print("Variable "); print(arg1); print(" set.\n");
      } else {
        print("Error: Environment variable table full.\n");
      }
    }
  } else if (strcmp(arg0, "echo") == 0) {
    if (arg1[0] == '$') {
      const char *val = env_get(arg1 + 1);
      if (val) {
        print(val);
      }
    } else {
      print(arg1);
      if (arg2[0] != '\0') {
        print(" ");
        print(arg2);
      }
    }
    print("\n");
  } else if (strcmp(arg0, "theme") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: theme <name>\n");
      print("Available themes: default, matrix, cyber, amber, ocean, monochrome\n");
    } else {
      int res = vga_set_theme(arg1);
      if (res == 0) {
        print("Theme applied: "); print(arg1); print("\n");
      } else {
        print("Error: Unknown theme '"); print(arg1); print("'\n");
        print("Available themes: default, matrix, cyber, amber, ocean, monochrome\n");
      }
    }
  } else if (strcmp(arg0, "stat") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: stat <filename>\n");
    } else {
      int res = ramfs_stat(arg1);
      if (res == -1) print("Error: File not found\n");
    }
  } else if (strcmp(arg0, "memmap") == 0) {
    pmm_print_map();
  } else if (strcmp(arg0, "ps") == 0) {
    print("Active kernel tasks:\n");
    scheduler_print_processes();
  } else if (strcmp(arg0, "kill") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: kill <pid>\n");
    } else {
      int pid = atoi(arg1);
      int res = scheduler_kill_process(pid);
      if (res == 0) {
        print("Process "); print_dec(pid); print(" terminated.\n");
      } else if (res == -2) {
        print("Error: Cannot kill the kernel shell.\n");
      } else {
        print("Error: Process "); print_dec(pid); print(" not found.\n");
      }
    }
  } else if (strcmp(arg0, "nice") == 0) {
    if (arg1[0] == '\0' || arg2[0] == '\0') {
      print("Usage: nice <pid> <priority>\n");
    } else {
      int pid = atoi(arg1);
      int priority = atoi(arg2);
      int res = shell_set_priority(pid, priority);
      if (res == 0) {
        print("Priority of process "); print_dec(pid); print(" set to "); print_dec(priority); print(".\n");
      } else {
        print("Error: Could not set priority. Ensure priority is between 1 and 20.\n");
      }
    }
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
      int fd = shell_open(arg1);
      if (fd < 0) {
        print("Error: Could not create file\n");
      } else {
        print("File created.\n");
        shell_close(fd);
      }
    }
  } else if (strcmp(arg0, "rm") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: rm <filename>\n");
    } else {
      int res = shell_delete(arg1);
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
  } else if (strcmp(arg0, "top") == 0) {
    process_t *child = create_process(top_task);
    while (child->state != PROCESS_DEAD) {
      shell_sleep(5);
    }
    terminal_initialize();
    print("Welcome back to the MINI OS Shell!\n\n");
  } else if (strcmp(arg0, "write") == 0) {
    if (arg1[0] == '\0' || arg2[0] == '\0') {
      print("Usage: write <filename> <content>\n");
    } else {
      int fd = shell_open(arg1);
      if (fd < 0) {
        print("Error: Could not open/create file\n");
      } else {
        int len = 0;
        while (arg2[len]) len++;
        int res = shell_write_file(fd, arg2, len);
        if (res == -1) print("Error: File not found\n");
        else print("File written.\n");
        shell_close(fd);
      }
    }
  } else if (strcmp(arg0, "cat") == 0) {
    if (arg1[0] == '\0') {
      print("Usage: cat <filename>\n");
    } else {
      int fd = shell_open(arg1);
      if (fd < 0) {
        print("Error: File not found\n");
      } else {
        char buf[256];
        int bytes = shell_read(fd, buf, sizeof(buf) - 1);
        if (bytes >= 0) {
          buf[bytes] = '\0';
          print(buf);
          print("\n");
        } else {
          print("Error: Could not read file\n");
        }
        shell_close(fd);
      }
    }
  } else if (strcmp(arg0, "calc") == 0) {
    if (arg1[0] == '\0' || arg2[0] == '\0') {
      print("Usage: calc <num1> <num2>\n");
    } else {
      int a = atoi(arg1);
      int b = atoi(arg2);
      print("Arithmetic Operations for "); print_dec(a); print(" and "); print_dec(b); print(":\n");
      print("  Addition       (a + b) = "); print_dec(a + b); print("\n");
      print("  Subtraction    (a - b) = "); print_dec(a >= b ? a - b : 0); print("\n");
      print("  Multiplication (a * b) = "); print_dec(a * b); print("\n");
      if (b != 0) {
        print("  Division       (a / b) = "); print_dec(a / b); print("\n");
      } else {
        print("  Division       (a / b) = Error (div by 0)\n");
      }
    }
  } else if (strcmp(arg0, "test_redirect") == 0) {
    print("Running redirection self-tests...\n");
    execute_command("help > test_help.txt");
    const char *content1 = ramfs_read("test_help.txt");
    if (content1 && content1[0] != '\0') {
      print("  [PASS] Test 1: Redirect 'help' output to file\n");
    } else {
      print("  [FAIL] Test 1: Redirect 'help' output failed\n");
    }
    execute_command("free > test_free.txt");
    const char *content2 = ramfs_read("test_free.txt");
    if (content2 && content2[0] != '\0') {
      print("  [PASS] Test 2: Redirect 'free' memory statistics\n");
    } else {
      print("  [FAIL] Test 2: Redirect 'free' failed\n");
    }
    execute_command("uptime > test_log.txt");
    execute_command("uptime >> test_log.txt");
    const char *content3 = ramfs_read("test_log.txt");
    // Verify it appended by looking for multiple 'System Uptime' texts
    int count = 0;
    int idx = 0;
    while (content3 && content3[idx]) {
      if (content3[idx] == 'U' && content3[idx+1] == 'p') {
        count++;
      }
      idx++;
    }
    if (count >= 2) {
      print("  [PASS] Test 3: Append output redirection ('>>')\n");
    } else {
      print("  [FAIL] Test 3: Append output redirection ('>>') failed\n");
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
  
  if (active) {
    vga_disable_redirect();
  }
}

void shell_task(void) {
  print("Welcome to the MINI OS Shell!\n");
  print("Type 'help' to see available commands.\n\n");

  print("Executing /init.sh...\n");
  shell_run_script("init.sh");
  print("Startup sequence complete.\n\n");

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
