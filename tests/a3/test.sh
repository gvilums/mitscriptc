#!/bin/bash
ROOT=..
TIMEOUT=300

COMPILER=$1
INTERPRETER=$2
RESULTS=$3

# to run from project-root/vm folder
# >>>  ../../../tests/a3/test.sh ./mitscriptc ./mitscript out.txt

echo "" > $RESULTS

# Note bash methods have their own parameters internally ($1 $2 etc)
run_both_tests () {
TOTAL=0
COUNT=0

for filename in $ROOT/$3/$4/*.mit; do
    rm -f tmp.out tmp.err
    $(timeout $TIMEOUT $1 $filename > tmp.mitbc 2>&1)
    CODE=$?
		if [[ $filename =~ $ROOT/$3/$4/bad[0-9]*.mit && $CODE != 0 ]]; then
        COUNT=$((COUNT+1))
    elif [[ $CODE = 0 ]]; then
        if test -f $filename.in; then
            $(timeout $TIMEOUT $2 -b tmp.mitbc < $filename.in > tmp.out 2> tmp.err)
        else
            $(timeout $TIMEOUT $2 -b tmp.mitbc > tmp.out 2> tmp.err)
        fi
        CODE=$?
        if [[ $(cat $ROOT/$3/$4/$(basename $filename).out) =~ [A-Z][A-Za-z]+Exception ]]; then
            # here we relaxed the exception matching requirement, and FWIW, bytecodetest6.mit.out
            # has two mentions of Exception, which throws off the grading anyway.
            # used to be [ $(cat tmp.err tmp.out) == *"${BASH_REMATCH[0]}"* ]
            if [[ $(cat tmp.err tmp.out) == *"Exception"* ]]; then
                COUNT=$((COUNT+1))
            else
                echo "Expected ${BASH_REMATCH[0]} got $(cat tmp.err tmp.out)"
                echo "Fail Exception: $(basename $filename) (exit code $CODE)" >> $RESULTS
            fi
        else
            if diff tmp.out $ROOT/$3/$4/$(basename $filename).out >> $RESULTS; then
                COUNT=$((COUNT+1))
            else
                echo "Fail: $(basename $filename) (exit code $CODE)" >> $RESULTS
            fi
        fi
    else
        echo "Conversion to Bytecode Fail: $(basename $filename) (exit code $CODE)" >> $RESULTS
    fi
    TOTAL=$((TOTAL+1))
    rm -f tmp.out tmp.err tmp.mitbc
done
echo "Done" >> $RESULTS
}

run_both_tests $COMPILER $INTERPRETER a2 public
echo "Passed $COUNT out of $TOTAL tests when testing compiler and interpreter separately on a2 public" >> $RESULTS
echo "Passed $COUNT out of $TOTAL tests when testing compiler and interpreter separately on a2 public"
# run_both_tests $COMPILER $INTERPRETER a2 private
# echo "Passed $COUNT out of $TOTAL tests when testing compiler and interpreter separately on a2 private" >> $RESULTS
# echo "Passed $COUNT out of $TOTAL tests when testing compiler and interpreter separately on a2 private"
run_both_tests $COMPILER $INTERPRETER a3 public/mit
echo "Passed $COUNT out of $TOTAL tests when testing compiler and interpreter separately on a3 public" >> $RESULTS
echo "Passed $COUNT out of $TOTAL tests when testing compiler and interpreter separately on a3 public"
# run_both_tests $COMPILER $INTERPRETER a3 private
# echo "Passed $COUNT out of $TOTAL tests when testing compiler and interpreter separately on a3 private" >> $RESULTS
# echo "Passed $COUNT out of $TOTAL tests when testing compiler and interpreter separately on a3 private"


run_interp_tests(){
TOTAL=0
COUNT=0

for filename in $ROOT/$1/$2/*.mit; do
    rm -f tmp.out tmp.err
    if test -f $filename.in; then
        $(timeout $TIMEOUT $INTERPRETER -s $filename < $filename.in > tmp.out 2> tmp.err)
    else
        $(timeout $TIMEOUT $INTERPRETER -s $filename > tmp.out 2> tmp.err)
    fi
    CODE=$?
		if [[ $filename =~ $ROOT/$1/$2/bad[0-9]*.mit && $CODE != 0 ]]; then
        COUNT=$((COUNT+1))
    elif [[ $(cat $ROOT/$1/$2/$(basename $filename).out) =~ [A-Z][A-Za-z]+Exception ]]; then
        # Used to be *"${BASH_REMATCH[0]}"*, but this was relaxed
        if [[ $(cat tmp.err tmp.out) == *"Exception"* ]]; then
            COUNT=$((COUNT+1))
        else
            echo "Fail Exception: $(basename $filename) (exit code $CODE)" >> $RESULTS
        fi
    else
        if diff tmp.out $ROOT/$1/$2/$(basename $filename).out >> $RESULTS; then
            COUNT=$((COUNT+1))
        else
            echo "Fail: $(basename $filename) (exit code $CODE)" >> $RESULTS
        fi
    fi
    TOTAL=$((TOTAL+1))
    rm -f tmp.out tmp.err
done
echo "Done" >> $RESULTS
}

run_interp_tests a2 public
echo "Passed $COUNT out of $TOTAL tests when testing just interpreter on public a2"
# run_interp_tests a2 private
# echo "Passed $COUNT out of $TOTAL tests when testing just interpreter on private a2"
run_interp_tests a3 public/mit
echo "Passed $COUNT out of $TOTAL tests when testing just interpreter on public a3"
# run_interp_tests a3 private
# echo "Passed $COUNT out of $TOTAL tests when testing just interpreter on private a3"


run_bad_tests(){

TOTAL=0
COUNT=0

for filename in $ROOT/a3/$1/*mitbc; do
    echo $filename
    rm -f tmp.out tmp.err
    timeout 5 $INTERPRETER -b $filename > tmp.out 2> tmp.err
    CODE=$?
    [[ $(cat $filename.out) =~ [A-Z][A-Za-z]+Exception ]]
    if [[ $(cat tmp.err tmp.out) == *"Exception"* ]]; then
        COUNT=$((COUNT+1))
    else
        echo "Expected ${BASH_REMATCH[0]} got $(cat tmp.err tmp.out)"
        echo "Fail Exception: $(basename $filename) (exit code $CODE)" >> $RESULTS
    fi
    TOTAL=$((TOTAL+1))
    rm -f tmp.out tmp.err
done
echo "Done" >> $RESULTS
}

run_bad_tests public/mitbc
echo "Passed $COUNT out of $TOTAL public bad bytecode tests"
# run_bad_tests private/badbytecode
# echo "Passed $COUNT out of $TOTAL private bad bytecode tests"

