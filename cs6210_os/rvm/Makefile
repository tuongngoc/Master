CC = gcc            
CFLAGS = -Wall   

# pattern rule for object files
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

rvm_main: seqsrchst.o steque.o rvm.o rvm_main.o
	$(CC) -o rvm_main seqsrchst.o steque.o rvm.o rvm_main.o

basic_test: testcases/basic.o seqsrchst.o steque.o rvm.o
	$(CC) -o basic_test seqsrchst.o steque.o rvm.o testcases/basic.o

multi_test: testcases/multi.o seqsrchst.o steque.o rvm.o
	$(CC) -o multi_test seqsrchst.o steque.o rvm.o testcases/multi.o
	
multi_abort_test: testcases/multi-abort.o seqsrchst.o steque.o rvm.o
	$(CC) -o multi_abort_test seqsrchst.o steque.o rvm.o testcases/multi-abort.o
	
truncate_test: testcases/truncate.o seqsrchst.o steque.o rvm.o
	$(CC) -o truncate_test seqsrchst.o steque.o rvm.o testcases/truncate.o

abort_test: testcases/abort.o seqsrchst.o steque.o rvm.o
	$(CC) -o abort_test seqsrchst.o steque.o rvm.o testcases/abort.o

crash_test: testcases/crash.o seqsrchst.o steque.o rvm.o
	$(CC) -o crash_test seqsrchst.o steque.o rvm.o testcases/crash.o

prelim_test: testcases/prelim.o seqsrchst.o steque.o rvm.o
	$(CC) -o prelim_test seqsrchst.o steque.o rvm.o testcases/prelim.o

allbad_test: testcases/doeverythingwrong.o seqsrchst.o steque.o rvm.o
	$(CC) -o allbad_test seqsrchst.o steque.o rvm.o testcases/doeverythingwrong.o
	

test: rvm_main basic_test multi_test multi_abort_test truncate_test
	./rvm_main 
	./basic_test 
	./multi_test 
	./multi_abort_test
	./truncate_test


clean:
	rm -f *.o testcases/*.o rvm_main basic_test abort_test crash_test \
	multi_test multi_abort_test
