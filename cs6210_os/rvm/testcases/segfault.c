/* abort.c - test that aborting a modification returns the segment to
 * its initial state */

#include "../rvm.h"
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
     char *segs[1];
     trans_t trans;
     
     rvm = rvm_init("rvm_segments");
     
     if(argc > 1)
	{
	printf("Reinit testseg\n");
	rvm_destroy(rvm, "testseg");
	}
     
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     seg = segs[0];

	printf("%s\n",seg);

     /* write some data and commit it */
     trans = rvm_begin_trans(rvm, 1, (void**) segs);
     rvm_about_to_modify(trans, seg, 0, 100);
     sprintf(seg, TEST_STRING1);
     
     rvm_about_to_modify(trans, seg, OFFSET2, 100);
     sprintf(seg+OFFSET2, TEST_STRING1);
  
     rvm_commit_trans(trans);

     /* start writing some different data, but abort */
     trans = rvm_begin_trans(rvm, 1, (void**) segs);
     rvm_about_to_modify(trans, seg, 0, 100);
     sprintf(seg, TEST_STRING2);
     
     rvm_about_to_modify(trans, seg, OFFSET2, 100);
     sprintf(seg+OFFSET2, TEST_STRING2);
return 0;
     rvm_abort_trans(trans);

	return 0;
}
