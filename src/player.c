/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  player.c - User-controlled entity with special properties
 */

#include "player.h"
#include "ai.h"


Player
player_init(const vec3 pos, const vec3 rot)
{
    if (atra_util_fileexists(PLAYER_ENTITY_PATH) == FALSE) {
        atra_log_print(LOG_WARN, "Unable to find player entity file: Creating default");
        const Cylinder hitbox = {{0.0f, 0.0f, 0.0f}, 0.8, 0.4};
        EntityList *entity = entitylist_create(AI_STATE_NONE, 0, 100,
                PLAYER_ENTITY_PATH,
                ENTITY_RENDER_DYNAMIC,
                hitbox,
                "res/model/gladiator_unified.glb",
                "res/texture/gladiator_albedo.png",
                "res/texture/gladiator_specular.png",
                "res/texture/gladiator_normal.png",
                "res/texture/gladiator_occlusion.png",
                "res/texture/gladiator_emission.png");
        entitylist_savefile(entity, PLAYER_ENTITY_PATH);
    }

    EntityList * const list = db_entity_get_bypath(PLAYER_ENTITY_PATH);

    Player p = {
        .ent = entity_create(list->id, pos, rot)
    };

    return p;
}

void
player_delete(Player *p)
{
    entitylist_remove(db_entity_get(p->ent->ent_type), p->ent->id);
}

void
player_handle(Player *p)
{
    vec2 vel = {0.0f, 0.0f};
   
    vel[0] += atra_key_isdown(ATRA_KEY_D) ? 1.0f : 0.0f;
    vel[0] -= atra_key_isdown(ATRA_KEY_A) ? 1.0f : 0.0f;
    vel[1] += atra_key_isdown(ATRA_KEY_S) ? 1.0f : 0.0f;
    vel[1] -= atra_key_isdown(ATRA_KEY_W) ? 1.0f : 0.0f;

    if (vel[0] != 0.0f || vel[1] != 0.0f) {
        glm_vec2_normalize(vel);
        p->ent->vel[0] = vel[0];
        p->ent->vel[2] = vel[1];
    }
}

void
player_update(Player *p, u32 dt)
{
    
}


