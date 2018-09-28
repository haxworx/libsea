#ifndef __FILE_H__
#define __FILE_H__

/**
 * @file
 * @brief These routines are used for file interaction.
 */

#include "list.h"
#include "buf.h"
#include <unistd.h>
#include <stdbool.h>

/**
 * @brief Files and Directories.
 * @defgroup File
 *
 * @{
 *
 * Manipulation of files and directories.
 *
 */

typedef struct stat_t
{
   char *filename;
   int mode;
   size_t size;
   size_t inode;
   size_t mtime;
   size_t ctime;
} stat_t;

/**
 * Check if file exists.
 *
 * @param path The path to test for existence.
 *
 * @return true (exists) or false.
 */
bool
file_exists(const char *path);

/**
 * Check whether path is a directory.
 *
 * @param path The path to test whether it is a directory.
 *
 * @return Return true when path is a directory, otherwise return false.
 */
bool
file_is_directory(const char *path);

/**
 * Check whether directory is empty.
 *
 * @param path The path of the directory to check.
 *
 * @return Return true if directory is empty, otherwise return false;
 */
bool
file_directory_is_empty(const char *path);

/**
 * Return the file size of file at path.
 *
 * @param path The path of the file to get the size of.
 *
 * @return The file size in bytes or -1 if an error occurred.
 */
ssize_t
file_size_get(const char *path);

/**
 * Return the base filename from the path.
 *
 * @param path The path to obtain the basename from.
 *
 * @return The basename of the path or NULL on error.
 */
char *
file_filename_get(const char *path);

/**
 * Read file contents into a buf_t.
 *
 * @param path The file to read the contents of.
 *
 * @return A buf_t structure containing the file contents.
 */
buf_t *
file_contents_get(const char *path);

/**
 * Read file statistic of given file and store in stat_t data structure.
 *
 * @param path The file to obtain the statistics of.
 *
 *
 * @return A newly allocated stat_t pointer with file statistics.
 */
stat_t *
file_stat(const char *path);

/**
 * Read all files in a directory and return a list of their names.
 *
 * @param directory The directory to read the file list from.
 *
 * @return A list with the files and directories found.
 */
list_t *
file_ls(const char *directory);

/**
 * Read all files in a directory and return a list of stat_t * entries.
 *
 * @param directory
 *
 * @return A list of all the files found in stat_t structure.
 */
list_t *
file_stat_ls(const char *directory);

/**
 * Free all memory used in a file_stat_ls() produced list.
 *
 * @param files The list returned from file_stat_ls.
 *
 */
void
file_stat_ls_free(list_t *files);

/**
 * Create directory at given location.
 *
 * @param path The location to create a directory.
 *
 * @return If successful return true.
 */
bool
file_mkdir(const char *path);

/**
 * Remove file at given location.
 *
 * @param path The file or directory to be removed.
 *
 * @return Return true on success, otherwise return false.
 */
bool
file_remove(const char *path);

/**
 * Copy file contents from one location to another.
 *
 * @param src The source path to be copied.
 * @param dest The destination of the file copied.
 *
 * @return Return true on success, otherwise return false.
 */
bool
file_copy(const char *src, const char *dest);

/**
 * Move a file from one location to another.
 *
 * @param src The original location of the file.
 * @param dest The destination of the file to be moved.
 *
 * @return Return true on success, otherwise return false.
 */
bool
file_move(const char *src, const char *dest);

/*
 * Recursively remove all files in path.
 *
 * @param path The directory to recursively remove all files and directories from.
 */
int
file_remove_all(const char *path);

typedef int (file_path_walk_cb)(const char *path, stat_t *st, void *data);

/**
 * Recursively walk a directory.
 *
 * @param directory The root path to recursively walk.
 * @param path_walk_cb The callback to be triggered on directory listing.
 * @param data User data to pass to the callback.
 */
void
file_path_walk(const char *directory, file_path_walk_cb path_walk_cb, void *data);

/**
 * Append file to path and return full new path name.
 *
 * @param path The path to append file to.
 * @param file The file to be appended.
 *
 * @return A newly allocated string containing the new appended path.
 */
char *
file_path_append(const char *path, const char *file);

/**
 * Escape given path for safe usage with shell.
 *
 * @param path The path to escape.
 *
 * @return A newly allocated and fully escaped string.
 */
char *
file_path_escape(const char *path);

/**
 * Obtain SHA256 checksum of file at given path.
 *
 * @param path The file to get the checksum of.
 *
 * @return A newly allocated string containing the hash.
 */
char *
file_sha256sum(const char *path);


/**
 * Obtain SHA512 checksum of file at given path.
 *
 * @param path The file to get the checksum of.
 *
 * @return A freshly allocated string containing the hash.
 */
char *
file_sha512sum(const char *path);

/**
 * @}
 */

#endif
