#!/bin/bash

if [ $# -ne 2 ]; then
    echo "
    USAGE:
        $0 <input file> <output filename>
        
    Expects input from run_experiment_A of two columns, the first being
    interval length and the second a count of occurences of that length.
    
    Output should be something .tex
    "  >&2
    exit 1;
fi

gnuplot << EOF
reset
set term epslatex size 4, 3 color 8
set output "$2"
set xlabel "interval length (cycles)"
set ylabel "number of intervals observed"
set pointsize 5
set key on center  center box
set style line 1 lw 5 lc rgb "#66ff66" pt 7 ps 1.5
set style line 2 lt 2 lw 1 lc rgb "#aaaaff" pt 3 ps 1
set logscale x
plot [70:*] [0:*] "$1" using 1:2 with linespoints ls 1 title "intervals",\
"" u 1:2 smooth cumulative with linespoints ls 2 title "cumulative intervals"
EOF
