#ifndef ARENA_H
#define ARENA_H
#include <stddef.h>
typedef struct {
    char* buffer;
    size_t capacity;
    size_t offset; 
} Arena;
typedef size_t Snapshot;

Arena* arena_create(size_t size);
void arena_destroy(Arena *arena);

void* arena_alloc(Arena *arena, size_t size);
Snapshot arena_snapshot(Arena *arena);
void arena_rollback(Arena *arena, Snapshot snapshot);

void arena_reset(Arena* arena);
#endif 