/* $Id$ */

/*
 *
 *  CSC 469 - Assignment 2
 *
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
     "Ultra Team Squad Alpha",
     /* Full name of first team member */
     "Anrei Soltan",
     /* Email address of first team member */
     "g0soltan@cdf.toronto.edu",
     /* Full name of second team member */
     "Jonathan Prindiville",
     /* Email address of second team member */
     "g2prindi@cdf.toronto.edu"
};

int mm_init (void)
{
}

void *mm_malloc (size_t size)
{
}

void mm_free (void *ptr)
{
}

