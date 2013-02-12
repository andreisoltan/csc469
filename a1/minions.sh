#!/bin/bash

CTX=`pwd`

if [ $# -ne 3 ]; then
    echo "
    USAGE:
        $0 <lab> <command> <log>
        
    Runs <command> on all free machine in <lab>, logs each into
    <log>-<wkstn>.log
    "  >&2
    exit 1;
fi

for wkstn in `find-empty-wkstn.sh $1`; do
    ssh $wkstn "cd $CTX && $2 >| $3-$wkstn.log"
done
