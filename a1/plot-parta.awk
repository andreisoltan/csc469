#!/usr/bin/awk -f

BEGIN {
    
    obj_seq=1;

    print "\
set title \"some title\"\n\
set xlabel \"Time (ms)\"\n\
set nokey\n\
set noytics\n\
set term postscript eps 10 size 20, 5\n";
}

NR == 1 {
    # Drop any leading path components, switch extension to .eps
    sub(/.*\//, "", FILENAME);
    sub(/\.log$/, ".eps", FILENAME);
    print "set output \"" FILENAME "\"\n";
}

NR > 1 {
    if ($1 == "parent") {
        y_start="0.25";
        y_end="1.25";
        colour="\"#ff6666\"";
    } else if ($1 == "child") {
        y_start="1.5";
        y_end="2.5";
        colour="\"#6666ff\"";
    }

    if ($2 == "active") {
        #style = "fc rgb \"black\" fs solid";
        style = "fc rgb " colour " fs solid";
    } else if ($2 == "inact") {
        style = "fs empty";
        #style = "fc rgb \"#f0f0f0\" fs solid";
    }

    print "set object ", obj_seq, " rect from ", $3, \
      ", ", y_start, " to ", $4, ", ", y_end, style;

    obj_seq++;
}

# Must be some way to set the axes adaptively...
END {
    print "plot [0:370] [0:2.75] 0";
}
