#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "gtmpi.h"
#include <string.h>

/*
    From the MCS Paper: A scalable, distributed tournament barrier with only local spinning

    type round_t = record
        role : (winner, loser, bye, champion, dropout)
	opponent : ^Boolean
	flag : Boolean
    shared rounds : array [0..P-1][0..LogP] of round_t
        // row vpid of rounds is allocated in shared memory
	// locally accessible to processor vpid

    processor private sense : Boolean := true
    processor private vpid : integer // a unique virtual processor index

    //initially
    //    rounds[i][k].flag = false for all i,k
    //rounds[i][k].role = 
    //    winner if k > 0, i mod 2^k = 0, i + 2^(k-1) < P , and 2^k < P
    //    bye if k > 0, i mode 2^k = 0, and i + 2^(k-1) >= P
    //    loser if k > 0 and i mode 2^k = 2^(k-1)
    //    champion if k > 0, i = 0, and 2^k >= P
    //    dropout if k = 0
    //    unused otherwise; value immaterial
    //rounds[i][k].opponent points to 
    //    round[i-2^(k-1)][k].flag if rounds[i][k].role = loser
    //    round[i+2^(k-1)][k].flag if rounds[i][k].role = winner or champion
    //    unused otherwise; value immaterial
    procedure tournament_barrier
        round : integer := 1
	loop   //arrival
	    case rounds[vpid][round].role of
	        loser:
	            rounds[vpid][round].opponent^ :=  sense
		    repeat until rounds[vpid][round].flag = sense
		    exit loop
   	        winner:
	            repeat until rounds[vpid][round].flag = sense
		bye:  //do nothing
		champion:
	            repeat until rounds[vpid][round].flag = sense
		    rounds[vpid][round].opponent^ := sense
		    exit loop
		dropout: // impossible
	    round := round + 1
	loop  // wakeup
	    round := round - 1
	    case rounds[vpid][round].role of
	        loser: // impossible
		winner:
		    rounds[vpid[round].opponent^ := sense
		bye: // do nothing
		champion: // impossible
		dropout:
		    exit loop
	sense := not sense
*/
#define MAX_NUM_THREADS  1000
#define MAX_LEVELS       1000
typedef enum { WINNER = 0, LOSER, BYE, CHAMPION, DROPOUT } roles;

typedef struct _round_t {
  int role;
  int opponent;
}round_t;

int num_processors;
int num_rounds;
int rank;
int sense;
MPI_Status status;

static round_t rounds[MAX_NUM_THREADS][MAX_LEVELS];

/*---------Utility Math functions ---------------*/

/* log base 2 of an int with lookup table*/
static const char LogTable256[256] =
  {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
        -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
        LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
  };

static inline
unsigned ceil_log_base_2 (unsigned int v)
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

static inline
int my_pow (int base, int power) 
{
  int n = 1;
  int i;
  for (i=0; i<power; i++) {
    n *= base;
  }
  return n;
}

void gtmpi_init(int num_threads) {
  int i;
  int j;
  int oppo_rank;
  num_processors = num_threads;
  num_rounds = ceil_log_base_2 (num_processors);
  sense = 1;
  //memset(rounds,0,sizeof(round_t)* MAX_NUM_THREADS * MAX_LEVELS);
  if(num_processors > MAX_NUM_THREADS) {
    fprintf(stderr, "\n **Error. Only support MAX_THREAD: <%d> \n",
	    MAX_NUM_THREADS);
    fprintf(stderr, "\n @@ Entered with num_threads: %d, level: %d \n",
	    num_processors, num_rounds);
    exit(1);
  }
  /*Get ID */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  /*clear out the rounds data*/
  for(i = 1; i <= num_processors ; i++) {
    rounds[rank][i].role = -1;
    rounds[rank][i].opponent = -1;
  }

  /*Initializing the roles*/
  for(i = 0; i < num_processors; i++) {
    for(j = 1; j <= num_rounds; j++) {    
      /*WINNER*/
      if( (j > 0) &&
	  (i % (int)my_pow(2,j) == 0 ) &&
	  (i + (int)my_pow(2, (j-1)) < num_processors) &&
	  ((int)my_pow(2,j) <  num_processors)) {
	rounds[i][j].role = WINNER;
      }
      /*BYE*/
      else if((j > 0) &&
	      ((i % (int)my_pow(2, j)) == 0 ) &&
	      ((i + (int)my_pow(2, j-1)) >=  num_processors)) {
	rounds[i][j].role = BYE;
      }
      /*LOSER*/
      else if((j > 0) &&
	      (i % (int)my_pow(2,j)) == (int)my_pow(2, j-1)) {
	rounds[i][j].role = LOSER;
      }
      /*CHAMPION*/
      else if((j > 0) &&
	      (i == 0) &&
	      ((int)my_pow(2, j) >=  num_processors )
	      ) {
	rounds[i][j].role = CHAMPION;
      }
      /*DROPOUT*/
      else if(j == 0) {
	rounds[i][j].role = DROPOUT;
      }
      /*Initializing the opponents*/
      if(rounds[i][j].role == LOSER) {
	oppo_rank = i - (int)my_pow(2,j-1);
	rounds[i][j].opponent = oppo_rank;
      }
      else if(rounds[i][j].role == WINNER ||
	      rounds[i][j].role == CHAMPION){
	oppo_rank = i + (int)my_pow(2, j-1);
	rounds[i][j].opponent = oppo_rank;
      }
    }
  } /* Done with Initialization*/
}

void gtmpi_barrier()
{
  const int MSG_TAG = 777;
  int round;
  int recd_sense = 1;
  int my_role;
  int my_opponent;
  /* The arival loop*/
  for(round = 1; round <= num_rounds; round++) {
    my_role = rounds[rank][round].role;
    my_opponent = rounds[rank][round].opponent;
    if(my_role == LOSER) {
      MPI_Send(&sense, 1, MPI_INT, my_opponent, MSG_TAG, MPI_COMM_WORLD);
      MPI_Recv(&recd_sense, 1, MPI_INT, my_opponent, MSG_TAG, MPI_COMM_WORLD, &status);
      break;
    }
    else if(my_role == WINNER) {     
      MPI_Recv(&recd_sense, 1, MPI_INT, my_opponent, MSG_TAG, MPI_COMM_WORLD, &status);
    }
    else if(my_role == CHAMPION) {     
      MPI_Recv(&recd_sense, 1, MPI_INT, my_opponent, MSG_TAG, MPI_COMM_WORLD, &status);
      MPI_Send(&sense, 1, MPI_INT, my_opponent, MSG_TAG, MPI_COMM_WORLD);
      break;
    }
  }/*end of arrival loop*/
  
  // printf("Value of round before wake up starts = %d \n ", round);
  /* The wakeup loop*/
  while(round > 0) {
    my_role = rounds[rank][round].role;
    my_opponent = rounds[rank][round].opponent;
    if(my_role == WINNER) {
      MPI_Send(&sense, 1, MPI_INT, my_opponent, MSG_TAG, MPI_COMM_WORLD);
    }
    else if(my_role == DROPOUT) {
      break;
    }
    round--;
  }/*end while wakeup loop*/
  /* toggle sense*/
  sense = (sense + 1) % 2;
}

void gtmpi_finalize() {
  num_processors = 0;
  num_rounds = 0;
  rank = 0;
}
  
