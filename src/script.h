/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  script.h - Interface for Lua run-time scripts used by the game
 *  
 *  */

#ifndef SCRIPT_H
#define SCRIPT_H

#include "atra.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

enum script_func_arg {
    SCRIPT_ARG_NONE,
    SCRIPT_ARG_I32,
    SCRIPT_ARG_I64,
    SCRIPT_ARG_U32,
    SCRIPT_ARG_U64,
    SCRIPT_ARG_F32,
    SCRIPT_ARG_F64,
    SCRIPT_ARG_BOOL,
    SCRIPT_ARG_ARRAY
};

typedef struct EventCallerListEntry {
    uint index;             /* Index of  */
    uint caller_type;
} EventCallerListEntry;

/* Initialise the Lua system */
int script_init(void);

/* Shut down the Lua system */
void script_stop(void);



/* Loads the given script file */
int script_loadscript(const char *path);

/* Send an event to a lua script with the corresponding event data */
int script_sendevent(uint e_type, void *data);

void script_stack_pop(uint n);

void script_set_global_i32(const char *var, i32 i);
void script_set_global_i64(const char *var, i64 i);
void script_set_global_u32(const char *var, u32 i);
void script_set_global_u64(const char *var, u64 i);
void script_set_global_f32(const char *var, f32 f);
void script_set_global_f64(const char *var, f64 f);
void script_set_global_bool(const char *var, int b);
void script_set_global_func(const char *var, int (*func)(lua_State *));

i32 script_get_global_i32(const char *var);
i64 script_get_global_i64(const char *var);
u32 script_get_global_u32(const char *var);
u64 script_get_global_u64(const char *var);
f32 script_get_global_f32(const char *var);
f64 script_get_global_f64(const char *var);
int script_get_global_bool(const char *var);
int script_get_global_table(const char *var);
char * script_get_global_str(const char *var);
double script_call_global_func(const char *var, int *types, ...);

i32 script_get_table_i32(const char *var, int i);
i64 script_get_table_i64(const char *var, int i);
u32 script_get_table_u32(const char *var, int i);
u64 script_get_table_u64(const char *var, int i);
f32 script_get_table_f32(const char *var, int i);
f64 script_get_table_f64(const char *var, int i);
int script_get_table_bool(const char *var, int i);
int script_get_table_table(const char *var, int i);
char * script_get_table_str(const char *var, int i);

i32 script_get_array_i32(uint index, int i);
i64 script_get_array_i64(uint index, int i);
u32 script_get_array_u32(uint index, int i);
u64 script_get_array_u64(uint index, int i);
f32 script_get_array_f32(uint index, int i);
f64 script_get_array_f64(uint index, int i);
int script_get_array_bool(uint index, int i);
char * script_get_array_str(uint index, int i);


#endif /* SCRIPT_H */
