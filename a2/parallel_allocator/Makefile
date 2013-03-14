export TOPDIR:=$(CURDIR)

RESULTDIR:=$(TOPDIR)/benchmarks/results-$(shell date +%F--%X)
LINKNAME:=$(TOPDIR)/benchmarks/results.latest

our-tests:
	cd util && make
	cd allocators && make ours
	cd benchmarks/cache-scratch && make ours
	cd benchmarks/cache-thrash && make ours
	cd benchmarks/threadtest && make ours
	cd benchmarks/larson && make ours
	./benchmarks/runallours.pl $(TOPDIR)/benchmarks

program:
	cd util && make
	cd allocators && make
	cd benchmarks/cache-scratch && make
	cd benchmarks/cache-thrash && make
	cd benchmarks/threadtest && make
	cd benchmarks/larson && make

all-test: program
	./benchmarks/runall.pl $(TOPDIR)/benchmarks

runall-tests: program
	rm -f $(LINKNAME)
	mkdir -p $(RESULTDIR)
	./benchmarks/runall.pl $(TOPDIR)/benchmarks
	ln -s $(RESULTDIR) $(LINKNAME)
	mv benchmarks/cache-scratch/Results $(RESULTDIR)/cache-scratch
	mv benchmarks/cache-thrash/Results $(RESULTDIR)/cache-thrash
	mv benchmarks/threadtest/Results $(RESULTDIR)/threadtest
	mv benchmarks/larson/Results $(RESULTDIR)/larson

clean:
	cd util && make clean
	cd allocators && make clean
	cd benchmarks/cache-scratch && make clean
	cd benchmarks/cache-thrash && make clean
	cd benchmarks/threadtest && make clean
	cd benchmarks/larson && make clean

clean-results:
	rm -rfv benchmarks/*/Results
