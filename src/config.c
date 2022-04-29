/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  config.h - Functions and structures for loading engine and game configs
 * 
 */

#include "config.h"
#include "file.h"

static const char *config_tags[] = {
    "window_width",
    "window_height",
    "fullscreen",
    "vsync_enabled",
    "anisofilter_level",
    "mipmap_level",
    "shadow_quality",
    "audio_master",
    "audio_music",
    "audio_effects"
};

typedef struct ConfigData {
    uint window_width;
    uint window_height;
    uint fullscreen;
    uint vsync_enabled;
    uint anisofilter_level;
    uint mipmap_level;
    uint shadow_quality;
    uint audio_master;
    uint audio_music;
    uint audio_effects;
} ConfigData;

static const ConfigData default_config = {
    .window_width = 1280,
    .window_height = 720,
    .fullscreen = 1,
    .vsync_enabled = 1,
    .anisofilter_level = 4,
    .mipmap_level = 4,
    .shadow_quality = 2,
    .audio_master = 50,
    .audio_music = 100,
    .audio_effects = 100
};

static ConfigData config;

/* Loads the game configuration file */
void
config_load(void)
{
    FileData file;
    size_t tag_size;
    uint val, i;
    u8 *tag;

    file = file_load(CONFIG_FILE_PATH);

    if (file.data == NULL) {
        atra_log_print(LOG_WARN,
                "config: Unable to find config file: Loading default config");
        config = default_config;
        return;
    }

    while (file.head < file.size) {
        file_expect_delim(&file, "\n\t ");
        tag_size = file_expect_alpha_many(&file);

        if (tag_size == 0) {
            atra_log_print(LOG_WARN,
                    "config: Error in config file: Loading default config");
            config = default_config;
            return;
        }
        tag = file.data + file.head - tag_size;
        file_expect_delim(&file, "\n\t ");
       
        if (file_expect_byte(&file, '=')) {
            atra_log_print(LOG_WARN,
                    "config: Error in config file: Loading default config");
            config = default_config;
            return;
        }
        file_expect_delim(&file, "\n\t ");
        val = file_expect_int(&file);
        file_expect_delim(&file, "\n\t ");

        for (i = 0; i < CONFIG_TOTAL; i++) {
            if (memcmp(config_tags[i], tag,
                    MIN(tag_size, strlen(config_tags[i]))) == 0) {
                ((uint *)&config)[i] = val;
                break;
            }
        }
    }
    file_delete(file);
}

/* Saves new configuration variables to config file */
void
config_save(void)
{
    FileData file;
    char n_str[FILE_MAX_DIGITS];
    int n_len;
    uint i;

    file = file_create(0);

    for (i = 0; i < CONFIG_TOTAL; i++) {
        n_len = snprintf(n_str, FILE_MAX_DIGITS, "%u", ((uint *)&config)[i]);

        file_write_char_array(&file, strlen(config_tags[i]), config_tags[i]);
        file_write_char(&file, '=');
        file_write_char_array(&file, n_len, n_str);
        file_write_char(&file, '\n');
    }
    file_save(&file, CONFIG_FILE_PATH);
    file_delete(file);
}

/* Returns the value of a game config variable */
uint
config_get(uint type)
{
    return ((uint *)&config)[type];
}

/* Manually sets a game config variable */
void
config_set(uint type, uint value)
{
    ((uint *)&config)[type] = value;
}
