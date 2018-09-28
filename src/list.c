#include "list.h"
#include <stdlib.h>

list_t *
list_new(void)
{
   return NULL;
}

list_t *
list_add(list_t *list, void *data)
{
   list_t *node = list;

   if (!node)
     {
        list = node = malloc(sizeof(list_t));
        node->data = data;
        node->next = NULL;
        return list;
     }

   while (node->next)
     node = node->next;

   if (node->next == NULL)
     {
        node->next = malloc(sizeof(list_t));
        node = node->next;
        node->data = data;
        node->next = NULL;
     }

   return list;
}

list_t *
list_del(list_t *list, void *data)
{
   list_t *prev, *node;

   prev = NULL;
   node = list;
   while (node)
     {
        if (node->data == data)
          {
             if (prev)
               prev->next = node->next;
             else
               list = node->next;

             free(node->data);
             free(node); node = NULL;
             return list;
          }
        prev = node;
        node = node->next;
     }

   return list;
}

void
list_free(list_t *list)
{
   list_t *next, *node = list;

   while (node)
     {
        next = node->next;
        free(node->data);
        free(node);
        node = next;
     }
}

static void
_list_swap_data(list_t *a, list_t *b)
{
   void *data = a->data;

   a->data = b->data;
   b->data = data;
}

list_t *
list_sort(list_t *list, int (*sort_cmp_fn)(void *, void *))
{
   list_t *node, *last = NULL;
   int swapped;

   do
     {
         swapped = 0;
         node = list;

         while (node->next != last)
           {
              int res = sort_cmp_fn(node->data, node->next->data);
              if (res > 0)
                {
                   _list_swap_data(node->next, node);
                   swapped = 1;
                }
              node = node->next;
           }

        last = node;
     }
   while (swapped);

   return list;
}

void *
list_nth(list_t *list, unsigned int index)
{
   unsigned int i;
   list_t *node = list;

   for (i = 0; node; i++)
     {
        if (i == index)
          {
             return node->data;
          }

        node = node->next;
     }

   return NULL;
}

unsigned int
list_count(list_t *list)
{
   list_t *node;
   unsigned int count = 0;

   for (node = list; node; node = node->next)
     {
        count++;
     }

   return count;
}

void *
list_find(list_t *list, void *data)
{
   list_t *node = list;

   while (node)
     {
        if (node->data == data)
          return node->data;

        node = node->next;
     }

   return NULL;
}

list_t *
list_next(list_t *l)
{
   return l->next;
}

list_t *
list_reverse(list_t *list)
{
   list_t *node = list;
   list_t *prev = NULL;
   list_t *next;

   while (node != NULL)
     {
        next = node->next;
        node->next = prev;
        prev = node;
        node = next;
     }

   return prev;
}

