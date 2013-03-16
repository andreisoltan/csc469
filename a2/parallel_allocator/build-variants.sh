#!/bin/bash

for lib in `ls $TOPDIR/allocators/alloclibs/libamalloc-*` ; do
    export VARIANT=`basename $lib | sed 's/\..*//g'`
    for bench in cache-scratch cache-thrash larson threadtest ; do
        pushd $TOPDIR/benchmarks/$bench
            make variant
        popd
    done
done
