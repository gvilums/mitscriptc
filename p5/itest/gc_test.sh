#!/bin/bash
INTERPRETER=$1
TIMEOUT=300

# Usage: ./cpptest.sh <path to interpreter>

run_gc_tests(){
    TOTAL=0
    COUNT=0

    MEM=4
    LIMIT="$((($MEM + 5) * 1000))"

    for filename in ./gc/garbagetest*.mit; do
        echo "RUNNING $(basename $filename)"
				rm -f tmp.out
        USAGE=$(timeout $TIMEOUT /usr/bin/time -v $INTERPRETER -mem $MEM -s $filename 2>&1 > tmp.out | grep Max | awk '{print $6}')
        CODE=$?
        if [[ $CODE = 0 ]]; then
            if diff tmp.out $filename.out; then
                if [[ $USAGE -gt $LIMIT ]]; then
                    echo "Fail: $(basename $filename) ($USAGE > $LIMIT)"
                else
                    echo "Pass: $(basename $filename) ($USAGE < $LIMIT)"
                    COUNT=$((COUNT+1))
                fi
            else
                echo "Fail: $(basename $filename) (exit code $CODE)"
						fi
        else
            echo "Fail: $(basename $filename) (exit code $CODE)"
        fi
        TOTAL=$((TOTAL+1))
				rm -f tmp.out
    done
    echo "Passed $COUNT out of $TOTAL public gc tests"
}

run_gc_tests_personal() {
    TOTAL=0
    COUNT=0

    MEM=10
    LIMIT="$((($MEM + 5) * 1000))"

    for filename in ./gcpersonal/*.mit; do
        echo "RUNNING $(basename $filename)"
				rm -f tmp.out
        USAGE=$(timeout $TIMEOUT /usr/bin/time -v $INTERPRETER -mem $MEM -s $filename 2>&1 > tmp.out | grep Max | awk '{print $6}')
        CODE=$?
        if [[ $CODE = 0 ]]; then
            if diff tmp.out $filename.out; then
                if [[ $USAGE -gt $LIMIT ]]; then
                    echo "Fail: $(basename $filename) ($USAGE > $LIMIT)"
                else
                    echo "Pass: $(basename $filename) ($USAGE < $LIMIT)"
                    COUNT=$((COUNT+1))
                fi
            else
                echo "Fail: $(basename $filename) (exit code $CODE)"
						fi
        else
            echo "Fail: $(basename $filename) (exit code $CODE)"
        fi
        TOTAL=$((TOTAL+1))
				rm -f tmp.out
    done
    echo "Passed $COUNT out of $TOTAL personal gc tests"
}

run_gc_tests public
run_gc_tests_personal
echo "Done"