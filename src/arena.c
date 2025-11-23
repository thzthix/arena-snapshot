#include "arena.h"
#include <stdlib.h>
#include <stdint.h>
typedef uint64_t guard_t;
#define ARENA_GUARD_SIZE sizeof(guard_t)
#define ARENA_GUARD_PATTERN 0xDEADBEEFDEADBEEFULL
typedef struct {
    size_t size;
}GuardHeader;

#define START_OFFSET 0
Arena* arena_create(size_t capacity){
    if (capacity == 0) {
        return NULL;
    }
    Arena *arena = malloc(sizeof(Arena));
    if(!arena){
        return NULL;
    }
    arena-> buffer = malloc(capacity);
    if (!arena->buffer){
        free(arena);
        return NULL;
    }
    arena-> capacity = capacity;
    arena-> offset = START_OFFSET;
    return arena;
}

void arena_destroy(Arena *arena){
    if (!arena){
        return;
    }
    free(arena-> buffer);
    free(arena);
}

void* arena_alloc(Arena *arena, size_t size, size_t align){
    if(!arena || size == 0 || align == 0 ){
        return NULL;
    }
    size_t align_mask = align - 1;
    if((align & align_mask)!=0){
        return NULL;
    }
    size_t aligned_offset = (arena->offset + align_mask ) & ~align_mask; 
    if(aligned_offset < arena->offset){ 
        return NULL;
    }
    size_t new_offset = aligned_offset + size;
    if(new_offset < aligned_offset || new_offset>arena->capacity){ 
        return NULL;
    }
    arena-> offset = new_offset;
    void* ptr = arena-> buffer +  aligned_offset;
    return ptr;
}
void* arena_alloc_guarded(Arena *arena, size_t size, size_t align){
    size_t total_size = sizeof(GuardHeader)+ size + ARENA_GUARD_SIZE;
    void* raw_ptr = arena_alloc(arena, total_size, align);
    if(!raw_ptr){
        return NULL;
    }
    GuardHeader* header = (GuardHeader*)raw_ptr;
    header->size = size;

    guard_t* front_gaurd = (guard_t*)((char*)raw_ptr+sizeof(GuardHeader));
    *front_gaurd = ARENA_GUARD_PATTERN;

    void* data_ptr = (char*)front_gaurd + ARENA_GUARD_SIZE;
    guard_t* back_gaurd = (guard_t*)((char*)data_ptr+size);
    *back_gaurd = ARENA_GUARD_PATTERN;
    return data_ptr;

}
int arena_check_guard(void* data_ptr){
    if(!data_ptr){
        return 0;
    }
    GuardHeader* header = (GuardHeader*)((char*)data_ptr -(sizeof(GuardHeader) + ARENA_GUARD_SIZE));
    guard_t* front_gaurd = (guard_t*)((char*)header+ sizeof(GuardHeader));
    guard_t* back_gaurd = (guard_t*)((char*)data_ptr + header->size);
    
    if(*front_gaurd!=ARENA_GUARD_PATTERN||*back_gaurd!=ARENA_GUARD_PATTERN){
        return 0;
    }
    return 1;

}
Snapshot arena_snapshot(Arena *arena){
    if(!arena){
        return 0;
    }
    return arena->offset;

}
void arena_rollback(Arena *arena, Snapshot snapshot){
    if(!arena || snapshot > arena->offset){ // 미래 시점으로 롤백 시도
        return;
    }
    arena-> offset = snapshot;
}

void arena_reset(Arena* arena){
    if(!arena){
        return;
    }
    arena->offset = START_OFFSET;

}