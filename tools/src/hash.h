#ifndef __HASH_H__
#define __HASH_H__

#include <stdbool.h>

/**
 * @file
 * @brief These routines are for using a hash table.
 */
#define TABLE_SIZE 33311

/**
 * @brief Hash table manipulation and creation.
 * @defgroup Hash
 *
 * @{
 *
 * Creation and manipulation of a simple hash table.
 *
 */
struct hash_t
{
   char   *key;
   void   *data;
   struct hash_t *bucket_next;
};

typedef struct hash_t * hash_t;

/**
 * Create a new hash table.
 *
 * @return A pointer to the newly created hash table.
 */
hash_t *
hash_new(void);

/**
 * Add data to a hash table.
 *
 * @param hashtable The hash table to add to.
 * @param key The key used to identify the entry in the hash table.
 * @param data The data to add to the hash table.
 */
void
hash_add(hash_t *hashtable, const char *key, void *data);

/**
 * Delete an item within a hash table identified by key.
 *
 * @param hashtable The hash table to remove an item from.
 * @param key The key to identify the data to be removed within the hash table.
 */
void
hash_del(hash_t *hashtable, const char *key);

/**
 * Find an item within a hash table identified by its key.
 *
 * @param hashtable The hash table to search within.
 * @param key The key to identify the item within the hash table.
 *
 * @return A pointer to the item if found or NULL if not.
 */
void *
hash_find(hash_t *hashtable, const char *key);

/**
 * Free the whole hash table structure and its members.
 *
 * @param hashtable The hash table to free.
 */
void
hash_free(hash_t *hashtable);

/**
 * Dump information about all members within a hash table.
 *
 * @param hashtable The hash table to query.
 */
void
hash_dump(hash_t *hashtable);

/*
 * Return whether a hash table is empty or not.
 *
 * @param hashtable The hash table to query.
 *
 * @return True or False.
 */
bool
hash_is_empty(hash_t *hashtable);

char **
hash_keys_get(hash_t *hashtable);

void
hash_keys_free(char **keys);

/**
 * @}
 */
#endif
