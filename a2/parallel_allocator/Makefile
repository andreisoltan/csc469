export TOPDIR = $(CURDIR)

program:
	cd util && make
	cd allocators && make
	cd benchmarks/cache-scratch && make
	cd benchmarks/cache-thrash && make
	cd benchmarks/threadtest && make
	cd benchmarks/larson && make

clean:
	cd util && make clean
	cd allocators && make clean
	cd benchmarks/cache-scratch && make clean
	cd benchmarks/cache-thrash && make clean
	cd benchmarks/threadtest && make clean
	cd benchmarks/larson && make clean

