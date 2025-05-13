#!/bin/bash

# Compile the program
make clean
make

# Test 1: Run with small number of cars
echo "Test 1: Running with 10 cars"
./hw4 10

# Test 2: Run with medium number of cars
echo "Test 2: Running with 50 cars"
./hw4 50

# Test 3: Run with large number of cars
echo "Test 3: Running with 100 cars"
./hw4 100

# Test 4: Run with very large number of cars
echo "Test 4: Running with 200 cars"
./hw4 200

# Test 5: Run multiple instances simultaneously
echo "Test 5: Running multiple instances simultaneously"
./hw4 30 &
./hw4 30 &
./hw4 30 &
wait

echo "All tests completed" 