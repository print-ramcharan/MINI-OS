#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096

// Initializes the physical memory manager given the size of the memory
void pmm_init(uint32_t mem_size_kb);

// Marks a physical region as in-use/reserved
void pmm_reserve_region(uint32_t start_addr, size_t size);

// Marks a physical region as free/available
void pmm_free_region(uint32_t start_addr, size_t size);

// Allocates a single 4KB page and returns its physical address
uint32_t pmm_alloc_page(void);

// Frees a single 4KB page
void pmm_free_page(uint32_t addr);

uint32_t pmm_get_max_blocks(void);
uint32_t pmm_get_used_blocks(void);
void pmm_print_map(void);

#endif
