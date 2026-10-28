/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  tiles.h - Functions and structures relating to world terrain
 * 
 */

#ifndef TILES_H
#define TILES_H

#define TILE_DB_DEF_CAP         8

#define TILE_CHUNK_SIZE         16
#define TILE_CHUNK_SIZE_POW2    TILE_CHUNK_SIZE * TILE_CHUNK_SIZE
#define TILE_CHUNK_DEF_INST_CAP 8

#define TILE_MESH_DEF_SIZE      16

#include "atra.h"

enum tile_mesh_type {
    TILE_MESH_TLRB,
    TILE_MESH_LRB,
    TILE_MESH_TRB,
    TILE_MESH_RB,
    TILE_MESH_TLB,
    TILE_MESH_LB,
    TILE_MESH_TB,
    TILE_MESH_B,
    TILE_MESH_TLR,
    TILE_MESH_LR,
    TILE_MESH_TR,
    TILE_MESH_R,
    TILE_MESH_TL,
    TILE_MESH_L,
    TILE_MESH_T,
    TILE_MESH_NONE,
    TILE_MESH_TOTAL
};

enum tile_neighbor_type {
    TILE_NEIGHBOR_UP,
    TILE_NEIGHBOR_LEFT,
    TILE_NEIGHBOR_RIGHT,
    TILE_NEIGHBOR_DOWN,
    TILE_NEIGHBOR_TOTAL
};

enum tile_mesh_vbo_type {
    TILE_MESH_VBO_POSITION,
    TILE_MESH_VBO_TEXCOORD,
    TILE_MESH_VBO_NORMAL,
    TILE_MESH_VBO_TANGENT,
    TILE_MESH_VBO_MODELMAT,
    TILE_MESH_VBO_INDEX,
    TILE_MESH_VBO_TOTAL
};

enum tile_texture_type {
    TILE_TEXTURE_ALBEDO,
    TILE_TEXTURE_NORMAL,
    TILE_TEXTURE_SPECULAR,
    TILE_TEXTURE_EMISSION,
    TILE_TEXTURE_TOTAL,
    TILE_TEXTURE_SHADOWMAP = TILE_TEXTURE_TOTAL
};

enum tile_shader_locs {
    TILE_LOCS_TEX_ALBEDO,
    TILE_LOCS_TEX_NORMAL,
    TILE_LOCS_TEX_SPECULAR,
    TILE_LOCS_TEX_EMISSION,
    TILE_LOCS_TEX_SHADOWMAP,
    TILE_LOCS_VEC_VIEWPOS,
    TILE_LOCS_VEC_AMBIENTCOLOR,
    TILE_LOCS_VEC_SUNCOLOR,
    TILE_LOCS_VEC_SUNDIR,
    TILE_LOCS_MAT_MODEL,
    TILE_LOCS_MAT_VIEW,
    TILE_LOCS_MAT_PROJ,
    TILE_LOCS_MAT_MVP,
    TILE_LOCS_MAT_LIGHTSPACE,
    TILE_LOCS_TOTAL
};

enum tile_property_flags {
    TILE_FLAG_COLLIDABLE = BIT(0)
};

typedef struct TileChunk TileChunk;

typedef struct TileMat {
    Texture maps[TILE_TEXTURE_TOTAL];
} TileMat;

typedef struct TileMesh {
    uint vert_count;
    uint vert_cap;
    uint index_count;
    uint index_cap;
    uint index_highest;     /* Value of the highest current index */

    uint vao_id;
    uint vbo_ids[TILE_MESH_VBO_TOTAL];

    float *verts;
    float *uvs;
    float *norms;
    float *tans;
    uint *indices;
} TileMesh;

typedef struct TileDB {
    uint size;
    uint cap;
    TileMesh *meshes;   /* List of mesh data entries (16 entries per type) */
    TileMat *mats;      /* Material data for each tile (1 per type) */
    u32 *flags;         /* List of property flags for each tile (1 per type) */
} TileDB;

struct TileChunk {
    uint tiles[TILE_CHUNK_SIZE_POW2];
    uint dirty;
    uint mesh_count;
    uint mesh_cap;
    uint *mesh_tile_ids;    /* Array of pointers to mesh materials */
    TileMesh *meshes;       /* Array of meshes to draw for this chunk */
    TileChunk *neighbors[TILE_NEIGHBOR_TOTAL];
};


/* Initialise the tile database */
void db_tile_init(void);

/* Delete the tile database */
void db_tile_shutdown(void);

/* Reset the tile database to it's initial state */
void db_tile_reset(void);

/* Loads the two default tiles into the database (floor and wall) */
void db_tile_load_defaults(void);

/* Load tile data from a .gltf file into a tile mesh data entry (single mesh entry) */
void db_tile_load_gltf(const char *path, uint tile_id, uint tile_mesh_type);

/* Load tile mesh and texture data from a .asf file (full mesh entry data) */
void db_tile_load_asf(const char *path);

/* Save tile mesh and texture data to a .asf file (DEVELOPER ONLY) */
void db_tile_save_asf(const char *path, uint tile_id,
        const char *albedo_path, const char *normal_path,
        const char *specular_path, const char *emission_path);

/* Returns the number of entries in the database */
uint db_tile_getsize(void);

/* Returns the mesh of a specific tile type's given mesh entry ID */
TileMesh * db_tile_getmesh(uint tile_id, uint tile_mesh_type);

/* Returns the material of a specific tile type */
TileMat * db_tile_getmaterial(uint tile_id);


/* Create a new tile chunk */
TileChunk * tile_chunk_create(void);

/* Delete an existing tile chunk */
void tile_chunk_delete(TileChunk *t);

/* Set the neighbors of the tile chunk */
void tile_chunk_setneighbors(TileChunk *t, TileChunk **neighbors);

/* Populate tile chunk mesh and uploads completed mesh */
void tile_chunk_mesh(TileChunk *t);

/* Render the tile chunk to the screen */
void tile_chunk_draw(TileChunk *t, Shader *shader, mat4 model_mat, mat4 projview, int shader_mode);

/* Sets the value of a tile at the given local chunk coordinate */
void tile_chunk_settile(TileChunk *t, uint x, uint z, uint tile_id);

/* Return the flags for the tile type at given chunk coordinate */
u32 tile_chunk_getflags(TileChunk *t, uint x, uint z);


/* Initialise a new, empty tile mesh */
TileMesh tile_mesh_create(uint vert_count, uint index_count);

/* Delete an existing tile mesh */
void tile_mesh_delete(TileMesh *mesh);

/* Upload mesh data to the GPU (should only be called once per mesh) */
void tile_mesh_upload(TileMesh *mesh);

/* Render a given tile chunk mesh */
void tile_mesh_draw(TileMesh *mesh, TileMat *mat, Shader *shader);


#endif /* TILES_H */
