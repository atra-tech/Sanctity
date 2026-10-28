/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  particle.h - Particle management and rendering system.
 *
 */

#ifndef PARTICLE_H
#define PARTICLE_H

#include "atra.h"

#define PARTICLE_DEF_TYPE_CAP       8
#define PARTICLE_DEF_LIST_CAP       32

#define PATH_PARTICLE_VS            "res/shader/particle_vert.glsl"
#define PATH_PARTICLE_FS            "res/shader/particle_frag.glsl"

enum particle_property_flags {
    PARTICLE_PROP_ISANIMATED    = BIT(0),
    PARTICLE_PROP_FADEONDEATH   = BIT(1),
    PARTICLE_PROP_RANDOMROTATE  = BIT(2)
};

enum particle_tex_unit {
    PARTICLE_TEXUNIT_ALBEDO,
    PARTICLE_TEXUNIT_EMISSION,
    PARTICLE_TEXUNIT_SHADOWMAP
};

enum particle_mesh_vbos {
    PARTICLE_VBO_POSITION,
    PARTICLE_VBO_NORMAL,
    PARTICLE_VBO_MODELS,
    PARTICLE_VBO_UV_0,
    PARTICLE_VBO_UV_1,
    PARTICLE_VBO_UV_2,
    PARTICLE_VBO_UV_3,
    PARTICLE_VBO_COLOR,
    PARTICLE_VBO_TOTAL
};

enum particle_mesh_locs {
    PARTICLE_LOC_POSITION,
    PARTICLE_LOC_NORMAL,
    PARTICLE_LOC_MODELS,
    PARTICLE_LOC_UV_0,
    PARTICLE_LOC_UV_1,
    PARTICLE_LOC_UV_2,
    PARTICLE_LOC_UV_3,
    PARTICLE_LOC_COLOR,
    PARTICLE_LOC_ALBEDO,
    PARTICLE_LOC_EMISSIVE,
    PARTICLE_LOC_SHADOWMAP,
    PARTICLE_LOC_VIEWPOS,
    PARTICLE_LOC_AMBIENTCOLOR,
    PARTICLE_LOC_SUNCOLOR,
    PARTICLE_LOC_SUNDIR,
    PARTICLE_LOC_VIEWPROJ,
    PARTICLE_LOC_LIGHTSPACE,
    PARTICLE_LOC_TOTAL
};


typedef struct IParticle {
    i32 life;           /* Time left before the particle dies */
    vec3 pos;           /* Particle position */
    vec3 rot;           /* Particle rotation */
    vec3 dir;           /* Particle direction vector */
    float grav;         /* Current downwards speed */
} IParticle;

typedef struct ParticleMesh {
    uint vao;
    uint vbos[PARTICLE_VBO_TOTAL];
    float verts[18];
    float norms[18];
} ParticleMesh;

typedef struct ParticleList {
    uint id;            /* Index in particle list */
    uint size;          /* Number of particle instances */
    uint cap;           /* Current particle list max capacity */
    char *file_path;    /* Path of particle data file */

    u32 lifetime;       /* Duration the particle will exist for */
    u32 flags;          /* Property flag bitfield for particle type */

    float start_scale;  /* Size of particle at start of animation */
    float end_scale;    /* Size of particle at end of animation */

    Color start_color;  /* Color of particle at start of animation */
    Color end_color;    /* Color of particle at end of animation */

    float start_speed;  /* Speed of particle at start of animation */
    float end_speed;    /* Speed of particle at end of animation */

    float gravity;      /* Vertical acceleration of particle per second */
    
    Texture tex_color;  /* Texture sheet for particle */
    Texture tex_emit;   /* Emissive texture sheet for particle */
    char *color_path;   /* Path string of albedo texture file */
    char *emit_path;    /* Path string of emission texture file */
    uint frame_size;    /* Width and height of animation frames */
    uint frame_count;   /* Number of frames in animated texture */

    ParticleMesh mesh;  /* Particle mesh data */
    IParticle *parts;   /* Array of particles in list */
    Color *colors;      /* Array of particle instance colors */
    float *modelmats;   /* Array of particle instance model matrices */
    float *quad_uvs;    /* Array of particle instance UV data */
} ParticleList;

typedef struct ParticleDB {
    uint size;          /* Number of particle types in database */
    uint cap;           /* Current particle type max capacity */
    ParticleList *lists;/* Array of particle lists for each particle type */
} ParticleDB;

typedef struct Emitter {
    uint part_id;       /* ID of database entry for particle to emit */
    
    vec3 pos;           /* Position vector */
    vec3 rot;           /* Rotation vector */
    vec3 dir;           /* Direction vector */
    vec3 right;         /* Right vector */
    vec3 up;            /* Up vector */

    float spread;       /* Angle to which particles can deviate (radians) */
    float spawn_spread; /* Maximum distance from the emitter a particle can spawn */

    uint fire_rate;     /* Number of particles to emit per second */
    u32 last_fire;      /* Time since the last particle was emitted */
    u32 curr_time;      /* Time the emitter has been firing for */
    u32 duration;       /* Time the emitter will fire for when triggered (0 = infinite) */
} Emitter;

typedef struct EmitterField {
    uint part_id;       /* ID of database entry for particle to emit */

    vec3 pos;           /* Position of the origin of the emitter field */
    vec3 dims;          /* Dimensions of the emitter field box (w, h, d) */

    uint fire_rate;     /* Number of particles to emit per second */
    u32 last_fire;
    u32 curr_time;
    u32 duration;
} EmitterField;

ATRA_OPTION_DECLARE(ParticleList)

/* Initialise the particle database */
void db_particle_init(void);

/* Delete the particle database */
void db_particle_shutdown(void);

/* Clear the particle database */
void db_particle_clear(void);

/* Load a new particle type from a file */
Option_uint db_particle_loadfile(const char *path);

/* Returns a pointer to the particle list associated with the given ID */
ParticleList * db_particle_getlist(uint id);

/* Returns a pointer to the particle list associated with the given file path.
 * NOTE: Implicitly loads given file into database if none exists. */
ParticleList * db_particle_getlist_bypath(const char *path);

/* Returns a pointer to the particle list associated with the given file path.
 * NOTE: Implicitly loads given file into database if none exists. */
uint db_particle_getid_bypath(const char *path);

/* Expand the capacity of the database */
void db_particle_grow(void);

/* Update all particle lists in the database */
void db_particle_update(u32 dt);

/* Update all particle list in the database (using thread pool) */
void db_particle_update_dispatch(u32 dt);

/* Upload all newly generated particle instance data to the GPU */
void db_particle_upload(void);

/* Render all particle lists in the database to the depth buffer */
void db_particle_draw_depth(void);

/* Render all particle lists in the database to the screen */
void db_particle_draw_forward(void);


/* Create a new particle list */
ParticleList particlelist_create(void);

/* Create a new particle list with custom parameters (use for file generation) */
Option_ParticleList particlelist_create_custom(u32 lifetime, u32 flags, const char *save_path,
    float start_scale, float end_scale,
    Color start_color, Color end_color,
    float start_speed, float end_speed,
    float gravity, const char *color_path, const char *emit_path,
    uint frame_size, uint frame_count);

/* Creates a new particle list from the given file */
Option_ParticleList particlelist_loadfile(const char *path);

/* Write the particle list properties to a file */
void particlelist_savefile(ParticleList *list, const char *path);

/* Delete an existing particle list */
void particlelist_delete(ParticleList *list);

/* Expand a given particle list's capacity */
void particlelist_grow(ParticleList *list);

/* Clear particle list of all existing particles */
void particlelist_clear(ParticleList *list);

/* Adds a new particle to the list with the given life and position.
 * NOTE: Life capped by containing list's `lifetime` variable */
void particlelist_add(ParticleList *list, vec3 pos, vec3 rot, vec3 dir, u32 life, float grav);

/* Remove a single particle instance from a particle list */
void particlelist_remove(ParticleList *list, uint index);

/* Remove a contiguous range of particles from a particle list in [start, end) */
void particlelist_removerange(ParticleList *list, uint start, uint end);

/* Update a particle list, and all contained particles */
void particlelist_update(ParticleList *list, u32 dt);

/* Update a particle list, and all contained particles (using thread pool) */
void particlelist_update_dispatch(ParticleList *list, u32 dt);

/* Upload particle instance data to the GPU */
void particlelist_upload(ParticleList *list);

/* Render all particles in a particle list to the screen */
void particlelist_render(ParticleList *list);


/* Create a new default particle mesh */
ParticleMesh particlemesh_create(void);

/* Delete an existing particle mesh */
void particlemesh_delete(ParticleMesh *mesh);


/* Create a new particle emitter */
Emitter emitter_create(uint part_id, vec3 pos, vec3 rot, uint rate, u32 dur, float spread);

/* Resets the firing time to 0, restarting firing */
void emitter_fire(Emitter *e);

/* Halts any emitter from firing */
void emitter_stop(Emitter *e);

/* Update an emitter, spawning new particles when necessary */
void emitter_update(Emitter *e, u32 dt);

#endif /* PARTICLE_H */
