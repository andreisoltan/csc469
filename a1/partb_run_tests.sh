# Assumes the following directory structure
# a1
# - partb_tests
# -- bzip2
# --- run
# ---- bzip2-O2
# ---- bzip2-O3
# ---- bzip2 

# Default options
FLAG_B=;
FLAG_L=;
FLAG_P=;
ASLR=0;


USAGE="
    USAGE:

        $0 [-b] [-l] [-p] [-a]

    Handy script for running a portion of partb.

    OPTIONS:
        -b run the bzip2 tests
		-l run the lbm tests
		-p run the perlbench tests
		-a with ASLR (for printing)

"

### Process args...

while getopts "blpa" opt; do
    case $opt in
        b)
            FLAG_B=1;
            ;;
        l)
            FLAG_L=1;
            ;;
        p)
            FLAG_P=1;
            ;;
        a)
            ASLR=1;
            ;;
        \?)
            echo "$USAGE" >&2;
            exit 1;
            ;;
    esac
done

ENV_SIZE=`echo $STRING | /usr/bin/wc -c`


# Go into the tests directory
cd partb_tests > /dev/null

if [ $FLAG_B ]; then

	# Run the bzip2 tests
	cd bzip2 > /dev/null

    # Running bzip2

	cd run > /dev/null


    if [[ $ASLR -eq 0 ]]; then
        # ... with O2
	    echo -e "input.program\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O2 input.program 10 >/dev/null ) 2>&1
	
	    echo -e "byoudoin.jpg\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O2 byoudoin.jpg 5 >/dev/null ) 2>&1
	
	    echo -e "input.combined\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O2 input.combined 80 >/dev/null ) 2>&1
	
        # ... with O3
	    echo -e "input.program\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O3 input.program 10 >/dev/null ) 2>&1
	
	    echo -e "byoudoin.jpg\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O3 byoudoin.jpg 5 >/dev/null ) 2>&1
	
	    echo -e "input.combined\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O3 input.combined 80 >/dev/null ) 2>&1
    else
    
        # ... with O2
	    echo "input.program\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O2 input.program 10 >/dev/null ) 2>&1
	
	    echo "byoudoin.jpg\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O2 byoudoin.jpg 5 >/dev/null ) 2>&1
	
	    echo "input.combined\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O2 input.combined 80 >/dev/null ) 2>&1
	
        # ... with O3
	    echo "input.program\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O3 input.program 10 >/dev/null ) 2>&1
	
	    echo "byoudoin.jpg\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O3 byoudoin.jpg 5 >/dev/null ) 2>&1
	
	    echo "input.combined\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./bzip2-O3 input.combined 80 >/dev/null ) 2>&1
    
    fi

	cd .. > /dev/null
	cd .. > /dev/null

fi

if [ $FLAG_L ]; then

	# Run the lbm tests
	cd lbm > /dev/null

	# Running lbm

	cd run > /dev/null


    if [[ $ASLR -eq 0 ]]; then
	    # ... with O2
        echo -e "lbm-O2\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./lbm-O2 `cat lbm.in` >/dev/null ) 2>&1

	    # ... with O3	
	    echo -e "lbm-O3\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./lbm-O3 `cat lbm.in` >/dev/null ) 2>&1
	    
    else
    
        # ... with O2
        echo "lbm-O2\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./lbm-O2 `cat lbm.in` >/dev/null ) 2>&1

	    # ... with O3	
	    echo "lbm-O3\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./lbm-O3 `cat lbm.in` >/dev/null ) 2>&1
    
    fi


	cd .. > /dev/null
	cd .. > /dev/null


fi

if [ $FLAG_P ]; then

	# Run the perlbench tests
	cd perlbench > /dev/null

	# Running perlbench

	cd run > /dev/null

    if [[ $ASLR -eq 0 ]]; then

	    # ... with O2		
	    echo -e "diffmail.pl\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O2 diffmail.pl `cat diffmail.in` >/dev/null ) 2>&1
	
	    echo -e "splitmail.pl\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O2 splitmail.pl `cat splitmail.in` >/dev/null ) 2>&1

	    echo -e "perfect.pl\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O2 perfect.pl `cat perfect.in` >/dev/null ) 2>&1

	    echo -e "scrabbl.pl\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O2 scrabbl.pl < scrabbl.in >/dev/null ) 2>&1
	
        # ... with O3
	    echo -e "diffmail.pl\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O3 diffmail.pl `cat diffmail.in` >/dev/null ) 2>&1
	
	    echo -e "splitmail.pl\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O3 splitmail.pl `cat splitmail.in` >/dev/null ) 2>&1

	    echo -e "perfect.pl\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O3 perfect.pl `cat perfect.in` >/dev/null ) 2>&1

	    echo -e "scrabbl.pl\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O3 scrabbl.pl < scrabbl.in >/dev/null ) 2>&1

    else
    
        # ... with O2		
	    echo "diffmail.pl\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O2 diffmail.pl `cat diffmail.in` >/dev/null ) 2>&1
	
	    echo "splitmail.pl\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O2 splitmail.pl `cat splitmail.in` >/dev/null ) 2>&1

	    echo "perfect.pl\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O2 perfect.pl `cat perfect.in` >/dev/null ) 2>&1

	    echo "scrabbl.pl\t2\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O2 scrabbl.pl < scrabbl.in >/dev/null ) 2>&1
	
        # ... with O3
	    echo "diffmail.pl\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O3 diffmail.pl `cat diffmail.in` >/dev/null ) 2>&1
	
	    echo "splitmail.pl\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O3 splitmail.pl `cat splitmail.in` >/dev/null ) 2>&1

	    echo "perfect.pl\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O3 perfect.pl `cat perfect.in` >/dev/null ) 2>&1

	    echo "scrabbl.pl\t3\t$ASLR\t$ENV_SIZE\t\c"
	    ( /usr/bin/time -f %e ./perlbench-O3 scrabbl.pl < scrabbl.in >/dev/null ) 2>&1

    
    fi
    
    
    cd .. > /dev/null
	cd .. > /dev/null

fi
