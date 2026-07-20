#include "scheduler.h"
#include "kheap.h"
#include "pmm.h"
#include "vga.h"

#define MAX_PROCESSES 64
#define PROCESS_STACK_SIZE 4096

process_t *current_process = NULL;
process_t *ready_queue = NULL;
uint32_t next_pid = 1;
uint8_t scheduler_enabled = 0;

extern void context_switch(uint32_t *old_esp, uint32_t new_esp);
extern void process_exit_stub();

void scheduler_init() {
  scheduler_enabled = 0;
  current_process = NULL;
  ready_queue = NULL;
}

process_t *create_process(void (*entry_point)()) {
  process_t *new_proc = (process_t *)kmalloc(sizeof(process_t));
  new_proc->pid = next_pid++;
  new_proc->state = PROCESS_READY;
  new_proc->sleep_ticks = 0;
  new_proc->priority = DEFAULT_PROCESS_PRIORITY;
  for (int i = 0; i < MAX_PROCESS_OPEN_FILES; i++) {
    new_proc->ofiles[i].used = 0;
    new_proc->ofiles[i].filename[0] = '\0';
    new_proc->ofiles[i].offset = 0;
  }
  new_proc->next = NULL;

  // Allocate stack physical page (4KB)
  uint32_t physical_stack = pmm_alloc_page();
  // Wait, the heap handles its own virtual mappings if we expand it properly,
  // but right now it's mapped to a single 4KB page. We will just use kmalloc
  // for the stack since we mapped 4KB to kheap. Wait, kmalloc buffer only has 1
  // page right now. Let's allocate raw memory for this stack via kmalloc and
  // extend the heap first if needed.
  uint32_t stack_mem = (uint32_t)kmalloc(PROCESS_STACK_SIZE);

  // Simulate the stack frame that context_switch expects.
  // Stack grows downwards
  uint32_t *stack_top = (uint32_t *)(stack_mem + PROCESS_STACK_SIZE);

  // Context switch needs (edi, esi, ebx, ebp, eip) -> 5 registers
  *(--stack_top) = (uint32_t)entry_point; // Return address (EIP)
  *(--stack_top) = 0;                     // EBP
  *(--stack_top) = 0;                     // EBX
  *(--stack_top) = 0;                     // ESI
  *(--stack_top) = 0;                     // EDI

  new_proc->esp = (uint32_t)stack_top;
  new_proc->ebp = 0;
  new_proc->kernel_stack = stack_mem;

  // Add to ready queue (round robin, add to end)
  if (!ready_queue) {
    ready_queue = new_proc;
    new_proc->next = new_proc; // Circular link
  } else {
    process_t *temp = ready_queue;
    while (temp->next != ready_queue) {
      temp = temp->next;
    }
    temp->next = new_proc;
    new_proc->next = ready_queue;
  }

  if (!current_process) {
    current_process = new_proc;
  }

  return new_proc;
}

void schedule(registers_t *regs) {
  // Only schedule if we have initialized the scheduler and have processes
  if (!scheduler_enabled || !current_process) {
    return;
  }

  process_t *prev = current_process;
  process_t *next = current_process->next;

  // Skip dead/blocked processes
  while (next->state != PROCESS_READY && next->state != PROCESS_RUNNING) {
    next = next->next;
    if (next == prev) {
      if (prev->state == PROCESS_DEAD) {
        print("\n[Scheduler] All processes finished. Halting CPU.\n");
        asm volatile("cli; hlt");
      }
      return; // All dead/blocked, but current is not dead (could be sleeping/blocked)
    }
  }

  if (prev == next) {
    return; // Only 1 process running
  }

  if (prev->state == PROCESS_RUNNING) {
    prev->state = PROCESS_READY;
  }
  next->state = PROCESS_RUNNING;
  current_process = next;

  // Perform assembly context switch
  context_switch(&prev->esp, next->esp);
}

void scheduler_tick(void) {
  if (!scheduler_enabled || !ready_queue) {
    return;
  }
  process_t *temp = ready_queue;
  do {
    if (temp->state == PROCESS_BLOCKED && temp->sleep_ticks > 0) {
      temp->sleep_ticks--;
      if (temp->sleep_ticks == 0) {
        temp->state = PROCESS_READY;
      }
    }
    temp = temp->next;
  } while (temp != ready_queue);
}

void enable_scheduler() { scheduler_enabled = 1; }

void scheduler_print_processes(void) {
  if (!ready_queue) {
    print("No processes running.\n");
    return;
  }
  print("  PID \t STATE   \t SLEEP_TICKS\n");
  process_t *temp = ready_queue;
  do {
    print("  ");
    print_dec(temp->pid);
    print("   \t ");
    switch (temp->state) {
      case PROCESS_RUNNING: print("RUNNING "); break;
      case PROCESS_READY:   print("READY   "); break;
      case PROCESS_BLOCKED: print("BLOCKED "); break;
      case PROCESS_DEAD:    print("DEAD    "); break;
    }
    print(" \t ");
    print_dec(temp->sleep_ticks);
    print("\n");
    temp = temp->next;
  } while (temp != ready_queue);
}

int scheduler_kill_process(uint32_t pid) {
  if (!ready_queue) return -1;
  if (pid == 1) return -2; // Cannot kill shell

  process_t *temp = ready_queue;
  do {
    if (temp->pid == pid) {
      temp->state = PROCESS_DEAD;
      return 0;
    }
    temp = temp->next;
  } while (temp != ready_queue);

  return -1; // Not found
}
