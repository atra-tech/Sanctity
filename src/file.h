/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *  
 *  file.h - Set of tools for facilitating reading and writing files.
 *           Also provides some parsing tools useful to custom file formats.
 * 
 */

#ifndef FILE_H
#define FILE_H

#include "atra.h"

#define FILE_DEFAULT_CAP    256
#define FILE_MAX_DIGITS     64

#define FILE_READ_FUNC_DECLARE(type) \
    type file_read_##type(FileData *file);
#define FILE_READ_ARRAY_FUNC_DECLARE(type) \
    void file_read_##type##_array(FileData *file, size_t n, type *data);
#define FILE_WRITE_FUNC_DECLARE(type) \
    void file_write_##type(FileData *file, type data);
#define FILE_WRITE_ARRAY_FUNC_DECLARE(type) \
    void file_write_##type##_array(FileData *file, size_t n, const type *data);

typedef struct FileData {
    size_t cap;     /* Current maximum size */
    size_t size;    /* Current number of bytes in file */
    size_t head;    /* Index of read/write head */
    u8 *data;       /* Pointer to start of file data */
} FileData;

/*
 *  File management functions
 */

/* Creates a new file data structure with given size.
 * Size of 0 falls back to defaults.
 * It is preferable to use sizes of 2^n, but not essential.
 */
FileData file_create(size_t size);

/* Deletes an initialised file data structure */
void file_delete(FileData file);

/* Reads a file into a file data structure */
FileData file_load(const char *path);

/* Saves a file data structure into a file.  Returns 1 on success. */
void file_save(FileData *file, const char *path);

/* Increases the size cap of a file data structure */
void file_grow(FileData *file);

/* Sets the read/write head back to the start of the file */
void file_rewind(FileData *file);


/*
 *  Parsing functions
 */

/* NOTE: All functions advance read/write head */

/* Returns non-zero if the given byte is at the head */
int file_expect_byte(FileData *file, u8 b);

/* Returns the length of a contiguous string of bytes matching the given byte */
size_t file_expect_byte_many(FileData *file, u8 b);

/* Returns non-zero if the byte at the read/write head is an alphabetic char */
int file_expect_alpha(FileData *file);

/* Returns the length of an expected alphabetic string at the head */
size_t file_expect_alpha_many(FileData *file);

/* Returns 1 if the byte at the read/write head is an alpha-numeric character */
int file_expect_alphanum(FileData *file);

/* Returns the length of an expected alpha-numeric string at the head */
size_t file_expect_alphanum_many(FileData *file);

/* Returns the value of an expected integer */
int file_expect_int(FileData *file);

/* Returns the value of an expected floating-point number */
double file_expect_float(FileData *file);

/* Skips over any characters in the given delimiter string until no match is found.
 * Returns the number of characters skipped. */
size_t file_expect_delim(FileData *file, const char *delims);


FILE_READ_FUNC_DECLARE(i8)
FILE_READ_FUNC_DECLARE(u8)
FILE_READ_FUNC_DECLARE(i16)
FILE_READ_FUNC_DECLARE(u16)
FILE_READ_FUNC_DECLARE(i32)
FILE_READ_FUNC_DECLARE(u32)
FILE_READ_FUNC_DECLARE(i64)
FILE_READ_FUNC_DECLARE(u64)
FILE_READ_FUNC_DECLARE(char)
FILE_READ_FUNC_DECLARE(uchar)
FILE_READ_FUNC_DECLARE(short)
FILE_READ_FUNC_DECLARE(ushort)
FILE_READ_FUNC_DECLARE(int)
FILE_READ_FUNC_DECLARE(uint)
FILE_READ_FUNC_DECLARE(long)
FILE_READ_FUNC_DECLARE(ulong)
FILE_READ_FUNC_DECLARE(float)
FILE_READ_FUNC_DECLARE(double)

FILE_READ_ARRAY_FUNC_DECLARE(i8)
FILE_READ_ARRAY_FUNC_DECLARE(u8)
FILE_READ_ARRAY_FUNC_DECLARE(i16)
FILE_READ_ARRAY_FUNC_DECLARE(u16)
FILE_READ_ARRAY_FUNC_DECLARE(i32)
FILE_READ_ARRAY_FUNC_DECLARE(u32)
FILE_READ_ARRAY_FUNC_DECLARE(i64)
FILE_READ_ARRAY_FUNC_DECLARE(u64)
FILE_READ_ARRAY_FUNC_DECLARE(char)
FILE_READ_ARRAY_FUNC_DECLARE(uchar)
FILE_READ_ARRAY_FUNC_DECLARE(short)
FILE_READ_ARRAY_FUNC_DECLARE(ushort)
FILE_READ_ARRAY_FUNC_DECLARE(int)
FILE_READ_ARRAY_FUNC_DECLARE(uint)
FILE_READ_ARRAY_FUNC_DECLARE(long)
FILE_READ_ARRAY_FUNC_DECLARE(ulong)
FILE_READ_ARRAY_FUNC_DECLARE(float)
FILE_READ_ARRAY_FUNC_DECLARE(double)

FILE_WRITE_FUNC_DECLARE(i8)
FILE_WRITE_FUNC_DECLARE(u8)
FILE_WRITE_FUNC_DECLARE(i16)
FILE_WRITE_FUNC_DECLARE(u16)
FILE_WRITE_FUNC_DECLARE(i32)
FILE_WRITE_FUNC_DECLARE(u32)
FILE_WRITE_FUNC_DECLARE(i64)
FILE_WRITE_FUNC_DECLARE(u64)
FILE_WRITE_FUNC_DECLARE(char)
FILE_WRITE_FUNC_DECLARE(uchar)
FILE_WRITE_FUNC_DECLARE(short)
FILE_WRITE_FUNC_DECLARE(ushort)
FILE_WRITE_FUNC_DECLARE(int)
FILE_WRITE_FUNC_DECLARE(uint)
FILE_WRITE_FUNC_DECLARE(long)
FILE_WRITE_FUNC_DECLARE(ulong)
FILE_WRITE_FUNC_DECLARE(float)
FILE_WRITE_FUNC_DECLARE(double)

FILE_WRITE_ARRAY_FUNC_DECLARE(i8)
FILE_WRITE_ARRAY_FUNC_DECLARE(u8)
FILE_WRITE_ARRAY_FUNC_DECLARE(i16)
FILE_WRITE_ARRAY_FUNC_DECLARE(u16)
FILE_WRITE_ARRAY_FUNC_DECLARE(i32)
FILE_WRITE_ARRAY_FUNC_DECLARE(u32)
FILE_WRITE_ARRAY_FUNC_DECLARE(i64)
FILE_WRITE_ARRAY_FUNC_DECLARE(u64)
FILE_WRITE_ARRAY_FUNC_DECLARE(char)
FILE_WRITE_ARRAY_FUNC_DECLARE(uchar)
FILE_WRITE_ARRAY_FUNC_DECLARE(short)
FILE_WRITE_ARRAY_FUNC_DECLARE(ushort)
FILE_WRITE_ARRAY_FUNC_DECLARE(int)
FILE_WRITE_ARRAY_FUNC_DECLARE(uint)
FILE_WRITE_ARRAY_FUNC_DECLARE(long)
FILE_WRITE_ARRAY_FUNC_DECLARE(ulong)
FILE_WRITE_ARRAY_FUNC_DECLARE(float)
FILE_WRITE_ARRAY_FUNC_DECLARE(double)

#endif /* FILE_H */
