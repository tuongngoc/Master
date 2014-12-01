CC = gcc            
CFLAGS = -Wall   

# pattern rule for object files
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

rvm_main: seqsrchst.o steque.o rvm.o rvm_main.o
	$(CC) -o rvm_main seqsrchst.o steque.o rvm.o rvm_main.o
	
#Amy add test cases	
basic_test: testcases/basic.o seqsrchst.o steque.o rvm.o
	$(CC) -o basic_test seqsrchst.o steque.o rvm.o testcases/basic.o

multi_test: testcases/multi.o seqsrchst.o steque.o rvm.o
	$(CC) -o multi_test seqsrchst.o steque.o rvm.o testcases/multi.o


abort_test: testcases/abort.o seqsrchst.o steque.o rvm.o
	$(CC) -o abort_test seqsrchst.o steque.o rvm.o testcases/abort.o

crash_test: testcases/crash.o seqsrchst.o steque.o rvm.o
	$(CC) -o crash_test seqsrchst.o steque.o rvm.o testcases/crash.o
	

clean:
	rm -f *.o rvm_main basic_test abort_test crash_test testcases/*.o