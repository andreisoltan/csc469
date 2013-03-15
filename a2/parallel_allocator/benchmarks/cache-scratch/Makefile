
INCLUDES = $(TOPDIR)/include
LIBDIR = $(TOPDIR)/util
LIBS = -lmmutil -lpthread
LIBS_DBG = -lmmutil_dbg -lpthread
DEPENDS = cache-scratch.c $(LIBDIR)/libmmutil.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h
DEPENDS_DBG = cache-scratch.c $(LIBDIR)/libmmutil_dbg.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h

CC = gcc
CC_FLAGS = -O3 -I$(INCLUDES) -L $(LIBDIR)
CC_DBG_FLAGS = -g -I$(INCLUDES) -L $(LIBDIR)

all: cache-scratch-kheap cache-scratch-cmu cache-scratch-libc cache-scratch-amalloc cache-scratch-amalloc-dbg

ours: cache-scratch-amalloc cache-scratch-amalloc-dbg

# Allocator based on our Hoard-like

cache-scratch-amalloc: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libamalloc.a
	$(CC) $(CC_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libamalloc.a -lmmutil -lpthread

cache-scratch-amalloc-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libamalloc_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libamalloc_dbg.a -lmmutil_dbg -lpthread


# Allocator based on OS/161 kheap

cache-scratch-kheap: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libkheap.a
	$(CC) $(CC_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libkheap.a -lmmutil -lpthread

cache-scratch-kheap-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a -lmmutil_dbg -lpthread

#Allocator based on CMU CS213 malloc/free

cache-scratch-cmu: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libcmumalloc.a
	$(CC) $(CC_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libcmumalloc.a -lmmutil -lpthread

cache-scratch-cmu-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a -lmmutil_dbg -lpthread



# Allocator using libc malloc/free inside mm_malloc/mm_free wrappers

cache-scratch-libc: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil -lpthread

cache-scratch-libc-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) cache-scratch.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil_dbg -lpthread

# Cleanup
clean:
	rm -f cache-scratch-* *~
