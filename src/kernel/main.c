#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "kheap.h"
#include "multiboot.h"
#include "paging.h"
#include "pmm.h"
#include "scheduler.h"
#include "syscall.h"
#include "timer.h"
#include "vga.h"
#include "shell.h"

// Defined in linker.ld
extern uint32_t __kernel_start;
extern uint32_t __kernel_end;

void yield() {
  asm volatile("mov $4, %%eax; int $0x80" ::: "eax");
}

void sleep(uint32_t ticks) {
  asm volatile("mov $2, %%eax; mov %0, %%ebx; int $0x80"
               :
               : "r"(ticks)
               : "eax", "ebx");
}

void task_exit() {
  asm volatile("mov $3, %%eax; int $0x80" ::: "eax");
}

char read_char() {
  uint32_t val;
  asm volatile("mov $5, %%eax; int $0x80; mov %%eax, %0"
               : "=r"(val)
               :
               : "eax");
  return (char)val;
}

void task_a() {
  char *hello = " [Syscall from Task A!] ";

  // Invoke SYS_WRITE (eax=1, ebx=string_ptr)
  asm volatile("mov $1, %%eax; mov %0, %%ebx; int $0x80"
               :
               : "r"(hello)
               : "eax", "ebx");

  for (int i = 0; i < 5; i++) {
    print("A");
    sleep(50); // Sleep for 1 second (50 ticks)
  }
  task_exit();
}

void task_b() {
  print(" [Task B: Interactive mode! Type keys to echo them.] \n");
  for (int i = 0; i < 200; i++) {
    char c = read_char();
    if (c != 0) {
      print(" [Typed: ");
      char str[2] = {c, 0};
      print(str);
      print("] ");
    }
    sleep(5); // Sleep 100ms
  }
  task_exit();
}

void kernel_main(struct multiboot_info *mbd, uint32_t magic) {
  // Stage 1: Core CPU Setup
  gdt_install();
  idt_install();
  syscall_init();

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

  // Enable Virtual Memory mapping
  init_paging();
  print("[OK] MEM: Virtual Memory (Paging) enabled\n");

  // Initialize Kernel Heap Allocator
  // Allocate 4 pages for heap testing and thread stacks initially
  uint32_t heap_phys = pmm_alloc_page();
  pmm_alloc_page();
  pmm_alloc_page();
  pmm_alloc_page();

  // Map one 4KB page for testing the heap initially
  map_page(heap_phys, heap_phys, PAGE_PRESENT | PAGE_RW);
  map_page(heap_phys + PAGE_SIZE, heap_phys + PAGE_SIZE,
           PAGE_PRESENT | PAGE_RW);
  map_page(heap_phys + PAGE_SIZE * 2, heap_phys + PAGE_SIZE * 2,
           PAGE_PRESENT | PAGE_RW);
  map_page(heap_phys + PAGE_SIZE * 3, heap_phys + PAGE_SIZE * 3,
           PAGE_PRESENT | PAGE_RW);
  kheap_init(heap_phys, PAGE_SIZE * 4);

  print("[OK] MEM: Kernel dynamic heap allocator initialized\n");

  // Start Drivers
  init_timer(50); // 50 Hz
  init_keyboard();

  print("[OK] DRV: VGA Text Mode initialized\n");
  print("[OK] DRV: PIT Timer started (50Hz)\n");
  print("[OK] DRV: Keyboard driver started\n\n");

  terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
  print("Memory subsystems passed CR3 / Malloc tests!\n");

  // kmalloc test
  uint32_t *test_ptr_1 = (uint32_t *)kmalloc(sizeof(uint32_t) * 10);
  print("`kmalloc` test -> allocated 40 bytes at: ");
  print_hex((uint32_t)test_ptr_1);
  print("\n");
  uint32_t *test_ptr_2 = (uint32_t *)kmalloc(sizeof(uint32_t) * 10);
  print("`kmalloc` test -> allocated 40 bytes at: ");
  print_hex((uint32_t)test_ptr_2);
  print("\n");
  print("`kfree` -> freeing first chunk... \n");
  kfree(test_ptr_1);
  print("Starting Process Scheduler demonstration...\n");

  scheduler_init();
  extern void enable_scheduler();
  create_process(shell_task);
  enable_scheduler();

  // To test page fault uncomment this line:
  // uint32_t *ptr = (uint32_t*)0xA0000000;
  // uint32_t do_page_fault = *ptr;

  while (1) {
  }
}
