#ifndef PLATFORM_H
#define PLATFORM_H

#include <windows.h>
#include "arena_alloc.hh"

struct Platform
{
    HWND window;
    POINT cursor_delta;
    POINT cursor_pos;
};

Platform *CreatePlatform(Arena *arena, int width, int height, const char *window_name);
void PlatformPollEvents(Platform *platform);

#endif //PLATFORM_H
