#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tsc.h"

u_int64_t inactive_periods(int num, u_int64_t threshold, u_int64_t *samples);

int main(int argc, char **argv) {

    // TODO: Determine clock speed to convert cycles/milliseconds

    int c;
    int num = 100;
    u_int64_t threshold = 5000;
    u_int64_t *samples;

    while ((c = getopt (argc, argv, "t:n:")) != -1) {
        switch (c) {
            // TODO: Checking that string->int conversions succeed...
            case 't':
                threshold = strtoull(optarg, NULL, 10);
                if (threshold == 0) {
                    fprintf(stderr, "-t may not be zero\n");
                    return 1;
                }
                break;
            case 'n':
                num = strtoul(optarg, NULL, 10);
                if (num == 0) {
                    fprintf(stderr, "-n may not be zero\n");
                    return 1;
                }
                break;
            case '?':
                if ((optopt == 't' )||(optopt == 'n'))
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character \\x%x.\n", optopt);
                return 1;
                
        }
    }

    printf("Collecting %d samples with threshold %lld...\n", num, threshold);

    samples = malloc(sizeof(u_int64_t)*num);
    inactive_periods(num, threshold, samples);
    free(samples);
    samples = NULL;

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

    fprintf(stderr, "Just kidding, this doesn't do shit yet\n");
	// TODO: Add code to do the measurements

	if(!samples)
		return -1;

    // "The function should return the initial reading - that is, the start of
    // the first active period." says the assignment.
    return samples[0];
	
}
