
#include <stdlib.h>
#include <stdio.h>
#include "tsc.h"

u_int64_t inactive_periods(int num, u_int64_t threshold, u_int64_t *samples);

int main() {


	// TODO: Do stuff to get the measurements

	return 0;
}

/**
 * This function continually checks the cycle counter
 * and detects when two successive readings differ by
 * more than threshold cycles, which indicates that 
 * the process has been inactive. 
 *
 * A trace of active and inactive periods for a process
 * running on an Intel Linux system can be created using
 * the provided start_counter() and get_counter() functions.
 */
u_int64_t
inactive_periods(int num, u_int64_t threshold, u_int64_t *samples) {

	// TODO: Add code to do the measurements

	if(!samples)
		return -1;
	
}
