#!/bin/bash
# USAGE: ./run-tests.sh [public|private] <path to mitscript-parser>
TESTS=$1
PARSER=$2

TOTAL=0
COUNT=0

for filename in $TESTS/bad*.mit; do
    OUT=$(cat $filename | $PARSER)
    CODE=$?
    if [[ $CODE -eq 0 ]] && [[ $OUT != *"Error"* ]]; then
        echo "Fail: $(basename $filename) (exit code $CODE)"
    else
        COUNT=$((COUNT + 1))
    fi
    TOTAL=$((TOTAL + 1))
done

for filename in $TESTS/good*.mit; do
    OUT=$(cat $filename | $PARSER)
    CODE=$?
    if [[ $CODE -ne 0 ]] || [[ $OUT = *"Error"* ]]; then
        echo "Fail: $(basename $filename) (exit code $CODE)"
    else
        COUNT=$((COUNT + 1))
    fi
    TOTAL=$((TOTAL + 1))
done

echo "$PARSER Passed $COUNT out of $TOTAL tests"
