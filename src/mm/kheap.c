#include "kheap.h"
#include "vga.h"

#define ALLOC_MAGIC 0x12345678
#define FREE_MAGIC 0x87654321

typedef struct heap_block {
  uint32_t magic;          // Magic number indicating allocated/free
  size_t size;             // Size of the block payload
  struct heap_block *next; // Pointer to the next block
} heap_block_t;

static heap_block_t *heap_start = NULL;

void kheap_init(uint32_t start_addr, uint32_t size) {
  heap_start = (heap_block_t *)start_addr;
  heap_start->magic = FREE_MAGIC;
  heap_start->size = size - sizeof(heap_block_t);
  heap_start->next = NULL;
}

void *kmalloc(size_t size) {
  if (size == 0)
    return NULL;

  // Align size to 4 bytes
  if (size % 4 != 0) {
    size += (4 - (size % 4));
  }

  heap_block_t *curr = heap_start;
  while (curr != NULL) {
    if (curr->magic == FREE_MAGIC && curr->size >= size) {
      // Found a block. Can we split it?
      if (curr->size > size + sizeof(heap_block_t) + 4) {
        heap_block_t *new_block =
            (heap_block_t *)((uint32_t)curr + sizeof(heap_block_t) + size);
        new_block->magic = FREE_MAGIC;
        new_block->size = curr->size - size - sizeof(heap_block_t);
        new_block->next = curr->next;

        curr->next = new_block;
        curr->size = size;
      }

      curr->magic = ALLOC_MAGIC;
      return (void *)((uint32_t)curr + sizeof(heap_block_t));
    }
    curr = curr->next;
  }

  print("[PANIC] Out of heap memory!\n");
  return NULL;
}

void kfree(void *ptr) {
  if (!ptr)
    return;

  heap_block_t *block = (heap_block_t *)((uint32_t)ptr - sizeof(heap_block_t));
  if (block->magic == ALLOC_MAGIC) {
    block->magic = FREE_MAGIC;

    // Coalesce adjacent free blocks
    heap_block_t *curr = heap_start;
    while (curr != NULL && curr->next != NULL) {
      if (curr->magic == FREE_MAGIC && curr->next->magic == FREE_MAGIC) {
        curr->size += curr->next->size + sizeof(heap_block_t);
        curr->next = curr->next->next;
      } else {
        curr = curr->next;
      }
    }
  } else {
    print("[PANIC] Invalid kfree pointer!\n");
  }
}

void kheap_get_stats(uint32_t *total_bytes, uint32_t *used_bytes, uint32_t *free_bytes) {
  uint32_t tot = 0;
  uint32_t usd = 0;
  uint32_t fre = 0;
  heap_block_t *curr = heap_start;
  while (curr != NULL) {
    tot += curr->size + sizeof(heap_block_t);
    if (curr->magic == ALLOC_MAGIC) {
      usd += curr->size;
    } else {
      fre += curr->size;
    }
    curr = curr->next;
  }
  if (total_bytes) *total_bytes = tot;
  if (used_bytes) *used_bytes = usd;
  if (free_bytes) *free_bytes = fre;
}
