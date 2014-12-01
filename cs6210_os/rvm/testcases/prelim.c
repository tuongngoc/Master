#include <stdio.h>
#include <string.h>
#include <rvm.h>

int main(int argc, char **argv)
{
	rvm_t rvm;
	char* segs[1];
	trans_t testTrans;

	rvm = rvm_init("rvm_segments");
	segs[0] = (char *) rvm_map(rvm, "testseg", 10000);


	testTrans = rvm_begin_trans(rvm,1,segs);
	if(testTrans != -1)
	{
		rvm_
	}
	strcpy(segs[0],"Hello World!");
	printf("%s",segs[0]);
	rvm_unmap(rvm,segs[0]);
	return 0;
}
