#!/bin/bash
INTERPRETER=$1 # don't forget a ./ preceeding your interp e.g. ./mitscript

for filename in bench/*.mit; do
    echo $filename
    if test -f $filename.input; then
        time $INTERPRETER -s $filename < $filename.input > tmp1.out 2> tmp1.err
    else
        time $INTERPRETER -s $filename > tmp1.out 2> tmp1.err
    fi
    CODE=$?
    if diff tmp1.out $filename.output; then
        PCOUNT=$((PCOUNT+1))
        COUNT=$((COUNT+1))
    else
          cat tmp1.out tmp1.err
        echo "Fail: $(basename $filename) (exit code $CODE)"
    fi
    TOTAL=$((TOTAL+1))
    rm -f tmp1.out tmp1.err
done