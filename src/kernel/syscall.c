#include "syscall.h"
#include "scheduler.h"
#include "timer.h"
#include "vga.h"
#include "keyboard.h"

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
  default:
    print("[SYSCALL] Unknown syscall number: ");
    print_hex(regs->eax);
    print("\n");
    break;
  }
}

void syscall_init() { register_interrupt_handler(128, syscall_handler); }
