CC=/usr/bin/gcc
CFLAGS=-DSPEC_CPU -DNDEBUG -DSPEC_CPU_LINUX_IA32
SRCS=$(wildcard *.c)
OBJS-O2=$(patsubst %.c,%-O2.o,$(wildcard *.c))
OBJS-O3=$(patsubst %.c,%-O3.o,$(wildcard *.c))

all:	lbm-O2 lbm-O3

# Optimize with -O2
%-O2.o:	%.c
	$(CC) -c $(CFLAGS) -O2 $< -o $@

lbm-O2:	$(OBJS-O2)
	$(CC) -o $@ $(OBJS-O2) -lm


# Optimize with -O3
%-O3.o:	%.c
	$(CC) -c $(CFLAGS) -O3 $< -o $@

lbm-O3:	$(OBJS-O3)
	$(CC) -o $@ $(OBJS-O3) -lm

.PHONY: clean fullclean

clean:
	rm -f *.o lbm lbm-O2 lbm-O3

fullclean:
	rm -f *.o *~ lbm lbm-O2 lbm-O3
