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

int main(void)
{
    simpleui_platform* platform = PLATFORM_INIT;
    if (!platform) {
        fprintf(stderr, "Failed to init platform\n");
        return 1;
    }

    if (!platform->init(platform, 400, 300, "SimpleUI Demo")) {
        fprintf(stderr, "Failed to create window\n");
        return 1;
    }

	char* options[] = {
		"option1",
		"option2",
		"option3",
		"option4"
	};

    simpleui_context ctx;
    simpleui_init(&ctx, platform);

    simpleui_add_button(&ctx, "btn1", 20, 20, 100, 30, "Click");
    simpleui_element* btn1 = simpleui_add_button(&ctx, "btn2", 130, 20, 100, 30, "Hello");
    simpleui_element* lbl1 = simpleui_add_label(&ctx, "lbl1", 50, 70, "SimpleUI Cross-Platform"); (void)lbl1;
    simpleui_element* chk1 = simpleui_add_checkbox(&ctx, "chk1", 20, 90, false);                  (void)chk1;
    simpleui_element* sld1 = simpleui_add_slider(&ctx, "sld1", 20, 130, 200, 0, 100, 50);         (void)sld1;
    simpleui_element* btn3 = simpleui_add_button(&ctx, "btn3", 20, 170, 100, 30, "Quit");
	simpleui_element* dropdown = simpleui_add_dropdown(&ctx, "dropdown1", 50, 50, 100, 24, options, 4, 0); (void)dropdown;

    while (!platform->get_quit(platform)) {
        platform->begin_frame(platform);

        platform->clear(platform, SIMPLEUI_COLOR(40, 40, 40, 255));

        simpleui_begin(&ctx);

        int mx, my;
        bool md, mc;
        platform->get_mouse(platform, &mx, &my, &md, &mc);

        ctx.mouse_x = mx;
        ctx.mouse_y = my;
        ctx.mouse_down = md;
        ctx.mouse_clicked = mc;

        simpleui_event evt = simpleui_process(&ctx); (void)evt;
        simpleui_draw(&ctx);

        if (btn1 && btn1->pressed) {
			fetch_resources(&ctx);
			btn1->pressed = false;
		}

        if (btn3 && btn3->pressed) {
			printf("Bye\n");
            break;
        }

        simpleui_end(&ctx);

        platform->end_frame(platform);
    }

    platform->shutdown(platform);
    free(platform);

    return 0;
}
