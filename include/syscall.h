#ifndef SYSCALL_H
#define SYSCALL_H

#include "idt.h"
#include <stdint.h>

#define SYS_WRITE 1
#define SYS_SLEEP 2
#define SYS_EXIT 3
#define SYS_YIELD 4

void syscall_init(void);

// Syscall handler triggered by int 0x80
void syscall_handler(registers_t *regs);

#endif
