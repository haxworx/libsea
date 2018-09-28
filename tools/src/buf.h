#ifndef __BUF_H__
#define __BUF_H__
#define _POSIX_C_SOURCE 200809L

/**
 * @file
 * @brief Routines for managing binary or string buffer.
 */

/**
 * @brief Buffered Input (data and strings).
 * @defgroup Buffers
 *
 * @{
 *
 * Manipulate buffers.
 *
 */

#include <unistd.h>

typedef struct _buf_t
{
   ssize_t len;
   char   *data;
} buf_t;

/**
 * Return a new buf_t structure.
 *
 * @return A pointer to the newly allocated buf_t structure.
 */
buf_t *
buf_new(void);

/**
 * Increase the capacity of the buffer.
 *
 * @param buf The buffer to increase capacity of.
 * @param len The number of bytes to increase and grow the buffer by.
 */
void
buf_grow(buf_t *buf, ssize_t len);

/**
 * Append data to the buffer.
 *
 * @param buf The buffer to append data to.
 * @param data The data to be appended.
 * @param len The length in bytes of the data.
 */
void
buf_append_data(buf_t *buf, const char *data, ssize_t len);

/**
 * Append a string to the end of data.
 *
 * @param buf The buffer to append the string to.
 * @param string The string to be appended to the buffer.
 */
void
buf_append(buf_t *buf, const char *string);

/**
 * Append a formatted string to the buffer with variable arguments.
 *
 * @param buf The buffer to append the formatted string and arguments to.
 * @param fmt The format of this string.
 * @param ... The list of arguments used by the format specifier.
 */
void
buf_append_printf(buf_t *buf, const char *fmt, ...);

/**
 * Return a valid NULL terminated string of the buffer.
 *
 * @return The buffer's data terminated by a NULL character.
 */
const char *
buf_string_get(buf_t *buf);

/**
 * Trim the length of a buffer to given size.
 *
 * @param buf The buffer to be trimmed.
 * @param start The new size of the buffer.
 */
void
buf_trim(buf_t *buf, ssize_t start);

/**
 * Reset the buffer to it's initial state for new usage.
 *
 * @param buf The buffer to reset.
 */
void
buf_reset(buf_t *buf);

/**
 * Free the contents of the buf_t buffer.
 *
 * @param buf The buffer to free.
 */
void
buf_free(buf_t *buf);

/**
 * @}
 */

#endif
