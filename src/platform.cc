#include "platform.hh"

POINT WindowCenter(HWND window, RECT window_rect)
{
    int width = window_rect.right - window_rect.left;
    int height = window_rect.bottom - window_rect.top;
    int center_x = width / 2;
    int center_y = height / 2;

    POINT point = {};
    point.x = center_x;
    point.y = center_y;
    ClientToScreen(window, &point);
    return point;
}

LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    GetWindowRect(window, &rect);
    POINT center = WindowCenter(window, rect);

    Platform *platform = 0;
    if(msg == WM_CREATE)
    {
        CREATESTRUCT *create = (CREATESTRUCT *)lParam;
        platform = (Platform *)create->lpCreateParams;
        platform->window = window;
        SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)platform);
    }

    else
    {
        platform = (Platform *)GetWindowLongPtr(window, GWLP_USERDATA);
    }

    switch(msg)
    {
        case WM_DESTROY:
        {
            ExitProcess(0);
        }

        case WM_MOUSEMOVE:
        {
            GetCursorPos(&platform->cursor_pos);
            ClientToScreen(window, &platform->cursor_pos);

            POINT point = platform->cursor_pos;
            platform->cursor_delta.x = point.x - center.x;
            platform->cursor_delta.y = point.y - center.y;
            ScreenToClient(window, &platform->cursor_delta);
            SetCursorPos(center.x, center.y);
        }

        default:
        {
            return DefWindowProcA(window, msg, wParam, lParam);
        }
    }
}

Platform *CreatePlatform(Arena *arena, int width, int height, const char *window_name)
{
    Platform *platform = ArenaAllocStruct(arena, Platform);

    WNDCLASS wndclass = {};
    wndclass.lpszClassName = "WINDOW CLASS";
    wndclass.lpfnWndProc = WindowProc;
    wndclass.hCursor = LoadCursor(0, IDC_ARROW);
    RegisterClass(&wndclass);

    CreateWindowEx(0, wndclass.lpszClassName, window_name,
                   WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                   width, height, 0, 0, 0, platform);

    ShowWindow(platform->window, SW_SHOW);

    return platform;
}

void PlatformPollEvents(Platform *platform)
{
    MSG msg;
    while(PeekMessage(&msg, platform->window, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
