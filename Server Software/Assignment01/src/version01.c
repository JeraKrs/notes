/* 
 * Server Software (COMSM2001), 2017-18, Coursework 1.
 *
 * This is a skeleton program for COMSM2001 (Server Software) coursework 1
 * "the project marking problem". Your task is to synchronise the threads
 * correctly by adding code in the places indicated by comments in the
 * student, marker and run functions.
 * You may create your own global variables and further functions.
 * The code in this skeleton program can be used without citation in the files
 * that you submit for your coursework.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

/*
 * Parameters of the program. The constraints are D < T and
 * S*K <= M*N.
 */
struct demo_parameters {
    int S;   /* Number of students */
    int M;   /* Number of markers */
    int K;   /* Number of markers per demo */
    int N;   /* Number of demos per marker */
    int T;   /* Length of session (minutes) */
    int D;   /* Length of demo (minutes) */
};

/* Global object holding the demo parameters. */
struct demo_parameters parameters;

/* The demo start time, set in the main function. Do not modify this. */
struct timeval starttime;


/* 
 * You may wish to place some global variables here. 
 * Remember, globals are shared between threads.
 * You can also create functions of your own.
 */

/* The status of markers */
struct marker_status {
	int markerID;	/* The ID of the recorded marker */
	int is_grabbed;	/* Marking whether the marker is grabbed by student */
	int studentID;	/* The ID of the student that grabbed this marker if it is grabbed */
	int job;		/* Indicate how many job the marker have done */
};
struct marker_status* markers;

/*
struct heap {
	int size;
	int max_size;
	struct marker_status* elements;
};
*/

/* The number of idle marker */
int idle_marker;

/* 
 * Marking whether the remaining time is enough for a new demo start.
 *	timeout=0 means the reamining time is enough;
 *	timeout=1 means all markers and students not in demo should exit lab;
 */
int timeout;

/* Mutex lock for preventing race conditions in global variables */
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

/* Condition variable for marker and student */
pthread_cond_t c_marker = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_student = PTHREAD_COND_INITIALIZER;

/* initialise(): initialise the global variables */
void initialise() {

	/* All markers are idle at the begining */
	idle_marker = parameters.M;

	/* 
	 * It must have time for a new demo at the begining
	 * since parameters.D >= parameters.T
	 */
	timeout = 0;

	/* Dynamically allocate the memory for the pointer of markers */
	markers = (struct marker_status *)malloc(parameters.M * sizeof(struct marker_status));
	if (markers == NULL) { 
		fprintf(stderr, "**ERROR**: main thread is failed to allocate "
				"memory for the pointer of markers.\n");
		abort();
	}

	/* Initialise the status of markers */
	for (int i = 0; i < parameters.M; i++) {
		markers[i].markerID = i;
		markers[i].is_grabbed = 0;
		markers[i].studentID = -1;
		markers[i].job = 0;
	}
}

/*
 * grab(): grab markers
 * return 0 means success, else fail
 */
int grab(int studentID, int* markerIDs) {

	int num_grabbed = 0, markerID = 0;

	while ((num_grabbed < parameters.K) && (markerID < parameters.M)) {
		if (markers[markerID].is_grabbed == 0) {

			/* Modify the status of marker */
			markerIDs[num_grabbed] = markerID;
			markers[markerID].is_grabbed = 1;
			markers[markerID].studentID = studentID;

			num_grabbed = num_grabbed + 1;
		}
		markerID = markerID + 1;
	}

	idle_marker = idle_marker - parameters.K;

	if (idle_marker < 0) { 
		fprintf(stderr, "grab(): The number of idle marker "
				"is less than zero after grabbed.\n");
		return 1;
	}

	if (num_grabbed != parameters.K) {
		fprintf(stderr, "grab(): The number of idle marker "
				"is less than %d.\n", parameters.K);
		return 2;
	}

	return 0;
}


/*
 * release(): release marker
 * return 0 means success, else fail
 */
int release(int markerID, int job) {

	if (markerID < 0 || markerID >= parameters.M) {
		fprintf(stderr, "release(): The markerID=%d is illegal.\n", markerID);
		return 2;
	}

	markers[markerID].job = job;

	idle_marker = idle_marker + 1;
	if (idle_marker > parameters.M) {
		fprintf(stderr, "release(): The number of idle marker is "
				"bigger than %d after release.\n", parameters.M);
		return 1;
	}

	return 0;
}


/*
 * timenow(): returns current simulated time in "minutes" (cs).
 * Assumes that starttime has been set already.
 * This function is safe to call in the student and marker threads as
 * starttime is set in the run() function.
 */
int timenow() {
	struct timeval now;
	gettimeofday(&now, NULL);
	return (now.tv_sec - starttime.tv_sec) * 100 + (now.tv_usec - starttime.tv_usec) / 10000;
}

/* delay(t): delays for t "minutes" (cs) */
void delay(int t) {
	struct timespec rqtp, rmtp;
	t *= 10;
	rqtp.tv_sec = t / 1000;
	rqtp.tv_nsec = 1000000 * (t % 1000);
	nanosleep(&rqtp, &rmtp);
}

/* panic(): simulates a student's panicking activity */
void panic() {
	delay(random() % (parameters.T - parameters.D));
}

/* demo(): simulates a demo activity */
void demo() {
	delay(parameters.D);
}

/*
 * A marker thread. You need to modify this function.
 * The parameter arg is the number of the current marker and the function
 * doesn't need to return any values.
 * Do not modify the printed output as it will be used as part of the testing.
 */
void *marker(void *arg) {
	int markerID = *(int *)arg;

	/*
	 * The following variable is used in the printf statements when a marker is
	 * grabbed by a student. It shall be set by this function whenever the
	 * marker is grabbed - and before the printf statements referencing it are
	 * executed.
	 */
	int studentID;

	/*
	 * The following variable shall indicate which job the marker is currently
	 * executing, the first being job 0. The variable ranges from 0 to
	 * (parameters.N - 1) .
	 */
	int job = 0;

	/* Error code */
	int err;

	/* 1. Enter the lab. */
	printf("%d marker %d: enters lab\n", timenow(), markerID);

	/* A marker marks up to N projects. */
	/* 2. Repeat (N times). 
	 *    (a) Wait to be grabbed by a student.
	 *    (b) Wait for the student's demo to begin
	 *        (you may not need to do anything here).
	 *    (c) Wait for the demo to finish.
	 *        Do not just wait a given time -
	 *        let the student signal when the demo is over.
	 *    (d) Exit the lab.
	 */

	/* Repeat N times */
	while (job < parameters.N) {

		err = pthread_mutex_lock(&mut);
		if (err != 0) {
			fprintf(stderr, "**ERROR**: Marker %d is failed to acquire "
					"locker (mut), %s.\n", markerID, strerror(errno));
			abort();
		}

		/* Wait to be grabbed by a student or timeout */
		while ((markers[markerID].is_grabbed == 0) && (timeout == 0)) {

			err = pthread_cond_wait(&c_marker, &mut);
			if (err != 0) {
				fprintf(stderr, "**ERROR**: Marker %d is failed to "
						"wait conditional variable (c_marker, mut), "
						"%s.\n", studentID, strerror(errno));
				abort();
			}
		}

		if (timeout == 1) {
			
			/* 
			 * 3. If the end of the session approaches (i.e. there is no time
			 *		to start another demo) then the marker waits for the current
			 *		demo to finish (if they are currently attending one) and then
			 *		exits the lab.
			 */

			/* 
			 * Marker exit lab since timeout
			 * Unlock the locker before exit lab
			 */

			err = pthread_mutex_unlock(&mut);
			if (err != 0) {
				fprintf(stderr, "**ERROR**: Marker %d is failed to release"
						" locker (mut), %s.\n", studentID, strerror(errno));
				abort();
			}

			printf("%d marker %d: exits lab (timeout)\n", timenow(), markerID);
			return NULL;

		} else {

			/* Get the studentID which grabed the current marker */
			studentID = markers[markerID].studentID;
			if (studentID < 0 || studentID >= parameters.S) {
				fprintf(stderr, "**ERROR**: Marker %d grabbed by student %d is illegal.\n", markerID, studentID);
				abort();
			}

			err = pthread_mutex_unlock(&mut);
			if (err != 0) {
				fprintf(stderr, "**ERROR**: Marker %d is failed to "
						"release locker (mut), %s.\n", studentID, strerror(errno));
				abort();
			}
		}

		/* The following line shall be printed when a marker is grabbed by a student. */
		printf("%d marker %d: grabbed by student %d (job %d)\n", timenow(), markerID, studentID, job + 1);

		demo();

		/* The following line shall be printed when a marker has finished attending a demo. */
		printf("%d marker %d: finished with student %d (job %d)\n", timenow(), markerID, studentID, job + 1);


		/* Wait for the demo to finish */
		err = pthread_mutex_lock(&mut);
		if (err != 0) {
			fprintf(stderr, "**ERROR**: Marker %d is failed to acquire "
					"locker (mut), %s.\n", markerID, strerror(errno));
			abort();
		}

		while (markers[markerID].is_grabbed == 1) {

			err = pthread_cond_wait(&c_marker, &mut);
			if (err != 0) {
				fprintf(stderr, "**ERROR**: Marker %d is failed to "
						"wait conditional variable (c_student, mut), "
						"%s.\n", studentID, strerror(errno));
				abort();
			}
		}

		job = job + 1;

		if (job < parameters.N) {

			err = release(markerID, job);
			if (err != 0) {
				fprintf(stderr, "**ERROR**: Marker %d is "
						"failed to release.\n", markerID);
				abort();
			}

			err = pthread_cond_broadcast(&c_student);
			if (err != 0) {
				fprintf(stderr, "**ERROR**: Marker %d is failed to "
						"broadcast student (c_student), "
						"%s.\n", markerID, strerror(errno));
				abort();
			}
		}

		err = pthread_mutex_unlock(&mut);
		if (err != 0) {
			fprintf(stderr, "**ERROR**: Marker %d is failed to release "
					"locker (mut), %s.\n", studentID, strerror(errno));
			abort();
		}
	}

	/* 
	 * When the marker exits the lab, exactly one of the following two lines shall be
	 * printed, depending on whether the marker has finished all their jobs or there
	 * is no time to complete another demo.
	 */

	printf("%d marker %d: exits lab (finished %d jobs)\n", timenow(), markerID, parameters.N);

	return NULL;
}


/*
 * A student thread. You must modify this function.
 */
void *student(void *arg) {
	/* The ID of the current student. */
	int studentID = *(int *)arg;

	/* The variable for storing error code */
	int err;

	/* Store the markerID which grabbed by the current student */
	int* markerIDs;

	markerIDs = (int *)malloc(parameters.K * sizeof(int));
	if (markerIDs == NULL) {
		fprintf(stderr, "**ERROR**: Student %d is failed to allocate "
				"memory for the pointer of markerIDs.\n", studentID);
		abort();
	}

	/* 1. Panic! */
	printf("%d student %d: starts panicking\n", timenow(), studentID);
	panic();

	/* 2. Enter the lab. */
	printf("%d student %d: enters lab\n", timenow(), studentID);

	/* 3. Grab K markers. */
	err = pthread_mutex_lock(&mut);
	if (err != 0) { 
		fprintf(stderr, "**ERROR**: Student %d is failed to acquire "
				"locker (mut), %s.\n", studentID, strerror(errno));
		abort();
	}

	/* Waiting for K idle markers or timeout */
	while ((idle_marker < parameters.K) && (timeout == 0)) {

		err = pthread_cond_wait(&c_student, &mut);
		if (err != 0) {
			fprintf(stderr, "**ERROR**: Student %d is failed to "
					"wait conditional variable (c_student, mut), "
					"%s.\n", studentID, strerror(errno));
			abort();
		}
	}

	if (timeout == 0) {

		/* Student grabs K markers */
		err = grab(studentID, markerIDs);
		if (err != 0) {
			fprintf(stderr, "**ERROR**: Student %d is failed to "
					"grab %d markers.\n", studentID, parameters.K);
			abort();
		}

		/* Student broadcast markers after grab */
		err = pthread_cond_broadcast(&c_marker);
		if (err != 0) { 
			fprintf(stderr, "**ERROR**: Student %d is failed to "
					"broadcast markers (c_marker), %s.\n",
					studentID, strerror(errno));
			abort();
		}

		err = pthread_mutex_unlock(&mut);
		if (err != 0) {
			fprintf(stderr, "**ERROR**: Student %d is failed to release"
					" locker (mut), %s.\n", studentID, strerror(errno));
			abort();
		}

	} else {

		/* 
		 * Student exit the lab since timeout
		 * Unlock the locker (mut) before exit the lab
		 */
		err = pthread_mutex_unlock(&mut);
		if (err != 0) {
			fprintf(stderr, "**ERROR**: Student %d is failed to release"
					" locker (mut), %s.\n", studentID, strerror(errno));
			abort();
		}

		printf("%d student %d: exits lab (timeout)\n", timenow(), studentID);

		return NULL;
	}


	/* 4. Demo! */
	/*
	 * If the student succeeds in grabbing K markers and there is enough time left
	 * for a demo, the following three lines shall be executed in order.
	 * If the student has not started their demo and there is not sufficient time
	 * left to do a full demo, the following three lines shall not be executed
	 * and the student proceeds to step 5.
	 */
	printf("%d student %d: starts demo\n", timenow(), studentID);
	demo();
	printf("%d student %d: ends demo\n", timenow(), studentID);


	/* Release the markers that grabbed the current student */
	err = pthread_mutex_lock(&mut);
	if (err != 0) { 
		fprintf(stderr, "**ERROR**: Student %d is failed to acquire "
				"locker (mut), %s.\n", studentID, strerror(errno));
		abort();
	}

	for (int i = 0; i < parameters.K; i++) {
		int markerID = markerIDs[i];
		if (markerID < 0 || markerID >= parameters.M) {
			fprintf(stderr, "**ERROR**: Student %d grabbed marker %d is"
					" illegal.\n", studentID, markerID);
			abort();
		}
		markers[markerID].is_grabbed = 0;
	}

	/* Student broadcast markers after demo */
	err = pthread_cond_broadcast(&c_marker);
	if (err != 0) { 
		fprintf(stderr, "**ERROR**: Student %d is failed to "
				"broadcast markers (c_marker), %s.\n",
				studentID, strerror(errno));
		abort();
	}

	err = pthread_mutex_unlock(&mut);
	if (err != 0) {
		fprintf(stderr, "**ERROR**: Student %d is failed to release"
				" locker (mut), %s.\n", studentID, strerror(errno));
		abort();
	}


	/* 5. Exit the lab. */
	/* 
	 * Exactly one of the following two lines shall be printed, depending on
	 * whether the student got to give their demo or not.
	 */
	printf("%d student %d: exits lab (finished)\n", timenow(), studentID);

	free(markerIDs);

	return NULL;
}


/* The function that runs the session.
 * You MAY want to modify this function.
 */
void run() {
	int i;
	int ok;
	int markerID[100], studentID[100];
	pthread_t markerT[100], studentT[100];

	printf("S=%d M=%d K=%d N=%d T=%d D=%d\n",
			parameters.S,
			parameters.M,
			parameters.K,
			parameters.N,
			parameters.T,
			parameters.D);
	gettimeofday(&starttime, NULL);  /* Save start of simulated time */

	/* Initalise the global variable */
	initialise();

	/* Create S student threads */
	for (i = 0; i < parameters.S; i++) {
		studentID[i] = i;
		ok = pthread_create(&studentT[i], NULL, student, &studentID[i]);
		if (ok != 0) { abort(); }
	}

	/* Create M marker threads */
	for (i = 0; i < parameters.M; i++) {
		markerID[i] = i;
		ok = pthread_create(&markerT[i], NULL, marker, &markerID[i]);
		if (ok != 0) { abort(); }
	}

	/* With the threads now started, the session is in full swing ... */
	delay(parameters.T - parameters.D);

	/* 
	 * When we reach here, this is the latest time a new demo could start.
	 * You might want to do something here or soon after.
	 */

	/* Main thread sets timeout and sends signal to students and markers. */
	ok = pthread_mutex_lock(&mut);
	if (ok != 0) {
		fprintf(stderr, "**ERROR**: Main thread is failed to "
				"acquire the locker (mut), %s.\n", strerror(errno));
		abort();
	}

	timeout = 1;

	ok = pthread_cond_broadcast(&c_student);
	if (ok != 0) {
		fprintf(stderr, "**ERROR**: Main thread is failed to "
				"broadcast students (c_student), %s.\n", strerror(errno));
		abort();
	}

	ok = pthread_cond_broadcast(&c_marker);
	if (ok != 0) {
		fprintf(stderr, "**ERROR**: Main thread is failed to "
				"broadcast markers (c_marker), %s.\n", strerror(errno));
		abort();
	}

	ok = pthread_mutex_unlock(&mut);
	if (ok != 0) {
		fprintf(stderr, "**ERROR**: Main thread is failed to "
				"release the locker (mut), %s.\n", strerror(errno));
		abort();
	}

	/* Wait for student threads to finish */
	for (i = 0; i<parameters.S; i++) {
		ok = pthread_join(studentT[i], NULL);
		if (ok != 0) { abort(); }
	}

	/* Wait for marker threads to finish */
	for (i = 0; i<parameters.M; i++) {
		ok = pthread_join(markerT[i], NULL);
		if (ok != 0) { abort(); }
	}

	/* Deallocates the memory */
	free(markers);
}


/*
 * main() checks that the parameters are ok. If they are, the interesting bit
 * is in run() so please don't modify main().
 */
int main(int argc, char *argv[]) {

	if (argc < 6) {
		puts("Usage: demo S M K N T D\n");
		exit(1);
	}

	parameters.S = atoi(argv[1]);
	parameters.M = atoi(argv[2]);
	parameters.K = atoi(argv[3]);
	parameters.N = atoi(argv[4]);
	parameters.T = atoi(argv[5]);
	parameters.D = atoi(argv[6]);

	if (parameters.M > 100 || parameters.S > 100) {
		puts("Maximum 100 markers and 100 students allowed.\n");
		exit(1);
	}

	if (parameters.D >= parameters.T) {
		puts("Constraint D < T violated.\n");
		exit(1);
	}

	if (parameters.S*parameters.K > parameters.M*parameters.N) {
		puts("Constraint S*K <= M*N violated.\n");
		exit(1);
	}

	srand(time(NULL));

	// We're good to go.

	run();

	return 0;
}
