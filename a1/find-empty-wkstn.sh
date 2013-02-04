#!/bin/bash
# Prints out unoccupied workstations in the specified lab

COUNTUSERS=`which count-users.sh`;

if [[ ! -e $COUNTUSERS ]]; then
    echo "count-users.sh not found" >&2
    exit 1;
fi

USAGE="
    Usage: $0 [b2200|b2210|b2220|b2240|b3175|b3185|b3195|b3200|s2360]
  
    You must provide at least one laboratory name. If you
    specify more than one, only the first will be used.";

# Number of machines to check in parallel
BANDWIDTH=5;

# Time (seconds) to pause between subsequent checks
PAUSE=2;

# To be spawned in parallel. Expects to receive one argument: the workstation
# number at which to begin checking. One instance of check() will begin at
# workstation $1 and proceed to check $1 + $BANDWIDTH, $1 + 2*$BANDWIDTH, etc
# until reaching $MAX. Between workstations we will sleep for $PAUSE seconds.
# Prints the names of workstations found to be unoccupied.
check () {
    if [[ ! $1 ]]; then
        echo "No start value passed to check()";
        exit 1;
    fi

    STATION=$1;

    # Loop on steps of $BANDWIDTH over workstations, printing only
    # if count-users.sh returns 0
    while [[ $STATION -le $MAX ]]; do
        TARGET=$LAB-`printf "%02d" $STATION`;
        COUNT=`ssh $TARGET count-users.sh`;

        if [[ $COUNT -eq 0 ]]; then
            echo $TARGET;
        fi

        (( STATION = STATION + BANDWIDTH ));
        sleep $PAUSE;
    done
}

# Process command-line argument (should be one and only one -- error otherwise)
# setting the highest-numbered workstation for the selected lab.
case $1 in
    b2200) MAX=12; ;;
    b2210) MAX=35; ;;
    b2220) MAX=24; ;;
    b2240) MAX=14; ;;
    b3175) MAX=18; ;;
    b3185) MAX=24; ;;
    b3195) MAX=18; ;;
    b3200) MAX=14; ;;
    s2360) MAX=15; ;;
    *)
        echo "Unrecognized lab name: '$1'" >&2
        echo "$USAGE" >&2 
        exit 1;
        ;;
esac

LAB=$1;

# Begin spawning parallel checks
for i in `seq 1 $BANDWIDTH`; do
    check $i &
done

