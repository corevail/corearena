# Local verification — 2026-10-09

Verified on arm64 macOS with Apple Clang 21.0.0 and CMake. The local results below are separate from remote CI. The published source also passed the Linux/macOS GitHub Actions matrix: [C11 checks, run 37919158090](https://github.com/Borep1945/corearena/actions/runs/37919158090), commit `286d6240fadc37710e92c1b90d199930f629ebe7`.

- Debug build: C11, `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`, AddressSanitizer and UndefinedBehaviorSanitizer enabled.
- `ctest --test-dir build --output-on-failure`: 1/1 executable passed. Checks exercise multiple alignments, exhausted arenas/pools, arithmetic overflow, zero initialization, mark ownership and generation, repeated reset, foreign/interior/double free, and destruction.
- Release build and CTest: 1/1 executable passed; `-UNDEBUG` keeps test assertions active in Release builds.
- Frame example: three successful frames, 800 bytes per frame with reset between frames.

## Local benchmark sample

Release build, Apple Clang 21, arm64 host. One million 32-byte allocations per allocator, batches of 1,000, 1,000 rounds. Timings measure process CPU time, include payload writes/checksum and arena/pool resets, and compare heap allocation plus immediate free. Volatile function pointers prevent heap-call elimination. These are one local run, not a general performance guarantee.

| Allocator | CPU seconds |
| --- | ---: |
| Arena | 0.004367 |
| Pool | 0.005141 |
| malloc/free | 0.024010 |

Checksum: `374148000`. Run `./build-release/corearena_bench` to measure your own machine. Runtime reset invalidation is a logical ownership contract; sanitizers do not detect all stale accesses inside retained allocations.
