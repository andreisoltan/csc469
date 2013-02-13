
echo -e "PROGRAM\tO2/O3\tO2/O3 w/ ASLR\tO2/O3 wo/ ASLR"

echo -e "perl1\t\c"
v1=`./partb-average-speedup.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		diffmail`

echo -e "\t"
./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		diffmail 0

echo -e "\t\c"
./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		diffmail 1

echo -e "\n"

echo -e "perl2\t\c"
./partb-average-speedup.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		splitmail

echo -e "\t\c"
./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		splitmail 0

echo -e "\t\c"
./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		splitmail 1

echo -e "\n"

echo -e "perl3\t\c"
./partb-average-speedup.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		perfect

echo -e "\t\c"
./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		perfect 0

echo -e "\t\c"
./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		perfect 1
echo -e "\n"


echo -e "perl4\t\c"
./partb-average-speedup.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		scrabbl

echo -e "\t\c"
./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		scrabbl 0

echo -e "\t\c"
./partb-average-speedup-aslr.sh perlbench-b3185-11.log \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		scrabbl 1
echo -e "\n"


echo -e "bzip1\t\c"
./partb-average-speedup.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		program

echo -e "\t\c"
./partb-average-speedup-aslr.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		program 0

echo -e "\t\c"
./partb-average-speedup-aslr.sh \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		program 1
echo -e "\n"


echo -e "bzip2\t\c"
./partb-average-speedup.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		byoudoin

echo -e "\t\c"
./partb-average-speedup-aslr.sh \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		byoudoin 0

echo -e "\t\c"
./partb-average-speedup-aslr.sh \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		byoudoin 1
echo -e "\n"


echo -e "bzip3\t\c"
./partb-average-speedup.sh \
		bzip2-b3185-01.log \
		bzip2-b3185-02.log \
		bzip2-b3185-03.log \
		bzip2-b3185-04.log \
		bzip2-b3185-05.log \
		combined

echo -e "\t\c"
./partb-average-speedup-aslr.sh \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		combined 0

echo -e "\t\c"
./partb-average-speedup-aslr.sh \
		perlbench-b3185-12.log \
		perlbench-b3185-13.log \
		perlbench-b3185-14.log \
		perlbench-b3185-15.log \
		combined 1
echo -e "\n"


