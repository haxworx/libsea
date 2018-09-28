#include "file.h"
#include "buf.h"
#include <sys/stat.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <openssl/sha.h>

bool
file_is_directory(const char *path)
{
   struct stat st;

   if (stat(path, &st) < 0) return false;

   if (S_ISDIR(st.st_mode))
     return true;

   return false;
}

bool
file_exists(const char *path)
{
   struct stat st;

   if (stat(path, &st) < 0)
     {
        if (errno == ENOENT || errno == EACCES ||
            errno == ELOOP || errno == ENOTDIR)
          return false;
     }

   return true;
}

ssize_t
file_size_get(const char *path)
{
   struct stat st;

   if (stat(path, &st) < 0) return -1;

   return st.st_size;
}

char *
file_filename_get(const char *path)
{
   char *filename;
   char *end = strrchr(path, '/');
   if (!end) return strdup(path);

   filename = end + 1;
   if (!filename) return strdup(path);

   return strdup(filename);
}

buf_t *
file_contents_get(const char *path)
{
   FILE *f;
   buf_t *buf;
   ssize_t size, block_size = 1024;

   f = fopen(path, "r");
   if (!f)
     return NULL;

   buf = buf_new();

   size = file_size_get(path);
   if (size > 0)
     {
        buf_grow(buf, block_size);
        buf->len += fread(buf->data, 1, block_size, f);
     }

   while (!feof(f) && !ferror(f))
     {
        buf_grow(buf, block_size);
        buf->len += fread(buf->data + buf->len, 1, block_size, f);
     }

   buf_grow(buf, 0);

   fclose(f);

   return buf;
}

stat_t *
file_stat(const char *path)
{
   stat_t *s;
   struct stat st;

   if (stat(path, &st) < 0) return NULL;

   s = calloc(1, sizeof(stat_t));
   // No filename so can just free pointer.
   s->size = st.st_size;
   s->mode = st.st_mode;
   s->inode = st.st_ino;
   s->ctime = st.st_ctime;
   s->mtime = st.st_mtime;

   return s;
}

list_t *
file_stat_ls(const char *directory)
{
   DIR *dir;
   struct dirent *ent;
   buf_t *path;
   list_t *files;

   dir = opendir(directory);
   if (!dir) return NULL;

   files = list_new();

   path = buf_new();

   while ((ent = readdir(dir)) != NULL)
     {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
          continue;

        buf_reset(path);
        buf_append_printf(path, "%s/%s", directory, ent->d_name);
        struct stat st;
        if (stat(buf_string_get(path), &st) < 0)
          {
             continue;
          }

        stat_t *s = malloc(sizeof(stat_t));
        s->filename = strdup(ent->d_name);
        s->size = st.st_size;
        s->mode = st.st_mode;
        s->inode = st.st_ino;
        s->ctime = st.st_ctime;
        s->mtime = st.st_mtime;

        files = list_add(files, s);
     }

   buf_free(path);
   closedir(dir);

   return files;
}

void
file_stat_ls_free(list_t *files)
{
   list_t *next, *l = files;
   while (l)
     {
        next = l->next;
        stat_t *st = l->data;
        free(st->filename);
        st->filename = NULL;
        free(st);
        st = NULL;
        free(l);
        l = NULL;
        l = next;
     }
}

list_t *
file_ls(const char *directory)
{
   DIR *dir;
   struct dirent *ent;
   list_t *files;

   dir = opendir(directory);
   if (!dir) return NULL;

   files = list_new();

   while ((ent = readdir(dir)) != NULL)
     {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
          continue;
        files = list_add(files, strdup(ent->d_name));
     }

   closedir(dir);

   return files;
}

bool
file_mkdir(const char *path)
{
   if (file_exists(path))
     return false;

   if (mkdir(path, 0755) == 0)
     return true;

   return false;
}


bool
file_directory_is_empty(const char *path)
{
   list_t *files;
   struct stat st;

   if (stat(path, &st) < 0)
     return false;

   if (!S_ISDIR(st.st_mode))
     return false;

   files = file_ls(path);
   if (files)
     {
        list_free(files);
        return false;
     }

   return true;
}

bool
file_remove(const char *path)
{
   if (!file_exists(path))
     return false;

   if (!file_is_directory(path))
     {
        unlink(path);
        return true;
     }
   else
     {
        if (file_directory_is_empty(path))
          {
             if (rmdir(path) == 0)
               return true;
          }
     }

   return false;
}

#if defined(__I_AM_INSANE__)

static void
_file_remove(const char *directory)
{
  buf_t *path;
  list_t *l, *files;

  path = buf_new();

  files = file_stat_ls(directory);
  if (!files) return;

   for (l = files; l; l = l->next)
     {
       stat_t *st = l->data;
       if (strcmp(st->filename, ".") && strcmp(st->filename, ".."))
         {
            buf_reset(path);
            buf_append_printf(path, "%s/%s", directory, st->filename);

            if (file_is_directory(buf_string_get(path)))
              {
                 _file_remove(buf_string_get(path));
              }
            else
              {
                 unlink(buf_string_get(path));
              }
         }
    }

   rmdir(directory);
   buf_free(path);

   file_stat_ls_free(files);
}

#endif

int
file_remove_all(const char *path)
{
   if (file_remove(path))
     return 1;
#if defined(__I_AM_INSANE__)
   _file_remove(path);
#else
    fprintf(stderr, "ERR: quietely refusing to recursively remove!\n");
#endif

   return 1;
}

void
file_path_walk(const char *directory, file_path_walk_cb path_walk_cb, void *data)
{
   buf_t *path;
   stat_t *st;

   path = buf_new();

   list_t *l, *files = file_stat_ls(directory);
   l = files;
   while (l)
     {
        st = l->data;
        if (strcmp(st->filename, ".") && strcmp(st->filename, ".."))
          {
             buf_reset(path);
             buf_append_printf(path, "%s/%s", directory, st->filename);

             if (file_is_directory(buf_string_get(path)))
               {
                  file_path_walk(buf_string_get(path), path_walk_cb, data);
               }

             path_walk_cb(buf_string_get(path), st, data);
          }
        l = l->next;
     }

   buf_free(path);

   file_stat_ls_free(files);
}

bool
file_copy(const char *src, const char *dest)
{
   FILE *in, *out;
   size_t length, bytes, total;
   char buf[4096];

   stat_t *st = file_stat(src);
   if (!st) return false;

   length = st->size;

   free(st);

   in = fopen(src, "rb");
   if (!in) return false;

   out = fopen(dest, "wb");
   if (!out) return false;

   total = 0;
   do
     {
         bytes = fread(buf, 1, sizeof(buf), in);
         if (bytes == 0)
           break;

         fwrite(buf, 1, bytes, out);

         total += bytes;
     }
   while (total < length);

   fclose(in);
   fclose(out);

   return true;
}

bool
file_move(const char *from, const char *to)
{
   return ! rename(from, to);
}

char *
file_path_append(const char *path, const char *file)
{
   char *concat;
   int len;
   char separator = '/';

   len = strlen(path) + strlen(file) + 2;
   concat = malloc(sizeof(char) * len);
   snprintf(concat, len, "%s%c%s", path, separator, file);

   return concat;
}

char *
file_sha256sum(const char *path)
{
   FILE *f;
   stat_t *st;
   ssize_t bytes;
   char buf[4096];
   size_t length, total;
   SHA256_CTX ctx;
   unsigned char result[SHA256_DIGEST_LENGTH] = { 0 };
   char sha256[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };
   int i, j;

   st = file_stat(path);
   if (!st) return NULL;

   length = st->size;

   free(st);

   f = fopen(path, "rb");
   if (!f)
     return NULL;

   SHA256_Init(&ctx);

   total = 0;

   do
     {
        bytes = fread(buf, 1, sizeof(buf), f);
        if (bytes == 0) break;
        SHA256_Update(&ctx, buf, bytes);
        total += bytes;
     }
   while (total < length);

   fclose(f);

   SHA256_Final(result, &ctx);

   for (i = 0, j = 0; i < SHA256_DIGEST_LENGTH; i++)
     {
        snprintf(&sha256[j], sizeof(sha256), "%02x", (unsigned int) result[i]);
        j += 2;
     }

   return strdup(sha256);
}

char *
file_sha512sum(const char *path)
{
   FILE *f;
   stat_t *st;
   ssize_t bytes;
   char buf[4096];
   size_t length, total;
   SHA512_CTX ctx;
   unsigned char result[SHA512_DIGEST_LENGTH] = { 0 };
   char sha512[2 * SHA512_DIGEST_LENGTH + 1] = { 0 };
   int i, j;

   st = file_stat(path);
   if (!st) return NULL;

   length = st->size;

   free(st);

   f = fopen(path, "rb");
   if (!f)
     return NULL;

   SHA512_Init(&ctx);

   total = 0;

   do
     {
        bytes = fread(buf, 1, sizeof(buf), f);
        if (bytes == 0) break;
        SHA512_Update(&ctx, buf, bytes);
        total += bytes;
     }
   while (total < length);

   fclose(f);

   SHA512_Final(result, &ctx);

   for (i = 0, j = 0; i < SHA512_DIGEST_LENGTH; i++)
     {
        snprintf(&sha512[j], sizeof(sha512), "%02x", (unsigned int) result[i]);
        j += 2;
     }

   return strdup(sha512);
}

char *
file_path_escape(const char *path)
{
   char *to, *output;
   char ch;
   int i, len;

   if (!path)
     return NULL;

   len = strlen(path);
   if (!len)
     return NULL;

   output = to = malloc((len * 2) + 1);

   for (i = 0; i < len; i++)
     {
        ch = path[i];
        if ((ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9'))
          {
             *to = ch;
             to++;
          }
        else
          {
             to[0] = '\\';
             to[1] = ch;
             to += 2;
          }
     }

   *to = 0x00;

   return output;
}

