#!/bin/bash

# Usage: ./cpptest.sh <path to project>/gc <path to project>/tests/a4/public

P4_ROOT=$1
TEST_DIR=$2

TOTAL=0
COUNT=0

for filename in $TEST_DIR/t*.cpp; do
  rm -f a.out
  g++ -std=c++17 -O2 -I$P4_ROOT $filename
  ./a.out
  if [ $? == 0 ]; then
    echo "Passed" $filename
    COUNT=$((COUNT+1))
  else
    echo "Failed" $filename
  fi
  TOTAL=$((TOTAL+1))
done
rm -f a.out

echo "Passed $COUNT out of $TOTAL tests in $TEST_DIR"
