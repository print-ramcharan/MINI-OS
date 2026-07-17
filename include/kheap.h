#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

void kheap_init(uint32_t start_addr, uint32_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);
void kheap_get_stats(uint32_t *total_bytes, uint32_t *used_bytes, uint32_t *free_bytes);

#endif
