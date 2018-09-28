#ifndef __BTREE_H__
#define __BTREE_H__

/**
 * @file
 * @brief These routines are for using a simple binary tree.
 */

#include <unistd.h>

/**
 * @brief Binary tree implementation.
 * @defgroup Tree
 *
 * @{
 *
 * Manipulation of simple binary tree.
 *
 */

typedef struct tree_t tree_t;
struct tree_t
{
   tree_t *left;
   tree_t *right;
   size_t value;
   void *data;
};

/**
 * Create a new tree.
 *
 * @return A pointer to a newly allocated binary tree.
 */
tree_t *
tree_new(void);

/**
 * Add an item to the tree.
 *
 * @param node A pointer to the tree.
 * @param value The value used to index the data.
 * @param data The data to be stored within the tree.
 *
 * @return A pointer to the tree with the newly inserted item.
 */
tree_t *
tree_add(tree_t *node, size_t value, void *data);

/**
 * Find item within a tree by its value.
 *
 * @param node The tree to search within.
 * @param value The index of the data witin the tree.
 *
 * @return A pointer to the data stored witin the tree.
 */
void *
tree_find(tree_t *node, size_t value);

/**
 * Free the whole tree including all data.
 *
 * @param node The tree to free.
 *
 */
void
tree_free(tree_t *node);

/**
 * @}
 */

#endif
