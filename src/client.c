/*
 *  ALCHYMIST
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  client.c - Manages client-side processing, communicates with server.
 *  
 *  */

#include "client.h"
#include "server.h"
#include "state_game.h"

void
client_start(void)
{
    atra_state_push(
            state_game_init,
            state_game_handle,
            state_game_update,
            state_game_render,
            state_game_stop);

    while (atra_window_shouldclose() == FALSE) {
        atra_state_handle();
        atra_state_update();
        atra_state_render();
    }
    atra_state_pop();
}

void
client_stop(void)
{
}
