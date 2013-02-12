#!/usr/bin/awk -f

## Accumulate the length of each interval based on active/inact column
NR>1 {
    time[$2]+=$5
}

## Print results
END {
    print "a:", time["active"]
    print "i:", time["inact"]

    print "i/(a+i):", time["inact"] / (time["active"] + time["inact"])
}
