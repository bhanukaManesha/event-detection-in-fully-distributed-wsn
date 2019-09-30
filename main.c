#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <string.h>


// Main program
int main(int argc, char *argv[])
 {
	
	// Defining the variables to store the number of tasks and rank
	int numtasks, rank;

	// Initialize MPI
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Status stat;

	

 }