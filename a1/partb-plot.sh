paste <(cat perlbench-b3185-11.log | grep scrabbl | awk 'NR {if($2 == 2 && $3 == 0) print $1, $3, $4, $5}') <(cat perlbench-b3185-11.log | grep scrabbl | awk 'NR {if($2 == 3 && $3 == 0) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++; print $3, diff}' > partb-plot.dat

gnuplot << EOF
reset
#set term epslatex size 4, 3 color 8
set output "partb-plot.tex"
set xlabel "Environment size (bytes)"
set ylabel "runtime(O2)/runtime(O3)"
#set pointsize 5
#set key on center center box
#set style line 1 lw 5 lc rgb "#66ff66" pt 7 ps 1.5
#set style line 2 lt 2 lw 1 lc rgb "#aaaaff" pt 3 ps 1
plot "partb-plot.dat" using 1:2 title "Environment size impact"

EOF

#paste <(cat perlbench-b3185-11.log | grep scrabbl | awk 'NR {if($2 == 2 && $3 == 0) print $1, $3, $4, $5}') <(cat perlbench-b3185-11.log | grep scrabbl | awk 'NR {if($2 == 3 && $3 == 0) print $5}') | awk 'BEGIN{sum=0;count=0} NR {diff=$4/$5; sum+=diff; count++; print $3, diff}' > partb-plot.dat
