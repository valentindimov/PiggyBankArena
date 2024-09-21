#!/bin/sh
g++ -static ./PiggyBankArenaTests.cpp ./catch2/catch_amalgamated.cpp -I. -o run_test
./run_test