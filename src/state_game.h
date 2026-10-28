/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  state_game.h - Engine state for main gameplay
 *  
 *  */


#include "atra.h"

#define SUNCAM_DEF_SIZE     2048
#define PATH_DEPTH_VSH      "res/shader/depth_vert.glsl"
#define PATH_DEPTH_FSH      "res/shader/depth_frag.glsl"
#define PATH_DEPTH_INST_VSH "res/shader/depth_vert_inst.glsl"
#define PATH_DEPTH_INST_FSH "res/shader/depth_frag_inst.glsl"

enum depth_shader_locs {
    DEPTH_LOC_ATR_POS,
    DEPTH_LOC_MAT_MODEL,
    DEPTH_LOC_ATR_UVS_0,
    DEPTH_LOC_ATR_UVS_1,
    DEPTH_LOC_ATR_UVS_2,
    DEPTH_LOC_ATR_UVS_3,
    DEPTH_LOC_MAT_LIGHTSPACE
};

typedef struct World World;

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

/* Returns a pointer to the current world instance */
World * state_world_get(void);

/* Returns a pointer to the sun camera */
DirLight * state_suncam_get(void);

/* Returns a pointer to the sun's depth shader */
Shader * state_suncam_getshader(void);

/* Returns a pointer to the sun's instanced depth shader */
Shader * state_suncam_getshader_inst(void);

/* Updates the sun camera position */
void state_suncam_update(void);

