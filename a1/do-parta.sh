#!/bin/bash

# Default options
FLAG_I=;
FLAG_M=;
FLAG_C=;
FLAG_T=;
CPU=0;
THRESH=;
NUM=100;

get_freq() {
    # Strip out the current clock frequency lines from /proc/cpuinfo and select
    # the line appropriate to our current choice of CPU.
    sed -n 's/cpu MHz[^:]*: //p' </proc/cpuinfo | sed -n "$((CPU+1)) s/\(.\)/\1/p"
}

USAGE="
    USAGE:

        $0 [-h] [-m] [-c | -i | -t <thresh>] [-n <num>] [-p <processor>]

    Handy script for running parta, our data collector for Part A of
    Assignment 1 for CSC469. Mostly passes arguments through, also binds
    execution to a specific CPU and can pass along the current clock frequency
    of that CPU.

    OPTIONS:

        -h  prints help message
        -m  report time in milliseconds, default to false (report in cycles)
        -c  context switching, passed to parta, defaults to false
        -i  interval mode, passed to parta, defaults to false, overrides -t, -c
        -t  threshold for inactivity measurement, passed to parta, overriden by -i
        -n  number of samples passed to parta, defaults to $NUM
        -p  processor to run on, passed to taskset, defaults to $CPU. taskset
            will complain if you choose something invalid.

"

### Process args...

while getopts "hicmt:n:p:" opt; do
    case $opt in
        i)
            FLAG_I=1;
            ;;
        m)
            FLAG_M=1;
            ;;
        c)
            FLAG_C=1;
            ;;
        t)
            FLAG_T=1;
            THRESH=$OPTARG;
            ;;
        n)
            NUM=$OPTARG;
            ;;
        p)
            CPU=$OPTARG;
            ;;
        h)  
            echo "$USAGE";
            exit;
            ;;
        \?)
            echo "$USAGE" >&2;
            exit 1;
            ;;
    esac
done

### Build arguments
ARGS="-n $NUM"

if [ $FLAG_T ]; then
    ARGS="$ARGS -t $THRESH"
fi

if [ $FLAG_I ]; then
    ARGS="$ARGS -i"
fi

if [ $FLAG_M ]; then
    ARGS="$ARGS -f `get_freq`"
fi

if [ $FLAG_C ]; then
    ARGS="$ARGS -c"
fi

### Gogogogogogo
taskset -c $CPU ./parta $ARGS
