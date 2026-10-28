/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  particle.c - Particle management and rendering system.
 *
 */

#include "particle.h"
#include "file.h"
#include "state_game.h"
#include "world.h"

#define LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#define LERP_BI(a, b, t) ((a > b) ? \
        ((b) + ((a) - (b)) * (1.0 - (t))) : \
        ((a) + ((b) - (a)) * (t)))

ATRA_OPTION_DEFINE(ParticleList)

static const float default_verts[18] = {
    -0.5f,  0.5f,  0.0f,
    -0.5f, -0.5f,  0.0f,
     0.5f, -0.5f,  0.0f,
     0.5f, -0.5f,  0.0f,
     0.5f,  0.5f,  0.0f,
    -0.5f,  0.5f,  0.0f
};

static const float default_norms[18] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
};

static const float default_uvs[8] = {
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f
};

static ParticleDB part_db;
static int db_isready;
static Shader part_shader;
static HashMap part_path_map;

static void particlelist_upload_init(ParticleList *list);
static void particlelist_upload_update(ParticleList *list);


/* Initialise the particle database */
void
db_particle_init(void)
{
    const uint attr_count = 8;
    const uint attr_ids[] = {0, 1, 2, 6, 7, 8, 9, 10};
    const char *attr_names[] = {
        "vert_pos",
        "vert_norm",
        "inst_modelmat",
        "inst_uvs_0",
        "inst_uvs_1",
        "inst_uvs_2",
        "inst_uvs_3",
        "inst_color"
    };

    if (db_isready) {
        atra_log_print(LOG_WARN, "particle: Attempted to initialize particle database multiple times");
        return;
    }
    db_isready = 1;

    #define getattrid(loc, name) \
        part_shader.locs[loc] = atra_shader_getattribloc(&part_shader, name);
    #define getlocid(loc, name) \
        part_shader.locs[loc] = atra_shader_getuniformloc(&part_shader, name);

    part_db.cap = PARTICLE_DEF_TYPE_CAP;
    part_db.size = 0;
    part_db.lists = malloc(part_db.cap * sizeof(ParticleList));

    part_shader = atra_shader_loadfile_locset(PATH_PARTICLE_VS, PATH_PARTICLE_FS, attr_names, attr_ids, attr_count);

    part_shader.locs[PARTICLE_LOC_POSITION] = attr_ids[0];
    part_shader.locs[PARTICLE_LOC_NORMAL] = attr_ids[1];
    part_shader.locs[PARTICLE_LOC_MODELS] = attr_ids[2];
    part_shader.locs[PARTICLE_LOC_UV_0] = attr_ids[3];
    part_shader.locs[PARTICLE_LOC_UV_1] = attr_ids[4];
    part_shader.locs[PARTICLE_LOC_UV_2] = attr_ids[5];
    part_shader.locs[PARTICLE_LOC_UV_3] = attr_ids[6];
    part_shader.locs[PARTICLE_LOC_COLOR] = attr_ids[7];

    getlocid(PARTICLE_LOC_ALBEDO, "albedo_map");
    getlocid(PARTICLE_LOC_EMISSIVE, "emission_map");
    getlocid(PARTICLE_LOC_SHADOWMAP, "shadow_map");
    getlocid(PARTICLE_LOC_VIEWPOS, "view_pos");
    getlocid(PARTICLE_LOC_AMBIENTCOLOR, "ambient_color");
    getlocid(PARTICLE_LOC_SUNCOLOR, "sun_color");
    getlocid(PARTICLE_LOC_SUNDIR, "sun_dir");
    getlocid(PARTICLE_LOC_VIEWPROJ, "viewproj");
    getlocid(PARTICLE_LOC_LIGHTSPACE, "lightspace");

    part_path_map = atra_hashmap_create(sizeof(char *), sizeof(uint), atra_hashmap_str_hash, atra_hashmap_str_eq);
}

/* Delete the particle database */
void
db_particle_shutdown(void)
{
    uint i;

    for (i = 0; i < part_db.size; i++) {
        particlelist_delete(part_db.lists + i);
    }
    free(part_db.lists);
    atra_hashmap_delete(&part_path_map, HASH_FREE_KEY);
    part_db.size = 0;
    db_isready = 0;
}

/* Clear the particle database */
void
db_particle_clear(void)
{
    uint i;

    for (i = 0; i < part_db.size; i++) {
        particlelist_delete(part_db.lists + i);
    }
    part_db.size = 0;
    atra_hashmap_clear(&part_path_map, HASH_FREE_KEY);
}

/* Load a new particle type from a file. Returns an index if possible. */
Option_uint
db_particle_loadfile(const char *path)
{
    Option_ParticleList op_list;
    ParticleList * const list = &part_db.lists[part_db.size];

    if (part_db.size == part_db.cap) { db_particle_grow(); }
    op_list = particlelist_loadfile(path);
    
    if (op_list.none) { return atra_option_uint_error(); }
    *list = op_list.some;
    list->id = part_db.size;
    part_db.size++;

    const char * const key = atra_util_strdup(path);
    atra_hashmap_add(&part_path_map, &key, &list->id);

    return atra_option_uint_ok(list->id);
}

/* Returns a pointer to the particle list associated with the given ID */
ParticleList *
db_particle_getlist(uint id)
{
    return &part_db.lists[id];
}

/* Returns a pointer to the particle list associated with the given file path.
 * NOTE: Implicitly loads given file into database if none exists. */
ParticleList *
db_particle_getlist_bypath(const char *path)
{
    uint *result = atra_hashmap_get(&part_path_map, &path);
    Option_uint op_id;
    uint id;

    if (result == NULL) {
        op_id = db_particle_loadfile(path);

        if (op_id.none) {
            atra_log_print(LOG_ERROR, "particle: Unable to get particle ID: Error loading file: \"%s\"", path);
            return NULL;
        }
        id = op_id.some;
    } else {
        id = *result;
    }
    
    return &part_db.lists[id];
}

/* Returns a pointer to the particle list associated with the given file path.
 * NOTE: Implicitly loads given file into database if none exists. */
uint
db_particle_getid_bypath(const char *path)
{
    uint *result = atra_hashmap_get(&part_path_map, &path);
    Option_uint op_id;
    uint id;

    if (result == NULL) {
        op_id = db_particle_loadfile(path);

        if (op_id.none) {
            atra_log_print(LOG_ERROR, "particle: Unable to get particle ID: Error loading file: \"%s\"", path);
            return 0;
        }
        id = op_id.some;
    } else {
        id = *result;
    }
    
    return id;
}

/* Expand the capacity of the database */
void
db_particle_grow(void)
{
    part_db.cap *= 2;
    part_db.lists = realloc(part_db.lists, part_db.cap * sizeof(ParticleList));
}

/* Update all particle lists in the database */
void
db_particle_update(u32 dt)
{
    uint i;
    ParticleList *plist;

    for (i = 0; i < part_db.size; i++) {
        plist = part_db.lists + i;
        particlelist_update(plist, dt);
    }
}

/* Update all particle list in the database (using thread pool) */
void
db_particle_update_dispatch(u32 dt)
{

}

/* Upload all newly generated particle instance data to the GPU */
void
db_particle_upload(void)
{
    uint i;
    ParticleList *plist;

    for (i = 0; i < part_db.size; i++) {
        plist = part_db.lists + i;
        particlelist_upload(plist);
    }
}

/* Render all particle lists in the database */
void
db_particle_drawall_depth(void)
{
    DirLight * const sunlight = state_suncam_get();

    ParticleList *plist;
    mat4 sunspace;
    uint i;

    Shader *depth_shader;

    depth_shader = state_suncam_getshader();

    glm_mat4_mul(sunlight->proj, sunlight->view, sunspace);
    atra_shader_setactive(depth_shader);
    atra_shader_setuniform(depth_shader->locs[DEPTH_LOC_MAT_LIGHTSPACE],
            sunspace, ATRA_SHADER_UNIFORM_MAT4, 1);

    for (i = 0; i < part_db.size; i++) {
        plist = part_db.lists + i;
        particlelist_render(plist);
    }
}

/* Render all particle lists in the database to the screen */
void
db_particle_draw_forward(void)
{
    Camera * const cam = atra_camera_getmain();
    DirLight * const sunlight = state_suncam_get();
    const World * const world = state_world_get();

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

    ParticleList *plist;
    int viewproj_loc, sunspace_loc;
    int viewpos_loc, ambient_loc, suncolor_loc, sundir_loc;
    int albedo_unit, emissive_unit, depthmap_unit;
    mat4 viewproj, sunspace;
    uint i;

    glm_mat4_mul(sunlight->proj, sunlight->view, sunspace);
    glm_mat4_mul(cam->proj, cam->view, viewproj);

    albedo_unit = PARTICLE_TEXUNIT_ALBEDO;
    emissive_unit = PARTICLE_TEXUNIT_EMISSION;
    depthmap_unit = PARTICLE_TEXUNIT_SHADOWMAP;
   
    atra_shader_setactive(&part_shader);
    atra_dirlight_binddepthtex(sunlight, depthmap_unit);

    viewproj_loc = part_shader.locs[PARTICLE_LOC_VIEWPROJ];
    sunspace_loc = part_shader.locs[PARTICLE_LOC_LIGHTSPACE];
    viewpos_loc = part_shader.locs[PARTICLE_LOC_VIEWPOS];
    ambient_loc = part_shader.locs[PARTICLE_LOC_AMBIENTCOLOR];
    suncolor_loc = part_shader.locs[PARTICLE_LOC_SUNCOLOR];
    sundir_loc = part_shader.locs[PARTICLE_LOC_SUNDIR];

    atra_shader_setuniform(viewproj_loc, viewproj, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(sunspace_loc, sunspace, ATRA_SHADER_UNIFORM_MAT4, 1);
    
    atra_shader_setuniform(viewpos_loc, cam->pos, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(ambient_loc, ambient_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(suncolor_loc, sun_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(sundir_loc, sunlight->dir, ATRA_SHADER_UNIFORM_VEC3, 1);
    
    atra_shader_setuniform(part_shader.locs[PARTICLE_LOC_ALBEDO], &albedo_unit, ATRA_SHADER_UNIFORM_INT, 1);
    atra_shader_setuniform(part_shader.locs[PARTICLE_LOC_EMISSIVE], &emissive_unit, ATRA_SHADER_UNIFORM_INT, 1);
    atra_shader_setuniform(part_shader.locs[PARTICLE_LOC_SHADOWMAP], &depthmap_unit, ATRA_SHADER_UNIFORM_INT, 1);

    for (i = 0; i < part_db.size; i++) {
        plist = part_db.lists + i;

        atra_gl_tex_setactiveunit(PARTICLE_TEXUNIT_ALBEDO);
        atra_gl_tex_enable(plist->tex_color.id);
        atra_gl_tex_setactiveunit(PARTICLE_TEXUNIT_EMISSION);
        atra_gl_tex_enable(plist->tex_emit.id);

        particlelist_render(plist);
    }
}


/* Create a new particle list */
ParticleList
particlelist_create(void)
{
    ParticleList list = {0};

    list.cap = PARTICLE_DEF_LIST_CAP;
    list.parts = malloc(list.cap * sizeof(IParticle));
    list.colors = malloc(list.cap * sizeof(Color));
    list.modelmats = malloc(list.cap * 16 * sizeof(float));
    list.quad_uvs = malloc(list.cap * 8 * sizeof(float));

    return list;
}

/* Create a new particle list with custom parameters (use for file generation) */
Option_ParticleList particlelist_create_custom(u32 lifetime, u32 flags, const char *save_path,
    float start_scale, float end_scale,
    Color start_color, Color end_color,
    float start_speed, float end_speed,
    float gravity, const char *color_path, const char *emit_path,
    uint frame_size, uint frame_count)
{
    ParticleList list = particlelist_create();
    Option_Texture op_tex;

    list.file_path = atra_util_strdup(save_path);
    list.lifetime = lifetime;
    list.flags = flags;
    list.start_scale = start_scale;
    list.end_scale = end_scale;
    list.start_color = start_color;
    list.end_color = end_color;
    list.start_speed = start_speed;
    list.end_speed = end_speed;
    list.gravity = gravity;

    op_tex = atra_texture_loadfile(color_path);
    if (op_tex.none) {
        atra_log_print(LOG_ERROR, "particle: Unable to load color texture: \"%s\"", color_path);
        return atra_option_ParticleList_error();
    }
    list.tex_color = op_tex.some;
    
    op_tex = atra_texture_loadfile(emit_path);
    if (op_tex.none) {
        atra_log_print(LOG_ERROR, "particle: Unable to load emission texture: \"%s\"", color_path);
        return atra_option_ParticleList_error();
    }
    list.tex_emit = op_tex.some;

    list.color_path = atra_util_strdup(color_path);
    list.emit_path = atra_util_strdup(emit_path);

    list.frame_size = frame_size;
    list.frame_count = frame_count;

    list.mesh = particlemesh_create();

    return atra_option_ParticleList_ok(list);
}


/* Creates a new particle list from the given file */
Option_ParticleList
particlelist_loadfile(const char *path)
{
    FileData file;
    ParticleList list;
    Option_Texture op_tex;

    u32 start_color, end_color;

    file = file_load(path);
    list = particlelist_create();

    /* TODO: Replace Color casting with safe, explicit conversion */

    list.file_path = atra_util_strdup(path);
    list.lifetime = file_read_u32(&file);
    list.flags = file_read_u32(&file);
    list.start_scale = file_read_float(&file);
    list.end_scale = file_read_float(&file);
    start_color = file_read_u32(&file);
    end_color = file_read_u32(&file);
    list.start_color = *(Color *)(&start_color);
    list.end_color = *(Color *)(&end_color);
    list.start_speed = file_read_float(&file);
    list.end_speed = file_read_float(&file);
    list.gravity = file_read_float(&file);
    
    /* Load albedo texture */
    list.color_path = file_read_string(&file);

    if (list.color_path != NULL) {
        op_tex = atra_texture_loadfile(list.color_path);
        if (op_tex.none) {
            atra_log_print(LOG_ERROR, "particle: Unable to load color texture: \"%s\"", list.color_path);
            return atra_option_ParticleList_error();
        }
        list.tex_color = op_tex.some;
    }

    /* Load emission texture */
    list.emit_path = file_read_string(&file);

    if (list.emit_path != NULL) {
        op_tex = atra_texture_loadfile(list.emit_path);
        if (op_tex.none) {
            atra_log_print(LOG_ERROR, "particle: Unable to load emission texture: \"%s\"", list.color_path);
            return atra_option_ParticleList_error();
        }
        list.tex_emit = op_tex.some;
    }
    list.frame_size = file_read_uint(&file);
    list.frame_count = file_read_uint(&file);
    file_delete(file);

    return atra_option_ParticleList_ok(list);
}

/* Write the particle list properties to a file */
void
particlelist_savefile(ParticleList *list, const char *path)
{
    FileData file;

    file = file_create(0);

    file_write_u32(&file, list->lifetime);
    file_write_u32(&file, list->flags);
    file_write_float(&file, list->start_scale);
    file_write_float(&file, list->end_scale);
    file_write_u32(&file, *(u32 *)(&list->start_color));
    file_write_u32(&file, *(u32 *)(&list->end_color));
    file_write_float(&file, list->start_speed);
    file_write_float(&file, list->end_speed);
    file_write_float(&file, list->gravity);

    file_write_string(&file, list->color_path);
    file_write_string(&file, list->emit_path);

    file_write_uint(&file, list->frame_size);
    file_write_uint(&file, list->frame_count);
    file_save(&file, path);
    file_delete(file);
}

/* Delete an existing particle list */
void
particlelist_delete(ParticleList *list)
{
    particlemesh_delete(&list->mesh);
    free(list->parts);
    free(list->colors);
    free(list->modelmats);
    free(list->quad_uvs);
    free(list->file_path);
    free(list->color_path);
    free(list->emit_path);
}

/* Expand a given particle list's capacity */
void
particlelist_grow(ParticleList *list)
{
    list->cap *= 2;

    list->parts = realloc(list->parts, list->cap * sizeof(IParticle));
    list->colors = realloc(list->colors, list->cap * sizeof(Color));
    list->modelmats = realloc(list->modelmats, list->cap * 16 * sizeof(float));
    list->quad_uvs = realloc(list->quad_uvs, list->cap * 8 * sizeof(float));
}

/* Clear particle list of all existing particles */
void
particlelist_clear(ParticleList *list)
{
    list->size = 0;
}

/* Adds a new particle to the list with the given life and position.
 * NOTE: Life capped by containing list's `lifetime` variable */
void
particlelist_add(ParticleList *list, vec3 pos, vec3 rot, vec3 dir, u32 life, float grav)
{
    if (list->size >= list->cap) {
        particlelist_grow(list);
    }
    IParticle * const p = &list->parts[list->size];
    
    p->pos[0] = pos[0];
    p->pos[1] = pos[1];
    p->pos[2] = pos[2];

    p->rot[0] = rot[0];
    p->rot[1] = rot[1];
    p->rot[2] = rot[2];

    p->dir[0] = dir[0];
    p->dir[1] = dir[1];
    p->dir[2] = dir[2];

    p->grav = grav;
    p->life = (life > list->lifetime) ? list->lifetime : life;

    list->size++;
}

/* Remove a single particle instance from a particle list */
void
particlelist_remove(ParticleList *list, uint index)
{
    const uint length = list->size - index - 1;

    IParticle * const part_src = list->parts + index + 1;
    IParticle * const part_dst = list->parts + index;
    float * const model_src = list->modelmats + (index + 1) * 16;
    float * const model_dst = list->modelmats + (index + 0) * 16;
    float * const uvs_src = list->quad_uvs + (index + 1) * 8;
    float * const uvs_dst = list->quad_uvs + (index + 0) * 8;

    memmove(part_dst, part_src, length * sizeof(IParticle));
    memmove(model_dst, model_src, length * 16 * sizeof(float));
    memmove(uvs_dst, uvs_src, length * 8 * sizeof(float));

    list->size--;
}

/* Remove a contiguous range of particles from a particle list in [start, end) */
void
particlelist_removerange(ParticleList *list, uint start, uint end)
{
    const uint length = list->size - end;
    
    IParticle * const part_src = list->parts + end;
    IParticle * const part_dst = list->parts + start;
    float * const model_src = list->modelmats + end * 16; 
    float * const model_dst = list->modelmats + start * 16;
    float * const uvs_src = list->quad_uvs + end * 8;
    float * const uvs_dst = list->quad_uvs + start * 8;

    memmove(part_dst, part_src, length * sizeof(IParticle));
    memmove(model_dst, model_src, length * 16 * sizeof(float));
    memmove(uvs_dst, uvs_src, length * 8 * sizeof(float));

    list->size -= end - start;
}

/* Update a particle list, and all contained particles */
void
particlelist_update(ParticleList *list, u32 dt)
{
    const float time = dt * 0.001;
    Camera * const cam = atra_camera_getmain();

    IParticle *p;
    Color *color;
    vec4 *model;
    float *uvs;

    float lerp_amount;
    float alpha_lerp;
    float scale;
    float speed;
    uint frame_num, frames_per_row;
    uint frame_x, frame_y;
    uint i;

    mat4 inv_view;

    glm_mat4_inv(cam->view, inv_view);
    inv_view[3][0] = 0.0f;
    inv_view[3][1] = 0.0f;
    inv_view[3][2] = 0.0f;

    for (i = 0; i < list->size; i++) {
        p = list->parts + i;
        
        if (p->life <= (i32) dt) {
            particlelist_remove(list, i);
            i--;
            continue;
        }
        p->life -= dt;

        color = list->colors + i;
        model = (vec4 *)(list->modelmats + i * 16);
        uvs = list->quad_uvs + i * 8;
        
        lerp_amount = 1.0f - ((float) p->life / list->lifetime);
        
        scale = LERP_BI(list->start_scale, list->end_scale, lerp_amount);
        speed = LERP_BI(list->start_speed, list->end_speed, lerp_amount) * time;

        p->grav += list->gravity * time;

        p->pos[0] += p->dir[0] * speed;
        p->pos[1] += p->dir[1] * speed - p->grav;
        p->pos[2] += p->dir[2] * speed;

        /* TODO: Add instance colors */
        color->r = LERP_BI(list->start_color.r, list->end_color.r, lerp_amount);
        color->g = LERP_BI(list->start_color.g, list->end_color.g, lerp_amount);
        color->b = LERP_BI(list->start_color.b, list->end_color.b, lerp_amount);
        color->a = LERP_BI(list->start_color.a, list->end_color.a, lerp_amount);

        if (BIT_CHK(list->flags, PARTICLE_PROP_FADEONDEATH)) {
            alpha_lerp = ((p->life < 250) ? 250 - p->life : 0) / 250.0f;
            color->a = LERP_BI(0, color->a, 1.0f - alpha_lerp);
        }

        glm_mat4_identity(model);
        
        glm_translate(model, p->pos);
        glm_scale_uni(model, scale);
        
        if (BIT_CHK(list->flags, PARTICLE_PROP_RANDOMROTATE)) {
            const float random = atra_util_randomfloat(0.0f, 2 * PI);
            glm_rotate_z(model, random, model);
        }
        glm_mat4_mul(model, inv_view, model);

        if (BIT_CHK(list->flags, PARTICLE_PROP_ISANIMATED)) {
            frames_per_row = list->tex_color.width / list->frame_size;
            frame_num = lerp_amount * list->frame_count;
            frame_x = frame_num % frames_per_row;
            frame_y = frame_num / frames_per_row;
            
            const IQuad quad = {
                .x = frame_x * list->frame_size,
                .y = frame_y * list->frame_size,
                .w = list->frame_size,
                .h = list->frame_size};
            atra_texture_quadtouvs(&list->tex_color, quad, uvs);
        } else {
            memcpy(uvs, default_uvs, 8 * sizeof(float));
        }
    }
}

/* Update a particle list, and all contained particles (using thread pool) */
void
particlelist_update_dispatch(ParticleList *list, u32 dt)
{

}

static void
particlelist_upload_init(ParticleList *list)
{
    const uint count = list->size;
    int * const locs = part_shader.locs;
    ParticleMesh * const mesh = &list->mesh;

    mesh->vao = atra_gl_vao_create();
    atra_gl_vao_enable(mesh->vao);

    mesh->vbos[PARTICLE_VBO_POSITION] = atra_gl_vbo_create(mesh->verts, 18 * sizeof(float), 0);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_POSITION], 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_POSITION]);

    mesh->vbos[PARTICLE_VBO_NORMAL] = atra_gl_vbo_create(mesh->norms, 18 * sizeof(float), 0);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_NORMAL], 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_NORMAL]);

    mesh->vbos[PARTICLE_VBO_MODELS] = atra_gl_vbo_create(list->modelmats, count * sizeof(mat4), 1);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_MODELS] + 0, 4, ATRA_FLOAT, 0, sizeof(mat4), (void *)0);
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_MODELS] + 0);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_MODELS] + 1, 4, ATRA_FLOAT, 0, sizeof(mat4), (void *)(1 * sizeof(vec4)));
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_MODELS] + 1);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_MODELS] + 2, 4, ATRA_FLOAT, 0, sizeof(mat4), (void *)(2 * sizeof(vec4)));
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_MODELS] + 2);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_MODELS] + 3, 4, ATRA_FLOAT, 0, sizeof(mat4), (void *)(3 * sizeof(vec4)));
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_MODELS] + 3);

    mesh->vbos[PARTICLE_VBO_UV_0] = atra_gl_vbo_create(list->quad_uvs, count * 4 * sizeof(vec2), 1);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_UV_0], 2, ATRA_FLOAT, 0, 4 * sizeof(vec2), (void *)0);
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_UV_0]);

    mesh->vbos[PARTICLE_VBO_UV_1] = atra_gl_vbo_create(list->quad_uvs, count * 4 * sizeof(vec2), 1);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_UV_1], 2, ATRA_FLOAT, 0, 4 * sizeof(vec2), (void *)(1 * sizeof(vec2)));
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_UV_1]);

    mesh->vbos[PARTICLE_VBO_UV_2] = atra_gl_vbo_create(list->quad_uvs, count * 4 * sizeof(vec2), 1);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_UV_2], 2, ATRA_FLOAT, 0, 4 * sizeof(vec2), (void *)(2 * sizeof(vec2)));
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_UV_2]);

    mesh->vbos[PARTICLE_VBO_UV_3] = atra_gl_vbo_create(list->quad_uvs, count * 4 * sizeof(vec2), 1);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_UV_3], 2, ATRA_FLOAT, 0, 4 * sizeof(vec2), (void *)(3 * sizeof(vec2)));
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_UV_3]);

    mesh->vbos[PARTICLE_VBO_COLOR] = atra_gl_vbo_create(list->colors, count * 4 * sizeof(u8), 1);
    atra_gl_vao_setattrib(locs[PARTICLE_LOC_COLOR], 4, ATRA_UNSIGNED_BYTE, 1, 0, NULL);
    atra_gl_vao_enableattrib(locs[PARTICLE_LOC_COLOR]);

    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_UV_0], 1);
    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_UV_1], 1);
    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_UV_2], 1);
    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_UV_3], 1);
    
    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_MODELS] + 0, 1);
    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_MODELS] + 1, 1);
    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_MODELS] + 2, 1);
    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_MODELS] + 3, 1);
    
    atra_gl_vao_setattribdivisor(locs[PARTICLE_LOC_COLOR], 1);
}

static void
particlelist_upload_update(ParticleList *list)
{
    const uint count = list->size;
    ParticleMesh *const mesh = &list->mesh;
    
    atra_gl_vao_enable(mesh->vao);

    atra_gl_vbo_overwrite(mesh->vbos[2], list->modelmats, count * sizeof(mat4), 1);
    atra_gl_vbo_overwrite(mesh->vbos[3], list->quad_uvs, count * 4 * sizeof(vec2), 1);
    atra_gl_vbo_overwrite(mesh->vbos[4], list->quad_uvs, count * 4 * sizeof(vec2), 1);
    atra_gl_vbo_overwrite(mesh->vbos[5], list->quad_uvs, count * 4 * sizeof(vec2), 1);
    atra_gl_vbo_overwrite(mesh->vbos[6], list->quad_uvs, count * 4 * sizeof(vec2), 1);
    atra_gl_vbo_overwrite(mesh->vbos[7], list->colors, count * 4 * sizeof(u8), 1);
}

/* Upload particle instance data to the GPU */
void
particlelist_upload(ParticleList *list)
{
    if (list->mesh.vao == 0) {
        particlelist_upload_init(list);
    } else {
        particlelist_upload_update(list);
    }
}

/* Render all particles in a particle list to the screen */
void
particlelist_render(ParticleList *list)
{
    if (list->mesh.vao == 0) {
        return;
    }

    atra_gl_vao_enable(list->mesh.vao);
    atra_gl_vao_drawinstanced(0, 6, list->size);
}


/* Create a new default particle mesh */
ParticleMesh
particlemesh_create(void)
{
    ParticleMesh mesh = {0};

    memcpy(mesh.verts, default_verts, 18 * sizeof(float));
    memcpy(mesh.norms, default_norms, 18 * sizeof(float));

    return mesh;
}

/* Delete an existing particle mesh */
void
particlemesh_delete(ParticleMesh *mesh)
{
    uint i;

    if (mesh->vao != 0) {
        atra_gl_vao_delete(mesh->vao);
    }

    for (i = 0; i < PARTICLE_VBO_TOTAL; i++) {
        if (mesh->vbos[i] != 0) {
            atra_gl_vbo_delete(mesh->vbos[i]);
        }
    }
}


/* Create a new particle emitter */
Emitter
emitter_create(uint part_id, vec3 pos, vec3 rot, uint rate, u32 dur, float spread)
{
    Emitter e = {0};

    e.part_id = part_id;
    memcpy(e.pos, pos, sizeof(vec3));
    memcpy(e.rot, rot, sizeof(vec3));

    e.fire_rate = rate;
    e.curr_time = 0;
    e.duration = dur;
    e.spread = spread;

    return e;
}

/* Resets the firing time to 0, restarting firing */
void
emitter_fire(Emitter *e)
{
    e->curr_time = 0;
}

/* Halts any emitter from firing */
void
emitter_stop(Emitter *e)
{
    e->curr_time = UINT32_MAX;
}

/* Update an emitter, spawning new particles when necessary */
void
emitter_update(Emitter *e, u32 dt)
{
    if (e->curr_time == UINT32_MAX ||
            (e->duration > 0 && e->duration < e->curr_time)) {
        return;
    }
    
    const u32 fire_gap = e->curr_time - e->last_fire + dt;
    const float fire_delay = 1000.0f / e->fire_rate;
    const u32 fire_count = fire_gap / fire_delay;

    ParticleList * const plist = part_db.lists + e->part_id;

    vec3 dir, rot;
    float spread_radius;
    uint p;

    vec3 sphere_point;
    float theta, phi, radius;

    /* Calculate emitter vectors */
    e->dir[0] = cosf(e->rot[0]) * sinf(e->rot[1]);
    e->dir[1] = sinf(e->rot[0]);
    e->dir[2] = cosf(e->rot[0]) * cosf(e->rot[1]);

    e->right[0] = sinf(e->rot[1] - (PI / 2.0f));
    e->right[1] = 0.0f;
    e->right[2] = cosf(e->rot[1] - (PI / 2.0f));

    glm_vec3_cross(e->right, e->dir, e->up);

    /* Add new particles */
    for (p = fire_count; p > 0; p--) {
        rot[0] = 0.0f;
        rot[1] = 0.0f;
        rot[2] = 0.0f;

        if (BIT_CHK(plist->flags, PARTICLE_PROP_RANDOMROTATE)) {
            rot[2] = atra_util_randomfloat(0, 2 * PI);
        }

        if (e->spread > 0.0f) {
            theta = atra_util_randomfloat(0.0f, 2 * PI);
            phi = atra_util_randomfloat(0.0f, PI);
            radius = atra_util_randomfloat(0.0f, e->spread);

            sphere_point[0] = radius * sinf(phi) * cosf(theta);
            sphere_point[1] = radius * sinf(phi) * sinf(theta);
            sphere_point[2] = radius * cosf(phi);

            spread_radius = tanf(e->spread / 2.0f);

            memcpy(dir, e->dir, sizeof(vec3));
            glm_vec3_muladds(sphere_point, spread_radius, dir);

            particlelist_add(plist, e->pos, rot, dir, plist->lifetime, plist->gravity);
        } else {
            particlelist_add(plist, e->pos, rot, e->dir, plist->lifetime, plist->gravity);
        }
    }
    e->last_fire += fire_count * fire_delay;
    e->curr_time += dt;
}

