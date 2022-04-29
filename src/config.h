/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  config.h - Functions and structures for loading engine and game configs
 * 
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "atra.h"

#define CONFIG_FILE_PATH    "res/settings.cfg"

enum config_entry_type {
    CONFIG_WINDOW_WIDTH,
    CONFIG_WINDOW_HEIGHT,
    CONFIG_FULLSCREEN,
    CONFIG_VSYNC,
    CONFIG_ANISOFILTER,
    CONFIG_MIPMAP_LEVEL,
    CONFIG_SHADOW_QUALITY,
    CONFIG_AUDIO_MASTER,
    CONFIG_AUDIO_MUSIC,
    CONFIG_AUDIO_EFFECTS,
    CONFIG_TOTAL
};

/* Loads the game configuration file */
void config_load(void);

/* Saves new configuration variables to config file */
void config_save(void);

/* Returns the value of a game config variable */
uint config_get(uint type);

/* Manually sets a game config variable */
void config_set(uint type, uint value);

#endif /* CONFIG_H */
