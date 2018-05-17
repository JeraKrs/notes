/* 
 * Header file for skiplist store.
 */

#ifndef _skiplist_h_
#define _skiplist_h_

#define SKL_MAXKEY 255
#define SKL_MAXLEVEL 10
#define SKL_MAXITEMS 100
#define SKL_PROB 0.25

struct node {
    char key[SKL_MAXKEY];
    char val[SKL_MAXKEY];
    struct node* forwards[SKL_MAXLEVEL];
};
typedef struct node Node;

struct skiplist {
    int nItems;
    int level;
    Node* header;
};
typedef struct skiplist Skiplist;

/* 
 * Apply a new node for skiplist
 * RETURNS: a pointer of node which allocates on the heap
 */
Node* new_node();

/*
 * Choosing a random level
 * RETURNS: a random number between 1 to SKL_MAXLEVEL
 */
int random_level();

/*
 * Initialise the skip list.
 * RETURNS: a pointer of skiplist or NULL
 */
Skiplist* init_skiplist();

/*
 * Search for the value stored under key. 
 * PARAS: - items: the pointer of skip list
 *		  - key: the key for search
 *		  - ret: the pointer for storing return value
 *				it needs memories, not only a pointer
 * RETURNS: if the key exists, it copies the value in ret and returns 0
 * if the key does not exist, it returns -1
 */
int search_skiplist(Skiplist* items, const char* key, char* ret);

/*
 * Check the value is exist or not
 * PARAS: - items: the pointer of skip list
 *		  - key: the key for search
 * RETURNS: if the key exists return 1
 * if the key does not exist, it returns 0 and -1 for error
 */
int exists_skiplist(Skiplist* items, const char* key);

/*
 * Insert a new item under the given key.
 * The store makes a copy of the key and value
 * PARAS: - items: the pointer of skip list
 *		  - key: the key for insertion
 *		  - val: the value for insertion
 * RETURNS: 0 for success and (-1) for error
 * ERRORS: - The items is NULL
 *		   - The key already exists
 *         - Number of slots exceeded
 *         - The key or value is NULL
 *         - Out of memory
 */
int insert_skiplist(Skiplist* items, const char* key, const char* val);

/*
 * Update a new item under the given key.
 * PARAS: - items: the pointer of skip list
 *		  - key: the key for updating, it must exist in the store
 *		  - val: the value for updating
 * RETURNS: 0 for success and (-1) for error
 * ERRORS: - The items is NULL
 *		   - The key or value is NULL
 *         - The key does not exist
 */
int update_skiplist(Skiplist* items, const char* key, const char* val);

/*
 * Delete an item, optionally freeing the value.
 * PARAS: - items: the pointer of skip list
 *		  - key: the key for deletion, it must exist in the store
 * RETURNS: 0 for success and (-1) for error
 * ERRORS: - The key is null
 *		   - The key does not exist
 */
int delete_skiplist(Skiplist* items, const char* key);

/*
 * Count the number of items stored.
 * PARAS: the pointer of skip list
 * RETURNS: the number of items stored, cannot fail
 */
int count_skiplist(Skiplist* items);

/*
 * Free the skip list.
 * PARAS: the pointer of skip list
 * RETURNS: 0 for success and (-1) for error
 */
int free_skiplist(Skiplist* items);

#endif
