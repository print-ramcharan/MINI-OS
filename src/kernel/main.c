#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "timer.h"
#include "vga.h"

void kernel_main() {
  // Stage 1: Core CPU Setup
  gdt_install();
  idt_install();

  // Stage 2: Hardware Init
  terminal_initialize();

  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
  print("==================================\n");
  print("       MINI OS KERNEL V1.0        \n");
  print("==================================\n\n");

  terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
  print("[OK] CPU: Global Descriptor Table loaded\n");
  print("[OK] CPU: Interrupt Descriptor Table loaded\n");

  // Start Drivers
  init_timer(50); // 50 Hz
  init_keyboard();

  print("[OK] DRV: VGA Text Mode initialized\n");
  print("[OK] DRV: PIT Timer started (50Hz)\n");
  print("[OK] DRV: Keyboard driver started\n\n");

  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
  print("Welcome to MINI OS! Try typing on the keyboard...\n> ");

  while (1) {
  }
}
