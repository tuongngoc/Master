#include <omp.h>
#include "gtmp.h"
#include <stdlib.h>
#include <stdio.h>

typedef int bool;
enum {false,true};

typedef struct barrier_t {
  int total_threads;
  int count;
  bool sense;
} barrier_t;

static barrier_t* Gtbarrier;

/*
  From the MCS Paper: A sense-reversing centralized barrier

  shared count : integer := P
  shared sense : Boolean := true
  processor private local_sense : Boolean := true

  procedure central_barrier
  local_sense := not local_sense // each processor toggles its own sense
  if fetch_and_decrement (&count) = 1
  count := P
  sense := local_sense // last processor toggles global sense
  else
  repeat until sense = local_sense
*/


void gtmp_init(int num_threads){
  omp_set_num_threads(num_threads);
  Gtbarrier = (barrier_t*) malloc(sizeof(barrier_t));
  if (Gtbarrier == NULL) {
    fprintf(stderr, "Failed to allocate memory. \n");
    exit(-1);
  }
  Gtbarrier->total_threads = num_threads;
  Gtbarrier->count = num_threads;
  Gtbarrier->sense = true;

}

void gtmp_barrier(){
  bool local_sense = false;

  /*each process toggle it own sense*/
  local_sense = Gtbarrier->sense;

  /*synchronizes data in all threads,
  A full memory barrier is created*/
  __sync_synchronize();

  if (Gtbarrier->sense == local_sense) {
    local_sense = !local_sense;

    /* subtracts the value count,and the last one reset count
    last one go to loop and wait for sense to change 
	*/
    if (__sync_sub_and_fetch(&(Gtbarrier->count), 1) == 0) {
#pragma omp critical
      {

        Gtbarrier->sense = local_sense;
        Gtbarrier->count = Gtbarrier->total_threads;
      }
    }
    else {
      /* just do nothing*/
      while (local_sense != Gtbarrier->sense);
    }
  }

}

void gtmp_finalize(){
  if (Gtbarrier != NULL) {
    free(Gtbarrier);
  }

}
