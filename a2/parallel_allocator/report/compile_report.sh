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

## Compile latex doc ############################
latex report.tex
latex report.tex
dvips -Ppdf -o report.ps report.dvi
ps2pdf report.ps

