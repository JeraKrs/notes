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

const double EPS = 1e-9;
static int N;
static int MAX_ITERATIONS;
static int SEED;
static double CONVERGENCE_THRESHOLD;

#define SEPARATOR "------------------------------------\n"

// Return the current time in seconds since the Epoch
double get_timestamp();
void sswap(double& x, double& y) { double t = x; x = y; y = t; }

// Parse command line arguments to set solver parameters
void parse_arguments(int argc, char *argv[]);

void gauss_elimination (double* a, int n) {
	int row = n + 1;

	for (int i = 0; i < n; i++) {
		int r = i;

		for (int j = i + 1; j < n; j++)
			if (fabs(a[j * row + i]) > fabs(a[r * row + i]))
				r = j;

		if (r != i) {
			for (int j = 0; j <= n; j++)
				sswap(a[r * row + j], a[i * row + j]);
		}

		if (fabs(a[i * row + i]) < 1e-9)
			continue;

		for (int k = i + 1; k < n; k++) {
			double f = a[k * row + i] / a[i * row + i];
			for (int j = 0; j <= n; j++)
				a[k * row + j] -= a[i * row + j] * f;
		}
	}

	for (int i = n-1; i >= 0; i--) {
		for (int j = i+1; j < n; j++)
			a[i * row + n] -= a[j * row + j] * a[i * row + j];
		a[i * row + i] = a[i * row + n] / a[i * row + i];
		if (fabs(a[i * row + i]) < 1e-9)
			a[i * row + i] = 0;
	}
}

// Run the Jacobi solver
// Returns the number of iterations performed
int run(double *A, double *b, double *x) {

	double *d = (double *)malloc(N*(N+1)*sizeof(double));
	for (int row = 0; row < N; row++) {
		for (int col = 0; col < N; col++)
			d[row * N + row + col] = A[row * N + col];
		d[row * (N + 1) + N] = b[row];
	}

	gauss_elimination(d, N);

	for (int row = 0; row < N; row++)
		x[row] = d[row * (N + 1) + row];

	return 0;
}

int main(int argc, char *argv[]) {
	parse_arguments(argc, argv);

	double *A    = (double *)malloc(N*N*sizeof(double));
	double *b    = (double *)malloc(N*sizeof(double));
	double *x    = (double *)malloc(N*sizeof(double));

	printf(SEPARATOR);
	printf("Matrix size:            %dx%d\n", N, N);
	printf("Maximum iterations:     %d\n", MAX_ITERATIONS);
	printf("Convergence threshold:  %lf\n", CONVERGENCE_THRESHOLD);
	printf(SEPARATOR);

	double total_start = get_timestamp();

	// Initialize data
	srand(SEED);
	for (int row = 0; row < N; row++) {
		double rowsum = 0.0;
		for (int col = 0; col < N; col++) {
			double value = rand() / (double)RAND_MAX;
			A[row * N + col] = value;
			rowsum += value;
		}
		A[row * N + row] += rowsum;
		b[row] = rand()/(double)RAND_MAX;
		x[row] = 0.0;
	}

	// Run Jacobi solver
	double solve_start = get_timestamp();
	int itr = run(A, b, x);
	double solve_end = get_timestamp();

	// Check error of final solution
	double err = 0.0;
	for (int row = 0; row < N; row++) {
		double tmp = 0.0;
		for (int col = 0; col < N; col++) {
			tmp += A[row * N + col] * x[col];
		}
		tmp = b[row] - tmp;
		err += tmp*tmp;
	}
	err = sqrt(err);

	double total_end = get_timestamp();

	printf("Solution error = %lf\n", err);
	printf("Iterations     = %d\n", itr);
	printf("Total runtime  = %lf seconds\n", (total_end-total_start));
	printf("Solver runtime = %lf seconds\n", (solve_end-solve_start));
	if (itr == MAX_ITERATIONS)
		printf("WARNING: solution did not converge\n");
	printf(SEPARATOR);

	free(A);
	free(b);
	free(x);

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
