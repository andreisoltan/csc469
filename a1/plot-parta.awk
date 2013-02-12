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

    i_colour="\"#d0d0d0\"";

    top=p_y2+margin;

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
set key off\n\
\n\
# define grid\n\
set style line 12 lc rgb'#808080' lt 0 lw 1\n\
set grid back ls 12\n\
# fake key styles\n\
set style line 13 lc rgb " p_colour " lt 0 lw 5\n\
set style line 14 lc rgb " c_colour " lt 0 lw 5\n\
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
        top=c_y2+margin;
        y_start=c_y1;
        y_end=c_y2;
        colour=c_colour;
    }

    if ($2 == "active") {
        style = "fc rgb " colour " fs solid";
    } else if ($2 == "inact") {
        style = "fc rgb " i_colour " fs solid";
    }

    print "set object ", obj_seq++, " rect from ", $3, \
      ", ", y_start, " to ", $4, ", ", y_end, style;

}

END {
    
    if (seen_child == 1) {
        top=c_y2+margin;
#        print "set label 'parent active' at screen 0,0 offset 5,0.5;";
#        print "set object " obj_seq++ " rect from screen 0,0 to 0.07,-0.05 fc rgb " p_colour " fs solid";

#       print "set label 'child active' at screen 0,0 offset 5,-1.2;";
#        print "set object " obj_seq++ " rect from screen 0,-0.07 to 0.07,-0.12 fc rgb " c_colour " fs solid";
    } else {
        # Did not see a child
#        print "set label 'process active' at screen 0,0 offset 5,0.5;";
#        print "set object " obj_seq++ " rect from screen 0,0 to 0.1,-0.1 fc rgb " p_colour " fs solid";
        top=p_y2+margin;
    }

    print "plot [0:" largest_x "] [0:" top "] 0 ";
}
