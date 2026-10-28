/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  world.h - Functions and structures relating to the game world
 *
 */

#ifndef WORLD_H
#define WORLD_H

#include "atra.h"
#include "terrain.h"
#include "tiles.h"

#define PATH_TERRAIN_VS         "res/shader/terrain_fwd_vert.glsl"
#define PATH_TERRAIN_FS         "res/shader/terrain_fwd_frag.glsl"
#define PATH_TILECHUNK_VS       "res/shader/tile_fwd_vert.glsl"
#define PATH_TILECHUNK_FS       "res/shader/tile_fwd_frag.glsl"

#define HASH_CHUNK_FREEMODE     HASH_FREE_KEY

enum world_path_type {
    PATH_TERR_ALBEDO_DEF,
    PATH_TERR_NORMAL_DEF,
    PATH_TERR_SPECULAR_DEF,
    PATH_TERR_EMISSION_DEF,
    PATH_TERR_ALBEDO_DMG,
    PATH_TERR_NORMAL_DMG,
    PATH_TERR_SPECULAR_DMG,
    PATH_TERR_EMISSION_DMG,
    PATH_TILES
};

typedef struct World {
    Material terrain_mat;
    Shader tile_shader;
    int sun_enabled;
    vec3 sun_rot;
    Color sun_color;
    Color ambient_light;
    HashMap chunks;
    /* ParticleField env_particles[4]; */
} World;

typedef struct Chunk {
    Terrain *terrain_chunk;
    TileChunk *tile_chunk;
    mat4 model_mat;
} Chunk;


/* Creates and returns an empty, default world */
World world_create_default(void);

/* Completely unloads the world and all it's components */
void world_delete(World *world);

/* Loads the world and all related assets from a given world file */
World world_load(const char *path);

/* Saves world data to a world file with the specified path */
void world_save(World *world, const char *path, const char **file_paths);

/* Add a new chunk to the world, returns a pointer to the new chunk */
Chunk * world_addchunk(World *world, int x, int z, Image *height, Image *blend);

/* Deletes the chunk with the given coordinate, if one exists */
void world_removechunk(World *world, int x, int z);

/* Updates the world, remeshing chunks which require it */
void world_update(World *world);

/* Calculates the meshes for all chunks in world which require it */
void world_calcmeshes(World *world);

/* Renders the scene to a depth map from the sun's perspective */
void world_drawdepth(World *world);

/* Renders the world to the screen */
void world_draw(World *world);

/* Returns a pointer to the chunk at given coordinate (x, z) in chunk-space */
Chunk * world_getchunk(World *world, int x, int z);

/* Calculate neighbor pointers for each chunk */
void world_calcneighbors(World *world);

/* Generates a crater at the given (x, z) coordinate with the given radius */
void world_makecrater(World *world, float radius, float wx, float wz);


/* Returns a newly initialised chunk */
Chunk world_chunk_create(Image *height, Image *blend);

/* Deletes an existing world chunk */
void world_chunk_delete(Chunk *chunk);

/* Calculates tile and terrain meshes */
void world_chunk_calcmeshes(Chunk *chunk, World *world);


#endif /* WORLD_H */
