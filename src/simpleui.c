#include <string.h>

#include "simpleui.h"

#define DEFAULT_BG SIMPLEUI_COLOR(40, 40, 40, 255)
#define DEFAULT_PRIMARY SIMPLEUI_COLOR(66, 135, 245, 255)
#define DEFAULT_TEXT SIMPLEUI_COLOR(255, 255, 255, 255)
#define DEFAULT_BORDER SIMPLEUI_COLOR(100, 100, 100, 255)
#define DEFAULT_HOVER SIMPLEUI_COLOR(90, 90, 90, 255)

void simpleui_init(simpleui_context* ctx, simpleui_platform* platform)
{
    memset(ctx, 0, sizeof(simpleui_context));
    ctx->platform = platform;
    ctx->bg_color = DEFAULT_BG;
    ctx->primary_color = DEFAULT_PRIMARY;
    ctx->text_color = DEFAULT_TEXT;
    ctx->border_color = DEFAULT_BORDER;
    ctx->hover_color = DEFAULT_HOVER;
    ctx->default_font = platform->load_font(platform, "/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf", 16);
}

void simpleui_begin(simpleui_context* ctx)
{
    ctx->hovered_element = NULL;
    for (size_t i = 0; i < ctx->element_count; i++) {
        ctx->elements[i].hovered = false;
    }
}

void simpleui_end(simpleui_context* ctx)
{
    (void)ctx;
}

static bool rect_contains(simpleui_rect r, int x, int y)
{
    return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}

simpleui_element* simpleui_add_button(simpleui_context* ctx, const char* id, int x, int y, int w, int h, const char* text)
{
    if (ctx->element_count >= SIMPLEUI_MAX_ELEMENTS) return NULL;

    simpleui_element* el = &ctx->elements[ctx->element_count++];
    el->type = SIMPLEUI_TYPE_BUTTON;
    el->bounds = (simpleui_rect){x, y, w, h};
    el->visible = true;
    el->enabled = true;
    el->hovered = false;
    el->pressed = false;
    el->id = id;
    el->user_data = NULL;
    el->style.text_color = ctx->text_color;
    el->style.bg_color = ctx->primary_color;
    el->style.border_color = ctx->border_color;
    el->style.font_height = 16;
    el->style.font_id = ctx->default_font;
    el->data.label_text = text;

    return el;
}

simpleui_element* simpleui_add_label(simpleui_context* ctx, const char* id, int x, int y, const char* text)
{
    simpleui_element* el = simpleui_add_button(ctx, id, x, y, 100, 20, NULL);
    if (el) {
        el->type = SIMPLEUI_TYPE_LABEL;
        el->style.bg_color = SIMPLEUI_BLACK;
        el->style.bg_color.a = 0;
        el->data.label_text = text;
    }
    return el;
}

simpleui_element* simpleui_add_checkbox(simpleui_context* ctx, const char* id, int x, int y, bool initial)
{
    if (ctx->element_count >= SIMPLEUI_MAX_ELEMENTS) return NULL;

    simpleui_element* el = &ctx->elements[ctx->element_count++];
    el->type = SIMPLEUI_TYPE_CHECKBOX;
    el->bounds = (simpleui_rect){x, y, 20, 20};
    el->visible = true;
    el->enabled = true;
    el->hovered = false;
    el->pressed = false;
    el->id = id;
    el->user_data = NULL;
    el->style.text_color = ctx->text_color;
    el->style.bg_color = ctx->bg_color;
    el->style.border_color = ctx->border_color;
    el->style.font_height = 16;
    el->data.checkbox.checked = initial;

    return el;
}

simpleui_element* simpleui_add_slider(simpleui_context* ctx, const char* id, int x, int y, int w, float min, float max, float initial)
{
    if (ctx->element_count >= SIMPLEUI_MAX_ELEMENTS) return NULL;

    simpleui_element* el = &ctx->elements[ctx->element_count++];
    el->type = SIMPLEUI_TYPE_SLIDER;
    el->bounds = (simpleui_rect){x, y, w, 24};
    el->visible = true;
    el->enabled = true;
    el->hovered = false;
    el->pressed = false;
    el->id = id;
    el->user_data = NULL;
    el->style.text_color = ctx->text_color;
    el->style.bg_color = ctx->bg_color;
    el->style.border_color = ctx->border_color;
    el->style.font_height = 16;
    el->data.slider.min = min;
    el->data.slider.max = max;
    el->data.slider.value = initial;

    return el;
}

simpleui_element* simpleui_add_dropdown(simpleui_context* ctx, const char* id, int x, int y, int w, int h, char** options, int count, int initial)
{
    if (ctx->element_count >= SIMPLEUI_MAX_ELEMENTS) return NULL;

    simpleui_element* el = &ctx->elements[ctx->element_count++];
    el->type = SIMPLEUI_TYPE_DROPDOWN_MENU;
    el->bounds = (simpleui_rect){x, y, w, h};
    el->visible = true;
    el->enabled = true;
    el->hovered = false;
    el->pressed = false;
    el->id = id;
    el->user_data = NULL;
    el->style.text_color = ctx->text_color;
    el->style.bg_color = ctx->bg_color;
    el->style.border_color = ctx->border_color;
    el->style.font_height = 16;
    el->data.dropdown.buffers = options;
    el->data.dropdown.size = count;
    el->data.dropdown.selected = initial;

    return el;
}

simpleui_element* simpleui_get(simpleui_context* ctx, const char* id)
{
    for (size_t i = 0; i < ctx->element_count; i++) {
        if (ctx->elements[i].id && strcmp(ctx->elements[i].id, id) == 0) {
            return &ctx->elements[i];
        }
    }
    return NULL;
}

simpleui_event simpleui_process(simpleui_context* ctx)
{
    simpleui_event result = SIMPLEUI_EVENT_NONE;

    for (size_t i = 0; i < ctx->element_count; i++) {
        simpleui_element* el = &ctx->elements[i];
        if (!el->visible || !el->enabled) continue;

        bool over = rect_contains(el->bounds, ctx->mouse_x, ctx->mouse_y);
        el->hovered = over;

        if (over) {
            ctx->hovered_element = el;
        }

        switch (el->type) {
            case SIMPLEUI_TYPE_BUTTON:
                if (over && ctx->mouse_clicked) {
                    el->pressed = true;
                    result = SIMPLEUI_EVENT_CLICKED;
                }
                if (!ctx->mouse_down) {
                    el->pressed = false;
                }
                break;

            case SIMPLEUI_TYPE_CHECKBOX:
                if (over && ctx->mouse_clicked) {
                    el->data.checkbox.checked = !el->data.checkbox.checked;
                    result = SIMPLEUI_EVENT_TOGGLED;
                }
                break;

            case SIMPLEUI_TYPE_SLIDER:
                if (over && ctx->mouse_down) {
                    float range = el->data.slider.max - el->data.slider.min;
                    float percent = (float)(ctx->mouse_x - el->bounds.x) / (float)el->bounds.width;
                    if (percent < 0) percent = 0;
                    if (percent > 1) percent = 1;
                    el->data.slider.value = el->data.slider.min + (percent * range);
                    result = SIMPLEUI_EVENT_VALUE_CHANGED;
                }
                break;

            case SIMPLEUI_TYPE_DROPDOWN_MENU:
                if (el->pressed) {
                    bool clicked_item = false;
                    for (int j = 0; j < el->data.dropdown.size; j++) {
                        simpleui_rect item_rect = {el->bounds.x, el->bounds.y + (j + 1) * el->bounds.height, el->bounds.width, el->bounds.height};
                        if (rect_contains(item_rect, ctx->mouse_x, ctx->mouse_y) && ctx->mouse_clicked) {
                            el->data.dropdown.selected = j;
                            el->pressed = false;
                            result = SIMPLEUI_EVENT_VALUE_CHANGED;
                            clicked_item = true;
                            break;
                        }
                    }
                    if (!clicked_item && !over && ctx->mouse_clicked) {
                        el->pressed = false;
                    }
                } else if (over && ctx->mouse_clicked) {
                    el->pressed = true;
                    result = SIMPLEUI_EVENT_CLICKED;
                }
                break;

            default:
                break;
        }
    }

    return result;
}

void simpleui_draw(simpleui_context* ctx)
{
    for (size_t i = 0; i < ctx->element_count; i++) {
        simpleui_element* el = &ctx->elements[i];
        if (!el->visible) continue;

        simpleui_color bg = el->style.bg_color;
        simpleui_rect r = el->bounds;

        if (el->type == SIMPLEUI_TYPE_BUTTON) {
            if (el->pressed) bg = ctx->primary_color;
            else if (el->hovered) bg = ctx->hover_color;
            else bg = ctx->bg_color;
        }

        if (bg.a > 0) {
            ctx->platform->draw_rect(ctx->platform, r, bg);
        }

        ctx->platform->draw_rect_outline(ctx->platform, r, el->style.border_color, 1);

        switch (el->type) {
            case SIMPLEUI_TYPE_BUTTON:
            case SIMPLEUI_TYPE_LABEL:
                if (el->data.label_text) {
                    int font_id = el->style.font_id >= 0 ? el->style.font_id : ctx->default_font;
                    int text_y = r.y + (r.height + el->style.font_height) / 2;
                    ctx->platform->draw_text(ctx->platform, r.x + 10, text_y,
                                             el->data.label_text, el->style.text_color, font_id);
                }
                break;

            case SIMPLEUI_TYPE_CHECKBOX: {
                if (el->data.checkbox.checked) {
                    simpleui_rect inner = {r.x + 4, r.y + 4, r.width - 8, r.height - 8};
                    ctx->platform->draw_rect(ctx->platform, inner, ctx->primary_color);
                }
                break;
            }

            case SIMPLEUI_TYPE_SLIDER: {
                float range = el->data.slider.max - el->data.slider.min;
                float percent = (el->data.slider.value - el->data.slider.min) / range;
                int knob_x = r.x + (int)(percent * r.width) - 8;
                simpleui_rect knob = {knob_x, r.y + 2, 16, r.height - 4};
                ctx->platform->draw_rect(ctx->platform, knob, ctx->primary_color);
                break;
            }

            case SIMPLEUI_TYPE_DROPDOWN_MENU: {
                if (el->data.dropdown.buffers && el->data.dropdown.selected >= 0 && el->data.dropdown.selected < el->data.dropdown.size) {
                    const char* sel = el->data.dropdown.buffers[el->data.dropdown.selected];
                    int font_id = el->style.font_id >= 0 ? el->style.font_id : ctx->default_font;
                    int text_y = r.y + (r.height + el->style.font_height) / 2;
                    ctx->platform->draw_text(ctx->platform, r.x + 10, text_y, sel, el->style.text_color, font_id);
                }
                if (el->pressed) {
                    for (int j = 0; j < el->data.dropdown.size; j++) {
                        simpleui_rect item_rect = {r.x, r.y + (j + 1) * r.height, r.width, r.height};
                        bool item_over = rect_contains(item_rect, ctx->mouse_x, ctx->mouse_y);
                        ctx->platform->draw_rect(ctx->platform, item_rect, item_over ? ctx->hover_color : ctx->bg_color);
                        ctx->platform->draw_rect_outline(ctx->platform, item_rect, el->style.border_color, 1);
                        if (el->data.dropdown.buffers[j]) {
                            int font_id = el->style.font_id >= 0 ? el->style.font_id : ctx->default_font;
                            int text_y = item_rect.y + (item_rect.height + el->style.font_height) / 2;
                            ctx->platform->draw_text(ctx->platform, item_rect.x + 10, text_y, el->data.dropdown.buffers[j], el->style.text_color, font_id);
                        }
                    }
                }
                break;
            }

            default:
                break;
        }
    }
}
