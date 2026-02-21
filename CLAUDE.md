# CLAUDE.md — Arena Allocator with Snapshot

This file provides guidance for AI assistants working on this codebase.

---

## Project Overview

A single-header-style C library implementing a **region/arena memory allocator** with:
- O(1) bump-pointer allocation
- Snapshot/rollback for reversible allocation phases
- Guard-pattern-based buffer overflow detection
- Explicit alignment support

The library is designed to be embedded in projects where predictable, fast allocation and bulk deallocation matter (parsers, game frames, request-scoped allocators, etc.).

---

## Repository Layout

```
arena-snapshot/
├── include/
│   └── arena.h       # Public API — types and function declarations
├── src/
│   ├── arena.c       # Implementation
│   └── test.c        # Test driver (not yet created; required by Makefile)
├── build/            # Compiler output — created by make, gitignored by convention
├── Makefile
└── README.md         # Korean-language project documentation
```

There is intentionally **no external dependency** — only the C11 standard library (`stdlib.h`, `stddef.h`, `stdint.h`).

---

## Technology Stack

| Concern       | Choice              |
|---------------|---------------------|
| Language      | C11                 |
| Compiler      | GCC                 |
| Build system  | GNU Make            |
| Test runner   | Manual (`make run`) |
| CI/CD         | None configured     |

---

## Build Commands

```bash
make          # compile arena.c + test.c → ./test
make run      # compile then execute ./test
make clean    # remove build/ directory and ./test binary
```

**Makefile variables (src/Makefile line 1-6):**
- `CC = gcc`
- `CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude`
- `BUILD_DIR = build`
- `TARGET = test`

The Makefile compiles `src/arena.c` and `src/test.c` separately into `build/*.o`, then links them. **`src/test.c` does not exist yet** — `make` will fail until it is created.

---

## Public API (`include/arena.h`)

### Types

```c
typedef struct {
    char*  buffer;    // heap-allocated backing store
    size_t capacity;  // total bytes in buffer
    size_t offset;    // next free byte index (bump pointer)
} Arena;

typedef size_t Snapshot;  // opaque offset value, used for rollback
```

### Functions

| Function | Signature | Purpose |
|---|---|---|
| `arena_create` | `Arena* (size_t capacity)` | Heap-allocate an Arena with a backing buffer of `capacity` bytes. Returns `NULL` on failure or if `capacity == 0`. |
| `arena_destroy` | `void (Arena*)` | Free the backing buffer and the Arena struct. No-op if `NULL`. |
| `arena_alloc` | `void* (Arena*, size_t size, size_t align)` | Bump-allocate `size` bytes aligned to `align` (must be a power of two). Returns `NULL` on overflow or capacity exhaustion. |
| `arena_alloc_guarded` | `void* (Arena*, size_t size, size_t align)` | Like `arena_alloc`, but places a `GuardHeader` + sentinel before the allocation and a sentinel after it for overflow detection. |
| `arena_check_guard` | `int (void* data_ptr)` | Returns `1` if both guard sentinels around `data_ptr` are intact; `0` if corrupted or pointer is `NULL`. |
| `arena_snapshot` | `Snapshot (Arena*)` | Capture the current offset. Returns `0` for `NULL` arena. |
| `arena_rollback` | `void (Arena*, Snapshot)` | Restore the offset to a previously captured snapshot, reclaiming everything allocated after it. No-op if snapshot is in the future (greater than current offset). |
| `arena_reset` | `void (Arena*)` | Reset offset to `0`, effectively freeing all allocations without releasing the backing buffer. |

---

## Implementation Details (`src/arena.c`)

### Guard pattern internals

Guarded allocations use the pattern `0xDEADBEEFDEADBEEFULL` (a `uint64_t`).

Memory layout for `arena_alloc_guarded(arena, size, align)`:

```
[padding][GuardHeader (8 bytes)][front guard (8 bytes)][data (size bytes)][back guard (8 bytes)]
 ^raw_ptr                                               ^returned data_ptr
```

- `GuardHeader.size` stores the original `size` so `arena_check_guard` can find the back sentinel.
- Padding is computed so `data_ptr` satisfies `align`.
- The entire region is allocated as a single call to `arena_alloc` with alignment 1, consuming `max_padding + prefix_size + size + ARENA_GUARD_SIZE` bytes.

### Overflow/alignment safety in `arena_alloc`

Two integer-overflow checks are performed before advancing the offset:
1. `aligned_offset < arena->offset` — catches wrap-around from the alignment calculation.
2. `new_offset < aligned_offset || new_offset > arena->capacity` — catches overflow from `aligned_offset + size` and capacity exhaustion.

`align` must be a power of two; the function returns `NULL` if `(align & (align - 1)) != 0`.

---

## Code Conventions

- **C11 standard** only — no compiler extensions.
- **Defensive NULL checks** at the top of every function.
- **No error codes** — functions return `NULL` or `0` on failure (consistent with malloc-family style).
- **No global state** — all state lives inside the `Arena` struct.
- **Internal-only types** (`guard_t`, `GuardHeader`) are defined in `arena.c`, not in the header.
- Commit messages follow **Conventional Commits** with Korean descriptions:
  - `feat:` — new functionality
  - `fix:` — bug fixes
  - `refactor:` — non-functional restructuring
  - `docs:` — documentation only

---

## Adding Tests (`src/test.c`)

The Makefile expects `src/test.c`. The test binary is the sole executable target. Follow these conventions when writing tests:

- Include `"arena.h"` and `<stdio.h>` / `<string.h>` as needed.
- Print `PASS` / `FAIL` per test case and return a non-zero exit code on failure so `make run` reflects failures.
- Cover: `arena_create` edge cases (0 capacity, large capacity), `arena_alloc` alignment and overflow, `arena_snapshot` + `arena_rollback`, `arena_reset`, and guard pattern detection.

Example minimal structure:

```c
#include "arena.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    Arena* a = arena_create(4096);
    assert(a != NULL);

    void* p = arena_alloc(a, 64, 8);
    assert(p != NULL);

    Snapshot snap = arena_snapshot(a);
    void* q = arena_alloc(a, 128, 8);
    assert(q != NULL);
    arena_rollback(a, snap);

    // After rollback, same address should be re-allocated
    void* r = arena_alloc(a, 128, 8);
    assert(r == q);

    arena_destroy(a);
    puts("All tests passed.");
    return 0;
}
```

---

## Known Gaps / Limitations

- `src/test.c` is **missing** — `make` fails without it.
- No CI pipeline — tests must be run locally with `make run`.
- Individual allocations **cannot be freed**; only bulk reset/rollback is supported.
- Arena capacity is fixed at creation time; there is no growth/reallocation.
- `arena_alloc_guarded` is intended for **debug/testing builds** only; the overhead (padding + two sentinels + header) is significant relative to small allocations.
- `arena_check_guard` trusts that `data_ptr` was returned by `arena_alloc_guarded`. Passing an arbitrary pointer is undefined behaviour.

---

## Git Workflow

- Primary development branch: `master`
- Feature/AI branches follow the naming pattern: `claude/<descriptor>-<session-id>`
- Commit messages are written in Korean with English Conventional Commit prefixes.
- No automated hooks or CI checks are present; linting and testing must be done manually before committing.
