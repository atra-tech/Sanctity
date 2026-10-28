/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  ai.h - AI behavior modules for entity movement and strategy
 *
 */

#ifndef AI_H
#define AI_H

#include "atra.h"
#include "world.h"
#include "entity.h"

#define AI_DEF_AREA_ADJACENTS	8
#define AI_DEF_AREA_ENTITIES	32


enum ai_state_type {
    AI_STATE_NONE,              /* No AI, do nothing */
    AI_STATE_PATHTOGOAL,        /* Path to the current win-condition objective */
    AI_STATE_PATHTOPLAYER,      /* Path to player's position */
    AI_STATE_ATTACK             /* AI is within attack range and is attacking */
};

enum ai_focus_type {
    AI_FOCUS_NONE,              /* No focus, do nothing */
    AI_FOCUS_NORMAL,            /* Paths towards objective, only attacks player when in aggro range */
    AI_FOCUS_PLAYER,            /* Paths towards and attacks only the player */
    AI_FOCUS_GOAL,              /* Paths towards and attacks only the objective */
};

enum ai_team_type {
    AI_TEAM_FRIENDLY,           /* Entity is friendly to player */
    AI_TEAM_AGGRESSIVE,         /* Entity is aggressive to player */
    AI_TEAM_PASSIVE             /* Entity remains passive until attacked */
};


typedef struct PathPoint {
    float x;
    float z;
} PathPoint;
ATRA_LIST_DECLARE(PathPoint)

typedef struct AIPath {
    List_PathPoint points;
} AIPath;
ATRA_OPTION_DECLARE(AIPath)
ATRA_OPTION_DECLARE(PathPoint)

typedef struct AICore {
    uint state;
    uint focus;
    uint team;

	Entity *target_ent;			/* Current target entity */
	Option_PathPoint target;	/* Current target position */
	Option_AIPath path;			/* Current path the AI has chosen (recalcs on world change or target switch) */
} AICore;

typedef struct FreeArea FreeArea;
ATRA_LIST_DECLARE(FreeArea)

struct FreeArea {
	List_uint adjacents;
	List_uint entities;
	ivec2 min;
	ivec2 max;
};
ATRA_OPTION_DECLARE(FreeArea)

typedef struct AreaGraph {
	List_FreeArea areas;
} AreaGraph;

/*	Area Graph:
 *		WARNING: 
 *			Large decorations and entities can overlap into other free areas.
 *			Perhaps allow adding them to multiple free areas?
 *			Issues with multiple checks being run on single object.
 *			Maintain check-list in the collision function to avoid this?
 *				
 *
 *		Has optimisations for other stuff, including collision search optimisations:
 *			If an entity is completely inside a free area:
 *				Only entity collisions need to be checked
 *				Only entities within the same free area should be checked
 *			If an entity is touching the edge of its area:
 *				Check for wall collisions too
 *				If entity within the same 
 *
 *      Should use quadtree to easily optimize physics calculation groups
 *      let physics system handle this, not AI module
 */


/* Instantiates an entity AI struct */
AICore ai_core_create(uint state, uint focus, uint team);

/* Update an entity's movement in the direction of the desired path */
void ai_update_entity(AICore *ai, EntityList *ents, Entity *ent);

/* Calculates and returns a path from the entity to a given target point */
AIPath ai_calcpath(AICore *ai, EntityList *ents, Entity *ent, PathPoint target);


/* Create a new free area with a position starting from the given tile coordinate.
 * Implicitly calculates the bounds of the free area. */
FreeArea ai_freearea_create(int *cover_map, int x, int z);

/* Clones an existing free area */
FreeArea ai_freearea_clone(const FreeArea *area);


/* Creates a new, empty free-area graph */
AreaGraph ai_areagraph_create(void);

/* Deletes an existing area graph */
void ai_areagraph_delete(AreaGraph *graph);

/* Calculates the free areas and area graph for the world */
void ai_areagraph_calculate(AreaGraph *graph, World *world);

/* Adds a new free area to the area graph, connecting it with adjacent areas */
void ai_areagraph_addarea(AreaGraph *graph, FreeArea *area);

/* Merges the graph 'src' into the graph 'dest' */
void ai_areagraph_merge(AreaGraph *dest, const AreaGraph *src);


#endif /* AI_H */
