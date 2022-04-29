/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  file.c - Set of tools for facilitating reading and writing files
 *           Also provides some parsing tools useful to custom file formats.
 * 
 */

#include "file.h"

#define FILE_READ_FUNC_CREATE(type) \
    type file_read_##type(FileData *file) \
    { \
        type i; \
        \
        if (file->head + sizeof(type) > file->size) { \
            atra_log_print(LOG_WARN, \
                    "file: Read overrun when reading type: " #type); \
            return 0; \
        } else { \
            i = *(type *)(file->data + file->head); \
            file->head += sizeof(type); \
        } \
        \
        return i; \
    }

#define FILE_READ_ARRAY_FUNC_CREATE(type) \
    void file_read_##type##_array(FileData *file, size_t n, type *data) \
    { \
        if (file->head + n * sizeof(type) > file->size) { \
            atra_log_print(LOG_WARN, \
                    "file: Read overrun when reading array of type: " #type); \
            return; \
        } \
        memcpy(data, file->data + file->head, n * sizeof(type)); \
        file->head += n * sizeof(type); \
    }

#define FILE_WRITE_FUNC_CREATE(type) \
    void file_write_##type(FileData *file, type data) \
    { \
        if (file->head + sizeof(type) > file->cap) { \
            file_grow(file); \
        } \
        *(type *)(file->data + file->head) = data; \
        file->head += sizeof(type); \
        file->size = MAX(file->head, file->size); \
    }

#define FILE_WRITE_ARRAY_FUNC_CREATE(type) \
    void file_write_##type##_array(FileData *file, size_t n, const type *data) \
    { \
        while (file->head + n * sizeof(type) > file->cap) { \
            file_grow(file); \
        } \
        memcpy(file->data + file->head, data, n * sizeof(type)); \
        file->head += n * sizeof(type); \
    }


/* Creates a new file data structure with given size.
 * Size of 0 falls back to defaults.
 * It is preferable to use sizes of 2^n, but not essential.
 */
FileData
file_create(size_t size)
{
    FileData file;

    file.cap = DECIDE(size == 0, FILE_DEFAULT_CAP, size);
    file.size = 0;
    file.head = 0;
    file.data = malloc(file.cap);

    return file;
}

/* Deletes an initialised file data structure */
void
file_delete(FileData file)
{
    free(file.data);
}

/* Reads a file into a file data structure */
FileData
file_load(const char *path)
{
    FileData file;
    size_t size = 0;
    u8 *data;

    data = atra_util_loadfile(path, &size);
    file.cap = size;
    file.size = size;
    file.head = 0;
    file.data = data;

    return file;
}

/* Saves a file data structure into a file.  Returns 1 on success. */
void
file_save(FileData *file, const char *path)
{
    atra_util_savefile(path, file->data, file->size);
}

/* Increases the size cap of a file data structure */
void
file_grow(FileData *file)
{
    size_t n = file->cap;

    /* Finds the next 2^n capacity */
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;

    if (n == 0) {
        atra_log_print(LOG_ERROR,
                "file: File has reached maximum size: refusing growth");
        return;
    }
    file->data = realloc(file->data, n);
    file->cap = n;

    atra_util_checkalloc(file->data);
}

/* Sets the read/write head back to the start of the file */
void
file_rewind(FileData *file)
{
    file->head = 0;
}

/* Returns 1 if the given byte is at the read/write head, otherwise 0 */
int
file_expect_byte(FileData *file, u8 b)
{
    const int result = *(file->data + file->head) == b;

    file->head++;

    return result;
}

/* Returns the length of a contiguous string of bytes matching the given byte */
size_t
file_expect_byte_many(FileData *file, u8 b)
{
    int result = 0;

    while (*(file->data + file->head) == b) {
        result++;
        file->head++;
    }

    return result;
}

/* Returns non-zero if the byte at the read/write head is an alphabetic char */
int
file_expect_alpha(FileData *file)
{
    u8 b = *(file->data + file->head);

    return ((b >= 'A' && b <= 'Z') ||
            (b >= 'a' && b <= 'z') ||
            (b == '_'));
}

/* Returns the length of an expected alphabetic string at the head */
size_t
file_expect_alpha_many(FileData *file)
{
    size_t size = 0;
    u8 b = *(file->data + file->head);

    while ( (b >= 'A' && b <= 'Z') ||
            (b >= 'a' && b <= 'z') ||
            (b == '_')) {
        file->head++;
        size++;
        b = *(file->data + file->head);
    }

    return size;
}

/* Returns 1 if the byte at the read/write head is an alpha-numeric character */
int
file_expect_alphanum(FileData *file)
{
    u8 b = *(file->data + file->head);

    return ((b >= 'A' && b <= 'Z') ||
            (b >= 'a' && b <= 'z') ||
            (b >= '0' && b <= '9') ||
            (b == '_'));
}

/* Returns the length of an expected alpha-numeric string at the head */
size_t
file_expect_alphanum_many(FileData *file)
{
    size_t size = 0;
    u8 b = *(file->data + file->head);

    while ( (b >= 'A' && b <= 'Z') ||
            (b >= 'a' && b <= 'z') ||
            (b >= '0' && b <= '9') ||
            (b == '_')) {
        file->head++;
        size++;
        b = *(file->data + file->head);
    }

    return size;
}

/* Returns the value of an expected integer */
int
file_expect_int(FileData *file)
{
    char num_str[FILE_MAX_DIGITS];
    u8 b = *(file->data + file->head);
    int size = 0;

    while (b >= '0' && b <= '9') {
        size++;
        b = *(file->data + file->head + size);
    }

    if (size > FILE_MAX_DIGITS) {
        atra_log_print(LOG_WARN, "file: Number conversion string exceeded max length");
    }
    strncpy(num_str, (char *)(file->data + file->head), MIN(size, FILE_MAX_DIGITS));
    num_str[MIN(size, FILE_MAX_DIGITS - 1)] = '\0';
    file->head += size;
    
    return atoi(num_str);
}

/* Returns the value of an expected floating-point number */
double
file_expect_float(FileData *file)
{
    char num_str[FILE_MAX_DIGITS];
    u8 b = *(file->data + file->head);
    int size = 0;
    int got_dot = 0;

    while ((b >= '0' && b <= '9') || b == '.') {
        if (b == '.' && got_dot) {
            break;
        } else if (b == '.' && !got_dot) {
            got_dot = 1;
        }
        size++;
        b = *(file->data + file->head + size);
    }

    if (size > FILE_MAX_DIGITS) {
        atra_log_print(LOG_WARN, "file: Number conversion string exceeded max length");
    }
    strncpy(num_str, (char *)(file->data + file->head), MIN(size, FILE_MAX_DIGITS));
    num_str[MIN(size, FILE_MAX_DIGITS - 1)] = '\0';
    file->head += size;

    return atof(num_str);
}

/* Skips over any characters in the given delimiter string until no match is found.
 * Returns the number of characters skipped. */
size_t
file_expect_delim(FileData *file, const char *delims)
{
    const ulong d_len = strlen(delims);
    size_t skip_size = 0;
    int match_found = 0;
    ulong i, j;
    u8 b;

    for (i = file->head; i < file->size; i++) {
        b = *(file->data + file->head);
        for (j = 0; j < d_len; j++) {
            match_found |= (b == delims[j]);
        }

        if (match_found == 0) {
            break;
        }
        match_found = 0;
        file->head++;
        skip_size++;
    }
    file->head += skip_size;

    return skip_size;
}


FILE_READ_FUNC_CREATE(i8)
FILE_READ_FUNC_CREATE(u8)
FILE_READ_FUNC_CREATE(i16)
FILE_READ_FUNC_CREATE(u16)
FILE_READ_FUNC_CREATE(i32)
FILE_READ_FUNC_CREATE(u32)
FILE_READ_FUNC_CREATE(i64)
FILE_READ_FUNC_CREATE(u64)
FILE_READ_FUNC_CREATE(char)
FILE_READ_FUNC_CREATE(uchar)
FILE_READ_FUNC_CREATE(short)
FILE_READ_FUNC_CREATE(ushort)
FILE_READ_FUNC_CREATE(int)
FILE_READ_FUNC_CREATE(uint)
FILE_READ_FUNC_CREATE(long)
FILE_READ_FUNC_CREATE(ulong)
FILE_READ_FUNC_CREATE(float)
FILE_READ_FUNC_CREATE(double)

FILE_READ_ARRAY_FUNC_CREATE(i8)
FILE_READ_ARRAY_FUNC_CREATE(u8)
FILE_READ_ARRAY_FUNC_CREATE(i16)
FILE_READ_ARRAY_FUNC_CREATE(u16)
FILE_READ_ARRAY_FUNC_CREATE(i32)
FILE_READ_ARRAY_FUNC_CREATE(u32)
FILE_READ_ARRAY_FUNC_CREATE(i64)
FILE_READ_ARRAY_FUNC_CREATE(u64)
FILE_READ_ARRAY_FUNC_CREATE(char)
FILE_READ_ARRAY_FUNC_CREATE(uchar)
FILE_READ_ARRAY_FUNC_CREATE(short)
FILE_READ_ARRAY_FUNC_CREATE(ushort)
FILE_READ_ARRAY_FUNC_CREATE(int)
FILE_READ_ARRAY_FUNC_CREATE(uint)
FILE_READ_ARRAY_FUNC_CREATE(long)
FILE_READ_ARRAY_FUNC_CREATE(ulong)
FILE_READ_ARRAY_FUNC_CREATE(float)
FILE_READ_ARRAY_FUNC_CREATE(double)

FILE_WRITE_FUNC_CREATE(i8)
FILE_WRITE_FUNC_CREATE(u8)
FILE_WRITE_FUNC_CREATE(i16)
FILE_WRITE_FUNC_CREATE(u16)
FILE_WRITE_FUNC_CREATE(i32)
FILE_WRITE_FUNC_CREATE(u32)
FILE_WRITE_FUNC_CREATE(i64)
FILE_WRITE_FUNC_CREATE(u64)
FILE_WRITE_FUNC_CREATE(char)
FILE_WRITE_FUNC_CREATE(uchar)
FILE_WRITE_FUNC_CREATE(short)
FILE_WRITE_FUNC_CREATE(ushort)
FILE_WRITE_FUNC_CREATE(int)
FILE_WRITE_FUNC_CREATE(uint)
FILE_WRITE_FUNC_CREATE(long)
FILE_WRITE_FUNC_CREATE(ulong)
FILE_WRITE_FUNC_CREATE(float)
FILE_WRITE_FUNC_CREATE(double)

FILE_WRITE_ARRAY_FUNC_CREATE(i8)
FILE_WRITE_ARRAY_FUNC_CREATE(u8)
FILE_WRITE_ARRAY_FUNC_CREATE(i16)
FILE_WRITE_ARRAY_FUNC_CREATE(u16)
FILE_WRITE_ARRAY_FUNC_CREATE(i32)
FILE_WRITE_ARRAY_FUNC_CREATE(u32)
FILE_WRITE_ARRAY_FUNC_CREATE(i64)
FILE_WRITE_ARRAY_FUNC_CREATE(u64)
FILE_WRITE_ARRAY_FUNC_CREATE(char)
FILE_WRITE_ARRAY_FUNC_CREATE(uchar)
FILE_WRITE_ARRAY_FUNC_CREATE(short)
FILE_WRITE_ARRAY_FUNC_CREATE(ushort)
FILE_WRITE_ARRAY_FUNC_CREATE(int)
FILE_WRITE_ARRAY_FUNC_CREATE(uint)
FILE_WRITE_ARRAY_FUNC_CREATE(long)
FILE_WRITE_ARRAY_FUNC_CREATE(ulong)
FILE_WRITE_ARRAY_FUNC_CREATE(float)
FILE_WRITE_ARRAY_FUNC_CREATE(double)
