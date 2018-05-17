/* 
 * The implementation of skip list.
 * Reference: 'Skip lists: a probabilistic alternative to balanced trees'
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "skiplist.h"

Node* new_node() {
	Node* ret = (Node*)malloc(sizeof(Node));
	if (ret == NULL) {
		fprintf(stderr, "**ERROR**: new_node() fail"
				" to apply new node.\n");
		exit(1);
	}
	memset(ret->forwards, 0, sizeof(ret->forwards));
	return ret;
}

int random_level() {
	int v = 1;
	while (v < SKL_MAXLEVEL && (double)rand() / RAND_MAX < SKL_PROB) {
		v++;
	}
	return v;
}

Skiplist* init_skiplist() {
	Skiplist* items = (Skiplist*)malloc(sizeof(Skiplist));

	if (items == NULL) {
		fprintf(stderr, "**ERROR**: init_skiplist() fail to"
				" initialise the skip list.\n");
		return NULL;
	}

	items->nItems = 0;
	items->level = 0;
	items->header = new_node();
	return items;
}

/* 0 = success, 1 = failed, -1 = error */
int search_skiplist(Skiplist* items, const char* key, char* ret) {
	if (items == NULL) return -1;
	if (key == NULL || strlen(key) >= SKL_MAXKEY) return -1;
    if (ret == NULL) return -1;

	Node* x = items->header;
	for (int i = items->level - 1; i >= 0; i--) {
		while (x->forwards[i] != NULL &&
				strcmp(x->forwards[i]->key, key) < 0) {
			x = x->forwards[i];
		}
	}

	if (x->forwards[0] != NULL) {
		x = x->forwards[0];
		if (strcmp(x->key, key) == 0) {
			strcpy(ret, x->val);
			return 0;
		}
	}

	return 1;
}

/* 1 = exist, 0 = no such item, -1 = error */
int exists_skiplist(Skiplist* items, const char* key) {
	if (items == NULL) return -1;
	if (key == NULL || strlen(key) >= SKL_MAXKEY) return -1;

	Node* x = items->header;
	for (int i = items->level - 1; i >= 0; i--) {
		while (x->forwards[i] != NULL &&
				strcmp(x->forwards[i]->key, key) < 0) {
			x = x->forwards[i];
		}
	}

	if (x->forwards[0] != NULL) {
		x = x->forwards[0];
		if (strcmp(x->key, key) == 0) {
			return 1;
		}
	}

	return 0;
}

/* 0 = success, -1 = error */
int insert_skiplist(Skiplist* items, const char* key, const char* val) {
	if (items == NULL) return -1;
	if (key == NULL || strlen(key) >= SKL_MAXKEY) return -1;
    if (val == NULL || strlen(val) >= SKL_MAXKEY) return -1;
	if (items->nItems == SKL_MAXITEMS - 1) return -1;

	Node* updates[SKL_MAXLEVEL];
	Node* x = items->header;

	for (int i = items->level - 1; i >= 0; i--) {
		while (x->forwards[i] != NULL &&
				strcmp(x->forwards[i]->key, key) < 0) {
			x = x->forwards[i];
		}
		updates[i] = x;
	}

	if (x->forwards[0] != NULL) {
		x = x->forwards[0];
		/* Key is exists */
		if (strcmp(x->key, key) == 0) {
			return -1;
		}
	}

	int lvl = random_level();

	if (lvl > items->level) {
		for (int i = items->level; i < lvl; i++)
			updates[i] = items->header;
		items->level = lvl;
	}

	Node* y = new_node();
	strcpy(y->key, key);
	strcpy(y->val, val);

	for (int i = 0; i < items->level; i++) {
		x = updates[i];
		y->forwards[i] = x->forwards[i];
		x->forwards[i] = y;
	}

	items->nItems++;
	return 0;
}

/* 0 = success, 1 = fail, -1 = error */
int update_skiplist(Skiplist* items, const char* key, const char* val) {
	if (items == NULL) return -1;
	if (key == NULL || strlen(key) >= SKL_MAXKEY) return -1;
    if (val == NULL || strlen(val) >= SKL_MAXKEY) return -1;

	Node* updates[SKL_MAXLEVEL];

	Node* x = items->header;
	for (int i = items->level - 1; i >= 0; i--) {
		while (x->forwards[i] != NULL &&
				strcmp(x->forwards[i]->key, key) < 0) {
			x = x->forwards[i];
		}
		updates[i] = x;
	}

	if (x->forwards[0] != NULL) {
		x = x->forwards[0];
		/* Key is exists */
		if (strcmp(x->key, key) == 0) {
			strcpy(x->val, val);
			return 0;
		}
	}

	return 1;
}

/* 0 = success, -1 = error */
int delete_skiplist(Skiplist* items, const char* key) {
	if (items == NULL) return -1;
	if (key == NULL || strlen(key) >= SKL_MAXKEY) return -1;

	Node* updates[SKL_MAXLEVEL];

	Node* x = items->header;
	for (int i = items->level - 1; i >= 0; i--) {
		while (x->forwards[i] != NULL &&
				strcmp(x->forwards[i]->key, key) < 0) {
			x = x->forwards[i];
		}
		updates[i] = x;
	}

	if (x->forwards[0] != NULL) {
		x = x->forwards[0];
		if (strcmp(x->key, key) == 0) {
			for (int i = 0; i < items->level; i++) {
				if (updates[i]->forwards[i] != x) break;
				updates[i]->forwards[i] = x->forwards[i];
			}
			free(x);
			items->nItems--;
			/* Update maximum level */
			while (items->level && items->header->forwards[items->level-1] == NULL) {
				items->level--;
			}
			return 0;
		}
	}
	return -1;
}

/* 0 = success, -1 = error */
int count_skiplist(Skiplist* items) {
	if (items == NULL) return -1;
	return items->nItems;
}

/* 0 = success, -1 = error */
int free_skiplist(Skiplist* items) {
	if (items == NULL) return -1;

	Node* x = items->header;
	Node* t;

	while (x != NULL) {
		t = x;
		x = x->forwards[0];
		free(t);
	}
	free(items);
	return 0;
}
