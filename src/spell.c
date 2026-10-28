/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  spell.c - Resources and functions relating to abilities and spells
 *
 */

#include "spell.h"
#include "file.h"

ATRA_OPTION_DEFINE(SpellInst)
ATRA_LIST_DEFINE(Option_SpellInst)

ATRA_OPTION_DEFINE(Spell)
ATRA_LIST_DEFINE(Spell)
ATRA_LIST_DEFINE(Option_uint)

static const Color default_color = {0xFF, 0x00, 0xFF, 0xFF};

SpellDB spell_db;

static Spell spell_create(void);
static Spell * spell_pushtodatabase(Spell *spell);


/* Initialise the spell system */
void
spell_init(void)
{
    spell_db.spells = atra_list_Spell_create(0);
    spell_db.path_map = atra_hashmap_create(sizeof(char *), sizeof(uint), atra_hashmap_str_hash, atra_hashmap_str_eq);
}

/* Shutdown the spell system */
void
spell_shutdown(void)
{
    atra_list_Spell_delete(&spell_db.spells);
    atra_hashmap_delete(&spell_db.path_map, HASH_FREE_KEY);
}

/* Create a new, empty spell type */
static Spell
spell_create(void)
{
    Spell spell = {0};

    spell.enabled = 1;
    spell.prj_id.none = 1;
    spell.prt_ids = atra_list_uint_create(0);
    spell.prt_counts = atra_list_uint_create(0);

    return spell;
}

/* Add the given spell to the spell database.
 * Returns a pointer to the spell in the database. */
static Spell *
spell_pushtodatabase(Spell *spell)
{
    const uint index = spell_db.spells.size;
    const char *path = atra_util_strdup(spell->file_path);

    atra_list_Spell_add(&spell_db.spells, *spell);
    atra_hashmap_add(&spell_db.path_map, &path, &index);
    spell->id = index;

    return &spell_db.spells.data[index];
}

/* Marshall a new spell instance with the given attributes.
 * Returns the resulting spell and registers it in the database. */
Spell *
spell_new(uint cast_type, uint team, uint attrs, const char *name,
        const char *file_path, const char *img_path, Option_uint prj_id,
        uint stack, i32 cooldown, i32 casttime)
{
    Spell spell = spell_create();
    Option_Image img;

    spell.cast_type = cast_type;
    spell.team = team;
    spell.attrs = attrs;
    spell.name = atra_util_strdup(name);
    spell.file_path = atra_util_strdup(file_path);
    
    img = atra_image_loadfile(img_path);
    spell.img_path = atra_util_strdup(img_path);

    if (img.none) {
        atra_log_print(LOG_WARN, "spell: Unable to load UI icon: \"%s\"", img_path);
        atra_log_print(LOG_WARN, "spell: Falling back on default icon for spell: \"%s\"", name);
        spell.spell_img = atra_image_gencolor(default_color, 1, 1);
    } else {
        spell.spell_img = img.some;
    }
    spell.spell_tex = atra_texture_loadimage(&spell.spell_img);
    spell.prj_id = prj_id;
    spell.stack_max = stack;
    spell.cooldown_max = cooldown;
    spell.casttime_max = casttime;

    return spell_pushtodatabase(&spell);
}

/* Delete an existing spell by it's ID */
void
spell_delete(uint id)
{
    Spell *spell = spell_get_byid(id);

    atra_hashmap_remove(&spell_db.path_map, &spell->file_path, HASH_FREE_KEY);

    free(spell->name);
    free(spell->file_path);
    free(spell->img_path);

    atra_image_destroy(&spell->spell_img);
    atra_texture_destroy(&spell->spell_tex);
    atra_list_uint_delete(&spell->prt_ids);
    atra_list_uint_delete(&spell->prt_counts);

    atra_list_Spell_remove(&spell_db.spells, id);
}

/* Load a spell into the database from a file at the given path.
 * Returns a pointer to the loaded spell, if possible. */
Spell *
spell_loadfile(const char *path)
{
    Spell spell = spell_create();
    FileData file = file_load(path);
    Option_Image img;
    char *res_path;
    uint particle_count, i;

    spell.cast_type = file_read_uint(&file);
    spell.team = file_read_uint(&file);
    spell.attrs = file_read_uint(&file);
    spell.name = file_read_string(&file);
    spell.file_path = atra_util_strdup(path);

    spell.img_path = file_read_string(&file);
    img = atra_image_loadfile(spell.img_path);

    if (img.none) {
        atra_log_print(LOG_WARN, "spell: Unable to load UI icon: \"%s\"", spell.img_path);
        atra_log_print(LOG_WARN, "spell: Falling back on default icon for spell: \"%s\"", spell.name);
        spell.spell_img = atra_image_gencolor(default_color, 1, 1);
    } else {
        spell.spell_img = img.some;
    }
    spell.spell_tex = atra_texture_loadimage(&spell.spell_img);
    res_path = file_read_string(&file);

    if (res_path != NULL) {
        spell.prj_id = (Option_uint) {
            .some = projectile_getid_bypath(res_path),
            .none = FALSE
        };
        free(res_path);
    } else {
        spell.prj_id = (Option_uint) { .none = TRUE };
    }
    spell.stack_max = file_read_uint(&file);
    spell.cooldown_max = file_read_i32(&file);
    spell.casttime_max = file_read_i32(&file);
    
    particle_count = file_read_uint(&file);

    for (i = 0; i < particle_count; i++) {
        res_path = file_read_string(&file);
        atra_list_uint_add(&spell.prt_ids, db_particle_getid_bypath(res_path));
        atra_list_uint_add(&spell.prt_counts, file_read_uint(&file));
        free(res_path);
    }
    file_delete(file);

    return spell_pushtodatabase(&spell);
}

/* Save a spell to the given file path */
void
spell_savefile(uint id)
{
    Spell *spell = spell_get_byid(id);
    FileData file = file_create(0);
    ProjectileData *proj;
    ParticleList *prt;
    uint i;

    file_write_uint(&file, spell->cast_type);
    file_write_uint(&file, spell->team);
    file_write_uint(&file, spell->attrs);
    file_write_string(&file, spell->name);

    file_write_string(&file, spell->img_path);

    if (spell->prj_id.none) {
        file_write_string_empty(&file);
    } else {
        proj = projectile_getlist(spell->prj_id.some);
        file_write_string(&file, proj->file_path);
    }
    file_write_uint(&file, spell->stack_max);
    file_write_i32(&file, spell->cooldown_max);
    file_write_i32(&file, spell->casttime_max);
    file_write_uint(&file, spell->prt_ids.size);

    for (i = 0; i < spell->prt_ids.size; i++) {
        prt = db_particle_getlist(spell->prt_ids.data[i]);
        file_write_string(&file, prt->file_path);
        file_write_uint(&file, spell->prt_counts.data[i]);
    }
    file_save(&file, spell->file_path);
    file_delete(file);
}

/* Return a pointer to a spell with the given ID */
Spell *
spell_get_byid(uint id)
{
    return &spell_db.spells.data[id];
}

/* Return a pointer to a spell with the given file path.
 * Implicitly loads a spell from the given path if no matching spell exists */
Spell *
spell_get_bypath(const char *path)
{
    const uint *id = atra_hashmap_get(&spell_db.path_map, &path);

    return spell_get_byid(*id);
}


/* Create a new spell instance with the given attributes */
SpellInst
spell_inst_new(uint spell_id, uint level)
{
    const Spell * const spell = spell_get_byid(spell_id);
    const SpellInst inst = {
        .spell_id = spell_id,
        .level = level,
        .state = SPELL_STATE_IDLE,
        .stacks = spell->stack_max,
        .cooldown = 0,
        .casttime = 0
    };

    return inst;
}

/* Update the given spell instance.
 * Returns true if the spell state changes */
int
spell_inst_update(SpellInst *inst, u32 dt)
{
    Spell *spell = spell_get_byid(inst->spell_id);
    int state_change = FALSE;
    
    if (inst->cooldown > 0) {
        if (inst->cooldown - dt <= 0) {
            if (inst->stacks < spell->stack_max) {
                inst->stacks++;
            }
            if (inst->stacks < spell->stack_max) {
                inst->cooldown = spell->cooldown_max;
            }
        } else {
            inst->cooldown -= dt;
        }
    }

    if (inst->casttime > 0 && inst->state == SPELL_STATE_CHARGE) {
        if (inst->casttime - dt <= 0) {
            inst->state = SPELL_STATE_RELEASE;
            state_change = TRUE;
            inst->stacks--;
        } else {
            inst->casttime -= dt;
        }
    }
    
    return state_change;
}

/* Refresh the given spell's cooldown */
void
spell_inst_refresh(SpellInst *inst)
{
    Spell *spell = spell_get_byid(inst->spell_id);

    inst->state = SPELL_STATE_IDLE;
    inst->stacks = spell->stack_max;
    inst->cooldown = 0;
    inst->casttime = 0;
}

/* Begins spell charging phase (Spell must be idle) */
void
spell_inst_charge(SpellInst *inst)
{
    Spell *spell = spell_get_byid(inst->spell_id);

    if (inst->state == SPELL_STATE_IDLE) {
        inst->state = SPELL_STATE_CHARGE;
        inst->casttime = spell->casttime_max;
    }
}

/* Begins spell release phase (Cast-point dictated by casting entity) */
void
spell_inst_release(SpellInst *inst)
{
    if (inst->state == SPELL_STATE_CHARGE) {
        inst->state = SPELL_STATE_RELEASE;
    }
}

/* Halts casting and resets the spell if invoked during spell charging */
void
spell_inst_halt(SpellInst *inst)
{
    if (inst->state == SPELL_STATE_CHARGE) {
        inst->state = SPELL_STATE_IDLE;
        inst->casttime = 0;
    }
}


/* Create a new spell bar with the given size */
SpellBar
spellbar_new(uint size)
{
    return (SpellBar) {
        .size = size,
        .lock = 0,
        .spells = malloc(size * sizeof(Option_SpellInst))
    };
}

/* Delete the given spell bar */
void
spellbar_delete(SpellBar *bar)
{
    free(bar->spells);
}

/* Resize the given spell bar */
void
spellbar_resize(SpellBar *bar, uint new_size)
{
    bar->size = new_size;
    bar->spells = realloc(bar->spells, bar->size * sizeof(Option_SpellInst));
}

/* Set a particular spell slot to the given spell.
 * Returns 0 on success, returns 1 if locked, returns 2 if duplicate. */
int
spellbar_slot_set(SpellBar *bar, uint index, uint spell_id)
{
    uint i;
    Option_SpellInst *inst;

    if (bar->lock) return 1;

    for (i = 0; i < bar->size; i++) {
        inst = &bar->spells[i];
        
        if (inst->none == 0) {
            if (inst->some.spell_id == spell_id && index != i) {
                return 2;
            }
        }
    }
    bar->spells[index] = (Option_SpellInst) {
        .some = spell_inst_new(spell_id, 0),
        .none = 0
    };

    return 0;
}

/* Clears the specified spell slot */
void
spellbar_slot_clear(SpellBar *bar, uint index)
{
    /* TODO: Require: Spell inst functions */
    bar->spells[index].none = 1;
}

/* Lock the spellbar, preventing player from modifying it */
void
spellbar_lock(SpellBar *bar)
{
    bar->lock = TRUE;
}

/* Unlock the spellbar, allowing player to modify it */
void
spellbar_unlock(SpellBar *bar)
{
    bar->lock = FALSE;
}

/* Immediately refresh all spells on the given spellbar */
void
spellbar_refreshall(SpellBar *bar)
{
    /* TODO: Require: Spell inst functions */
    uint i;
    Option_SpellInst *inst;

    for (i = 0; i < bar->size; i++) {
        inst = &bar->spells[i];
        if (inst->none) continue;
        spell_inst_refresh(&inst->some);
    }
}

/* Iterate over all the spells and update them */
void
spellbar_update(SpellBar *bar, u32 dt)
{
    uint i;
    Option_SpellInst *inst;

    for (i = 0; i < bar->size; i++) {
        inst = &bar->spells[i];
        if (inst->none) continue;
        spell_inst_update(&inst->some, dt);
    }
}

/* Set the spell at the given index to charge */
void
spellbar_charge(SpellBar *bar, uint index)
{
    Option_SpellInst * const inst = &bar->spells[index];

    if (inst->none) return;
    spell_inst_charge(&inst->some);
}

/* Set the spell at the given index to release */
void
spellbar_release(SpellBar *bar, uint index)
{
    Option_SpellInst * const inst = &bar->spells[index];

    if (inst->none) return;
    spell_inst_release(&inst->some);
}

/* Cancel charging for the spell at the given index */
void
spellbar_haltcast(SpellBar *bar, uint index)
{
    Option_SpellInst * const inst = &bar->spells[index];

    if (inst->none) return;
    spell_inst_halt(&inst->some);
}


