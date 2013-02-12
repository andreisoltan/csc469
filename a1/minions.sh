#!/bin/bash

CTX=`pwd`

if [ $# -ne 3 ]; then
    echo "
    USAGE:
        $0 <lab> <command> <log>
        
    Runs <command> on all free machine in <lab>, logs each into
    <log>-<wkstn>-\`date +%s\`.log
    "  >&2
    exit 1;
fi

SUFFIX=`date +%s`.log

for wkstn in `find-empty-wkstn.sh $1`; do
    ssh $wkstn "hostname; cd $CTX && $2 >| $3-$wkstn-$SUFFIX" &
done
