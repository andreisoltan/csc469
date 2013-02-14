paste <(cat perlbench-b3185-11.log | grep scrabbl | awk 'NR {if($2 == 2 && $3 == 0) print $1, $3, $4, $5}') <(cat perlbench-b3185-11.log | grep scrabbl | awk 'NR {if($2 == 3 && $3 == 0) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++; print $3, diff}' > partb-plot.dat

gnuplot << EOF
reset
set term epslatex size 4, 3 color 8
set output "partb-plot.tex"
set xlabel "Environment size (bytes)"
set ylabel "runtime(O2)/runtime(O3)"
plot "partb-plot.dat" using 1:2 notitle

EOF

#paste <(cat perlbench-b3185-11.log | grep scrabbl | awk 'NR {if($2 == 2 && $3 == 0) print $1, $3, $4, $5}') <(cat perlbench-b3185-11.log | grep scrabbl | awk 'NR {if($2 == 3 && $3 == 0) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++; print $3, diff}' > partb-plot.dat
