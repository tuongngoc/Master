#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"
#include <sys/time.h>

/*
    From the MCS Paper: The scalable, distributed dissemination barrier with only local spinning.

    type flags = record
        myflags : array [0..1] of array [0..LogP - 1] of Boolean
	partnerflags : array [0..1] of array [0..LogP - 1] of ^Boolean
	
    processor private parity : integer := 0
    processor private sense : Boolean := true
    processor private localflags : ^flags

    shared allnodes : array [0..P-1] of flags
        //allnodes[i] is allocated in shared memory
	//locally accessible to processor i

    //on processor i, localflags points to allnodes[i]
    //initially allnodes[i].myflags[r][k] is false for all i, r, k
    //if j = (i+2^k) mod P, then for r = 0 , 1:
    //    allnodes[i].partnerflags[r][k] points to allnodes[j].myflags[r][k]

    procedure dissemination_barrier
        for instance : integer :0 to LogP-1
	    localflags^.partnerflags[parity][instance]^ := sense
	    repeat until localflags^.myflags[parity][instance] = sense
	if parity = 1
	    sense := not sense
	parity := 1 - parity
*/

typedef int bool;

enum { false, true };

/* number of round */
int num_rounds = 0;
int num_processors = 0;
bool sense;
int tag;
MPI_Status status;

/*---------Utility Math functions ---------------*/

/* log base 2 of an int with lookup table*/
static const char LogTable256[256] =
  {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
        -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
        LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
  };

static inline unsigned ceil_log_base_2 (unsigned int v)
{
  unsigned r;   // r will be lg(v)
  register unsigned int t, tt; // temporaries
  int i;
  unsigned int n = 1;
 
  if ((tt = v >> 16)) {
    r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
  }
  else {
    r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
  }
    	
  for (i=0 ; i < r -1 ; i++) {
    n = n * 2;
  }
  if (v > n) {
    r = r+1;
  }
  return r;
}
 
static inline int my_pow (int base, int power) 
{
  int n = 1;
  int i;
  for (i=0; i < power; i++) {
    n *= base;
  }
  return n;
}

void gtmpi_init(int num_threads)
{
  sense = true;
  tag = 1;
  num_processors = num_threads;
  /*round = ceil ( log2 (N) */
  num_rounds = ceil_log_base_2(num_threads);
}

void gtmpi_barrier(){
  int round = 0;
  int my_id;
  int partner_id;
  int listen_partner_id;
  int recv_sense = 1;
  
  /*Get ID*/
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  
  /*doing round */
  for(round = 0; round < num_rounds; round++) {
    /*send msg to correct thread*/
    partner_id = (my_id + (int)my_pow(2,round)) % num_processors;
    /*Recieve Message from correct threads*/
    listen_partner_id = (my_id - (int)my_pow(2,round)) + num_processors;
    listen_partner_id = listen_partner_id % num_processors;
    MPI_Send(&sense, 1, MPI_INT, partner_id, tag, MPI_COMM_WORLD);
    MPI_Recv(&recv_sense, 1, MPI_INT, listen_partner_id, tag, MPI_COMM_WORLD, &status);    
  }
  /*Toggle the sense*/
  sense = !sense;
}

void gtmpi_finalize(){
  /*reset global values*/
  num_rounds = 0;
  num_processors = 0;
}
