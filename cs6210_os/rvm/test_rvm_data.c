//#include "seqsrchst.h"
#include "rvm.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_SEG_NUM	5 
#define TEST_STRING1 "hello, world"
#define TEST_STRING2 "GATECH"


/* trans data */
static rvm_t rvm;
struct _trans_t transaction;
static steque_t *stack_mods;

/*
  struct _trans_t{
  rvm_t rvm;          
  int numsegs;       
  segment_t* segments;
  };

  struct _rvm_t{
  char prefix[128];   
  int redofd;         
  seqsrchst_t segst;   
  };

  struct _segment_t{
  char segname[128];
  void *segbase;
  int size;
  trans_t cur_trans;
  steque_t mods;
  };

  _rvm_t
  +---------+
  | prefix[]|-> "rvm_segments"
  | redofd  |
  | segst   |-> seg1 -> seg2-> 0
  +---------+

  trans_t (in memory, for each transaction)
  +-----------+
  | rvm       |
  | numsegs   |     array
  | segments[]|--->+-----------+        _segment_t
  +-----------+    | _segment_t   |<==>+----------+    
  | _segment_t   |    | segname[]|      
  | ...          |    | segbase  |
  | _segment_t   |    | size     |   
  +-----------+       | cur_trans|    
  | mods     |
  +----------+ 
*/


int
eqfunc(seqsrchst_key a, seqsrchst_key b)
{
  int x, y;

  x = *(int *)a;
  y = *(int *)b;

  if (x == y) {
    return 1;
  } else {
    return 0;
  }
}

int main()
{
  int size, isempty, contains;
  int numsegs = 2;
  segment_t* segments;
  segment_t seg;
  segment_t seg1;
  char *data[2];

  /*create rvm data*/
  rvm = malloc(sizeof(struct _rvm_t));
  strcpy(rvm->prefix, "amytest");

  segments = malloc(sizeof( struct _segment_t) * numsegs);
  /* build transaction*/
  transaction.rvm = rvm;
  transaction.numsegs = numsegs;
  transaction.segments = segments;

  //build seg
  printf("------------------------------\n");
  printf("init\n");
  seqsrchst_init(&rvm->segst, eqfunc);
  stack_mods= malloc (sizeof(steque_t));
  steque_init(stack_mods);

  //data
  data[0] = malloc (17);
  sprintf(data[0], TEST_STRING1);

  // seg data 0
  seg = malloc(sizeof(struct _segment_t));
  printf("-->put 1:testseg**\n");
  int size_to_create =1;
  strcpy(seg->segname, "testseg");
  seg->size = 17;
  seg->segbase = malloc(17);
  seg->segbase = (void *)data[0];
  printf("->put1- segbase: %s \n", (char *) seg->segbase);
  seqsrchst_put(&rvm->segst, seg->segbase, seg);

  // seg data 1
  data[1] = malloc (17);
  sprintf(data[1], TEST_STRING2);

  seg1 = malloc(sizeof(struct _segment_t));
  printf("-->put 2:testseg2**\n");
  strcpy(seg1->segname, "testseg2");
  seg1->size = 17;
  seg1->segbase = malloc(17);
  seg1->segbase = (void *)data[1];
  printf("->put2- segbase: %s \n", (char *) seg1->segbase);
  seqsrchst_put(&rvm->segst, seg1->segbase, seg1);

  isempty = seqsrchst_isempty(&rvm->segst);
  printf("isempty=%d\n", isempty);

  size = seqsrchst_size(&rvm->segst);
  printf("size=%d\n", size);

  printf("------------------------------\n");
  printf("->Get- key_segbase: %s \n", (char *) seg->segbase);
  segment_t pseg_temp = seqsrchst_get(&rvm->segst, seg->segbase);
  printf ("** segname: %s, size: %d , segbase: %s \n", pseg_temp->segname,
             pseg_temp->size, (char *)pseg_temp->segbase);

  printf("------------------------------\n");
  printf("->Get- key_segbase: %s \n", (char *) seg1->segbase);
  pseg_temp = seqsrchst_get(&rvm->segst, seg1->segbase);
  printf ("** segname: %s, size: %d , segbase: %s \n", pseg_temp->segname,
          pseg_temp->size, (char *)pseg_temp->segbase);

  seqsrchst_destroy(&rvm->segst);
  printf("------------------------------\n");

  return 0;
}
