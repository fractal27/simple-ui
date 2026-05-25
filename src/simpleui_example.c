#include <stdio.h>
#include <stdlib.h>
#include "simpleui.h"

#ifdef _WIN32
#define PLATFORM_INIT simpleui_platform_win32_init()
#else
#include <unistd.h>
#define PLATFORM_INIT simpleui_platform_x11_init()
#endif

void fetch_resources(simpleui_context* ctx){
	for(int x = 0; x < 1000; x++){
		printf("[%.2lf%%]fetching resources...\n",(double)x/10);
		usleep(1000);
	}
}
float identity_f(float x) { return x; }
float func(float x) { return x*x; }

void data_from_func(float (*f)(float), float x_min, float x_max, size_t n, float** x, float** y){
	float* x_data;
	float* y_data;
	if(x != NULL) x_data = calloc(1,sizeof(float)*n);
	if(y != NULL) y_data = calloc(1,sizeof(float)*n);
	float dx = (x_max - x_min) / n;
	size_t i = 0;
	for(float curr_x = x_min; curr_x < x_max; curr_x += dx){
		if(x != NULL) x_data[i] = curr_x;
		if(y != NULL) y_data[i] = f(curr_x);
		i++;
	}
	if(x != NULL) *x = x_data;
	if(y != NULL) *y = y_data;
}


int main(void)
{
    simpleui_platform* platform = PLATFORM_INIT;
    if (!platform) {
        fprintf(stderr, "Failed to init platform\n");
        return 1;
    }

    if (!platform->init(platform, 1920, 1080, "SimpleUI Demo")) {
        fprintf(stderr, "Failed to create window\n");
        return 1;
    }

    simpleui_context ctx;
    simpleui_init(&ctx, platform);

	int base_x = 0;
	int base_y = 0;

    simpleui_add_label(&ctx, "label", base_x += 10, base_y += 10, "SimpleUI Cross-Platform");

    simpleui_element* btn1 = simpleui_add_button(&ctx, "install_button", base_x += 20, base_y += 50, 100, 30, "Install");

	float *x_data, *y_data;
	float* identity_y_data;

	data_from_func(func, 0.0, 10.0, 1000, &x_data, &y_data);
	data_from_func(identity_f, 0.0, 10.0, 10, NULL, &identity_y_data);


	base_y += 50;
	simpleui_add_graph(&ctx, "graph_line", SIMPLEUI_GRAPH_TYPE_LINE,
			base_x, base_y, 150, 150,
			0.0, 10.0, 0.0, 10.0,
			x_data, 1000, y_data, 1000);
	printf("graph_line.x = %d\n",base_x);

	base_x += 160;
	simpleui_add_graph(&ctx, "graph_area", SIMPLEUI_GRAPH_TYPE_AREA,
			base_x, base_y, 150, 150,
			0.0, 10.0, 0.0, 10.0,
			x_data, 1000, y_data, 1000);
	printf("graph_area.x = %d\n",base_x);

	base_x -= 160;
   	base_y += 160;

	simpleui_add_graph(&ctx, "graph_chart", SIMPLEUI_GRAPH_TYPE_CHART,
			base_x, base_y, 150, 150,
			0.0, 10.0, 0.0, 10.0,
			x_data, 10, identity_y_data, 10);

	base_x += 160;
	simpleui_add_graph(&ctx, "graph_cols", SIMPLEUI_GRAPH_TYPE_COLS,
			base_x, base_y, 150, 150,
			0.0, 10.0, 0.0, 10.0,
			x_data, 10, identity_y_data, 10);

	base_y += 160;
	simpleui_add_graph(&ctx, "graph_rows", SIMPLEUI_GRAPH_TYPE_ROWS,
			base_x, base_y, 150, 150,
			0.0, 10.0, 0.0, 10.0,
			x_data, 10, identity_y_data, 10);

    simpleui_element* btn3 = simpleui_add_button(&ctx, "quit_button", base_x, base_y += 200, 100, 30, "Quit");

    while (!platform->get_quit(platform)) {
        platform->begin_frame(platform);

        platform->get_mouse(platform, &ctx.mouse_x, &ctx.mouse_y, &ctx.mouse_down, &ctx.mouse_clicked);

        simpleui_event evt = simpleui_process(&ctx);
		if(evt != SIMPLEUI_EVENT_NONE){
			platform->clear(platform, SIMPLEUI_COLOR(40, 40, 40, 255));
			simpleui_draw(&ctx);

			if (btn1 && btn1->pressed) {
				fetch_resources(&ctx);
				btn1->pressed = false;
			}

			if (btn3 && btn3->pressed) {
				printf("Bye\n");
				break;
			}
		}

        platform->end_frame(platform);
		usleep(8000);
    }

    platform->shutdown(platform);
    free(platform);

    return 0;
}
