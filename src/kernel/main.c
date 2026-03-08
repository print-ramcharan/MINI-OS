#include "gdt.h"
#include "vga.h"

void kernel_main() {
  // Stage 1: Core CPU Setup
  gdt_install();

  // Stage 2: Hardware Init
  terminal_initialize();

  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
  print("==================================\n");
  print("       MINI OS KERNEL V1.0        \n");
  print("==================================\n\n");

  terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
  print("[OK] CPU: Global Descriptor Table loaded\n");
  print("[OK] DRV: VGA Text Mode initialized\n");

  while (1) {
  }
}
