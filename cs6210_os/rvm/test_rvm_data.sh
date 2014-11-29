#!/bin/bash

rm test_rvm_data
gcc -g steque.c seqsrchst.c test_rvm_data.c -o test_rvm_data

./test_rvm_data