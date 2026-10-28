/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  decor.h - World decorations and props
 *
 */

#ifndef DECOR_H
#define DECOR_H

#include "atra.h"
#include "physics.h"
#include "particle.h"

#define ATRA_DECOR_DEFAULT_CAP	16

ATRA_LIST_DECLARE(Emitter)
ATRA_OPTION_DECLARE(ModelAnimRaw)

enum decor_vbo_type {
    DECOR_VBO_POSITION,			/* Vertex world-space position */
    DECOR_VBO_TEXCOORD,			/* Vertex primary texture coords */
    DECOR_VBO_NORMAL,			/* Vertex normals */
    DECOR_VBO_COLOR,			/* Vertex colors */
    DECOR_VBO_TANGENT,			/* Vertex tangents */
    DECOR_VBO_INDICES,			/* Vertex indices */
};

enum decor_tex_unit {
	DECOR_TEXUNIT_ALBEDO,		/* Albedo texture */
	DECOR_TEXUNIT_SPECULAR,		/* Specularity texture */
	DECOR_TEXUNIT_NORMAL,		/* Surface normal texture */
	DECOR_TEXUNIT_EMISSION		/* Emissive texture */
};

/* Individual decoration instance data */
typedef struct DecorInst {
	uint type;					/* Index of common decoration data in database */
	u32 anim_time;				/* Current time, in milliseconds, of the animation */
	Transform transform;		/* Position, rotation and scale data for decoration */
	List_Emitter emitters;		/* Emitters associated with this decoration */
} DecorInst;

ATRA_LIST_DECLARE(DecorInst)

/* Decoration collision map */
typedef struct DecorCollide {
	char *map;
	uint width;
	uint height;
} DecorCollide;

/* Entry in the decoration database */
typedef struct Decor {
	Model model;				/* Visual model for the decoration */

	PhysBody body;				/* Physics collision body */
	Option_ModelAnimRaw anim;	/* Animation that the decoration should have, if any */
	DecorCollide collide_map;	/* Tile-based collision map for AI pathing logic */

	char *name;					/* Decoration display string */
	char *file_path;			/* Path where decoration is saved */
	char *model_path;			/* Model file associated with this decoration type */
	char *anim_path;			/* Animation file associated with this decoration type */

	List_DecorInst decors;		/* List of decoration instances of this type */
	List_Emitter emitters;		/* Emitter templates for decoration type */
	float *model_mats;			/* Model matrices for each decoration instance */
} Decor;

ATRA_LIST_DECLARE(Decor)

typedef struct DecorDB {
	List_Decor decor;			/* Decoration type data */
	HashMap paths;				/* Decoration type data file paths */
} DecorDB;


/* Initialise the decoration database */
void decor_init(void);

/* Shutdown the decoration database, freeing all allocated data */
void decor_shutdown(void);

/* Loads a new decoration type into the database from the given file path */
uint decor_loadfile(const char *path);

/* Saves an existing decoration to a given file path */
void decor_savefile(const Decor *decor, const char *path);

/* Adds a given decoration type into the database */
uint decor_add(Decor *decor);

/* Creates a decoration type with custom attributes */
uint decor_new(PhysBody body, DecorCollide collide_map, const char *name,
		const char *file_path, const char *model_path,
		const char *anim_path, List_Emitter emitters);

/* Clears the database of all decoration types, resetting it */
void decor_clear(void);

/* Returns a pointer to a decoration type with the matching ID.
 * Returns NULL on failure. */
Decor * decor_get_byid(uint id);

/* Returns a pointer to a decoration type with the matching file path.
 * Returns NULL on failure. */
Decor * decor_get_bypath(const char *path);

/* Update all decorations in the database */
void decor_updateall(u32 dt);

/* Draw all decorations in the database (depth mode) */
void decor_drawall_depth(void);

/* Draw all decorations in the database (forward mode) */
void decor_drawall_forward(void);

/* Update a given decoration type and associated instances */
void decor_update(Decor *decor, u32 dt);

/* Render a given decoration type and associated instances (depth mode) */
void decor_draw_depth(Decor *decor, vec4 *lightspace);

/* Render a given decoration type and associated instances (forward mode) */
void decor_draw_forward(Decor *decor, vec4 *lightspace);



/* Create a new decoration instance with a given position, rotation and scale. */
uint decor_inst_new(Decor *decor, vec3 pos, vec3 rot, vec3 scale);

/* Deletes a given decoration instance */
void decor_inst_delete(Decor *decor, uint index);

/* Update a decoration instance */
void decor_inst_update(DecorInst *inst, u32 dt);

#endif /* DECOR_H */
