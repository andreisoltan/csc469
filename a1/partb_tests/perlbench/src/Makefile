CC=/usr/bin/gcc
CFLAGS=-DSPEC_CPU -DNDEBUG -DPERL_CORE -DSPEC_CPU_LINUX_IA32
SRCS=av.c deb.c doio.c doop.c dump.c globals.c gv.c hv.c locale.c mg.c numeric.c \
	op.c pad.c perl.c perlapi.c perlio.c perlmain.c perly.c pp.c pp_ctl.c \
	pp_hot.c pp_pack.c pp_sort.c pp_sys.c regcomp.c regexec.c run.c \
	scope.c sv.c taint.c toke.c universal.c utf8.c util.c xsutils.c  \
	Base64.c Cwd.c Dumper.c  HiRes.c  IO.c Peek.c attrs.c poll.c stdio.c \
	DynaLoader.c MD5.c Storable.c Parser.c specrand.c Hostname.c Opcode.c

OBJS-O2=$(patsubst %.c,%-O2.o,$(SRCS))
OBJS-O3=$(patsubst %.c,%-O3.o,$(SRCS))

all:	perlbench-O2 perlbench-O3

# Optimize with -O2
%-O2.o:	%.c
	$(CC) -c $(CFLAGS) -O2 $< -o $@

perlbench-O2:	$(OBJS-O2)
	$(CC) -o $@ $(OBJS-O2) -lm


# Optimize with -O3
%-O3.o:	%.c
	$(CC) -c $(CFLAGS) -O3 $< -o $@

perlbench-O3:	$(OBJS-O3)
	$(CC) -o $@ $(OBJS-O3) -lm

.PHONY: clean fullclean

clean:
	rm -f *.o perlbench perlbench-O2 perlbench-O3

fullclean:
	rm -f *.o *~ perlbench perlbench-O2 perlbench-O3
