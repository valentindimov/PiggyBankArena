#!/bin/sh
g++ -static PiggyBankArenaTestsC.cpp PiggyBankArenaTestsCPP.cpp catch2/catch_amalgamated.cpp -I. -o run_test
./run_test