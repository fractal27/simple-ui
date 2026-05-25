#include <string.h>
#include <stdio.h>
#include <math.h>

#include "simpleui.h"

#define DEFAULT_BG SIMPLEUI_COLOR(40, 40, 40, 255)
#define DEFAULT_PRIMARY SIMPLEUI_COLOR(66, 135, 245, 255)
#define DEFAULT_BG_GRAPH SIMPLEUI_COLOR(255, 255, 255, 255)
#define DEFAULT_TEXT SIMPLEUI_COLOR(255, 255, 255, 255)
#define DEFAULT_BORDER SIMPLEUI_COLOR(100, 100, 100, 255)
#define DEFAULT_HOVER SIMPLEUI_COLOR(90, 90, 90, 255)
#define DEFAULT_FONT_SIZE 23

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef DEBUG
#define DEBUG_EVENT(...) printf(__VA_ARGS__)
#else 
#define DEBUG_EVENT(...) 
#endif

void simpleui_init(simpleui_context* ctx, simpleui_platform* platform)
{
    memset(ctx, 0, sizeof(simpleui_context));
    ctx->platform = platform;
    ctx->rendered_once = false;
    ctx->graph_bg_color = DEFAULT_BG_GRAPH;
    ctx->bg_color = DEFAULT_BG;
    ctx->primary_color = DEFAULT_PRIMARY;
    ctx->text_color = DEFAULT_TEXT;
    ctx->border_color = DEFAULT_BORDER;
    ctx->hover_color = DEFAULT_HOVER;
    ctx->default_font = platform->load_font(platform,
			"/usr/share/fonts/urw-fonts/NimbusSans-Regular.ttf",
			DEFAULT_FONT_SIZE);
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
    el->style.render_border = true;
    el->style.font_height = DEFAULT_FONT_SIZE;
    el->style.font_id = ctx->default_font;
    el->data.label_text = text;

    return el;
}

simpleui_element* simpleui_add_label(simpleui_context* ctx, const char* id, int x, int y, const char* text)
{
    if (ctx->element_count >= SIMPLEUI_MAX_ELEMENTS) return NULL;

    simpleui_element* el = &ctx->elements[ctx->element_count++];
    el->type = SIMPLEUI_TYPE_LABEL;
    el->bounds = (simpleui_rect){x, y, 0, 0};
    el->visible = true;
    el->enabled = true;
    el->hovered = false;
    el->id = id;
    el->user_data = NULL;
    el->style.text_color = ctx->text_color;
    el->style.bg_color = SIMPLEUI_COLOR(0, 0, 0, 0);
    el->style.border_color = SIMPLEUI_COLOR(0, 0, 0, 0);
    el->style.render_border = false;
    el->style.font_height = DEFAULT_FONT_SIZE;
    el->style.font_id = ctx->default_font;
    el->data.label_text = text;
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

simpleui_element* simpleui_add_graph(simpleui_context* ctx, const char* id, simpleui_graph_type type,
              unsigned x, unsigned y, unsigned w, unsigned h, 
			  float min_x, float max_x, float min_y, float max_y,
              float* data_x, size_t data_x_size, float* data_y, size_t data_y_size)
{
    if (ctx->element_count >= SIMPLEUI_MAX_ELEMENTS) return NULL;

    simpleui_element* el = &ctx->elements[ctx->element_count++];
    el->type = SIMPLEUI_TYPE_GRAPH;
    el->bounds = (simpleui_rect){x, y, w, h};
    el->visible = true;
    el->enabled = true;
    el->hovered = false;
    el->pressed = false;
    el->id = id;
    el->user_data = NULL;
    el->style.text_color = ctx->text_color;
    el->style.bg_color = ctx->graph_bg_color;
    el->style.border_color = ctx->border_color;
    el->style.font_height = 16;
    el->data.graph.type = type;
    el->data.graph.x = x; el->data.graph.y = y;
    el->data.graph.w = w; el->data.graph.h = h;
    el->data.graph.max_x = max_x; el->data.graph.max_y = max_y;
    el->data.graph.min_x = min_x; el->data.graph.min_y = min_y;
    el->data.graph.data_x = data_x; el->data.graph.data_x_size = data_x_size;
    el->data.graph.data_y = data_y; el->data.graph.data_y_size = data_y_size;

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
	if(!ctx->rendered_once){
		DEBUG_EVENT("event: rendered_once\n");
		result = SIMPLEUI_EVENT_VALUE_CHANGED;
		ctx->rendered_once = true;
	}

    for (size_t i = 0; i < ctx->element_count; i++) {
        simpleui_element* el = &ctx->elements[i];
        if (!el->visible || !el->enabled) continue;

        bool over = rect_contains(el->bounds, ctx->mouse_x, ctx->mouse_y);
		if(el->hovered != over) {
			DEBUG_EVENT("event: changed element hover\n");
			result = SIMPLEUI_EVENT_CHANGED_HOVER;
			el->hovered = over;
		}

        if (over) {
            ctx->hovered_element = el;
        }

        switch (el->type) {
            case SIMPLEUI_TYPE_BUTTON:
                if (over && ctx->mouse_clicked) {
                    el->pressed = true;
					DEBUG_EVENT("event: button clicked\n");
                    result = SIMPLEUI_EVENT_CLICKED;
                }
                if (!ctx->mouse_down) {
                    el->pressed = false;
                }
                break;

            case SIMPLEUI_TYPE_CHECKBOX:
                if (over && ctx->mouse_clicked) {
                    el->data.checkbox.checked = !el->data.checkbox.checked;
					DEBUG_EVENT("event: checkbox toggle\n");
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
					DEBUG_EVENT("event: slider value changed\n");
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
							DEBUG_EVENT("event: dropdown value changed\n");
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
					DEBUG_EVENT("event: clicked\n");
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

        if(el->style.render_border){
			ctx->platform->draw_rect_outline(ctx->platform, r, el->style.border_color, 1);
		}
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

			case SIMPLEUI_TYPE_GRAPH:{
				  simpleui_data_graph* g = &el->data.graph;
				  switch (g->type) {
					  case SIMPLEUI_GRAPH_TYPE_LINE: {
						  size_t n = g->data_x_size < g->data_y_size ? g->data_x_size : g->data_y_size;
						  if (n < 2) break;
						  float range_x = (float)(g->max_x - g->min_x);
						  float range_y = (float)(g->max_y - g->min_y);
						  if (range_x == 0) range_x = 1;
						  if (range_y == 0) range_y = 1;
						  int graph_top = r.y;
						  int graph_bottom = r.y + r.height;
						  for (size_t i = 0; i < n - 1; i++) {
							  int x1 = r.x + (int)((g->data_x[i] - g->min_x) / range_x * r.width);
							  int y1 = r.y + r.height - (int)((g->data_y[i] - g->min_y) / range_y * r.height);
							  int x2 = r.x + (int)((g->data_x[i+1] - g->min_x) / range_x * r.width);
							  int y2 = r.y + r.height - (int)((g->data_y[i+1] - g->min_y) / range_y * r.height);
							  if (y1 < graph_top) y1 = graph_top;
							  if (y1 > graph_bottom) y1 = graph_bottom;
							  if (y2 < graph_top) y2 = graph_top;
							  if (y2 > graph_bottom) y2 = graph_bottom;
							  ctx->platform->draw_line(ctx->platform, x1, y1, x2, y2, ctx->primary_color, 1);
						  }
						  break;
					  }
					  case SIMPLEUI_GRAPH_TYPE_AREA: {
						  size_t n = g->data_x_size < g->data_y_size ? g->data_x_size : g->data_y_size;
						  if (n < 2) break;
						  float range_x = (float)(g->max_x - g->min_x);
						  float range_y = (float)(g->max_y - g->min_y);
						  if (range_x == 0) range_x = 1;
						  if (range_y == 0) range_y = 1;
						  int graph_top = r.y;
						  int bottom = r.y + r.height;
						  simpleui_color fill_color = ctx->primary_color;
						  fill_color.a = 80;
						  for (size_t i = 0; i < n - 1; i++) {
							  int x1 = r.x + (int)((g->data_x[i] - g->min_x) / range_x * r.width);
							  int y1 = bottom - (int)((g->data_y[i] - g->min_y) / range_y * r.height);
							  int x2 = r.x + (int)((g->data_x[i+1] - g->min_x) / range_x * r.width);
							  int y2 = bottom - (int)((g->data_y[i+1] - g->min_y) / range_y * r.height);
							  if (x2 <= x1) continue;
							  for (int x = x1; x <= x2; x++) {
								  float t = (float)(x - x1) / (float)(x2 - x1);
								  int y = y1 + (int)(t * (float)(y2 - y1));
								  if (y < graph_top) y = graph_top;
								  if (y > bottom) y = bottom;
								  ctx->platform->draw_line(ctx->platform, x, y, x, bottom, fill_color, 1);
							  }
						  }
						  for (size_t i = 0; i < n - 1; i++) {
							  int x1 = r.x + (int)((g->data_x[i] - g->min_x) / range_x * r.width);
							  int y1 = bottom - (int)((g->data_y[i] - g->min_y) / range_y * r.height);
							  int x2 = r.x + (int)((g->data_x[i+1] - g->min_x) / range_x * r.width);
							  int y2 = bottom - (int)((g->data_y[i+1] - g->min_y) / range_y * r.height);
							  if (y1 < graph_top) y1 = graph_top;
							  if (y1 > bottom) y1 = bottom;
							  if (y2 < graph_top) y2 = graph_top;
							  if (y2 > bottom) y2 = bottom;
							  ctx->platform->draw_line(ctx->platform, x1, y1, x2, y2, ctx->primary_color, 2);
						  }
						  break;
					  }
					  case SIMPLEUI_GRAPH_TYPE_CHART: {
						  size_t n = g->data_y_size;
						  if (n < 1) break;
						  float total = 0;
						  for (size_t i = 0; i < n; i++) total += g->data_y[i];
						  if (total <= 0) break;
						  int cx = r.x + r.width / 2;
						  int cy = r.y + r.height / 2;
						  int radius = (r.width < r.height ? r.width : r.height) / 2 - 1;
						  if (radius < 2) break;
						  static const unsigned int pie_colors[][4] = {
							  {66, 135, 245, 255}, {245, 66, 66, 255},
							  {66, 245, 135, 255}, {245, 200, 66, 255},
							  {200, 66, 245, 255}, {66, 200, 245, 255},
							  {245, 135, 66, 255}, {135, 66, 245, 255},
						  };
						  float start_angle = -M_PI / 2.0f;
						  for (size_t i = 0; i < n; i++) {
							  float sweep = (g->data_y[i] / total) * 2.0f * M_PI;
							  float end_angle = start_angle + sweep;
							  int segs = (int)(sweep / 0.05f) + 2;
							  if (segs < 2) segs = 2;
							  if (segs > 100) segs = 100;
							  int num_verts = segs + 2;
							  int px[num_verts], py[num_verts];
							  px[0] = cx; py[0] = cy;
							  for (int j = 0; j <= segs; j++) {
								  float a = start_angle + sweep * j / segs;
								  px[j+1] = cx + (int)(radius * cos(a));
								  py[j+1] = cy + (int)(radius * sin(a));
							  }
							  int ci = i % 8;
							  simpleui_color slice_color = SIMPLEUI_COLOR(pie_colors[ci][0], pie_colors[ci][1], pie_colors[ci][2], pie_colors[ci][3]);
							  int min_y = py[0], max_y = py[0];
							  for (int j = 1; j < num_verts; j++) {
								  if (py[j] < min_y) min_y = py[j];
								  if (py[j] > max_y) max_y = py[j];
							  }
							  int xs[128];
							  for (int y = min_y; y <= max_y; y++) {
								  int xc = 0;
								  for (int j = 0; j < num_verts; j++) {
									  int k = (j + 1) % num_verts;
									  int y1 = py[j], y2 = py[k];
									  if (y1 == y2) continue;
									  if ((y >= y1 && y < y2) || (y >= y2 && y < y1)) {
										  int x = px[j] + (y - y1) * (px[k] - px[j]) / (y2 - y1);
										  if (xc < 128) xs[xc++] = x;
									  }
								  }
								  for (int a = 0; a < xc; a++) {
									  for (int b = a + 1; b < xc; b++) {
										  if (xs[a] > xs[b]) { int t = xs[a]; xs[a] = xs[b]; xs[b] = t; }
									  }
								  }
								  for (int a = 0; a + 1 < xc; a += 2) {
									  ctx->platform->draw_line(ctx->platform, xs[a], y, xs[a+1], y, slice_color, 1);
								  }
							  }
							  start_angle = end_angle;
						  }
						  start_angle = -M_PI / 2.0f;
						  for (size_t i = 0; i < n; i++) {
							  float sweep = (g->data_y[i] / total) * 2.0f * M_PI;
							  float end_angle = start_angle + sweep;
							  int segs = (int)(sweep / 0.05f) + 2;
							  if (segs < 2) segs = 2;
							  if (segs > 100) segs = 100;
							  for (int j = 0; j < segs; j++) {
								  float a1 = start_angle + sweep * j / segs;
								  float a2 = start_angle + sweep * (j+1) / segs;
								  int x1 = cx + (int)(radius * cos(a1));
								  int y1 = cy + (int)(radius * sin(a1));
								  int x2 = cx + (int)(radius * cos(a2));
								  int y2 = cy + (int)(radius * sin(a2));
								  ctx->platform->draw_line(ctx->platform, x1, y1, x2, y2, ctx->border_color, 1);
							  }
							  int ex = cx + (int)(radius * cos(end_angle));
							  int ey = cy + (int)(radius * sin(end_angle));
							  ctx->platform->draw_line(ctx->platform, cx, cy, ex, ey, ctx->border_color, 1);
							  start_angle = end_angle;
						  }
						  int sx = cx + (int)(radius * cos(-M_PI/2.0));
						  int sy = cy + (int)(radius * sin(-M_PI/2.0));
						  ctx->platform->draw_line(ctx->platform, cx, cy, sx, sy, ctx->border_color, 1);
						  break;
					  }
					  case SIMPLEUI_GRAPH_TYPE_COLS: {
						  int block = r.width / g->data_x_size; 
						  int full = block * .75;

						  int yrange = g->max_y - g->min_y;
						  for(int i = 0; i < (int)g->data_x_size; i++)
						  {
							  int current_x = r.x + block * i;
							  int current_y = r.y + r.height * (1 - (g->data_y[i] + g->min_y) / yrange);
							  simpleui_rect rect = {current_x, current_y, full, r.y + r.width - current_y};
							  ctx->platform->draw_rect(ctx->platform, rect, ctx->primary_color);
						  }
						  break;
					  }
					  case SIMPLEUI_GRAPH_TYPE_ROWS: {
						  int block = r.height / g->data_x_size; 
						  int full = block * .75;

						  int rangey = g->max_y - g->min_y;
						  for(int i = 0; i < (int)g->data_x_size; i++)
						  {
							  int current_y = r.y + block * i;
							  simpleui_rect rect = {r.x, 
								  current_y, 
								  r.width * (g->data_y[i] + g->min_y) / rangey,
								  full};
							  ctx->platform->draw_rect(ctx->platform, rect, ctx->primary_color);
						  }
						  break;
					  }
					  default:
					      printf("GRAPH TYPE UNKNOWN: %d, SIMPLEUI_GRAPH_TYPE_LINE=%d\n", g->type, SIMPLEUI_GRAPH_TYPE_LINE);
						  break;
				  }
				  break;
			 }
            default:
                break;
        }
    }
}
