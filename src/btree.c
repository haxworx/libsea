#include "btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

tree_t *
tree_new(void)
{
   return NULL;
}

static tree_t *
_leaf_new(size_t value, void *data)
{
   tree_t *leaf = malloc(sizeof(tree_t));
   leaf->value = value;
   leaf->data = data;
   leaf->left = leaf->right = NULL;

   return leaf;
}

tree_t *
tree_add(tree_t *leaf, size_t value, void *data)
{
   if (!leaf)
     {
        leaf = _leaf_new(value, data);
        return leaf;
     }

   if (leaf->value < value)
     {
        leaf->left = tree_add(leaf->left, value, data);
     }
   else if (leaf->value > value)
     {
        leaf->right = tree_add(leaf->right, value, data);
     }

   return leaf;
}

void *
tree_find(tree_t *leaf, size_t value)
{
   if (!leaf) return NULL;

   if (leaf->value < value)
     return tree_find(leaf->left, value);
   else if (leaf->value > value)
     return tree_find(leaf->right, value);

   return leaf->data;
}

void
tree_free(tree_t *leaf)
{
   if (!leaf) return;

   if (leaf->left)
     tree_free(leaf->left);
   if (leaf->right)
     tree_free(leaf->right);

   free(leaf->data);
   free(leaf);
}

