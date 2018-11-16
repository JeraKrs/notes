//
// Implementation of the iterative Jacobi method.
//
// Given a known, diagonally dominant matrix A and a known vector b, we aim to
// to find the vector x that satisfies the following equation:
//
//     Ax = b
//
// We first split the matrix A into the diagonal D and the remainder R:
//
//     (D + R)x = b
//
// We then rearrange to form an iterative solution:
//
//     x' = (b - Rx) / D
//
// More information:
// -> https://en.wikipedia.org/wiki/Jacobi_method
//

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <x86intrin.h>
#include <immintrin.h>
#include "mpi.h"


static int N;
static int MAX_ITERATIONS;
static int SEED;
static double CONVERGENCE_THRESHOLD;
#define MASTER 0

#define SEPARATOR "------------------------------------\n"

// Return the current time in seconds since the Epoch
double get_timestamp();

// Parse command line arguments to set solver parameters
void parse_arguments(int argc, char *argv[]);

// Run the Jacobi solver
// Returns the number of iterations performed
int run(int argc, char *argv[], float* A, float* b, float* x, float* xtmp) {

	int itr;
	float diff;
	float sqdiff;
	float *ptrtmp;

	float convergence_threshold_square = CONVERGENCE_THRESHOLD * CONVERGENCE_THRESHOLD;

	float* d = malloc(N*sizeof(float));
	for (int row = 0; row < N; row++) d[row] = 1 / A[row * N + row];

	int row, ridx, col;

	int rank;           /* process rank */
	int nprocs;         /* number of processes */
	int source;         /* rank of sender */
	int dest = MASTER;  /* all procs send to master */
	int tag = 0;        /* message tag */
//	MPI_Request request;   /* request struct used in non-blocking comms calls */      
	MPI_Status status;  /* struct to hold message status */
	int width, start, end;
	float dot;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	// Loop until converged or maximum iterations reached
	itr = 0;

	do {
		width = N / nprocs;
		start = rank * width;
		end = start + width;

		// Perfom Jacobi iteration
		for (row = start; row < end; row++) {
			ridx = row * N;
			dot = 0.0;

			for (col = 0; col < row; col++) dot += A[ridx + col] * x[col];
			for (col = row + 1; col < N; col++) dot += A[ridx + col] * x[col];
			xtmp[row] = (b[row] - dot) * d[row];
		}

		if (rank == MASTER) {
			for (source = 1; source < nprocs; source++)
				MPI_Recv(xtmp + source * width, width, MPI_DOUBLE, source, tag, MPI_COMM_WORLD, &status);
			for (source = 1; source < nprocs; source++)
				MPI_Send(xtmp, N, MPI_DOUBLE, source, tag, MPI_COMM_WORLD);
		} else {
			MPI_Send(xtmp + rank * width, width, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
			MPI_Recv(xtmp, N, MPI_DOUBLE, dest, tag, MPI_COMM_WORLD, &status);
		}

		// Swap pointers
		ptrtmp = x;
		x      = xtmp;
		xtmp   = ptrtmp; 
		// Check for convergence
		sqdiff = 0.0;
		for (row = 0; row < N; row++) {
			diff    = xtmp[row] - x[row];
			sqdiff += diff * diff;
		}

		itr++;
	} while ((itr < MAX_ITERATIONS) && (sqdiff > convergence_threshold_square));


	return itr;
}

int main(int argc, char *argv[]) {

	parse_arguments(argc, argv);

	float *A    = malloc(N*N*sizeof(float));
	float *b    = malloc(N*sizeof(float));
	float *x    = malloc(N*sizeof(float));
	float *xtmp = malloc(N*sizeof(float));

	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == MASTER) {
		printf(SEPARATOR);
		printf("Matrix size:            %dx%d\n", N, N);
		printf("Maximum iterations:     %d\n", MAX_ITERATIONS);
		printf("Convergence threshold:  %lf\n", CONVERGENCE_THRESHOLD);
		printf(SEPARATOR);
	}

	double total_start = get_timestamp();

	// Initialize data
	srand(SEED);
	int row, col;
	for (row = 0; row < N; row++) {
		int ridx = row * N;
		float rowsum = 0.0;
		for (col = 0; col < N; col++) {
			float value = rand()/(float)RAND_MAX;
			A[ridx + col] = value;
			rowsum += value;
		}
		A[ridx + row] += rowsum;
		b[row] = rand()/(float)RAND_MAX;
		x[row] = 0.0;
	}

	// Run Jacobi solver
	double solve_start = get_timestamp();
	int itr = run(argc, argv, A, b, x, xtmp);
	double solve_end = get_timestamp();

	// Check error of final solution
	double err = 0.0;
	for (int row = 0, ridx = 0; row < N; row++, ridx += N) {
		double tmp = 0.0;
		for (int col = 0; col < N; col++) {
			tmp += A[ridx + col] * x[col];
		}
		tmp = b[row] - tmp;
		err += tmp*tmp;
	}
	err = sqrt(err);

	double total_end = get_timestamp();

	if (rank == MASTER) {
		printf("Solution error = %lf\n", err);
		printf("Iterations     = %d\n", itr);
		printf("Total runtime  = %lf seconds\n", (total_end-total_start));
		printf("Solver runtime = %lf seconds\n", (solve_end-solve_start));
		if (itr == MAX_ITERATIONS)
			printf("WARNING: solution did not converge\n");
		printf(SEPARATOR);
	}

	MPI_Finalize();

	free(A);
	free(b);
	free(x);
	free(xtmp);

	return 0;
}

double get_timestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec*1e-6;
}

int parse_int(const char *str) {
	char *next;
	int value = strtoul(str, &next, 10);
	return strlen(next) ? -1 : value;
}

double parse_double(const char *str) {
	char *next;
	double value = strtod(str, &next);
	return strlen(next) ? -1 : value;
}

void parse_arguments(int argc, char *argv[]) {
	// Set default values
	N = 1000;
	MAX_ITERATIONS = 20000;
	CONVERGENCE_THRESHOLD = 0.0001;
	SEED = 0;

	for (int i = 1; i < argc; i++) {

		if (!strcmp(argv[i], "--convergence") || !strcmp(argv[i], "-c")) {

			if (++i >= argc || (CONVERGENCE_THRESHOLD = parse_double(argv[i])) < 0) {
				printf("Invalid convergence threshold\n");
				exit(1);
			}
		} else if (!strcmp(argv[i], "--iterations") || !strcmp(argv[i], "-i")) {

			if (++i >= argc || (MAX_ITERATIONS = parse_int(argv[i])) < 0) {
				printf("Invalid number of iterations\n");
				exit(1);
			}
		} else if (!strcmp(argv[i], "--norder") || !strcmp(argv[i], "-n")) {

			if (++i >= argc || (N = parse_int(argv[i])) < 0) {
				printf("Invalid matrix order\n");
				exit(1);
			}
		} else if (!strcmp(argv[i], "--seed") || !strcmp(argv[i], "-s")) {

			if (++i >= argc || (SEED = parse_int(argv[i])) < 0) {
				printf("Invalid seed\n");
				exit(1);
			}
		} else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			printf("\n");
			printf("Usage: ./jacobi [OPTIONS]\n\n");
			printf("Options:\n");
			printf("  -h  --help               Print this message\n");
			printf("  -c  --convergence  C     Set convergence threshold\n");
			printf("  -i  --iterations   I     Set maximum number of iterations\n");
			printf("  -n  --norder       N     Set maxtrix order\n");
			printf("  -s  --seed         S     Set random number seed\n");
			printf("\n");
			exit(0);
		} else {
			printf("Unrecognized argument '%s' (try '--help')\n", argv[i]);
			exit(1);
		}
	}
}
