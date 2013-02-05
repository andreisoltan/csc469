submit: a1.tgz
	echo "\nTo submit, execute 'submit -c csc469h -a a1 a1.tgz'"

build: parta.o

parta.o: parta.c
	gcc -o parta.o parta.c

# Add other files required for submission...
a1.tgz: docs build
	tar -czvf a1.tgz report.pdf parta.c run_experiment_A

# Add gnuplot generation...
docs: report.pdf

report.pdf: report.tex
	latex report.tex
	latex report.tex
	dvips -o report.ps report.dvi
	ps2pdf report.ps

# Add cleanup for program compilation...
clean:
	rm -rf report.aux report.dvi report.log report.pdf report.ps report.toc a1.tgz parta.o
