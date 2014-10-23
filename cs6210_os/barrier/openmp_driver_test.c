#include "omp.h"
#include "gtmp.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "unistd.h"
#include <sys/utsname.h>
#include <sys/time.h>

/*#include "gtmp.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <omp.h>
#include <sys/utsname.h>
#include <sys/time.h>
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
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [NUM_THREADS]\n", argv[0]);
        exit(1);
    }

    printf("This is the serial section\n");

    int num_threads;
    num_threads = strtol(argv[1], NULL, 10);

    gtmp_init(num_threads);
    
    // initialize global random seeds
    unsigned int seed = time(NULL);
    srand(seed);

    // initialize per thread random seeds
    unsigned int *thread_seeds = calloc(num_threads, sizeof(unsigned int));
    if (!thread_seeds) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    int i;
    for (i = 0; i < num_threads; i++) {
        thread_seeds[i] = rand();
    }

    // setup openmp
    omp_set_dynamic(0);
    if (omp_get_dynamic()) {
        printf("Warning: dynamic adjustment of threads has been set\n");
    }
    omp_set_num_threads(num_threads);

    // first parallel section
#pragma omp parallel
    {
        int thread_num = omp_get_thread_num();
        struct utsname ugnm;

        num_threads = omp_get_num_threads();
        uname(&ugnm);

        double sec = rand_dbl_range(0.5, 4, &thread_seeds[thread_num]);
        printf
            ("Hello World from thread %d of %d, running on %s, spinning for %d usec\n",
             thread_num, num_threads, ugnm.nodename,
             (unsigned int) (sec * 1000));
        spincpu(sec);

        printf("%d of %d reached barrier\n", thread_num, num_threads);

        // our goal is to replace the pragma with gtmp_barrier();
        #pragma omp barrier
	gtmp_barrier();

        printf("%d of %d exited barrier\n", thread_num, num_threads);

    }
    // implied barrier

    // Resume serial code
    printf("Back in the serial section again\n");


    // second parallel section
#pragma omp parallel
    {
        int thread_num = omp_get_thread_num();
        struct utsname ugnm;

        num_threads = omp_get_num_threads();
        uname(&ugnm);

        double sec = rand_dbl_range(0.5, 4, &thread_seeds[thread_num]);
        printf
            ("Hello World from thread %d of %d, running on %s, spinning for %d usec\n",
             thread_num, num_threads, ugnm.nodename,
             (unsigned int) (sec * 1000));
        spincpu(sec);
        printf("%d of %d reached barrier\n", thread_num, num_threads);
        #pragma omp barrier
        printf("%d of %d exited barrier\n", thread_num, num_threads);
	gtmp_barrier();

    }
    // implied barrier

    // Resume serial code
    printf("Back in the serial section - exit\n");


     gtmp_finalize();

    return 0;
}
