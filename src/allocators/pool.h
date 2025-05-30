#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include <stdint.h>

typedef struct pool_t
{
	uint8_t* allocation;
	uint32_t head; // allocation relative address
	uint32_t alloc_size; // treated as NULL
	uint32_t chunk_size;
	uint32_t chunk_cap;
} pool_t;


#if __cplusplus
extern "C" {
#endif

void pool_init(pool_t* pool, uint32_t chunk_size, uint32_t chunk_cap);
void pool_free(pool_t* pool, void* ptr);
void* pool_calloc(pool_t* pool);
void pool_free_all(pool_t* pool);

#if __cplusplus
}
#endif

#endif /* End ALLOCATORS_H */
