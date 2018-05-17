#ifndef _queue_h_
#define _queue_h_

#define QUEUE_SIZE 10

struct queue {
	int front;
	int rear;
	int count;
	int intArray[QUEUE_SIZE];
};
typedef struct queue Queue;

/*
 * Initialise the queue.
 * RETURNS: a pointer of queue or NULL
 */
Queue* init_queue();

/*
 * Check the queue is empty or not.
 * PARAS: - que: the pointer of queue
 * RETURNS: if the queue is empty return 1
 * else return 0 and -1 for error
 */
int empty_queue(Queue* que);

/*
 * Check the queue is full or not.
 * PARAS: - que: the pointer of queue
 * RETURNS: if the queue is full return 1
 * else return 0 and -1 for error
 */
int full_queue(Queue* que);

/*
 * Gain the size of queue.
 * PARAS: - que: the pointer of queue
 * RETURNS: the size of queue and -1 for error
 */
int size_queue(Queue* que);

/*
 * Add a new value into the queue.
 * PARAS: - que: the pointer of queue
 *		  - val: the new value
 * RETURNS: 0 for success, 1 for failure and -1 for error
 */
int push_queue(Queue* que, int val);

/*
 * Gain and delete the first value from queue.
 * PARAS: - que: the pointer of queue
 *		  - val: the pointer for storing the value
 * RETURNS: 0 for success, 1 for failure and -1 for error
 */
int pop_queue(Queue* que, int* val);

/*
 * Release the queue.
 * PARAS: - que: the pointer of queue
 * RETURNS: 0 for success  and -1 for error
 */
int free_queue(Queue* que);

#endif
