#include "corearena.h"
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static volatile unsigned long checksum;
/* Volatile function pointers keep the compiler from eliminating heap calls. */
static void *(*volatile heap_alloc)(size_t) = malloc;
static void (*volatile heap_free)(void *) = free;
static double elapsed(clock_t start) { return (double)(clock() - start) / CLOCKS_PER_SEC; }
int main(void) {
    const size_t rounds = 1000, batch = 1000;
    ca_arena a; ca_pool pool;
    if (!ca_arena_init(&a, batch * 64) || !ca_pool_init(&pool, 32, batch, alignof(max_align_t))) return 1;
    clock_t start = clock();
    for (size_t r = 0; r < rounds; ++r) {
        for (size_t i = 0; i < batch; ++i) { unsigned char *p = ca_arena_alloc(&a, 32, 8); if (!p) return 1; p[0] = (unsigned char)i; checksum += p[0]; }
        ca_arena_reset(&a);
    }
    printf("arena: %.6f CPU seconds for %zu allocations\n", elapsed(start), rounds * batch);
    start = clock();
    for (size_t r = 0; r < rounds; ++r) {
        for (size_t i = 0; i < batch; ++i) { unsigned char *p = ca_pool_alloc(&pool); if (!p) return 1; p[0] = (unsigned char)i; checksum += p[0]; }
        ca_pool_reset(&pool);
    }
    printf("pool: %.6f CPU seconds for %zu allocations\n", elapsed(start), rounds * batch);
    start = clock();
    for (size_t r = 0; r < rounds; ++r) for (size_t i = 0; i < batch; ++i) {
        unsigned char *p = heap_alloc(32); if (!p) return 1; p[0] = (unsigned char)i; checksum += p[0]; heap_free(p);
    }
    printf("malloc/free: %.6f CPU seconds for %zu allocations\n", elapsed(start), rounds * batch);
    printf("checksum: %lu\n", checksum);
    ca_pool_destroy(&pool); ca_arena_destroy(&a); return 0;
}
