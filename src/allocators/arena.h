#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

typedef struct arena_t
{
	uint8_t* allocation;
	uint32_t size;
	uint32_t cap;
} arena_t;

#ifdef __cplusplus
extern "C" {
#endif

void arena_init(arena_t* arena, uint32_t size);
void* arena_alloc(arena_t* arena, uint32_t size);
void* arena_scratch(arena_t* arena, uint32_t size);
void arena_free_all(arena_t* arena);

#ifdef __cplusplus
}
#endif

#endif /* End ARENA_H */
