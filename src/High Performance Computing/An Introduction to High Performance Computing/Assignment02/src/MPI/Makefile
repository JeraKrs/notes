CC = mpicc
CFLAGS = -Wall -O3 -msse -fopenmp
LDFLAGS = -lm -ffast-math

jacobi: jacobi.c
	$(CC) $(CFLAGS) -o jacobi jacobi.c $(LDFLAGS)
