/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  state_game.c - Engine state for main gameplay
 *  
 *  */


#include "state_game.h"
#include "script.h"

static Camera camera;


/* Initialise the main game state */
void
state_game_init(void)
{
    camera = atra_camera_create(ATRA_CAMERA_PROJECTION, 1280, 720);
    atra_camera_setmain(&camera);

    //script_init();
}

/* Handle device input for the main game state */
void
state_game_handle(void)
{   

}

/* Update the main game state */
void
state_game_update(u32 dt)
{

}

/* Render the main game state */
void
state_game_render(void)
{

}

/* Shutdown the main game state */
void
state_game_stop(void)
{
}
