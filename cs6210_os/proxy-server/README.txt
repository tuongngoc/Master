*RPC tasks
------------

1) RPC Client

a) To Build proxy_rpc
------------------
rpcgen proxy_rpc.x

->Will Genereate 3 files:
 proxy_rpc_clnt.c - the codes for client side
 proxy_rpc_svc.c - the code for server side
 proxy_rpc.h - a header for both side


b) To Build Client test
----------------------
make client_test client

c) now run 
sudo ./client_test

in another terminal run
./client localhost example.com


2) Cache Policy Implementations
---------------------------------

*Using the provided utility files:
---------------------------------
hstbl.[ch]: contains a string-indexed hashtable to help you quickly retrieve 
data from the cache.

indexminpq.[ch]: contains a indexed minimum priority queue[link] generic 
data structure to help you quickly figure out which item to replace in the 
cache.

indexrndq.[ch] : contains an indexed randomized queue supporting constant-
time arbitrary deletion.

steque.[ch] : contains a steque data structure which might be useful for 
keeping track of available ids.

*a)Least Recently Used (LRU) :remove the item that has been requested least 
recently.

#make perform
which will produce lru_perform, rnd_perform, lfu_perform, and lrumin_perform executables.  
If you list of requests is in a file requests.txt, you can then you can test the performance of your algorithms 
by passing the file through stdin.  
For example, to see the performance of lru on requests.txt with 2000 requests, one would run
/*Usage: cache_test [FAKE_WWW] [NUM_REQUESTS]*/
#./lru_perform webdata.txt 2000 < requests.txt

./lfu_perform amy_webdata.txt 7 < requests.txt


* Test LFU only
---------------
make lfu_perform
#./lfu_perform 

./lfu_perform amy_webdata.txt 7 < requests.txt

* Test LRU only
---------------
make lru_perform
#./lru_perform 

./lru_perform amy_webdata.txt 7 < requests.txt

*Test Random
--------------
make rnd_perform
./rnd_perform amy_webdata.txt 7 < requests.txt


********************************************
* Test Codes
********************************************
test_indexminpq
gcc -g indexminpq.c test_indexminpq.c -o test_indexminpq

test_steque
gcc -g steque.c test_steque.c -o test_steque

test_indexrndq
gcc -g indexrndq.c test_indexrndq.c -o test_indexrndq

Test LFU
---------
make lfu_test

Note: NOT complete the lrumin.c task


