/* $Id$ */

/*
 *  CSC 469 - Assignment 2
 *
 *  Hoard-like allocator.
 */

#define _GNU_SOURCE // see `man sched_getcpu'

#include <assert.h>
#include <pthread.h>
#include <sched.h> // for sched_getcpu()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "malloc.h"
#include "memlib.h"
#include "mm_thread.h"

#ifndef DEBUG
#define DEBUG 0
#endif

// This debug_print macro is borrowed from Jonathan Leffler, here:
// http://stackoverflow.com/questions/1644868/c-define-macro-for-debug-printing
#define debug_print(...) \
            do { if (DEBUG) fprintf(stderr, ##__VA_ARGS__); } while (0)

name_t myname = {
     /* team name to be displayed on webpage */
     "Ultra TeamSquad Alpha",
     /* Full name of first team member */
     "Anrei Soltan",
     /* Email address of first team member */
     "g0soltan@cdf.toronto.edu",
     /* Full name of second team member */
     "Jonathan Prindiville",
     /* Email address of second team member */
     "g2prindi@cdf.toronto.edu"
};

#define ALIGN_UP_TO(p,X)   ((void *)((((unsigned long)(p) + X - 1) / X ) * X))
#define ALIGN_DOWN_TO(p,X) ((void *)(((unsigned long)(p) / X) * X))

// Empty fraction before superblock is released
#define SB_EMPTYFRAC ( 0.25 )

// Size of a superblock in pages
#define SB_PAGES 2
//#define SB_PAGES 1

// Number of free superblocks before we release one
#define SB_RELTHRESHOLD 4

// Size classes
// TODO: try bringing this down to maybe 6 classes (256), see Miser
#define BLOCK_SZ_SM 8 // We'll use the "0 size" entry for empty superblocks
#define BLOCK_SZ_LG 1024
#define NSIZES 9
static const size_t size_classes[NSIZES] =
    { 0, 8, 16, 32, 64, 128, 256, 512, 1024 };
// We make the first superpage (the one with our bookkeeping stuff on it)
// of size class 1 arbitrarily, but with the hope that programs will use
// allocations of that size and the space after our allocators data
// structures will not be wasted.
#define FIRST_SB_CLASS 1

// Each superblock tracks a free list (LIFO) of available blocks
typedef struct freelist_s freelist_t;
struct freelist_s {
    freelist_t *next;
};

#define SB_HAS_FREE(sb_ptr) ( (sb_ptr)->free != NULL )
typedef struct superblock_s superblock_t;
struct superblock_s {
    //pthread_mutex_t lock;
    int usage;
    int heap;
    size_t size_class;
    superblock_t *next;
    freelist_t *free;
};

// For each CPU heap and the global heap we will need:
// - a lock
// - usage and allocated counts
// - list of superblocks belonging to that heap separated by block size
// (right now these are simple linked lists -- Hoard also maintains its
// superblocks in groups according to fullness)
typedef struct heap_s {
    pthread_mutex_t lock;
    int usage;
    int allocated;
    // We'll use the "0 size" entry for empty superblocks, 1 through NSIZES
    // will correspond to superblocks having the sizes corresponding to those
    // listed in the global var sizes, above.
    superblock_t *sbs[NSIZES];
} heap_t;

#define GLOB_HEAP ((heap_t*)(dseg_lo + sizeof(superblock_t)))
#define CPU_HEAPS (GLOB_HEAP + 1)

superblock_t* sb_find_free(superblock_t **sb);
superblock_t* sb_get(int core, int sz);
freelist_t* fl_init(void *first, void *limit, size_t sz);

size_t pagesize = 0;
size_t sb_size = 0;

/**
 * Do initial setup for memory allocator.
 *
 * Where N is the number of CPUs, the first (N+1)*sizeof(heap_t) bytes
 * of the heap will be treated as an array of heap_t. Index 0 being the
 * global heap data, and indices i=1..N belonging to the corresponding
 * CPUs. For convenience we've define the GLOB_HEAP and CPU_HEAPS macros
 * above.
 */
int mm_init (void) {

    int i = 0;
    int n_cpu = 0;
    size_t alloc_size = 0;
    void *base = NULL;
    heap_t *heap = NULL;
    superblock_t *sb = NULL;
    freelist_t *fl = NULL;

    debug_print("%20s|entry; sizeof(heap_t): %d, "
        "sizeof(superblock_t): %d\n", "mm_init", 
        sizeof(heap_t), sizeof(superblock_t));

    if (dseg_lo == NULL && dseg_hi == NULL) {

        //debug_print("%20s|calling mem_init...\n", "mem_init");

        // Initialize memory system
        mem_init();

        // Calculate some parmeters
        pagesize = mem_pagesize();
        sb_size = pagesize * SB_PAGES; 
        n_cpu = getNumProcessors();

        debug_print("%20s|pagesize: %d, sb_size: %d, n_cpu: %d\n",
            "mem_init", pagesize, sb_size, n_cpu);

        // initial sbrk for our allocator's book-keeping data
        // structures will be the size of a superblock. We do 
        // this in order to keep things in our chunk of memory aligned
        // with superblock-sized boundaries.
        //
        // Because we don't have 8k of allocator data structures, we
        // want to make the remainder of the superblock available to
        // callers. Therefore we will manage this initial superblock as
        // we will with other normal allocation requests, *dseg_lo will
        // have a superblock_t written on it.
        //
        // Once we know the size of the allocator's data structures, we
        // align that up to some sub-block size class and construct the
        // superblock's free list in the remaining space.
        //
        // The size class does not matter, but it should be some size that
        // will not be left unused (in which case the allocator's data 
        // structures will effectively be consuming 8k.)

        // We'll calculate the size of our bookkeeping data (# cores + 1
        // for the heaps, plus the superblock)
        alloc_size = sizeof(heap_t) * (n_cpu+1) + sizeof(superblock_t);

        debug_print("%20s|alloc_size: %d\n", "mm_init", alloc_size);

        // Get some memory (a superblock's worth)
        if ( (base = mem_sbrk(sb_size)) ) {

            debug_print("%20s|sb@%p created\n", "mm_init", base);

            // Zero the heap structures
            bzero(base, alloc_size);

            // Initialize locks on all heaps
            heap =  GLOB_HEAP;
            for (i = 0; i < n_cpu+1; i++, heap++)
                pthread_mutex_init(&(heap->lock), NULL);

            // Set up this superblock's header
            sb = (superblock_t *)base;
            sb->size_class = FIRST_SB_CLASS;
            //sb->next = NULL; // not necessary, we bzero'd this ^^
            //sb->usage = 0;  // not necessary, we bzero'd this ^^
            
            // Find where we can start laying down free list nodes after
            // the heap data:
            fl = (freelist_t *) ( ((unsigned long)base) +
                ((unsigned long) ALIGN_UP_TO(alloc_size, size_classes[FIRST_SB_CLASS])));

            // Build freelist and register in superblock header
            sb->free = fl_init(fl, dseg_hi, size_classes[FIRST_SB_CLASS]); 

            // Lastly, we'll associate this superblock with the global
            // heap
            GLOB_HEAP->sbs[FIRST_SB_CLASS] = sb;

        } else {
            fprintf(stderr, "Failed to initialize heap. Exiting.\n");
            exit(1);
        }
    }

    return 0;
}

void *mm_malloc (size_t size) {

    void *ret = NULL;
    superblock_t *sb = NULL;

    // Find our CPU, size class of request
    int cpu = sched_getcpu();
    int class = 0;

    while ((size_classes[class] < size) && (class < NSIZES))
        class++;

    // TODO: large allocations
    if (class > NSIZES) {
        // Larger than we track in our size classes
        fprintf(stderr, "TODO: large allocations\n");
        exit(1);
    }

    //debug_print("%20s|cpu: %d, size: %d, class: %d\n",
    //    "mm_malloc", cpu, size, size_classes[class]);

    pthread_mutex_lock(&(CPU_HEAPS[cpu].lock));

    // Found our size class (size_classes[class] >= size), get an
    // appropriate superblock
    if ((sb = sb_get(cpu, class)) == NULL) {
        fprintf(stderr, "Could not malloc %d bytes. Whoops.\n", size);
        exit(1);
    }

    // Take block from freelist
    ret = sb->free;
    sb->free = sb->free->next;

    // Unlock and return
    pthread_mutex_unlock(&(CPU_HEAPS[cpu].lock));
    
//    debug_print("%20s|%p\n", "mm_malloc", ret);
    
    return ret; 
}

// Frees the memory associated with ptr. If ptr was not a previously
// allocated memory segment returned by mm_malloc, or ptr was previously
// freed behaviour is undefined. 
void mm_free (void *ptr) {

    superblock_t *sb = NULL;

    // Make sure the pointer is from memory that we allocated
    if ( ((char*)ptr < dseg_lo) || ((char *)ptr > dseg_hi) )
        return;
    
    // TODO: If this is a large allocation deal with it separately...

    // General case:
    // Find nearest superblock alignment below *ptr
    //sb = ALIGN_DOWN_TO(ptr, sb_size);
    // We have to be careful about dseg_lo not superblock, but page aligned
    sb = (superblock_t*) (
        ( (char*) ALIGN_DOWN_TO(((char*)ptr - dseg_lo), sb_size) )
        + (unsigned long) dseg_lo );

//    debug_print("%20s|%p from sb@%p\n", "mm_free", ptr, sb);

    // Insert the freed block at the head of the list (LIFO)
    ((freelist_t*)ptr)->next = sb->free;
    sb->free = (freelist_t*)ptr;

    // Adjust statistics
    sb->usage -= size_classes[sb->size_class];
    GLOB_HEAP[sb->heap].usage -= size_classes[sb->size_class];

    // TODO: rebalance
}

// Returns a pointer to a superblock having free blocks on it in CPU #core's
// heap of size class sz. If none exists, we first check for an empty
// superblock on core's heap, then we check for a sz-class superblock on the
// global heap, then we check for an empty superblock on the global heap. If
// we have no success there we will request more memory from mem_sbrk.
//
// If the superblock we find is not presently in core's heap in the list of
// sz-class superblocks (for instance if we use an empty superblock, or one
// from the global heap), sb_get will move the superblock into the appropriate
// list
superblock_t* sb_get(int core, int sz) {

    char from_global = 0;   // moving superblock from global heap 
    char new_sb = 0;        // newly allocated superblock
    char new_or_empty = 0;  // need to build superblock's freelist
    superblock_t *result = NULL;

    if ((result = sb_find_free(&(CPU_HEAPS[core].sbs[sz]))) == NULL) {
        if ((result = sb_find_free(&(CPU_HEAPS[core].sbs[0]))) == NULL) {
            // We didn't find anything suitable on our own heap, will get
            // from the global, or a new one. In either of those cases
            // Lock the global heap before going up there to look...
            // TODO: Hoard doesn't seem to lock the global heap, why?!
            pthread_mutex_lock(&(GLOB_HEAP->lock));
            if ((result = sb_find_free(&(GLOB_HEAP->sbs[sz]))) == NULL) {
                if ((result = sb_find_free(&(GLOB_HEAP->sbs[0]))) == NULL) {
                    // Nothing available, make new space...
                    new_sb = 1;
                    result = (superblock_t *) mem_sbrk(sb_size);
                    bzero(result, sizeof(superblock_t)); 
                    debug_print("%20s|sb@%p created (core %d, sz %d)\n",
                        "sb_get", result, core, size_classes[sz]);
                } else {
                    from_global = 1;
                }
                // Found an empty SB in the global heap or we mem_sbrk'ed a
                // new SB, we'll need to initialize its freelist.
                new_or_empty = 1;
            } else {
                // Found a non-empty SB in global
                from_global = 1;
            }
            // TODO: Hoard doesn't seem to lock the global heap, why?!
            // Done with the global heap
            pthread_mutex_unlock(&(GLOB_HEAP->lock));
        } else {
            // Found an empty SB in this core's heap, must init its freelist
            new_or_empty = 1;
        }
    } else {
        // We found a non-empty SB in this core's heap
    }

    // Put the SB at the head of the list for this cpu/size-class
    result->next = CPU_HEAPS[core].sbs[sz];
    CPU_HEAPS[core].sbs[sz] = result;

    // Adjust core's heap stats if necessary
    if (new_sb) {
        result->heap = core + 1;
        CPU_HEAPS[core].allocated += sb_size;
        //new_or_empty = 1; // redundant
    }

    // Adjust global/core stats if necessary
    if (from_global) {
        result->heap = core + 1;
        GLOB_HEAP->allocated -= sb_size;
        GLOB_HEAP->usage -= result->usage;
        CPU_HEAPS[core].allocated += sb_size;
    //    CPU_HEAPS[core].usage += result->usage;
    }

    // Increment usage counters
    result->usage += size_classes[sz];
    CPU_HEAPS[core].usage += size_classes[sz];

    // If we found a new or empty SB, init its freelist, assign size class
    if (new_or_empty) {
        result->free = fl_init(
            ALIGN_UP_TO((((unsigned long)result) + sizeof(superblock_t)), size_classes[sz]),
            (void*) (((unsigned long)result) + sb_size),
            size_classes[sz]);
        result->size_class = sz;
    }

    // Now we are guaranteed to have a superblock with free space pointed
    // at by result.
    return result;
}

// Looks through the superblock list starting at *sb for one having free space
// Upon finding such a superblock, removes it from the list and returns a
// pointer to it. If none is found, return NULL.
//
// ASSUMES THAT ANY LOCKS GOVERNING THE SUPERBLOCK IN QUESTION ARE ALREADY HELD
//
// SUPERBLOCKS RETURNED BY THIS FUNCTION ARE NOT IN ANY HEAP'S LIST, YOU
// HAVE TO PUT THEM SOMEWHERE.
superblock_t* sb_find_free(superblock_t **sb) {
    
    superblock_t *prev = NULL;
    superblock_t *ret = *sb;

    while( (ret != NULL) && ( ! SB_HAS_FREE(ret)) ) {
        prev = ret;
        ret = ret->next;
    }

    if (ret != NULL) {
        // Found something with free blocks
        if (prev == NULL) {
            // This was the first thing on the list, update that head pointer,
            // take the thing we're returning out of the list.
            *sb = ret->next;
        } else {
            // This was not the first thing in the list, patch over it.
            prev->next = ret->next;
        }
        ret->next = NULL;
    }

    return ret;
}

// Starting at *first, fl_init() writes out freelist_t nodes in a linked list
// spaced by sz bytes, stopping at *limit. Returns *first upon success.
freelist_t* fl_init(void *first, void *limit, size_t sz) {

#ifdef DEBUG
    int count = 0;
#endif

    freelist_t *fl = (freelist_t *) first;
    unsigned long next_fl = ((unsigned long) fl) + sz;

    // Walk through the rest of the superblock in increments
    // equal to the chunk size while building the free list
    while (next_fl < (unsigned long)limit) {
#ifdef DEBUG
        count++;
#endif
        fl->next = (freelist_t*) next_fl;
        fl = (freelist_t*) next_fl;
        next_fl += sz;
    }

    // The last freelist node should point nowhere
    fl->next = NULL;

    debug_print("%20s|%p .. %p: %d @ %d\n", "fl_init", first, limit, count, sz);

    return fl;

}
