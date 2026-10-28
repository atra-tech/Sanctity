/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  projectile.h - Interactable projectiles which affect the world and entities
 *
 */

#include "projectile.h"
#include "state_game.h"
#include "world.h"
#include "terrain.h"
#include "math_tools.h"
#include "file.h"

ATRA_LIST_DECLARE(ProjectileData)
ATRA_LIST_DEFINE(ProjectileData)
ATRA_LIST_DEFINE(Projectile)

List_ProjectileData proj_db;
HashMap proj_path_map;

static void projectile_calcworldcollisions(ProjectileData *list, Projectile *p, World *world);
static void projectile_calcterraincollisions(ProjectileData *list, Projectile *p, Terrain *chunk);
static void projectile_calctilecollisions(ProjectileData *list, Projectile *p, TileChunk *chunk, ivec3 pos);
static void projectile_calcentitycollisions(ProjectileData *list, Projectile *p, EntityList *ents);
static void projectile_updatephysics(ProjectileData *list, Projectile *p, u32 dt);

/* Initialise the projectile database */
void
projectile_init(void)
{
    proj_db = atra_list_ProjectileData_create(PROJECTILE_DB_INIT_CAP);
    proj_path_map = atra_hashmap_create(sizeof(char *), sizeof(uint), atra_hashmap_str_hash, atra_hashmap_str_eq);
}

/* Shudown the projectile database */
void
projectile_shutdown(void)
{
    uint i;

    for (i = 0; i < proj_db.size; i++) {
        projectilelist_delete(&proj_db.data[i]);
    }
    atra_list_ProjectileData_delete(&proj_db);
    atra_hashmap_delete(&proj_path_map, HASH_FREE_KEY);
}

/* Load a projectile type into the database from a file. Returns the projectile ID */
uint
projectile_loadfile(const char *path)
{
    ProjectileData list = projectilelist_loadfile(path);
    const char *key;

    atra_list_ProjectileData_addptr(&proj_db, &list);
    
    list.id = proj_db.size - 1;
    key = atra_util_strdup(path);
    atra_hashmap_add(&proj_path_map, &key, &list.id);

    return list.id;
}

/* Add a given projectile data entry to the database */
uint
projectile_pushlist(ProjectileData *list)
{
    const char *key;

    atra_list_ProjectileData_addptr(&proj_db, list);

    list->id = proj_db.size - 1;
    key = atra_util_strdup(list->file_path);
    atra_hashmap_add(&proj_path_map, &key, &list->id);

    return list->id;
}

/* Returns a pointer to the projectile data list at the given ID */
ProjectileData *
projectile_getlist(uint id)
{
    return atra_list_ProjectileData_getptr(&proj_db, id);
}

/* Returns a pointer to the projectile data list associated with the given path.
 * Implicitly loads the projectile at given path if it is not present in the database. */
ProjectileData *
projectile_getlist_bypath(const char *path)
{
    return projectile_getlist(projectile_getid_bypath(path));
}

/* Returns the ID of the projectile data list associated with the given path.
 * Implicitly loads the projectile at given path if it is not present in the database. */
uint
projectile_getid_bypath(const char *path)
{
    uint *id_p = atra_hashmap_get(&proj_path_map, &path);

    return id_p == NULL ? projectile_loadfile(path) : *id_p;
}

/* Update all projectiles */
void
projectile_updateall(u32 dt)
{
    uint i;

    for (i = 0; i < proj_db.size; i++) {
        projectilelist_update(&proj_db.data[i], dt);
    }
}

/* Draw all projectiles */
void
projectile_drawall(void)
{
    /*
     *  Don't bother with instancing.  Unlikely that a LOT of meshes will be present
     */
}


/* Create a new, empty projectile data list */
/* TODO: Set up to allow custom specification of all projectile data fields */
ProjectileData
projectilelist_create(void)
{
    ProjectileData list = {0};

    list.projs = atra_list_Projectile_create(PROJECTILE_LIST_INIT_CAP);

    return list;
}

/* Manually sets up a projectile list */
ProjectileData projectilelist_create_manual(u32 attrs, i32 max_life, const char *save_path,
        int damage_start, int damage_end,
        float speed_start, float speed_end,
        float scale_start, float scale_end,
        float force_start, float force_end,
        uint tick_rate, float radius, float gravity)
{
    ProjectileData list = {0};

    list.attrs = attrs;
    list.max_life = max_life;
    list.file_path = atra_util_strdup(save_path);

    list.damage_start = damage_start;
    list.damage_end = damage_end;
    list.speed_start = speed_start;
    list.speed_end = speed_end;
    list.scale_start = scale_start;
    list.scale_end = scale_end;
    list.force_start = force_start;
    list.force_end = force_end;

    list.tick_rate = tick_rate;
    list.radius = radius;
    list.gravity = gravity;

    list.projs = atra_list_Projectile_create(PROJECTILE_LIST_INIT_CAP);

    return list;
}

/* Load a new projectile type from given file path */
ProjectileData
projectilelist_loadfile(const char *path)
{
    /* TODO: Saving projectile IDs directly is a no-go
     *       Figure something out to tie IDs to file paths
     *       or something more concrete */

    FileData file = file_load(path);
    ProjectileData list = {0};
    ProjectileEvent *e;
    char *p_path;
    uint count;
    uint i, j;

    list.attrs = file_read_u32(&file);
    list.max_life = file_read_i32(&file);
    list.file_path = atra_util_strdup(path);
    
    list.damage_start = file_read_int(&file);
    list.damage_end = file_read_int(&file);
    list.speed_start = file_read_float(&file);
    list.speed_end = file_read_float(&file);
    list.scale_start = file_read_float(&file);
    list.scale_end = file_read_float(&file);
    list.force_start = file_read_float(&file);
    list.force_end = file_read_float(&file);
    
    list.tick_rate = file_read_uint(&file);
    list.radius = file_read_float(&file);
    list.gravity = file_read_float(&file);

    for (i = 0; i < PROJECTILE_EVENT_TOTAL; i++) {
        list.event_flags[i] = file_read_int(&file);
        
        if (list.event_flags[i] != 0) {
            e = &list.events[i];
            e->type = i;
            p_path = file_read_string(&file);

            if (p_path != NULL) {
                e->sound_id = atra_audio_buffer_getid_bypath(p_path);
            }
            count = file_read_uint(&file);

            for (j = 0; j < count; j++) {
                p_path = file_read_string(&file);
                
                atra_list_uint_add(&e->prt_ids, db_particle_getid_bypath(p_path));
                atra_list_uint_add(&e->prt_counts, file_read_uint(&file));
                atra_list_uint_add(&e->prt_emittype, file_read_uint(&file));
                atra_list_float_add(&e->prt_spread, file_read_float(&file));

                free(p_path);
            }
            count = file_read_uint(&file);

            for (j = 0; j < count; j++) {
                p_path = file_read_string(&file);
                
                atra_list_uint_add(&e->prj_ids, projectile_getid_bypath(p_path));
                atra_list_uint_add(&e->prj_counts, file_read_uint(&file));
                atra_list_uint_add(&e->prj_emittype, file_read_uint(&file));
                atra_list_float_add(&e->prj_spread, file_read_float(&file));

                free(p_path);
            }
        }
    }
    file_delete(file);
    
    return list;
}

/* Save a projectile type to file at path */
void
projectilelist_savefile(ProjectileData *list)
{
    FileData file = file_create(0);
    ProjectileData *proj;
    ParticleList *part;
    ProjectileEvent *e;
    AudioBuffer *audio_buf;
    uint i, j;

    file_write_u32(&file, list->attrs);
    file_write_i32(&file, list->max_life);
    
    file_write_int(&file, list->damage_start);
    file_write_int(&file, list->damage_end);
    file_write_float(&file, list->speed_start);
    file_write_float(&file, list->speed_end);
    file_write_float(&file, list->scale_start);
    file_write_float(&file, list->scale_end);
    file_write_float(&file, list->force_start);
    file_write_float(&file, list->force_end);

    file_write_uint(&file, list->tick_rate);
    file_write_float(&file, list->radius);
    file_write_float(&file, list->gravity);

    for (i = 0; i < PROJECTILE_EVENT_TOTAL; i++) {
        file_write_int(&file, list->event_flags[i]);

        if (list->event_flags[i] != 0) {
            e = &list->events[i];

            if (e->sound_id != 0) {
                audio_buf = atra_audio_buffer_get(e->sound_id);
                file_write_string(&file, audio_buf->file_path);
            } else {
                file_write_string_empty(&file);
            }
            file_write_uint(&file, e->prt_ids.size);

            for (j = 0; j < e->prt_ids.size; j++) {
                part = db_particle_getlist(e->prt_ids.data[j]);
                file_write_string(&file, part->file_path);
                file_write_uint(&file, e->prt_counts.data[j]);
                file_write_uint(&file, e->prt_emittype.data[j]);
                file_write_float(&file, e->prt_spread.data[j]);
            }
            file_write_uint(&file, e->prj_ids.size);

            for (j = 0; j < e->prj_ids.size; j++) {
                proj = projectile_getlist(e->prj_ids.data[j]);
                file_write_string(&file, proj->file_path);
                file_write_uint(&file, e->prj_counts.data[j]);
                file_write_uint(&file, e->prj_emittype.data[j]);
                file_write_float(&file, e->prj_spread.data[j]);
            }
        }
    }
    file_delete(file);
}

/* Delete a projectile list, and all contained projectiles */
void
projectilelist_delete(ProjectileData *list)
{
    uint i;

    for (i = 0; i < PROJECTILE_EVENT_TOTAL; i++) {
        if (list->event_flags[i] == 0) {
            continue;
        }
        atra_list_uint_delete(&list->events[i].prt_ids);
        atra_list_uint_delete(&list->events[i].prt_counts);
        atra_list_uint_delete(&list->events[i].prt_emittype);
        atra_list_float_delete(&list->events[i].prt_spread);
        atra_list_uint_delete(&list->events[i].prj_ids);
        atra_list_uint_delete(&list->events[i].prj_counts);
        atra_list_uint_delete(&list->events[i].prj_emittype);
        atra_list_float_delete(&list->events[i].prj_spread);
    }
    free(list->file_path);
    atra_list_Projectile_delete(&list->projs);
}

/* Create a new projectile for the given list with supplied attributes */
void
projectilelist_add_ptr(ProjectileData *list, i32 life, vec3 pos, vec3 rot, vec3 dir, float grav, Entity *origin)
{
    Projectile p;

    p.life = life;

    p.pos[0] = pos[0];
    p.pos[1] = pos[1];
    p.pos[2] = pos[2];

    p.rot[0] = rot[0];
    p.rot[1] = rot[1];
    p.rot[2] = rot[2];

    p.dir[0] = dir[0];
    p.dir[1] = dir[1];
    p.dir[2] = dir[2];

    p.grav = grav;
    p.origin = origin;
    p.snd_src = atra_audio_source_create();

    atra_list_Projectile_addptr(&list->projs, &p);
}

/* Create a new projectile for the given type id with supplied attributes */
void
projectilelist_add_id(uint id, i32 life, vec3 pos, vec3 rot, vec3 dir, float grav, Entity *origin)
{
    ProjectileData * const list = atra_list_ProjectileData_getptr(&proj_db, id);

    projectilelist_add_ptr(list, life, pos, rot, dir, grav, origin);
}

/* Initialises and enables the given event type for a projectile type */
void
projectilelist_event_enable(ProjectileData *list, uint event_type)
{
    if (list->event_flags[event_type] != 0) {
        return;
    }
    ProjectileEvent *e = &list->events[event_type];
    list->event_flags[event_type] = 1;

    e->prt_ids = atra_list_uint_create(PROJECTILE_EVENT_LIST_SIZE);
    e->prt_counts = atra_list_uint_create(PROJECTILE_EVENT_LIST_SIZE);
    e->prt_emittype = atra_list_uint_create(PROJECTILE_EVENT_LIST_SIZE);
    e->prt_spread = atra_list_float_create(PROJECTILE_EVENT_LIST_SIZE);
    e->prj_ids = atra_list_uint_create(PROJECTILE_EVENT_LIST_SIZE);
    e->prj_counts = atra_list_uint_create(PROJECTILE_EVENT_LIST_SIZE);
    e->prj_emittype = atra_list_uint_create(PROJECTILE_EVENT_LIST_SIZE);
    e->prj_spread = atra_list_float_create(PROJECTILE_EVENT_LIST_SIZE);
}

/* Adds a new projectile to an event in a projectile list */
void
projectilelist_event_addparticle(ProjectileData *list, uint event_type,
        uint prt_id, uint prt_count, uint prt_emittype, uint prt_spread)
{
    if (list->event_flags[event_type] == 0) {
        projectilelist_event_enable(list, event_type);
    }
    atra_list_uint_add(&list->events[event_type].prt_ids, prt_id);
    atra_list_uint_add(&list->events[event_type].prt_counts, prt_count);
    atra_list_uint_add(&list->events[event_type].prt_emittype, prt_emittype);
    atra_list_float_add(&list->events[event_type].prt_spread, prt_spread);
}

/* Adds a new projectile to an event in a projectile list */
void
projectilelist_event_addprojectile(ProjectileData *list, uint event_type,
        uint prj_id, uint prj_count, uint prj_emittype, uint prj_spread)
{
    if (list->event_flags[event_type] == 0) {
        projectilelist_event_enable(list, event_type);
    }
    atra_list_uint_add(&list->events[event_type].prj_ids, prj_id);
    atra_list_uint_add(&list->events[event_type].prj_counts, prj_count);
    atra_list_uint_add(&list->events[event_type].prj_emittype, prj_emittype);
    atra_list_float_add(&list->events[event_type].prj_spread, prj_spread);
}

/* Sets the sounds of the given event in a projectile list */
void
projectilelist_event_setsound(ProjectileData *list, uint event_type, uint snd_id)
{
    if (list->event_flags[event_type] == 0) {
        projectilelist_event_enable(list, event_type);
    }
    list->events[event_type].sound_id = snd_id;
}

/* Runs an event type for the given projectile data list */
void
projectilelist_event_run(ProjectileData *list, Projectile *p, uint event_type)
{
    ProjectileEvent * const e = &list->events[event_type];
    AudioBuffer * const snd_buf = atra_audio_buffer_get(e->sound_id);
    const size_t prt_list_size = e->prt_ids.size;
    const size_t prj_list_size = e->prj_ids.size;

    ParticleList *prt_list;
    ProjectileData *prj_list;
    vec2 dir_2d;
    vec3 pos, dir;
    uint count, emit_type;
    float spread;
    uint i, j;

    atra_audio_source_attachbuffer(&p->snd_src, snd_buf);

    for (i = 0; i < prt_list_size; i++) {
        prt_list = db_particle_getlist(e->prt_ids.data[i]);
        count = e->prt_counts.data[i];
        emit_type = e->prt_emittype.data[i];
        spread = e->prt_spread.data[i];
       
        switch (emit_type) {
        case PROJECTILE_EMIT_STATIC:
            glm_vec3_copy(p->dir, dir);
            break;
        case PROJECTILE_EMIT_RADIAL:
            rand_dir_vec2_circle(dir_2d);
            dir[0] = dir_2d[0];
            dir[1] = 0.0f;
            dir[2] = dir_2d[1];
            break;
        case PROJECTILE_EMIT_FAN:
            rand_dir_vec2_fan(p->dir, spread, dir_2d);
            dir[0] = dir_2d[0];
            dir[1] = 0.0f;
            dir[2] = dir_2d[1];
            break;
        case PROJECTILE_EMIT_SPHERE:
            rand_dir_vec3_sphere(dir);
            break;
        case PROJECTILE_EMIT_CONE:
            rand_dir_vec3_cone(p->dir, spread, dir);
            break;
        }
        rand_getuniformpoint_sphere(p->pos, list->radius, pos);

        for (j = 0; j < count; j++) {
            particlelist_add(prt_list, pos, p->rot, dir, prt_list->lifetime, 0);
        }
    }

    for (i = 0; i < prj_list_size; i++) {
        prj_list = projectile_getlist(e->prj_ids.data[i]);
        count = e->prj_counts.data[i];
        emit_type = e->prj_emittype.data[i];
        spread = e->prj_spread.data[i];
        
        switch (emit_type) {
        case PROJECTILE_EMIT_STATIC:
            glm_vec3_copy(p->dir, dir);
            break;
        case PROJECTILE_EMIT_RADIAL:
            rand_dir_vec2_circle(dir_2d);
            dir[0] = dir_2d[0];
            dir[1] = 0.0f;
            dir[2] = dir_2d[1];
            break;
        case PROJECTILE_EMIT_FAN:
            rand_dir_vec2_fan(p->dir, spread, dir_2d);
            dir[0] = dir_2d[0];
            dir[1] = 0.0f;
            dir[2] = dir_2d[1];
            break;
        case PROJECTILE_EMIT_SPHERE:
            rand_dir_vec3_sphere(dir);
            break;
        case PROJECTILE_EMIT_CONE:
            rand_dir_vec3_cone(p->dir, spread, dir);
            break;
        }

        for (j = 0; j < count; j++) {
            projectilelist_add_ptr(prj_list, prj_list->max_life, p->pos, p->rot, dir, 0, p->origin);
        }
    }

}

/* Calculates the collisions for each particle in the list */
void
projectilelist_calccollisions(ProjectileData *list)
{
    const uint ent_list_size = db_entity_size();

    World * const world = state_world_get();
    Projectile *p;
    uint i, j;

    if ((BIT_CHK(list->attrs, PROJECTILE_ATTR_DIEONCOLLIDE) == 0 &&
            BIT_CHK(list->attrs, PROJECTILE_ATTR_BOUNCING) == 0) ||
            BIT_CHK(list->attrs, PROJECTILE_ATTR_NOCOLLIDE) == 1) {
        return;
    }

    for (i = 0; i < list->projs.size; i++) {
        p = &list->projs.data[i];
        projectile_calcworldcollisions(list, p, world);

        for (j = 0; j < ent_list_size; j++) {
            projectile_calcentitycollisions(list, p, db_entity_get(j));
        }
        /* TODO: Decoration collisions */
    }
}

static void
projectile_calcworldcollisions(ProjectileData *list, Projectile *p, World *world)
{
    int i, j;
    uint chunk_x, chunk_z;
    Chunk *chunk;
    ivec3 pos;

    /* Tile collisions */
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            pos[0] = p->pos[0] + i;
            pos[1] = p->pos[1];
            pos[2] = p->pos[2] + j;

            chunk_x = pos[0] / TILE_CHUNK_SIZE;
            chunk_z = pos[2] / TILE_CHUNK_SIZE;

            chunk = world_getchunk(world, chunk_x, chunk_z);

            if (chunk != NULL) {
                projectile_calctilecollisions(list, p, chunk->tile_chunk, pos);
            }
        }
    }
    chunk_x = p->pos[0] / TILE_CHUNK_SIZE;
    chunk_z = p->pos[2] / TILE_CHUNK_SIZE;

    chunk = world_getchunk(world, chunk_x, chunk_z);

    if (chunk != NULL) {
        projectile_calcterraincollisions(list, p, chunk->terrain_chunk);
    }
}

static void
projectile_calcterraincollisions(ProjectileData *list, Projectile *p, Terrain *chunk)
{
    vec2 pos = {p->pos[0], p->pos[2]};
    float h;

    h = terrain_getheight(chunk, pos);
    
    if (p->pos[1] - list->radius <= h) {

        if (BIT_CHK(list->attrs, PROJECTILE_ATTR_DIEONCOLLIDE)) {
            projectilelist_dieptr(list, p, PROJECTILE_HIT_TERRAIN, chunk);
        } else if (BIT_CHK(list->attrs, PROJECTILE_ATTR_BOUNCING)) {
            p->grav = -p->grav;
        }
    }
}

static void
projectile_calctilecollisions(ProjectileData *list, Projectile *p, TileChunk *chunk, ivec3 pos)
{   
    const int x_mod = (int) pos[0] % TILE_CHUNK_SIZE;
    const int z_mod = (int) pos[2] % TILE_CHUNK_SIZE;

    const int ch_pos_x = (x_mod < 0) ? TILE_CHUNK_SIZE + x_mod : x_mod;
    const int ch_pos_z = (z_mod < 0) ? TILE_CHUNK_SIZE + z_mod : z_mod;

    if (chunk == NULL) {
        return;
    }

    if (chunk->tiles[ch_pos_x + ch_pos_z * TILE_CHUNK_SIZE] == 0) {
        return;
    }

    const u32 flags = tile_chunk_getflags(chunk, ch_pos_x, ch_pos_z);
    const int box_y = DECIDE(BIT_CHK(flags, TILE_FLAG_COLLIDABLE), 2.0f, 0.0f);
    const Sphere sphere = {
        .pos = {p->pos[0], p->pos[1], p->pos[2]},
        .r = list->radius * p->scale
    };
    const AABB box = {
        .min = {pos[0] + 0.0f, -2.0f, pos[2] + 0.0f},
        .max = {pos[0] + 1.0f, box_y, pos[2] + 1.0f}
    };

    vec3 res_b, res_s, dir_b;

    if (BIT_CHK(list->attrs, PROJECTILE_ATTR_DIEONCOLLIDE)) {
        if (atra_aabb_sphere_collision(box, sphere) == 0) {
            return;
        }
        projectilelist_event_run(list, p, PROJECTILE_EVENT_DEATH);
        projectilelist_dieptr(list, p, PROJECTILE_HIT_TILE, chunk);
    } else if (BIT_CHK(list->attrs, PROJECTILE_ATTR_BOUNCING)) {
        if (atra_aabb_sphere_reflect(box, sphere,
                res_b, res_s, dir_b, p->dir, 1, 0) == 0) {
            return;
        }
        glm_vec3_add(p->pos, res_s, p->pos);
        projectilelist_event_run(list, p, PROJECTILE_EVENT_BOUNCE);
    }
}

static void
projectile_calcentitycollisions(ProjectileData *list, Projectile *p, EntityList *ents)
{
    const Sphere sphere = {
        .pos = {p->pos[0], p->pos[1], p->pos[2]},
        .r = list->radius * p->scale
    };

    uint i;
    Entity *ent;
    vec3 res_c, res_s, dir_c;

    for (i = 0; i < ents->size; i++) {
        ent = &ents->ents[i];
    
        if (BIT_CHK(list->attrs, PROJECTILE_ATTR_DIEONCOLLIDE)) {
            if (atra_sphere_cylinder_resolve(sphere, ents->hitbox, res_s, res_c, 0, 0) == 0) {
                return;
            }
            glm_vec3_normalize(res_c);
            glm_vec3_muladds(res_c, p->force, ent->force);
            projectilelist_event_run(list, p, PROJECTILE_EVENT_DEATH);
            projectilelist_dieptr(list, p, PROJECTILE_HIT_ENTITY, ent);
        } else if (BIT_CHK(list->attrs, PROJECTILE_ATTR_BOUNCING)) {
            if (atra_sphere_cylinder_reflect(sphere, ents->hitbox, res_s, res_c, p->dir, dir_c, 0, 0) == 0) {
                return;
            }
            glm_vec3_normalize(res_c);
            glm_vec3_muladds(res_c, p->force, ent->force);
            glm_vec3_add(p->pos, res_s, p->pos);
            projectilelist_event_run(list, p, PROJECTILE_EVENT_BOUNCE);
        }
    }
}

/* Advances the physics simulation for each particle in the list */
void
projectilelist_updatephysics(ProjectileData *list, u32 dt)
{
    const size_t p_count = list->projs.size;

    Projectile *p;
    uint i;

    for (i = 0; i < p_count; i++) {
        p = &list->projs.data[i];
        projectile_updatephysics(list, p, dt);
    }
}

static void
projectile_updatephysics(ProjectileData *list, Projectile *p, u32 dt)
{
    float dt_secs = dt / 1000.0f;

    p->grav += list->gravity * dt_secs;
    glm_vec3_muladds(p->dir, p->force * dt_secs, p->pos);
    
}

/* Calls for a specific projectile to die. */
void
projectilelist_die(ProjectileData *list, uint id, uint hit_type, void *hit)
{
    Projectile * const p = &list->projs.data[id];

    projectilelist_dieptr(list, p, hit_type, hit);
}

/* Calls for a specific projectile to die */
void
projectilelist_dieptr(ProjectileData *list, Projectile *p, uint hit_type, void *hit)
{
    Entity *ent = NULL;
    Terrain *terrachunk = NULL;
    TileChunk *tilechunk = NULL;

    if (hit_type == PROJECTILE_HIT_ENTITY) {
        ent = hit;
        entity_damage(ent, p->damage);
    } else if (hit_type == PROJECTILE_HIT_TERRAIN) {
        terrachunk = hit;
        /* TODO: Do damage to terrain */
    } else if (hit_type == PROJECTILE_HIT_TILE) {
        tilechunk = hit;
        /* TODO: Do tile damage (none at all at the moment) */
    }
}

/* Updates each particle in the given data list */
void
projectilelist_update(ProjectileData *list, u32 dt)
{
    /*
     *  Update lifetime and lifetime-related death
     *  Update Physics
     *  Check collisions
     *  Update collision calculations
     */
    

    projectilelist_updatephysics(list, dt);
    projectilelist_calccollisions(list);
}

/* Draw all projectiles in the given projectile data list */
void
projectilelist_draw(ProjectileData *list, int draw_mode)
{

}

