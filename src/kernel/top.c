#include "top.h"
#include "vga.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "scheduler.h"
#include "keyboard.h"

static void top_render_header(void) {
  terminal_initialize();
  print("================================================================================\n");
  print("                       MINI-OS KERNEL SYSTEM MONITOR (top)                     \n");
  print("================================================================================\n");

  timer_print_uptime();

  uint32_t max_pages = pmm_get_max_blocks();
  uint32_t used_pages = pmm_get_used_blocks();
  print("PMM Memory:  "); print_dec(used_pages * 4); print(" KB / "); print_dec(max_pages * 4); print(" KB used\n");

  uint32_t heap_tot = 0, heap_usd = 0, heap_fre = 0;
  kheap_get_stats(&heap_tot, &heap_usd, &heap_fre);
  print("Heap Memory: "); print_dec(heap_usd); print(" B / "); print_dec(heap_tot); print(" B used\n");
  print("--------------------------------------------------------------------------------\n");
}

static char top_read_char(void) {
  uint32_t val;
  asm volatile("mov $5, %%eax; int $0x80; mov %%eax, %0" : "=r"(val) :: "eax");
  return (char)val;
}

static void top_sleep(uint32_t ticks) {
  asm volatile("mov $2, %%eax; mov %0, %%ebx; int $0x80" :: "r"(ticks) : "eax", "ebx");
}

static void top_exit(void) {
  asm volatile("mov $3, %%eax; int $0x80" ::: "eax");
}

void top_task(void) {
  top_render_header();
  scheduler_print_processes();
  print("--------------------------------------------------------------------------------\n");
  print("Press any key to exit top monitor...\n");

  while (1) {
    char c = top_read_char();
    if (c != 0) {
      break;
    }
    top_sleep(5);
  }
  top_exit();
}
