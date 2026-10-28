/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  player.h - User-controlled entity with special properties
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "atra.h"
#include "entity.h"

#define PLAYER_MOVESPEED    1.0f

#define PLAYER_ENTITY_PATH  "res/entity/player.ent"

typedef struct PlayerAttribs {
    uint luck;          /* Luck stat: Increases drop rate/amount/quality */
    float dodge_chance; /* Percentage chance for player to negate all damage */
    float cost_reduce;  /* Percentage reduction of purchase costs */
    float exp_gain;     /* Percentage experience gain bonus */
} PlayerAttribs;

typedef struct Player {
    Entity *ent;            /* Pointer to player entity in entity database */
    PlayerAttribs p_attrs;  /* Player-specific  */
} Player;


Player player_init(const vec3 pos, const vec3 rot);

void player_delete(Player *p);

void player_handle(Player *p);

void player_update(Player *p, u32 dt);


#endif /* PLAYER_H */
