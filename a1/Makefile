COMPILE=gcc -c
LINK=gcc -o

build: parta

parta: parta.o tsc.o
	$(LINK) $@ parta.o tsc.o

parta.o: parta.c tsc.h
	$(COMPILE) parta.c
	
tsc.o: tsc.c tsc.h
	$(COMPILE) tsc.c

submit: a1.tgz
	echo "\nTo submit, execute 'submit -c csc469h -a a1 a1.tgz'"

# Submission tarball
a1.tgz: docs build
	tar -czvf $@ report.pdf parta.c run_experiment_A

# Add gnuplot generation...
docs: report.pdf

report.pdf: report.tex
	latex report.tex
	latex report.tex
	dvips -o report.ps report.dvi
	ps2pdf report.ps

# Add cleanup for program compilation...
clean:
	rm -rf report.aux report.dvi report.log report.pdf report.ps report.toc a1.tgz *.o parta
