#ifndef PAGING_H
#define PAGING_H

#include "idt.h"
#include <stdint.h>

// Page Table / Directory entry flags
#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4

typedef uint32_t page_entry_t;

// A page table contains 1024 pages
typedef struct page_table {
  page_entry_t pages[1024];
} page_table_t;

// A page directory contains 1024 page tables
typedef struct page_directory {
  page_entry_t tables[1024];
} page_directory_t;

void init_paging(void);
void map_page(uint32_t phys, uint32_t virt, uint32_t flags);
void unmap_page(uint32_t virt);
void page_fault(registers_t *regs);

#endif
