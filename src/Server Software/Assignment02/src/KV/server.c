/*
 * Server program for key-value store.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <poll.h>
#include <pthread.h>

#include "parser.h"
#include "kv.h"
#include "queue.h"

#define NTHREADS 4
#define BACKLOG 10


/* 
 * Global variables
 * Including queue and five semaphores
 */
/* queue */
Queue* Que;
/* semaphores */
sem_t s_shutdown, s_work_avail, s_space_avail, s_data_lock, s_queue_lock;


/* 
 * data_work(fd): Handle the data commands.
 * PARAS: fd - a file descriptor of the control socket.
 * Three kind control command:
 * - D_PUT: PUT or UPDATE a key.
 * - D_GET: GET a key.
 * - D_COUNT: COUNT the item number in kv store.
 * - D_DELETE: DELETE a key.
 * - D_EXISTS: Check a key is exist or not.
 * - D_END: Stop the data request.
 * - D_ERR_OL: Command is too long.
 * - D_ERR_INVALID: Invalid command.
 * - D_ERR_SHORT: Too few arguments.
 * - D_ERR_LONG: Too many arguments.
 */
void data_work(int fd)
{
	/* The variable for storing error code. */
	int err;
	/* The variable for describing loop status. */
	int run = 1;
	/* The variable for storing the command. */
	enum DATA_CMD cmd;
	/* The variable for IO. */
	char buffer[LINE], copy[LINE];
	/* The variable for storing the key and value. */
	char *key, *text;

	/* Loop for waiting new command. */
	while (run) {

		/* Output the shell prompt. */
		strncpy(buffer, "Please enter a command > ", LINE);
		err = write(fd, buffer, LINE);
		if (err < 0) {
			perror("Write response fail");
			exit(EXIT_FAILURE);
		}

		/* Read in command from file descriptor. */
		err = read(fd, buffer, LINE);
		if (err < 0) {
			perror("Read command fail");
			exit(EXIT_FAILURE);
		}

		err = parse_d(buffer, &cmd, &key, &text);


		/* Acquire the data locker. */
		err = sem_wait(&s_data_lock);
		if (err != 0) {
			perror("Wait s_data_lock failed");
			exit(EXIT_FAILURE);
		}

		if (cmd == D_PUT) {
			/* PUT command. */
			
			/* Copy the value to the memory which is allocated by malloc. */
			char* cp_msg = (char*) malloc((strlen(text)+1)*sizeof(char));
			if (cp_msg == NULL) {
				fprintf(stderr, "Memory allocation problem.\n");
				exit(EXIT_FAILURE);
			}
			strncpy(cp_msg, text, strlen(text) + 1);

			/* Check the key is exist or not. */
			err = itemExists(key);
			if (err == 0) {

				/* Insert command. */
				err = createItem(key, cp_msg);
				if (err == 0) {
					strncpy(buffer, "Stored key successfully.\n", LINE);
				} else {
					strncpy(buffer, "Error storing key.\n", LINE);
				}

			} else if (err == 1) {

				/* Update command. */
				err = updateItem(key, cp_msg);
				if (err == 0) {
					strncpy(buffer, "Stored key successfully.\n", LINE);
				} else {
					strncpy(buffer, "Error storing key.\n", LINE);
				}

			} else {
				/* Error for check key. */
				strncpy(buffer, "Error in exists function.\n", LINE);
			}

		} else if (cmd == D_GET) {

			/* GET command. */
			text = findValue(key);
			if (text != NULL) {
				sprintf(copy, "%s\n", text);
				strncpy(buffer, copy, LINE);
			} else {
				strncpy(buffer, "No such key.\n", LINE);
			}

		} else if (cmd == D_COUNT) {

			/* Count command. */
			sprintf(copy, "%d\n", countItems());
			strncpy(buffer, copy, LINE);

		} else if (cmd == D_DELETE) {

			/* Delete command. */
			err = deleteItem(key, 1);
			if (err == 0) {
				strncpy(buffer, "Deleting key successfully.\n", LINE);
			} else {
				strncpy(buffer, "Error deleting key.\n", LINE);
			}

		} else if (cmd == D_EXISTS) {

			/* Exist command. */
			err = itemExists(key);
			if (err == 1) {
				strncpy(buffer, "1\n", LINE);
			} else if (err == 0) {
				strncpy(buffer, "0\n", LINE);
			} else {
				strncpy(buffer, "Error in exist function.\n", LINE);
			}

		} else if (cmd == D_END) {

			/* End command. */
			/* Write response message to buffer. */
			strncpy(buffer, "goodbye.\n", LINE);
			/* Stop the loop. */
			run = 0;

		} else if (cmd == D_ERR_OL) {

			/* Command too lang. */
			strncpy(buffer, "Command too long.\n", LINE);

		} else if (cmd == D_ERR_INVALID) {

			/* Invalid command. */
			strncpy(buffer, "Invalid command.\n", LINE);

		} else if (cmd == D_ERR_SHORT) {

			/* Too few parameters . */
			strncpy(buffer, "Too few parameters.\n", LINE);

		} else if (cmd == D_ERR_LONG) {

			/* Too many parameters . */
			strncpy(buffer, "Too many parameters.\n", LINE);

		} else {

			/* Error command. */
			strncpy(buffer, "What the deuce?\n", LINE);

		}

		/* Release the data locker. */
		err = sem_post(&s_data_lock);
		if (err != 0) {
			perror("Post s_data_lock failed");
			exit(EXIT_FAILURE);
		}

		/* Write response message to file descriptor. */
		err = write(fd, buffer, LINE);
		if (err < 0) {
			perror("Write reponse failed");
			exit(EXIT_FAILURE);
		}
	}
	
	/* Close the file descriptor. */
	err = close(fd);
	if (err != 0) {
		perror("Close the file descriptor failed");
		exit(EXIT_FAILURE);
	}
}


/* 
 * control_work(fd, pid): Handle the control commands.
 * PARAS: fd - a file descriptor of the control socket.
 *		  pid - the thread number for SHUTDOWN command.
 * Three kind control command:
 * - C_SHUTDOWN: set the shutdown signal and wait for all thread exit.
 * - C_COUNT: get the item number in kv-store.
 * - C_ERROR: error command.
 */
void control_work(int fd, pthread_t* pid)
{
	/* The variable for describing loop status. */
	int run = 1;
	/* The variable for storing error code. */
	int err;
	/* The variable for storing the command. */
	enum CONTROL_CMD cmd;
	/* The variable for IO. */
	char buffer[LINE], copy[LINE];

	/* Loop for waiting new command. */
	while (run) {

		/* Output the shell prompt. */
		strncpy(buffer, "Please enter a command > ", LINE);
		err = write(fd, buffer, LINE);
		if (err < 0) {
			perror("Write response fail");
			exit(EXIT_FAILURE);
		}

		/* Read in command from file descriptor. */
		err = read(fd, buffer, LINE);
		if (err < 0) {
			perror("Read command fail");
			exit(EXIT_FAILURE);
		}

		cmd = parse_c(buffer);

		if (cmd == C_SHUTDOWN) {
			/* SHUTDOWN command. */

			/* Set the shutdown signal. */
			err = sem_post(&s_shutdown);
			if (err != 0) {
				perror("Post s_shutdown failed");
				exit(EXIT_FAILURE);
			}

			/* Release enough work avail. */
			for (int i = 0; i < NTHREADS; i++) {
				err = sem_post(&s_work_avail);
				if (err != 0) {
					perror("Post s_work_avail failed");
					exit(EXIT_FAILURE);
				}
			}

			/* Wait for all worker threads to finish. */
			for (int i = 0; i < NTHREADS; i++) {
				err = pthread_join(pid[i], NULL);
				if (err != 0) {
					perror("Wait thread finish failed");
					exit(EXIT_FAILURE);
				}
			}

			/* Write response message to buffer. */
			strncpy(buffer, "Server is closed.\n", LINE);

			/* Stop the loop. */
			run = 0;

		} else if (cmd == C_COUNT) {
			/* COUNT command. */

			/* Acquire the data locker. */
			err = sem_wait(&s_data_lock);
			if (err != 0) {
				perror("Wait s_data_lock failed");
				exit(EXIT_FAILURE);
			}

			/* Gain the item number in kv-store. */
			sprintf(copy, "%d\n", countItems());

			/* Release the data locker. */
			err = sem_post(&s_data_lock);
			if (err != 0) {
				perror("Post s_data_lock failed");
				exit(EXIT_FAILURE);
			}

			/* Write response message to buffer. */
			strncpy(buffer, copy, LINE);

		} else {

			/* Error command. */
			strncpy(buffer, "What the deuce?\n", LINE);

		}

		/* Write response message to file descriptor. */
		err = write(fd, buffer, LINE);
		if (err < 0) {
			perror("Write reponse failed");
			exit(EXIT_FAILURE);
		}
	}

	/* Close the file descriptor. */
	err = close(fd);
	if (err != 0) {
		perror("Close the file descriptor failed");
		exit(EXIT_FAILURE);
	}
}


/* 
 * A worker thread: create a new work thread
 * to handle the data requests.
 */
void *worker(void *p)
{
	/* The variable for storing file descriptor. */
	int fd;
	/* The variable for storing error code. */
	int err;
	/* The variable for describing loop status. */
	int run = 1;
	/* The variable for storing return value. */
	int val;
	/* The variable for IO. */
	char buffer[LINE];

	/* Work thread waits to handle request. */
	while (run) {

		/* Acquire the work space. */
		err = sem_wait(&s_work_avail);
		if (err != 0) {
			perror("Wait s_work_avail fail");
			exit(EXIT_FAILURE);
		}

		/* Acquire the queue locker. */
		err = sem_wait(&s_queue_lock);
		if (err != 0) {
			perror("Wait s_queue_lock fail");
			exit(EXIT_FAILURE);
		}

		/* Gain the file descriptor of request. */
		err = pop_queue(Que, &fd);
		if (err == -1) {
			exit(EXIT_FAILURE);
		}
		
		/* Check is there a new request or just shutdown server. */
		if (err == 0) {
			/* New data request. */

			/* Release the queue space. */
			err = sem_post(&s_space_avail);
			if (err != 0) {
				perror("Post s_space_avail fail");
				exit(EXIT_FAILURE);
			}

			/* Release the queue locker. */
			err = sem_post(&s_queue_lock);
			if (err != 0) {
				perror("Post s_queue_lock fail");
				exit(EXIT_FAILURE);
			}

			/* Gain the shutdown signal before work. */
			err = sem_getvalue(&s_shutdown, &val);
			if (err != 0) {
				perror("Get s_shudown value failed");
				exit(EXIT_FAILURE);
			}

			/* Check the server is shutdown or not. */
			if (val != 0) {

				/* Write response message to file descriptor. */
				strncpy(buffer, "Server is shutdown.\n", LINE);
				err = write(fd, buffer, LINE);
				if (err < 0) {
					perror("Write response fail");
					exit(EXIT_FAILURE);
				}

				/* Close the file descriptor before
				 * worker thread exit. */
				close(fd);
				continue;
			}

			/* Handle the data commands. */
			data_work(fd);

		} else {
			/* Server shutdown or pop a empty queue. */

			/* Release the queue locker. */
			err = sem_post(&s_queue_lock);
			if (err != 0) {
				perror("Post s_queue_lock fail");
				exit(EXIT_FAILURE);
			}

			/* Check the shutdown signal. */
			err = sem_getvalue(&s_shutdown, &val);
			if (err != 0) {
				perror("Get s_shudown value failed");
				exit(EXIT_FAILURE);
			}

			/* Check the server is shutdown or not. */
			if (val != 0) return NULL;
		}
	}
	return NULL;
}


/* 
 * init_socket(port): Initialise a new socket.
 * Create a new socket and bind it to the specific port.
 * PARAS: port - the port number for binding
 * RETURNS: a file descriptor
 */
int init_socket(int port)
{
	/* The variable for storing error code. */
	int err;
	/* The variable for storing server file descriptor. */
	int server_fd;
	/* The variable for creating socket. */
	struct sockaddr_in address;

	/* Create new socket. */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("Create new socket failed");
		exit(EXIT_FAILURE);
	}

	/* Initialise the new socket. */
	memset(&address, '0', sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	/* Bind the socket to port. */
	err = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
	if (err != 0) {
		perror("Bind socket failed");
		exit(EXIT_FAILURE);
	}

	/* Listen the port. */
	err = listen(server_fd, 10);
	if (err != 0) {
		perror("Listen socket failed");
		exit(EXIT_FAILURE);
	}

	return server_fd;
}


/* 
 * initialise(): Initialise the global variables.
 * including kv-store, queue and five sempahores.
 */
void initialise()
{
	/* The variable for storing error code. */
	int err;

	/*
	 * Initialise five semaphores.
	 * including s_data_lock, s_queue_lock,
	 * s_work_avail, s_space_avail, s_shutdown
	 */

	/* Initialise the data locker. */
	err = sem_init(&s_data_lock, 0, 1);
	if (err != 0) {
		perror("Initialise s_data_lock failed");
		exit(EXIT_FAILURE);
	}

	/* Initialise the queue locker. */
	err = sem_init(&s_queue_lock, 0, 1);
	if (err != 0) {
		perror("Initialise s_queue_lock failed");
		exit(EXIT_FAILURE);
	}

	/* Initialise the work avail. */
	err = sem_init(&s_work_avail, 0, 0);
	if (err != 0) {
		perror("Initialise s_work_avail failed");
		exit(EXIT_FAILURE);
	}

	/* Initialise the space avail. */
	err = sem_init(&s_space_avail, 0, QUEUE_SIZE + 1);
	if (err != 0) {
		perror("Initialise s_space_avail failed");
		exit(EXIT_FAILURE);
	}

	/* Initialise the shutdown signal. */
	err = sem_init(&s_shutdown, 0, 0);
	if (err != 0) {
		perror("Initialise s_shutdown failed");
		exit(EXIT_FAILURE);
	}


	/*
	 * Initialise the kv store.
	 * The global variable `Items` protected by s_data_lock.
	 */


	/*
	 * Initialise the queue.
	 * The global variable `Que` protected by s_queue_lock.
	 */

	/* Acquire the queue locker. */
	err = sem_wait(&s_queue_lock);
	if (err != 0) {
		perror("Wait s_queue_lock failed");
		exit(EXIT_FAILURE);
	}

	/* Initialise the queue. */
	Que = init_queue();
	if (Que == NULL) {
		exit(EXIT_FAILURE);
	}

	/* Release the queue locker. */
	err = sem_post(&s_queue_lock);
	if (err != 0) {
		perror("Post s_queue_lock failed");
		exit(EXIT_FAILURE);
	}
}


/* 
 * finalise(): Release the global variables.
 * including kv-store, queue and five sempahores.
 */
void finalise()
{
	/* The variable for storing error code. */
	int err;

	/*
	 * Release the queue.
	 * The global variable `Que` protected by s_queue_lock.
	 */

	/* Acuqire the queue locker. */
	err = sem_wait(&s_queue_lock);
	if (err != 0) {
		perror("Wait s_data_lock failed");
		exit(EXIT_FAILURE);
	}

	/* Release the queue. */
	err = free_queue(Que);
	if (err != 0) {
		exit(EXIT_FAILURE);
	}

	/* Release the queue locker. */
	err = sem_post(&s_queue_lock);
	if (err != 0) {
		perror("Post s_data_lock failed");
		exit(EXIT_FAILURE);
	}


	/*
	 * Release five semaphores.
	 * including s_data_lock, s_queue_lock,
	 * s_work_avail, s_space_avail, s_shutdown
	 */

	/* Release the data locker. */
	err = sem_destroy(&s_data_lock);
	if (err != 0) {
		perror("Destroy s_data_lock failed");
		exit(EXIT_FAILURE);
	}

	/* Release the queue locker. */
	err = sem_destroy(&s_queue_lock);
	if (err != 0) {
		perror("Destroy s_queue_lock failed");
		exit(EXIT_FAILURE);
	}

	/* Release the work avail. */
	err = sem_destroy(&s_work_avail);
	if (err != 0) {
		perror("Destroy s_work_avail failed");
		exit(EXIT_FAILURE);
	}

	/* Release the space avail. */
	err = sem_destroy(&s_space_avail);
	if (err != 0) {
		perror("Destroy s_space_avail failed");
		exit(EXIT_FAILURE);
	}

	/* Release the shutdown signal. */
	err = sem_destroy(&s_shutdown);
	if (err != 0) {
		perror("Destroy s_shutdown failed");
		exit(EXIT_FAILURE);
	}
}


/* 
 * handle_request(cport_fd, dport_fd): Listen the 
 * control port and data port to handle the requests.
 * pid - the thread numbers for contorl requests.
 */
void handle_request(int cport_fd, int dport_fd, pthread_t* pid)
{
	/* The variable for storing error code. */
	int err;
	/* The variable for describing loop status. */
	int run = 1;
	/* The variable for storing file descriptor. */
	int connfd;
	/* The variable for storing return value. */
	int val;

	/* Set polling configuration. */
	struct pollfd fds[2];
	fds[0].fd = cport_fd;
	fds[0].events = POLLIN;
	fds[1].fd = dport_fd;
	fds[1].events = POLLIN;

	/* Main thread waits for client requests. */
	while (run) {

		/* Polling the contorl and data file descriptors. */
		poll(fds, 2, -1);

		/* Handle control request. */
		if (fds[0].revents && POLLIN) {
			/* Accept request from control port. */
			connfd = accept(fds[0].fd, (struct sockaddr*)NULL, NULL);
			if (connfd < 0) {
				perror("Accept control request failed");
				exit(EXIT_FAILURE);
			}

			/* Handle the control commands. */
			control_work(connfd, pid);

			/* Check the shutdown signal. */
			err = sem_getvalue(&s_shutdown, &val);
			if (err != 0) {
				perror("Get s_shudown value failed");
				exit(EXIT_FAILURE);
			}

			/* Server should shutdown. */
			if (val != 0) break;
		}

		/* Handle data request. */
		if (fds[1].revents && POLLIN) {
			/* Accept request from data port. */
			connfd = accept(fds[1].fd, (struct sockaddr*)NULL, NULL);
			if (connfd < 0) {
				perror("Accept data request failed");
				exit(EXIT_FAILURE);
			}

			/* Acquire the queue space. */
			err = sem_wait(&s_space_avail);
			if (err != 0) {
				perror("Wait s_space_avail failed");
				exit(EXIT_FAILURE);
			}

			/* Acquire the queue lock. */
			err = sem_wait(&s_queue_lock);
			if (err != 0) {
				perror("Wait s_queue_lock failed");
				exit(EXIT_FAILURE);
			}

			/* Add the file descriptor to the queue. */ 
			err = push_queue(Que, connfd);
			if (err == 1) {

				/* Queue is full, write busy message to file descriptor. */
				char buffer[LINE];
				strncpy(buffer, "Server is busy now.\n", LINE);
				err = write(connfd, buffer, LINE);
				if (err < 0) {
					perror("Write reponse failed");
					exit(EXIT_FAILURE);
				}   
				close(connfd);

				/* Release the space avail. */
				err = sem_post(&s_space_avail);
				if (err != 0) {
					perror("Post s_space_avail failed");
					exit(EXIT_FAILURE);
				}

			} else if (err == -1) {
				fprintf(stderr, "push_queue: error.\n");
				exit(EXIT_FAILURE);
			} else {
				/* Release the worker avail. */
				err = sem_post(&s_work_avail);
				if (err != 0) {
					perror("Post s_work_avail failed");
					exit(EXIT_FAILURE);
				}
			}

			/* Release the queue locker. */
			err = sem_post(&s_queue_lock);
			if (err != 0) {
				perror("Post s_queue_lock failed");
				exit(EXIT_FAILURE);
			}
		}
	}
}


int main(int argc, char **argv)
{
	/* The variable for storing error code */
	int err;

	/* Check the number of arguments is enough or not. */
	if (argc < 3) {
		fprintf(stderr, "Usage: %s control-port data-port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Initialise the global variables. */
	initialise();

	/* Gain the contorl and data ports from arguments. */
	int cport = atoi(argv[1]);
	int dport = atoi(argv[2]);

	/* Initalise the control and data sockets. */
	int cport_fd = init_socket(cport);
	int dport_fd = init_socket(dport);

	/* Create the workers to handle data requests. */
	pthread_t pid[NTHREADS];
	for (int i = 0; i < NTHREADS; i++) {
		err = pthread_create(&pid[i], NULL, worker, NULL);
		if (err != 0) {
			perror("Create thread failed");
			exit(EXIT_FAILURE);
		}
	}

	/* Handle the request from control port and data port. */
	handle_request(cport_fd, dport_fd, pid);

	/* Close the file descriptor of control port. */
	err = close(cport_fd);
	if (err != 0) {
		perror("Close the file descriptor failed");
		exit(EXIT_FAILURE);
	}

	/* Close the file descriptor of data port. */
	err = close(dport_fd);
	if (err != 0) {
		perror("Close the file descriptor failed");
		exit(EXIT_FAILURE);
	}

	/* Release the global variables. */
	finalise();

	return 0;
}
