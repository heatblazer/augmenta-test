#!/bin/bash
#local test file to build and run 100 tests of the order cache since the tests have randomnes
#latest average speed in out.txt
#after hash optimization speed fell from 12~13 to 4~5

g++ --std=c++17 OrderCacheTest.cpp OrderCache.cpp -o OrderCacheTest -lgtest -lgtest_main -pthread -lOpenCL

for i in {1..100}
do
	./OrderCacheTest
done
