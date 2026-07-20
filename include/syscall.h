#ifndef SYSCALL_H
#define SYSCALL_H

#include "idt.h"
#include <stdint.h>

#define SYS_WRITE 1
#define SYS_SLEEP 2
#define SYS_EXIT 3
#define SYS_YIELD 4
#define SYS_READ_CHAR 5
#define SYS_OPEN 6
#define SYS_READ 7
#define SYS_WRITE_FILE 8
#define SYS_CLOSE 9
#define SYS_DELETE 10
#define SYS_SET_PRIORITY 11

void syscall_init(void);

// Syscall handler triggered by int 0x80
void syscall_handler(registers_t *regs);

#endif
