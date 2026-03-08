#include "paging.h"
#include "pmm.h"
#include "vga.h"

// The kernel's page directory
page_directory_t *kernel_directory = 0;

static inline void write_cr3(uint32_t val) {
  asm volatile("mov %0, %%cr3" : : "r"(val));
}

static inline void write_cr0(uint32_t val) {
  asm volatile("mov %0, %%cr0" : : "r"(val));
}

static inline uint32_t read_cr0() {
  uint32_t val;
  asm volatile("mov %%cr0, %0" : "=r"(val));
  return val;
}

static inline uint32_t read_cr2() {
  uint32_t val;
  asm volatile("mov %%cr2, %0" : "=r"(val));
  return val;
}

void map_page(uint32_t phys, uint32_t virt, uint32_t flags) {
  uint32_t pd_index = virt >> 22;
  uint32_t pt_index = (virt >> 12) & 0x03FF;

  if (!(kernel_directory->tables[pd_index] & PAGE_PRESENT)) {
    // We need to allocate a new page table
    uint32_t pt_phys = pmm_alloc_page();
    if (!pt_phys) {
      print("[PANIC] Out of memory mapping page table!\n");
      for (;;)
        ;
    }

    // Zero out the page table (we can directly access it since we identity
    // mapped memory initially)
    page_table_t *pt = (page_table_t *)pt_phys;
    for (int i = 0; i < 1024; i++) {
      pt->pages[i] = 0;
    }

    kernel_directory->tables[pd_index] =
        pt_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;
  }

  page_table_t *pt =
      (page_table_t *)(kernel_directory->tables[pd_index] & ~0xFFF);
  pt->pages[pt_index] = phys | flags;
}

void unmap_page(uint32_t virt) {
  uint32_t pd_index = virt >> 22;
  uint32_t pt_index = (virt >> 12) & 0x03FF;

  if (kernel_directory->tables[pd_index] & PAGE_PRESENT) {
    page_table_t *pt =
        (page_table_t *)(kernel_directory->tables[pd_index] & ~0xFFF);
    pt->pages[pt_index] = 0;
  }
}

void page_fault(registers_t *regs) {
  uint32_t faulting_address = read_cr2();

  // The error code gives us details of what happened.
  int present = !(regs->err_code & 0x1); // Page not present
  int rw = regs->err_code & 0x2;         // Write operation?
  int us = regs->err_code & 0x4;         // Processor was in user-mode?
  int reserved =
      regs->err_code & 0x8; // Overwritten CPU-reserved bits of page entry?
  int id = regs->err_code & 0x10; // Caused by an instruction fetch?

  terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
  print("Page fault! ( ");
  if (present)
    print("present ");
  if (rw)
    print("read-only ");
  if (us)
    print("user-mode ");
  if (reserved)
    print("reserved ");
  print(") at 0x");
  print_hex(faulting_address);
  print(" - EIP: ");
  print_hex(regs->eip);
  print("\n");
  for (;;)
    ;
}

void init_paging() {
  // 1. Allocate a page directory
  uint32_t pd_phys = pmm_alloc_page();
  kernel_directory = (page_directory_t *)pd_phys;

  for (int i = 0; i < 1024; i++) {
    kernel_directory->tables[i] = 2; // User layout, not present but read/write
  }

  // 2. Identity map the kernel memory and hardware ports (0 to 16MB)
  // This allows our physical addresses to equal our virtual addresses in the
  // kernel.
  for (uint32_t i = 0; i < 16 * 1024 * 1024; i += PAGE_SIZE) {
    map_page(i, i, PAGE_PRESENT | PAGE_RW);
  }

  // 3. Register our page fault handler
  register_interrupt_handler(14, page_fault);

  // 4. Enable paging
  write_cr3((uint32_t)kernel_directory);
  uint32_t cr0 = read_cr0();
  cr0 |= 0x80000000; // Enable paging bit
  write_cr0(cr0);
}
