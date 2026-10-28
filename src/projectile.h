/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  projectile.h - Interactable projectiles which affect the world and entities
 *
 */

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "atra.h"
#include "entity.h"
#include "particle.h"

#define PROJECTILE_DB_INIT_CAP      32
#define PROJECTILE_LIST_INIT_CAP    16
#define PROJECTILE_EVENT_LIST_SIZE  4

#define PROJECTILE_PATH_DEPTH_VS    "res/shader/proj_depth_vert.glsl"
#define PROJECTILE_PATH_DEPTH_FS    "res/shader/proj_depth_frag.glsl"
#define PROJECTILE_PATH_FORWARD_VS  "res/shader/proj_forward_vert.glsl"
#define PROJECTILE_PATH_FORWARD_FS  "res/shader/proj_forward_frag.glsl"


enum projectile_attribs {
    PROJECTILE_ATTR_DIEONCOLLIDE = BIT(0),  /* Despawns on collision */
    PROJECTILE_ATTR_NOMOVEMENT   = BIT(1),  /* Projectile does not move */
    PROJECTILE_ATTR_NOCOLLIDE    = BIT(2),  /* Projectile does not collide in any way */
    PROJECTILE_ATTR_HOMING       = BIT(3),  /* Projectile homes in on target entity */
    PROJECTILE_ATTR_BOUNCING     = BIT(4),  /* Bounces off of collidables */
    PROJECTILE_ATTR_MULTIHIT     = BIT(5),  /* Projectile can affect entities multiple times */
    PROJECTILE_ATTR_ORBIT        = BIT(6),  /* Projectile orbits originator entity */
    PROJECTILE_ATTR_IMMORTAL     = BIT(7),  /* Projectile does not die over time */
    PROJECTILE_ATTR_FLOORLOCKED  = BIT(8),  /* Y-coordinate will always be locked to the ground */
    PROJECTILE_ATTR_TERRAINDMG   = BIT(9),  /* Projectile digs out terrain */
};

enum projectile_emission_behavior {
    PROJECTILE_EMIT_STATIC,     /* Non-moving projectile */
    PROJECTILE_EMIT_RADIAL,     /* Fire in circle around y-axis */
    PROJECTILE_EMIT_FAN,        /* Fire in a flat, horizontal spread */
    PROJECTILE_EMIT_SPHERE,     /* Fire in spherical shape */
    PROJECTILE_EMIT_CONE        /* Fire in directional cone shape */
};

enum projectile_collision_target_type {
    PROJECTILE_HIT_NONE,
    PROJECTILE_HIT_TERRAIN,
    PROJECTILE_HIT_TILE,
    PROJECTILE_HIT_ENTITY,
    PROJECTILE_HIT_DECORATION
};

enum projectile_event_type {
    PROJECTILE_EVENT_UPDATE,    /* Run every frame */
    PROJECTILE_EVENT_TICK,      /* Run every tick interval */
    PROJECTILE_EVENT_DEATH,     /* Run on projectile death */
    PROJECTILE_EVENT_BOUNCE,    /* Run on projectile bounce collision */
    PROJECTILE_EVENT_TOTAL
};

enum projectile_draw_mode {
    PROJECTILE_DRAW_FORWARD,
    PROJECTILE_DRAW_DEPTH
};

/* A collidable projectile which can damage entities and world */
typedef struct Projectile {
    i32 life;               /* Current time left to live */
    vec3 pos;               /* Position of projectile */
    vec3 rot;               /* Rotation of projectile */
    vec3 dir;               /* Direction of projectile movement */
    int damage;             /* Current projectile damage */
    float grav;             /* Current downwards vertical speed */
    float speed;            /* Current direct movement speed */
    float scale;            /* Current scale multiplier */
    float force;            /* Current force to impart on entities */
    Color color;            /* Current color tint modifier */
    Entity *origin;         /* Pointer to the entity which created the particle */
    AudioSource snd_src;
} Projectile;

ATRA_LIST_DECLARE(Projectile)

typedef struct ProjectileEvent {
    uint type;              /* Type of event */
    uint sound_id;          /* ID of audio buffer to play through projectile on event */
    List_uint prt_ids;      /* Particle types produced by event */
    List_uint prt_counts;   /* Number of each particle type to spawn */
    List_uint prt_emittype; /* Emission style for each particle type */
    List_float prt_spread;  /* Angle of spread for candidate emission types */
    List_uint prj_ids;      /* Projectile types produced by event */
    List_uint prj_counts;   /* Number of each projectile type to spawn */
    List_uint prj_emittype; /* Emission style for each projectile type */
    List_float prj_spread;  /* Angle of spread for candidate emission types */
} ProjectileEvent;

/* List in database concerning all particles of a common type */
typedef struct ProjectileData {
    uint id;                /* Index of projectile list in database */
    u32 attrs;              /* Projectile attribute flags */
    i32 max_life;           /* Amount of time the projectile lives for */
    char *file_path;        /* Path to file where projectile data is stored */

    int damage_start;       /* Damage/healing of projectile. Start value */
    int damage_end;         /* Damage/healing of projectile. End value */

    float speed_start;      /* Projectile movement speed. Start value */
    float speed_end;        /* Projectile movement speed. End value */

    float scale_start;      /* Projectile scale. Start value */
    float scale_end;        /* Projectile scale. End value */

    float force_start;      /* Projectile impact force. Start value */
    float force_end;        /* Projectile impact force. End value */

    uint tick_rate;         /* Number of update ticks per second */
    u32 curr_time;          /* Current number of ms the projectile has existed */
    u32 last_tick;          /* Timestamp of the last update tick */

    float radius;           /* Effective collision radius (affected by scale) */
    float gravity;          /* Gravity acceleration accumulated by particles per second */

    int event_flags[PROJECTILE_EVENT_TOTAL];        /* Set when corresponding event is enabled */
    ProjectileEvent events[PROJECTILE_EVENT_TOTAL]; /* Events for particle events */

    List_Projectile projs;  /* All projectile instances sharing common data */
} ProjectileData;


/* Initialise the projectile database */
void projectile_init(void);

/* Shudown the projectile database */
void projectile_shutdown(void);

/* Load a projectile type into the database from a file. Returns the projectile ID */
uint projectile_loadfile(const char *path);

/* Add a given projectile data entry to the database */
uint projectile_pushlist(ProjectileData *list);

/* Returns a pointer to the projectile data list at the given ID */
ProjectileData * projectile_getlist(uint id);

/* Returns a pointer to the projectile data list associated with the given path.
 * Implicitly loads the projectile at given path if it is not present in the database. */
ProjectileData * projectile_getlist_bypath(const char *path);

/* Returns the ID of the projectile data list associated with the given path.
 * Implicitly loads the projectile at given path if it is not present in the database. */
uint projectile_getid_bypath(const char *path);

/* Creates a new projectile of the given type ID, from the projectile which spawns it */
//void projectile_spawnfrom(uint proj_id, Projectile *src, Entity *origin);

/* Update all projectiles */
void projectile_updateall(u32 dt);

/* Draw all projectiles */
void projectile_drawall(void);


/* Create a new, empty projectile data list */
/* TODO: Set up to allow custom specification of all projectile data fields */
ProjectileData projectilelist_create(void);

/* Manually sets up a projectile list */
ProjectileData projectilelist_create_manual(u32 attrs, i32 max_life, const char *save_path,
        int damage_start, int damage_end,
        float speed_start, float speed_end,
        float scale_start, float scale_end,
        float force_start, float force_end,
        uint tick_rate, float radius, float gravity);

/* Load a new projectile type from given file path */
ProjectileData projectilelist_loadfile(const char *path);

/* Save a projectile type to file at path */
void projectilelist_savefile(ProjectileData *list);

/* Delete a projectile list, and all contained projectiles */
void projectilelist_delete(ProjectileData *list);

/* Create a new projectile for the given list with supplied attributes */
void projectilelist_add_ptr(ProjectileData *list, i32 life, vec3 pos, vec3 rot, vec3 dir, float grav, Entity *origin);

/* Create a new projectile for the given type id with supplied attributes */
void projectilelist_add_id(uint id, i32 life, vec3 pos, vec3 rot, vec3 dir, float grav, Entity *origin);

/* Initialises and enables the given event type for a projectile type */
void projectilelist_event_enable(ProjectileData *list, uint event_type);

/* Adds a new projectile to an event in a projectile list */
void projectilelist_event_addparticle(ProjectileData *list, uint event_type,
        uint prt_id, uint prt_count, uint prt_emittype, uint prt_spread);

/* Adds a new projectile to an event in a projectile list */
void projectilelist_event_addprojectile(ProjectileData *list, uint event_type,
        uint prj_id, uint prj_count, uint prj_emittype, uint prj_spread);

/* Sets the sounds of the given event in a projectile list */
void projectilelist_event_setsound(ProjectileData *list, uint event_type, uint snd_id);

/* Runs an event type for the given projectile data list */
void projectilelist_event_run(ProjectileData *list, Projectile *p, uint event_type);

/* Calculates the collisions for each particle in the list */
void projectilelist_calccollisions(ProjectileData *list);

/* Advances the physics simulation for each particle in the list */
void projectilelist_updatephysics(ProjectileData *list, u32 dt);

/* Calls for a specific projectile to die */
void projectilelist_die(ProjectileData *list, uint id, uint hit_type, void *hit);

/* Calls for a specific projectile to die */
void projectilelist_dieptr(ProjectileData *list, Projectile *p, uint hit_type, void *hit);

/* Updates each particle in the given data list */
void projectilelist_update(ProjectileData *list, u32 dt);

/* Draw all projectiles in the given projectile data list */
void projectilelist_draw(ProjectileData *list, int draw_mode);


#endif /* PROJECTILE_H */
