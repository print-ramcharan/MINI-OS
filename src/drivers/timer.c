#include "timer.h"
#include "idt.h"
#include "scheduler.h"

static inline void outb_timer(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint32_t tick = 0;

static void timer_callback(registers_t *regs) {
  tick++;
  // Update process sleep states
  scheduler_tick();
  // Trigger the scheduler to swap context running on this CPU
  schedule(regs);
}

void init_timer(uint32_t frequency) {
  register_interrupt_handler(32, timer_callback);

  // The value we send to the PIT is the value to divide it's input clock
  // (1193180 Hz) by, to get our required frequency. Important to note is
  // that the divisor must be small enough to fit into 16-bits.
  uint32_t divisor = 1193180 / frequency;

  // Send the command byte.
  outb_timer(0x43, 0x36);

  // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
  uint8_t l = (uint8_t)(divisor & 0xFF);
  uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

  // Send the frequency divisor.
  outb_timer(0x40, l);
  outb_timer(0x40, h);
}

uint32_t get_tick() { return tick; }

#include "vga.h"

void timer_print_uptime(void) {
  uint32_t total_sec = tick / 50;
  uint32_t hours = total_sec / 3600;
  uint32_t mins = (total_sec % 3600) / 60;
  uint32_t secs = total_sec % 60;

  print("System Uptime: ");
  print_dec(hours); print("h ");
  print_dec(mins); print("m ");
  print_dec(secs); print("s (");
  print_dec(tick); print(" ticks)\n");
}
