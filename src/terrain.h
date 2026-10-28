/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  terrain.h - Functions and structures relating to world terrain
 * 
 */

#ifndef TERRAIN_H
#define TERRAIN_H

#include "atra.h"
#include "tiles.h"

#define TERRAIN_NULL_ID         0xFFFFFFFF

#define TERRAIN_SIZE            64
#define TERRAIN_SIZE_POW2       TERRAIN_SIZE * TERRAIN_SIZE

#define TERRAIN_VERTEX_COUNT    (TERRAIN_SIZE + 1) * (TERRAIN_SIZE + 1)
#define TERRAIN_INDEX_COUNT     TERRAIN_SIZE_POW2 * 6

#define HEIGHT_MAP_SIZE         TERRAIN_SIZE
#define BLEND_MAP_SIZE          256

#define HEIGHT_MAP_SIZE_POW2    HEIGHT_MAP_SIZE * HEIGHT_MAP_SIZE
#define BLEND_MAP_SIZE_POW2     BLEND_MAP_SIZE * BLEND_MAP_SIZE

enum neighbor_dir {
    TERRAIN_NEIGHBOR_TL,
    TERRAIN_NEIGHBOR_T,
    TERRAIN_NEIGHBOR_TR,
    TERRAIN_NEIGHBOR_L,
    TERRAIN_NEIGHBOR_R,
    TERRAIN_NEIGHBOR_BL,
    TERRAIN_NEIGHBOR_B,
    TERRAIN_NEIGHBOR_BR,
    TERRAIN_NEIGHBOR_TOTAL
};

enum terrain_texture_type {
    TERRAIN_TEX_DEF_ALBEDO,
    TERRAIN_TEX_DEF_NORMAL,
    TERRAIN_TEX_DEF_SPECULAR,
    TERRAIN_TEX_DEF_EMISSION,
    TERRAIN_TEX_DMG_ALBEDO,
    TERRAIN_TEX_DMG_NORMAL,
    TERRAIN_TEX_DMG_SPECULAR,
    TERRAIN_TEX_DMG_EMISSION,
    TERRAIN_TEX_TOTAL,
    TERRAIN_TEX_BLENDMAP = TERRAIN_TEX_TOTAL,
    TERRAIN_TEX_SHADOWMAP
};

enum terrain_mesh_vbo_type {
    TERRAIN_MESH_VBO_VERTEX,
    TERRAIN_MESH_VBO_TEXCOORD,
    TERRAIN_MESH_VBO_TEXCOORD2,
    TERRAIN_MESH_VBO_NORMAL,
    TERRAIN_MESH_VBO_TANGENT,
    TERRAIN_MESH_VBO_COLOR,
    TERRAIN_MESH_VBO_INDEX,
    TERRAIN_MESH_VBO_TOTAL
};

enum terrain_quad_corner {
    TERRAIN_QUAD_CORNER_TL,
    TERRAIN_QUAD_CORNER_TR,
    TERRAIN_QUAD_CORNER_BL,
    TERRAIN_QUAD_CORNER_BR,
    TERRAIN_QUAD_CORNER_TOTAL
};

enum terrain_shader_locs {
    TERRAIN_LOCS_ATR_POSITION,
    TERRAIN_LOCS_ATR_TEXCOORD,
    TERRAIN_LOCS_ATR_TEXCOORD2,
    TERRAIN_LOCS_ATR_NORMAL,
    TERRAIN_LOCS_ATR_TANGENT,
    TERRAIN_LOCS_ATR_COLOR,
    TERRAIN_LOCS_TEX_ALBEDO_DEF,
    TERRAIN_LOCS_TEX_NORMAL_DEF,
    TERRAIN_LOCS_TEX_SPECULAR_DEF,
    TERRAIN_LOCS_TEX_EMISSION_DEF,
    TERRAIN_LOCS_TEX_ALBEDO_DMG,
    TERRAIN_LOCS_TEX_NORMAL_DMG,
    TERRAIN_LOCS_TEX_SPECULAR_DMG,
    TERRAIN_LOCS_TEX_EMISSION_DMG,
    TERRAIN_LOCS_TEX_BLENDMAP,
    TERRAIN_LOCS_TEX_SHADOWMAP,
    TERRAIN_LOCS_VEC_VIEWPOS,
    TERRAIN_LOCS_VEC_AMBIENTCOLOR,
    TERRAIN_LOCS_VEC_SUNCOLOR,
    TERRAIN_LOCS_VEC_SUNDIR,
    TERRAIN_LOCS_MAT_MODEL,
    TERRAIN_LOCS_MAT_VIEW,
    TERRAIN_LOCS_MAT_PROJ,
    TERRAIN_LOCS_MAT_MVP,
    TERRAIN_LOCS_MAT_LIGHTSPACE,
    TERRAIN_LOCS_TOTAL
};

typedef struct Terrain Terrain;

typedef struct TerrainMesh {
    uint vert_count;
    uint index_count;

    uint vao_id;
    uint vbo_ids[TERRAIN_MESH_VBO_TOTAL];

    float verts[TERRAIN_VERTEX_COUNT * 3];
    float uvs[TERRAIN_VERTEX_COUNT * 2];
    float uvs2[TERRAIN_VERTEX_COUNT * 2];
    float norms[TERRAIN_VERTEX_COUNT * 3];
    float tans[TERRAIN_VERTEX_COUNT * 3];
    u8 colors[TERRAIN_VERTEX_COUNT * 4];
    uint indices[TERRAIN_INDEX_COUNT];
} TerrainMesh;

struct Terrain {
    TerrainMesh mesh;
    int dirty;

    Image height_img;
    Image blend_img;
    Texture blend_tex;

    uint index_map[TERRAIN_VERTEX_COUNT];
    float normal_buffer[TERRAIN_VERTEX_COUNT * 3];
    Terrain *neighbors[TERRAIN_NEIGHBOR_TOTAL];
};


/* Creates a new terrain instance.
 * Terrain image data arguments can be NULL to use default maps instead */
Terrain * terrain_create(Image *height, Image *blend);

/* Deletes an existing terrain instance */
void terrain_delete(Terrain *t);

/* Sets the terrain neighbors */
void terrain_setneighbors(Terrain *t, Terrain **neighbors);

/* Uploads terrain data images to textures, writing them to the GPU */
void terrain_upload_tex(Terrain *t);

/* Calculates the mesh of the terrain (fills norm buffer) */
void terrain_calcmesh(Terrain *t, TileChunk *tile_chunk);

/* Calculates terrain mesh normals */
void terrain_calcnorms(Terrain *t);

/* Gets the Y-coordinate of the given terrain chunk at a specific coordinate */
float terrain_getheight(Terrain *t, vec2 pos);

/* Upload the terrain data to the GPU */
void terrain_upload(Terrain *t, Material *mat);

/* Draws the terrain mesh to the screen with the given material */
void terrain_draw(Terrain *t, Material *mat, vec4 *model_mat, int depth_mode);

/* Draws an explosion crater to the height and blend maps (implicit remesh) */
void terrain_makecrater(Terrain *t, float radius, int x, int z);


/* Returns a new, empty terrain mesh instance */
TerrainMesh terrain_mesh_create(void);

/* Deletes a given terrain mesh */
void terrain_mesh_delete(TerrainMesh *mesh);

/* Uploads terrain mesh data to the GPU */
void terrain_mesh_upload(TerrainMesh *mesh, Material *mat);

/* Draws the terrain mesh to the screen */
void terrain_mesh_draw(TerrainMesh *mesh, Material *mat, vec4 *model_mat, int depth_mode);


#endif /* TERRAIN_H */
