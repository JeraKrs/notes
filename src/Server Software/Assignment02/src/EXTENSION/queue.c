/*
 * The implementation of queue.
 */
#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

Queue* init_queue() {
	Queue* que = (Queue*)malloc(sizeof(Queue));
	if (que == NULL) {
		fprintf(stderr, "**ERROR**: initQueue() fail to"
				" initialise the queue.\n");
		return NULL;
	}

	que->front = 0;
	que->rear = 0;
	que->count = 0;
	return que;
}

/* returns 1 when empty and 0 otherwise */
int empty_queue(Queue* que) {
	if (que == NULL) return -1;
	return que->count == 0;
}

/* returns 1 when full and 0 otherwise */
int full_queue(Queue* que) {
	if (que == NULL) return -1;
	return que->count == QUEUE_SIZE;
}

/* returns number of items in queue */
int size_queue(Queue* que) {
	if (que == NULL) return -1;
	return que->count;
}

/* 0 for success, 1 for failure and -1 for error */
int push_queue(Queue* que, int val) {
	if (que == NULL) return -1;
	if (full_queue(que) == 1) return 1;

	que->intArray[que->rear] = val;
	que->rear = (que->rear + 1) % QUEUE_SIZE;
	que->count = que->count + 1;

	return 0;
}

/* 0 for success, 1 for failure and -1 for error */
int pop_queue(Queue* que, int* val) {
	if (que == NULL) return -1;
	if (empty_queue(que) == 1) return 1;

	*val = que->intArray[que->front];
	que->front = (que->front + 1) % QUEUE_SIZE;
	que->count = que->count - 1;

	return 0;
}

/* 0 for success and -1 for error */
int free_queue(Queue* que) {
	if (que == NULL) return -1;
	free(que);
	return 0;
}
