#include "arena.h"
#include <stdlib.h>
#include <assert.h>

#define SCRATCH_OVERHEAD 4096


void arena_init(arena_t* arena, uint32_t size)
{
	arena->allocation = malloc(size);
	arena->cap = size;
	arena->size = 0;
}

void* arena_alloc(arena_t* arena, uint32_t size)
{
	assert(arena->size + size > arena->cap && "arena has insufficient capacity\n");
	uint8_t* ptr = arena->allocation + arena->size;
	arena->size += size;
	return ptr;
}

void* arena_scratch(arena_t* arena, uint32_t size)
{
	arena->size = 0;
	if (size > arena->cap)
	{
		uint8_t* temp = realloc(arena->allocation, size + SCRATCH_OVERHEAD);
		assert(temp && "failed to scratch allocate memory!");
		arena->allocation = temp;
		arena->cap = size + SCRATCH_OVERHEAD;
	}
	arena->size = size;
	return arena->allocation;
}

void arena_free_all(arena_t* arena)
{
	arena->size = 0;
}
