#ifndef ARENA_ALLOC_H
#define ARENA_ALLOC_H

#define KB 1024
#define MB 1024 * 1024
#define GB 1024 * 1024 * 1024

#include <stddef.h>

struct Arena
{
    char *memory;
    ptrdiff_t used;
    ptrdiff_t capacity;
};

struct TempArena
{
    Arena *arena;
    ptrdiff_t position;
};

Arena CreateNewArena(Arena *parent, ptrdiff_t max_arena_capacity);
void DestroyArena(Arena arena);

void *ArenaAlloc(Arena *arena, ptrdiff_t allocation_size, ptrdiff_t allign);
#define ArenaAllocStruct(arena, type) (type *)ArenaAlloc((arena), sizeof (type), 0)
void ArenaClear(Arena *arena);

TempArena BeginTempArena(Arena *arena);
void EndTempArena(TempArena temp);

#endif //ARENA_ALLOC_H
