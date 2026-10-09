#include "corearena.h"
#include <stdlib.h>
#include <string.h>

static bool power_two(size_t n) { return n != 0 && (n & (n - 1)) == 0; }
static bool add(size_t a, size_t b, size_t *out) {
    if (a > SIZE_MAX - b) return false;
    *out = a + b; return true;
}
static bool multiply(size_t a, size_t b, size_t *out) {
    if (a != 0 && b > SIZE_MAX / a) return false;
    *out = a * b; return true;
}
static void fail(ca_stats *s) { if (s->failures < SIZE_MAX) ++s->failures; }
static void record(ca_stats *s) {
    if (s->allocations < SIZE_MAX) ++s->allocations;
    if (s->used > s->peak) s->peak = s->used;
}
static void advance_generation(ca_arena *a) {
    /* Practically unreachable wrap: keep zero reserved for invalid marks. */
    ++a->generation;
    if (a->generation == 0) a->generation = 1;
}
bool ca_arena_init(ca_arena *a, size_t capacity) {
    if (!a || capacity == 0) return false;
    *a = (ca_arena){0};
    a->memory = malloc(capacity);
    if (!a->memory) return false;
    a->generation = 1; a->stats.capacity = capacity;
    return true;
}
void *ca_arena_alloc(ca_arena *a, size_t size, size_t alignment) {
    if (!a) return NULL;
    if (!a->memory || size == 0 || !power_two(alignment)) { fail(&a->stats); return NULL; }
    uintptr_t current = (uintptr_t)(a->memory + a->offset);
    size_t padding = (size_t)((0 - current) & (uintptr_t)(alignment - 1));
    size_t aligned, end;
    if (!add(a->offset, padding, &aligned) || !add(aligned, size, &end) || end > a->stats.capacity) {
        fail(&a->stats); return NULL;
    }
    void *result = a->memory + aligned;
    a->offset = end; a->stats.used = end; ++a->stats.live; record(&a->stats);
    return result;
}
void *ca_arena_calloc(ca_arena *a, size_t count, size_t size, size_t alignment) {
    size_t bytes;
    if (!multiply(count, size, &bytes)) { if (a) fail(&a->stats); return NULL; }
    void *result = ca_arena_alloc(a, bytes, alignment);
    if (result) memset(result, 0, bytes);
    return result;
}
ca_mark ca_arena_mark(const ca_arena *a) {
    return a && a->memory ? (ca_mark){a->offset, a->stats.live, a->generation, a} : (ca_mark){0};
}
bool ca_arena_rewind(ca_arena *a, ca_mark mark) {
    if (!a || !a->memory || mark.owner != a || mark.generation != a->generation || mark.offset > a->offset) return false;
    a->offset = mark.offset; a->stats.used = mark.offset;
    a->stats.live = mark.live; advance_generation(a); return true;
}
void ca_arena_reset(ca_arena *a) {
    if (!a || !a->memory) return;
    a->offset = 0; a->stats.used = 0; a->stats.live = 0; advance_generation(a);
}
void ca_arena_destroy(ca_arena *a) {
    if (!a) return;
    free(a->memory); *a = (ca_arena){0};
}
bool ca_pool_init(ca_pool *p, size_t block_size, size_t count, size_t alignment) {
    if (!p || block_size == 0 || count == 0 || !power_two(alignment)) return false;
    *p = (ca_pool){0};
    size_t padded, stride, bytes, raw_size, index_size;
    if (!add(block_size, alignment - 1, &padded)) return false;
    stride = padded & ~(alignment - 1);
    if (!multiply(stride, count, &bytes) || !add(bytes, alignment - 1, &raw_size) || !multiply(count, sizeof(size_t), &index_size)) return false;
    p->raw = malloc(raw_size); p->next = malloc(index_size); p->occupied = calloc(count, 1);
    if (!p->raw || !p->next || !p->occupied) { ca_pool_destroy(p); return false; }
    uintptr_t start = (uintptr_t)p->raw;
    size_t padding = (size_t)((0 - start) & (uintptr_t)(alignment - 1));
    p->memory = p->raw + padding; p->stride = stride; p->count = count;
    p->stats.capacity = bytes; ca_pool_reset(p); return true;
}
void *ca_pool_alloc(ca_pool *p) {
    if (!p) return NULL;
    if (!p->memory || p->head == SIZE_MAX) { fail(&p->stats); return NULL; }
    size_t index = p->head; p->head = p->next[index]; p->occupied[index] = 1;
    ++p->stats.live; p->stats.used += p->stride; record(&p->stats);
    return p->memory + index * p->stride;
}
bool ca_pool_free(ca_pool *p, void *pointer) {
    if (!p || !p->memory || !pointer) return false;
    uintptr_t address = (uintptr_t)pointer, base = (uintptr_t)p->memory;
    if (address < base) return false;
    uintptr_t delta = address - base;
    if (delta >= p->stats.capacity || delta % p->stride != 0) return false;
    size_t index = (size_t)(delta / p->stride);
    if (!p->occupied[index]) return false;
    p->occupied[index] = 0; p->next[index] = p->head; p->head = index;
    --p->stats.live; p->stats.used -= p->stride; return true;
}
void ca_pool_reset(ca_pool *p) {
    if (!p || !p->memory) return;
    for (size_t i = 0; i < p->count; ++i) p->next[i] = i + 1 < p->count ? i + 1 : SIZE_MAX;
    memset(p->occupied, 0, p->count); p->head = 0; p->stats.live = 0; p->stats.used = 0;
}
void ca_pool_destroy(ca_pool *p) {
    if (!p) return;
    free(p->raw); free(p->next); free(p->occupied); *p = (ca_pool){0};
}
