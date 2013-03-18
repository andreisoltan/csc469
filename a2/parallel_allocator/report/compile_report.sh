#!/bin/bash

## Generate timing-server plots #################
pushd timing-server
    for dir in `ls` ; do
        pushd $dir
            ./graphtests.pl >| out
        popd
    done
popd

## Generate larson-greywolf plot ################
pushd larson-greywolf
    ./graphtests.pl >| out
popd

## Generate tabular data from variants ##########
pushd latest-variants/
    for file in allout-frag-grouped.tab allout-scal-grouped.tab ; do
        grep larson $file \
        | awk '{print $1, $2}' \
        | sed -e '/[0-9]/ s/ / \& /g' \
            -e '/[0-9]/ s/$/ \\\\ \\hline/ ' \
            -e 's/^ *$/\\hline/g' >| `basename $file .tab`.tex
    done
popd

## Compile latex doc ############################
latex report.tex
latex report.tex
dvips -Ppdf -o report.ps report.dvi
ps2pdf report.ps

