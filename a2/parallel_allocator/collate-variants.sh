#!/bin/bash

if [ $# -lt 3 ]; then
    echo "
    USAGE:
        $0 <outputdir> <input dirs ...>

    Merges the results of the input dirs into the output dir
    " >&2
    exit 1;
fi

OUTDIR=$1 ; shift;

if [ -e $OUTDIR ] ; then
    echo "$OUTDIR exists, qutting." >&2
    exit 1
fi

sedhead='s/\(^my @namelist = (\).*\();\)/\1'
sedtail='\2/'

for bench in "cache-scratch" "cache-thrash" "threadtest" "larson"  ; do
    # on the first input directoy of each benchmark, build a namelist
    # for the graphtests.pl patch
    makelist=1;
    namelist=""
    # For each run we're merging...
    for dir in "$@" ; do
        # For each build-type
        for buildpath in `ls -d benchmarks/$bench/$dir/Results/libamalloc-*` ; do
            buildname=`basename $buildpath`
            target=$OUTDIR/$bench/Results/$buildname
            if [ -d $target ] ; then
                # Not the first files to hit this directory, append
                for result in `ls $buildpath` ; do
                    cat $buildpath/$result >> $target/$result
                done
            else
                # First result files to hit this directory, copy over
                mkdir -p $target
                cp $buildpath/* $target
            fi
            if [ $makelist -eq 1 ] ; then
                namelist="\"$buildname\", $namelist"
            fi
        done
        makelist=0;
    done

    
    cat benchmarks/$bench/graphtests.pl \
        | sed 's/\(my $nthread =\) 8;/\1 4;/' \
        | sed "$sedhead$namelist$sedtail" \
        > $OUTDIR/$bench/graphtests.pl

    pushd $OUTDIR/$bench
        chmod u+x graphtests.pl
        ./graphtests.pl >| out.graph
    popd

    grep -A2 '^name' $OUTDIR/$bench/out.graph \
        | sed -f flatten.sed \
        >| $OUTDIR/$bench/out.tab
done

echo "$@" > $OUTDIR/merged
