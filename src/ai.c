/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  ai.c - AI behavior modules for entity movement and strategy
 *
 */

#include "ai.h"

ATRA_LIST_DEFINE(PathPoint)
ATRA_OPTION_DEFINE(AIPath)
ATRA_OPTION_DEFINE(PathPoint)
ATRA_LIST_DEFINE(FreeArea)
ATRA_OPTION_DEFINE(FreeArea)

static AIPath ai_reconstructpath();
static AIPath ai_calcpath_astar(AICore *ai, EntityList *ents, uint ent_id,
		PathPoint target, float (*heur)(void));

/* Instantiates an entity AI struct */
AICore
ai_core_create(uint state, uint focus, uint team)
{
	return (AICore) {
		.state = state,
		.focus = focus,
		.team = team,
	};
}

/* Update an entity's movement in the direction of the desired path */
/*void
ai_update_entity(AICore *ai, EntityList *ents, uint ent_id)
{
	
}*/

/* Calculates and returns a path from the entity to a given target point */
/*Option_AIPath
ai_calcpath(AICore *ai, EntityList *ents, Entity *ent, PathPoint target)
{
	ai_calcpath_astar(ai, ents, ent, target);

}*/

/*static Option_AIPath
ai_calcpath_astar(AICore *ai, EntityList *ents, Entity *ent,
		PathPoint target, float (*heur)(void))
{
	//Entity *ent = &ents->ents[ent_id];

}*/


/* Create a new free area with a position starting from the given tile coordinate.
 * Implicitly calculates the bounds of the free area. */
FreeArea
ai_freearea_create(int *cover_map, int x, int z)
{
	FreeArea area = (FreeArea) {
		.adjacents = atra_list_uint_create(AI_DEF_AREA_ADJACENTS),
		.entities = atra_list_uint_create(AI_DEF_AREA_ENTITIES),
		.min = {x, z},
		.max = {x, z}
	};

	const int zero_row[TILE_CHUNK_SIZE] = {0};
	const ivec2 chunk_pos = {x / TILE_CHUNK_SIZE, z / TILE_CHUNK_SIZE};
	const ivec2 pos_mod = {x % TILE_CHUNK_SIZE, z % TILE_CHUNK_SIZE};
	const ivec2 local_pos = {
		(pos_mod[0] < 0) ? TILE_CHUNK_SIZE + pos_mod[0] : pos_mod[0],
		(pos_mod[1] < 0) ? TILE_CHUNK_SIZE + pos_mod[1] : pos_mod[1],
	};
	//TileChunk * const chunk = world_getchunk(world, chunk_pos[0], chunk_pos[1])->tile_chunk;

	int cx, cz;
	

	/* Find area width */
	for (cx = local_pos[0] + 1, cz = local_pos[1]; cx < TILE_CHUNK_SIZE; cx++) {
		if (cover_map[cx + cz * TILE_CHUNK_SIZE] == TRUE) {
			break;
		}
	}
	area.max[0] = cx;
	
	/* Find area height */
	for (cz = local_pos[1]; cz < TILE_CHUNK_SIZE; cz++) {
		const int *row = &cover_map[cz * TILE_CHUNK_SIZE];
		if (memcmp(row, zero_row, TILE_CHUNK_SIZE * sizeof(int)) != 0) {
			break;
		}
		/* Update cover map if row is clear of blockages */
		for (cx = local_pos[0]; cx < area.max[0]; cx++) {
			cover_map[cx + cz * TILE_CHUNK_SIZE] = 1;
		}
	}
	area.max[0] = cx + chunk_pos[0] * TILE_CHUNK_SIZE;
	area.max[1] = cz + chunk_pos[1] * TILE_CHUNK_SIZE;

	return area;
}

/* Clones an existing free area */
FreeArea
ai_freearea_clone(const FreeArea *area)
{
	FreeArea result;

	result.adjacents = atra_list_uint_create(area->adjacents.cap);
	result.entities = atra_list_uint_create(area->entities.cap);

	result.adjacents.size = area->adjacents.size;
	result.entities.size = area->entities.size;

	memcpy(result.adjacents.data, area->adjacents.data, result.adjacents.size * sizeof(int));
	memcpy(result.entities.data, area->entities.data, result.entities.size * sizeof(int));

	result.min[0] = area->min[0];
	result.min[1] = area->min[1];
	result.max[0] = area->max[0];
	result.max[1] = area->max[1];

	return result;
}

/* Creates a new, empty free-area graph */
AreaGraph
ai_areagraph_create(void)
{
	return (AreaGraph) {
		.areas = atra_list_FreeArea_create(32)
	};
}

/* Deletes an existing area graph */
void
ai_areagraph_delete(AreaGraph *graph)
{
	atra_list_FreeArea_delete(&graph->areas);
}

/* Calculates the free areas for a given chunk */
static void
ai_areagraph_calcchunk(AreaGraph *graph, TileChunk *chunk)
{
	int cover_map[TILE_CHUNK_SIZE_POW2] = {FALSE};
	FreeArea area;
	uint x, z;

	/* Compose an initial cover map for the chunk */
	for (z = 0; z < TILE_CHUNK_SIZE; z++) {
		for (x = 0; x < TILE_CHUNK_SIZE; x++) {
			const uint index = x + z * TILE_CHUNK_SIZE;
			
			if (chunk->tiles[index] != 0 &&
					tile_chunk_getflags(chunk, x, z) & TILE_FLAG_COLLIDABLE) {
				cover_map[index] = TRUE;
			}
		}
	}

	/* Traverse cover map and deploy area graphs until chunk is complete */
	for (z = 0; z < TILE_CHUNK_SIZE; z++) {
		for (x = 0; x < TILE_CHUNK_SIZE; x++) {
			if (cover_map[x + z * TILE_CHUNK_SIZE] == FALSE) {
				area = ai_freearea_create(cover_map, x, z);
				ai_areagraph_addarea(graph, &area);
			}
		}
	}
}

/* Calculates the free areas and area graph for the world */
void
ai_areagraph_calculate(AreaGraph *graph, World *world)
{
	/* Calculate area graph for each chunk */

	/* TODO: Connect graphs from adjacent chunks together */
	const size_t chunk_count = world->chunks.size;

	int **chunk_pos;
	Chunk *chunk;
	uint i;

	for (i = 0; i < chunk_count; i++) {
		chunk_pos = atra_hashmap_getkey(&world->chunks, i);
		
		if (*chunk_pos == NULL) {
			continue;
		}
		chunk = atra_hashmap_getval(&world->chunks, i);
		ai_areagraph_calcchunk(graph, chunk->tile_chunk);
	}
}

/* Adds a new free area to the area graph, connecting it with adjacent areas */
void
ai_areagraph_addarea(AreaGraph *graph, FreeArea *area)
{
	const size_t graph_size = graph->areas.size;

	FreeArea *adj;
	uint i;

	atra_list_FreeArea_add(&graph->areas, *area);

	for (i = 0; i < graph_size; i++) {
		adj = &graph->areas.data[i];

		/* Add edge entries to both free areas if adjacent */
		if ((area->max[0] >= adj->min[0]) && (area->max[1] >= adj->min[1]) &&
				(area->min[0] <= adj->max[0]) && (area->min[1] <= adj->max[1])) {
			atra_list_uint_add(&area->adjacents, i);
			atra_list_uint_add(&adj->adjacents, graph->areas.size - 1);
		}
	}
}

/* Merges the graph `src` into the graph `dest`. Creates a copy of `src`. */
void
ai_areagraph_merge(AreaGraph *dest, const AreaGraph *src)
{
	/* Get offset info for dest
	 * Add each new area to graph, adding offset to adjacency lists of each area
	 * Connect new graph to the current graph
	 */
	const uint src_size = src->areas.size;
	const uint offset = dest->areas.size;

	FreeArea area;
	uint i, j;

	for (i = 0; i < src_size; i++) {
		area = ai_freearea_clone(&src->areas.data[i]);

		for (j = 0; j < area.adjacents.size; j++) {
			area.adjacents.data[j] += offset;
		}
		ai_areagraph_addarea(dest, &area);
	}
}

