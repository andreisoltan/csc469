#!/usr/bin/awk -f

#BEGIN {
#    
#}

# The first line of the file is the header, we'll use this opportunity to
# do some setup and print some gnuplot header stuff
NR == 1 {

    # For gnuplot's object sequencing
    obj_seq=1;

    # Parent and child block setup
    seen_child=0;
    block_height=0.5;
    margin=0.1;
    gap=0.1;

    p_y1=margin;
    p_y2=p_y1+block_height;
    p_colour="\"#ff6666\"";

    c_y1=p_y2+gap;
    c_y2=c_y1+block_height;
    c_colour="\"#6666ff\"";

    # For determining plot size
    largest_x=0;

    # Build output file name (drop leading path components, switch extension)
    if (FILENAME=="-") { #stdin
        outfile="out";
    } else {
        outfile=FILENAME
        sub(/.*\//, "", outfile);
        sub(/\.log$/, "", outfile);
    }

    # Determine measurement unit based on column headers
    if (match($3, /-MS$/)) {
        units="ms";
        tics=20;
    } else if (match($3, /-CYCLE$/)) {
        units="cycles";
        tics=100000000;
    } else { # this should not happen...
        print "Wha happa?";
        exit;
    }

    # Gnuplot header
    print "\
set term epslatex size 7, 3 color \n\
set output \"" outfile ".tex\"\n\
\n\
set xlabel \"Time (" units ")\"\n\
set noytics\n\
set xtics mirror " tics "\n\
set mxtics 4\n\
set key on outside right bottom\n\
\n\
# define grid\n\
set style line 12 lc rgb'#808080' lt 0 lw 1\n\
set grid back ls 12\n\
\n";

}

NR > 1 {

    if ($4 > largest_x)
        largest_x=$4;

    if ($1 == "parent") {
        y_start=p_y1;
        y_end=p_y2;
        colour=p_colour;
    } else if ($1 == "child") {
        seen_child=1;
        y_start=c_y1;
        y_end=c_y2;
        colour=c_colour;
    }

    if ($2 == "active") {
        #style = "fc rgb \"black\" fs solid";
        style = "fc rgb " colour " fs solid";
    } else if ($2 == "inact") {
        #style = "fs empty";
        style = "fc rgb \"#f0f0f0\" fs solid";
    }

    print "set object ", obj_seq++, " rect from ", $3, \
      ", ", y_start, " to ", $4, ", ", y_end, style;

}

END {
    
    if (seen_child == 1) {
        top=c_y2+margin;
    } else {
        top=p_y2+margin;
    }

    print "plot [0:" largest_x "] [0:" top "] 0";
}
