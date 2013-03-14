
INCLUDES = $(TOPDIR)/include
LIBDIR = $(TOPDIR)/util
LIBS = -lmmutil -lpthread
LIBS_DBG = -lmmutil_dbg -lpthread
DEPENDS = threadtest.c $(LIBDIR)/libmmutil.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h
DEPENDS_DBG = threadtest.c $(LIBDIR)/libmmutil_dbg.a $(INCLUDES)/tsc.h $(INCLUDES)/mm_thread.h $(INCLUDES)/timer.h

CC = gcc
CC_FLAGS = -O3 -I$(INCLUDES) -L $(LIBDIR)
CC_DBG_FLAGS = -g -I$(INCLUDES) -L $(LIBDIR)

all: threadtest-kheap threadtest-cmu threadtest-libc threadtest-amalloc threadtest-amalloc-dbg

ours: threadtest-amalloc threadtest-amalloc-dbg

# Our first Hoard-like
threadtest-amalloc: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libamalloc.a
	$(CC) $(CC_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libamalloc.a -lmmutil -lpthread

threadtest-amalloc-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libamalloc_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libamalloc_dbg.a -lmmutil_dbg -lpthread


# Allocator based on OS/161 kheap

threadtest-kheap: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libkheap.a
	$(CC) $(CC_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libkheap.a -lmmutil -lpthread

threadtest-kheap-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libkheap_dbg.a -lmmutil_dbg -lpthread

#Allocator based on CMU CS213 malloc/free

threadtest-cmu: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libcmumalloc.a
	$(CC) $(CC_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libcmumalloc.a -lmmutil -lpthread

threadtest-cmu-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a 
	$(CC) $(CC_DBG_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libcmumalloc_dbg.a -lmmutil_dbg -lpthread



# Allocator using libc malloc/free inside mm_malloc/mm_free wrappers

threadtest-libc: $(DEPENDS) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil -lpthread

threadtest-libc-dbg: $(DEPENDS_DBG) $(TOPDIR)/allocators/alloclibs/libmmlibc.a
	$(CC) $(CC_DBG_FLAGS) -o $(@) threadtest.c $(TOPDIR)/allocators/alloclibs/libmmlibc.a -lmmutil_dbg -lpthread

# Cleanup
clean:
	rm -f threadtest-* *~
