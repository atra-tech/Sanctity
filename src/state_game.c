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
#include "world.h"
#include "particle.h"
#include "entity.h"
#include "player.h"

static Camera camera;
static World world;
static Player player;

static DirLight sunlight;
static Shader depth_shader;
static Shader depth_shader_inst;

static TPool *threadpool;

static vec3 PLAYER_CAM_OFFSET = {0.0, 8.0f, 2.0f};
static float CAMERA_ANGLE = DEG_TO_RAD(-70);

/* Initialise the main game state */
void
state_game_init(void)
{
    const Color sun_color = {0xFF, 0xEE, 0xAA, 0xFF};
    const uint inst_attr_count = 2;
    const uint inst_attr_ids[] = {0, 2};
    const char *inst_attr_names[] = {
        "vert_pos",
        "inst_modelmat"
    };

    vec3 player_pos = {4.0f, 0.0f, -4.0f};
    vec3 player_rot = {0.0f, 0.0f, 0.0f};

    camera = atra_camera_create(ATRA_CAMERA_PROJECTION, 1280, 720);
    atra_camera_setmain(&camera);
    atra_camera_setfov(&camera, DEG_TO_RAD(70));

    sunlight = atra_dirlight_create(ATRA_LIGHT_ORTHO,
            32, 32, SUNCAM_DEF_SIZE, SUNCAM_DEF_SIZE,
            TRUE, FALSE, sun_color, player_pos);
    atra_dirlight_setplanes(&sunlight, 1.0f, 32.0f);

    //threadpool = atra_tpool_create(atra_util_getcorecount());

    db_tile_init();
    db_particle_init();
    db_entity_init();
    
    world = world_create_default();
    player = player_init(player_pos, player_rot);

    glm_vec3_add(player.ent->pos, PLAYER_CAM_OFFSET, camera.pos);
    camera.rot[0] = CAMERA_ANGLE;

    atra_gl_clearcolor(0x00, 0x00, 0x00, 0xFF);

    /* Set up the depth shader */
    depth_shader = atra_shader_loadfile(PATH_DEPTH_VSH, PATH_DEPTH_FSH);
    depth_shader.locs[DEPTH_LOC_ATR_POS] = atra_shader_getattribloc(&depth_shader, "vert_pos");
    depth_shader.locs[DEPTH_LOC_MAT_MODEL] = atra_shader_getuniformloc(&depth_shader, "model_mat");
    depth_shader.locs[DEPTH_LOC_MAT_LIGHTSPACE] = atra_shader_getuniformloc(&depth_shader, "lightspace");

    depth_shader_inst = atra_shader_loadfile_locset(PATH_DEPTH_INST_VSH, PATH_DEPTH_INST_FSH,
            inst_attr_names, inst_attr_ids, inst_attr_count);
    depth_shader_inst.locs[DEPTH_LOC_ATR_POS] = atra_shader_getattribloc(&depth_shader_inst, "vert_pos");
    depth_shader_inst.locs[DEPTH_LOC_MAT_MODEL] = atra_shader_getattribloc(&depth_shader_inst, "inst_modelmat");
    depth_shader_inst.locs[DEPTH_LOC_MAT_LIGHTSPACE] = atra_shader_getuniformloc(&depth_shader_inst, "lightspace");

    atra_dirlight_bindfbo(&sunlight);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    atra_dirlight_unbindfbo();
}

/* Handle device input for the main game state */
void
state_game_handle(void)
{
    if (atra_window_isresized()) {
        atra_camera_setsize(&camera,
                atra_window_getwidth(),
                atra_window_getheight());
        atra_camera_updateprojmat(&camera);
    }

    player_handle(&player);
}

/* Update the main game state */
void
state_game_update(u32 dt)
{
    /*  
     *  Update Projectiles
     *  Update Particles
     *  Update World (check visible chunks for remesh)
     *  Update NPCs (animations, positions, AI, etc.)
     *  Update Player (animation, position)
     *
     */

    player_update(&player, dt);
    db_entity_updateall(dt);

    glm_vec3_add(player.ent->pos, PLAYER_CAM_OFFSET, camera.pos);

    atra_camera_update(&camera);
    state_suncam_update();

    atra_gui_update(dt);

    db_particle_update(dt);
    db_particle_upload();
    world_update(&world);
}

/* Render the main game state */
void
state_game_render(void)
{
    atra_gl_enabledepthtest();
    atra_gl_enablebackfaceculling();

    /* Draw the depth map */
    atra_gl_viewport(0, 0, SUNCAM_DEF_SIZE, SUNCAM_DEF_SIZE);
    atra_gl_clearscreenbuffers();
    atra_dirlight_bindfbo(&sunlight);
    {
        atra_gl_clearscreenbuffers();
        world_drawdepth(&world);
        db_entity_drawall_depth();
    }
    atra_dirlight_unbindfbo();
    atra_window_setupviewport();

    /* Draw the scene */
    atra_gl_clearscreenbuffers();
    atra_gl_enablecolorblend();
    {
        world_draw(&world);
        db_particle_draw_forward();
        db_entity_drawall_forward();
    }

    /* Draw the UI */
    atra_gl_disabledepthtest();
    atra_gui_render();
}

/* Shutdown the main game state */
void
state_game_stop(void)
{
    world_delete(&world);
    //atra_tpool_delete(threadpool);
    atra_shader_destroy(&depth_shader);
    db_entity_shutdown();
}

/* Returns a pointer to the current world instance */
World *
state_world_get(void)
{
    return &world;
}

/* Returns a pointer to the sun camera */
DirLight *
state_suncam_get(void)
{
    return &sunlight;
}

/* Returns a pointer to the sun's depth shader */
Shader *
state_suncam_getshader(void)
{
    return &depth_shader;
}

/* Returns a pointer to the sun's instanced depth shader */
Shader *
state_suncam_getshader_inst(void)
{
    return &depth_shader_inst;
}

/* Updates the sun camera position */
void
state_suncam_update(void)
{
    sunlight.rot[0] = world.sun_rot[0];
    sunlight.rot[1] = world.sun_rot[1];
    sunlight.rot[2] = world.sun_rot[2] + DEG_TO_RAD(15);
    atra_dirlight_updatevectors(&sunlight);
    
    sunlight.pos[0] = player.ent->pos[0] - sunlight.dir[0] * 16 + 2;
    sunlight.pos[1] = player.ent->pos[1] - sunlight.dir[1] * 16;
    sunlight.pos[2] = player.ent->pos[2] - sunlight.dir[2] * 16 - 4;
    atra_dirlight_updateview(&sunlight);
}
