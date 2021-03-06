# This assumes that run_benchmarks_B has already been run on the particular machines.


echo -e "PROGRAM\tO2/O3\t\tO2/O3 w/ ASLR\tO2/O3 wo/ ASLR"


v1=`./partb-average-speedup.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		diffmail`


v2=`./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		diffmail 0`


v3=`./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		diffmail 1`

echo -e "perl1\t$v1\t$v2\t$v3"

v1=`./partb-average-speedup.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		splitmail`


v2=`./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		splitmail 0`


v3=`./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		splitmail 1`

echo -e "perl2\t$v1\t$v2\t$v3"


v1=`./partb-average-speedup.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		perfect`

v2=`./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		perfect 0`

v3=`./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		perfect 1`

echo -e "perl3\t$v1\t$v2\t$v3"


v1=`./partb-average-speedup.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		scrabbl`

v2=`./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		scrabbl 0`

v3=`./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		scrabbl 1`

echo -e "perl4\t$v1\t$v2\t$v3"


v1=`./partb-average-speedup.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		program`

v2=`./partb-average-speedup-aslr.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		program 0`


v3=`./partb-average-speedup-aslr.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		program 1`

echo -e "bzip1\t$v1\t$v2\t$v3"


v1=`./partb-average-speedup.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		byoudoin`


v2=`./partb-average-speedup-aslr.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		byoudoin 0`


v3=`./partb-average-speedup-aslr.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		byoudoin 1`

echo -e "bzip2\t$v1\t$v2\t$v3"


v1=`./partb-average-speedup.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		combined`

v2=`./partb-average-speedup-aslr.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		combined 0`

v3=`./partb-average-speedup-aslr.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		combined 1`

echo -e "bzip3\t$v1\t$v2\t$v3"

v1=`./partb-average-speedup.sh \
		lbm-b3185-06.log \
		lbm-b3185-07.log \
		lbm-b3185-08.log \
		lbm-b3185-09.log \
		lbm-b3185-10.log \
		.`

v2=`./partb-average-speedup-aslr.sh \
		lbm-b3185-06.log \
		lbm-b3185-07.log \
		lbm-b3185-08.log \
		lbm-b3185-09.log \
		lbm-b3185-10.log \
		. 0`

v3=`./partb-average-speedup-aslr.sh \
		lbm-b3185-06.log \
		lbm-b3185-07.log \
		lbm-b3185-08.log \
		lbm-b3185-09.log \
		lbm-b3185-10.log \
		. 1`

echo -e "lbm\t$v1\t$v2\t$v3"
