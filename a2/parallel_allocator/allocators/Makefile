CC = gcc

CC_FLAGS = -c -Wall -fmessage-length=0 -pipe -O3 -finline-limit=65000 -fkeep-inline-functions -finline-functions -ffast-math -fomit-frame-pointer -DNDEBUG -I. -I$(TOPDIR)/include -D_REENTRANT=1

CC_DBG_FLAGS = -c -Wall -fmessage-length=0 -pipe -g -finline-limit=65000 -fkeep-inline-functions -finline-functions -ffast-math -fomit-frame-pointer -DDEBUG -DNDEBUG -I. -I$(TOPDIR)/include -D_REENTRANT=1

all: libkheap libcmumalloc libmmlibc libamalloc libamalloc_dbg amalloc-variants

ours: libmmlibc libamalloc libamalloc_dbg

alloclibs:
	mkdir alloclibs

# Library containing our first Hoard-like allocator
libamalloc: alloclibs
	cd a; $(CC) $(CC_FLAGS) malloc.c; ar rs ../alloclibs/libamalloc.a malloc.o
	cp alloclibs/libamalloc.a alloclibs/libamalloc-reg.a

libamalloc_dbg: alloclibs
	cd a; $(CC) $(CC_DBG_FLAGS) malloc.c; ar rs ../alloclibs/libamalloc_dbg.a malloc.o

###############################################################################
# Variants of our allocator ###################################################

amalloc-variants: libamalloc libamalloc-lowfrac libamalloc-lowthresh libamalloc-smsb libamalloc-smblock libamalloc-lowfrac-lowthresh libamalloc-lowfrac-smsb libamalloc-lowfrac-smsb libamalloc-lowfrac-smblock libamalloc-lowthresh-smsb libamalloc-lowthresh-smblock libamalloc-smsb-smblock libamalloc-lowfrac-lowthresh-smsb libamalloc-lowfrac-lowthresh-smblock libamalloc-lowthresh-smsb-smblock libamalloc-lowfrac-lowthresh-smsb-smblock

## 1 ##########################################################################
libamalloc-lowfrac: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_FRAC malloc.c; ar rs ../alloclibs/libamalloc-lowfrac.a malloc.o

libamalloc-lowthresh: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_THRESH malloc.c; ar rs ../alloclibs/libamalloc-lowthresh.a malloc.o

libamalloc-smsb: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_SMALL_SB malloc.c; ar rs ../alloclibs/libamalloc-smsb.a malloc.o

libamalloc-smblock: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_SMALL_BLOCK_LIMIT malloc.c; ar rs ../alloclibs/libamalloc-smblock.a malloc.o

## 2 ##########################################################################
libamalloc-lowfrac-lowthresh: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_FRAC -DOPT_LOW_THRESH malloc.c; ar rs ../alloclibs/libamalloc-lowfrac-lowthresh.a malloc.o

libamalloc-lowfrac-smsb: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_FRAC -DOPT_SMALL_SB malloc.c; ar rs ../alloclibs/libamalloc-lowfrac-smsb.a malloc.o

libamalloc-lowfrac-smblock: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_FRAC -DOPT_SMALL_BLOCK_LIMIT malloc.c; ar rs ../alloclibs/libamalloc-lowfrac-smblock.a malloc.o

libamalloc-lowthresh-smsb: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_THRESH -DOPT_SMALL_SB malloc.c; ar rs ../alloclibs/libamalloc-lowthresh-smsb.a malloc.o

libamalloc-lowthresh-smblock: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_THRESH -DOPT_SMALL_BLOCK_LIMIT malloc.c; ar rs ../alloclibs/libamalloc-lowthresh-smblock.a malloc.o

libamalloc-smsb-smblock: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_SMALL_SB -DOPT_SMALL_BLOCK_LIMIT malloc.c; ar rs ../alloclibs/libamalloc-smsb-smblock.a malloc.o

## 3 ##########################################################################
libamalloc-lowfrac-lowthresh-smsb: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_FRAC -DOPT_LOW_THRESH -DOPT_SMALL_SB malloc.c; ar rs ../alloclibs/libamalloc-lowfrac-lowthresh-smsb.a malloc.o

libamalloc-lowfrac-lowthresh-smblock: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_FRAC -DOPT_LOW_THRESH -DOPT_SMALL_BLOCK_LIMIT malloc.c; ar rs ../alloclibs/libamalloc-lowfrac-lowthresh-smblock.a malloc.o

libamalloc-lowthresh-smsb-smblock: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_THRESH -DOPT_SMALL_SB -DOPT_SMALL_BLOCK_LIMIT malloc.c; ar rs ../alloclibs/libamalloc-lowthresh-smsb-smblock.a malloc.o

## 4 ##########################################################################
libamalloc-lowfrac-lowthresh-smsb-smblock: alloclibs
	cd a; $(CC) $(CC_FLAGS) -DOPT_LOW_FRAC -DOPT_LOW_THRESH -DOPT_SMALL_SB -DOPT_SMALL_BLOCK_LIMIT malloc.c; ar rs ../alloclibs/libamalloc-lowfrac-lowthresh-smsb-smblock.a malloc.o

## End of variants ############################################################


# Library containing mm_malloc and mm_free for OS/161 kheap-based allocator

libkheap: alloclibs
	cd kheap; $(CC) $(CC_FLAGS) kheap.c; ar rs ../alloclibs/libkheap.a kheap.o

libkheap_dbg: alloclibs
	cd kheap; $(CC) $(CC_DBG_FLAGS) kheap.c; ar rs ../alloclibs/libkheap_dbg.a kheap.o 

# Library containing mm_malloc and mm_free for CMU CS213 malloc

libcmumalloc:	alloclibs cmu_malloc/malloc.c
	cd cmu_malloc; $(CC) $(CC_FLAGS) malloc.c; ar rs ../alloclibs/libcmumalloc.a malloc.o

libcmumalloc_dbg:	alloclibs cmu_malloc/malloc.c
	cd cmu_malloc; $(CC) $(CC_DBG_FLAGS) malloc.c; ar rs ../alloclibs/libcmumalloc_dbg.a malloc.o


# Library containing mm_malloc and mm_free wrappers for libc allocator
libmmlibc:	alloclibs
	cd libc; $(CC) $(CC_FLAGS) libc_wrapper.c; ar rs ../alloclibs/libmmlibc.a libc_wrapper.o

clean:
	rm -rf alloclibs; rm -f */*.o; rm -f *~
