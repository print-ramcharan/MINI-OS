#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "multiboot.h"
#include "pmm.h"
#include "timer.h"
#include "vga.h"

// Defined in linker.ld
extern uint32_t __kernel_start;
extern uint32_t __kernel_end;

void kernel_main(struct multiboot_info *mbd, uint32_t magic) {
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

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    print("[FAIL] Invalid magic number from bootloader\n");
    return;
  }

  print("[OK] CPU: Global Descriptor Table loaded\n");
  print("[OK] CPU: Interrupt Descriptor Table loaded\n");

  // Memory calculation
  uint32_t mem_size_kb = 1024 + mbd->mem_lower + mbd->mem_upper;
  pmm_init(mem_size_kb);

  // Parse multiboot memory map to find free regions
  multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbd->mmap_addr;
  while ((uint32_t)mmap < mbd->mmap_addr + mbd->mmap_length) {
    if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
      pmm_free_region(mmap->addr_low, mmap->len_low);
    }
    mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size +
                                      sizeof(uint32_t));
  }

  // Reserve the kernel memory itself so we don't overwrite it!
  uint32_t kernel_start = (uint32_t)&__kernel_start;
  uint32_t kernel_end = (uint32_t)&__kernel_end;
  uint32_t kernel_size = kernel_end - kernel_start;
  pmm_reserve_region(kernel_start, kernel_size);

  // Also reserve the 0x0 to 0x100000 region (first 1MB: bios, VBE, grub, etc.)
  pmm_reserve_region(0x0, 0x100000);

  print("[OK] MEM: Physical Memory Manager (PMM) setup done\n");

  // Start Drivers
  init_timer(50); // 50 Hz
  init_keyboard();

  print("[OK] DRV: VGA Text Mode initialized\n");
  print("[OK] DRV: PIT Timer started (50Hz)\n");
  print("[OK] DRV: Keyboard driver started\n\n");

  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
  print("Physical memory mapped successfully!\n");
  print("Trying memory allocation test: \n");
  uint32_t p = pmm_alloc_page();
  print("Allocated page at: ");
  print_hex(p);
  print("\n");
  pmm_free_page(p);
  print("> ");

  while (1) {
  }
}
