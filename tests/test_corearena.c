#include "corearena.h"
#include <assert.h>
#include <stdalign.h>
#include <stdio.h>
#include <string.h>

static void arena_tests(void) {
    ca_arena a = {0}, other = {0};
    assert(!ca_arena_init(NULL, 100)); assert(!ca_arena_init(&a, 0));
    assert(ca_arena_init(&a, 8192)); assert(ca_arena_init(&other, 64));
    assert(ca_arena_alloc(&a, 0, 8) == NULL); assert(ca_arena_alloc(&a, 8, 3) == NULL);
    assert(ca_arena_alloc(&a, SIZE_MAX, 8) == NULL);
    assert(ca_arena_calloc(&a, SIZE_MAX, 2, 8) == NULL);
    for (size_t alignment = 1; alignment <= 1024; alignment *= 2) {
        void *pointer = ca_arena_alloc(&a, 3, alignment);
        assert(pointer && ((uintptr_t)pointer % alignment) == 0);
    }
    ca_mark mark = ca_arena_mark(&a);
    unsigned char *zero = ca_arena_calloc(&a, 8, 7, 8); assert(zero);
    for (size_t i = 0; i < 56; ++i) assert(zero[i] == 0);
    assert(!ca_arena_rewind(&other, mark));
    assert(ca_arena_rewind(&a, mark)); assert(a.stats.used == mark.offset); assert(a.stats.live == mark.live);
    assert(!ca_arena_rewind(&a, mark)); /* single-use mark */
    mark = ca_arena_mark(&a); ca_arena_reset(&a); assert(!ca_arena_rewind(&a, mark));
    assert(a.stats.used == 0 && a.stats.live == 0 && a.stats.failures == 4);
    assert(ca_arena_alloc(&a, 8192, 1)); assert(!ca_arena_alloc(&a, 1, 1));
    assert(a.stats.peak == 8192);
    ca_arena_destroy(&a); ca_arena_destroy(&a); ca_arena_destroy(&other);
    assert(!ca_arena_alloc(&a, 1, 1));
}
static void pool_tests(void) {
    ca_pool p = {0}; int foreign = 0;
    assert(!ca_pool_init(&p, 0, 4, 8)); assert(!ca_pool_init(&p, 8, 0, 8));
    assert(!ca_pool_init(&p, 8, 4, 3)); assert(!ca_pool_init(&p, SIZE_MAX, 4, 8));
    assert(!ca_pool_init(&p, 8, SIZE_MAX, 8));
    assert(ca_pool_init(&p, 17, 32, 64)); void *blocks[32];
    for (size_t i = 0; i < 32; ++i) { blocks[i] = ca_pool_alloc(&p); assert(blocks[i]); assert((uintptr_t)blocks[i] % 64 == 0); memset(blocks[i], 7, 17); }
    assert(!ca_pool_alloc(&p)); assert(p.stats.live == 32);
    assert(!ca_pool_free(&p, &foreign)); assert(!ca_pool_free(&p, NULL));
    assert(!ca_pool_free(&p, (unsigned char *)blocks[0] + 1));
    assert(!ca_pool_free(&p, p.memory + p.stats.capacity));
    assert(ca_pool_free(&p, blocks[7])); assert(!ca_pool_free(&p, blocks[7]));
    assert(ca_pool_alloc(&p) == blocks[7]);
    for (size_t i = 0; i < 32; ++i) assert(ca_pool_free(&p, blocks[i]));
    assert(p.stats.live == 0 && p.stats.used == 0);
    for (size_t round = 0; round < 1000; ++round) {
        for (size_t i = 0; i < 32; ++i) assert(ca_pool_alloc(&p));
        ca_pool_reset(&p);
    }
    assert(p.stats.peak == 2048);
    ca_pool_destroy(&p); ca_pool_destroy(&p); assert(!ca_pool_alloc(&p));
}
int main(void) { arena_tests(); pool_tests(); puts("corearena: arena and pool checks passed"); return 0; }
