#ifndef _SIMPLEUI_H
#define _SIMPLEUI_H

#include <stddef.h>
#include <stdbool.h>

#define SIMPLEUI_MAX_ELEMENTS 128

typedef enum {
    SIMPLEUI_GRAPH_TYPE_LINE = 0,
    SIMPLEUI_GRAPH_TYPE_AREA,
    SIMPLEUI_GRAPH_TYPE_CHART,
    SIMPLEUI_GRAPH_TYPE_COLS,
    SIMPLEUI_GRAPH_TYPE_ROWS,
} simpleui_graph_type;

typedef enum {
    SIMPLEUI_TYPE_NONE = 0,
    SIMPLEUI_TYPE_BUTTON,
    SIMPLEUI_TYPE_LABEL,
    SIMPLEUI_TYPE_CHECKBOX,
    SIMPLEUI_TYPE_SLIDER,
    SIMPLEUI_TYPE_PANEL,
	SIMPLEUI_TYPE_DROPDOWN_MENU,
	SIMPLEUI_TYPE_GRAPH
} simpleui_type;

typedef enum {
    SIMPLEUI_EVENT_NONE = 0,
    SIMPLEUI_EVENT_CLICKED,
    SIMPLEUI_EVENT_TOGGLED,
    SIMPLEUI_EVENT_CHANGED_HOVER,
    SIMPLEUI_EVENT_VALUE_CHANGED
} simpleui_event;

typedef struct simpleui_element simpleui_element;

typedef struct {
    int x, y;
    int width, height;
} simpleui_rect;

typedef struct {
    unsigned char r, g, b, a;
} simpleui_color;

#define SIMPLEUI_COLOR(r, g, b, a) ((simpleui_color){r, g, b, a})
#define SIMPLEUI_BLACK SIMPLEUI_COLOR(0, 0, 0, 255)
#define SIMPLEUI_WHITE SIMPLEUI_COLOR(255, 255, 255, 255)
#define SIMPLEUI_GRAY SIMPLEUI_COLOR(128, 128, 128, 255)

typedef struct {
    const char* text;
    simpleui_color text_color;
    simpleui_color bg_color;
    simpleui_color border_color;
    bool render_border;
    int font_height;
    int font_id;
} simpleui_style;

typedef struct {
    bool checked;
} simpleui_data_checkbox;

typedef struct {
    float value;
    float min;
    float max;
} simpleui_data_slider;

typedef struct {
    char** buffers;
    int selected;
    int size;
} simpleui_data_dropdown;

typedef struct {
    simpleui_graph_type type;

    unsigned x,y,w,h;

    unsigned max_x, max_y;
    unsigned min_x, min_y;

    float* data_x; size_t data_x_size;
    float* data_y; size_t data_y_size;

} simpleui_data_graph;

struct simpleui_element {
    simpleui_type type;
    simpleui_rect bounds;
    simpleui_style style;
    bool visible;
    bool enabled;
    bool hovered;
    bool pressed;
    const char* id;
    void* user_data;

    union {
        const char* label_text;
        simpleui_data_checkbox checkbox;
        simpleui_data_slider slider;
		simpleui_data_dropdown dropdown;
		simpleui_data_graph graph;
    } data;
};

typedef struct simpleui_platform simpleui_platform;

struct simpleui_platform {
    void* impl;

    bool (*init)(simpleui_platform* p, int width, int height, const char* title);
    void (*shutdown)(simpleui_platform* p);

    void (*begin_frame)(simpleui_platform* p);
    void (*end_frame)(simpleui_platform* p);

    void (*clear)(simpleui_platform* p, simpleui_color color);

    int (*load_font)(simpleui_platform* p, const char* path, int height);
    void (*free_font)(simpleui_platform* p, int font_id);

    void (*draw_rect)(simpleui_platform* p, simpleui_rect rect, simpleui_color color);
    void (*draw_rect_outline)(simpleui_platform* p, simpleui_rect rect, simpleui_color color, int line_width);
    void (*draw_line)(simpleui_platform* p, int x1, int y1, int x2, int y2, simpleui_color color, int width);
    void (*draw_text)(simpleui_platform* p, int x, int y, const char* text, simpleui_color color, int font_id);

    bool (*get_mouse)(simpleui_platform* p, int* x, int* y, bool* down, bool* clicked);
    bool (*get_quit)(simpleui_platform* p);
    void (*get_window_size)(simpleui_platform* p, int* w, int* h);
    void (*capture_screenshot)(simpleui_platform* p, const char* path);
};

typedef struct {
    simpleui_element elements[SIMPLEUI_MAX_ELEMENTS];
    size_t element_count;

    simpleui_platform* platform;
    simpleui_color graph_bg_color;
    simpleui_color bg_color;
    simpleui_color primary_color;
    simpleui_color text_color;
    simpleui_color border_color;
    simpleui_color hover_color;
    int default_font;
    bool rendered_once;

    int mouse_x;
    int mouse_y;
    bool mouse_down;
    bool mouse_clicked;

    simpleui_element* hovered_element;
} simpleui_context;

void simpleui_init(simpleui_context* ctx, simpleui_platform* platform);
void simpleui_begin(simpleui_context* ctx);
void simpleui_end(simpleui_context* ctx);

simpleui_element* simpleui_add_button(simpleui_context* ctx, const char* id, int x, int y, int w, int h, const char* text);
simpleui_element* simpleui_add_label(simpleui_context* ctx, const char* id, int x, int y, const char* text);
simpleui_element* simpleui_add_checkbox(simpleui_context* ctx, const char* id, int x, int y, bool initial);
simpleui_element* simpleui_add_slider(simpleui_context* ctx, const char* id, int x, int y, int w, float min, float max, float initial);
simpleui_element* simpleui_add_dropdown(simpleui_context* ctx, const char* id, int x, int y, int w, int h, char** options, int count, int initial);
simpleui_element* simpleui_add_graph(simpleui_context* ctx, const char* id, simpleui_graph_type type, unsigned x, unsigned y, unsigned w, unsigned h, 
              float min_x, float max_x, float min_y, float max_y,
              float* data_x, size_t data_x_size, float* data_y, size_t data_y_size);

simpleui_element* simpleui_get(simpleui_context* ctx, const char* id);

simpleui_event simpleui_process(simpleui_context* ctx);
void simpleui_draw(simpleui_context* ctx);

simpleui_platform* simpleui_platform_x11_init(void);
simpleui_platform* simpleui_platform_win32_init(void);

#endif
