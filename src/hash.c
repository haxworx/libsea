#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

static uint32_t
hashish(const char *s)
{
   const char *p = s;
   uint32_t res = 0;

   while (*p)
     {
        res *= (uint32_t)*p + 101;
        p++;
     }

   return res % TABLE_SIZE;
}

struct hash_t **
hash_new(void)
{
   int i;
   struct hash_t **hashtable = malloc(TABLE_SIZE * sizeof(struct hash_t *));

   for (i = 0; i < TABLE_SIZE; i++)
     {
        hashtable[i] = NULL;
     }

   return hashtable;
}

void
hash_free(struct hash_t **hashtable)
{
   for (int i = 0; i < TABLE_SIZE; i++)
     {
        struct hash_t *cursor = hashtable[i];
        while (cursor)
          {
             free(cursor->key);
             if (cursor->data)
               free(cursor->data);
             struct hash_t *next = cursor->bucket_next;
             free(cursor);
             cursor = next;
          }
     }
   free(hashtable);
}

void
hash_add(struct hash_t **hashtable, const char *key, void *data)
{
   uint32_t index = hashish(key);

   struct hash_t *cursor = hashtable[index];

   if (!cursor)
     {
        hashtable[index] = cursor = calloc(1, sizeof(struct hash_t));
        cursor->key = strdup(key);
        cursor->data = data;
        cursor->bucket_next = NULL;
        return;
     }

   while (cursor->bucket_next)
     cursor = cursor->bucket_next;

   if (cursor->bucket_next == NULL)
     {
        cursor->bucket_next = calloc(1, sizeof(struct hash_t));
        cursor = cursor->bucket_next;
        cursor->key = strdup(key);
        cursor->data = data;
        cursor->bucket_next = NULL;
     }
}

void
hash_del(struct hash_t **hashtable, const char *key)
{
   uint32_t idx = hashish(key);
   struct hash_t *tmp = hashtable[idx];
   if (!tmp)
     return;

   struct hash_t *last = NULL;

   while (tmp)
     {
        if (tmp->key && !strcmp(tmp->key, key))
          {
             if (tmp->data)
               free(tmp->data);
             if (last)
               last->bucket_next = tmp->bucket_next;
             else
               hashtable[idx] = tmp->bucket_next;

             if (tmp->key)
               free(tmp->key);

             free(tmp);

             return;
          }

        last = tmp;
        tmp = tmp->bucket_next;
     }
}

void *
hash_find(struct hash_t **hashtable, const char *key)
{
   uint32_t idx;
   struct hash_t *tmp;

   if (!key) return NULL;

   idx  = hashish(key);
   tmp = hashtable[idx];

   if (!tmp)
     {
        return NULL;
     }

   while (tmp)
     {
        if (tmp->key && !strcmp(tmp->key, key))
          {
             return tmp->data;
          }

        tmp = tmp->bucket_next;
     }

   return NULL;
}

void
hash_dump(struct hash_t **hashtable)
{
   for (int i = 0; i < TABLE_SIZE; i++)
     {
        struct hash_t *cursor = hashtable[i];
        while (cursor)
          {
             if (cursor->data)
               {
                  printf("key -> %s data -> %p\n",
                         cursor->key, cursor->data);
               }
             cursor = cursor->bucket_next;
          }
     }
}

bool
hash_is_empty(struct hash_t **hashtable)
{
   for (int i = 0; i < TABLE_SIZE; i++)
     {
        struct hash_t *cursor = hashtable[i];
        if (cursor)
          return false;
     }

   return true;
}

char **
hash_keys_get(hash_t *hashtable)
{
   char **keys = malloc(sizeof(char *));
   int idx = 0, count = 1;

   for (int i = 0; i < TABLE_SIZE; i++)
     {
        struct hash_t *cursor = hashtable[i];
        while (cursor)
          {
             keys[idx++] = strdup(cursor->key);
             count++;
             void *tmp = realloc(keys, count * sizeof(char *));
             keys = tmp;
             cursor = cursor->bucket_next;
          }
     }

   keys[idx] = NULL;

   return keys;
}

void
hash_keys_free(char **keys)
{
   int i;

   for (i = 0; keys[i] != NULL; i++)
     {
        free(keys[i]);
     }
   free(keys[i]);
   free(keys);
}

