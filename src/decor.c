/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  decor.c - World decorations and props
 *
 */

#include "decor.h"
#include "state_game.h"
#include "world.h" 
#include "file.h"

ATRA_LIST_DEFINE(Emitter)
ATRA_LIST_DEFINE(DecorInst)
ATRA_LIST_DEFINE(Decor)
ATRA_OPTION_DEFINE(ModelAnimRaw)

DecorDB decor_db;

static Model decormodel_loadfile(const char *path);
static void decormodel_uploadmesh(Mesh *mesh);
static void decormodel_loadtextures(Model *model, const char *albedo_path,
        const char *spec_path, const char *norm_path, const char *emission_path);


/* Initialise the decoration database */
void
decor_init(void)
{
	decor_db = (DecorDB){
		.decor = atra_list_Decor_create(0),
		.paths = atra_hashmap_create(sizeof(char *), sizeof(uint), atra_hashmap_str_hash, atra_hashmap_str_eq)
	};
}

/* Shutdown the decoration database, freeing all allocated data */
void
decor_shutdown(void)
{
	decor_clear();
	atra_list_Decor_delete(&decor_db.decor);
	atra_hashmap_delete(&decor_db.paths, 0);
}

/* Loads a new decoration type into the database from the given file path */
uint
decor_loadfile(const char *path)
{
	FileData file = file_load(path);
	Decor decor;
	uint id;

	
}

/* Saves an existing decoration to a given file path */
void
decor_savefile(const Decor *decor, const char *path)
{
	
}

/* Adds a given decoration type into the database, with an optional path.
 * Returns the index of the added decoration type.  */
uint
decor_add(Decor *decor)
{
	if (decor->file_path != NULL) {
		atra_hashmap_add(&decor_db.paths, &decor->file_path, &decor_db.decor.size);
	} else {
		atra_log_print(LOG_WARN, "decor: No file path provided with decoration type: \"%s\"", decor->name);
	}

	if (decor_db.decor.size + 1 >= decor_db.decor.cap) {
		decor->model_mats = realloc(decor->model_mats, decor->decors.cap * 2 * 16 * sizeof(float)); 
	}
	atra_list_Decor_add(&decor_db.decor, *decor);

	return decor_db.decor.size - 1;
}

/* Creates a decoration type with custom attributes and adds it to the database.
 * Returns the index of the decoration type in the database.
 * `anim_path` is optional and can be NULL. */
uint
decor_new(PhysBody body, DecorCollide collide_map, const char *name,
		const char *file_path, const char *model_path,
		const char *anim_path, List_Emitter emitters)
{
	Decor decor;
	int anim_count;
	ModelAnimRaw *anims;

	decor.model = atra_model_loadfile(model_path, 1);
	decor.body = body;
	
	if (anim_path != NULL) {
		anims = atra_rawanim_loadfile(anim_path, &anim_count);
		if (anims != NULL) {
			decor.anim = atra_option_ModelAnimRaw_ok(anims[0]);
		}
	} else {
		decor.anim = atra_option_ModelAnimRaw_error();
	}
	decor.collide_map = collide_map;
	decor.name = atra_util_strdup(name);
	decor.file_path = atra_util_strdup(file_path);
	decor.model_path = atra_util_strdup(model_path);
	decor.anim_path = atra_util_strdup(anim_path);
	decor.emitters = emitters;
	decor.decors = atra_list_DecorInst_create(ATRA_DECOR_DEFAULT_CAP);
	decor.model_mats = malloc(decor.decors.cap * 16 * sizeof(float));

	return decor_add(&decor);
}

/* Clears the database of all decoration types, resetting it */
void
decor_clear(void)
{
	Decor *decor;
	DecorInst *inst;
	uint i, j;

	for (i = 0; i < decor_db.decor.size; i++) {
		decor = &decor_db.decor.data[i];
		atra_model_delete(&decor->model);

		if (decor->anim.none == FALSE) {
			atra_rawanim_delete(&decor->anim.some);
		}
		free(decor->collide_map.map);
		free(decor->name);
		free(decor->file_path);
		free(decor->model_path);
		free(decor->anim_path);

		for (j = 0; j < decor->decors.size; j++) {
			inst = &decor->decors.data[j];
			atra_list_Emitter_delete(&inst->emitters);
		}
		atra_list_DecorInst_delete(&decor->decors);
		atra_list_Emitter_delete(&decor->emitters);
		free(decor->model_mats);
	}
	atra_hashmap_delete(&decor_db.paths, 0);
}

/* Returns a pointer to a decoration type with the matching ID.
 * Returns NULL on failure. */
Decor *
decor_get_byid(uint id)
{
	return atra_list_Decor_getptr(&decor_db.decor, id);
}

/* Returns a pointer to a decoration type with the matching file path.
 * Returns NULL on failure. */
Decor *
decor_get_bypath(const char *path)
{
	const uint *id = atra_hashmap_get(&decor_db.paths, &path);
	return atra_list_Decor_getptr(&decor_db.decor, *id);
}

/* Update all decorations in the database */
void
decor_updateall(u32 dt)
{
	Decor *decor;
	uint i, j;

	for (i = 0; i < decor_db.decor.size; i++) {
		decor = &decor_db.decor.data[i];

		for (j = 0; j < decor->decors.size; j++) {
			//decor_inst_update(&decor->decors.data[j], dt);
		}
	}
}

/* Draw all decorations in the database (depth mode) */
void
decor_drawall_depth(void)
{
	DirLight * const sunlight = state_suncam_get();
	
	Decor *decor;
	mat4 lightspace;
	uint i;

    glm_mat4_mul(sunlight->proj, sunlight->view, lightspace);

	for (i = 0; i < decor_db.decor.size; i++) {
		decor = &decor_db.decor.data[i];
		decor_draw_depth(decor, lightspace);
	}
}

/* Draw all decorations in the database (forward mode) */
void
decor_drawall_forward(void)
{

	DirLight * const sunlight = state_suncam_get();
	
	Decor *decor;
	mat4 lightspace;
	uint i;

    glm_mat4_mul(sunlight->proj, sunlight->view, lightspace);

	for (i = 0; i < decor_db.decor.size; i++) {
		decor = &decor_db.decor.data[i];
		decor_draw_depth(decor, lightspace);
	}
}

/* Update a given decoration type and associated instances */
void
decor_update(Decor *decor, u32 dt)
{

}

/* Render a given decoration type and associated instances (depth mode) */
void
decor_draw_depth(Decor *decor, vec4 *lightspace)
{
	
}

/* Render a given decoration type and associated instances (forward mode) */
void
decor_draw_forward(Decor *decor, vec4 *lightspace)
{
	Camera * const cam = atra_camera_getmain();
    DirLight * const sunlight = state_suncam_get();
    const World * const world = state_world_get();
    
    //const int * const locs = ent_shader_fwd.locs;
    const int sun_enabled = (world->sun_enabled != 0);

    const vec3 ambient_color = {
        world->ambient_light.r / 255.0f,
        world->ambient_light.g / 255.0f,
        world->ambient_light.b / 255.0f};

    const vec3 sun_color = {
        (world->sun_color.r / 255.0f) * sun_enabled,
        (world->sun_color.g / 255.0f) * sun_enabled,
        (world->sun_color.b / 255.0f) * sun_enabled};

	int i, j;
/*
    EntityList *list;
    mat4 viewproj, lightspace;
    int texunit_depth;
    uint i;

    texunit_depth = ENTITY_TEXUNIT_SHADOWMAP;

    glm_mat4_mul(cam->proj, cam->view, viewproj);
    glm_mat4_mul(sunlight->proj, sunlight->view, lightspace);

    atra_shader_setactive(&ent_shader_fwd);

    atra_dirlight_binddepthtex(sunlight, ENTITY_TEXUNIT_SHADOWMAP);

    atra_shader_setuniform(locs[ENTITY_LOC_VIEWPOS], cam->pos, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_AMBIENTCOLOR], ambient_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_SUNCOLOR], sun_color, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_SUNDIR], sunlight->dir, ATRA_SHADER_UNIFORM_VEC3, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_VIEWPROJ], viewproj, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_LIGHTSPACE], lightspace, ATRA_SHADER_UNIFORM_MAT4, 1);
    atra_shader_setuniform(locs[ENTITY_LOC_TEX_SHADOWMAP], &texunit_depth, ATRA_SHADER_UNIFORM_INT, 1);

    for (i = 0; i < ent_db.size; i++) {
        list = ent_db.ent_lists + i;
        entitylist_render(list, ENTITY_DRAWMODE_FORWARD);
    }*/

	for (i = 0; i < decor->model.mesh_count; i++) {
		//TODO: Deal with instanced rendering at some point

		/* Non-instanced rendering */
	}
}

static Model
decormodel_loadfile(const char *path)
{
    Model model = atra_model_loadfile(path, 0);
    int i;

    for (i = 0; i < model.mesh_count; i++) {
        decormodel_uploadmesh(&model.meshes[i]);
    }

    return model;
}

static void
decormodel_uploadmesh(Mesh *mesh)
{
    mesh->vbo_ids = calloc(ATRA_MAX_MESH_VERTEX_BUFFERS, sizeof(uint));
    mesh->vao_id = atra_gl_vao_create();
    atra_gl_vao_enable(mesh->vao_id);

    mesh->tangents = malloc(mesh->vert_count * 3 * sizeof(float));
    atra_mesh_calctangents(mesh);

    mesh->vbo_ids[DECOR_VBO_POSITION] =
            atra_gl_vbo_create(mesh->verts, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(DECOR_VBO_POSITION, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(DECOR_VBO_POSITION);

    mesh->vbo_ids[DECOR_VBO_TEXCOORD] =
            atra_gl_vbo_create(mesh->uvs, mesh->vert_count * 2 * sizeof(float), 0);
    atra_gl_vao_setattrib(DECOR_VBO_TEXCOORD, 2, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(DECOR_VBO_TEXCOORD);

    mesh->vbo_ids[DECOR_VBO_NORMAL] =
            atra_gl_vbo_create(mesh->norms, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(DECOR_VBO_NORMAL, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(DECOR_VBO_NORMAL);

    mesh->vbo_ids[DECOR_VBO_COLOR] =
            atra_gl_vbo_create(mesh->colors, mesh->vert_count * 4 * sizeof(u8), 0);
    atra_gl_vao_setattrib(DECOR_VBO_COLOR, 4, ATRA_UNSIGNED_BYTE, 1, 0, NULL);
    atra_gl_vao_enableattrib(DECOR_VBO_COLOR);

    mesh->vbo_ids[DECOR_VBO_TANGENT] =
            atra_gl_vbo_create(mesh->tangents, mesh->vert_count * 3 * sizeof(float), 0);
    atra_gl_vao_setattrib(DECOR_VBO_TANGENT, 3, ATRA_FLOAT, 0, 0, NULL);
    atra_gl_vao_enableattrib(DECOR_VBO_TANGENT);

    mesh->vbo_ids[DECOR_VBO_INDICES] =
            atra_gl_ebo_create(mesh->indices, mesh->tri_count * 3 * sizeof(uint), 0);
}

static void
decormodel_loadtextures(Model *model, const char *albedo_path,
        const char *spec_path, const char *norm_path, const char *emission_path)
{
    const char *paths[] = {
        albedo_path,
        spec_path,
        norm_path,
        emission_path
    };

    const Color norm_color = {0x7F, 0x7F, 0xFF, 0xFF};
    const Color white_color = {0xFF, 0xFF, 0xFF, 0xFF};
    const Color black_color = {0x00, 0x00, 0x00, 0xFF};

    MaterialMap *map;
    Option_Texture maybe_tex;
    Texture tex;
    Image img;
    uint i;

    for (i = 0; i < 4; i++) {
        map = &model->materials[0].maps[i];
        maybe_tex = atra_texture_loadfile(paths[i]);
        
        if (maybe_tex.none) {
            atra_log_print(LOG_ERROR, "entity: Unable to load texture");
            
            switch (i) {
            case DECOR_TEXUNIT_ALBEDO:
            case DECOR_TEXUNIT_SPECULAR:
                img = atra_image_gencolor(white_color, 1, 1);
                break;
            case DECOR_TEXUNIT_NORMAL:
                img = atra_image_gencolor(norm_color, 1, 1);
                break;
            case DECOR_TEXUNIT_EMISSION:
                img = atra_image_gencolor(black_color, 1, 1);
                break;
            }
            tex = atra_texture_loadimage(&img);
        } else {
            tex = maybe_tex.some;
        }
        map->tex = tex;
        map->color = white_color;
        map->value = 1.0f;
    }

}


