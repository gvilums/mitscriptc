#!/bin/bash
INTERPRETER=$1 # don't forget a ./ preceeding your interp e.g. ./mitscript
TIMEOUT=60

TOTAL=0
COUNT=0

# for filename in public/bad*.mit; do
#     echo $filename
#     OUT=$(timeout $TIMEOUT $INTERPRETER $filename 2>&1)
#     CODE=$?
#     if [[ $CODE -ne 0 ]]; then
#         COUNT=$((COUNT+1))
#     else
#         echo "Fail: $(basename $filename) (exit code $CODE) (OUT: $OUT)"
#     fi
#     TOTAL=$((TOTAL+1))
# done

for filename in public/good*.mit; do
    echo $filename
    timeout $TIMEOUT $INTERPRETER -s $filename > tmp.out
    CODE=$?
    if diff tmp.out public/$(basename $filename).out; then
        COUNT=$((COUNT+1))
    else
        echo "Fail: $(basename $filename) (exit code $CODE)"
    fi
    TOTAL=$((TOTAL+1))
    rm -f tmp.out
done
for filename in personal/*.mit; do
    echo $filename
    timeout $TIMEOUT $INTERPRETER -s $filename > tmp.out
    CODE=$?
    if diff tmp.out public/$(basename $filename).out; then
        COUNT=$((COUNT+1))
    else
        echo "Fail: $(basename $filename) (exit code $CODE)"
    fi
    TOTAL=$((TOTAL+1))
    rm -f tmp.out
done
for filename in new/*.mit; do
    echo $filename
    timeout $TIMEOUT $INTERPRETER -s $filename > tmp.out
    CODE=$?
    if diff tmp.out public/$(basename $filename).out; then
        COUNT=$((COUNT+1))
    else
        echo "Fail: $(basename $filename) (exit code $CODE)"
    fi
    TOTAL=$((TOTAL+1))
    rm -f tmp.out
done

echo "Passed $COUNT out of $TOTAL tests"
