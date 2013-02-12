#!/bin/bash



# Default options
FLAG_B=;
FLAG_L=;
FLAG_P=;
FILE=;
OPT=0;


USAGE="
    USAGE:

        $0 [-b] [-l] [-p] [-f <output-file>]

    Handy script for running partb.

    OPTIONS:
        -b run the bzip2 tests
		-l run the lbm tests
		-p run the perlbench tests
		-f the output file to write to

"

### Process args...

while getopts "blpf:" opt; do
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
        f)
            FILE=$OPTARG;
            ;;
        \?)
            echo "$USAGE" >&2;
            exit 1;
            ;;
    esac
done

SETARCH=`which setarch`
UNAME=`which uname`


if [ $FLAG_B ]; then

    ## Unset all the environment variables (except the 3 we can't remove)
    ## Not using the shebang "#!/usr/bin/env - /bin/bash" since didn't work on CDF
    unset $(/usr/bin/env | egrep '^(\w+)=(.*)$' | \
		      egrep -vw 'PWD|USER|LANG' | /usr/bin/cut -d= -f1);

    echo -e "PROGRAM\t\tOPT\tASLR\tENV\tTIME"

    for i in {1..20}
    do
        # With ASLR
        ./partb_run_tests.sh -b
        # Without ASLR
        $SETARCH `$UNAME -m` -R ./partb_run_tests.sh -b -a
        
        if [[ $i -eq 1 ]]; then
            MY_STRING="ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ012345678901"
            export STRING=${MY_STRING}${MY_STRING}
            
        else
            # increase the length of the string
            STRING=${STRING}${MY_STRING}${MY_STRING}
        fi
        
    done
fi

if [ $FLAG_L ]; then

    ## Unset all the environment variables (except the 3 we can't remove)
    ## Not using the shebang "#!/usr/bin/env - /bin/bash" since didn't work on CDF
    unset $(/usr/bin/env | egrep '^(\w+)=(.*)$' | \
		      egrep -vw 'PWD|USER|LANG' | /usr/bin/cut -d= -f1);


    echo -e "PROGRAM\tOPT\tASLR\tENV\tTIME"

    for i in {1..20}
    do

        # With ASLR
        ./partb_run_tests.sh -l
        # Without ASLR
        $SETARCH `$UNAME -m` -R ./partb_run_tests.sh -l -a
        
        if [[ $i -eq 1 ]]; then
            MY_STRING="ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ012345678901"
            export STRING=${MY_STRING}${MY_STRING}
            
        else
            # increase the length of the string
            STRING=${STRING}${MY_STRING}${MY_STRING}
        fi

    done

fi

if [ $FLAG_P ]; then

    ## Unset all the environment variables (except the 3 we can't remove)
    ## Not using the shebang "#!/usr/bin/env - /bin/bash" since didn't work on CDF
    unset $(/usr/bin/env | egrep '^(\w+)=(.*)$' | \
		      egrep -vw 'PWD|USER|LANG' | /usr/bin/cut -d= -f1);


    echo -e "PROGRAM\t\tOPT\tASLR\tENV\tTIME"


    for i in {1..20}
    do
        # With ASLR
        ./partb_run_tests.sh -p
        # Without ASLR
        $SETARCH `$UNAME -m` -R ./partb_run_tests.sh -p -a

        if [[ $i -eq 1 ]]; then
            MY_STRING="ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ012345678901"
            export STRING=${MY_STRING}${MY_STRING}
            
        else
            # increase the length of the string
            STRING=${STRING}${MY_STRING}${MY_STRING}
        fi

    done
    
fi
