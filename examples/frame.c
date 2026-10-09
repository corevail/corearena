#include "corearena.h"
#include <stdalign.h>
#include <stdio.h>
typedef struct { float x, y; } point;
int main(void) {
    ca_arena arena;
    if (!ca_arena_init(&arena, 65536)) return 1;
    for (int frame = 0; frame < 3; ++frame) {
        point *points = ca_arena_calloc(&arena, 100, sizeof(point), alignof(point));
        if (!points) { ca_arena_destroy(&arena); return 1; }
        points[0].x = (float)frame;
        printf("frame %d: %zu bytes, first x %.0f\n", frame, arena.stats.used, (double)points[0].x);
        ca_arena_reset(&arena); /* points must never be used after this line. */
    }
    ca_arena_destroy(&arena); return 0;
}
