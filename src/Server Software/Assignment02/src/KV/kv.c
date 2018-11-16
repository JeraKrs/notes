/* The kv store implementation.
 * Neither thread-safe nor optimized.
 */

#include <string.h>
#include <stdlib.h>
#include "kv.h"

#define NITEMS 100

struct item {
    char* key;
    char* value;
};

struct item items[NITEMS];
int nItems = 0;

/* Of course a hashtable would be much more efficient. */

/* Check if an item exists. If so, return the item, else NULL. */
struct item* findItem(const char* key) {
    for (int i = 0; i < nItems; i++) {
        if (!strcmp(items[i].key, key)) { return &items[i]; }
    }
    return NULL;
}

/* API version of find. */
char* findValue(const char* key) {
    struct item *i = findItem(key);
    if (i != NULL) {
        return i->value;
    } else {
        return NULL;
    }
}

/* 1 = exists, 0 = does not exist. */
int itemExists(const char* key) {
    return (findItem(key) != NULL);
}

/* 0 = success, -1 = failed (item exists or table full) */
int createItem(const char* key, char* value) {
	if (key == NULL)      { return -1; }
	if (value == NULL)    { return -1; }
	if (nItems == NITEMS - 1) { return -1; }
	if (itemExists(key))  { return -1; }
	char* copy = malloc(strlen(key) + 1);
	if (copy == NULL) { return -1; }
	strncpy(copy, key, strlen(key) + 1);
	items[nItems].key = copy;
	items[nItems].value = value;
	nItems++;
	return 0;
}

/* 0 = success, -1 = failed (does not exist) */
int updateItem(const char* key, char* newValue) {
	if (key == NULL || newValue == NULL) { return -1; }
	struct item *i = findItem(key);
	if (i == NULL) { return -1; }
	i->value = newValue;
	return 0;
}

/* 0 = success, -1 = error (does not exist) */
int deleteItem(const char* key, int free_it) {
	if (key == NULL) { return -1; }
	struct item *i = findItem(key);
	if (i == NULL) { return -1; }
	if (free_it) { free(i->value); }

	struct item *last = items + nItems - 1;
	if (i == last) {
		nItems--;
		return 0;
	} else {
		memcpy(i, last, sizeof(struct item));
		nItems--;
		return 0;
	}
}

int countItems() {
	return nItems;
}
