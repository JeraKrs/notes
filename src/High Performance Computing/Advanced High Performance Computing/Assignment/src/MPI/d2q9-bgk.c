/*
 ** Code to implement a d2q9-bgk lattice boltzmann scheme.
 ** 'd2' inidates a 2-dimensional grid, and
 ** 'q9' indicates 9 velocities per grid cell.
 ** 'bgk' refers to the Bhatnagar-Gross-Krook collision step.
 **
 ** The ' in each cell are numbered as follows:
 **
 ** 6 2 5
 **  \|/
 ** 3-0-1
 **  /|\
 ** 7 4 8
 **
 ** A 2D grid:
 **
 **           cols
 **       --- --- ---
 **      | D | E | F |
 ** rows  --- --- ---
 **      | A | B | C |
 **       --- --- ---
 **
 ** 'unwrapped' in row major order to give a 1D array:
 **
 **  --- --- --- --- --- ---
 ** | A | B | C | D | E | F |
 **  --- --- --- --- --- ---
 **
 ** Grid indicies are:
 **
 **          ny
 **          ^       cols(ii)
 **          |  ----- ----- -----
 **          | | ... | ... | etc |
 **          |  ----- ----- -----
 ** rows(jj) | | 1,0 | 1,1 | 1,2 |
 **          |  ----- ----- -----
 **          | | 0,0 | 0,1 | 0,2 |
 **          |  ----- ----- -----
 **          ----------------------> nx
 **
 ** Note the names of the input parameter and obstacle files
 ** are passed on the command line, e.g.:
 **
 **   ./d2q9-bgk input.params obstacles.dat
 **
 ** Be sure to adjust the grid dimensions in the parameter file
 ** if you choose a different obstacle file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "mpi.h"

#define NSPEEDS         9
#define FINALSTATEFILE  "final_state.dat"
#define AVVELSFILE      "av_vels.dat"
#define MASTER			0

/* struct to hold the parameter values */
typedef struct
{
	int    nx;            /* no. of cells in x-direction */
	int    ny;            /* no. of cells in y-direction */
	int    maxIters;      /* no. of iterations */
	int    reynolds_dim;  /* dimension for Reynolds number */
	float density;       /* density per link */
	float accel;         /* density redistribution */
	float omega;         /* relaxation parameter */
	int   start;
	int   stop;
} t_param;

/* struct to hold the 'speed' values */
typedef float t_speed[NSPEEDS];

/*
 ** function prototypes
 */

/* load params, allocate memory, load obstacles & initialise fluid particle densities */
int initialise(const char* paramfile, const char* obstaclefile,
		t_param* params, t_speed** cells_ptr, t_speed** tmp_cells_ptr,
		int** obstacles_ptr, float** av_vels_ptr);

/*
 ** The main calculation methods.
 ** timestep calls, in order, the functions:
 ** accelerate_flow(), propagate(), rebound() & collision()
 */
float timestep(const t_param params, t_speed* cells, t_speed* tmp_cells, int* obstacles);
int accelerate_flow(const t_param params, t_speed* cells, int* obstacles);
float move(const t_param params, t_speed* cells, t_speed* tmp_cells, int* obstacles);
int write_values(const t_param params, t_speed* cells, int* obstacles, float* av_vels);

/* finalise, including freeing up allocated memory */
int finalise(const t_param* params, t_speed** cells_ptr, t_speed** tmp_cells_ptr,
		int** obstacles_ptr, float** av_vels_ptr);

/* compute average velocity */
float av_velocity(const t_param params, t_speed* cells, int* obstacles);

/* Sum all the densities in the grid.
 ** The total should remain constant from one timestep to the next. */
float total_density(const t_param params, t_speed* cells);

/* calculate Reynolds number */
float calc_reynolds(const t_param params, t_speed* cells, int* obstacles);

/* utility functions */
void die(const char* message, const int line, const char* file);
void usage(const char* exe);

void distribute(int s, int n, int r, int* start, int* stop)
{
	int k = s % n;
	if (r < k) {
		*start = r * (s / n + 1);
		*stop = (r + 1) * (s / n + 1);
	} else {
		int bias = k * (s / n + 1);
		r = r - k;
		*start = bias + r * (s / n);
		*stop = bias + (r + 1) * (s / n);
	}
}

/*
 ** main program:
 ** initialise, timestep loop, finalise
 */
int main(int argc, char* argv[])
{
	char*    paramfile = NULL;    /* name of the input parameter file */
	char*    obstaclefile = NULL; /* name of a the input obstacle file */
	t_param  params;              /* struct to hold parameter values */
	t_speed* cells     = NULL;    /* grid containing fluid densities */
	t_speed* tmp_cells = NULL;    /* scratch space */
	int*     obstacles = NULL;    /* grid indicating which cells are blocked */
	float* av_vels   = NULL;     /* a record of the av. velocity computed for each timestep */
	struct timeval timstr;        /* structure to hold elapsed time */
	struct rusage ru;             /* structure to hold CPU time--system and user */
	double tic, toc;              /* floating point numbers to calculate elapsed wallclock time */
	double usrtim;                /* floating point number to record elapsed user CPU time */
	double systim;                /* floating point number to record elapsed system CPU time */

	/* parse the command line */
	if (argc != 3) {
		usage(argv[0]);
	} else {
		paramfile = argv[1];
		obstaclefile = argv[2];
	}

	/* initialise our data structures and load values from file */
	initialise(paramfile, obstaclefile, &params, &cells, &tmp_cells, &obstacles, &av_vels);

	int total_cell = 0;
	for (int jj = 0; jj < params.ny; jj++) {
		for (int ii = 0; ii < params.nx; ii++)
			if (!obstacles[ii + jj * params.nx])
				total_cell++;
	} 

	MPI_Init(&argc, &argv);

	/* Set the task */
	int rank;
	int nprocs;
	MPI_Status status;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	distribute(params.ny, nprocs, rank, &params.start, &params.stop);
	
	int l_nb = (rank - 1 + nprocs) % nprocs;
	int l_bf = (params.start - 1 + params.ny) % params.ny;
	int r_nb = (rank + 1) % nprocs;
	int r_bf = params.stop % params.ny;

	/* iterate for maxIters timesteps */
	gettimeofday(&timstr, NULL);
	tic = timstr.tv_sec + (timstr.tv_usec / 1000000.0);

	for (int tt = 0; tt < params.maxIters; tt++)
	{
		float local = timestep(params, cells, tmp_cells, obstacles);
		float recv_value;

		/* Halo exchange */
		if (nprocs != 1) {
			MPI_Sendrecv(cells+(params.stop-1)*params.nx,
					params.nx*NSPEEDS, MPI_FLOAT, r_nb, 0,
					cells+l_bf*params.nx, params.nx*NSPEEDS,
					MPI_FLOAT, l_nb, 0, MPI_COMM_WORLD, &status);

			MPI_Sendrecv(cells+params.start*params.nx,
					params.nx*NSPEEDS, MPI_FLOAT, l_nb, 0,
					cells+r_bf*params.nx, params.nx*NSPEEDS,
					MPI_FLOAT, r_nb, 0, MPI_COMM_WORLD, &status);
		}

		MPI_Reduce(&local, &recv_value, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
		av_vels[tt] = recv_value / (float)total_cell;
		/*
		if (rank == MASTER) {
			for (int rr = 1; rr < nprocs; rr++) {
				MPI_Recv(&recv_value, 1, MPI_FLOAT, rr, 0, MPI_COMM_WORLD, &status);
				local += recv_value;
			}
		} else {
			MPI_Send(&local, 1, MPI_FLOAT, MASTER, 0, MPI_COMM_WORLD);
		}
		*/
	}

	if (rank != MASTER) {
		MPI_Send(cells+params.start*params.nx,
				params.nx*NSPEEDS*(params.stop-params.start),
				MPI_FLOAT, MASTER, 0, MPI_COMM_WORLD);
		finalise(&params, &cells, &tmp_cells, &obstacles, &av_vels);
		MPI_Finalize();
		return EXIT_SUCCESS;
	}

	gettimeofday(&timstr, NULL);
	toc = timstr.tv_sec + (timstr.tv_usec / 1000000.0);
	getrusage(RUSAGE_SELF, &ru);
	timstr = ru.ru_utime;
	usrtim = timstr.tv_sec + (timstr.tv_usec / 1000000.0);
	timstr = ru.ru_stime;
	systim = timstr.tv_sec + (timstr.tv_usec / 1000000.0);

	/* MASTER */
	for (int rr = 1; rr < nprocs; rr++) {
		int start, stop;
		distribute(params.ny, nprocs, rr, &start, &stop);
		MPI_Recv(cells+params.nx*start,
				params.nx*NSPEEDS*(stop-start),
				MPI_FLOAT, rr, 0, MPI_COMM_WORLD, &status);
	}
	params.start = 0;
	params.stop = params.ny;

	/* write final values and free memory */
	printf("==done==\n");
	printf("Reynolds number:\t\t%.12E\n", av_vels[params.maxIters - 1] * calc_reynolds(params, cells, obstacles));
	printf("Elapsed time:\t\t\t%.6lf (s)\n", toc - tic);
	printf("Elapsed user CPU time:\t\t%.6lf (s)\n", usrtim);
	printf("Elapsed system CPU time:\t%.6lf (s)\n", systim);

	write_values(params, cells, obstacles, av_vels);
	finalise(&params, &cells, &tmp_cells, &obstacles, &av_vels);
	MPI_Finalize();

	return EXIT_SUCCESS;
}

float timestep(const t_param params, t_speed* cells, t_speed* tmp_cells, int* obstacles)
{
	accelerate_flow(params, cells, obstacles);
	return move(params, cells, tmp_cells, obstacles);
}

int accelerate_flow(const t_param params, t_speed* cells, int* obstacles)
{
	/* compute weighting factors */
	float w1 = params.density * params.accel / 9.f;
	float w2 = params.density * params.accel / 36.f;

	/* modify the 2nd row of the grid */
	int jj = params.ny - 2;
	int row = jj*params.nx;

	if (jj < params.start - 1 || jj > params.stop)
		return EXIT_SUCCESS;

	for (int ii = 0; ii < params.nx; ii++)
	{
		int col = row + ii;
		/* if the cell is not occupied and
		 ** we don't send a negative density */
		if (!obstacles[col]
				&& (cells[col][3] - w1) > 0.f
				&& (cells[col][6] - w2) > 0.f
				&& (cells[col][7] - w2) > 0.f)
		{
			/* increase 'east-side' densities */
			cells[col][1] += w1;
			cells[col][5] += w2;
			cells[col][8] += w2;
			/* decrease 'west-side' densities */
			cells[col][3] -= w1;
			cells[col][6] -= w2;
			cells[col][7] -= w2;
		}
	}

	return EXIT_SUCCESS;
}

float move(const t_param params, t_speed* cells, t_speed* tmp_cells, int* obstacles)
{
	/* loop over _all_ cells */
	for (int jj = params.start; jj < params.stop; jj++)
	{
		int row = jj*params.nx;
		for (int ii = 0; ii < params.nx; ii++)
		{
			int col = row + ii;

			/* determine indices of axis-direction neighbours
			 ** respecting periodic boundary conditions (wrap around) */
			int y_n = (jj + 1) % params.ny;
			int x_e = (ii + 1) % params.nx;
			int y_s = (jj == 0) ? (jj + params.ny - 1) : (jj - 1);
			int x_w = (ii == 0) ? (ii + params.nx - 1) : (ii - 1);
			/* propagate densities from neighbouring cells, following
			 ** appropriate directions of travel and writing into
			 ** scratch space grid */
			tmp_cells[col][0] = cells[ii + jj*params.nx][0]; /* central cell, no movement */
			tmp_cells[col][1] = cells[x_w + jj*params.nx][1]; /* east */
			tmp_cells[col][2] = cells[ii + y_s*params.nx][2]; /* north */
			tmp_cells[col][3] = cells[x_e + jj*params.nx][3]; /* west */
			tmp_cells[col][4] = cells[ii + y_n*params.nx][4]; /* south */
			tmp_cells[col][5] = cells[x_w + y_s*params.nx][5]; /* north-east */
			tmp_cells[col][6] = cells[x_e + y_s*params.nx][6]; /* north-west */
			tmp_cells[col][7] = cells[x_e + y_n*params.nx][7]; /* south-west */
			tmp_cells[col][8] = cells[x_w + y_n*params.nx][8]; /* south-east */
		}
	}

	const float d_sq = 3.f;
	const float dh_sq = 1.5f;
	const float dh2_sq = 4.5f;
	const float w0 = 4.f / 9.f;  /* weighting factor */
	const float w1 = 1.f / 9.f;  /* weighting factor */
	const float w2 = 1.f / 36.f; /* weighting factor */

	float tot_u = 0.f;          /* accumulated magnitudes of velocity for each cell */

	for (int jj = params.start; jj < params.stop; jj++)
	{
		int row = jj*params.nx;
		for (int ii = 0; ii < params.nx; ii++)
		{

			int col = row + ii;

			/* if the cell contains an obstacle */
			if (obstacles[col])
			{
				/* called after propagate, so taking values from scratch space
				 ** mirroring, and writing into main grid */
				cells[col][1] = tmp_cells[col][3];
				cells[col][2] = tmp_cells[col][4];
				cells[col][3] = tmp_cells[col][1];
				cells[col][4] = tmp_cells[col][2];
				cells[col][5] = tmp_cells[col][7];
				cells[col][6] = tmp_cells[col][8];
				cells[col][7] = tmp_cells[col][5];
				cells[col][8] = tmp_cells[col][6];
			}
			/* don't consider occupied cells */
			else
			{
				/* compute local density total */
				float local_density = 0.f;

				for (int kk = 0; kk < NSPEEDS; kk++)
				{
					local_density += tmp_cells[col][kk];
				}

				/* compute x velocity component */
				float u_x = (tmp_cells[col][1]
						+ tmp_cells[col][5]
						+ tmp_cells[col][8]
						- (tmp_cells[col][3]
							+ tmp_cells[col][6]
							+ tmp_cells[col][7]))
					/ local_density;
				/* compute y velocity component */
				float u_y = (tmp_cells[col][2]
						+ tmp_cells[col][5]
						+ tmp_cells[col][6]
						- (tmp_cells[col][4]
							+ tmp_cells[col][7]
							+ tmp_cells[col][8]))
					/ local_density;

				/* velocity squared */
				float u_sq = u_x * u_x + u_y * u_y;

				/* directional velocity components */
				float u[NSPEEDS];
				u[1] =   u_x;        /* east */
				u[2] =         u_y;  /* north */
				u[3] = - u_x;        /* west */
				u[4] =       - u_y;  /* south */
				u[5] =   u_x + u_y;  /* north-east */
				u[6] = - u_x + u_y;  /* north-west */
				u[7] = - u_x - u_y;  /* south-west */
				u[8] =   u_x - u_y;  /* south-east */

				/* equilibrium densities */
				float d_equ[NSPEEDS];
				/* zero velocity density: weight w0 */
				d_equ[0] = w0 * local_density * (1.f - u_sq * dh_sq);
				/* axis: weight w1 */
				d_equ[1] = w1 * local_density * (1.f + u[1] * d_sq
						+ (u[1] * u[1]) * dh2_sq - u_sq * dh_sq);
				d_equ[2] = w1 * local_density * (1.f + u[2] * d_sq
						+ (u[2] * u[2]) * dh2_sq - u_sq * dh_sq);
				d_equ[3] = w1 * local_density * (1.f + u[3] * d_sq
						+ (u[3] * u[3]) * dh2_sq - u_sq * dh_sq);
				d_equ[4] = w1 * local_density * (1.f + u[4] * d_sq
						+ (u[4] * u[4]) * dh2_sq - u_sq * dh_sq);
				/* diagonal: weight w2 */
				d_equ[5] = w2 * local_density * (1.f + u[5] * d_sq
						+ (u[5] * u[5]) * dh2_sq - u_sq * dh_sq);
				d_equ[6] = w2 * local_density * (1.f + u[6] * d_sq
						+ (u[6] * u[6]) * dh2_sq - u_sq * dh_sq);
				d_equ[7] = w2 * local_density * (1.f + u[7] * d_sq
						+ (u[7] * u[7]) * dh2_sq - u_sq * dh_sq);
				d_equ[8] = w2 * local_density * (1.f + u[8] * d_sq
						+ (u[8] * u[8]) * dh2_sq - u_sq * dh_sq);

				local_density = 0.f;

				/* relaxation step */
				for (int kk = 0; kk < NSPEEDS; kk++)
				{
					cells[col][kk] = tmp_cells[col][kk]
						+ params.omega
						* (d_equ[kk] - tmp_cells[col][kk]);
					local_density += cells[col][kk];
				}

				u_x = (cells[col][1]
						+ cells[col][5]
						+ cells[col][8]
						- (cells[col][3]
							+ cells[col][6]
							+ cells[col][7]))
					/ local_density;
				u_y = (cells[col][2]
						+ cells[col][5]
						+ cells[col][6]
						- (cells[col][4]
							+ cells[col][7]
							+ cells[col][8]))
					/ local_density;
				tot_u += sqrtf((u_x * u_x) + (u_y * u_y));
			}

		}
	}

	return tot_u;
}

int initialise(const char* paramfile, const char* obstaclefile,
		t_param* params, t_speed** cells_ptr, t_speed** tmp_cells_ptr,
		int** obstacles_ptr, float** av_vels_ptr)
{
	char   message[1024];  /* message buffer */
	FILE*   fp;            /* file pointer */
	int    xx, yy;         /* generic array indices */
	int    blocked;        /* indicates whether a cell is blocked by an obstacle */
	int    retval;         /* to hold return value for checking */

	/* open the parameter file */
	fp = fopen(paramfile, "r");

	if (fp == NULL)
	{
		sprintf(message, "could not open input parameter file: %s", paramfile);
		die(message, __LINE__, __FILE__);
	}

	/* read in the parameter values */
	retval = fscanf(fp, "%d\n", &(params->nx));

	if (retval != 1) die("could not read param file: nx", __LINE__, __FILE__);

	retval = fscanf(fp, "%d\n", &(params->ny));

	if (retval != 1) die("could not read param file: ny", __LINE__, __FILE__);

	retval = fscanf(fp, "%d\n", &(params->maxIters));

	if (retval != 1) die("could not read param file: maxIters", __LINE__, __FILE__);

	retval = fscanf(fp, "%d\n", &(params->reynolds_dim));

	if (retval != 1) die("could not read param file: reynolds_dim", __LINE__, __FILE__);

	retval = fscanf(fp, "%f\n", &(params->density));

	if (retval != 1) die("could not read param file: density", __LINE__, __FILE__);

	retval = fscanf(fp, "%f\n", &(params->accel));

	if (retval != 1) die("could not read param file: accel", __LINE__, __FILE__);

	retval = fscanf(fp, "%f\n", &(params->omega));

	if (retval != 1) die("could not read param file: omega", __LINE__, __FILE__);

	/* and close up the file */
	fclose(fp);

	/*
	 ** Allocate memory.
	 **
	 ** Remember C is pass-by-value, so we need to
	 ** pass pointers into the initialise function.
	 **
	 ** NB we are allocating a 1D array, so that the
	 ** memory will be contiguous.  We still want to
	 ** index this memory as if it were a (row major
	 ** ordered) 2D array, however.  We will perform
	 ** some arithmetic using the row and column
	 ** coordinates, inside the square brackets, when
	 ** we want to access elements of this array.
	 **
	 ** Note also that we are using a structure to
	 ** hold an array of '.  We will allocate
	 ** a 1D array of these structs.
	 */

	/* main grid */
	*cells_ptr = (t_speed*)malloc(sizeof(t_speed) * (params->ny * params->nx));

	if (*cells_ptr == NULL) die("cannot allocate memory for cells", __LINE__, __FILE__);

	/* 'helper' grid, used as scratch space */
	*tmp_cells_ptr = (t_speed*)malloc(sizeof(t_speed) * (params->ny * params->nx));

	if (*tmp_cells_ptr == NULL) die("cannot allocate memory for tmp_cells", __LINE__, __FILE__);

	/* the map of obstacles */
	*obstacles_ptr = malloc(sizeof(int) * (params->ny * params->nx));

	if (*obstacles_ptr == NULL) die("cannot allocate column memory for obstacles", __LINE__, __FILE__);

	/* initialise densities */
	float w0 = params->density * 4.f / 9.f;
	float w1 = params->density      / 9.f;
	float w2 = params->density      / 36.f;

	for (int jj = 0; jj < params->ny; jj++)
	{
		int row = jj*params->nx;
		for (int ii = 0; ii < params->nx; ii++)
		{
			int col = row + ii;

			/* centre */
			(*cells_ptr)[col][0] = w0;
			/* axis directions */
			(*cells_ptr)[col][1] = w1;
			(*cells_ptr)[col][2] = w1;
			(*cells_ptr)[col][3] = w1;
			(*cells_ptr)[col][4] = w1;
			/* diagonals */
			(*cells_ptr)[col][5] = w2;
			(*cells_ptr)[col][6] = w2;
			(*cells_ptr)[col][7] = w2;
			(*cells_ptr)[col][8] = w2;
		}
	}

	/* first set all cells in obstacle array to zero */
	for (int jj = 0; jj < params->ny; jj++)
	{
		for (int ii = 0; ii < params->nx; ii++)
		{
			(*obstacles_ptr)[ii + jj*params->nx] = 0;
		}
	}

	/* open the obstacle data file */
	fp = fopen(obstaclefile, "r");

	if (fp == NULL)
	{
		sprintf(message, "could not open input obstacles file: %s", obstaclefile);
		die(message, __LINE__, __FILE__);
	}

	/* read-in the blocked cells list */
	while ((retval = fscanf(fp, "%d %d %d\n", &xx, &yy, &blocked)) != EOF)
	{
		/* some checks */
		if (retval != 3) die("expected 3 values per line in obstacle file", __LINE__, __FILE__);

		if (xx < 0 || xx > params->nx - 1) die("obstacle x-coord out of range", __LINE__, __FILE__);

		if (yy < 0 || yy > params->ny - 1) die("obstacle y-coord out of range", __LINE__, __FILE__);

		if (blocked != 1) die("obstacle blocked value should be 1", __LINE__, __FILE__);

		/* assign to array */
		(*obstacles_ptr)[xx + yy*params->nx] = blocked;
	}

	/* and close the file */
	fclose(fp);

	/*
	 ** allocate space to hold a record of the avarage velocities computed
	 ** at each timestep
	 */
	*av_vels_ptr = (float*)malloc(sizeof(float) * params->maxIters);

	return EXIT_SUCCESS;
}

int finalise(const t_param* params, t_speed** cells_ptr, t_speed** tmp_cells_ptr,
		int** obstacles_ptr, float** av_vels_ptr)
{
	/*
	 ** free up allocated memory
	 */
	free(*cells_ptr);
	*cells_ptr = NULL;

	free(*tmp_cells_ptr);
	*tmp_cells_ptr = NULL;

	free(*obstacles_ptr);
	*obstacles_ptr = NULL;

	free(*av_vels_ptr);
	*av_vels_ptr = NULL;

	return EXIT_SUCCESS;
}

float av_velocity(const t_param params, t_speed* cells, int* obstacles)
{
	int    tot_cells = 0;  /* no. of cells used in calculation */
	float tot_u;          /* accumulated magnitudes of velocity for each cell */

	/* initialise */
	tot_u = 0.f;

	/* loop over all non-blocked cells */
	for (int jj = 0; jj < params.ny; jj++)
	{
		for (int ii = 0; ii < params.nx; ii++)
		{
			/* ignore occupied cells */
			if (!obstacles[ii + jj*params.nx])
			{
				/* local density total */
				float local_density = 0.f;

				for (int kk = 0; kk < NSPEEDS; kk++)
				{
					local_density += cells[ii + jj*params.nx][kk];
				}

				/* x-component of velocity */
				float u_x = (cells[ii + jj*params.nx][1]
						+ cells[ii + jj*params.nx][5]
						+ cells[ii + jj*params.nx][8]
						- (cells[ii + jj*params.nx][3]
							+ cells[ii + jj*params.nx][6]
							+ cells[ii + jj*params.nx][7]))
					/ local_density;
				/* compute y velocity component */
				float u_y = (cells[ii + jj*params.nx][2]
						+ cells[ii + jj*params.nx][5]
						+ cells[ii + jj*params.nx][6]
						- (cells[ii + jj*params.nx][4]
							+ cells[ii + jj*params.nx][7]
							+ cells[ii + jj*params.nx][8]))
					/ local_density;
				/* accumulate the norm of x- and y- velocity components */
				tot_u += sqrtf((u_x * u_x) + (u_y * u_y));
				/* increase counter of inspected cells */
				++tot_cells;
			}
		}
	}

	return tot_u / (float)tot_cells;
}

float calc_reynolds(const t_param params, t_speed* cells, int* obstacles)
{
	const float viscosity = 1.f / 6.f * (2.f / params.omega - 1.f);

	return av_velocity(params, cells, obstacles) * params.reynolds_dim / viscosity;
}

float total_density(const t_param params, t_speed* cells)
{
	float total = 0.f;  /* accumulator */

	for (int jj = 0; jj < params.ny; jj++)
	{
		for (int ii = 0; ii < params.nx; ii++)
		{
			for (int kk = 0; kk < NSPEEDS; kk++)
			{
				total += cells[ii + jj*params.nx][kk];
			}
		}
	}

	return total;
}

int write_values(const t_param params, t_speed* cells, int* obstacles, float* av_vels)
{
	FILE* fp;                     /* file pointer */
	const float c_sq = 1.f / 3.f; /* sq. of speed of sound */
	float local_density;         /* per grid cell sum of densities */
	float pressure;              /* fluid pressure in grid cell */
	float u_x;                   /* x-component of velocity in grid cell */
	float u_y;                   /* y-component of velocity in grid cell */
	float u;                     /* norm--root of summed squares--of u_x and u_y */

	fp = fopen(FINALSTATEFILE, "w");

	if (fp == NULL)
	{
		die("could not open file output file", __LINE__, __FILE__);
	}

	for (int jj = 0; jj < params.ny; jj++)
	{
		for (int ii = 0; ii < params.nx; ii++)
		{
			/* an occupied cell */
			if (obstacles[ii + jj*params.nx])
			{
				u_x = u_y = u = 0.f;
				pressure = params.density * c_sq;
			}
			/* no obstacle */
			else
			{
				local_density = 0.f;

				for (int kk = 0; kk < NSPEEDS; kk++)
				{
					local_density += cells[ii + jj*params.nx][kk];
				}

				/* compute x velocity component */
				u_x = (cells[ii + jj*params.nx][1]
						+ cells[ii + jj*params.nx][5]
						+ cells[ii + jj*params.nx][8]
						- (cells[ii + jj*params.nx][3]
							+ cells[ii + jj*params.nx][6]
							+ cells[ii + jj*params.nx][7]))
					/ local_density;
				/* compute y velocity component */
				u_y = (cells[ii + jj*params.nx][2]
						+ cells[ii + jj*params.nx][5]
						+ cells[ii + jj*params.nx][6]
						- (cells[ii + jj*params.nx][4]
							+ cells[ii + jj*params.nx][7]
							+ cells[ii + jj*params.nx][8]))
					/ local_density;
				/* compute norm of velocity */
				u = sqrtf((u_x * u_x) + (u_y * u_y));
				/* compute pressure */
				pressure = local_density * c_sq;
			}

			/* write to file */
			fprintf(fp, "%d %d %.12E %.12E %.12E %.12E %d\n", ii, jj, u_x, u_y, u, pressure, obstacles[ii * params.nx + jj]);
		}
	}

	fclose(fp);

	fp = fopen(AVVELSFILE, "w");

	if (fp == NULL)
	{
		die("could not open file output file", __LINE__, __FILE__);
	}

	for (int ii = 0; ii < params.maxIters; ii++)
	{
		fprintf(fp, "%d:\t%.12E\n", ii, av_vels[ii]);
	}

	fclose(fp);

	return EXIT_SUCCESS;
}

void die(const char* message, const int line, const char* file)
{
	fprintf(stderr, "Error at line %d of file %s:\n", line, file);
	fprintf(stderr, "%s\n", message);
	fflush(stderr);
	exit(EXIT_FAILURE);
}

void usage(const char* exe)
{
	fprintf(stderr, "Usage: %s <paramfile> <obstaclefile>\n", exe);
	exit(EXIT_FAILURE);
}
