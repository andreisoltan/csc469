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

#define ALIGN_UP_TO(p,X) ((void *)((((unsigned long)(p) + X - 1) / X ) * X))

// Empty fraction before superblock is released
#define SB_EMPTYFRAC ( 0.25 )

// Size of a superblock in pages
#define SB_PAGES 2

// Number of free superblocks before we release one
#define SB_RELTHRESHOLD 4

// Size classes
// TODO: try bringing this down to maybe 6 classes (256), see Miser
#define BLOCK_SZ_SM 8 // We'll use the "0 size" entry for empty superblocks
#define BLOCK_SZ_LG 1024
#define NSIZES 9
static const size_t size_classes[NSIZES] =
    { 0, 8, 16, 32, 64, 128, 256, 512, 1024 };

// Each superblock tracks a free list (LIFO) of available blocks
typedef struct free_list_s {
    struct free_list_s *next;
} free_list_t;

#define SB_HAS_FREE(sb_ptr) ( (sb_ptr)->free != NULL )
typedef struct superblock_s superblock_t;
struct superblock_s {
    //pthread_mutex_t lock;
    int usage;
    superblock_t *next;
    free_list_t *free;
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

#define GLOB_HEAP ( (heap_t*)dseg_lo )
#define CPU_HEAPS ( ((heap_t*)dseg_lo) + 1 )

superblock_t* sb_find_free(superblock_t **sb);
superblock_t* sb_get(int core, int sz);

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

    void *base = NULL;
    heap_t *heap = NULL;
    int i = 0;
    int n_cpu = 0;
    size_t alloc_size = 0;

    if (dseg_lo == NULL && dseg_hi == NULL) {
        // Initialize memory system
        mem_init();

        // Should we align this to a page boundary? I am doing alignment
        // to 64 to accomodate cache-lines
        n_cpu = getNumProcessors();
        alloc_size = (size_t) ALIGN_UP_TO(((n_cpu+1)*sizeof(heap_t)), 64);

        // Get enough memory to set up our own book-keeping structures.
        if ( (base = mem_sbrk(alloc_size)) ) {

            // Zero the heap structures
            bzero(base, alloc_size);

            // Initialize locks on all heaps
            heap =  GLOB_HEAP;
            for (i = 0; i < n_cpu+1; i++, heap++)
                pthread_mutex_init(&(heap->lock), NULL);

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

    pthread_mutex_lock(&(CPU_HEAPS[cpu].lock));

    // Found our size class (size_classes[class] >= size), get an
    // appropriate superblock
    if ((sb = sb_get(cpu, class)) == NULL) {
        fprintf(stderr, "Could not malloc %d bytes. Whoops.\n", size);
        exit(1);
    }

    // Take block from freelist
    ret = sb->free;

    // TODO: update accounting, rebalance
    sb->usage++;
    CPU_HEAPS[cpu].usage++;

    // Put the SB at the head of the list for this cpu/size-class
    sb->next = CPU_HEAPS[cpu].sbs[class];
    CPU_HEAPS[cpu].sbs[class] = sb;

    // Unlock and return
    pthread_mutex_unlock(&(CPU_HEAPS[cpu].lock));
    return ret;    
}

void mm_free (void *ptr) {
    // Make sure the pointer is from memory that we allocated. Should be
    // within [dseg_lo + ( (n_cores+1)*(sizeof(heap_t)) ), dseg_hi]
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
    char init_free = 0;
    superblock_t *result = NULL;

    if ((result = sb_find_free(&(CPU_HEAPS[core].sbs[sz]))) == NULL) {
        if ((result = sb_find_free(&(CPU_HEAPS[core].sbs[0]))) == NULL) {
            // Lock the global heap before going up there to look...
            pthread_mutex_lock(&(GLOB_HEAP->lock));
            if ((result = sb_find_free(&(GLOB_HEAP->sbs[sz]))) == NULL) {
                if ((result = sb_find_free(&(GLOB_HEAP->sbs[0]))) == NULL) {
                    // TODO: result = new sbrk'ed superblock
                    // TODO: usage = 0; next = NULL;
                }
                // If we are here, we either found an empty SB in the 
                // global heap or we mem_sbrk'ed a new SB. In either case
                // we'll need to initialize its freelist.
                init_free = 1;
            }
            // Done with the global heap
            pthread_mutex_unlock(&(GLOB_HEAP->lock));
        } else {
            // Found an empty SB, before returning initialize its freelist
            init_free = 1;
        }
    }

    if (init_free) {
        // TODO: init free list for size sz
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
            // This was not the first thing in the list
            prev->next = ret->next;
        }
        ret->next = NULL;
    }

    return ret;
}
