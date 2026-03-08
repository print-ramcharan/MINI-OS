#include "pmm.h"
#include "vga.h"

// We assume a max of 4GB RAM.
// 4GB / 4KB = 1048576 pages.
// 1048576 bits = 131072 bytes array.
uint8_t pmm_bitmap[131072];
uint32_t pmm_max_blocks = 0;
uint32_t pmm_used_blocks = 0;

static inline void bitmap_set(int bit) {
  pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(int bit) {
  pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline uint8_t bitmap_test(int bit) {
  return pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(uint32_t mem_size_kb) {
  // Determine the number of 4KB blocks
  pmm_max_blocks = (mem_size_kb * 1024) / PAGE_SIZE;
  pmm_used_blocks = pmm_max_blocks;

  // By default, set all blocks as used (we only free what multiboot tells us is
  // free)
  for (size_t i = 0; i < sizeof(pmm_bitmap); i++) {
    pmm_bitmap[i] = 0xFF;
  }
}

void pmm_reserve_region(uint32_t start_addr, size_t size) {
  uint32_t align_start = start_addr / PAGE_SIZE;
  uint32_t blocks = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  for (uint32_t i = 0; i < blocks; i++) {
    bitmap_set(align_start + i);
    pmm_used_blocks++;
  }
}

void pmm_free_region(uint32_t start_addr, size_t size) {
  uint32_t align_start = start_addr / PAGE_SIZE;
  uint32_t blocks = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  for (uint32_t i = 0; i < blocks; i++) {
    bitmap_clear(align_start + i);
    pmm_used_blocks--;
  }
}

uint32_t pmm_alloc_page(void) {
  for (uint32_t i = 0; i < pmm_max_blocks; i++) {
    if (!bitmap_test(i)) {
      bitmap_set(i);
      pmm_used_blocks++;
      return i * PAGE_SIZE;
    }
  }
  return 0; // Out of memory
}

void pmm_free_page(uint32_t addr) {
  uint32_t block = addr / PAGE_SIZE;
  bitmap_clear(block);
  pmm_used_blocks--;
}
