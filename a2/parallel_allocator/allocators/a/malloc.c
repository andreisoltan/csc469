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


// This //debug_print macro is borrowed from Jonathan Leffler, here:
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
#ifdef OPT_LOW_FRAC
    #define SB_EMPTYFRAC ( 0.10 )
#else
    #define SB_EMPTYFRAC ( 0.25 )
#endif

// Size of a superblock in pages
#ifdef OPT_SMALL_SB
    #define SB_PAGES 1
#else
    #define SB_PAGES 2
#endif

// Number of free superblocks before we release one
#ifdef OPT_LOW_THRESH
    #define SB_RELTHRESHOLD 4
#else
    #define SB_RELTHRESHOLD 4
#endif

// Size classes
// TODO: try bringing this down to maybe 6 classes (256), see Miser
#define BLOCK_SZ_SM 8
#ifdef OPT_SM_BLOCK_LIMIT
    #define BLOCK_SZ_LG 1024
    #define NSIZES 9
#else
    #define BLOCK_SZ_LG 4096
    #define NSIZES 11
#endif
#define MAX_NSIZES 11
// We'll use the "0 size" entry for empty superblocks
static const size_t size_classes[MAX_NSIZES] = { 0, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

// We make the first superpage (the one with our bookkeeping stuff on it)
// of size class 1 arbitrarily, but with the hope that programs will use
// allocations of that size and the space after our allocators data
// structures will not be wasted.
#define FIRST_SB_CLASS 1

/* A ll_node contains one of the links of the linked list. */
typedef struct _link {
    void * data;
    unsigned long size;
    struct _link * prev;
    struct _link * next;
} ll_node;

typedef struct _link megablock_t;

/* linked_list_t contains a linked list. */
typedef struct linked_list
{
    ll_node * first;
    ll_node * last;
}
linked_list_t;

// Each superblock tracks a free list (LIFO) of available blocks
typedef struct freelist_s freelist_t;
struct freelist_s {
    freelist_t *next;
};

#define SB_HAS_FREE(sb_ptr) ( (sb_ptr)->free != NULL )
typedef struct superblock_s superblock_t;
struct superblock_s {
    pthread_mutex_t lock;
    int usage;
    int heap;
    size_t size_class;
    superblock_t *prev;
    superblock_t *next;
    superblock_t **group;
    freelist_t *free;
};

// For each size class, we segregate the superblocks into
// fullness groups.
typedef struct sizeclass_s sizeclass_t;
struct sizeclass_s {
    superblock_t *full;    // usage == sb_size
    superblock_t *fullish; // usage / sb_size > SB_EMPTYFRAC
    superblock_t *emptyish;// usage / sb_size <= SB_EMPTYFRAC
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
    size_t allocated;
    // We'll use the "0 size" entry for empty superblocks, 1 through NSIZES
    // will correspond to superblocks having the sizes corresponding to those
    // listed in the global var sizes, above.
    sizeclass_t sizes[NSIZES];
} heap_t;


#define HEAPS ((heap_t*)(sizeof(superblock_t)+dseg_lo))

// Forward function declarations
superblock_t* sb_find_free(sizeclass_t *class);
superblock_t* _sb_find_free(superblock_t **sb);
superblock_t* sb_get(int core, int sz);
superblock_t* sb_find_victim(int heap);
freelist_t* fl_init(void *first, void *limit, size_t sz);
void sb_insert(superblock_t *sb, int heap);
void *mm_sbrk (ptrdiff_t increment);
megablock_t * sb_get_large(size_t size);
void sb_free_large(linked_list_t * list, void * data);

// Linked list functions forward declarations
void linked_list_init (linked_list_t * list);
void linked_list_add (linked_list_t * list, ll_node * link);
void linked_list_delete (linked_list_t * list, ll_node * link);
void linked_list_traverse (linked_list_t * list, void (*callback) (void *));
megablock_t * linked_list_find (linked_list_t * list, void *data);
void linked_list_traverse_delete (linked_list_t * list, void * data);


size_t pagesize = 0;
size_t sb_size = 0;
pthread_mutexattr_t mutexattr;
pthread_mutex_t sbrk_lock;

linked_list_t large_list;
pthread_mutex_t large_list_lock;

#ifdef DEBUG
int mallocs = 0;
int frees = 0;
#endif

// debugging functions ////////////////////////////////////////////////
#ifdef DEBUG
void dbg_print_freelist(superblock_t *sb) {
    freelist_t *fl;
    int steps = sb_size / BLOCK_SZ_SM;

    fl = sb->free;
    //debug_print("%20s|sb@%p->free = ", __func__, sb);
    while ((fl) && (steps > 0)) {
        steps--;
        //debug_print("%p -> ", fl);
        fl = fl->next;
    }
}

void dbg_print_heap_info(const char *who) {
    int cores = 0, i = 0;
    cores = getNumProcessors();
    for (i = 0; i <= cores; i++) {
        //debug_print("%20s|heap%d %d/%d == %f\n",
        //    who, i, HEAPS[i].usage, HEAPS[i].allocated,
        //    1.0 * HEAPS[i].usage / HEAPS[i].allocated);
    }
}

/*
void dbg_print_heap_details(int heap_num) {
    int i = 0;
    //freelist_t *fl = NULL;
    superblock_t *sb = (superblock_t *) &(HEAPS[heap_num]);

    //debug_print("heap %d (usage %d)\n\n"
    //    "%15s %15s %15s %15s\n",
    //    heap_num, sb->usage,
    //    "sb_addr", "sz_class", "usage", "fl_len");

    while (sb != NULL) {
        //fl = sb->free;
        //while (fl) {
        //    i++;
        //    fl = fl->next;
        //}

        //debug_print("%15p %15d %15d %15d\n",
        //    sb, sb->size_class, sb->usage, i);
        
        sb = sb->next;
    }

}
*/
#endif
// end debug functions ////////////////////////////////////////////////
 
/**
 * Do initial setup for memory allocator.
 *
 * Where N is the number of CPUs, the first (N+1)*sizeof(heap_t) bytes
 * of the heap will be treated as an array of heap_t. Index 0 being the
 * global heap data, and indices i=1..N belonging to the corresponding
 * CPUs.
 */
int mm_init (void) {

    int i = 0;
    int n_cpu = 0;
    size_t alloc_size = 0;
    void *base = NULL;
    heap_t *heap = NULL;
    superblock_t *sb = NULL;
    freelist_t *fl = NULL;

    //debug_print("\n%20s|entry; sizeof(heap_t): %d, sizeof(superblock_t): %d\n",
    //    __func__, sizeof(heap_t), sizeof(superblock_t));

    if (dseg_lo == NULL && dseg_hi == NULL) {

        pthread_mutexattr_init(&mutexattr);
#ifdef DEBUG
       pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);
#endif

       ////debug_print("%20s|calling mem_init...\n", __func__);

        // Initialize memory system
        mem_init();

        // Calculate some parameters
        pagesize = mem_pagesize();
        sb_size = pagesize * SB_PAGES; 
        n_cpu = getNumProcessors();

		// Initialize the large list
		linked_list_init(&large_list);
		pthread_mutex_init(&large_list_lock, &mutexattr);

        debug_print("%20s|##### Initialized large_list\n", __func__);

        ////debug_print("%20s|pagesize: %d, sb_size: %d, n_cpu: %d\n",
        //    who, pagesize, sb_size, n_cpu);

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

        ////debug_print("%20s|alloc_size: %d\n", who, alloc_size);
        pthread_mutex_init(&(sbrk_lock), &mutexattr);
        // Get some memory (a superblock's worth)
        if ( (base = mm_sbrk(sb_size)) ) {

            ////debug_print("%20s|sb@%p created, size %d\n",
            //    __func__, base, size_classes[FIRST_SB_CLASS]);

            // Zero the heap structures
            bzero(base, alloc_size);

            // Initialize locks and large lists on all heaps
            heap = HEAPS;
            for (i = 0; i < n_cpu+1; i++, heap++) {
                debug_print("%20s|&HEAPS[%d].usage = %p\n", __func__, i, &(heap->usage));
                pthread_mutex_init(&(heap->lock), &mutexattr);

            }

            // Set up this superblock's header
            sb = (superblock_t *)base;
            sb->size_class = FIRST_SB_CLASS;
            pthread_mutex_init(&(sb->lock), &mutexattr);
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
            sb_insert(sb, 0);
            HEAPS[0].allocated = sb_size;
            
            // Not necessary, we consider that space to be overhead, not
            // usage by allocation:
            //HEAPS[0].usage = sizeof(heap_t) * (n_cpu+1);

            //dbg_print_heap_info(who);

        } else {
            fprintf(stderr, "Failed to initialize heap. Exiting.\n");
            exit(1);
        }
    }

    return 0;
}

// Inserts the superblock into the appropriate size class and fullness
// group of the given heap.
void sb_insert(superblock_t *sb, int heap) {
    
    float frac; // how full are we?
    superblock_t **group; // which fullness group to use?

#ifdef DEBUG
    freelist_t *fl;
    int steps = sb_size / BLOCK_SZ_SM;
#endif        
    if (sb) {

        // Figure out the fullness group
        if (sb->usage == sb_size) {
            group = &(HEAPS[heap].sizes[sb->size_class].full);
        } else {
            frac = (1.0 * sb->usage) / sb_size;
            if (frac > SB_EMPTYFRAC) {
                //debug_print("%20s|sb@%p -> heap%d fullish (frac%f)\n",
                //    __func__, sb, heap, frac);
                group = &(HEAPS[heap].sizes[sb->size_class].fullish);
            } else {
                //debug_print("%20s|sb@%p -> heap%d emptyish (frac%f)\n",
                //    __func__, sb, heap, frac);
                group = &(HEAPS[heap].sizes[sb->size_class].emptyish);
            }
        }

        // Insert at head of fullness group
        if (*group)
            (*group)->prev = sb;
        sb->prev = NULL;
        sb->next = *group;
        sb->group = group;
        *group = sb;
    
#ifdef DEBUG
        //Check that the freelist is sane
        fl = sb->free;
        while ((fl) && steps > 0) {
            steps--;

            if (((char*)fl < dseg_lo) || ((char*)fl > dseg_hi)) {
                debug_print("%20s|wtf? fl entry out of bounds: %p\n",
                    __func__, fl);
            }
            fl = fl->next;
        }
        //if (steps == 0)
            //debug_print("%20s|cycle in sb@%p's freelist\n", __func__, sb);
#endif        
    }
}

void *mm_malloc (size_t size) {
    //const char* who = "mm_malloc";
    void *ret = NULL;
    superblock_t *sb = NULL;


    // Find our CPU, size class of request
    int cpu = sched_getcpu();
    int class = 0;


#ifdef DEBUG
    mallocs++;
#endif


    while ((size_classes[class] < size) && (class < NSIZES))
        class++;

	debug_print("%20s|##### class size %d\n", __func__, class);

    // Handle large allocations
    if (class == NSIZES) {
        // The requestes size is larger than we track in our size classes

        // Get the large block
        megablock_t* large_block = sb_get_large(size);

		// Handle allocation errors (out of memory)
		if(large_block == NULL) {
        	fprintf(stderr, "Could not malloc %d bytes. Whoops.\n", size);
	        exit(1);
		}

	    //debug_print("%20s|sb@%p|heap%d usage%d class%d\n",
	    //    __func__, sb, sb->heap, sb->usage, sb->size_class);

		pthread_mutex_lock(&large_list_lock);

		// Add the new node to the rest of the list for this heap
		linked_list_add(&large_list, large_block);

		pthread_mutex_unlock(&large_list_lock);
		
		ret = large_block->data;


        debug_print("%20s|##### Added a block in  large_list @%p\n",
        		__func__, large_block);

    } else {

		////debug_print("%20s|lock heap%d\n", __func__, cpu+1);
		// Lock the heap since we will be modifying it
		pthread_mutex_lock(&(HEAPS[cpu+1].lock));

		// Found our size class (size_classes[class] >= size), get an
		// appropriate superblock WHICH WILL BE LOCKED.
		if ((sb = sb_get(cpu, class)) == NULL) {
			fprintf(stderr, "Could not malloc %d bytes. Whoops.\n", size);
			exit(1);
		}


		//debug_print("%20s|sb@%p|heap%d usage%d class%d\n",
		//	__func__, sb, sb->heap, sb->usage, sb->size_class);

		// Take block from freelist
		ret = sb->free;
		sb->free = sb->free->next;

		// Unlock the superblock
		////debug_print("%20s|unlock sb%p\n", __func__, sb);
		pthread_mutex_unlock(&(sb->lock));

		// Unlock the heap and return
		////debug_print("%20s|unlock heap%d\n", __func__, cpu+1);
		pthread_mutex_unlock(&(HEAPS[cpu+1].lock));

	}

//    //debug_print("%20s|%p\n", "mm_malloc", ret);
    //dbg_print_heap_info(who);
    return ret; 
}

// Frees the memory associated with ptr. If ptr was not a previously
// allocated memory segment returned by mm_malloc, or ptr was previously
// freed behaviour is undefined. 
void mm_free (void *ptr) {

    superblock_t *sb = NULL, *prev = NULL, *next = NULL;
    superblock_t *victim = NULL;
    megablock_t *mega = NULL;
    int heap = -1;

#ifdef DEBUG
    frees++;
#endif

    // Make sure the pointer is from memory that we allocated
    if ( ((char*)ptr < dseg_lo) || ((char *)ptr > dseg_hi) )
        return;

	pthread_mutex_lock(&large_list_lock);
       
    // TODO: If this is a large allocation deal with it separately...
    mega = linked_list_find (&large_list, ptr);

	if(mega != NULL) {

		//debug_print("%20s|Freeing large block @ %p\n", __func__, mega);

		sb_free_large(&large_list, ptr);

		pthread_mutex_unlock(&large_list_lock);

        debug_print("%20s|##### Removed a block from  large_list @%p\n",
        		__func__, mega);

   	    return;
    }

	pthread_mutex_unlock(&large_list_lock);
	
    // General case:
    // Find nearest superblock alignment below *ptr
    //sb = ALIGN_DOWN_TO(ptr, sb_size);
    // We have to be careful about dseg_lo not superblock, but page aligned
    sb = (superblock_t*) (
        ( (char*) ALIGN_DOWN_TO(((char*)ptr - dseg_lo), sb_size) )
        + (unsigned long) dseg_lo );

	// Lock the heap since we will be modifying it
    ////debug_print("%20s|lock heap%d\n", __func__, sb->heap);
    pthread_mutex_lock(&(HEAPS[sb->heap].lock)); 

    ////debug_print("%20s|lock sb@%p\n", __func__, sb);
    pthread_mutex_lock(&(sb->lock)); 

    // Save the heap number that we're freeing from
    heap = sb->heap;

    // Take the superblock out of the list and re-insert it
    // This brings it to the front of the list, hopefully re-using it while it
    // is still in cache and gives us a chance to re-file it in an appropriate
    // fullness group.
    prev = sb->prev; sb->prev = NULL;
    next = sb->next; sb->next = NULL;

    if (prev == NULL) {
        // We were the head of the list...
        *(sb->group) = sb->next;
        if (*(sb->group))
            (*(sb->group))->prev = NULL;
    } else {
        prev->next = next;
        if (next)
            next->prev = prev;
    }

    sb_insert(sb, heap);

    //debug_print("%20s|entry|heap%d|sb@%p/sz%d|usage%d\n",
    //    __func__, heap, sb, size_classes[sb->size_class], HEAPS[heap].usage);

    // Insert the freed block at the head of the list (LIFO)
    ////debug_print("%20s|%p->free = %p->next = %p\n",
    //    __func__, sb, ptr, sb->free);
    ((freelist_t*)ptr)->next = sb->free;
    sb->free = (freelist_t*)ptr;
#ifdef DEBUG
    if (((char*)(sb->free) < dseg_lo) || ((char*)(sb->free) > dseg_hi))
        debug_print("%20s|wtf?", __func__);
#endif

    // Adjust statistics
    sb->usage -= size_classes[sb->size_class];
    HEAPS[heap].usage -= size_classes[sb->size_class];

    // Done with this superblock, unlock it
    //debug_print("%20s|unlock sb%p\n", __func__, sb);
    pthread_mutex_unlock(&(sb->lock)); 
   
    //if (HEAPS[heap].usage < 0)
    //    debug_print("%20s|heap%d.usage < 0: %d\n",
    //        __func__, heap, HEAPS[heap].usage);
    
    // Evict a superblock up to the glboal heap if we've crossed the 
    // emptiness threshold...
    if ((HEAPS[heap].allocated - HEAPS[heap].usage)
        > (SB_RELTHRESHOLD * sb_size)) {
        

        //debug_print("%20s|heap%d|alloc-usage:%d > %d:REL_THRESH*sb_sz\n",
        //    __func__,
        //    sb->heap, (HEAPS[sb->heap].allocated - HEAPS[sb->heap].usage),
        //    (SB_RELTHRESHOLD * sb_size));
        
        if (HEAPS[heap].usage <
            ((1.0 - SB_EMPTYFRAC) * HEAPS[heap].allocated)) {
            //debug_print("%20s| %d < ((1.0 - %f) * %d)) -- should release!\n",
            //    __func__,
            //    HEAPS[heap].usage, SB_EMPTYFRAC,
            //    HEAPS[heap].allocated);

            // Choose a victim -- VICTIM IS RETURNED ALREADY LOCKED
            victim = sb_find_victim(heap);

            // Lock global heap
            pthread_mutex_lock(&(HEAPS[0].lock));

            // Adjust stats
            HEAPS[0].usage += victim->usage;
            HEAPS[victim->heap].usage -= victim->usage;
            HEAPS[0].allocated += sb_size;
            HEAPS[victim->heap].allocated -= sb_size;

            // Insert into global heap
            sb_insert(victim, 0);

            // Don't forget to unlock the victim
            pthread_mutex_unlock(&(victim->lock));

            // Unlock global heap
            pthread_mutex_unlock(&(HEAPS[0].lock));

        }
    }
    
    //debug_print("%20s|exit |heap%d|sb@%p/sz%d|usage%d\n\n",
    //    __func__, heap, sb, size_classes[sb->size_class], HEAPS[heap].usage);

    // Done with the heap
    //debug_print("%20s|unlock heap%d\n", __func__, sb->heap);
    pthread_mutex_unlock(&(HEAPS[heap].lock));
}

// Looks for an emptyish superblock in the given heap which we can send back
// to the global heap. Upon return from sb_find_victim, the superblock will be
// locked and will have been removed from the heap. We will not update the
// statistics
//
// WE ASSUME THAT THE CALLER ALREADY HAS LOCKED THE HEAP
superblock_t* sb_find_victim(int heap) {

    superblock_t **list = NULL;
    superblock_t *victim = NULL;
    int i = 0;

    // Find and remove a superblock from size class 0 -- the completely
    // empty superblocks or from one of the emptyish lists of the other
    // size classes...
    for (i = 0; i < NSIZES; i++) {
        list = &(HEAPS[heap].sizes[i].emptyish);
        victim = *list;
        if (victim)
            break;
    }

    // If nothing found yet, look at the fullish lists of each size class
    // (not sure if we'll ever hit this, come to think about it)...
    if (!victim) {
        //debug_print("%20s|looking in fullish lists...\n", __func__);
        for (i = 1; i < NSIZES; i++) {
            list = &(HEAPS[heap].sizes[i].fullish);
            victim = *list;
            if (victim)
                break;
        }
    }

    if (!victim) {
        debug_print("%20s|something weird is going on...\n", __func__);
    } else {
        // Lock the victim
        pthread_mutex_lock(&(victim->lock));

        // Remove the victim from its heap -- we are taking from the head
        // of the list, so we've saved that address in order to update it.
        *list = victim->next;
        if (*list)
            (*list)->prev = NULL;
        victim->next = NULL;
    }

    //debug_print("%20s|victim.usage=%d\n", __func__, victim->usage);
    
    return victim;
}

// Wraps, serializes calls to mem_sbrk()
void *mm_sbrk (ptrdiff_t increment) {
    void *ret;
    pthread_mutex_lock(&sbrk_lock);
    ret = mem_sbrk(increment);
    pthread_mutex_unlock(&sbrk_lock);
    return ret;
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
//
// SUPERBLOCKS RETURNED BY THIS FUNCTION ARE LOCKED, YOU MUST UNLOCK THEM
// WHEN YOU'RE DONE
superblock_t* sb_get(int core, int sz) {

    superblock_t *result = NULL;

#define _DEC_GLOBAL_HEAP HEAPS[0].allocated -= sb_size; \
    HEAPS[0].usage -= result->usage;
#define _INC_CPU_HEAP HEAPS[core+1].allocated += sb_size;
#define _SET_HEAP result->heap = core+1;
#define _SET_SZ_CLASS result->size_class = sz;
#define _DO_FREELIST result->free = fl_init(ALIGN_UP_TO((((unsigned long)result)\
    + sizeof(superblock_t)), size_classes[sz]),\
    (void*) (((unsigned long)result) + sb_size), size_classes[sz]);

    if ((result = sb_find_free(&(HEAPS[core+1].sizes[sz])))) {
        // Found one on our heap of the correct size class, cool!
    } else if ((result = sb_find_free(&(HEAPS[core+1].sizes[0])))) {
        // Found one on our heap that was empty
        _DO_FREELIST; _SET_SZ_CLASS;
    } else {
        // Check in the global heap...
        pthread_mutex_lock(&(HEAPS[0].lock));
        if ((result = sb_find_free(&(HEAPS[0].sizes[sz])))) {
            // Found a global SB of the correct size class
            _SET_HEAP; _DEC_GLOBAL_HEAP; _INC_CPU_HEAP;
        } else if ((result = sb_find_free(&(HEAPS[0].sizes[0])))) {
            // Found an empty global SB, need to set up the free list
            // and size class
            _DO_FREELIST; _SET_SZ_CLASS; _SET_HEAP;
            _DEC_GLOBAL_HEAP; _INC_CPU_HEAP;
        } else if ((result = (superblock_t *) mm_sbrk(sb_size))) {
            // Made a new one, need to set up the free list, lock
            // and size class
            bzero(result, sizeof(superblock_t));
            pthread_mutex_init(&(result->lock), &mutexattr);
            _DO_FREELIST; _SET_SZ_CLASS; _SET_HEAP;
        } else {
            // mm_sbrk failed...
            return NULL;
        }
        pthread_mutex_unlock(&(HEAPS[0].lock));
    }

    pthread_mutex_lock(&(result->lock));

    // Increment usage counters
    result->usage += size_classes[sz];
    HEAPS[core+1].usage += size_classes[sz];

    // Put the SB at the head of the list for this cpu/size-class
    sb_insert(result, core+1);

    // Now we are guaranteed to have a superblock with free space pointed
    // at by result.
    return result;
}

// Try to find something from the fullish superblocks first
//
// ASSUMES THAT ANY LOCKS GOVERNING THE HEAP WE ARE SEARCHING ARE ALREADY HELD
//
// SUPERBLOCKS RETURNED BY THIS FUNCTION ARE NOT IN ANY HEAP'S LIST, YOU
// HAVE TO PUT THEM SOMEWHERE.
superblock_t* sb_find_free(sizeclass_t *class) {
    superblock_t *from_fullish = NULL;

    return (from_fullish = _sb_find_free(&(class->fullish)))?
        from_fullish : _sb_find_free(&(class->emptyish));
}

// TODO: Lock the SB before returning?
// Looks through the superblock list starting at *sb for one having free space
// Upon finding such a superblock, removes it from the list and returns a
// pointer to it. If none is found, return NULL.
//
// ASSUMES THAT ANY LOCKS GOVERNING THE HEAP WE ARE SEARCHING ARE ALREADY HELD
//
// SUPERBLOCKS RETURNED BY THIS FUNCTION ARE NOT IN ANY HEAP'S LIST, YOU
// HAVE TO PUT THEM SOMEWHERE.
superblock_t* _sb_find_free(superblock_t **sb) {
    
    superblock_t *prev = NULL;
    superblock_t *ret = *sb;

    ////debug_print("%20s|sb=%p, *sb=ret=%p\n",
    //    "sb_find_free", sb, ret);

    while( (ret != NULL) && ( ! SB_HAS_FREE(ret)) ) {
        prev = ret;
        ret = ret->next;
        ////debug_print("%20s|stepping... sb@%p->free=%p\n",
        //    "sb_find_free", ret, ret?(ret->free):(0));
    }

    if (ret != NULL) {
        // Found something with free blocks
        if (prev == NULL) {
            // This was the first thing on the list, update that head pointer,
            // take the thing we're returning out of the list.
            *sb = ret->next;
            if (*sb)
                (*sb)->prev = NULL;
        } else {
            // This was not the first thing in the list, patch over it.
            prev->next = ret->next;
            if (prev->next)
                prev->next->prev = prev;
        }

        ret->prev = NULL;
        ret->next = NULL;
    }

    ////debug_print("%20s|returning %p\n", "sb_find_free", ret);
    return ret;
}

// Starting at *first, fl_init() writes out freelist_t nodes in a linked list
// spaced by sz bytes, stopping at *limit. Since the freelist_t nodes are
// written on the beginning of a free block of size sz, the highest that we're
// allowed to write one is at limit - szReturns *first upon success.
freelist_t* fl_init(void *first, void *limit, size_t sz) {

//#ifdef DEBUG
    int count = 0;
//#endif

    freelist_t *fl = (freelist_t *) first;
    unsigned long next_fl = ((unsigned long) fl) + sz;

    // Walk through the rest of the superblock in increments
    // equal to the chunk size while building the free list
    while (next_fl < (((unsigned long)limit) - sz)) {
//#ifdef DEBUG
        count++;
//#endif
        fl->next = (freelist_t*) next_fl;
        fl = (freelist_t*) next_fl;
        next_fl += sz;
    }

    // The last freelist node should point nowhere
    fl->next = NULL;

    ////debug_print("%20s|%p .. %p: %d @ %d\n", "fl_init", first, limit, count, sz);

    return (freelist_t *) first;

}

megablock_t *
sb_get_large(size_t size) {

	megablock_t* new_node = NULL;
	size_t alloc_size = 0;

	// Calculate the size for the new block (including node)
	// Has to be sb_size aligned for the other stuff to work
	alloc_size = (size_t) ALIGN_UP_TO(size + sizeof(megablock_t), sb_size);
	
	// Get the new memory
	new_node = (megablock_t*) mem_sbrk(sb_size);

	// If can't get more memory return NULL
	if(!new_node)
		return NULL;

	// Zero the allocated region
    bzero(new_node, alloc_size);

	// Set the data pointer to the memory block given to the user
	// This is not really necessary, because we don't use it.
	// For consisency and convenience later on.
	new_node->data = (void *) new_node + sizeof(megablock_t);
	new_node->size = alloc_size;
	
	return new_node;
}

void
sb_free_large(linked_list_t * list, void * data) {
	megablock_t* node;
	unsigned long i, length;
	superblock_t *sb;

	// No locking required, since it is done by the callers

	// Get the LL node to work with
	node = (megablock_t*) ((unsigned long) data) - sizeof(megablock_t);

	// Disconnect the node from the data
	node->data = NULL;
	length = node->size;
	node->size = 0;

	// Remove the node from the list
	linked_list_delete(list, node);

	// TODO: Cut up the newly freed data region into superblocks
	// and pass them up to the global heap
	for(i = 0; i < length; i += sb_size) {

		sb = (superblock_t *) ((unsigned long) node) + i;

		bzero((void*) sb, sizeof(superblock_t));

		sb_insert(sb, 0);
	}

	
}


////////////////////////////////////////////////////////////

/* The following function initializes the linked list by putting zeros
   into the pointers containing the first and last links of the linked
   list. */

void
linked_list_init (linked_list_t * list)
{
    list->first = list->last = 0;
}

/* The following function adds a new link to the end of the linked
   list. The node has to be preallocated and have the data set. */
void
linked_list_add (linked_list_t * list, ll_node * link)
{

    if (! link) {
        fprintf (stderr, "can't insert invalid node.\n");
        exit (EXIT_FAILURE);
    }

    if (list->last) {
        /* Join the two final links together. */
        list->last->next = link;
        link->prev = list->last;
        list->last = link;
    } else {
        list->first = link;
        list->last = link;
    }
}

/* Delete a given node from the list. */
void
linked_list_delete (linked_list_t * list, ll_node * link)
{
    ll_node * prev;
    ll_node * next;

    prev = link->prev;
    next = link->next;
    
    if (prev) {
        if (next) {
            /* Both the previous and next links are valid, so just
               bypass "link" without altering "list" at all. */
            prev->next = next;
            next->prev = prev;
        } else {
            /* Only the previous link is valid, so "prev" is now the
               last link in "list". */
            prev->next = 0;
            list->last = prev;
        }
    } else {
        if (next) {
            /* Only the next link is valid, not the previous one, so
               "next" is now the first link in "list". */
            next->prev = 0;
            list->first = next;
        } else {
            /* Neither previous nor next links are valid, so the list
               is now empty. */
            list->first = 0;
            list->last = 0;
        }
    }

}

/* Traverse the list, using a callback function (i.e. visitor) */
void
linked_list_traverse (linked_list_t * list, void (*callback) (void *))
{
    ll_node * link;

    for (link = list->first; link; link = link->next) {
        callback ((void *) link->data);
    }
}

/* Find an element in the list, given the data pointer. */
megablock_t *
linked_list_find (linked_list_t * list, void *data)
{
    ll_node * link;

    for (link = list->first; link; link = link->next) {
    	if(link->data == data)
    		return link;
    }

    return NULL;
}
 
void
linked_list_traverse_delete (linked_list_t * list, void * data)
{
	ll_node * link;

	for (link = list->first; link; link = link->next) {
		if (link->data == data) 
	    	linked_list_delete (list, link);
	}
}
////////////////////////////////////////////////////////////

