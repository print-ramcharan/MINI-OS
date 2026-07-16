#include "syscall.h"
#include "scheduler.h"
#include "timer.h"
#include "vga.h"

// System call implementations

static void sys_write(char *str) { print(str); }

static void sys_sleep(uint32_t ticks) {
  // Basic busy sleep using PIT ticks
  extern uint32_t tick;
  uint32_t end_tick = tick + ticks;
  while (tick < end_tick) {
    // Wait
  }
}

static void sys_exit() {
  print("\n[SYSCALL] Process exited.\n");
  // For now, just halt the CPU as we don't have user processes to cleanly tear
  // down
  asm volatile("cli; hlt");
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
    sys_sleep(regs->ebx);
    break;
  case SYS_EXIT:
    sys_exit();
    break;
  case SYS_YIELD:
    schedule(regs);
    break;
  default:
    print("[SYSCALL] Unknown syscall number: ");
    print_hex(regs->eax);
    print("\n");
    break;
  }
}

void syscall_init() { register_interrupt_handler(128, syscall_handler); }
