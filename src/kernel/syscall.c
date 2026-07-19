#include "syscall.h"
#include "scheduler.h"
#include "timer.h"
#include "vga.h"
#include "keyboard.h"
#include "ramfs.h"

// System call implementations

static void sys_write(char *str) { print(str); }

static void sys_sleep(uint32_t ticks, registers_t *regs) {
  if (ticks > 0 && current_process) {
    current_process->sleep_ticks = ticks;
    current_process->state = PROCESS_BLOCKED;
    schedule(regs);
  }
}

static void sys_exit(registers_t *regs) {
  if (current_process) {
    print("\n[SYSCALL] Process ");
    print_dec(current_process->pid);
    print(" exited cleanly.\n");
    current_process->state = PROCESS_DEAD;
    schedule(regs);
  }
  for (;;) {
    asm volatile("cli; hlt");
  }
}

static char sys_read_char(void) {
  return keyboard_read_char();
}

static int sys_open(const char *filename) {
  if (!current_process || !filename) return -1;

  int fd = -1;
  for (int i = 0; i < MAX_PROCESS_OPEN_FILES; i++) {
    if (!current_process->ofiles[i].used) {
      fd = i;
      break;
    }
  }
  if (fd == -1) return -1;

  const char *content = ramfs_read(filename);
  if (!content) {
    int res = ramfs_create(filename);
    if (res < 0 && res != -1) return -1;
  }

  current_process->ofiles[fd].used = 1;
  current_process->ofiles[fd].offset = 0;
  int j = 0;
  while (filename[j] && j < MAX_FILENAME_LEN - 1) {
    current_process->ofiles[fd].filename[j] = filename[j];
    j++;
  }
  current_process->ofiles[fd].filename[j] = '\0';

  return fd;
}
static int sys_read(int fd, char *buf, uint32_t size) {
  if (!current_process || fd < 0 || fd >= MAX_PROCESS_OPEN_FILES) return -1;
  if (!current_process->ofiles[fd].used || !buf) return -1;

  const char *content = ramfs_read(current_process->ofiles[fd].filename);
  if (!content) return -1;

  uint32_t offset = current_process->ofiles[fd].offset;
  uint32_t i = 0;
  while (content[offset + i] && i < size) {
    buf[i] = content[offset + i];
    i++;
  }
  buf[i] = '\0';
  current_process->ofiles[fd].offset += i;
  return (int)i;
}
static int sys_write_file(int fd, const char *buf, uint32_t size) { (void)fd; (void)buf; (void)size; return -1; }
static int sys_close(int fd) { (void)fd; return -1; }
static int sys_delete(const char *filename) { (void)filename; return -1; }

void syscall_handler(registers_t *regs) {
  // The syscall number is passed in EAX
  // Arguments are passed in EBX, ECX, EDX, ESI, EDI
  if (!regs)
    return;

  switch (regs->eax) {
  case SYS_WRITE:
    sys_write((char *)regs->ebx);
    break;
  case SYS_SLEEP:
    sys_sleep(regs->ebx, regs);
    break;
  case SYS_EXIT:
    sys_exit(regs);
    break;
  case SYS_YIELD:
    schedule(regs);
    break;
  case SYS_READ_CHAR:
    regs->eax = (uint32_t)sys_read_char();
    break;
  case SYS_OPEN:
    regs->eax = (uint32_t)sys_open((const char *)regs->ebx);
    break;
  case SYS_READ:
    regs->eax = (uint32_t)sys_read((int)regs->ebx, (char *)regs->ecx, (uint32_t)regs->edx);
    break;
  case SYS_WRITE_FILE:
    regs->eax = (uint32_t)sys_write_file((int)regs->ebx, (const char *)regs->ecx, (uint32_t)regs->edx);
    break;
  case SYS_CLOSE:
    regs->eax = (uint32_t)sys_close((int)regs->ebx);
    break;
  case SYS_DELETE:
    regs->eax = (uint32_t)sys_delete((const char *)regs->ebx);
    break;
  default:
    print("[SYSCALL] Unknown syscall number: ");
    print_hex(regs->eax);
    print("\n");
    break;
  }
}

void syscall_init() { register_interrupt_handler(128, syscall_handler); }
