# API reference

Include `corearena.h` and link `corearena`. Public structs expose counters for inspection; clients must not mutate allocator internals.

| Function | Behavior |
| --- | --- |
| `ca_arena_init(a, bytes)` | Own one allocation; reject zero capacity and failed malloc |
| `ca_arena_alloc(a, size, alignment)` | Return aligned bytes; reject zero size, invalid alignment, arithmetic overflow and exhaustion |
| `ca_arena_calloc(a, count, size, alignment)` | Check multiplication then zero payload |
| `ca_arena_mark(a)` | Record a rewind point; empty mark on uninitialized arena |
| `ca_arena_rewind(a, mark)` | Reclaim later data; reject wrong owner, stale generation, or future offset |
| `ca_arena_reset(a)` | Reclaim every allocation and expire marks |
| `ca_arena_destroy(a)` | Release storage and zero the object; repeat calls are safe |
| `ca_pool_init(p, size, count, alignment)` | Round stride to alignment and build free list |
| `ca_pool_alloc(p)` | Take one block; NULL when exhausted |
| `ca_pool_free(p, pointer)` | Validate address and ownership; return false on invalid free |
| `ca_pool_reset(p)` | Restore full free list and invalidate outstanding ownership |
| `ca_pool_destroy(p)` | Release payload and metadata; zero object |

`ca_stats.capacity` is arena bytes or pool payload stride × count; pool metadata is excluded. `used` includes arena padding or live pool strides. `live` counts currently allocated items, `peak` is maximum used bytes, and `allocations`/`failures` are cumulative allocation attempts accepted/rejected. Invalid free and invalid rewind are returned through boolean results and do not increment allocation failures. Counters survive reset and rewind. Generation wrap after 2^64 operations is outside the lifetime guarantee.

Alignment is any representable nonzero power of two, subject to capacity and checked arithmetic. An arena is not guaranteed to satisfy large alignment without spending leading padding. Allocator state must be zero initialized before calling destroy on an object that never initialized successfully.
