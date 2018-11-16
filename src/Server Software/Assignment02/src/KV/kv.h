/* Header file for kv store. 
 * You may use all the methods in this file.
 * Note: the implementation is not thread-safe.
 */

#ifndef _kv_h_
#define _kv_h_

/*
 * Search for the value stored under key.
 * PRE: key is not null.
 * RETURNS: If the key exists, a pointer to the value stored under this key.
 * If the key does not exist, it returns NULL.
 */
char* findValue(const char* key);

/*
 * Test if a key exists.
 * PRE: key is not null (if it is, this function returns 0).
 * RETURNS: If the key exists, the function returns 1, else 0.
 */
int itemExists(const char* key);

/*
 * Create a new item under the given key.
 * The store makes a copy of the key, so it is fine to pass a pointer to a key
 * which lives on the stack. The value however is not copied - it must be
 * allocated on the heap.
 * PRE: Neither key nor value may be NULL and
 * an item with the given key must not exist yet.
 * POST: if successful, the pair (key, value) is added to the store.
 * RETURNS: 0 for success and (-1) for error.
 * ERRORS: - The key already exists.
 *         - Number of slots exceeded.
 *         - Key or value is NULL.
 *         - Out of memory.
 */
int createItem(const char* key, char* value);

/*
 * Update a new item under the given key.
 * The old value is not freed - if you want to do this,
 * use delete followed by create.
 * PRE: Neither key nor value may be NULL. Key must exist in the store.
 * POST: On success, the pair (key, value) is stored.
 * RETURNS: 0 for success, (-1) on error.
 * ERRORS: - Key or value is NULL.
 *         - Key does not exist.
 */
int updateItem(const char* key, char* value);

/*
 * Delete an item, optionally freeing the value.
 * PRE: key is not NULL and exists in the store.
 * POST: On success, the key is deleted; if free_it was nonzero then
 * the value under this key is freed, for free_it == 0 the value is left alone.
 * RETURNS: 0 on success, (-1) on error.
 * ERRORS: - key is null.
           - key does not exist.
 */
int deleteItem(const char* key, int free_it);

/* 
 * Count the number of items stored.
 * RETURNS: the number of items stored. Cannot fail.
 */
int countItems();

#endif
