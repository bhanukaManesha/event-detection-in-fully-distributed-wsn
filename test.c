#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <pthread.h>
#include "/usr/local/Cellar/gcc/9.2.0_1/lib/gcc/9/gcc/x86_64-apple-darwin18/9.2.0/include/omp.h"

int main(int argc, char *argv[])
{
int numprocs, rank, namelen;
char processor_name[MPI_MAX_PROCESSOR_NAME];
int iam = 0, np = 1;
MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Get_processor_name(processor_name, &namelen);
    #pragma omp parallel default(shared) private(iam, np)
    {
        np = omp_get_num_threads();
        iam = omp_get_thread_num();
        printf("Hybrid: Hello from thread %d out of %d from process %d out of %d on %s\n",
                iam, np, rank, numprocs, processor_name);
    }
MPI_Finalize();
return 0;
}