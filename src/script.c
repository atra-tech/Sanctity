/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  script.c - Interface for Lua run-time scripts used by the game
 *  
 *  */

#include "script.h"

static lua_State *lua;


/*
 *  When loading a script for some type of object:
 *      Check each relevant event function string to see if it is defined
 *      If the function is defined:
 *          Add entity ID to corresponding event list
                (better than flags for fast access)
                (also takes up a big chunk of memory)
 *
 */

/* Initialise the Lua system */
int
script_init(void)
{
    lua = luaL_newstate();
    luaL_openlibs(lua);

    /* TODO: Redo above definitions to factor in different caller types */
    /* TODO: Populate lists with entities which have the corresponding function */
    /* TODO: Maybe use a hashtable using the entity type's ID number as a key */
    /* TODO: Maybe just use a bitfield of flags in the database to decide if
     *       object type has event or not */
    
    return 1;
}

/* Shut down the Lua system */
void
script_stop(void)
{
    lua_close(lua);
}

/* Loads the given script file */
int
script_loadscript(const char *path)
{
    if (luaL_dofile(lua, path) == LUA_OK) {
        return 1;
    } else {
        atra_log_print(LOG_ERROR, "script: Error loading lua script file\n");
        luaL_error(lua, "lua: %s\n", lua_tostring(lua, -1));
        return 0;
    }
}

/* Send an event to a lua script with the corresponding event data */
int
script_sendevent(uint e_type, void *data)
{

}


void
script_stack_pop(uint n)
{
    lua_pop(lua, n);
}

void
script_set_global_i32(const char *var, i32 i)
{
    lua_pushnumber(lua, i);
    lua_setglobal(lua, var);
}

void
script_set_global_i64(const char *var, i64 i)
{
    lua_pushnumber(lua, i);
    lua_setglobal(lua, var);
}

void
script_set_global_u32(const char *var, u32 i)
{
    lua_pushnumber(lua, i);
    lua_setglobal(lua, var);
}

void
script_set_global_u64(const char *var, u64 i)
{
    lua_pushnumber(lua, i);
    lua_setglobal(lua, var);
}

void
script_set_global_f32(const char *var, f32 f)
{

    lua_pushnumber(lua, f);
    lua_setglobal(lua, var);
}

void
script_set_global_f64(const char *var, f64 f)
{
    lua_pushnumber(lua, f);
    lua_setglobal(lua, var);
}

void
script_set_global_bool(const char *var, int b)
{
    lua_pushboolean(lua, b);
    lua_setglobal(lua, var);
}

void
script_set_global_func(const char *var, int (*func)(lua_State *))
{
    lua_pushcfunction(lua, func);
    lua_setglobal(lua, var);
}

i32
script_get_global_i32(const char *var)
{
    lua_getglobal(lua, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

i64
script_get_global_i64(const char *var)
{
    lua_getglobal(lua, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

u32
script_get_global_u32(const char *var)
{
    lua_getglobal(lua, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

u64
script_get_global_u64(const char *var)
{
    lua_getglobal(lua, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

f32
script_get_global_f32(const char *var)
{
    lua_getglobal(lua, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

f64
script_get_global_f64(const char *var)
{
    lua_getglobal(lua, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

int
script_get_global_bool(const char *var)
{
    lua_getglobal(lua, var);
    int b = lua_toboolean(lua, -1);
    lua_pop(lua, 1);
    return b;
}

int
script_get_global_table(const char *var)
{
    lua_getglobal(lua, var);

    if (lua_istable(lua, -1) == 0) {
        atra_log_print(LOG_WARN, "script: No such table: \"%s\"", var);
        return 0;
    }

    return 1;
}

char *
script_get_global_str(const char *var)
{
    lua_getglobal(lua, var);

    if (lua_isstring(lua, -1) == 0) {
        atra_log_print(LOG_WARN, "script: No such string: \"%s\"", var);
        lua_pop(lua, 1);
        return NULL;
    }

    const char *lua_str = lua_tostring(lua, -1);
    size_t len = strlen(lua_str) + 1;

    char *str = malloc(len * sizeof(char));
    memcpy(str, lua_str, len);
    lua_pop(lua, 1);

    return str;
}

double
script_call_global_func(const char *var, int *types, ...)
{
    va_list args;
    int i, arg_count = 0;
    double result = 0;

    va_start(args, types);

    lua_getglobal(lua, var);

    if (lua_isfunction(lua, -1)) {
        /* TODO: Figure out how to push arguments */
        for (i = 0; types[i] != SCRIPT_ARG_NONE; i++) {
            switch (types[i]) {
            case SCRIPT_ARG_NONE:
                break;
            case SCRIPT_ARG_I32:
                lua_pushnumber(lua, va_arg(args, i32));
                break;
            case SCRIPT_ARG_I64:
                lua_pushnumber(lua, va_arg(args, i64));
                break;
            case SCRIPT_ARG_U32:
                lua_pushnumber(lua, va_arg(args, u32));
                break;
            case SCRIPT_ARG_U64:
                lua_pushnumber(lua, va_arg(args, u64));
                break;
            case SCRIPT_ARG_F32:
            case SCRIPT_ARG_F64:
                lua_pushnumber(lua, va_arg(args, f64));
                break;
            case SCRIPT_ARG_BOOL:
                lua_pushboolean(lua, va_arg(args, i32));
                break;
            case SCRIPT_ARG_ARRAY:
                /* TODO: Figure out array method */
                break;
            }
            arg_count++;
        }
        lua_pcall(lua, arg_count, 1, 0);
        result = lua_tonumber(lua, -1);
        lua_pop(lua, 1);
    } else {
        atra_log_print(LOG_WARN, "script: No such function: \"%s\"", var);
    }
    va_end(args);

    return result;
}



i32
script_get_table_i32(const char *var, int i)
{
    lua_getfield(lua, i, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

i64
script_get_table_i64(const char *var, int i)
{
    lua_getfield(lua, i, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

u32
script_get_table_u32(const char *var, int i)
{
    lua_getfield(lua, i, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

u64
script_get_table_u64(const char *var, int i)
{
    lua_getfield(lua, i, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

f32
script_get_table_f32(const char *var, int i)
{
    lua_getfield(lua, i, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

f64
script_get_table_f64(const char *var, int i)
{
    lua_getfield(lua, i, var);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

int
script_get_table_bool(const char *var, int i)
{
    lua_getfield(lua, i, var);
    int b = lua_toboolean(lua, -1);
    lua_pop(lua, 1);
    return b;
}

int
script_get_table_table(const char *var, int i)
{
    lua_getfield(lua, i, var);

    if (lua_istable(lua, -1) == 0) {
        atra_log_print(LOG_WARN, "script: No such table: \"%s\"", var);
        return 0;
    }

    return 1;
}

char *
script_get_table_str(const char *var, int i)
{
    lua_getfield(lua, i, var);

    if (lua_isstring(lua, -1) == 0) {
        atra_log_print(LOG_WARN, "script: No such string: \"%s\"", var);
        lua_pop(lua, 1);
        return NULL;
    }

    const char *lua_str = lua_tostring(lua, -1);
    size_t len = strlen(lua_str) + 1;

    char *str = malloc(len * sizeof(char));
    memcpy(str, lua_str, len);
    lua_pop(lua, 1);

    return str;
}



i32
script_get_array_i32(uint index, int i)
{
    lua_rawgeti(lua, i, index);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

i64
script_get_array_i64(uint index, int i)
{
    lua_rawgeti(lua, i, index);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

u32
script_get_array_u32(uint index, int i)
{
    lua_rawgeti(lua, i, index);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

u64
script_get_array_u64(uint index, int i)
{
    lua_rawgeti(lua, i, index);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

f32
script_get_array_f32(uint index, int i)
{
    lua_rawgeti(lua, i, index);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

f64
script_get_array_f64(uint index, int i)
{
    lua_rawgeti(lua, i, index);
    lua_Number num = lua_tonumber(lua, -1);
    lua_pop(lua, 1);
    return num;
}

int
script_get_array_bool(uint index, int i)
{
    lua_rawgeti(lua, i, index);
    int b = lua_toboolean(lua, -1);
    lua_pop(lua, 1);
    return b;
}

char *
script_get_array_str(uint index, int i)
{
    lua_rawgeti(lua, i, index);

    if (lua_isstring(lua, -1) == 0) {
        atra_log_print(LOG_WARN, "script: No string at index: %d", index);
        lua_pop(lua, 1);
        return NULL;
    }

    const char *lua_str = lua_tostring(lua, -1);
    size_t len = strlen(lua_str) + 1;

    char *str = malloc(len * sizeof(char));
    memcpy(str, lua_str, len);
    lua_pop(lua, 1);

    return str;
}
