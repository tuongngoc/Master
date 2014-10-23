#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <mpi.h>
#include "gtmpi.h"

/*--------------------------------
  To run the executable created by mpicc, use
  mpirun -n 4 ./hello_mpi 

  NAME
  mpiexec -  Run an MPI program
  -n <np>              - Specify the number of processes to use

  Run this program:
  mpirun -n 5 ./dissemination
*/
static inline double
rand_dbl_range(double min, double max, unsigned int *seed)
{
    return min + ((rand_r(seed) / (double) (RAND_MAX)) * (max - min));
}

// useful function for a lot of tests
static inline void
spincpu(double secs)
{
    struct timeval start, end;
    if (gettimeofday(&start, NULL) != 0)
        return;
    while (gettimeofday(&end, NULL) == 0) {
        double elapsed =
            end.tv_sec - start.tv_sec +
            ((end.tv_usec - start.tv_usec) / 1000000.0);
        if (elapsed >= secs) {
            break;
        }
    }
}

int
main(int argc, char **argv)
{
  int my_id, num_processes;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

  unsigned int seed = time(NULL) + my_id;
  gtmpi_init(num_processes);

  double sec = rand_dbl_range(0.5, 4, &seed);

  printf("Hello World from thread %d of %d, spinning for %d.\n",
	 my_id, num_processes, (unsigned int)(sec*1000));

  spincpu(sec);

  printf("%d of %d reached barrier\n", my_id, num_processes);

  gtmpi_barrier();

  printf("%d of %d exited barrier\n", my_id, num_processes);

  MPI_Finalize();
  gtmpi_finalize();  

  printf("thread %d of %d done.\n", my_id, num_processes);

  return 0;
}
