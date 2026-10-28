/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  entity.c - Common intersection for world-space entities which interact
 *             with the world, and each other.
 *
 */

#include "entity.h"
#include "state_game.h"
#include "world.h" 
#include "file.h"
#include "math_tools.h"

ATRA_LIST_DEFINE(Entity)

static EntityDB ent_db;
static Shader ent_shader_depth;
static Shader ent_shader_fwd;

static FILE_READ_FUNC_DECLARE(Cylinder)
static FILE_READ_ARRAY_FUNC_DECLARE(DamageEvent)
static FILE_READ_ARRAY_FUNC_DECLARE(SoundEvent)

static FILE_WRITE_FUNC_DECLARE(Cylinder)
static FILE_WRITE_ARRAY_FUNC_DECLARE(DamageEvent)
static FILE_WRITE_ARRAY_FUNC_DECLARE(SoundEvent)

static void entity_updaterotation(Entity *ent, float dt_secs);
static void entity_updateanimation(EntityList *list, Entity *ent, uint index, float dt_secs);

static Model entitymodel_loadfile(const char *path);
static void entitymodel_uploadmesh(Mesh *mesh);
static void entitymodel_loadtextures(Model *model, const char *albedo_path,
        const char *spec_path, const char *norm_path, const char *occlusion_path,
        const char *emission_path);


/* Initialise the entity database */
void
db_entity_init(void)
{
    ent_db.size = 0;
    ent_db.cap = ENTITY_DB_DEF_CAP;
    ent_db.ent_lists = malloc(ent_db.cap * sizeof(EntityList));
    ent_db.ent_paths = atra_hashmap_create(sizeof(char *), sizeof(uint), atra_hashmap_str_hash, atra_hashmap_str_eq);

    #define SETUNILOC_DEPTH(loc, name) \
            ent_shader_depth.locs[loc] = atra_shader_getuniformloc(&ent_shader_depth, name); 
    #define SETUNILOC_FWD(loc, name) \
            ent_shader_fwd.locs[loc] = atra_shader_getuniformloc(&ent_shader_fwd, name); 

    /* Set up the depth shader */
    ent_shader_depth = atra_shader_loadfile(ENTITY_PATH_DEPTH_VS, ENTITY_PATH_DEPTH_FS);
    
    SETUNILOC_DEPTH(ENTITY_LOC_BONEMATS, ENTITY_UNIFORM_NAME_BONEMATS);
    SETUNILOC_DEPTH(ENTITY_LOC_MODELMAT, ENTITY_UNIFORM_NAME_MODELMAT);
    SETUNILOC_DEPTH(ENTITY_LOC_LIGHTSPACE, ENTITY_UNIFORM_NAME_LIGHTSPACE);

    /* Set up the forward shader */
    ent_shader_fwd = atra_shader_loadfile(ENTITY_PATH_FWD_VS, ENTITY_PATH_FWD_FS);

    SETUNILOC_FWD(ENTITY_LOC_BONEMATS, ENTITY_UNIFORM_NAME_BONEMATS);
    SETUNILOC_FWD(ENTITY_LOC_VIEWPOS, ENTITY_UNIFORM_NAME_VIEWPOS);
    SETUNILOC_FWD(ENTITY_LOC_AMBIENTCOLOR, ENTITY_UNIFORM_NAME_AMBIENTCOLOR);
    SETUNILOC_FWD(ENTITY_LOC_SUNCOLOR, ENTITY_UNIFORM_NAME_SUNCOLOR);
    SETUNILOC_FWD(ENTITY_LOC_SUNDIR, ENTITY_UNIFORM_NAME_SUNDIR);
    SETUNILOC_FWD(ENTITY_LOC_MODELMAT, ENTITY_UNIFORM_NAME_MODELMAT);
    SETUNILOC_FWD(ENTITY_LOC_VIEWPROJ, ENTITY_UNIFORM_NAME_VIEWPROJ);
    SETUNILOC_FWD(ENTITY_LOC_LIGHTSPACE, ENTITY_UNIFORM_NAME_LIGHTSPACE);
    SETUNILOC_FWD(ENTITY_LOC_TEX_ALBEDO, ENTITY_UNIFORM_NAME_TEX_ALBEDO);
    SETUNILOC_FWD(ENTITY_LOC_TEX_SPECULAR, ENTITY_UNIFORM_NAME_TEX_SPECULAR);
    SETUNILOC_FWD(ENTITY_LOC_TEX_NORMAL, ENTITY_UNIFORM_NAME_TEX_NORMAL);
    SETUNILOC_FWD(ENTITY_LOC_TEX_OCCLUSION, ENTITY_UNIFORM_NAME_TEX_OCCLUSION);
    SETUNILOC_FWD(ENTITY_LOC_TEX_EMISSION, ENTITY_UNIFORM_NAME_TEX_EMISSION);
    SETUNILOC_FWD(ENTITY_LOC_TEX_SHADOWMAP, ENTITY_UNIFORM_NAME_TEX_SHADOWMAP);
}

/* Loads a new entity type into the database.
 * Returns a pointer to the location of the entity list in the database. */
EntityList *
db_entity_loadfile(const char *path)
{
    return entitylist_loadfile(path);
}

/* Deletes the entity database */
void
db_entity_shutdown(void)
{
    uint i;
    EntityList *list;

    for (i = 0; i < ent_db.size; i++) {
        list = ent_db.ent_lists + i;
        entitylist_delete(list);
        //atra_rawanim_deletearray(list->anims, ENTITY_ANIM_TOTAL);
    }
    free(ent_db.ent_lists);
    atra_hashmap_delete(&ent_db.ent_paths, 0);
}

/* Removes a specific entity type from the database, given it's ID */
/*void
db_entity_remove(uint id)
{
    const int cpy_len = ent_db.size - id - 1;

    if (cpy_len <= 0) {
        return;
    }
    memmove(ent_db.ent_lists + id, ent_db.ent_lists + id + 1, cpy_len * sizeof(EntityList));
    ent_db.size--;
}*/

/* Adds a given entity list to the database. Returns a pointer to the entity list. */
EntityList *
db_entity_add(EntityList list)
{
    const uint index = ent_db.size;

    if (ent_db.size == ent_db.cap) {
        db_entity_grow();
    }
    list.id = index;;
    ent_db.ent_lists[index] = list;
    ent_db.size++;

    atra_hashmap_add(&ent_db.ent_paths, &list.file_path, &list.id);

    return &ent_db.ent_lists[index];
}

/* Clears the database of all entries */
void
db_entity_clear(void)
{
    uint i;

    for (i = 0; i < ent_db.size; i++) {
        entitylist_delete(&ent_db.ent_lists[i]);
    }
    ent_db.size = 0;
}

/* Returns a pointer to the entity list at index */
EntityList *
db_entity_get(uint id)
{
    return &ent_db.ent_lists[id];
}

/* Returns a pointer to the entity list with the given file path */
EntityList *
db_entity_get_bypath(const char *path)
{
    uint *id;

    id = atra_hashmap_get(&ent_db.ent_paths, &path);

    if (id == NULL) {
        return db_entity_loadfile(path);
    }

    return db_entity_get(*id);
}

/* Returns the index of the entity list with the given file path */
uint
db_entity_getid_bypath(const char *path)
{
    uint *id;

    id = atra_hashmap_get(&ent_db.ent_paths, &path);

    if (id == NULL) {
        return db_entity_loadfile(path)->id;
    }

    return *id; 
}

/* Doubles the maximum capacity of the entity database */
void
db_entity_grow(void)
{
    ent_db.cap *= 2;
    ent_db.ent_lists = realloc(ent_db.ent_lists, ent_db.cap * sizeof(EntityList));
}

/* Returns the number of entity lists in the database */
uint
db_entity_size(void)
{
    return ent_db.size;
}

/* Update all entities in the database (collisions, AI, etc.) */
void
db_entity_updateall(u32 dt)
{
    uint i;
    EntityList *list;

    for (i = 0; i < ent_db.size; i++) {
        list = &ent_db.ent_lists[i];
        entitylist_update(list, dt);
    }
    db_entity_calccollisions();
}

/* Calculates and rectifies collisions for all entities */
void
db_entity_calccollisions(void)
{
    uint i;
    EntityList *list;

    for (i = 0; i < ent_db.size; i++) {
        list = &ent_db.ent_lists[i];
        entitylist_clearcollisionflags(list);
    }

    for (i = 0; i < ent_db.size; i++) {
        list = &ent_db.ent_lists[i];
        entitylist_calccollisions(list);
    }
}

/* Draw all entities in the database to depth buffer */
void
db_entity_drawall_depth(void)
{
    DirLight * const sunlight = state_suncam_get();

    EntityList *list;
    mat4 lightspace;
    uint i;

    glm_mat4_mul(sunlight->proj, sunlight->view, lightspace);
    
    atra_shader_setactive(&ent_shader_depth);
    atra_shader_setuniform(ent_shader_depth.locs[ENTITY_LOC_LIGHTSPACE],
            lightspace, ATRA_SHADER_UNIFORM_MAT4, 1);

    for (i = 0; i < ent_db.size; i++) {
        list = ent_db.ent_lists + i;
        entitylist_render(list, ENTITY_DRAWMODE_DEPTH);
    }
}

/* Draw all entities in the database to the screen */
void
db_entity_drawall_forward(void)
{
    Camera * const cam = atra_camera_getmain();
    DirLight * const sunlight = state_suncam_get();
    const World * const world = state_world_get();
    
    const int * const locs = ent_shader_fwd.locs;
    const int sun_enabled = (world->sun_enabled != 0);

    const vec3 ambient_color = {
        world->ambient_light.r / 255.0f,
        world->ambient_light.g / 255.0f,
        world->ambient_light.b / 255.0f};

    const vec3 sun_color = {
        (world->sun_color.r / 255.0f) * sun_enabled,
        (world->sun_color.g / 255.0f) * sun_enabled,
        (world->sun_color.b / 255.0f) * sun_enabled};

    EntityList *list;
    mat4 viewproj, lightspace;
    int texunit_depth;
    uint i;

    texunit_depth = ENTITY_TEXUNIT_SHADOWMAP;

    glm_mat4_mul(cam->proj, cam->view, viewproj);
    glm_mat4_mul(sunlight->proj, sunlight->view, lightspace);

    atra_shader_setactive(&ent_shader_fwd);

    atra_dirlight_binddepthtex(sunlight, ENTITY_TEXUNIT_SHADOWMAP);

    atra_shader_setuniform(locs[ENTITY_LOC_VIEWPOS], cam->pos, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_AMBIENTCOLOR], ambient_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_SUNCOLOR], sun_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_SUNDIR], sunlight->dir, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_VIEWPROJ], viewproj, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_LIGHTSPACE], lightspace, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_TEX_SHADOWMAP], &texunit_depth, ATRA_SHADER_UNIFORM_INT, 1);

    for (i = 0; i < ent_db.size; i++) {
        list = ent_db.ent_lists + i;
        entitylist_render(list, ENTITY_DRAWMODE_FORWARD);
    }
}

/* Creates a new entity type from manual arguments.
 * Adds entity type to database, and returns entity type ID. */
EntityList *
entitylist_create(uint ai_type, uint flags, int max_health, const char *path,
        uint rendermode, Cylinder hitbox, const char *model_path,
        const char *albedo_path, const char *spec_path, const char *norm_path,
        const char *occlusion_path, const char *emission_path)
{
    EntityList list;
    int anim_count;

    list.cap = ENTITY_LIST_DEF_CAP;
    list.size = 0;
    list.ai_type = ai_type;
    list.flags = flags;
    list.max_health = max_health;
    list.rendermode = rendermode;
    list.hitbox = hitbox;
    list.file_path = atra_util_strdup(path);
    
    list.model = entitymodel_loadfile(model_path);
    list.anims = atra_rawanim_loadfile(model_path, &anim_count);
    list.model_path = atra_util_strdup(model_path);
    
    list.map_paths[ENTITY_TEXUNIT_ALBEDO] = atra_util_strdup(albedo_path);
    list.map_paths[ENTITY_TEXUNIT_SPECULAR] = atra_util_strdup(spec_path);
    list.map_paths[ENTITY_TEXUNIT_NORMAL] = atra_util_strdup(norm_path);
    list.map_paths[ENTITY_TEXUNIT_OCCLUSION] = atra_util_strdup(occlusion_path);
    list.map_paths[ENTITY_TEXUNIT_EMISSION] = atra_util_strdup(emission_path);

    entitymodel_loadtextures(&list.model, albedo_path, spec_path, norm_path, occlusion_path, emission_path);
    memset(list.anim_props, 0, ENTITY_ANIM_TOTAL * sizeof(EntityAnim));

    list.ents = malloc(list.cap * sizeof(Entity));
    list.modelmats = malloc(list.cap * 16 * sizeof(float));
    list.bone_mats = malloc(list.cap * list.model.bone_count * 16 * sizeof(float));
    list.collision_flags = malloc(list.cap * sizeof(int));

    return db_entity_add(list);
}

/* Load a new entity type from the given data file.
 * Registers the entity list in the database, and returns a pointer to it. */
EntityList *
entitylist_loadfile(const char *path)
{
    EntityList list;
    FileData file;
    int anim_count;
    uint i;

    EntityAnim *anim;

    file = file_load(path);

    list.size = 0;
    list.cap = ENTITY_LIST_DEF_CAP;

    list.ai_type = file_read_uint(&file);
    list.flags = file_read_uint(&file);
    list.max_health = file_read_int(&file);
    list.rendermode = file_read_uint(&file);
    list.hitbox = file_read_Cylinder(&file);
    list.file_path = atra_util_strdup(path);

    list.model_path = file_read_string(&file);
    list.model = entitymodel_loadfile(list.model_path);
    list.anims = atra_rawanim_loadfile(list.model_path, &anim_count);

    list.map_paths[ENTITY_TEXUNIT_ALBEDO] = file_read_string(&file);
    list.map_paths[ENTITY_TEXUNIT_SPECULAR] = file_read_string(&file);
    list.map_paths[ENTITY_TEXUNIT_NORMAL] = file_read_string(&file);
    list.map_paths[ENTITY_TEXUNIT_OCCLUSION] = file_read_string(&file);
    list.map_paths[ENTITY_TEXUNIT_EMISSION] = file_read_string(&file);

    entitymodel_loadtextures(&list.model, list.map_paths[0], list.map_paths[1],
            list.map_paths[2], list.map_paths[3], list.map_paths[4]);
    
    list.ents = malloc(list.cap * sizeof(Entity));
    list.modelmats = malloc(list.cap * 16 * sizeof(float));
    list.bone_mats = malloc(list.cap * list.model.bone_count * 16 * sizeof(float));
    list.collision_flags = malloc(list.cap * sizeof(int));

    if (anim_count != ENTITY_ANIM_TOTAL) {
        atra_log_print(LOG_WARN, "entity: Error loading entity: Incorrect animation count");
        memset(list.anim_props, 0, ENTITY_ANIM_TOTAL * sizeof(EntityAnim));
        file_delete(file);
        return NULL;
    }
    
    for (i = 0; i < ENTITY_ANIM_TOTAL; i++) {
        anim = list.anim_props + i;
        anim->anim_flags = file_read_i32(&file);
        anim->anim_dmg_count = file_read_uint(&file);
        anim->anim_snd_count = file_read_uint(&file);
        anim->dmg_events = malloc(anim->anim_dmg_count * sizeof(DamageEvent));
        anim->snd_events = malloc(anim->anim_snd_count * sizeof(SoundEvent));
        
        file_read_DamageEvent_array(&file, anim->anim_dmg_count, anim->dmg_events);
        file_read_SoundEvent_array(&file, anim->anim_snd_count, anim->snd_events);
    }
    file_delete(file);
    
    return db_entity_add(list);
}

/* Save an entity type to the given file path */
void
entitylist_savefile(EntityList *list, const char *path)
{
    FileData file = file_create(0);
    EntityAnim *anim;
    uint i;
    
    file_write_uint(&file, list->ai_type);
    file_write_uint(&file, list->flags);
    file_write_int(&file, list->max_health);
    file_write_uint(&file, list->rendermode);
    file_write_Cylinder(&file, list->hitbox);

    file_write_string(&file, list->model_path);
    file_write_string(&file, list->map_paths[ENTITY_TEXUNIT_ALBEDO]);
    file_write_string(&file, list->map_paths[ENTITY_TEXUNIT_SPECULAR]);
    file_write_string(&file, list->map_paths[ENTITY_TEXUNIT_NORMAL]);
    file_write_string(&file, list->map_paths[ENTITY_TEXUNIT_OCCLUSION]);
    file_write_string(&file, list->map_paths[ENTITY_TEXUNIT_EMISSION]);

    for (i = 0; i < ENTITY_ANIM_TOTAL; i++) {
        anim = list->anim_props + i;
        file_write_i32(&file, anim->anim_flags);
        file_write_uint(&file, anim->anim_dmg_count);
        file_write_uint(&file, anim->anim_snd_count);
        file_write_DamageEvent_array(&file, anim->anim_dmg_count, anim->dmg_events);
        file_write_SoundEvent_array(&file, anim->anim_snd_count, anim->snd_events);
    }
    file_save(&file, path);
    file_delete(file);
}

/* Deletes an entity list */
void
entitylist_delete(EntityList *list)
{
    uint i;

    atra_model_delete(&list->model);

    if (list->rendermode == ENTITY_RENDER_DYNAMIC) {
        atra_rawanim_deletearray(list->anims, ENTITY_ANIM_TOTAL);
    }

    for (i = 0; i < 5; i++) {
        free(list->map_paths[i]);
    }

    for (i = 0; i < ENTITY_ANIM_TOTAL; i++) {
        free(list->anim_props[i].dmg_events);
        free(list->anim_props[i].snd_events);
    }
    free(list->file_path);
    free(list->model_path);
    free(list->ents);
    free(list->modelmats);
    free(list->bone_mats);
    free(list->collision_flags);
}

/* Pushes a new entity to the given entity list.
 * Returns a pointer to the entity in the entity list */
Entity *
entitylist_push(EntityList *list, const Entity *ent)
{
    Entity *result;

    if (list->size >= list->cap) {
        entitylist_grow(list);
    }
    list->ents[list->size] = *ent;
    result = &list->ents[list->size];
    list->size++;
    
    result->id = list->size;
    result->ent_type = list->id;

    return result;
}

/* Remove entity at given index from the entity list */
void
entitylist_remove(EntityList *list, uint id)
{
    const uint length = list->size - id - 1;
    const uint bone_count = list->model.bone_count;

    Entity * const ent = list->ents + id;
    float * const model = list->modelmats + id * 16;
    float * const bones = list->bone_mats + id * bone_count * 16;
    int * const collide_flag = list->collision_flags + id;

    memmove(ent, ent + 1, length * sizeof(Entity));
    memmove(model, model + 16, length * 16 * sizeof(float));
    memmove(bones, bones + 16 * bone_count, length * bone_count * 16 * sizeof(float));
    memmove(collide_flag, collide_flag + 1, length * sizeof(int));

    list->size--;
}

/* Doubles the maximum capacity of an entity list */
void
entitylist_grow(EntityList *list)
{
    const uint bone_count = list->model.bone_count;

    list->cap *= 2;
    list->ents = realloc(list->ents, list->cap * sizeof(Entity));
    list->modelmats = realloc(list->modelmats, list->cap * 16 * sizeof(float));
    list->bone_mats = realloc(list->bone_mats, list->cap * bone_count * 16 * sizeof(float));
    list->collision_flags = realloc(list->collision_flags, list->cap * sizeof(int));
}

/* Sets the entity animation properties for the specified animation */
void
entitylist_setanimprop(EntityList *list, uint anim_id, EntityAnim props)
{
    list->anim_props[anim_id] = props;
}

/* Clears entity collision completion flags */
void
entitylist_clearcollisionflags(EntityList *list)
{
    memset(list->collision_flags, 0, list->size * sizeof(int));
}

/* Check for and resolve a collision between an entity and a tile.
 * Returns true if collision occurred */
static int
entity_tile_collide(World *world, Entity *ent, int x, int z, float radius)
{
    const vec2 a_pos = {ent->pos[0], -ent->pos[2]};
    const ivec2 b_pos = {ent->pos[0] + x, -ent->pos[2] + z};

    const int x_mod = b_pos[0] % TILE_CHUNK_SIZE;
    const int z_mod = b_pos[1] % TILE_CHUNK_SIZE;

    const int ch_pos_x = (x_mod < 0) ? TILE_CHUNK_SIZE + x_mod : x_mod;
    const int ch_pos_z = (z_mod < 0) ? TILE_CHUNK_SIZE + z_mod : z_mod;

    vec2 res_a, res_b;
    Chunk *chunk = world_getchunk(world,
            b_pos[0] / TILE_CHUNK_SIZE,
            b_pos[1] / TILE_CHUNK_SIZE);

    if (chunk == NULL) {
        return 0;
    }

    if (chunk->tile_chunk->tiles[ch_pos_x + ch_pos_z * TILE_CHUNK_SIZE] == 0) {
        return 0;
    }
    const u32 flags = tile_chunk_getflags(chunk->tile_chunk, ch_pos_x, ch_pos_z);

    if (BIT_CHK(flags, TILE_FLAG_COLLIDABLE)) {
        const Circle c_a = {{a_pos[0], a_pos[1]}, radius};
        const AABQ quad = {
            {b_pos[0] + 0.0f, b_pos[1] + 0.0f},
            {b_pos[0] + 1.0f, b_pos[1] + 1.0f}};

        const int collides = atra_aabq_circle_resolve(quad, c_a, res_b, res_a, 1, 0);

        if (collides) {
            ent->pos[0] -= res_a[0];
            ent->pos[2] += res_a[1];

            atra_log_print(LOG_INFO, "Found collision (%d, %d)", x, z);
            return 1;
        }
    }

    return 0;
}

/* Calculate world collisions for the given entity */
static void
entity_tile_checkcollisions(World *world, Entity *ent, float radius)
{
	int halt_flags[8] = {FALSE};
	int x, z, r;
    int r_range;

	r_range = ceilf(radius);
	r_range = CLAMP(r_range, 1, ENTITY_MAX_RADIUS);
	if (r_range > ENTITY_MAX_RADIUS) {
		atra_log_print(LOG_WARN, "entity: Hitbox range too large: Range must be <= %d", ENTITY_MAX_RADIUS);
		r_range = ENTITY_MAX_RADIUS;
	}

	entity_tile_collide(world, ent, 0, 0, radius);

	for (r = 1; r <= r_range; r++) {
		for (z = 0, x = r; z < r; z++) {
			if (halt_flags[4] && halt_flags[5] && halt_flags[6] && halt_flags[7]) {
				break;
			}

			/* Left-lower octant */
			if (halt_flags[4] == FALSE) {
				halt_flags[4] = entity_tile_collide(world, ent, -x, -z, radius);
				halt_flags[5] = (z == 0 && halt_flags[4]) ? TRUE : halt_flags[5];
			}

			/* Left-upper octant */
			if (halt_flags[5] == FALSE) {
				halt_flags[5] = entity_tile_collide(world, ent, -x, z, radius);
			}

			/* Right-lower octant */
			if (halt_flags[6] == FALSE) {
				halt_flags[6] = entity_tile_collide(world, ent, x, -z, radius);
				halt_flags[7] = (z == 0 && halt_flags[6]) ? TRUE : halt_flags[7];
			}

			/* Right-upper octant */
			if (halt_flags[7] == FALSE) {
				halt_flags[7] = entity_tile_collide(world, ent, x, z, radius);
			}
		}

		for (x = 0, z = r; x <= r; x++) {
			if (halt_flags[0] && halt_flags[1] && halt_flags[2] && halt_flags[3]) {
				break;
			}

			/* Top-left octant */
			if (halt_flags[0] == FALSE && (x != r || halt_flags[5] == FALSE)) {
				halt_flags[0] = entity_tile_collide(world, ent, -x, z, radius);
				halt_flags[1] = (x == 0 && halt_flags[0]) ? TRUE : halt_flags[1];
			}

			/* Top-right octant */
			if (halt_flags[1] == FALSE && (x != r || halt_flags[7] == FALSE)) {
				halt_flags[1] = entity_tile_collide(world, ent, x, z, radius);
			}

			/* Bottom-left octant */
			if (halt_flags[2] == FALSE && (x != r || halt_flags[4] == FALSE)) {
				halt_flags[2] = entity_tile_collide(world, ent, -x, -z, radius);
				halt_flags[3] = (x == 0 && halt_flags[2]) ? TRUE : halt_flags[3];
			}

			/* Bottom-right octant */
			if (halt_flags[3] == FALSE && (x != r || halt_flags[6] == FALSE)) {
				halt_flags[3] = entity_tile_collide(world, ent, x, -z, radius);
			}
		}
	}
}

/* Calculates the entity's y-coordinate based on terrain height and tiles */
static void
entity_calc_y_pos(World *world, EntityList *list, Entity *ent, int ent_index)
{
	const vec2 a_pos = {ent->pos[0], -ent->pos[2]};
	vec2 local_fpos = {
		a_pos[0] - (int)(a_pos[0] / TILE_CHUNK_SIZE) * TILE_CHUNK_SIZE,
		a_pos[1] - (int)(a_pos[1] / TILE_CHUNK_SIZE) * TILE_CHUNK_SIZE
	};
	local_fpos[0] = (local_fpos[0] < 0) ? TILE_CHUNK_SIZE + local_fpos[0] : local_fpos[0];
	local_fpos[1] = (local_fpos[1] < 0) ? TILE_CHUNK_SIZE + local_fpos[1] : local_fpos[1];

	const ivec2 local_ipos = {local_fpos[0], local_fpos[1]};
	const ivec2 chunk_pos = {
		a_pos[0] / TILE_CHUNK_SIZE - (a_pos[0] < 0),
		a_pos[1] / TILE_CHUNK_SIZE - (a_pos[1] < 0)
	};

    Chunk *chunk;

	chunk = world_getchunk(world, chunk_pos[0], chunk_pos[1]);

	if (chunk != NULL) {
		if (chunk->tile_chunk->tiles[local_ipos[0] + local_ipos[1] * TILE_CHUNK_SIZE] == 0) {
			ent->pos[1] = terrain_getheight(chunk->terrain_chunk, local_fpos);
		} else {
			ent->pos[1] = 0.0f; 
		}
	}
	list->collision_flags[ent_index] = 1;
}

/* Calculates and resolves entity-entity collisions */
static void
entity_calcentitycollisions(EntityList *list_a, Entity *ent_a)
{
    EntityList *target_list;
	Entity *ent_b;

    vec2 res_a, res_b;
    float r_a, r_b;
	uint i, j;

    r_a = list_a->hitbox.r;

	for (i = 0; i < ent_db.size; i++) {
		target_list = &ent_db.ent_lists[i];

		r_b = target_list->hitbox.r;

		for (j = 0; j < target_list->size; j++) {
			ent_b = &target_list->ents[j];

			if ((ent_a == ent_b) || target_list->collision_flags[j] != 0) {
				continue;
			}
			const Circle c_a = {{ent_a->pos[0], -ent_a->pos[2]}, r_a};
			const Circle c_b = {{ent_b->pos[0], -ent_b->pos[2]}, r_b};

			const int collides = atra_circle_circle_resolve(c_a, c_b, res_a, res_b, 0, 0);

			if (collides) {
				ent_a->force[0] = res_a[0] * ENTITY_COLLIDE_FACTOR;
				ent_a->force[2] = res_a[1] * ENTITY_COLLIDE_FACTOR;
				ent_b->force[0] = res_b[0] * ENTITY_COLLIDE_FACTOR;
				ent_b->force[2] = res_b[1] * ENTITY_COLLIDE_FACTOR;
			}
		}
	}

}


/* Calculates collisions for all entities in list */
void
entitylist_calccollisions(EntityList *list)
{
    Entity *ent;
    World *world;
    float radius;
    uint i;

    radius = list->hitbox.r;
    world = state_world_get();

    for (i = 0; i < list->size; i++) {
        ent = &list->ents[i];

		entity_calcentitycollisions(list, ent);
		entity_tile_checkcollisions(world, ent, radius);
		entity_calc_y_pos(world, list, ent, i);
    }
}

static void
entity_updateposition(Entity *ent, float dt_secs)
{
	ent->pos[0] += ent->vel[0] * dt_secs * ENTITY_SPEED_FACTOR;
	ent->pos[1] += ent->vel[1] * dt_secs * ENTITY_SPEED_FACTOR;
	ent->pos[2] += ent->vel[2] * dt_secs * ENTITY_SPEED_FACTOR;

	ent->pos[0] += ent->force[0] * dt_secs;
	ent->pos[1] += ent->force[1] * dt_secs;
	ent->pos[2] += ent->force[2] * dt_secs;

	ent->vel[0] *= ENTITY_DECELERATION_FACTOR * dt_secs;
	ent->vel[1] *= ENTITY_DECELERATION_FACTOR * dt_secs;
	ent->vel[2] *= ENTITY_DECELERATION_FACTOR * dt_secs;

	ent->force[0] *= ENTITY_DECELERATION_FACTOR * dt_secs;
	ent->force[1] *= ENTITY_DECELERATION_FACTOR * dt_secs;
	ent->force[2] *= ENTITY_DECELERATION_FACTOR * dt_secs;
}

static void
entity_updaterotation(Entity *ent, float dt_secs)
{
    vec2 vel_2d = {ent->vel[0], ent->vel[2]};
    float vel_len2 = vel_2d[0] * vel_2d[0] + vel_2d[1] * vel_2d[1];
    float target_rot;

    if (vel_len2 > 0.0f) {
        glm_vec2_normalize(vel_2d);
        target_rot = atan2f(vel_2d[0], vel_2d[1]);

        if (fabsf(target_rot - ent->rot[1]) > PI) {
            target_rot += (target_rot < 0.0) ? PI * 2 : -PI * 2;
        }
        ent->rot[1] = lerpf(ent->rot[1], target_rot, ENTITY_TURN_SPEED * dt_secs);

        if (ent->rot[1] > PI) {
            ent->rot[1] -= PI * 2;
        } else if (ent->rot[1] < -PI) {
            ent->rot[1] += PI * 2;
        }
    }
}

static void
entity_updateanimation(EntityList *list, Entity *ent, uint index, float dt_secs)
{
    const uint bone_count = list->model.bone_count;
    ModelAnimRaw *anim;
    vec4 *bonemats;

    vec2 vel_2d = {ent->vel[0], ent->vel[2]};
    const float vel_len2 = vel_2d[0] * vel_2d[0] + vel_2d[1] * vel_2d[1];

    if (ent->health <= 0) {
        /* TODO: Die: Play death animation, remove collision, etc. */
    } else if (vel_len2 > 0.0f) {
        if (ent->anim != ENTITY_ANIM_RUN) {
            ent->anim = ENTITY_ANIM_RUN;
            ent->anim_time = 0.0f;
        }
    } else {
        if (ent->anim != ENTITY_ANIM_IDLE) {
            ent->anim = ENTITY_ANIM_IDLE;
            ent->anim_time = 0.0f;
        }
    }
    bonemats = (vec4 *) &list->bone_mats[16 * index * bone_count];
    anim = &list->anims[ent->anim];
    atra_rawanim_updatemodelext(anim, &list->model, ent->anim_time);
    memcpy(bonemats, list->model.bone_mats, bone_count * 16 * sizeof(float));
    ent->anim_time += dt_secs;
}

/* Updates each entity in the list */
void
entitylist_update(EntityList *list, u32 dt)
{
    const float dt_secs = dt / 1000.0f;

    Entity *ent;
    vec4 *modelmat;
    uint i;

    for (i = 0; i < list->size; i++) {
        ent = &list->ents[i];

        /* TODO: AI processing here */

		entity_updateposition(ent, dt_secs);
        entity_updaterotation(ent, dt_secs);
        entity_updateanimation(list, ent, i, dt_secs);

        modelmat = (vec4 *) &list->modelmats[16 * i];
        glm_mat4_identity(modelmat);
        glm_translate(modelmat, ent->pos);
        glm_rotate_y(modelmat, ent->rot[1], modelmat);
        glm_scale_uni(modelmat, ENTITY_MODEL_SCALE);
    }
}

/* Renders each entity in the list by the given rendering method */
void
entitylist_render(EntityList *list, int draw_mode)
{
    const uint bone_count = list->model.bone_count;
    const int *locs;
    const int texunits[] = {
        ENTITY_TEXUNIT_ALBEDO,
        ENTITY_TEXUNIT_SPECULAR,
        ENTITY_TEXUNIT_NORMAL,
        ENTITY_TEXUNIT_OCCLUSION,
        ENTITY_TEXUNIT_EMISSION,
    };

    uint i, j;
    Material *material;
    Mesh *mesh;
    float *modelmat;
    float *bonemats;

    /* Set up shader uniforms and textures */

    /* TODO: Remove auto GLTF material loading, supplant with manual texture loading */

    if (draw_mode == ENTITY_DRAWMODE_DEPTH) {
        locs = ent_shader_depth.locs;
    } else {
        locs = ent_shader_fwd.locs; 
        atra_shader_setuniform(locs[ENTITY_LOC_TEX_ALBEDO], &texunits[0], ATRA_SHADER_UNIFORM_INT, 1);
        atra_shader_setuniform(locs[ENTITY_LOC_TEX_SPECULAR], &texunits[1], ATRA_SHADER_UNIFORM_INT, 1);
        atra_shader_setuniform(locs[ENTITY_LOC_TEX_NORMAL], &texunits[2], ATRA_SHADER_UNIFORM_INT, 1);
        atra_shader_setuniform(locs[ENTITY_LOC_TEX_OCCLUSION], &texunits[3], ATRA_SHADER_UNIFORM_INT, 1);
        atra_shader_setuniform(locs[ENTITY_LOC_TEX_EMISSION], &texunits[4], ATRA_SHADER_UNIFORM_INT, 1);
    }
    
    for (i = 0; i < (uint) list->model.mesh_count; i++) {
        mesh = &list->model.meshes[i];
        //material = &list->model.materials[list->model.mesh_materials[i]];
        material = &list->model.materials[0];
       
        if (draw_mode == ENTITY_DRAWMODE_FORWARD) {
            /* TODO: Redo texture bind function to use proper unit offset */
            atra_texture_bind(&material->maps[ENTITY_TEXUNIT_ALBEDO].tex, GL_TEXTURE0 + ENTITY_TEXUNIT_ALBEDO);
            atra_texture_bind(&material->maps[ENTITY_TEXUNIT_SPECULAR].tex, GL_TEXTURE0 + ENTITY_TEXUNIT_SPECULAR);
            atra_texture_bind(&material->maps[ENTITY_TEXUNIT_NORMAL].tex, GL_TEXTURE0 + ENTITY_TEXUNIT_NORMAL);
            atra_texture_bind(&material->maps[ENTITY_TEXUNIT_OCCLUSION].tex, GL_TEXTURE0 + ENTITY_TEXUNIT_OCCLUSION);
            atra_texture_bind(&material->maps[ENTITY_TEXUNIT_EMISSION].tex, GL_TEXTURE0 + ENTITY_TEXUNIT_EMISSION);
        }
        atra_gl_vao_enable(mesh->vao_id);

        for (j = 0; j < list->size; j++) {
            modelmat = list->modelmats + j * 16;
            bonemats = list->bone_mats + bone_count * j * 16;
        
            atra_shader_setuniform(locs[ENTITY_LOC_MODELMAT], modelmat, ATRA_SHADER_UNIFORM_MAT4, 1);
            atra_shader_setuniform(locs[ENTITY_LOC_BONEMATS], bonemats, ATRA_SHADER_UNIFORM_MAT4, bone_count);

            atra_gl_vao_drawelements(0, mesh->tri_count * 3, NULL);
        }
    }
}

static Model
entitymodel_loadfile(const char *path)
{
    Model model = atra_model_loadfile(path, 0);
    int i;

    for (i = 0; i < model.mesh_count; i++) {
        entitymodel_uploadmesh(&model.meshes[i]);
    }

    return model;
}

static void
entitymodel_uploadmesh(Mesh *mesh)
{
    mesh->vbo_ids = calloc(ATRA_MAX_MESH_VERTEX_BUFFERS, sizeof(uint));
    mesh->vao_id = atra_gl_vao_create();
    atra_gl_vao_enable(mesh->vao_id);

    mesh->tangents = malloc(mesh->vert_count * 3 * sizeof(float));
    atra_mesh_calctangents(mesh);

    mesh->vbo_ids[ENTITY_VBO_POSITION] =
            atra_gl_vbo_create(mesh->anim_verts, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(ENTITY_VBO_POSITION, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(ENTITY_VBO_POSITION);

    mesh->vbo_ids[ENTITY_VBO_TEXCOORD] =
            atra_gl_vbo_create(mesh->uvs, mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vao_setattrib(ENTITY_VBO_TEXCOORD, 2, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(ENTITY_VBO_TEXCOORD);

    mesh->vbo_ids[ENTITY_VBO_NORMAL] =
            atra_gl_vbo_create(mesh->anim_norms, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(ENTITY_VBO_NORMAL, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(ENTITY_VBO_NORMAL);

    mesh->vbo_ids[ENTITY_VBO_COLOR] =
            atra_gl_vbo_create(mesh->colors, mesh->vert_count * 4 * sizeof(u8), 0);
    atra_gl_vao_setattrib(ENTITY_VBO_COLOR, 4, ATRA_UNSIGNED_BYTE, 1, 0, NULL);
    atra_gl_vao_enableattrib(ENTITY_VBO_COLOR);

    mesh->vbo_ids[ENTITY_VBO_TANGENT] =
            atra_gl_vbo_create(mesh->tangents, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(ENTITY_VBO_TANGENT, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(ENTITY_VBO_TANGENT);

    mesh->vbo_ids[ENTITY_VBO_TEXCOORD2] =
            atra_gl_vbo_create(mesh->uvs2, mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vao_setattrib(ENTITY_VBO_TEXCOORD2, 2, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(ENTITY_VBO_TEXCOORD2);

    mesh->vbo_ids[ENTITY_VBO_BONEIDS] =
            atra_gl_vbo_create(mesh->bone_ids, mesh->vert_count * 4 * sizeof(float), 0);
    atra_gl_vao_setattrib(ENTITY_VBO_BONEIDS, 4, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(ENTITY_VBO_BONEIDS);

    mesh->vbo_ids[ENTITY_VBO_BONEWEIGHTS] =
            atra_gl_vbo_create(mesh->bone_weights, mesh->vert_count * 4 * sizeof(float), 0);
    atra_gl_vao_setattrib(ENTITY_VBO_BONEWEIGHTS, 4, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(ENTITY_VBO_BONEWEIGHTS);

    mesh->vbo_ids[ENTITY_VBO_INDICES] =
            atra_gl_ebo_create(mesh->indices, mesh->tri_count * 3 * sizeof(uint), 0);
}

static void
entitymodel_loadtextures(Model *model, const char *albedo_path,
        const char *spec_path, const char *norm_path, const char *occlusion_path,
        const char *emission_path)
{
    const char *paths[] = {
        albedo_path,
        spec_path,
        norm_path,
        occlusion_path,
        emission_path
    };

    const Color norm_color = {0x7F, 0x7F, 0xFF, 0xFF};
    const Color white_color = {0xFF, 0xFF, 0xFF, 0xFF};
    const Color black_color = {0x00, 0x00, 0x00, 0xFF};

    MaterialMap *map;
    Option_Texture maybe_tex;
    Texture tex;
    Image img;
    uint i;

    for (i = 0; i < 5; i++) {
        map = &model->materials[0].maps[i];
        maybe_tex = atra_texture_loadfile(paths[i]);
        
        if (maybe_tex.none) {
            atra_log_print(LOG_ERROR, "entity: Unable to load texture");
            
            switch (i) {
            case ENTITY_TEXUNIT_ALBEDO:
            case ENTITY_TEXUNIT_SPECULAR:
            case ENTITY_TEXUNIT_OCCLUSION:
                img = atra_image_gencolor(white_color, 1, 1);
                break;
            case ENTITY_TEXUNIT_NORMAL:
                img = atra_image_gencolor(norm_color, 1, 1);
                break;
            case ENTITY_TEXUNIT_EMISSION:
                img = atra_image_gencolor(black_color, 1, 1);
                break;
            }
            tex = atra_texture_loadimage(&img);
        } else {
            tex = maybe_tex.some;
        }
        map->tex = tex;
        map->color = white_color;
        map->value = 1.0f;
    }
}

/* Create a new instance of an entity from the database.
 * Adds entity to the respective entity list */
Entity *
entity_create(uint ent_type, const vec3 pos, const vec3 rot)
{
    EntityList *list = db_entity_get(ent_type);
    Entity ent = {
        .ent_type = ent_type,
        .pos = {pos[0], pos[1], pos[2]},
        .rot = {rot[0], rot[1], rot[2]},
        .vel = {0},
        .force = {0},
        .health = list->max_health,
        .anim = ENTITY_ANIM_IDLE,
        .anim_time = 0.0f,
    };

    return entitylist_push(list, &ent);
}

/* Deals damage to the entity */
void entity_damage(Entity *ent, int damage)
{
    ent->health -= damage;

    if (ent->health <= 0) {
        /* TODO: Switch to death animation 
         *
         * NEED TO MAKE ANIMATION SWITCHING SYSTEM */
    }
}


/* Supplementary file functions */
static FILE_READ_FUNC_CREATE(Cylinder)
static FILE_READ_ARRAY_FUNC_CREATE(DamageEvent)

static FILE_WRITE_FUNC_CREATE(Cylinder)
static FILE_WRITE_ARRAY_FUNC_CREATE(DamageEvent)

static void
file_read_SoundEvent_array(FileData *file, size_t n, SoundEvent *data)
{
    SoundEvent *e;
    uint i;

    for (i = 0; i < n; i++) {
        e = data + i;
        e->start = file_read_float(file);
        e->buf_path = file_read_string(file);

        // TODO: Initialise the sound buffer pointer using the file path
    }
}

static void
file_write_SoundEvent_array(FileData *file, size_t n, const SoundEvent *data)
{
    const SoundEvent *e;
    uint i;

    for (i = 0; i < n; i++) {
        e = data + i;
        file_write_float(file, e->start);
        file_write_string(file, e->buf_path);
    }
}
