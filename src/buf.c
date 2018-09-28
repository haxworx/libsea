#include "buf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

buf_t *
buf_new(void)
{
   buf_t *buf = calloc(1, sizeof(buf_t));

   return buf;
}

void
buf_grow(buf_t *buf, ssize_t len)
{
   void *tmp = realloc(buf->data, buf->len + len + 1);
   buf->data = tmp;
}

void
buf_append_data(buf_t *buf, const char *data, ssize_t len)
{
   buf_grow(buf, len);
   memcpy(buf->data + buf->len, data, len);
   buf->len += len;
}

void
buf_append(buf_t *buf, const char *string)
{
   ssize_t len;

   len = strlen(string);
   buf_append_data(buf, string, len);
}

void
buf_append_printf(buf_t *buf, const char *fmt, ...)
{
   char tmp_buf[4096];
   va_list ap, ap2;
   int len;

   va_start(ap, fmt);
   va_copy(ap2, ap);

   len = vsnprintf(tmp_buf, sizeof(tmp_buf), fmt, ap);
   if (len < (int)sizeof(tmp_buf))
     {
        buf_append(buf, tmp_buf);
     }
   else
     {
        buf_grow(buf, len);
        vsnprintf(buf->data + buf->len, len + 1, fmt, ap2);
        buf->len += len;
     }

   va_end(ap2);
   va_end(ap);
}

const char *
buf_string_get(buf_t *buf)
{
   if (!buf->len)
     {
        buf->data = strdup("");
        buf->len = 0;
        return buf->data;
     }

   buf->data[buf->len] = '\0';

   return buf->data;
}

void
buf_trim(buf_t *buf, ssize_t start)
{
   if (buf->len < start) return;

   buf->len = start;
   buf->data[start] = '\0';
}

void
buf_reset(buf_t *buf)
{
   free(buf->data);
   buf->data = NULL;
   buf->len = 0;
}

void
buf_free(buf_t *buf)
{
   free(buf->data);
   free(buf);
}

