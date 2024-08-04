#include <windows.h>
#include "arena_alloc.hh"

Arena CreateNewArena(Arena *parent, ptrdiff_t max_arena_capacity)
{
    Arena arena = {};
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    ptrdiff_t page_size = sys_info.dwPageSize;
    if(max_arena_capacity > page_size)
    {
        ptrdiff_t num_pages = (max_arena_capacity + 1) / page_size;
        max_arena_capacity = num_pages * page_size;
    }

    if(parent)
    {
        arena.memory = (char *)ArenaAlloc(parent, max_arena_capacity, 0);
    }

    else
    {
        DWORD protect = PAGE_READWRITE;
        DWORD alloc_type = MEM_RESERVE | MEM_COMMIT;
        arena.memory = (char *)VirtualAlloc(0, max_arena_capacity, alloc_type, protect);
    }
    
    arena.capacity = max_arena_capacity;
    return arena;
}

void DestroyArena(Arena *arena)
{
    VirtualFree(arena->memory, arena->capacity, MEM_FREE);
    arena->memory = 0;
}

void *ArenaAlloc(Arena *arena, ptrdiff_t allocation_size, ptrdiff_t allign)
{
    if(allign < 8) allign = 8;
    ptrdiff_t alligned = (allocation_size + allign - 1) & -allign;

    if(arena->capacity < arena->used + alligned)
    {
        return 0;
    }

    char *block = arena->memory + arena->used;
    arena->used += alligned;
    return block;
}

void ArenaClear(Arena *arena)
{
    arena->used = 0;
}

TempArena BeginTempArena(Arena *arena)
{
    TempArena temp = {};
    temp.arena = arena;
    temp.position = arena->used;
    return temp;
}

void EndTempArena(TempArena temp)
{
    temp.arena->used = temp.position;
}
