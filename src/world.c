/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  world.c - Functions and structures relating to the game world
 * 
 */

#include "world.h"
#include "file.h"
#include "state_game.h"

static const u8 terrain_img_colors[] = {
        0x91, 0x9B, 0x33, 0xFF,     /* Default Albedo */
        0x7F, 0x7F, 0xFF, 0xFF,     /* Default Normal */
        0x00, 0x00, 0x00, 0xFF,     /* Default Specular */
        0x00, 0x00, 0x00, 0xFF,     /* Default Emission */
        0x44, 0x25, 0x13, 0xFF,     /* Damaged Albedo */
        0x7F, 0x7F, 0xFF, 0xFF,     /* Damaged Normal */
        0x00, 0x00, 0x00, 0xFF,     /* Damaged Specular */
        0x00, 0x00, 0x00, 0xFF,     /* Damaged Emission */
};

static void world_shaders_init(World *world);
static void world_chunk_flattenatwall(Chunk *chunk);
static size_t ivec2_hash(const void *a);
static int ivec2_eq(const void *a, const void *b);

/* Creates and returns an empty, default world */
World
world_create_default(void)
{
    World w = {0};
    MaterialMap * const terrain_mats = w.terrain_mat.maps;

    Image img;
    uint i, j;

    w.sun_enabled = 1;
    w.sun_color = (Color){0xFF, 0xDD, 0xAA, 0xFF};
    w.sun_rot[0] = DEG_TO_RAD(-45);
    w.sun_rot[1] = DEG_TO_RAD(70);
    w.sun_rot[2] = 0.0f;

    w.ambient_light = (Color){0x10, 0x20, 0x30, 0xFF};
    w.chunks = atra_hashmap_create(sizeof(int *), sizeof(Chunk),
            ivec2_hash, ivec2_eq);

    world_shaders_init(&w);

    for (i = 0; i < TERRAIN_TEX_TOTAL; i++) {
        const Color * const color = (Color *)(terrain_img_colors + i * 4);
        img = atra_image_gencolor(*color, 1, 1);

        terrain_mats[i].tex = atra_texture_loadimage(&img);
        terrain_mats[i].color = (Color){0xFF, 0xFF, 0xFF, 0xFF};
        terrain_mats[i].value = 1.0f;
        
        atra_image_destroy(&img);
    }
    db_tile_load_defaults();

    /* TODO: Construct a set of default chunks and add them to the world */
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            //Image h_map = atra_image_genperlinnoise(64, 64, 64 * j, 64 * i, 2.0f);
            Chunk * const ch = world_addchunk(&w, j, i, NULL, NULL);
            ch->tile_chunk->tiles[7 + 7 * TILE_CHUNK_SIZE] = 2;
            ch->tile_chunk->tiles[8 + 7 * TILE_CHUNK_SIZE] = 2;
            ch->tile_chunk->tiles[7 + 8 * TILE_CHUNK_SIZE] = 2;
            ch->tile_chunk->tiles[8 + 8 * TILE_CHUNK_SIZE] = 2;

            ch->tile_chunk->tiles[6 + 6 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[7 + 6 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[8 + 6 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[9 + 6 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[6 + 7 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[9 + 7 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[6 + 8 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[9 + 8 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[6 + 9 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[7 + 9 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[8 + 9 * TILE_CHUNK_SIZE] = 1;
            ch->tile_chunk->tiles[9 + 9 * TILE_CHUNK_SIZE] = 1;
        }
    }
    world_calcneighbors(&w);
    world_makecrater(&w, 8.0f, 16.0f, 16.0f);
    world_makecrater(&w, 3.0f, 12.0f, 12.0f);

    return w;
}

/* Completely unloads the world and all it's components (also resets the tile database) */
void
world_delete(World *world)
{
    Chunk *chunk;
    int **pos;
    uint i;

    for (i = 0; i < TERRAIN_TEX_TOTAL; i++) {
        atra_texture_destroy(&world->terrain_mat.maps[i].tex);
    }
    atra_shader_destroy(&world->terrain_mat.shader);
    atra_shader_destroy(&world->tile_shader);

    for (i = 0; i < world->chunks.size; i++) {
        pos = atra_hashmap_getkey(&world->chunks, i);

        if (*pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);
        world_chunk_delete(chunk);
    }
    atra_hashmap_delete(&world->chunks, HASH_CHUNK_FREEMODE);
    db_tile_reset();
}

/* Loads the world and all related assets from a given world file */
World
world_load(const char *path)
{
    /* TODO: Add actor loading to the file standard after actor system is made */

    uint tl_num;
    uint ck_num;
    char head[8];
    char f_path[PATH_MAX];

    FileData file;
    Chunk *ck;
    TileChunk *t_ck;
    Option_Texture tex;
    ivec2 pos;
    uint path_len;
    uint i;

    Image height = {NULL, HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE, 1};
    Image blend = {NULL, BLEND_MAP_SIZE, BLEND_MAP_SIZE, 1};

    World w = {0};

    w.chunks = atra_hashmap_create(sizeof(int *), sizeof(Chunk),
            ivec2_hash, ivec2_eq);

    file = file_load(path);

    file_read_char_array(&file, 8, head);
    
    if (memcmp(head, "ASF:TILE", 8 * sizeof(char))) {
        atra_log_print(LOG_WARN, "tile: Unable to load .asf file: %s: Header code mismatch", path);
        return w;
    }
    tl_num = file_read_uint(&file);
    ck_num = file_read_uint(&file);

    for (i = 0; i < PATH_TILES; i++) {
        path_len = file_read_uint(&file);
        file_read_char_array(&file, path_len, f_path);

        tex = atra_texture_loadfile(f_path);
        w.terrain_mat.maps[i].tex = tex.some;
    }
    w.sun_enabled = file_read_int(&file);
    file_read_float_array(&file, 3, w.sun_rot);
    file_read_u8_array(&file, 4, (u8 *) &w.sun_color);
    file_read_u8_array(&file, 4, (u8 *) &w.ambient_light);

    for (i = 0; i < tl_num; i++) {
        path_len = file_read_uint(&file);
        file_read_char_array(&file, path_len, f_path);
        db_tile_load_asf(f_path);
    }
    world_shaders_init(&w);

    for (i = 0; i < ck_num; i++) {
        file_read_int_array(&file, 2, pos);

        height.data = malloc(HEIGHT_MAP_SIZE_POW2 * sizeof(u32));
        blend.data = malloc(BLEND_MAP_SIZE_POW2 * sizeof(u32));

        file_read_u32_array(&file, HEIGHT_MAP_SIZE_POW2, height.data);
        file_read_u32_array(&file, BLEND_MAP_SIZE_POW2, blend.data);

        ck = world_addchunk(&w, pos[0], pos[1], &height, &blend);
        t_ck = ck->tile_chunk;

        file_read_uint_array(&file, TILE_CHUNK_SIZE_POW2, t_ck->tiles);
    }

    return w;
}

/* Saves world data to a world file with the specified path */
void
world_save(World *world, const char *path, const char **file_paths)
{
    const uint tile_count = db_tile_getsize();
    const uint chunk_count = world->chunks.occupied;
    
    FileData file;
    uint path_size;
    uint i;

    int **pos;
    Chunk *chunk;
    TileChunk *t_chunk;
    Image *img;

    /* TODO: Add actor loading to the file standard after actor system is made */

    file = file_create(0);

    file_write_char_array(&file, 8, "ASF:WRLD");
    file_write_uint(&file, tile_count);
    file_write_uint(&file, chunk_count);

    for (i = 0; i < PATH_TILES; i++) {
        path_size = strlen(file_paths[i]) + 1;
        file_write_uint(&file, path_size);
        file_write_char_array(&file, path_size, file_paths[i]);
    }
    file_write_int(&file, world->sun_enabled);
    file_write_float_array(&file, 3, world->sun_rot);
    file_write_u8_array(&file, 4, (u8 *) &world->sun_color);
    file_write_u8_array(&file, 4, (u8 *) &world->ambient_light);

    for (i = 0; i < tile_count; i++) {
        path_size = strlen(file_paths[i + PATH_TILES]) + 1;
        file_write_uint(&file, path_size);
        file_write_char_array(&file, path_size, file_paths[i + PATH_TILES]);
    }

    for (i = 0; i < world->chunks.size; i++) {
        pos = atra_hashmap_getkey(&world->chunks, i);

        if (*pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);
        t_chunk = chunk->tile_chunk;

        file_write_int_array(&file, 2, *pos);
        img = &chunk->terrain_chunk->height_img;
        file_write_u32_array(&file, HEIGHT_MAP_SIZE_POW2, img->data);
        img = &chunk->terrain_chunk->blend_img;
        file_write_u32_array(&file, BLEND_MAP_SIZE_POW2, img->data);
        file_write_uint_array(&file, TILE_CHUNK_SIZE_POW2, t_chunk->tiles);
    }
    file_save(&file, path);
    file_delete(file);
}

/* Add a new chunk to the world */
Chunk *
world_addchunk(World *world, int x, int z, Image *height, Image *blend)
{
    Chunk chunk;
    int *pos;

    pos = malloc(2 * sizeof(int));
    pos[0] = x;
    pos[1] = z;

    chunk = world_chunk_create(height, blend);

    glm_mat4_identity(chunk.model_mat);
    glm_translate_x(chunk.model_mat, x * TILE_CHUNK_SIZE);
    glm_translate_z(chunk.model_mat, -z * TILE_CHUNK_SIZE);

    atra_hashmap_add(&world->chunks, &pos, &chunk);

    return atra_hashmap_get(&world->chunks, &pos);
}

/* Deletes the chunk with the given coordinate, if one exists */
void
world_removechunk(World *world, int x, int z)
{
    const int pos[2] = {x, z};
    const int * const p_pos = pos;

    Chunk *chunk;

    chunk = atra_hashmap_get(&world->chunks, &p_pos);

    if (chunk == NULL) {
        return;
    }
    world_chunk_delete(chunk);
    atra_hashmap_remove(&world->chunks, &p_pos, HASH_CHUNK_FREEMODE);
}

/* Updates the world, remeshing chunks which require it */
void
world_update(World *world)
{
    world_calcmeshes(world);
}

static void
world_shaders_init(World *world)
{
    Shader *shader;
    
    #define getattrid(loc, name) \
            shader->locs[loc] = atra_shader_getattribloc(shader, name)
    #define getlocid(loc, name) \
            shader->locs[loc] = atra_shader_getuniformloc(shader, name)

    /* Initialise terrain shader */
    shader = &world->terrain_mat.shader;
    *shader = atra_shader_loadfile(PATH_TERRAIN_VS, PATH_TERRAIN_FS);

    getattrid(TERRAIN_LOCS_ATR_POSITION, "vert_pos");
    getattrid(TERRAIN_LOCS_ATR_TEXCOORD, "vert_uv");
    getattrid(TERRAIN_LOCS_ATR_TEXCOORD2, "vert_uv2");
    getattrid(TERRAIN_LOCS_ATR_NORMAL, "vert_norm");
    getattrid(TERRAIN_LOCS_ATR_TANGENT, "vert_tan");
    getattrid(TERRAIN_LOCS_ATR_COLOR, "vert_color");

    getlocid(TERRAIN_LOCS_TEX_ALBEDO_DEF, "albedo_def");
    getlocid(TERRAIN_LOCS_TEX_NORMAL_DEF, "normal_def");
    getlocid(TERRAIN_LOCS_TEX_SPECULAR_DEF, "specular_def");
    getlocid(TERRAIN_LOCS_TEX_EMISSION_DEF, "emission_def");
    getlocid(TERRAIN_LOCS_TEX_ALBEDO_DMG, "albedo_dmg");
    getlocid(TERRAIN_LOCS_TEX_NORMAL_DMG, "normal_dmg");
    getlocid(TERRAIN_LOCS_TEX_SPECULAR_DMG, "specular_dmg");
    getlocid(TERRAIN_LOCS_TEX_EMISSION_DMG, "emission_dmg");

    getlocid(TERRAIN_LOCS_TEX_BLENDMAP, "blend_map");
    getlocid(TERRAIN_LOCS_TEX_SHADOWMAP, "shadow_map");

    getlocid(TERRAIN_LOCS_VEC_VIEWPOS, "view_pos");
    getlocid(TERRAIN_LOCS_VEC_AMBIENTCOLOR, "ambient_color");
    getlocid(TERRAIN_LOCS_VEC_SUNCOLOR, "sun_color");
    getlocid(TERRAIN_LOCS_VEC_SUNDIR, "sun_dir");

    getlocid(TERRAIN_LOCS_MAT_MODEL, "model_mat");
    getlocid(TERRAIN_LOCS_MAT_VIEW, "view_mat");
    getlocid(TERRAIN_LOCS_MAT_PROJ, "proj_mat");
    getlocid(TERRAIN_LOCS_MAT_MVP, "mvp");
    getlocid(TERRAIN_LOCS_MAT_LIGHTSPACE, "lightspace");

    /* Initialise tile chunk shader */
    shader = &world->tile_shader;
    *shader = atra_shader_loadfile(PATH_TILECHUNK_VS, PATH_TILECHUNK_FS);

    getlocid(TILE_LOCS_TEX_ALBEDO, "albedo_map");
    getlocid(TILE_LOCS_TEX_NORMAL, "normal_map");
    getlocid(TILE_LOCS_TEX_SPECULAR, "specular_map");
    getlocid(TILE_LOCS_TEX_EMISSION, "emission_map");
    getlocid(TILE_LOCS_TEX_SHADOWMAP, "shadow_map");

    getlocid(TILE_LOCS_VEC_VIEWPOS, "view_pos");
    getlocid(TILE_LOCS_VEC_AMBIENTCOLOR, "ambient_color");
    getlocid(TILE_LOCS_VEC_SUNCOLOR, "sun_color");
    getlocid(TILE_LOCS_VEC_SUNDIR, "sun_dir");

    getlocid(TILE_LOCS_MAT_MODEL, "model_mat");
    getlocid(TILE_LOCS_MAT_VIEW, "view_mat");
    getlocid(TILE_LOCS_MAT_PROJ, "proj_mat");
    getlocid(TILE_LOCS_MAT_MVP, "mvp");
    getlocid(TILE_LOCS_MAT_LIGHTSPACE, "lightspace");
}

/* Calculates the meshes for all chunks in world which require it */
void
world_calcmeshes(World *world)
{
    uint i;
    int **chunk_pos;
    Chunk *chunk;

    /* Calculate meshes */
    for (i = 0; i < world->chunks.size; i++) {
        chunk_pos = atra_hashmap_getkey(&world->chunks, i);

        if (*chunk_pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);

        world_chunk_flattenatwall(chunk);
        world_chunk_calcmeshes(chunk, world);
    }

    /* Second meshing pass for terrain */
    for (i = 0; i < world->chunks.size; i++) {
        chunk_pos = atra_hashmap_getkey(&world->chunks, i);

        if (*chunk_pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);

        terrain_calcnorms(chunk->terrain_chunk);
        terrain_upload(chunk->terrain_chunk, &world->terrain_mat);
    }
}

/* Renders the scene to a depth map from the sun's perspective */
void
world_drawdepth(World *world)
{
    DirLight * const sunlight = state_suncam_get();

    TileChunk *tile_chunk;
    Chunk *chunk;
    int **chunk_pos;
    int sunspace_loc;
    mat4 sunspace;
    uint i;

    Shader *depth_shader;

    depth_shader = state_suncam_getshader();
    glm_mat4_mul(sunlight->proj, sunlight->view, sunspace);
    atra_shader_setactive(depth_shader);

    /* Set up shader uniforms common to all terrain chunks */
    sunspace_loc = depth_shader->locs[DEPTH_LOC_MAT_LIGHTSPACE];
    atra_shader_setuniform(sunspace_loc, sunspace, ATRA_SHADER_UNIFORM_MAT4, 1);

    /* Draw all terrain chunks */
    for (i = 0; i < world->chunks.size; i++) {
        chunk_pos = atra_hashmap_getkey(&world->chunks, i);

        if (*chunk_pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);
        terrain_draw(chunk->terrain_chunk, NULL, chunk->model_mat, 1);
    }

    /* Draw all tile chunks */
    for (i = 0; i < world->chunks.size; i++) {
        chunk_pos = atra_hashmap_getkey(&world->chunks, i);

        if (*chunk_pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);
        tile_chunk = chunk->tile_chunk;
        tile_chunk_draw(tile_chunk, depth_shader, chunk->model_mat, sunspace, 1);
    }
}

/* Renders the world to the screen */
void
world_draw(World *world)
{
    const int sun_enabled = (world->sun_enabled != 0);

    const vec3 ambient_color = {
        world->ambient_light.r / 255.0f,
        world->ambient_light.g / 255.0f,
        world->ambient_light.b / 255.0f
    };
    const vec3 sun_color = {
        (world->sun_color.r / 255.0f) * sun_enabled,
        (world->sun_color.g / 255.0f) * sun_enabled,
        (world->sun_color.b / 255.0f) * sun_enabled
    };

    TileChunk *tile_chunk;
    Chunk *chunk;
    int **chunk_pos;
    int view_loc, proj_loc, sunspace_loc, depthmap_loc;
    int viewpos_loc, ambient_loc, suncolor_loc, sundir_loc;
    int depthmap_unit;
    mat4 viewproj, sunspace;
    uint i;

    Shader *tile_shader;
    Material *terrain_mat;
    Camera *cam;
    DirLight *sunlight;

    cam = atra_camera_getmain();
    sunlight = state_suncam_get();
    terrain_mat = &world->terrain_mat;
    tile_shader = &world->tile_shader;
    glm_mat4_mul(sunlight->proj, sunlight->view, sunspace);

    /* Set up shader uniforms common to all terrain chunks */
    atra_shader_setactive(&terrain_mat->shader);

    for (i = 0; i < TERRAIN_TEX_TOTAL; i++) {
        const uint tex_index = i + TERRAIN_LOCS_TEX_ALBEDO_DEF;
        const int tex_loc = terrain_mat->shader.locs[tex_index];

        atra_gl_tex_setactiveunit(i);
        atra_gl_tex_enable(terrain_mat->maps[i].tex.id);
        atra_shader_setuniform(tex_loc, &i, ATRA_SHADER_UNIFORM_INT, 1);
    }
    /* Bind the depth map */
    depthmap_unit = TERRAIN_TEX_SHADOWMAP;
    depthmap_loc = terrain_mat->shader.locs[TERRAIN_LOCS_TEX_SHADOWMAP];
    atra_dirlight_binddepthtex(sunlight, depthmap_unit);
    atra_shader_setuniform(depthmap_loc, &depthmap_unit, ATRA_SHADER_UNIFORM_INT, 1);

    /* Set up the shader uniforms */
    view_loc = terrain_mat->shader.locs[TERRAIN_LOCS_MAT_VIEW];
    proj_loc = terrain_mat->shader.locs[TERRAIN_LOCS_MAT_PROJ];
    sunspace_loc = terrain_mat->shader.locs[TERRAIN_LOCS_MAT_LIGHTSPACE];

    viewpos_loc = terrain_mat->shader.locs[TERRAIN_LOCS_VEC_VIEWPOS];
    ambient_loc = terrain_mat->shader.locs[TERRAIN_LOCS_VEC_AMBIENTCOLOR];
    suncolor_loc = terrain_mat->shader.locs[TERRAIN_LOCS_VEC_SUNCOLOR];
    sundir_loc = terrain_mat->shader.locs[TERRAIN_LOCS_VEC_SUNDIR];

    atra_shader_setuniform(viewpos_loc, cam->pos, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(ambient_loc, ambient_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(suncolor_loc, sun_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(sundir_loc, sunlight->dir, ATRA_SHADER_UNIFORM_VEC3, 1);

    atra_shader_setuniform(view_loc, cam->view, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(proj_loc, cam->proj, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(sunspace_loc, sunspace, ATRA_SHADER_UNIFORM_MAT4, 1);

    /* Draw all terrain chunks */
    for (i = 0; i < world->chunks.size; i++) {
        chunk_pos = atra_hashmap_getkey(&world->chunks, i);

        if (*chunk_pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);
        terrain_draw(chunk->terrain_chunk, terrain_mat, chunk->model_mat, 0);
    }


    /* Draw all tile chunks */
    atra_shader_setactive(tile_shader);
    glm_mat4_mul(cam->proj, cam->view, viewproj);

    depthmap_unit = TILE_TEXTURE_SHADOWMAP;
    depthmap_loc = tile_shader->locs[TILE_LOCS_TEX_SHADOWMAP];
    atra_dirlight_binddepthtex(sunlight, depthmap_unit);
    atra_shader_setuniform(depthmap_loc, &depthmap_unit, ATRA_SHADER_UNIFORM_INT, 1);

    view_loc = tile_shader->locs[TILE_LOCS_MAT_VIEW];
    proj_loc = tile_shader->locs[TILE_LOCS_MAT_PROJ];
    sunspace_loc = tile_shader->locs[TILE_LOCS_MAT_LIGHTSPACE];

    viewpos_loc = tile_shader->locs[TILE_LOCS_VEC_VIEWPOS];
    ambient_loc = tile_shader->locs[TILE_LOCS_VEC_AMBIENTCOLOR];
    suncolor_loc = tile_shader->locs[TILE_LOCS_VEC_SUNCOLOR];
    sundir_loc = tile_shader->locs[TILE_LOCS_VEC_SUNDIR];

    atra_shader_setuniform(viewpos_loc, cam->pos, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(ambient_loc, ambient_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(suncolor_loc, sun_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(sundir_loc, sunlight->dir, ATRA_SHADER_UNIFORM_VEC3, 1);

    atra_shader_setuniform(view_loc, cam->view, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(proj_loc, cam->proj, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(sunspace_loc, sunspace, ATRA_SHADER_UNIFORM_MAT4, 1);

    for (i = 0; i < world->chunks.size; i++) {
        chunk_pos = atra_hashmap_getkey(&world->chunks, i);

        if (*chunk_pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);
        tile_chunk = chunk->tile_chunk;
        tile_chunk_draw(tile_chunk, tile_shader, chunk->model_mat, viewproj, 0);
    }
}

/* Returns a pointer to the chunk at given coordinate (x, z) in chunk-space */
Chunk *
world_getchunk(World *world, int x, int z)
{
    const int pos[] = {x, z};
    const int *p_pos = pos;

    Chunk * const chunk = atra_hashmap_get(&world->chunks, &p_pos);

    return chunk;
}

/* Calculate neighbor pointers for each chunk */
void
world_calcneighbors(World *world)
{
    uint i;
    int **chunk_pos;
    Chunk *chunk;
    Chunk *neighs[8];
    Terrain *terrain;
    TileChunk *tile_chunk;

    for (i = 0; i < world->chunks.size; i++) {
        chunk_pos = atra_hashmap_getkey(&world->chunks, i);

        if (*chunk_pos == NULL) {
            continue;
        }
        chunk = atra_hashmap_getval(&world->chunks, i);

        const uint x = (*chunk_pos)[0];
        const uint z = (*chunk_pos)[1];

        neighs[0] = world_getchunk(world, x - 1, z + 1);
        neighs[1] = world_getchunk(world, x + 0, z + 1);
        neighs[2] = world_getchunk(world, x + 1, z + 1);
        neighs[3] = world_getchunk(world, x - 1, z + 0);
        neighs[4] = world_getchunk(world, x + 1, z + 0);
        neighs[5] = world_getchunk(world, x - 1, z - 1);
        neighs[6] = world_getchunk(world, x + 0, z - 1);
        neighs[7] = world_getchunk(world, x + 1, z - 1);


        terrain = chunk->terrain_chunk;
        terrain->neighbors[0] = (neighs[0] != NULL) ? neighs[0]->terrain_chunk : NULL;
        terrain->neighbors[1] = (neighs[1] != NULL) ? neighs[1]->terrain_chunk : NULL;
        terrain->neighbors[2] = (neighs[2] != NULL) ? neighs[2]->terrain_chunk : NULL;
        terrain->neighbors[3] = (neighs[3] != NULL) ? neighs[3]->terrain_chunk : NULL;
        terrain->neighbors[4] = (neighs[4] != NULL) ? neighs[4]->terrain_chunk : NULL;
        terrain->neighbors[5] = (neighs[5] != NULL) ? neighs[5]->terrain_chunk : NULL;
        terrain->neighbors[6] = (neighs[6] != NULL) ? neighs[6]->terrain_chunk : NULL;
        terrain->neighbors[7] = (neighs[7] != NULL) ? neighs[7]->terrain_chunk : NULL;

        tile_chunk = chunk->tile_chunk;
        tile_chunk->neighbors[0] = (neighs[1] != NULL) ? neighs[1]->tile_chunk : NULL;
        tile_chunk->neighbors[1] = (neighs[3] != NULL) ? neighs[3]->tile_chunk : NULL;
        tile_chunk->neighbors[2] = (neighs[4] != NULL) ? neighs[4]->tile_chunk : NULL;
        tile_chunk->neighbors[3] = (neighs[6] != NULL) ? neighs[6]->tile_chunk : NULL;
    }
}

/* Generates a crater at the given (wx, wz) coordinate with the given radius.
 * NOTE: Coordinates are world-space coordinates */
void
world_makecrater(World *world, float radius, float wx, float wz)
{
    const uint ratio = TERRAIN_SIZE / TILE_CHUNK_SIZE;
    const int tx = wx * ratio;
    const int tz = wz * ratio;

    Chunk *chunks[4];
    int cpos[2];
    int pos[2];
    int * const p_cpos = cpos;
    uint i;

    cpos[0] = (tx / TERRAIN_SIZE) - (tx % TERRAIN_SIZE < TERRAIN_SIZE / 2);
    cpos[1] = (tz / TERRAIN_SIZE) - (tz % TERRAIN_SIZE < TERRAIN_SIZE / 2);

    chunks[0] = atra_hashmap_get(&world->chunks, &p_cpos);
    cpos[0]++;
    chunks[1] = atra_hashmap_get(&world->chunks, &p_cpos);
    cpos[0]--;
    cpos[1]++;
    chunks[2] = atra_hashmap_get(&world->chunks, &p_cpos);
    cpos[0]++;
    chunks[3] = atra_hashmap_get(&world->chunks, &p_cpos);

    cpos[0]--;
    cpos[1]--;
    pos[0] = tx - cpos[0] * TERRAIN_SIZE;
    pos[1] = tz - cpos[1] * TERRAIN_SIZE;

    for (i = 0; i < 4; i++) {
        if (chunks[i] == NULL) {
            continue;
        }
        cpos[0] = pos[0] - TERRAIN_SIZE * (i == 1 || i == 3);
        cpos[1] = pos[1] - TERRAIN_SIZE * (i == 2 || i == 3);

        terrain_makecrater(chunks[i]->terrain_chunk, radius, cpos[0], cpos[1]);
    }
}

/* Returns a newly initialised chunk */
Chunk
world_chunk_create(Image *height, Image *blend)
{
    Chunk chunk;

    chunk.terrain_chunk = terrain_create(height, blend);
    chunk.tile_chunk = tile_chunk_create();

    return chunk;
}

/* Deletes an existing world chunk */
void
world_chunk_delete(Chunk *chunk)
{
    terrain_delete(chunk->terrain_chunk);
    tile_chunk_delete(chunk->tile_chunk);
    free(chunk->terrain_chunk);
    free(chunk->tile_chunk);
}

/* Calculates tile and terrain meshes */
void
world_chunk_calcmeshes(Chunk *chunk, World *w)
{
    terrain_calcmesh(chunk->terrain_chunk, chunk->tile_chunk);
    tile_chunk_mesh(chunk->tile_chunk);
}

static void
world_chunk_flattenatwall(Chunk *chunk)
{
    uint * const tiles = chunk->tile_chunk->tiles;
    const uint ratio = TERRAIN_SIZE / TILE_CHUNK_SIZE;
    u8 * const h_map = chunk->terrain_chunk->height_img.data;

    uint x, z;
    int tx, tz;
    uint idx;
    int i, j, found;


    for (z = 0; z < TERRAIN_SIZE; z++) {
        for (x = 0; x < TERRAIN_SIZE; x++) {
            idx = (x + z * TERRAIN_SIZE) * 4;
            found = 0;

            /* Check surroundings for any tiles */
            for (i = -1; i <= 1; i++) {
                for (j = -1; j <= 1; j++) {
                    tx = MIN(MAX(x + j, 0), TERRAIN_SIZE - 1) / ratio;
                    tz = MIN(MAX(z + i, 0), TERRAIN_SIZE - 1) / ratio;

                    found += (tiles[tx + tz * TILE_CHUNK_SIZE] != 0);
                }
            }

            if (found != 0) {
                h_map[idx + 0] = 0x77;
                h_map[idx + 1] = 0x77;
                h_map[idx + 2] = 0x77;
                h_map[idx + 3] = 0xFF;
            }
        }
    }
}

static size_t
ivec2_hash(const void *a)
{
    const int * const va = *((int **) a);

    return (va[0] * 73856093 + va[1] * 19349669);
}

static int
ivec2_eq(const void *a, const void *b)
{
    const int * const va = *((int **) a);
    const int * const vb = *((int **) b);
    
    return (memcmp(va, vb, 2 * sizeof(int)) == 0);
}
