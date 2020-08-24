#!/bin/sh

rm -f todo

g++ -Wall -std=c++17 -g -L/usr/lib -lstdc++fs -lboost_system -lboost_program_options -O5 *.cpp -o todo

