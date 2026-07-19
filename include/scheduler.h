#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "idt.h"
#include <stdint.h>

typedef enum {
  PROCESS_RUNNING,
  PROCESS_READY,
  PROCESS_BLOCKED,
  PROCESS_DEAD
} process_state_t;

// Context structure describing the CPU registers during a context switch
typedef struct context {
  uint32_t edi, esi, ebx, ebp, eip;
} context_t;

#include "ramfs.h"

typedef struct process {
  uint32_t pid;
  process_state_t state;
  uint32_t esp;          // The stack pointer for the process
  uint32_t ebp;          // The base pointer
  uint32_t kernel_stack; // Top of the allocated stack
  uint32_t sleep_ticks;  // Sleep duration remaining in ticks
  open_file_t ofiles[MAX_PROCESS_OPEN_FILES];

  struct process *next; // Round-robin links
} process_t;

void scheduler_init(void);
void scheduler_tick(void);

extern process_t *current_process;

// Creates a new process that will start execution at the given function
process_t *create_process(void (*entry_point)());

// Called by the timer interrupt to switch to the next ready process
void schedule(registers_t *regs);

void scheduler_print_processes(void);
int scheduler_kill_process(uint32_t pid);

#endif
