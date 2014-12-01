/* abort.c - test that aborting a modification returns the segment to
 * its initial state */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_STRING1 "hello, world"
#define TEST_STRING2 "bleg!"
#define OFFSET2 1000


int main(int argc, char **argv)
{
     rvm_t rvm;
     char *seg;
     char *segs[2];
     trans_t trans;
     trans_t badtrans;
     
     rvm = rvm_init("rvm_segments");
     
     rvm_destroy(rvm, "testseg");
     
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     if(segs[0] == (char*)-1)
     {
     		printf("rvm_map error\n");
     		exit(0);
     	}
     
     segs[1] = (char *) rvm_map(rvm, "testseg", 15000);
     
     if(segs[1] == (char*)-1)
     {
     		printf("rvm_map error PASS\n");
     	}
     	else
     	{
     		printf("rvm_map error FAIL\n");
     	}
     	
     seg = segs[0];

     /* write some data and commit it */
     trans = rvm_begin_trans(rvm, 1, (void**) segs);
     
     badtrans = rvm_begin_trans(NULL,1,(void**)segs);
     if(badtrans != (trans_t)-1) printf("badtrans FAIL\n"); else printf("badtrans PASS\n");
     badtrans = rvm_begin_trans(NULL,0,(void**)segs);
     if(badtrans != (trans_t)-1) printf("badtrans FAIL\n"); else printf("badtrans PASS\n");
     badtrans = rvm_begin_trans(rvm, 1, (void**) NULL);
     if(badtrans != (trans_t)-1) printf("badtrans FAIL\n"); else printf("badtrans PASS\n");
          
     rvm_about_to_modify(trans, seg, 0, 100);
     sprintf(seg, TEST_STRING1);
     
     rvm_about_to_modify(trans, seg, OFFSET2, 100);
     sprintf(seg+OFFSET2, TEST_STRING1);
  
     rvm_commit_trans(NULL);
     rvm_commit_trans((trans_t)-1);
     rvm_commit_trans(trans);

     /* start writing some different data, but abort */

     trans = rvm_begin_trans(rvm, 1, (void**) segs);
     rvm_about_to_modify(trans, seg, 0, 100);
     sprintf(seg, TEST_STRING2);
     
     rvm_about_to_modify(trans, seg, OFFSET2, 100);
     sprintf(seg+OFFSET2, TEST_STRING2);

     rvm_abort_trans(NULL);
     rvm_abort_trans((trans_t)-1);

     rvm_abort_trans(trans);


     /* test that the data was restored */
     if(strcmp(seg+OFFSET2, TEST_STRING1)) {
	  printf("ERROR: second hello is incorrect (%s)\n",
		 seg+OFFSET2);
	  exit(2);
     }

     if(strcmp(seg, TEST_STRING1)) {
	  printf("ERROR: first hello is incorrect (%s)\n",
		 seg);
	  exit(2);
     }
     

     rvm_unmap(rvm, seg);
     printf("OK\n");
     exit(0);
}

