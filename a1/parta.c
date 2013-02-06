#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tsc.h"

#define DEFAULT_THRESH 500
#define DEFAULT_NUM    1000

u_int64_t inactive_periods(int num, u_int64_t threshold, u_int64_t *samples);
u_int64_t collect_intervals(int num, u_int64_t *samples);

int main(int argc, char **argv) {

    // TODO: Determine clock speed to convert cycles/milliseconds
    // (/proc/cpuinfo contains the current speed of the processor)

    char flag_i = 0;
    char flag_t = 0;
    char flag_n = 0;

    int c;
    int num = DEFAULT_NUM;
    u_int64_t threshold = DEFAULT_THRESH;
    u_int64_t *samples;

    int i;

    // No arguments, print usage, exit.
    if (argc == 1) {
        fprintf(stderr, "\n\
    USAGE:  %s [-t <threshold>] [-n <num>] [-i]\n\
\n\
      -t <threshold>\n\
          Detect inactive time periods at this threshold (defaults to %d)\n\
\n\
      -n <num>\n\
          Number of samples to collect (defaults to %d)\n\
\n\
      -i\n\
          Collect interval times. We'll use this to try to determine a\n\
          suitable threshold. This option will override -t\n\n",
          argv[0], DEFAULT_THRESH, DEFAULT_NUM);

        return 1;
    }

    while ((c = getopt (argc, argv, "t:n:i")) != -1) {
        switch (c) {
            // TODO: Checking that string->int conversions succeed...
            case 't':
                flag_t = 1;
                threshold = strtoull(optarg, NULL, 10);
                if (threshold == 0) {
                    fprintf(stderr, "-t may not be zero\n");
                    return 1;
                }
                break;
            case 'n':
                flag_n = 1;
                num = strtoul(optarg, NULL, 10);
                if (num == 0) {
                    fprintf(stderr, "-n may not be zero\n");
                    return 1;
                }
                break;
            case 'i':
                flag_i = 1;
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

    // Storage space for recorded samples
    samples = malloc(sizeof(u_int64_t)*num);

    if (!samples) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    // Make cycle counter available
    start_counter();

    if (flag_i) { // Collect interval timing

        if (flag_t)
            fprintf(stderr, "Both -i and -t were set, ignoring -t\n");

        collect_intervals(num, samples);

        for (i = 0; i < num; i++) {
            printf("%lld\n", samples[i]);
        }

    } else { // Measure inactive periods
        inactive_periods(num, threshold, samples);

        printf(" %8s \t%15s\t%15s\t%15s\n",
          "TYPE", "START-CYCLE", "END-CYCLE", "DURATION");
        i = 1;
        while ( i < (num - 2 )) {
            printf("%10s\t%15lld\t%15lld\t%15lld\n",
              ((i%2)==0)?(" active "):(" inactive "),
              samples[i], samples[i+1], samples[i+1] - samples[i]);
            i++;
        }

    }

    // Cleanup
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
 *
 * (samples[n],samples[n+1]) represents an active period iff
 * n is even and represents an inactive period iff n is odd:
 *
 * samples[0] -
 *             } First active period
 * samples[1] - 
 *             } First inactive period
 * samples[2] -
 *             } Second active period
 * samples[3] -
 *             } Second inactive period
 * etc.   ...
 *
 */
u_int64_t
inactive_periods(int num, u_int64_t threshold, u_int64_t *samples) {

    int i = 1;
    u_int64_t last;
    u_int64_t now;

	if(!samples)
		return -1;

    // Begin first active period
    samples[0] = (last = get_counter());

    while ( i < (num - 1) ) {
        now = get_counter();

        // If we have exceeded the threshold, then we must have been
        // inactive from last until now:
        if ((now - last) > threshold) {
            samples[i++] = last; // odd index
            samples[i++] = now;  // even index
        }

        last = now;
    }

    // TODO: Something with the last entry in samples in the case that
    // we break out of the loop with i = (num - 1)?

    // "The function should return the initial reading - that is, the start of
    // the first active period." says the assignment.
    return samples[0];
	
}

/**
 * This function continually checks the cycle counter and computes num
 * differences between successive readings, storing the results in
 * samples.
 *
 * Returns the length of the first interval or -1 if problems occur.
 */
u_int64_t
collect_intervals(int num, u_int64_t *samples) {

    int i;
    u_int64_t last;
    u_int64_t now;

    if (!samples)
        return -1;

    // Computes num interval times into samples
    last = get_counter();
    for (i = 0; i < num; i++) {
        now = get_counter();
        samples[i] = now - last;
        last = now;
    }

    return samples[0];  
    
}
