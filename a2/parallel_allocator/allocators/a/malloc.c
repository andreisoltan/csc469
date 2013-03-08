/* $Id$ */

/*
 *  CSC 469 - Assignment 2
 *
 *  Hoard-like allocator.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "memlib.h"
#include "malloc.h"

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
static const size_t sizes[NSIZES] = { 0, 8, 16, 32, 64, 128, 256, 512, 1024 };

// Each superblock tracks a free list (LIFO) of available blocks
typedef struct free_list_s {
    struct free_list_s *next;
} free_list_t;

typedef struct superblock_s {
    // Do we need to track usage/allocated per superblock?
    struct superblock_s *next;
    free_list_t *free;
} superblock_t;

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
    const superblock_t *sbs[NSIZES];
} heap_t;

#define GLOB_HEAP ( (*heap_t)(dseg_lo) )
#define CPU_HEAPS ( ((*heap_t)(dseg_lo)) + 1 )


/**
 * Do initial setup for memory allocator.
 *
 * Where N is the number of CPUs, the first (N+1)*sizeof(heap_s) bytes
 * of the heap will be treated as an array of heap_t. Index 0 being the
 * global heap data, and indices i=1..N belonging to the corresponding
 * CPUs. For convenience we've define the GLOB_HEAP and CPU_HEAPS macros
 * above.
 */
int mm_init (void) {

    void *base = NULL;
    heap_t *heap = NULL;
    int n_cpu = 0;
    int alloc_size = 0;

    if (dseg_lo == NULL && dseg_hi == NULL) {
        // Initialize memory system
        mem_init();

        n_cpu = getNumProcessors();
        alloc_size = (n_cpu+1)*sizeof(heap_s);

        // Get enough memory to set up our own book-keeping structures.
        if (base = mem_sbrk(alloc_size)) {

            // Zero the heap structures
            bzero(base, alloc_size);

            // Initialize locks on all heaps
            heap = GLOB_HEAP;
            for (int i = 0; i < n_cpu+1; i++, heap++)
                heap->lock = PTHREAD_MUTEX_INITIALIZER;

        } else {
            fprintf(stderr, "Failed to initialize heap. Exiting.\n");
            exit(1);
        }
    }

    return 0;
}

void *mm_malloc (size_t size) {
    // Find: our CPU, size class of request
    // Acquire CPU_HEAPS[x].lock
    // Search CPU_HEAPS[x].sbs[sz]
    // if not found:
    //      Search CPU_HEAPS[x].sbs[0]
    //      if not found:
    //          Acquire GLOB_HEAP.lock
    //          Search GLOB_HEAP.sbs[sz]
    //          if not found:
    //              Search GLOB_HEAP.sbs[0]
    //              if not found:
    //                  mem_sbrk a new sb
    //              Release GLOB_HEAP.lock
    // Take block from freelist (update accounting)
    // Release CPU_HEAPS[x].lock
    // Return
}

void mm_free (void *ptr) {
    // Make sure the pointer is from memory that we allocated.
}

