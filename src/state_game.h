/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  state_game.h - Engine state for main gameplay
 *  
 *  */


#include "atra.h"

/* Initialise the main game state */
void state_game_init(void);

/* Handle device input for the main game state */
void state_game_handle(void);

/* Update the main game state */
void state_game_update(u32 dt);

/* Render the main game state */
void state_game_render(void);

/* Shutdown the main game state */
void state_game_stop(void);
