#ifndef __LIST_H__
#define __LIST_H__

/**
 * @file
 * @brief These routines are for manipulating and using a linked list.
 */

/**
 * @brief Linked list creation and manipulation.
 * @defgroup List
 *
 * @{
 *
 * Creation and manipulation of a linked list.
 */
typedef struct list_t list_t;
struct list_t
{
   void *data;
   list_t *next;
};

/**
 * Initialize a new list.
 *
 * @return A pointer to the newly initialized list.
 */
list_t
*list_new(void);

/**
 * Add an item to the list.
 *
 * @param list The list to add to.
 * @param data The data to be appended to the list.
 *
 * @return A pointer to the list with the appended data.
 */
list_t
*list_add(list_t *list, void *data);

/**
 * Return the next item after list_t pointer.
 *
 * @param l The node within a list.
 *
 * @return The next item within the list after the node.
 */
list_t
*list_next(list_t *l);

/**
 * Sort a list.
 *
 * @param list The list to sort.
 * @param sort_cmp_fn The function pointer used to compare and sort the list.
 *
 * @return The sorted list.
 */
list_t *
list_sort(list_t *list, int (*sort_cmp_fn)(void *, void *));

/**
 * Reverse a list.
 *
 * @param list The list to be reversed in order.
 *
 * @return The newly reversed list.
 */
list_t *
list_reverse(list_t *list);

/**
 * Find an item within the list matching data.
 *
 * @param list The list to search within.
 * @param data The data to search for.
 *
 * @return A pointer to the data within the list or NULL if not found.
 */
void
*list_find(list_t *list, void *data);

/**
 * Delete an entry in the list.
 *
 * @param list The list to remove a member from.
 * @param data Item to be removed if found within the list.
 *
 * @return A pointer to the list with removed entry.
 */
list_t
*list_del(list_t *list, void *data);

/**
 * Return the data at nth position within the list if found.
 *
 * @param list The list to be indexed.
 * @param index The index within the list to be returned.
 *
 * @return The data found at the nth location within the list.
 */
void
*list_nth(list_t *list, unsigned int index);

/**
 * A count of how many entries are within the list.
 *
 * @param list The list to count members of.
 *
 * @return The number of items within the given list.
 */
unsigned int
list_count(list_t *list);

/**
 * Free a list and all its members.
 *
 * @param list The list to free.
 */
void
list_free(list_t *list);

#if defined(LIST_FOREACH)
# undef LIST_FOREACH
#endif

#define LIST_FOREACH(_list, _l, _data) \
        if (_list) \
          for (_l = _list; _l && (_data = _l->data); _l = _l->next)

/**
 * @}
 */

#endif
