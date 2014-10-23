#OpenMP Flags Etc.
#OMPFLAGS = -g -Wall -fopenmp -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE`
#OMPFLAGS = -g -Wall -fopenmp
#OMPLIBS = -lgomp
#CC = gcc
OMPFLAGS = -fopenmp
OMPLIBS = -lgomp -lm

CC = gcc
CPPFLAGS = -g -Wall $(OMPFLAGS)
LDFLAGS = -g -Wall
LDLIBS = $(OMPLIBS)


#MPI Flags Etc (may need to customize)
#MPICH = /usr/lib64/openmpi/1.4-gcc
MPIFLAGS = -g -Wall #-I$(MPICH)/include
MPILIBS =
#MPICC = /opt/openmpi-1.4.3-gcc44/bin/mpicc
MPICC = mpicc

all: hello_openmp gtmp_counter gtmp_mcs gtmp_tree \
	hello_mpi dissemination_mpi tournament_mpi gtmpi_counter

hello_openmp: hello_openmp.c
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

gtmp_counter: openmp_driver_test.o gtmp_counter.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

gtmp_mcs: openmp_driver_test.o gtmp_mcs.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

gtmp_tree: openmp_driver_test.o gtmp_tree.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)


hello_mpi: hello_mpi.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

dissemination_mpi: mpi_driver_test.c gtmpi_dissemination.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

tournament_mpi: mpi_driver_test.c gtmpi_tournament.c 
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

gtmpi_counter: mpi_driver_test.c gtmpi_counter.c
	$(MPICC) $(MPIFLAGS) -o $@ $^ $(MPILIBS)

clean:
	rm -rf *.o hello_openmp hello_mpi gtmp_counter gtmp_mcs \
	gtmp_tree dissemination_mpi tournament_mpi gtmpi_counter
