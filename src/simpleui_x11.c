#include <X11/Xlib.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "simpleui.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define MAX_FONTS 8
#define ATLAS_WIDTH 512
#define ATLAS_HEIGHT 64

typedef struct {
    stbtt_fontinfo info;
    float scale;
    int ascent;
} simpleui_font_data;

typedef struct {
    Display* display;
    Window window;
    Pixmap back_buffer;
    GC gc;
    int screen;
    int width;
    int height;
    int epoll_fd;
    bool quit;
    int mouse_x, mouse_y;
    bool mouse_down;
    bool mouse_clicked;
    bool prev_mouse_down;

    unsigned char font_data[MAX_FONTS][512 * 1024];
    simpleui_font_data fonts[MAX_FONTS];
    int font_count;
    Pixmap font_atlas;
    GC font_gc;
    int atlas_generation;
} simpleui_x11;

static int x11_load_font(simpleui_platform* p, const char* path, int height)
{
    simpleui_x11* x11 = p->impl;
    if (x11->font_count >= MAX_FONTS) return -1;

    int id = x11->font_count++;
    FILE* f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size > sizeof(x11->font_data[id])) {
        fclose(f);
        x11->font_count--;
        return -1;
    }

    fread(x11->font_data[id], 1, size, f);
    fclose(f);

    if (!stbtt_InitFont(&x11->fonts[id].info, x11->font_data[id], 0)) {
        x11->font_count--;
        return -1;
    }

    x11->fonts[id].scale = stbtt_ScaleForPixelHeight(&x11->fonts[id].info, (float)height);
    stbtt_GetFontVMetrics(&x11->fonts[id].info, &x11->fonts[id].ascent, 0, 0);

    return id;
}

static void x11_free_font(simpleui_platform* p, int font_id)
{
    (void)p;
    (void)font_id;
}

static void x11_render_glyph(simpleui_x11* x11, int font_id, int ch, int* x, int* y, int height, unsigned char* bitmap, int bw, int bh)
{
    if (*x + bw > ATLAS_WIDTH) {
        *x = 0;
        *y += height + 2;
    }
    if (*y + bh > ATLAS_HEIGHT) return;

    stbtt_fontinfo* info = &x11->fonts[font_id].info;
    float scale = x11->fonts[font_id].scale;
    int glyph = stbtt_FindGlyphIndex(info, ch);
    int w = 0, h = 0, xoff = 0, yoff = 0;

    unsigned char* buf = stbtt_GetGlyphBitmap(info, scale, scale, glyph, &w, &h, &xoff, &yoff);
    if (!buf || w == 0 || h == 0) {
        if (buf) free(buf);
        return;
    }

    for (int row = 0; row < h && (*y + row) < ATLAS_HEIGHT; row++) {
        for (int col = 0; col < w && (*x + col) < ATLAS_WIDTH; col++) {
            int atlas_row = *y + row;
            int atlas_col = *x + col;
            int idx = atlas_row * ATLAS_WIDTH + atlas_col;
            bitmap[idx] = buf[row * w + col];
        }
    }

    int advance = 0;
    stbtt_GetGlyphHMetrics(info, glyph, &advance, 0);
    *x += w + 2;

    free(buf);
}

static void x11_build_atlas(simpleui_x11* x11)
{
    unsigned char atlas[ATLAS_WIDTH * ATLAS_HEIGHT] = {0};
    int x = 0, y = 0;

    for (int fi = 0; fi < x11->font_count; fi++) {
        int height = 16;
        for (int ch = 32; ch < 128; ch++) {
            stbtt_fontinfo* info = &x11->fonts[fi].info;
            float scale = x11->fonts[fi].scale;
            int glyph = stbtt_FindGlyphIndex(info, ch);
            int w = 0, h = 0;

            int adv = 0;
            stbtt_GetGlyphHMetrics(info, glyph, &adv, 0);
            int gw = (int)(adv * scale) + 2;

            unsigned char* buf = stbtt_GetGlyphBitmap(info, scale, scale, glyph, &w, &h, 0, 0);
            if (buf && w > 0 && h > 0) {
                int start_x = x;
                if (x + w > ATLAS_WIDTH) {
                    x = 0;
                    y += height + 2;
                }
                if (y + h > ATLAS_HEIGHT) break;

                for (int row = 0; row < h && (y + row) < ATLAS_HEIGHT; row++) {
                    for (int col = 0; col < w && (x + col) < ATLAS_WIDTH; col++) {
                        atlas[(y + row) * ATLAS_WIDTH + (x + col)] = buf[row * w + col];
                    }
                }
                x += w + 2;
                free(buf);
            } else {
                x += gw;
                if (x >= ATLAS_WIDTH) {
                    x = 0;
                    y += height + 2;
                }
            }
        }
    }

    if (x11->font_atlas) XFreePixmap(x11->display, x11->font_atlas);
    x11->font_atlas = XCreatePixmap(x11->display, x11->window, ATLAS_WIDTH, ATLAS_HEIGHT, 8);

    XGCValues gcv;
    gcv.foreground = BlackPixel(x11->display, x11->screen);
    gcv.background = WhitePixel(x11->display, x11->screen);
    GC mono_gc = XCreateGC(x11->display, x11->font_atlas, GCForeground | GCBackground, &gcv);
    XSetForeground(x11->display, mono_gc, 0);
    XFillRectangle(x11->display, x11->font_atlas, mono_gc, 0, 0, ATLAS_WIDTH, ATLAS_HEIGHT);

    for (int row = 0; row < ATLAS_HEIGHT; row++) {
        for (int col = 0; col < ATLAS_WIDTH; col++) {
            unsigned char c = atlas[row * ATLAS_WIDTH + col];
            if (c > 128) c = 255;
            else if (c > 0) c = 128;
            XSetForeground(x11->display, mono_gc, c);
            XDrawPoint(x11->display, x11->font_atlas, mono_gc, col, row);
        }
    }
    XFreeGC(x11->display, mono_gc);
    x11->atlas_generation++;
}

static void x11_draw_text(simpleui_platform* p, int x, int y, const char* text, simpleui_color color, int font_id)
{
    simpleui_x11* x11 = p->impl;
    (void)x; (void)y; (void)text; (void)color; (void)font_id;

    unsigned long c = (color.r << 16) | (color.g << 8) | color.b;

    for (int i = 0; text[i]; i++) {
        int ch = (unsigned char)text[i];
        if (ch < 32 || ch >= 128) continue;

        stbtt_fontinfo* info = &x11->fonts[font_id].info;
        float scale = x11->fonts[font_id].scale;
        int glyph = stbtt_FindGlyphIndex(info, ch);

        int w = 0, h = 0, xoff = 0, yoff = 0;
        unsigned char* buf = stbtt_GetGlyphBitmap(info, scale, scale, glyph, &w, &h, &xoff, &yoff);

        if (buf && w > 0 && h > 0) {
            Pixmap pix = XCreatePixmap(x11->display, x11->window, w, h, 1);

            GC mono_gc = XCreateGC(x11->display, pix, 0, NULL);
            XSetForeground(x11->display, mono_gc, 0);
            XFillRectangle(x11->display, pix, mono_gc, 0, 0, w, h);
            XSetForeground(x11->display, mono_gc, 1);

            for (int row = 0; row < h; row++) {
                for (int col = 0; col < w; col++) {
                    unsigned char alpha = buf[row * w + col];
                    if (alpha > 128) {
                        XDrawPoint(x11->display, pix, mono_gc, col, row);
                    }
                }
            }

            XGCValues gcv;
            gcv.foreground = c;
            gcv.background = 0;
            GC text_gc = XCreateGC(x11->display, x11->back_buffer, GCForeground | GCBackground, &gcv);

            XCopyPlane(x11->display, pix, x11->back_buffer, text_gc, 0, 0, w, h, x + xoff, y + yoff, 1);

            XFreeGC(x11->display, mono_gc);
            XFreeGC(x11->display, text_gc);
            XFreePixmap(x11->display, pix);
        }

        int adv = 0;
        stbtt_GetGlyphHMetrics(info, glyph, &adv, 0);
        x += (int)(adv * scale);

        if (buf) free(buf);
    }
}

static bool x11_init(simpleui_platform* p, int width, int height, const char* title)
{
    simpleui_x11* x11 = malloc(sizeof(simpleui_x11));
    if (!x11) { fprintf(stderr, "alloc failed\n"); return false; }

    memset(x11, 0, sizeof(simpleui_x11));

    x11->display = XOpenDisplay(NULL);
    if (!x11->display) {
        fprintf(stderr, "XOpenDisplay failed\n");
        free(x11);
        return false;
    }
    fprintf(stderr, "display opened\n");

    x11->screen = DefaultScreen(x11->display);
    Window root = RootWindow(x11->display, x11->screen);

    x11->window = XCreateSimpleWindow(
        x11->display, root,
        100, 100, width, height,
        1, BlackPixel(x11->display, x11->screen),
        WhitePixel(x11->display, x11->screen));

    XSelectInput(x11->display, x11->window,
        ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask);

    XStoreName(x11->display, x11->window, title);
    XMapWindow(x11->display, x11->window);

    x11->gc = XCreateGC(x11->display, x11->window, 0, NULL);

    x11->width = width;
    x11->height = height;
    x11->back_buffer = XCreatePixmap(x11->display, x11->window,
                                     width, height,
                                     DefaultDepth(x11->display, x11->screen));

    x11->epoll_fd = epoll_create1(0);
    if (x11->epoll_fd < 0) {
        XCloseDisplay(x11->display);
        free(x11);
        return false;
    }

    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.fd = ConnectionNumber(x11->display)
    };
    epoll_ctl(x11->epoll_fd, EPOLL_CTL_ADD, ConnectionNumber(x11->display), &ev);

    XFlush(x11->display);

    p->impl = x11;
    return true;
}

static void x11_shutdown(simpleui_platform* p)
{
    if (!p->impl) return;
    simpleui_x11* x11 = p->impl;

    close(x11->epoll_fd);
    if (x11->font_atlas) XFreePixmap(x11->display, x11->font_atlas);
    XFreePixmap(x11->display, x11->back_buffer);
    XFreeGC(x11->display, x11->gc);
    XDestroyWindow(x11->display, x11->window);
    XCloseDisplay(x11->display);

    free(x11);
}

static void x11_begin_frame(simpleui_platform* p)
{
    simpleui_x11* x11 = p->impl;
    x11->mouse_clicked = false;

    struct epoll_event ev;
    int nready = epoll_wait(x11->epoll_fd, &ev, 1, 0);
    if (nready <= 0) return;

    while (XPending(x11->display)) {
        XEvent e;
        XNextEvent(x11->display, &e);

        switch (e.type) {
            case Expose:
                break;
            case MotionNotify:
                x11->mouse_x = e.xmotion.x;
                x11->mouse_y = e.xmotion.y;
                break;
            case ButtonPress:
                x11->mouse_x = e.xbutton.x;
                x11->mouse_y = e.xbutton.y;
                if (!x11->prev_mouse_down) {
                    x11->mouse_clicked = true;
                }
                x11->mouse_down = true;
                x11->prev_mouse_down = true;
                break;
            case ButtonRelease:
                x11->mouse_down = false;
                x11->prev_mouse_down = false;
                break;
            case KeyPress:
                if (e.xkey.keycode == 9) {
                    x11->quit = true;
                }
                break;
        }
    }
}

static void x11_end_frame(simpleui_platform* p)
{
    simpleui_x11* x11 = p->impl;
    XCopyArea(x11->display, x11->back_buffer, x11->window, x11->gc,
              0, 0, x11->width, x11->height, 0, 0);
    XFlush(x11->display);
}

static void x11_clear(simpleui_platform* p, simpleui_color color)
{
    simpleui_x11* x11 = p->impl;
    unsigned long c = (color.r << 16) | (color.g << 8) | color.b;
    XSetForeground(x11->display, x11->gc, c);
    XFillRectangle(x11->display, x11->back_buffer, x11->gc, 0, 0, 32767, 32767);
}

static void x11_draw_rect(simpleui_platform* p, simpleui_rect rect, simpleui_color color)
{
    simpleui_x11* x11 = p->impl;
    unsigned long c = (color.r << 16) | (color.g << 8) | color.b;
    XSetForeground(x11->display, x11->gc, c);
    XFillRectangle(x11->display, x11->back_buffer, x11->gc,
                   rect.x, rect.y, rect.width, rect.height);
}

static void x11_draw_rect_outline(simpleui_platform* p, simpleui_rect rect, simpleui_color color, int line_width)
{
    simpleui_x11* x11 = p->impl;
    unsigned long c = (color.r << 16) | (color.g << 8) | color.b;
    XSetForeground(x11->display, x11->gc, c);
    XSetLineAttributes(x11->display, x11->gc, line_width, LineSolid, CapButt, JoinMiter);
    XDrawRectangle(x11->display, x11->back_buffer, x11->gc,
                   rect.x, rect.y, rect.width, rect.height);
}

static void x11_draw_line(simpleui_platform* p, int x1, int y1, int x2, int y2, simpleui_color color, int width)
{
    simpleui_x11* x11 = p->impl;
    unsigned long c = (color.r << 16) | (color.g << 8) | color.b;
    XSetForeground(x11->display, x11->gc, c);
    XSetLineAttributes(x11->display, x11->gc, width, LineSolid, CapButt, JoinMiter);
    XDrawLine(x11->display, x11->back_buffer, x11->gc, x1, y1, x2, y2);
}

static bool x11_get_mouse(simpleui_platform* p, int* x, int* y, bool* down, bool* clicked)
{
    simpleui_x11* x11 = p->impl;
    if (x) *x = x11->mouse_x;
    if (y) *y = x11->mouse_y;
    if (down) *down = x11->mouse_down;
    if (clicked) *clicked = x11->mouse_clicked;
    return true;
}

static bool x11_get_quit(simpleui_platform* p)
{
    simpleui_x11* x11 = p->impl;
    return x11->quit;
}

simpleui_platform* simpleui_platform_x11_init(void)
{
    simpleui_platform* p = calloc(1, sizeof(simpleui_platform));
    if (!p) return NULL;

    p->impl = NULL;
    p->init = x11_init;
    p->shutdown = x11_shutdown;
    p->begin_frame = x11_begin_frame;
    p->end_frame = x11_end_frame;
    p->clear = x11_clear;
    p->load_font = x11_load_font;
    p->free_font = x11_free_font;
    p->draw_rect = x11_draw_rect;
    p->draw_rect_outline = x11_draw_rect_outline;
    p->draw_line = x11_draw_line;
    p->draw_text = x11_draw_text;
    p->get_mouse = x11_get_mouse;
    p->get_quit = x11_get_quit;

    return p;
}