#!/bin/bash

THREADS=4
ITER=5
WHEN=`date +%F-%H:%M:%S`

TOPDIR=`pwd`

for bench in cache-scratch cache-thrash larson threadtest ; do
    
    mkdir -p $TOPDIR/benchmarks/$bench/variants-$WHEN/Results
    
    ln -s $TOPDIR/benchmarks/$bench/Input \
        $TOPDIR/benchmarks/$bench/variants-$WHEN/

    for lib in `ls $TOPDIR/allocators/alloclibs/libamalloc-*` ; do
        libname=`basename $lib | sed 's/\..*//g'` 

        ln  $TOPDIR/benchmarks/$bench/$bench-$libname \
            $TOPDIR/benchmarks/$bench/variants-$WHEN/$bench-$libname

        $TOPDIR/benchmarks/$bench/runone.pl \
            $TOPDIR/benchmarks/$bench/variants-$WHEN \
            $libname $THREADS $ITER
    done
done
