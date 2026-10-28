/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  terrain.c - Functions and structures relating to world terrain
 * 
 */

#include "terrain.h"
#include "state_game.h"

static int terrain_init = 0;
static Image height_default;
static Image blend_default;

static void terrain_initialise(void);
static void terrain_getneighnorm(Terrain *t, float *norm, uint neigh_id, uint x, uint z);
static void terrain_mesh_calctangents(TerrainMesh *mesh);
static void terrain_mesh_initialupload(TerrainMesh *mesh, Material *mat);
static void terrain_mesh_overwritevao(TerrainMesh *mesh);
static float terrain_mesh_calc_y(Terrain *t, uint x, uint z);
static void terrain_mesh_calcquadnorms(Terrain *t, const uint *indices, float *norms);

static void
terrain_initialise(void)
{
    Image height = atra_image_genperlinnoise(
            HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE, 32, 32, 2.0f);
    
    height_default = height;

    //height_default.width = HEIGHT_MAP_SIZE;
    //height_default.height = HEIGHT_MAP_SIZE;
    //height_default.mipmaps = 1;
    //height_default.data = malloc(HEIGHT_MAP_SIZE_POW2 * sizeof(Color));
    //memset(height_default.data, 0x7F, HEIGHT_MAP_SIZE_POW2 * sizeof(Color));

    blend_default.width = BLEND_MAP_SIZE;
    blend_default.height = BLEND_MAP_SIZE;
    blend_default.mipmaps = 1;
    blend_default.data = malloc(BLEND_MAP_SIZE_POW2 * sizeof(Color));
    memset(blend_default.data, 0, BLEND_MAP_SIZE_POW2 * sizeof(Color));

    terrain_init = 1;
}

/* Creates a new terrain instance.
 * Terrain image data arguments can be NULL to use default maps instead */
Terrain *
terrain_create(Image *height, Image *blend)
{
    Terrain *t = calloc(1, sizeof(Terrain));

    if (terrain_init == 0) {
        terrain_initialise();
    }

    if (height != NULL) {
        t->height_img = *height;
    } else {
        t->height_img = height_default;
    }
    
    if (blend != NULL) {
        t->blend_img = *blend;
    } else {
        t->blend_img = blend_default;
    }
    t->dirty = 1;

    return t;
}

/* Deletes an existing terrain instance */
void
terrain_delete(Terrain *t)
{
    if (t->mesh.vao_id != 0) {
        terrain_mesh_delete(&t->mesh);
    }

    if (memcmp(&t->height_img, &height_default, sizeof(Image)) != 0) {
        atra_image_destroy(&t->height_img);
    }
    
    if (memcmp(&t->blend_img, &blend_default, sizeof(Image)) != 0) {
        atra_image_destroy(&t->blend_img);
    }
    atra_texture_destroy(&t->blend_tex);
}

/* Sets the terrain neighbors */
void
terrain_setneighbors(Terrain *t, Terrain **neighbors)
{
    memcpy(t->neighbors, neighbors, TERRAIN_NEIGHBOR_TOTAL * sizeof(Terrain *));
}

/* Uploads terrain data images to textures, writing them to the GPU.
 * Resolution is locked after creation, subsequent calls update texture data. */
void
terrain_upload_img(Terrain *t)
{
    if (t->blend_tex.id == 0) {
        t->blend_tex = atra_texture_loadimage(&t->blend_img);
        atra_texture_samplebase(&t->blend_tex, ATRA_SAMPLE_LINEAR);
    } else {
        atra_texture_write(&t->blend_tex, t->blend_img.data);
    }
}

/* Checks if the neighbour exists, returns 1 if it does and sets the image pointer */
static int
terrain_mesh_calc_y_setimg(Terrain *neighbor, Image **img)
{
    if (neighbor != NULL) {
        *img = &neighbor->height_img;
        return 1;
    } else {
        *img = NULL;
        return 0;
    }
}

static float
terrain_mesh_calc_y(Terrain *t, uint x, uint z)
{
    /* TODO: Attempt to refactor into branchless maths */
    uint tl[2], tr[2], bl[2], br[2];
    Image *tl_img, *tr_img, *bl_img, *br_img;

    uint sample_count;
    float y = 0.0f;
    
    #define calc_y_setimg(neighbor_type, img) \
            terrain_mesh_calc_y_setimg(t->neighbors[neighbor_type], img)

    tl[0] = (x - 1) % TERRAIN_SIZE;
    tl[1] = (z - 0) % TERRAIN_SIZE;

    tr[0] = (x - 0) % TERRAIN_SIZE;
    tr[1] = (z - 0) % TERRAIN_SIZE;

    bl[0] = (x - 1) % TERRAIN_SIZE;
    bl[1] = (z - 1) % TERRAIN_SIZE;

    br[0] = (x - 0) % TERRAIN_SIZE;
    br[1] = (z - 1) % TERRAIN_SIZE;

    if (x == 0) {
        if (z == 0) {
            sample_count = 1;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_L, &tl_img);
            tr_img = &t->height_img;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_BL, &bl_img);
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_B, &br_img);
        } else if (z == TERRAIN_SIZE) {
            sample_count = 1;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_TL, &tl_img);
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_T, &tr_img);
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_L, &bl_img);
            br_img = &t->height_img;
        } else {
            sample_count = 2;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_L, &tl_img);
            tr_img = &t->height_img;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_L, &bl_img);
            br_img = &t->height_img;
        }
    } else if (x == TERRAIN_SIZE) {
        if (z == 0) {
            sample_count = 1;
            tl_img = &t->height_img;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_R, &tr_img);
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_B, &bl_img);
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_BR, &br_img);
        } else if (z == TERRAIN_SIZE) {
            sample_count = 1;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_T, &tl_img);
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_TR, &tr_img);
            bl_img = &t->height_img;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_R, &br_img);
        } else {
            sample_count = 2;
            tl_img = &t->height_img;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_R, &tr_img);
            bl_img = &t->height_img;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_R, &br_img);
        }
    } else {
        if (z == 0) {
            sample_count = 2;
            tl_img = &t->height_img;
            tr_img = &t->height_img;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_B, &bl_img);
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_B, &br_img);
        } else if (z == TERRAIN_SIZE) {
            sample_count = 2;
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_T, &tl_img);
            sample_count += calc_y_setimg(TERRAIN_NEIGHBOR_T, &tr_img);
            bl_img = &t->height_img;
            br_img = &t->height_img;
        } else {
            sample_count = 4;
            tl_img = &t->height_img;
            tr_img = &t->height_img;
            bl_img = &t->height_img;
            br_img = &t->height_img;
        }
    }

    if (tl_img != NULL) {
        const uint index = tl[0] + tl[1] * TERRAIN_SIZE;
        y += *((u8 *) tl_img->data + index * 4) / 255.0f;
    }

    if (tr_img != NULL) {
        const uint index = tr[0] + tr[1] * TERRAIN_SIZE;
        y += *((u8 *) tr_img->data + index * 4) / 255.0f;
    }

    if (bl_img != NULL) {
        const uint index = bl[0] + bl[1] * TERRAIN_SIZE;
        y += *((u8 *) bl_img->data + index * 4) / 255.0f;
    }

    if (br_img != NULL) {
        const uint index = br[0] + br[1] * TERRAIN_SIZE;
        y += *((u8 *) br_img->data + index * 4) / 255.0f;
    }

    return ((y / sample_count) * 2) - 1;
}

static void
terrain_mesh_calcquadnorms(Terrain *t, const uint *indices, float *norms)
{
    float * const verts = t->mesh.verts;

    float *p1, *p2, *p3;
    vec3 a, b;

    p1 = verts + indices[0] * 3;
    p2 = verts + indices[1] * 3;
    p3 = verts + indices[2] * 3;

    glm_vec3_sub(p2, p1, a);
    glm_vec3_sub(p3, p1, b);
    glm_vec3_crossn(a, b, norms);
    
    p1 = verts + indices[3] * 3;
    p2 = verts + indices[4] * 3;
    p3 = verts + indices[5] * 3;
   
    glm_vec3_sub(p2, p1, a);
    glm_vec3_sub(p3, p1, b);
    glm_vec3_crossn(a, b, norms + 3);
}

/* Calculates the mesh of the terrain */
void
terrain_calcmesh(Terrain *t, TileChunk *tile_chunk)
{
    const float pos_step = 0.25f;
    const float uv_step = 0.0625f;
    const float uv2_step = 0.015625f;

    //uint vert_map[TERRAIN_VERTEX_COUNT] = {TERRAIN_NULL_ID};
    float norm_map[TERRAIN_SIZE_POW2 * 6] = {0};

    uint indices[TERRAIN_QUAD_CORNER_TOTAL];

    uint x, z, c;
    uint vert_count, index_count;

    vert_count = 0;
    index_count = 0;

    /* Reset the index and normal maps */
    for (c = 0; c < TERRAIN_VERTEX_COUNT; c++) {
        t->index_map[c] = TERRAIN_NULL_ID;
    }
    memset(t->normal_buffer, 0, TERRAIN_VERTEX_COUNT * 3 * sizeof(float));

    /* Calculate the terrain information that isn't the normals */
    for (z = 0; z < TERRAIN_SIZE; z++) {
        for (x = 0; x < TERRAIN_SIZE; x++) {
            const uint ratio = TERRAIN_SIZE / TILE_CHUNK_SIZE;
            const uint tile_index = (x / ratio) + (z / ratio) * TILE_CHUNK_SIZE;

            if (tile_chunk->tiles[tile_index] != 0) {
                continue;
            }
            const uint vert_index = x + z * (TERRAIN_SIZE + 1);

            /* Check for existing vertex in all corners, except bottom-right */
            for (c = 0; c < TERRAIN_QUAD_CORNER_TOTAL; c++) {
                const uint offset_x =
                        (c == TERRAIN_QUAD_CORNER_TR) || 
                        (c == TERRAIN_QUAD_CORNER_BR);
                const uint offset_z =
                        (c == TERRAIN_QUAD_CORNER_TL) ||
                        (c == TERRAIN_QUAD_CORNER_TR);
                const uint index =
                        vert_index + offset_x + offset_z * (TERRAIN_SIZE + 1);

                if (t->index_map[index] == TERRAIN_NULL_ID) {
                    /* Create a new vertex and record the index */
                    float * const vert = &t->mesh.verts[vert_count * 3];
                    float * const uv = &t->mesh.uvs[vert_count * 2];
                    float * const uv2 = &t->mesh.uvs2[vert_count * 2];
                    u8 * const color = &t->mesh.colors[vert_count * 4];

                    vert[0] = (x + offset_x) * pos_step;
                    vert[1] = terrain_mesh_calc_y(t, x + offset_x, z + offset_z);
                    vert[2] = (z + offset_z) * -pos_step;

                    uv[0] = (x + offset_x) * uv_step;
                    uv[1] = (z + offset_z) * uv_step;
                        
                    uv2[0] = (x + offset_x) * uv2_step;
                    uv2[1] = (z + offset_z) * uv2_step;
                    
                    color[0] = 0xFF;
                    color[1] = 0xFF;
                    color[2] = 0xFF;
                    color[3] = 0xFF;

                    t->index_map[index] = vert_count;
                    indices[c] = vert_count;
                    vert_count++;
                } else {
                    /* Read the index from the vertex map */
                    indices[c] = t->index_map[index];
                }
            }
            /* Write the indices to the mesh */
            const uint idx[6] = {
                indices[TERRAIN_QUAD_CORNER_TL],
                indices[TERRAIN_QUAD_CORNER_BL],
                indices[TERRAIN_QUAD_CORNER_BR],
                indices[TERRAIN_QUAD_CORNER_BR],
                indices[TERRAIN_QUAD_CORNER_TR],
                indices[TERRAIN_QUAD_CORNER_TL]
            };
            memcpy(t->mesh.indices + index_count, idx, 6 * sizeof(uint));
            index_count += 6;
            
            /* Calculate triangle normals and write to normal buffer */
            const uint norm_index = (x + z * TERRAIN_SIZE) * 6;
            terrain_mesh_calcquadnorms(t, idx, norm_map + norm_index);
        }
    }

    /* Populate normal buffer */
    for (z = 0; z <= TERRAIN_SIZE; z++) {
        for (x = 0; x <= TERRAIN_SIZE; x++) {
            /* Calculate the average normal for each vertex
               and write it to the normal buffer */
            const uint index = x + z * (TERRAIN_SIZE + 1);
            
            if (t->index_map[index] == TERRAIN_NULL_ID) {
                continue;
            }
            const uint tl_off = ((x - 1) + (z - 0) * TERRAIN_SIZE) * 6;
            const uint tr_off = ((x - 0) + (z - 0) * TERRAIN_SIZE) * 6;
            const uint bl_off = ((x - 1) + (z - 1) * TERRAIN_SIZE) * 6;
            const uint br_off = ((x - 0) + (z - 1) * TERRAIN_SIZE) * 6;

            const uint MAX = TERRAIN_SIZE;

            vec3 norm;

            float *tl = norm_map + tl_off + 0;
            float *tr = norm_map + tr_off + 0;
            float *bl = norm_map + bl_off + 3;
            float *br = norm_map + br_off + 0;

            tl = (float *) DECIDE(z == MAX || x == 0, 0, (ulong) tl);
            tr = (float *) DECIDE(z == MAX || x == MAX, 0, (ulong) tr);
            bl = (float *) DECIDE(z == 0 || x == 0, 0, (ulong) bl);
            br = (float *) DECIDE(z == 0 || x == MAX, 0, (ulong) br);

            glm_vec3_zero(norm);

            if (tl != NULL) {
                glm_vec3_add(norm, tl + 0, norm);
                glm_vec3_add(norm, tl + 3, norm);
            }
            if (tr != NULL) {
                glm_vec3_add(norm, tr, norm);
            }
            if (bl != NULL) {
                glm_vec3_add(norm, bl, norm);
            }
            if (br != NULL) {
                glm_vec3_add(norm, br + 0, norm);
                glm_vec3_add(norm, br + 3, norm);
            }
            glm_vec3_normalize(norm);

            memcpy(t->normal_buffer + index * 3, norm, 3 * sizeof(float));
        }
    }
    t->mesh.vert_count = vert_count;
    t->mesh.index_count = index_count;

    terrain_mesh_calctangents(&t->mesh);
    terrain_upload_img(t);
}

static void
terrain_mesh_calctangents(TerrainMesh *mesh)
{
    const uint index_count = mesh->index_count;
    const uint *indices = mesh->indices;

    float *verts = mesh->verts;
    float *uvs   = mesh->uvs;
    float *tans  = mesh->tans;

    uint i;

    for (i = 0; i < index_count; i += 3) {
        const uint idx1 = indices[i + 0];
        const uint idx2 = indices[i + 1];
        const uint idx3 = indices[i + 2];

        float *pos1 = verts + (idx1 * 3);
        float *pos2 = verts + (idx2 * 3);
        float *pos3 = verts + (idx3 * 3);

        float *uv1 = uvs + (idx1 * 2);
        float *uv2 = uvs + (idx2 * 2);
        float *uv3 = uvs + (idx3 * 2);

        vec3 e1, e2;
        vec2 d_uv1, d_uv2;

        glm_vec3_sub(pos2, pos1, e1);
        glm_vec3_sub(pos3, pos1, e2);
        glm_vec2_sub(uv2, uv1, d_uv1);
        glm_vec2_sub(uv3, uv1, d_uv2);

        float f = 1.0f / (d_uv1[0] * d_uv2[1] - d_uv2[0] * d_uv1[1]);
        vec3 tan;

        tan[0] = f * (d_uv2[1] * e1[0] - d_uv1[1] * e2[0]);
        tan[1] = f * (d_uv2[1] * e1[1] - d_uv1[1] * e2[1]);
        tan[2] = f * (d_uv2[1] * e1[2] - d_uv1[1] * e2[2]);

        glm_vec3_normalize(tan);

        memcpy(tans + (idx1 * 3), tan, 3 * sizeof(float));
        memcpy(tans + (idx2 * 3), tan, 3 * sizeof(float));
        memcpy(tans + (idx3 * 3), tan, 3 * sizeof(float));
    }
}

/* Fetches the desired normal from a neighbor and adds it to `norm` */
static void
terrain_getneighnorm(Terrain *t, float *norm, uint neigh_id, uint x, uint z)
{
    Terrain *neigh;
    float *n_norm;

    neigh = t->neighbors[neigh_id];

    if (neigh == NULL) {
        return;
    }
    n_norm = neigh->normal_buffer + (x + z * (TERRAIN_SIZE + 1)) * 3;
    glm_vec3_add(norm, n_norm, norm);
}

/* Calculates terrain mesh normals, using neighboring chunk normals */
void
terrain_calcnorms(Terrain *t)
{
    const uint MAX = TERRAIN_SIZE;

    uint needs_neighbors;
    uint x, z;
    vec3 norm;
    uint vbo_index;
    float *norm_dst;

    if (t->dirty == 0) {
        return;
    }

    for (z = 0; z <= MAX; z++) {
        for (x = 0; x <= MAX; x++) {
            const uint index = x + z * (MAX + 1);

            if (t->index_map[index] == TERRAIN_NULL_ID) {
                continue;
            }
            const int x_atedge = x == 0 || x == MAX;
            const int z_atedge = z == 0 || z == MAX;
            
            memcpy(norm, t->normal_buffer + index * 3, 3 * sizeof(float));

            needs_neighbors = x_atedge || z_atedge;

            if (needs_neighbors) {
                if (x == 0) {
                    terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_L, MAX, z);
                    if (z == 0) {
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_B, 0, MAX);
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_BL, MAX, MAX);
                    } else if (z == MAX) {
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_T, 0, 0);
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_TL, MAX, 0);
                    }
                } else if (x == MAX) {
                    terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_R, 0, z);
                    if (z == 0) {
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_B, MAX, MAX);
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_BR, 0, MAX);
                    } else if (z == MAX) {
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_T, MAX, 0);
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_TR, 0, 0);
                    }
                } else {
                    if (z == 0) {
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_B, x, MAX);
                    } else if (z == MAX) {
                        terrain_getneighnorm(t, norm, TERRAIN_NEIGHBOR_T, x, 0);
                    }
                }
                glm_vec3_normalize(norm);
            }
            vbo_index = t->index_map[index];
            norm_dst = t->mesh.norms + vbo_index * 3;

            memcpy(norm_dst, norm, 3 * sizeof(float));
        }
    }
}

/* Gets the Y-coordinate of the given terrain chunk at a specific coordinate */
float
terrain_getheight(Terrain *t, vec2 pos)
{
    const float scale_factor = 1 / 0.25;
    const ivec2 p = {pos[0] * scale_factor, pos[1] * scale_factor};
    const float x_lerp = (pos[0] * scale_factor - p[0]);
    const float z_lerp = (pos[1] * scale_factor - p[1]);

    #define LERP(t, a, b) ((a) + ((b) - (a)) * (t))
    #define Y_FETCH(x, z) t->mesh.verts[t->index_map[((x) + (z) * (TERRAIN_SIZE + 1))] * 3 + 1]

    const float x_min = Y_FETCH(p[0] + 0, p[1] + 0);
    const float x_max = Y_FETCH(p[0] + 1, p[1] + 0);
    const float z_min = Y_FETCH(p[0] + 0, p[1] + 1);
    const float z_max = Y_FETCH(p[0] + 1, p[1] + 1);

    const float a = (x_min > x_max) ? LERP(1.0 - x_lerp, x_max, x_min) : LERP(x_lerp, x_min, x_max);
    const float b = (z_min > z_max) ? LERP(1.0 - x_lerp, z_max, z_min) : LERP(x_lerp, z_min, z_max);

    return ((a > b) ? LERP(1.0 - z_lerp, b, a) : LERP(z_lerp, a, b));
}

/* Upload the terrain data to the GPU */
void
terrain_upload(Terrain *t, Material *mat)
{
    if (t->dirty == 0) {
        return;
    }
    terrain_mesh_upload(&t->mesh, mat);
    t->dirty = 0;
}

/* Draws the terrain mesh to the screen with the given material */
void
terrain_draw(Terrain *t, Material *mat, vec4 *model_mat, int depth_mode)
{
    const int unit_id = TERRAIN_TEX_BLENDMAP;

    if (depth_mode == 0) {
        atra_gl_tex_setactiveunit(unit_id);
        atra_gl_tex_enable(t->blend_tex.id);
        atra_shader_setuniform(mat->shader.locs[TERRAIN_LOCS_TEX_BLENDMAP],
                &unit_id, ATRA_SHADER_UNIFORM_INT, 1);
    }

    terrain_mesh_draw(&t->mesh, mat, model_mat, depth_mode);
}

/*static float p_dist(int x, int z) {
    union {
        f32 f;
        u32 i;
    } c;
    int n = x * x + z * z;
    c.f = n;
    c.i = 0x5f3759df - (c.i >> 1);
    c.f *= 1.5f - (n * c.f * c.f * 0.5f);

    return (1.0f / c.f);
}*/

/* Draws an explosion crater to the height and blend maps (implicit remesh) */
void
terrain_makecrater(Terrain *t, float radius, int x, int z)
{
    const uint midpoint = TERRAIN_SIZE / 2;
    const int ratio = BLEND_MAP_SIZE / TERRAIN_SIZE;
    const int r = fabsf(MIN(roundf(radius), midpoint - 1));
    const int br = r * ratio; 

    IQuad cr_quad;
    float dist, amount, depth;
    int i, j, u, v, idx;
    u8 *h_map, *b_map;
    int p1[2], p2[2];

    if (radius < 1.0f) {
        return;
    }
    p1[0] = MAX(MIN(x - r, TERRAIN_SIZE), 0);
    p1[1] = MAX(MIN(z - r, TERRAIN_SIZE), 0);
    p2[0] = MAX(MIN(x + r + 1, TERRAIN_SIZE), 0);
    p2[1] = MAX(MIN(z + r + 1, TERRAIN_SIZE), 0);

    cr_quad.x = p1[0];
    cr_quad.y = p1[1];
    cr_quad.w = p2[0] - p1[0];
    cr_quad.h = p2[1] - p1[1];

    if (cr_quad.w == 0 || cr_quad.h == 0) {
        return;
    }
    h_map = t->height_img.data;
    b_map = t->blend_img.data;

    for (i = cr_quad.y; i < cr_quad.y + cr_quad.h; i++) {
        for (j = cr_quad.x; j < cr_quad.x + cr_quad.w; j++) {
            p1[0] = j - x;
            p1[1] = i - z;
            idx = (j + i * HEIGHT_MAP_SIZE) * 4;
            dist = sqrtf(p1[0] * p1[0] + p1[1] * p1[1]);
            amount = dist / ((r - 2) * 2);
            depth = ((amount * amount) - 1);

            if (dist < r - 1) {
                h_map[idx + 0] = depth * 0xFF;
                h_map[idx + 1] = depth * 0xFF;
                h_map[idx + 2] = depth * 0xFF;
                h_map[idx + 3] = 0xFF;
            }
            
            for (u = 0; u < ratio; u++) {
                for (v = 0; v < ratio; v++) {
                    const int bx = j * ratio + v;
                    const int bz = i * ratio + u;
                    const int bidx = (bx + bz * BLEND_MAP_SIZE) * 4;

                    if (dist <= MAX(r - 2, 0)) {
                        b_map[bidx + 0] = 0xFF;
                        b_map[bidx + 1] = 0xFF;
                        b_map[bidx + 2] = 0xFF;
                        b_map[bidx + 3] = 0xFF;
                        continue;
                    }
                    const int bx_r = bx - (x + 0.5) * ratio;
                    const int bz_r = bz - (z + 0.5) * ratio;
                    const float b_dist = sqrt(bx_r * bx_r + bz_r * bz_r);

                    if (b_dist >= br - 4) {
                        const float diff = MAX(br - b_dist, 0) * 0.25;
                        b_map[bidx + 0] = diff * 0xFF;
                        b_map[bidx + 1] = diff * 0xFF;
                        b_map[bidx + 2] = diff * 0xFF;
                        b_map[bidx + 3] = 0xFF;
                    } else {
                        b_map[bidx + 0] = 0xFF;
                        b_map[bidx + 1] = 0xFF;
                        b_map[bidx + 2] = 0xFF;
                        b_map[bidx + 3] = 0xFF;
                    }
                }
            }
        }
    }
    t->dirty = 1;
}



/* Returns a new, empty terrain mesh instance */
TerrainMesh
terrain_mesh_create(void)
{
    TerrainMesh mesh = {0};

    return mesh;
}

/* Deletes a given terrain mesh */
void
terrain_mesh_delete(TerrainMesh *mesh)
{
    uint i;

    if (mesh->vao_id == 0) {
        return;
    }
    
    for (i = 0; i < TERRAIN_MESH_VBO_TOTAL; i++) {
        atra_gl_vbo_delete(mesh->vbo_ids[i]);
    }
    atra_gl_vao_delete(mesh->vao_id);
}

static void
terrain_mesh_initialupload(TerrainMesh *mesh, Material *mat)
{
    int *locs = mat->shader.locs;

    mesh->vao_id = atra_gl_vao_create();
    atra_gl_vao_enable(mesh->vao_id);

    mesh->vbo_ids[0] = atra_gl_vbo_create(mesh->verts, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(locs[TERRAIN_LOCS_ATR_POSITION], 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(locs[TERRAIN_LOCS_ATR_POSITION]);

    mesh->vbo_ids[1] = atra_gl_vbo_create(mesh->uvs, mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vao_setattrib(locs[TERRAIN_LOCS_ATR_TEXCOORD], 2, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(locs[TERRAIN_LOCS_ATR_TEXCOORD]);

    mesh->vbo_ids[2] = atra_gl_vbo_create(mesh->uvs2, mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vao_setattrib(locs[TERRAIN_LOCS_ATR_TEXCOORD2], 2, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(locs[TERRAIN_LOCS_ATR_TEXCOORD2]);

    mesh->vbo_ids[3] = atra_gl_vbo_create(mesh->norms, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(locs[TERRAIN_LOCS_ATR_NORMAL], 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(locs[TERRAIN_LOCS_ATR_NORMAL]);

    mesh->vbo_ids[4] = atra_gl_vbo_create(mesh->tans, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(locs[TERRAIN_LOCS_ATR_TANGENT], 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(locs[TERRAIN_LOCS_ATR_TANGENT]);

    mesh->vbo_ids[5] = atra_gl_vbo_create(mesh->colors, mesh->vert_count * 4 * sizeof(u8), 0);
    atra_gl_vao_setattrib(locs[TERRAIN_LOCS_ATR_COLOR], 4, ATRA_UNSIGNED_BYTE, 1, 0, NULL);
    atra_gl_vao_enableattrib(locs[TERRAIN_LOCS_ATR_COLOR]);

    mesh->vbo_ids[6] = atra_gl_ebo_create(mesh->indices, mesh->index_count * sizeof(uint), 0);
}

static void
terrain_mesh_overwritevao(TerrainMesh *mesh)
{
    atra_gl_vao_enable(mesh->vao_id);
 
    atra_gl_vbo_overwrite(mesh->vbo_ids[0], mesh->verts, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vbo_overwrite(mesh->vbo_ids[1], mesh->uvs, mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vbo_overwrite(mesh->vbo_ids[2], mesh->uvs2, mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vbo_overwrite(mesh->vbo_ids[3], mesh->norms, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vbo_overwrite(mesh->vbo_ids[4], mesh->tans, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vbo_overwrite(mesh->vbo_ids[5], mesh->colors, mesh->vert_count * 4 * sizeof(u8), 0);
    atra_gl_ebo_overwrite(mesh->vbo_ids[6], mesh->indices, mesh->index_count * sizeof(uint), 0);
}

/* Uploads terrain mesh data to the GPU */
void
terrain_mesh_upload(TerrainMesh *mesh, Material *mat)
{
    if (mesh->vao_id == 0) {
        terrain_mesh_initialupload(mesh, mat);
    } else {
        terrain_mesh_overwritevao(mesh);
    }
}

/* Draws the terrain mesh to the screen */
void
terrain_mesh_draw(TerrainMesh *mesh, Material *mat, vec4 *model_mat, int depth_mode)
{
    Camera * const cam = atra_camera_getmain();

    mat4 modelview;
    mat4 mvp;
    int model_loc;
    int mvp_loc;

    Shader *depth_shader = state_suncam_getshader();

    if (mesh->vert_count == 0) {
        return;
    }

    if (depth_mode == 0) {
        model_loc = mat->shader.locs[TERRAIN_LOCS_MAT_MODEL];
        mvp_loc = mat->shader.locs[TERRAIN_LOCS_MAT_MVP];
    } else {
        model_loc = depth_shader->locs[DEPTH_LOC_MAT_MODEL];
    }
    atra_shader_setuniform(model_loc, model_mat, ATRA_SHADER_UNIFORM_MAT4, 1);

    if (depth_mode == 0) {
        glm_mat4_mul(cam->view, model_mat, modelview);
        glm_mat4_mul(cam->proj, modelview, mvp);
        atra_shader_setuniform(mvp_loc, mvp, ATRA_SHADER_UNIFORM_MAT4, 1);
    }

    atra_gl_vao_enable(mesh->vao_id);
    atra_gl_vao_drawelements(0, mesh->index_count, NULL);
}
