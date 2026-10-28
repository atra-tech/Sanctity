/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  entity.h - Common intersection for world-space entities which interact
 *             with the world, and each other.
 */

#ifndef ENTITY_H
#define ENTITY_H

#include "atra.h"

#define ENTITY_DB_DEF_CAP           8
#define ENTITY_LIST_DEF_CAP         16

#define ENTITY_COLLIDE_FACTOR       0.5f
#define ENTITY_DECELERATION_FACTOR  0.5f
#define ENTITY_SPEED_FACTOR         2.0f
#define ENTITY_TURN_SPEED           16.0f
#define ENTITY_MODEL_SCALE          0.7f
#define ENTITY_MAX_RADIUS           3       /* Collision may fail at larger values */
#define ENTITY_MAX_DIAMETER         (ENTITY_MAX_RADIUS * 2 + 1)

#define ENTITY_PATH_DEPTH_VS        "res/shader/entity_depth_vert.glsl"
#define ENTITY_PATH_DEPTH_FS        "res/shader/entity_depth_frag.glsl"

#define ENTITY_PATH_FWD_VS          "res/shader/entity_fwd_vert.glsl"
#define ENTITY_PATH_FWD_FS          "res/shader/entity_fwd_frag.glsl"

#define ENTITY_UNIFORM_NAME_BONEMATS        "bone_mats"
#define ENTITY_UNIFORM_NAME_VIEWPOS         "view_pos"
#define ENTITY_UNIFORM_NAME_AMBIENTCOLOR    "ambient_color"
#define ENTITY_UNIFORM_NAME_SUNCOLOR        "sun_color"
#define ENTITY_UNIFORM_NAME_SUNDIR          "sun_dir"
#define ENTITY_UNIFORM_NAME_MODELMAT        "modelmat"
#define ENTITY_UNIFORM_NAME_VIEWPROJ        "viewprojmat"
#define ENTITY_UNIFORM_NAME_LIGHTSPACE      "lightspace"
#define ENTITY_UNIFORM_NAME_TEX_ALBEDO      "albedo_map"
#define ENTITY_UNIFORM_NAME_TEX_SPECULAR    "specular_map"
#define ENTITY_UNIFORM_NAME_TEX_NORMAL      "normal_map"
#define ENTITY_UNIFORM_NAME_TEX_OCCLUSION   "occlusion_map"
#define ENTITY_UNIFORM_NAME_TEX_EMISSION    "emission_map"
#define ENTITY_UNIFORM_NAME_TEX_SHADOWMAP   "shadow_map"


enum entity_render_method {
    ENTITY_RENDER_STATIC,       /* Entity is not animated */
    ENTITY_RENDER_SPRITE,       /* Rendering is done by UV frames on CPU */
    ENTITY_RENDER_DYNAMIC       /* Rendering is skeletally animated on GPU */
};

enum entity_drawmode {
    ENTITY_DRAWMODE_FORWARD,
    ENTITY_DRAWMODE_DEPTH
};

enum entity_frame_event_type {
    ENTITY_FRAMEEVENT_AUDIO,
    ENTITY_FRAMEEVENT_DAMAGE
};

enum entity_texture_units {
    ENTITY_TEXUNIT_ALBEDO,
    ENTITY_TEXUNIT_SPECULAR,
    ENTITY_TEXUNIT_NORMAL,
    ENTITY_TEXUNIT_OCCLUSION,
    ENTITY_TEXUNIT_EMISSION,
    ENTITY_TEXUNIT_SHADOWMAP
};

enum entity_vbo_type {
    ENTITY_VBO_POSITION,        /* Vertex world-space position */
    ENTITY_VBO_TEXCOORD,        /* Vertex primary texture coords */
    ENTITY_VBO_NORMAL,          /* Vertex normals */
    ENTITY_VBO_COLOR,           /* Vertex colors */
    ENTITY_VBO_TANGENT,         /* Vertex tangents */
    ENTITY_VBO_TEXCOORD2,       /* Vertex secondary texture coords */
    ENTITY_VBO_BONEIDS,         /* Vertex associated bone IDs */
    ENTITY_VBO_BONEWEIGHTS,     /* Vertex bone influence weights */
    ENTITY_VBO_INDICES,
};

enum entity_location_type {
    ENTITY_LOC_POSITION,
    ENTITY_LOC_TEXCOORD,
    ENTITY_LOC_NORMAL,
    ENTITY_LOC_COLOR,
    ENTITY_LOC_TANGENT,
    ENTITY_LOC_TEXCOORD2,
    ENTITY_LOC_BONEIDS,
    ENTITY_LOC_BONEWEIGHTS,
    ENTITY_LOC_BONEMATS,
    ENTITY_LOC_VIEWPOS,
    ENTITY_LOC_AMBIENTCOLOR,
    ENTITY_LOC_SUNCOLOR,
    ENTITY_LOC_SUNDIR,
    ENTITY_LOC_COLORTINT,
    ENTITY_LOC_MODELMAT,
    ENTITY_LOC_VIEWPROJ,
    ENTITY_LOC_LIGHTSPACE,
    ENTITY_LOC_TEX_ALBEDO,
    ENTITY_LOC_TEX_SPECULAR,
    ENTITY_LOC_TEX_NORMAL,
    ENTITY_LOC_TEX_OCCLUSION,
    ENTITY_LOC_TEX_EMISSION,
    ENTITY_LOC_TEX_SHADOWMAP,
};

enum entity_anim_type {
    ENTITY_ANIM_TPOSE,          /* Default animation pose */
    ENTITY_ANIM_IDLE,           /* Standard idle animation */
    ENTITY_ANIM_RUN,            /* Plays when entity moves */
    ENTITY_ANIM_ATTACK_0,       /* Attack animation variant */
    ENTITY_ANIM_ATTACK_1,       /* Attack animation variant */
    ENTITY_ANIM_CAST_TARGET,    /* Spell casting animation variant */
    //ENTITY_ANIM_CAST_TRAP,      /* Spell casting animation variant */
    //ENTITY_ANIM_CAST_AREA,      /* Spell casting animation variant */
    //ENTITY_ANIM_CAST_BUFF,      /* Spell casting animation variant */
    ENTITY_ANIM_DIE,            /* Death animation variant */
    ENTITY_ANIM_DIE_IDLE,       /* Death idle animation */

    ENTITY_ANIM_TOTAL
};

/*
 *  Uniforms:
 *      Bone count
 * */

typedef struct Entity {
    uint ent_type;      /* Index of entity type in database */
    uint id;            /* Index of entity inside entity type's list */
    vec3 pos;           /* Position of entity */
    vec3 rot;           /* Rotation of entity */
    vec3 vel;           /* Current directional movement velocity */
    vec3 force;         /* Physics impulse force */
    int health;         /* Current health of entity */
    uint anim;          /* Current animation being played by entity */
    float anim_time;    /* Current animation timestamp of entity */
} Entity;

typedef struct SoundEvent {
    float start;            /* Start time of event */
    char *buf_path;         /* Path to sound file of audio buffer */
    AudioBuffer *buf;       /* Audio buffer for sound events */
} SoundEvent;

typedef struct DamageEvent {
    float start;            /* Start time of event */
    float end;              /* End time of event */
    AABB dmgbox;            /* Damage box for damage events */
    int damage;             /* Amount of damage done by the damage box */
} DamageEvent;

typedef struct EntityAnim {
    i32 anim_flags;
    uint anim_dmg_count;
    uint anim_snd_count;
    DamageEvent *dmg_events;
    SoundEvent *snd_events;
} EntityAnim;

typedef struct EntityList {
    size_t size;            /* Size of entity list */
    size_t cap;             /* List current max capacity */

    uint id;                /* Index in entity database */
    uint ai_type;           /* AI algorithm type used by entity */
    uint flags;             /* Property flags for entity type */
    int max_health;         /* Maximum amount of health for entity type (value < 0 -> indestructible) */
    char *file_path;        /* Path to file containing entity data */

    uint rendermode;        /* Rendering method for drawing the entities */
    Cylinder hitbox;        /* Hitbox for the entities */
    Model model;            /* Model data used by entities */
    char *model_path;       /* Model data file path */
    
    char *map_paths[5];     /* Paths of texture maps used by model */

    ModelAnimRaw *anims;                        /* Animation data array */
    EntityAnim anim_props[ENTITY_ANIM_TOTAL];   /* Supplementary animation data */

    Entity *ents;           /* Entity instances for the current type */
    float *modelmats;       /* Model matrix for each entity */
    float *bone_mats;       /* Transform matrix for bones of each model */

    int *collision_flags;   /* Set when entity collision checks for entity are done */
} EntityList;

/* TODO: Find a way to bind damage frames and sound triggers to animations */
/* TODO: Ensure only enough data is here to allow for entity AI */

/* Database containing all entity lists */
typedef struct EntityDB {
    uint size;              /* Number of entity types in database */
    uint cap;               /* Database current max capacity */
    EntityList *ent_lists;  /* List of instances of each entity type */
    HashMap ent_paths;      /* Hashmap of entity data file paths */
} EntityDB;

ATRA_LIST_DECLARE(Entity)

/* Initialise the entity database */
void db_entity_init(void);

/* Loads a new entity type into the database.
 * Returns a pointer to the location of the entity list in the database. */
EntityList * db_entity_loadfile(const char *path);

/* Deletes the entity database */
void db_entity_shutdown(void);

/* Adds a given entity list to the database.
 * Returns a pointer to the location of the entity list in the database. */
EntityList * db_entity_add(EntityList list);

/* Returns the number of entity lists in the database */
uint db_entity_size(void);

/* Removes a specific entity type from the database, given it's ID */
//void db_entity_remove(uint id);

/* Clears the database of all entries */
void db_entity_clear(void);

/* Returns a pointer to the entity list at index */
EntityList * db_entity_get(uint id);

/* Returns a pointer to the entity list with the given file path */
EntityList * db_entity_get_bypath(const char *path);

/* Returns the index of the entity list with the given file path */
uint db_entity_getid_bypath(const char *path);

/* Doubles the maximum capacity of the entity database */
void db_entity_grow(void);

/* Update all entities in the database (collisions, AI, etc.) */
void db_entity_updateall(u32 dt);

/* Calculates and rectifies collisions for all entities */
void db_entity_calccollisions(void);

/* Draw all entities in the database to depth buffer */
void db_entity_drawall_depth(void);

/* Draw all entities in the database to the screen */
void db_entity_drawall_forward(void);


/* Creates a new entity type from manual arguments.
 * Registers the entity list in the database, and returns a pointer to it. */
EntityList * entitylist_create(uint ai_type, uint flags, int max_health, const char *path,
        uint rendermode, Cylinder hitbox, const char *model_path, const char *albedo_path,
        const char *spec_path, const char *norm_path, const char *occlusion_path,
        const char *emission_path);

/* Load a new entity type from the given data file.
 * Registers the entity list in the database, and returns a pointer to it. */
EntityList * entitylist_loadfile(const char *path);

/* Save an entity type to the given file path */
void entitylist_savefile(EntityList *list, const char *path);

/* Deletes an entity list */
void entitylist_delete(EntityList *list);

/* Pushes a new entity to the given entity list.
 * Returns a pointer to the entity in the entity list */
Entity * entitylist_push(EntityList *list, const Entity *ent);

/* Remove entity at given index from the entity list */
void entitylist_remove(EntityList *list, uint id);

/* Doubles the maximum capacity of an entity list */
void entitylist_grow(EntityList *list);

/* Sets the entity animation properties for the specified animation */
void entitylist_setanimprop(EntityList *list, uint anim_id, EntityAnim props);

/* Clears entity collision completion flags */
void entitylist_clearcollisionflags(EntityList *list);

/* Calculates collisions for all entities in list */
void entitylist_calccollisions(EntityList *list);

/* Updates each entity in the list */
void entitylist_update(EntityList *list, u32 dt);

/* Renders each entity in the list by the given rendering method */
void entitylist_render(EntityList *list, int draw_mode);


/* Create a new instance of an entity from the database
 * Adds entity to the respective entity list */
Entity * entity_create(uint ent_type, const vec3 pos, const vec3 rot);

/* Deals damage to the entity */
void entity_damage(Entity *ent, int damage);


#endif /* ENTITY_H */
