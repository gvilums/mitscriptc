#!/bin/bash
INTERPRETER=$1
TIMEOUT=300

run_gc_tests(){
    TOTAL=0
    COUNT=0

    MEM=4
    LIMIT="$((($MEM + 5) * 1000))"

    for filename in $1/garbagetest*.mit; do
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
    echo "Passed $COUNT out of $TOTAL gc tests"
}

run_mem_tests(){
    TOTAL=0
    COUNT=0

    run_and_parse_mem() {
        echo "RUNNING $1"
        TOTAL=$((TOTAL+1))

        LIMIT="$((($2 + 5) * 1000))"
        PREFIX="private/$1"
				rm -f tmp.out
        USAGE=$(timeout $TIMEOUT /usr/bin/time -v $INTERPRETER -mem $2 -s $PREFIX.mit < $PREFIX.input 2>&1 > tmp.out | grep Max | awk '{print $6}')
        if diff tmp.out $PREFIX.mit.out; then
            if [[ $USAGE -gt $LIMIT ]]; then
                echo "Fail: $1 ($USAGE > $LIMIT)"
            else
                echo "Pass: $1 ($USAGE < $LIMIT)"
                COUNT=$((COUNT+1))
            fi
        else
            echo "Fail: $1 ($USAGE < $LIMIT)"
        fi
				rm -f tmp.out
    }

    run_and_parse_mem refcollect 4
    run_and_parse_mem textproc 4
    run_and_parse_mem treeproc 4
    run_and_parse_mem bignum 40
    run_and_parse_mem carsim 4

    echo "Passed $COUNT out of $TOTAL mem tests"
}

run_gc_tests public
run_gc_tests private
run_mem_tests
echo "Done"
