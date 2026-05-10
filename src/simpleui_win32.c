#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simpleui.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define MAX_FONTS 8

typedef struct {
    stbtt_fontinfo info;
    float scale;
} simpleui_font_data;

typedef struct {
    HWND hwnd;
    HDC hdc;
    bool quit;
    int mouse_x, mouse_y;
    bool mouse_down;
    bool mouse_clicked;
    bool prev_mouse_down;

    unsigned char font_data[MAX_FONTS][512 * 1024];
    simpleui_font_data fonts[MAX_FONTS];
    int font_count;
    BITMAPINFO bmi;
    HBITMAP hbmAtlas;
    void* atlas_bits;
} simpleui_win32;

static int win32_load_font(simpleui_platform* p, const char* path, int height)
{
    simpleui_win32* win = p->impl;
    if (win->font_count >= MAX_FONTS) return -1;

    int id = win->font_count++;
    FILE* f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size > sizeof(win->font_data[id])) {
        fclose(f);
        win->font_count--;
        return -1;
    }

    fread(win->font_data[id], 1, size, f);
    fclose(f);

    if (!stbtt_InitFont(&win->fonts[id].info, win->font_data[id], 0)) {
        win->font_count--;
        return -1;
    }

    win->fonts[id].scale = stbtt_ScaleForPixelHeight(&win->fonts[id].info, (float)height);

    return id;
}

static void win32_free_font(simpleui_platform* p, int font_id)
{
    (void)p;
    (void)font_id;
}

static bool win32_init(simpleui_platform* p, int width, int height, const char* title)
{
    simpleui_win32* win = malloc(sizeof(simpleui_win32));
    if (!win) return false;

    memset(win, 0, sizeof(simpleui_win32));

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "SimpleUI";
    RegisterClassA(&wc);

    win->hwnd = CreateWindowA("SimpleUI", title, WS_OVERLAPPEDWINDOW,
                               100, 100, width, height,
                               NULL, NULL, GetModuleHandleA(NULL), NULL);

    if (!win->hwnd) {
        free(win);
        return false;
    }

    win->hdc = GetDC(win->hwnd);
    ShowWindow(win->hwnd, SW_SHOW);
    UpdateWindow(win->hwnd);

    memset(&win->bmi, 0, sizeof(BITMAPINFO));
    win->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    win->bmi.bmiHeader.biWidth = 512;
    win->bmi.bmiHeader.biHeight = -64;
    win->bmi.bmiHeader.biPlanes = 1;
    win->bmi.bmiHeader.biBitCount = 8;
    win->bmi.bmiHeader.biCompression = BI_RGB;

    win->hbmAtlas = CreateDIBSection(win->hdc, &win->bmi, DIB_RGB_COLORS, &win->atlas_bits, NULL, 0);

    p->impl = win;
    return true;
}

static void win32_shutdown(simpleui_platform* p)
{
    if (!p->impl) return;
    simpleui_win32* win = p->impl;

    if (win->hbmAtlas) DeleteObject(win->hbmAtlas);
    ReleaseDC(win->hwnd, win->hdc);
    DestroyWindow(win->hwnd);
    free(win);
}

static void win32_begin_frame(simpleui_platform* p)
{
    simpleui_win32* win = p->impl;
    win->mouse_clicked = false;

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        if (msg.message == WM_QUIT) {
            win->quit = true;
        }
    }
}

static void win32_end_frame(simpleui_platform* p)
{
    simpleui_win32* win = p->impl;
    SwapBuffers(win->hdc);
}

static void win32_clear(simpleui_platform* p, simpleui_color color)
{
    simpleui_win32* win = p->impl;
    RECT r = {0, 0, 32767, 32767};
    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    FillRect(win->hdc, &r, brush);
    DeleteObject(brush);
}

static void win32_draw_rect(simpleui_platform* p, simpleui_rect rect, simpleui_color color)
{
    simpleui_win32* win = p->impl;
    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    RECT r = {rect.x, rect.y, rect.x + rect.width, rect.y + rect.height};
    FillRect(win->hdc, &r, brush);
    DeleteObject(brush);
}

static void win32_draw_rect_outline(simpleui_platform* p, simpleui_rect rect, simpleui_color color, int line_width)
{
    simpleui_win32* win = p->impl;
    HPEN pen = CreatePen(PS_SOLID, line_width, RGB(color.r, color.g, color.b));
    HPEN old = SelectObject(win->hdc, pen);
    Rectangle(win->hdc, rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
    SelectObject(win->hdc, old);
    DeleteObject(pen);
}

static void win32_draw_line(simpleui_platform* p, int x1, int y1, int x2, int y2, simpleui_color color, int width)
{
    simpleui_win32* win = p->impl;
    HPEN pen = CreatePen(PS_SOLID, width, RGB(color.r, color.g, color.b));
    HPEN old = SelectObject(win->hdc, pen);
    MoveToEx(win->hdc, x1, y1, NULL);
    LineTo(win->hdc, x2, y2);
    SelectObject(win->hdc, old);
    DeleteObject(pen);
}

static void win32_draw_text(simpleui_platform* p, int x, int y, const char* text, simpleui_color color, int font_id)
{
    simpleui_win32* win = p->impl;

    COLORREF fg = RGB(color.r, color.g, color.b);

    for (int i = 0; text[i]; i++) {
        int ch = (unsigned char)text[i];

        stbtt_fontinfo* info = &win->fonts[font_id].info;
        float scale = win->fonts[font_id].scale;
        int glyph = stbtt_FindGlyphIndex(info, ch);

        int w = 0, h = 0, xoff = 0, yoff = 0;
        unsigned char* buf = stbtt_GetGlyphBitmap(info, scale, scale, glyph, &w, &h, &xoff, &yoff);

        if (buf && w > 0 && h > 0) {
            BITMAPINFO bmi = {0};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = w;
            bmi.bmiHeader.biHeight = -h;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            unsigned char* argb = malloc(w * h * 4);
            for (int py = 0; py < h; py++) {
                for (int px = 0; px < w; px++) {
                    unsigned char alpha = buf[py * w + px];
                    unsigned char* pixel = &argb[py * w * 4 + px * 4];
                    pixel[0] = color.b;
                    pixel[1] = color.g;
                    pixel[2] = color.r;
                    pixel[3] = alpha;
                }
            }

            int draw_x = x + xoff;
            int draw_y = y + yoff;
            StretchDIBits(win->hdc, draw_x, draw_y, w, h, 0, 0, w, h, argb, &bmi, DIB_RGB_COLORS, SRCCOPY);

            free(argb);
        }

        int adv = 0;
        stbtt_GetGlyphHMetrics(info, glyph, &adv, 0);
        x += (int)(adv * scale);

        if (buf) free(buf);
    }
}

static bool win32_get_mouse(simpleui_platform* p, int* x, int* y, bool* down, bool* clicked)
{
    simpleui_win32* win = p->impl;

    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(win->hwnd, &pt);

    if (x) *x = pt.x;
    if (y) *y = pt.y;
    if (down) *down = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
    if (clicked) *clicked = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) && !win->prev_mouse_down;

    win->prev_mouse_down = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
    return true;
}

static bool win32_get_quit(simpleui_platform* p)
{
    simpleui_win32* win = p->impl;
    return win->quit;
}

simpleui_platform* simpleui_platform_win32_init(void)
{
    simpleui_platform* p = calloc(1, sizeof(simpleui_platform));
    if (!p) return NULL;

    p->impl = NULL;
    p->init = win32_init;
    p->shutdown = win32_shutdown;
    p->begin_frame = win32_begin_frame;
    p->end_frame = win32_end_frame;
    p->clear = win32_clear;
    p->load_font = win32_load_font;
    p->free_font = win32_free_font;
    p->draw_rect = win32_draw_rect;
    p->draw_rect_outline = win32_draw_rect_outline;
    p->draw_line = win32_draw_line;
    p->draw_text = win32_draw_text;
    p->get_mouse = win32_get_mouse;
    p->get_quit = win32_get_quit;

    return p;
}

#endif