#!/bin/bash

CTX=`pwd`

for wkstn in `find-empty-wkstn.sh b3195`; do
    ssh $wkstn cd $CTX && do-parta.sh -t 600 -m -n 10000 | active-inactive.awk >| proportion-$wkstn.log
done
