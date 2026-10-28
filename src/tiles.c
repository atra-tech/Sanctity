/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  tiles.h - Functions and structures relating to world terrain
 * 
 */

#include "tiles.h"
#include "file.h"
#include "state_game.h"

static const float def_wall_verts[] = {
    0.0f,  2.0f,  0.0f,
    0.0f, -2.0f,  0.0f,
    1.0f, -2.0f,  0.0f,
    1.0f,  2.0f,  0.0f,
    1.0f,  2.0f, -1.0f,
    1.0f, -2.0f, -1.0f,
    0.0f, -2.0f, -1.0f,
    0.0f,  2.0f, -1.0f,
    1.0f,  2.0f,  0.0f,
    1.0f, -2.0f,  0.0f,
    1.0f, -2.0f, -1.0f,
    1.0f,  2.0f, -1.0f,
    0.0f,  2.0f, -1.0f,
    0.0f, -2.0f, -1.0f,
    0.0f, -2.0f,  0.0f,
    0.0f,  2.0f,  0.0f,
    0.0f,  2.0f, -1.0f,
    0.0f,  2.0f,  0.0f,
    1.0f,  2.0f,  0.0f,
    1.0f,  2.0f, -1.0f,
    0.0f, -2.0f,  0.0f,
    0.0f, -2.0f, -1.0f,
    1.0f, -2.0f, -1.0f,
    1.0f, -2.0f,  0.0f
};

static const float def_floor_verts[] = {
    0.0f,  0.0f,  0.0f,
    0.0f, -2.0f,  0.0f,
    1.0f, -2.0f,  0.0f,
    1.0f,  0.0f,  0.0f,
    1.0f,  0.0f, -1.0f,
    1.0f, -2.0f, -1.0f,
    0.0f, -2.0f, -1.0f,
    0.0f,  0.0f, -1.0f,
    1.0f,  0.0f,  0.0f,
    1.0f, -2.0f,  0.0f,
    1.0f, -2.0f, -1.0f,
    1.0f,  0.0f, -1.0f,
    0.0f,  0.0f, -1.0f,
    0.0f, -2.0f, -1.0f,
    0.0f, -2.0f,  0.0f,
    0.0f,  0.0f,  0.0f,
    0.0f,  0.0f, -1.0f,
    0.0f,  0.0f,  0.0f,
    1.0f,  0.0f,  0.0f,
    1.0f,  0.0f, -1.0f,
    0.0f, -2.0f,  0.0f,
    0.0f, -2.0f, -1.0f,
    1.0f, -2.0f, -1.0f,
    1.0f, -2.0f,  0.0f
};

static const float def_norms[] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f,-1.0f,
    0.0f, 0.0f,-1.0f,
    0.0f, 0.0f,-1.0f,
    0.0f, 0.0f,-1.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
   -1.0f, 0.0f, 0.0f,
   -1.0f, 0.0f, 0.0f,
   -1.0f, 0.0f, 0.0f,
   -1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f,-1.0f, 0.0f,
    0.0f,-1.0f, 0.0f,
    0.0f,-1.0f, 0.0f,
    0.0f,-1.0f, 0.0f
};

static const float def_uvs[] = {
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
};

static const float def_indices[] = {
    0, 1, 2,
    2, 3, 0
};

static const u8 def_floor_colors[] = {
    0x7F, 0x7F, 0x7F, 0xFF,
    0x7F, 0x7F, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0xFF
};
static const u8 def_wall_colors[] = {
    0xC0, 0xC0, 0xC0, 0xFF,
    0x7F, 0x7F, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0xFF
};

static TileDB tile_db;

static void db_tile_grow(void);
static void tile_chunk_grow(TileChunk *t);
static void tile_mesh_grow(TileMesh *mesh);
static void tile_mesh_add(TileMesh *dst, const TileMesh *src, uint x, uint z);
static void tile_mesh_initialupload(TileMesh *mesh);
static void tile_mesh_overwrite(TileMesh *mesh);
static void tile_mesh_calctans(TileMesh *mesh);

/* Initialise the tile database */
void
db_tile_init(void)
{
    tile_db.size = 1;
    tile_db.cap = TILE_DB_DEF_CAP;
    tile_db.meshes = malloc(TILE_DB_DEF_CAP * TILE_MESH_TOTAL * sizeof(TileMesh));
    tile_db.mats = malloc(TILE_DB_DEF_CAP * sizeof(TileMat));
    tile_db.flags = malloc(TILE_DB_DEF_CAP * sizeof(u32));

    /* Set up the default material */
}

/* Delete the tile database */
void
db_tile_shutdown(void)
{
    uint i, j;

    for (i = 1; i < tile_db.size; i++) {
        for (j = 0; j < 16; j++) {
            tile_mesh_delete(tile_db.meshes + j + i * TILE_MESH_TOTAL);
        }

        for (j = 0; j < TILE_TEXTURE_TOTAL; j++) {
            atra_texture_destroy(&tile_db.mats[i].maps[j]);
        }
    }
}

/* Reset the tile database to it's initial state */
void
db_tile_reset(void)
{
    uint i, j;

    for (i = 1; i < tile_db.size; i++) {
        for (j = 0; j < 16; j++) {
            tile_mesh_delete(tile_db.meshes + j + i * TILE_MESH_TOTAL);
        }
        
        for (j = 0; j < TILE_TEXTURE_TOTAL; j++) {
            atra_texture_destroy(&tile_db.mats[i].maps[j]);
        }
    }
    tile_db.size = 1;
}

/* Loads the default tile into the database */
void
db_tile_load_defaults(void)
{
    const uint vertex_count = 4 * 6;
    const uint index_count = 6 * 6;

    TileMesh *f_ms;
    TileMesh *w_ms;
    TileMat *f_mat, *w_mat;
    Image f_img, w_img;
    Color f_color, w_color;
    uint *idx;
    uint i, j;

    for (i = 0; i < TILE_MESH_TOTAL; i++) {
        f_ms = tile_db.meshes + i + 1 * TILE_MESH_TOTAL;
        w_ms = tile_db.meshes + i + 2 * TILE_MESH_TOTAL;
        *f_ms = tile_mesh_create(vertex_count, index_count);
        *w_ms = tile_mesh_create(vertex_count, index_count);

        memcpy(f_ms->verts, def_floor_verts, vertex_count * 3 * sizeof(float));
        memcpy(f_ms->uvs, def_uvs, vertex_count * 2 * sizeof(float));
        memcpy(f_ms->norms, def_norms, vertex_count * 3 * sizeof(float));

        memcpy(w_ms->verts, def_wall_verts, vertex_count * 3 * sizeof(float));
        memcpy(w_ms->uvs, def_uvs, vertex_count * 2 * sizeof(float));
        memcpy(w_ms->norms, def_norms, vertex_count * 3 * sizeof(float));

        for (j = 0; j < 6; j++) {
            idx = f_ms->indices + j * 6;
            idx[0] = def_indices[0] + j * 4;
            idx[1] = def_indices[1] + j * 4;
            idx[2] = def_indices[2] + j * 4;
            idx[3] = def_indices[3] + j * 4;
            idx[4] = def_indices[4] + j * 4;
            idx[5] = def_indices[5] + j * 4;
        }

        for (j = 0; j < 6; j++) {
            idx = w_ms->indices + j * 6;
            idx[0] = def_indices[0] + j * 4;
            idx[1] = def_indices[1] + j * 4;
            idx[2] = def_indices[2] + j * 4;
            idx[3] = def_indices[3] + j * 4;
            idx[4] = def_indices[4] + j * 4;
            idx[5] = def_indices[5] + j * 4;
        }
        f_ms->index_highest = vertex_count;
        w_ms->index_highest = vertex_count;
        tile_mesh_calctans(f_ms);
        tile_mesh_calctans(w_ms);
    }
    f_mat = tile_db.mats + 1;
    w_mat = tile_db.mats + 2;
 
    for (i = 0; i < TILE_TEXTURE_TOTAL; i++) {
        f_color = *(Color *)(def_floor_colors + i * 4);
        w_color = *(Color *)(def_wall_colors + i * 4);
        f_img = atra_image_gencolor(f_color, 1, 1);
        w_img = atra_image_gencolor(w_color, 1, 1);
        f_mat->maps[i] = atra_texture_loadimage(&f_img);
        w_mat->maps[i] = atra_texture_loadimage(&w_img);
        atra_image_destroy(&f_img);
        atra_image_destroy(&w_img);
    }
    tile_db.flags[1] = 0; 
    tile_db.flags[2] = TILE_FLAG_COLLIDABLE; 

    tile_db.size += 2;
}

/* Load tile data from a .gltf file into a tile mesh data entry (single mesh entry) */
void
db_tile_load_gltf(const char *path, uint tile_id, uint tile_mesh_type)
{
    /* TODO: Add code pathway to allow dev to convert .gltf to .asf */
    Model model = atra_model_loadfile(path, 1);
    TileMesh *t_mesh;
    Mesh *mesh;
    uint max, i;

    uint vert_count;
    uint index_count; 

    while (tile_db.size >= tile_db.cap) {
        db_tile_grow();
    }
    mesh = model.meshes;
    t_mesh = tile_db.meshes + tile_mesh_type + tile_id * TILE_MESH_TOTAL;

    if (t_mesh->verts != NULL) {
        tile_mesh_delete(t_mesh);
    }
    vert_count = mesh->vert_count;
    index_count = mesh->tri_count * 3;

    *t_mesh = tile_mesh_create(vert_count, index_count);

    memcpy(t_mesh->verts, mesh->verts, vert_count * 3 * sizeof(float));
    memcpy(t_mesh->uvs, mesh->uvs, vert_count * 2 * sizeof(float));
    memcpy(t_mesh->norms, mesh->norms, vert_count * 3 * sizeof(float));
    memcpy(t_mesh->tans, mesh->tangents, vert_count * 3 * sizeof(float));
    memcpy(t_mesh->indices, mesh->indices, index_count * sizeof(uint));

    atra_model_delete(&model);

    for (i = 0, max = 0; i < index_count; i++) {
        max = MAX(t_mesh->indices[i], max);
    }
    t_mesh->index_highest = max;
}

/* Load tile mesh and texture data from a .asf file (full mesh entry data) */
void
db_tile_load_asf(const char *path)
{
    FileData file;
    TileMesh *meshes;
    TileMat *mat;
    Option_Texture tex;
    u32 *flags;
    char header[8];
    size_t str_len;
    char *str;
    uint i;

    while (tile_db.size >= tile_db.cap) {
        db_tile_grow();
    }
    file = file_load(path);
    meshes = db_tile_getmesh(tile_db.size, 0);
    mat = db_tile_getmaterial(tile_db.size);
    flags = &tile_db.flags[tile_db.size];
    tile_db.size++;

    /* TODO: Add decompression to file module */

    file_read_char_array(&file, 8, header);

    if (memcmp(header, "ASF:TILE", 8 * sizeof(char))) {
        atra_log_print(LOG_WARN, "tile: Unable to load .asf file: %s: Header code mismatch", path);
        return;
    }
    *flags = file_read_u32(&file);
    
    for (i = 0; i < TILE_MESH_TOTAL; i++) {
        TileMesh * const mesh = meshes + i;
        const uint vert_count = file_read_uint(&file);
        const uint index_count = file_read_uint(&file);        
        mesh->index_highest = file_read_uint(&file);

        tile_mesh_create(vert_count, index_count);

        file_read_float_array(&file, vert_count * 3, mesh->verts);
        file_read_float_array(&file, vert_count * 2, mesh->uvs);
        file_read_float_array(&file, vert_count * 3, mesh->norms);
        file_read_float_array(&file, vert_count * 3, mesh->tans);
        file_read_uint_array(&file, index_count, mesh->indices);
    }

    for (i = 0; i < TILE_TEXTURE_TOTAL; i++) {
        str_len = file_read_ulong(&file);
        str = malloc(str_len * sizeof(char));
        file_read_char_array(&file, str_len, str);
        
        tex = atra_texture_loadfile(str);
        mat->maps[i] = tex.some;

        free(str);
    }
    file_delete(file);
}

/* Save tile mesh and texture data to a .asf file (DEVELOPER ONLY) */
void
db_tile_save_asf(const char *path, uint tile_id, const char *albedo_path,
        const char *normal_path, const char *specular_path, const char *emission_path)
{
    /* TODO: Rewrite to fit a custom mesh type */
    /* TODO: Change tile mesh to custom mesh type */
    /* TODO: Change Atra Engine mesh to use index count rather than triangle count */

    const size_t albedo_len = strlen(albedo_path);
    const size_t normal_len = strlen(normal_path);
    const size_t specular_len = strlen(specular_path);
    const size_t emission_len = strlen(emission_path);

    FileData file;
    TileMesh *meshes;
    uint i;

    file = file_create(0);
    meshes = db_tile_getmesh(tile_id, 0);

    file_write_char_array(&file, 8, "ASF:TILE");
    file_write_u32(&file, tile_db.flags[tile_id]);
    
    for (i = 0; i < TILE_MESH_TOTAL; i++) {
        const TileMesh * const mesh = meshes + i;
        const uint vert_count = mesh->vert_count;
        const uint index_count = mesh->index_count;

        file_write_uint(&file, vert_count);
        file_write_uint(&file, index_count);
        file_write_uint(&file, mesh->index_highest);
        file_write_float_array(&file, vert_count * 3, mesh->verts);
        file_write_float_array(&file, vert_count * 2, mesh->uvs);
        file_write_float_array(&file, vert_count * 3, mesh->norms);
        file_write_float_array(&file, vert_count * 3, mesh->tans);
        file_write_uint_array(&file, index_count, mesh->indices);
    }
    file_write_ulong(&file, albedo_len);
    file_write_char_array(&file, albedo_len, albedo_path);
    file_write_ulong(&file, normal_len);
    file_write_char_array(&file, normal_len, normal_path);
    file_write_ulong(&file, specular_len);
    file_write_char_array(&file, specular_len, specular_path);
    file_write_ulong(&file, emission_len);
    file_write_char_array(&file, emission_len, emission_path);

    /* TODO: Add compression to file module */

    file_save(&file, path);
    file_delete(file);
}

/* Returns the mesh data of a specific tile type's mesh entry */
TileMesh *
db_tile_getmesh(uint tile_id, uint tile_mesh_type)
{
    return tile_db.meshes + tile_mesh_type + tile_id * TILE_MESH_TOTAL;
}

/* Returns the material of a specific tile type */
TileMat *
db_tile_getmaterial(uint tile_id)
{
    return tile_db.mats + tile_id;
}

/* Returns the number of entries in the database */
uint
db_tile_getsize(void)
{
    return tile_db.size;
}

static void
db_tile_grow(void)
{
    tile_db.cap *= 2;
    tile_db.meshes = realloc(tile_db.meshes, tile_db.cap * TILE_MESH_TOTAL * sizeof(TileMesh));
    tile_db.mats = realloc(tile_db.mats, tile_db.cap * sizeof(TileMat));
    tile_db.flags = realloc(tile_db.flags, tile_db.cap * sizeof(u32));
}


/* Creates a new tile chunk with given chunk-space coordinate position */
TileChunk *
tile_chunk_create(void)
{
    TileChunk *t = calloc(1, sizeof(TileChunk));

    t->mesh_count = 0;
    t->mesh_cap = TILE_CHUNK_DEF_INST_CAP;
    t->mesh_tile_ids = malloc(t->mesh_cap * sizeof(uint));
    t->meshes = malloc(t->mesh_cap * sizeof(TileMesh));
    t->dirty = 1;

    return t;
}

/* Delete an existing tile chunk */
void
tile_chunk_delete(TileChunk *t)
{
    uint i;

    for (i = 0; i < t->mesh_count; i++) {
        tile_mesh_delete(t->meshes + i);
    }
    free(t->mesh_tile_ids);
    free(t->meshes);
}

/* Set the neighbors of the tile chunk */
void
tile_chunk_setneighbors(TileChunk *t, TileChunk **neighbors)
{
    memcpy(t->neighbors, neighbors, TILE_NEIGHBOR_TOTAL * sizeof(TileChunk *));
}

/* Safely fetches the neighbor tile flag, returning non-zero if `tile` and
 * neighbor tile match.  Result is shifted by `n_type` to correspond with the mesh type */
static uint
tile_chunk_getneighborflag(TileChunk **neigh, uint tile, uint n_type, uint x, uint z)
{
    const uint index = x + z * TILE_CHUNK_SIZE;

    if (neigh[n_type] == NULL) {
        return 0;
    }
    
    return (neigh[n_type]->tiles[index] == tile) << n_type;
}

/* Populate tile chunk mesh and uploads completed mesh */
void
tile_chunk_mesh(TileChunk *t)
{
    uint mesh_type;
    uint found;
    uint i, j, k;

    #define getneighflag(dir, u, v) \
            tile_chunk_getneighborflag(t->neighbors, tile, dir, u, v)
    #define getchunkflag(dir, idx) \
            ((t->tiles[idx] == tile) << dir)

    /* TODO: Optimisation opportunity: Rework meshing to detect which meshes
     *       require updating, with regards to which tiles have been changed */

    /* Cancel meshing if tile chunk does not require remesh */
    if (t->dirty == 0) {
        return;
    }

    /* Reset the chunk meshes */
    for (i = 0; i < t->mesh_count; i++) {
        tile_mesh_delete(t->meshes + i);
    }
    t->mesh_count = 0;

    for (i = 0; i < TILE_CHUNK_SIZE; i++) {
        for (j = 0; j < TILE_CHUNK_SIZE; j++) {
            const uint index = j + i * TILE_CHUNK_SIZE;
            const uint tile = t->tiles[index];
            
            if (tile == 0) {
                continue;
            }
            mesh_type = 0;

            /* Fetch the values of neighboring tiles */
            if (i == 0) {
                mesh_type |= getchunkflag(TILE_NEIGHBOR_UP, index + TILE_CHUNK_SIZE);
                mesh_type |= getneighflag(TILE_NEIGHBOR_DOWN, j, TILE_CHUNK_SIZE - 1);
            } else if (i == TILE_CHUNK_SIZE - 1) {
                mesh_type |= getneighflag(TILE_NEIGHBOR_UP, j, 0);
                mesh_type |= getchunkflag(TILE_NEIGHBOR_DOWN, index - TILE_CHUNK_SIZE);
            } else {
                mesh_type |= getchunkflag(TILE_NEIGHBOR_UP, index + TILE_CHUNK_SIZE);
                mesh_type |= getchunkflag(TILE_NEIGHBOR_DOWN, index - TILE_CHUNK_SIZE);
            }

            if (j == 0) {
                mesh_type |= getneighflag(TILE_NEIGHBOR_LEFT, TILE_CHUNK_SIZE - 1, i);
                mesh_type |= getchunkflag(TILE_NEIGHBOR_RIGHT, index + 1);
            } else if (j == TILE_CHUNK_SIZE - 1) {
                mesh_type |= getchunkflag(TILE_NEIGHBOR_LEFT, index - 1);
                mesh_type |= getneighflag(TILE_NEIGHBOR_RIGHT, 0, i);
            } else {
                mesh_type |= getchunkflag(TILE_NEIGHBOR_LEFT, index - 1);
                mesh_type |= getchunkflag(TILE_NEIGHBOR_RIGHT, index + 1);
            }
            found = 0;

            /* Find the matching tile mesh index, if it exists */
            for (k = 0; k < t->mesh_count; k++) {
                if (t->mesh_tile_ids[k] == tile) {
                    found = 1;
                    break;
                }
            }

            /* Push mesh data from database to tile chunk */
            const uint mesh_index = mesh_type + tile * TILE_MESH_TOTAL;
            const TileMesh * const src = tile_db.meshes + mesh_index;
            TileMesh * const dst = t->meshes + k;

            if (found == 0) {
                while (t->mesh_count >= t->mesh_cap) {
                    tile_chunk_grow(t);
                }
                *dst = tile_mesh_create(0, 0);
                t->mesh_tile_ids[k] = tile;
                t->mesh_count++;
            }
            tile_mesh_add(dst, src, j, i);
        }
    }

    /* Upload the tile meshes to the GPU */ 
    for (i = 0; i < t->mesh_count; i++) {
        tile_mesh_upload(t->meshes + i);
    }
    t->dirty = 0;
}

/* Render the tile chunk to the screen */
void
tile_chunk_draw(TileChunk *t, Shader *shader, mat4 model_mat, mat4 projview, int shader_mode)
{
    uint i, j;
    uint tile_id;
    mat4 mvp;

    Shader *depth_shader = state_suncam_getshader();

    glm_mat4_mul(projview, model_mat, mvp);

    if (shader_mode == 0) {
        atra_shader_setuniform(shader->locs[TILE_LOCS_MAT_MODEL], model_mat,
                ATRA_SHADER_UNIFORM_MAT4, 1);
        atra_shader_setuniform(shader->locs[TILE_LOCS_MAT_MVP], mvp,
                ATRA_SHADER_UNIFORM_MAT4, 1);
    } else {
        atra_shader_setuniform(depth_shader->locs[DEPTH_LOC_MAT_MODEL], model_mat,
                ATRA_SHADER_UNIFORM_MAT4, 1);
    }

    for (i = 0; i < t->mesh_count; i++) {
        tile_id = t->mesh_tile_ids[i];

        if (shader_mode == 0) {
            for (j = 0; j < TILE_TEXTURE_TOTAL; j++) {
                atra_gl_tex_setactiveunit(j);
                atra_gl_tex_enable(tile_db.mats[tile_id].maps[j].id);
                atra_shader_setuniform(shader->locs[j], &j, ATRA_SHADER_UNIFORM_INT, 1);
            }
        }
        atra_gl_vao_enable(t->meshes[i].vao_id);
        atra_gl_vao_drawelements(0, t->meshes[i].index_count, NULL);
    }
}

/* Sets the value of a tile at the given local chunk coordinate */
void
tile_chunk_settile(TileChunk *t, uint x, uint z, uint tile_id)
{
    const uint index = x + z * TILE_CHUNK_SIZE;
    t->tiles[index] = tile_id;
}

/* Return the flags for the tile type at given chunk coordinate */
u32
tile_chunk_getflags(TileChunk *t, uint x, uint z)
{
    return tile_db.flags[t->tiles[x + z * TILE_CHUNK_SIZE]];
}

/* Increases the size of the mesh instance arrays in the tile chunk */
static void
tile_chunk_grow(TileChunk *t)
{
    t->mesh_cap *= 2;
    t->mesh_tile_ids = realloc(t->mesh_tile_ids, t->mesh_cap * sizeof(uint));
    t->meshes = realloc(t->meshes, t->mesh_cap * sizeof(TileMesh));
}

/* Initialise a new, empty tile mesh */
TileMesh
tile_mesh_create(uint vert_count, uint index_count)
{
    const uint vert_cap = MAX(TILE_MESH_DEF_SIZE, vert_count);
    const uint index_cap = MAX(TILE_MESH_DEF_SIZE, index_count);

    TileMesh mesh = {0};

    mesh.vert_count = vert_count;
    mesh.vert_cap = vert_cap;
    mesh.index_count = index_count;
    mesh.index_cap = index_cap;

    mesh.verts = malloc(vert_cap * 3 * sizeof(float));
    mesh.uvs = malloc(vert_cap * 2 * sizeof(float));
    mesh.norms = malloc(vert_cap * 3 * sizeof(float));
    mesh.tans = malloc(vert_cap * 3 * sizeof(float));
    mesh.indices = malloc(index_cap * sizeof(uint));

    return mesh;
}

/* Delete an existing tile mesh */
void
tile_mesh_delete(TileMesh *mesh)
{
    uint i;

    if (mesh->vao_id != 0) {
        atra_gl_vao_delete(mesh->vao_id);
    
        for (i = 0; i < TILE_MESH_VBO_TOTAL; i++) {
            atra_gl_vbo_delete(mesh->vbo_ids[i]);
        }
    }
    free(mesh->verts);
    free(mesh->uvs);
    free(mesh->norms);
    free(mesh->tans);
    free(mesh->indices);
}

static void
tile_mesh_initialupload(TileMesh *mesh)
{
    mesh->vao_id = atra_gl_vao_create();
    atra_gl_vao_enable(mesh->vao_id);

    mesh->vbo_ids[0] = atra_gl_vbo_create(mesh->verts, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(0, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(0);

    mesh->vbo_ids[1] = atra_gl_vbo_create(mesh->uvs, mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vao_setattrib(1, 2, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(1);

    mesh->vbo_ids[2] = atra_gl_vbo_create(mesh->norms, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(2, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(2);

    mesh->vbo_ids[3] = atra_gl_vbo_create(mesh->tans, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(4, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(4);

    mesh->vbo_ids[4] = atra_gl_ebo_create(mesh->indices, mesh->index_count * sizeof(uint), 0);
}

static void
tile_mesh_overwrite(TileMesh *mesh)
{
    atra_gl_vao_enable(mesh->vao_id);

    atra_gl_vbo_overwrite(mesh->vbo_ids[0], mesh->verts,
            mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vbo_overwrite(mesh->vbo_ids[1], mesh->uvs,
            mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vbo_overwrite(mesh->vbo_ids[2], mesh->norms,
            mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vbo_overwrite(mesh->vbo_ids[3], mesh->tans,
            mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_ebo_overwrite(mesh->vbo_ids[4], mesh->indices,
            mesh->index_count * sizeof(uint), 0);
}

static void
tile_mesh_calctans(TileMesh *mesh)
{
    const uint ic = mesh->index_count;
    uint * const is = mesh->indices;
    float * const vs = mesh->verts;
    float * const uvs = mesh->uvs;
    float * const tans = mesh->tans;

    uint i;
    
    for (i = 0; i < ic; i += 3) {
        const uint id[3] = {
            is[i + 0],
            is[i + 1],
            is[i + 2]
        };
        float * const p[3] = {
            vs + (id[0] * 3),
            vs + (id[1] * 3),
            vs + (id[2] * 3)
        };
        float * const t[3] = {
            uvs + (id[0] * 2),
            uvs + (id[1] * 2),
            uvs + (id[2] * 2),
        };

        vec3 pd[2];
        vec2 td[2];
        vec3 tan;

        glm_vec3_sub(p[1], p[0], pd[0]);
        glm_vec3_sub(p[2], p[0], pd[1]);
        glm_vec2_sub(t[1], t[0], td[0]);
        glm_vec2_sub(t[2], t[0], td[1]);

        const float f = 1.0f / (td[0][0] * td[1][1] - td[1][0] * td[0][1]);

        tan[0] = f * (td[1][1] * pd[0][0] - td[0][1] * pd[1][0]);
        tan[1] = f * (td[1][1] * pd[0][1] - td[0][1] * pd[1][1]);
        tan[2] = f * (td[1][1] * pd[0][2] - td[0][1] * pd[1][2]);

        memcpy(tans + (id[0] * 3), tan, sizeof(vec3));
        memcpy(tans + (id[1] * 3), tan, sizeof(vec3));
        memcpy(tans + (id[2] * 3), tan, sizeof(vec3));
    }
}

/* Upload mesh data to the GPU (should only be called once per mesh) */
void
tile_mesh_upload(TileMesh *mesh)
{
    if (mesh->vao_id == 0) {
        tile_mesh_initialupload(mesh);
    } else {
        tile_mesh_overwrite(mesh);
    }
}

/* Render a given tile chunk mesh */
void
tile_mesh_draw(TileMesh *mesh, TileMat *mat, Shader *shader)
{
    uint i;

    for (i = 0; i < TILE_TEXTURE_TOTAL; i++) {
        const int tex_id = mat->maps[i].id;
        const int tex_loc = shader->locs[i];
        
        atra_gl_tex_setactiveunit(i);
        atra_gl_tex_enable(tex_id);
        atra_shader_setuniform(tex_loc, &tex_id, ATRA_SHADER_UNIFORM_INT, 1);
    }
    atra_gl_vao_enable(mesh->vao_id);
    atra_gl_vao_drawelements(0, mesh->index_count, NULL);
}

static void
tile_mesh_grow(TileMesh *mesh)
{
    mesh->vert_cap *= 2;
    mesh->index_cap *= 2;

    mesh->verts = realloc(mesh->verts, mesh->vert_cap * 3 * sizeof(float));
    mesh->uvs = realloc(mesh->uvs, mesh->vert_cap * 2 * sizeof(float));
    mesh->norms = realloc(mesh->norms, mesh->vert_cap * 3 * sizeof(float));
    mesh->tans = realloc(mesh->tans, mesh->vert_cap * 3 * sizeof(float));
    mesh->indices = realloc(mesh->indices, mesh->index_cap * sizeof(uint));
}

static void
tile_mesh_add(TileMesh *dst, const TileMesh *src, uint x, uint z)
{
    uint i;

    while ((dst->vert_count + src->vert_count > dst->vert_cap) ||
            (dst->index_count + src->index_count > dst->index_cap)) {
        tile_mesh_grow(dst);
    }

    for (i = 0; i < src->vert_count; i++) {
        const float * const src_vert = src->verts + i * 3;
        float * const dst_vert = dst->verts + (dst->vert_count + i) * 3;

        dst_vert[0] = src_vert[0] + x;
        dst_vert[1] = src_vert[1];
        dst_vert[2] = src_vert[2] - z;
    }
    memcpy(dst->uvs + dst->vert_count * 2, src->uvs, src->vert_count * 2 * sizeof(float));
    memcpy(dst->norms + dst->vert_count * 3, src->norms, src->vert_count * 3 * sizeof(float));
    memcpy(dst->tans + dst->vert_count * 3, src->tans, src->vert_count * 3 * sizeof(float));

    for (i = 0; i < src->index_count; i++) {
        const uint * const src_idx = src->indices + i;
        uint * const dst_idx = dst->indices + dst->index_count + i;

        *dst_idx = *src_idx + dst->index_highest;
    }
    dst->vert_count += src->vert_count;
    dst->index_count += src->index_count;
    dst->index_highest += src->index_highest;
}
