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

test-submit: submit
	rm -rf testsub
	mkdir testsub
	cp a1.tgz testsub
	tar -C testsub -xzf a1.tgz
	rm -f a1.tgz
	mv testsub/report.pdf testsub/original-report.pdf
#	make -C testsub submit

# Submission tarball
a1.tgz: run
	ln -fs log/parta.latest/report.pdf
	tar -hczvf $@ \
	  README Makefile \
	  active-inactive.awk minions.sh \
	  count-users.sh find-empty-wkstn.sh gather-platform-info.sh platform-info.sh \
	  tsc.c tsc.h \
	  parta.c \
	  do-parta.sh run_experiment_A  \
	  plot-parta.awk \
	  report.tex report.pdf
	rm -f report.pdf

run: build
	./run_experiment_A

# Add cleanup for program compilation...
clean:
	rm -rf *.aux *.dvi *.log *.pdf *.ps *.toc a1.tgz *.o parta
