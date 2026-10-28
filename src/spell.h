/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  spell.h - Resources and functions relating to abilities and spells
 *
 */

/*
 *  REQUIREMENTS:
 *      GUI Image Ability Icons
 *      Projectile spawning
 *      Cooldowns
 *      Types (Single, Continuous)
 */

/*  OTHER STUFF IN OTHER SYSTEMS:
 *      Buffs/Debuffs
 */

#ifndef SPELL_H
#define SPELL_H

#include "atra.h"
#include "projectile.h"
#include "particle.h"

/*  TODO LIST:
 *      - Add buffs/debuff applications (through projectiles)
 *          + Add buff/debuff system
 */

/* Type of spell, corresponding to animations to be used for it */
enum spell_cast_type {
    SPELL_CAST_TARGET,      /* Spell is a simple, targeted projectile/effect */
    SPELL_CAST_TRAP,        /* Spell lays a trap on the ground */
    SPELL_CAST_AREA,        /* Spell leaves a temporary AoE effect */
    SPELL_CAST_BUFF         /* Spell buffs/debuffs */
};

/* Type of entity the spell should affect based on the entity's team */
enum spell_teamaffect {
    SPELL_HOSTILE,          /* Spell should only affect those hostile to caster */
    SPELL_FRIENDLY,         /* Spell should only affect those friendly to caster */
    SPELL_ALL               /* Spell should affect all entities, including themselves */
};

enum spell_state {
    SPELL_STATE_IDLE,       /* Spell is not being cast */
    SPELL_STATE_CHARGE,     /* Spell is being charged for a cast (interruptable) */
    SPELL_STATE_RELEASE     /* Spell is being cast (uninterruptable) */
};


/* Instance of a spell type */
typedef struct SpellInst {
    uint spell_id;          /* Index of the spell's type in the database */
    uint level;             /* Current power level of spell */
    uint state;             /* Current stage of spell casting */
    uint stacks;            /* Current number of cast stacks */
    i32 cooldown;           /* Current time remaining on cooldown */
    i32 casttime;           /* Current time remaining on cast charge */
} SpellInst;

ATRA_OPTION_DECLARE(SpellInst)
ATRA_LIST_DECLARE(Option_SpellInst)

/* Spells casted by player/entities */
typedef struct Spell {
    uint id;                /* Index of spell in the database */
    uint enabled;           /* Set when spell is owned by the player */

    uint cast_type;         /* Spell category and cast animation behavior */
    uint team;              /* What team, relative to caster, the spell should affect */
    uint attrs;             /* Spell attributes */
    char *name;             /* Display name of spell */
    char *file_path;        /* Path to file where spell data is stored */

    Image spell_img;        /* Image of spell UI icon */
    Texture spell_tex;      /* Texture of spell UI icon */
    char *img_path;         /* Path to spell icon image file */

    Option_uint prj_id;     /* Projectile produced by spell */

    uint stack_max;         /* Maximum number of stacking spell casts */
    i32 cooldown_max;       /* Time it takes for spell cooldown to complete */
    i32 casttime_max;       /* Time it takes for spell casting to complete */

    List_uint prt_ids;      /* List of particles to emit while casting */
    List_uint prt_counts;   /* List of number of each particle type to produce */
} Spell;

ATRA_OPTION_DECLARE(Spell)
ATRA_LIST_DECLARE(Spell)
ATRA_LIST_DECLARE(Option_uint)

/* Database of spells loaded into the game */
typedef struct SpellDB {
    List_Spell spells;
    HashMap path_map;
} SpellDB;

/* Container for entity's equipped spells */
typedef struct SpellBar {
    uint size;                  /* Current size of spell bar */
    uint lock;                  /* Disables ability for player to change spells when set */
    Option_SpellInst *spells;   /* List of spell instances */
    Option_uint current;        /* Current spell being charged/released */
} SpellBar;


/* Initialise the spell system */
void spell_init(void);

/* Shutdown the spell system */
void spell_shutdown(void);

/* Marshall a new spell instance with the given attributes.
 * Returns the resulting spell and registers it in the database. */
Spell * spell_new(uint cast_type, uint team, uint attrs, const char *name,
        const char *file_path, const char *img_path, Option_uint prj_id,
        uint stack, i32 cooldown, i32 casttime);

/* Delete an existing spell by it's ID */
void spell_delete(uint id);

/* Load a spell into the database from a file at the given path.
 * Returns a pointer to the loaded spell, if possible. */
Spell * spell_loadfile(const char *path);

/* Save a spell to the given file path */
void spell_savefile(uint id);

/* Return a pointer to a spell with the given ID */
Spell * spell_get_byid(uint id);

/* Return a pointer to a spell with the given file path.
 * Implicitly loads a spell from the given path if no matching spell exists */
Spell * spell_get_bypath(const char *path);


/* Create a new spell instance with the given attributes */
SpellInst spell_inst_new(uint spell_id, uint level);

/* Update the given spell instance */
int spell_inst_update(SpellInst *inst, u32 dt);

/* Refresh the given spell's cooldown */
void spell_inst_refresh(SpellInst *inst);

/* Begins spell charging phase */
void spell_inst_charge(SpellInst *inst);

/* Begins spell release phase (cast-point dictated by casting entity)*/
void spell_inst_release(SpellInst *inst);

/* Halts casting and resets the spell if invoked during charging phase */
void spell_inst_halt(SpellInst *inst);


/* Create a new spell bar with the given size */
SpellBar spellbar_new(uint size);

/* Delete the given spell bar */
void spellbar_delete(SpellBar *bar);

/* Resize the given spell bar */
void spellbar_resize(SpellBar *bar, uint new_size);

/* Set a particular spell slot to the given spell */
int spellbar_slot_set(SpellBar *bar, uint index, uint spell_id);

/* Clears the specified spell slot */
void spellbar_slot_clear(SpellBar *bar, uint index);

/* Lock the spellbar, preventing player from modifying it */
void spellbar_lock(SpellBar *bar);

/* Unlock the spellbar, allowing player to modify it */
void spellbar_unlock(SpellBar *bar);

/* Immediately refresh all spells on the given spellbar */
void spellbar_refreshall(SpellBar *bar);

/* Iterate over all the spells and update them */
void spellbar_update(SpellBar *bar, u32 dt);

/* Set the spell at the given index to charge */
void spellbar_charge(SpellBar *bar, uint index);

/* Set the spell at the given index to release */
void spellbar_release(SpellBar *bar, uint index);

/* Cancel charging for the spell at the given index */
void spellbar_haltcast(SpellBar *bar, uint index);


#endif /* SPELL_H */
