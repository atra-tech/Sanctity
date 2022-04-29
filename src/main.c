/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  main.c - Program entrypoint 
 *  
 *  */

#include "atra.h"
#include "client.h"
#include "config.h"

int
main(int argc, char **argv)
{
    ATRA_UNUSED(argc);
    ATRA_UNUSED(argv);

    Image window_icon;
    Image cursor_normal;
    Image cursor_ibeam;

    /* TODO: Load config file and set config variables */
    config_load();

    atra_engine_start(
            config_get(CONFIG_WINDOW_WIDTH),
            config_get(CONFIG_WINDOW_HEIGHT),
            "Sanctity");

    window_icon = atra_image_loadfile("res/texture/window_icon.png");
    cursor_normal = atra_image_loadfile("res/texture/cursor_normal.png");
    cursor_ibeam = atra_image_loadfile("res/texture/cursor_ibeam.png");

    atra_window_setminsize(800, 600);
    atra_window_seticon(&window_icon);
    atra_mouse_setcursorimage(ATRA_CURSOR_ARROW, &cursor_normal, 0, 0);
    atra_mouse_setcursorimage(ATRA_CURSOR_IBEAM, &cursor_ibeam, 4, 15);
    atra_mouse_setcursor(ATRA_CURSOR_ARROW);

    atra_image_destroy(&cursor_normal);
    atra_image_destroy(&cursor_ibeam);
    atra_image_destroy(&window_icon);

    client_start();
    client_stop();

    return 0;
}
