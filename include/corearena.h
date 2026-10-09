#ifndef COREARENA_H
#define COREARENA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Counters are cumulative until destruction; used and live are current. */
typedef struct {
    size_t capacity, used, peak, live, allocations, failures;
} ca_stats;
typedef struct {
    unsigned char *memory;
    size_t offset;
    uint64_t generation;
    ca_stats stats;
} ca_arena;
typedef struct { size_t offset, live; uint64_t generation; const ca_arena *owner; } ca_mark;
/** Initialize an empty arena; zero capacity is rejected. */
bool ca_arena_init(ca_arena *arena, size_t capacity);
/** alignment must be a nonzero power of two. size=0 is rejected. */
void *ca_arena_alloc(ca_arena *arena, size_t size, size_t alignment);
/** Zero-initialized allocation, with multiplication overflow checking. */
void *ca_arena_calloc(ca_arena *arena, size_t count, size_t size, size_t alignment);
ca_mark ca_arena_mark(const ca_arena *arena);
/** Rewind invalidates marks and pointers after the target; all old marks expire. */
bool ca_arena_rewind(ca_arena *arena, ca_mark mark);
/** Invalidates every pointer and mark. Storage remains available. */
void ca_arena_reset(ca_arena *arena);
void ca_arena_destroy(ca_arena *arena);

typedef struct {
    unsigned char *raw, *memory;
    size_t *next;
    unsigned char *occupied;
    size_t stride, count, head;
    ca_stats stats;
} ca_pool;
/** Fixed-size, fixed-alignment blocks. Overflow and invalid arguments fail. */
bool ca_pool_init(ca_pool *pool, size_t block_size, size_t count, size_t alignment);
void *ca_pool_alloc(ca_pool *pool);
/** Foreign, interior and already freed pointers are rejected. */
bool ca_pool_free(ca_pool *pool, void *pointer);
/** Invalidates every outstanding block and resets the free list. */
void ca_pool_reset(ca_pool *pool);
void ca_pool_destroy(ca_pool *pool);
#endif
