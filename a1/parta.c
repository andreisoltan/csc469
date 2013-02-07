#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tsc.h"
#include <unistd.h>

#define DEFAULT_THRESH 500
#define DEFAULT_NUM    1000

u_int64_t inactive_periods(int num, u_int64_t threshold, u_int64_t *samples);
u_int64_t collect_intervals(int num, u_int64_t *samples);

int main(int argc, char **argv) {

    // TODO: Determine clock speed to convert cycles/milliseconds
    // (/proc/cpuinfo contains the current speed of the processor)

    char flag_c = 0;
    char flag_f = 0;
    char flag_i = 0;
    char flag_n = 0;
    char flag_t = 0;

    int c;
    double freq;
    int num = DEFAULT_NUM;
    u_int64_t threshold = DEFAULT_THRESH;
    u_int64_t *samples;

    pid_t childpid = 0;

    int i;

    // No arguments, print usage, exit.
    if (argc == 1) {
        fprintf(stderr, "\n\
    USAGE:  %s [-c] [-f <MHz>] [-i] [-n <num>] [-t <threshold>]\n\
\n\
    We'll use this tool to gather data regarding:\n\
    * A1a: Tracking process activity with (optional) arguments -f -n -t\n\
    * A1a: Context switching with the -c flag (and optionally, -f -n -t)\n\
\n\
      -c\n\
          The process will fork, both parent and child will record\n\
          inactive periods. We'll use this data to try to gain some\n\
          insight into context switching.\n\
\n\
      -f <MHz>\n\
          Using this floating point value assumed to be the clock rate\n\
          in MHz, output millisecond measurements for inactive periods.\n\
          This argument has no effect when used with -i.\n\
\n\
      -i\n\
          Collect interval times. We'll use this to try to determine a\n\
          suitable threshold. This option will override -t.\n\n\
\n\
      -n <num>\n\
          Number of samples to collect (defaults to %d).\n\
\n\
      -t <threshold>\n\
          Detect inactive time periods longer than this threshold (defaults\n\
          to %d).\n",
          argv[0], DEFAULT_NUM, DEFAULT_THRESH);

        return 1;
    }

    while ((c = getopt (argc, argv, "f:t:n:ic")) != -1) {
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
                num = strtoull(optarg, NULL, 10);
                if (num == 0) {
                    fprintf(stderr, "-n may not be zero\n");
                    return 1;
                }
                break;
            case 'i':
                flag_i = 1;
                break;
            case 'f':
                flag_f = 1;
                freq = strtod(optarg, NULL);
                if (freq == 0.0) {
                    fprintf(stderr, "-f may not be zero\n");
                    return 1;
                }
                break;
            case 'c':
                flag_c = 1;
                break;    
            case '?':
                if ((optopt == 't' )||(optopt == 'n')||(optopt == 'f'))
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character \\x%x.\n", optopt);
                return 1;
                
        }
    }

    // We are given the frequency in MHz (million-cycles/second), we'll convert
    // it here to cycles/millisecond for convenience later.
    freq = freq * (1000000 / 1000);

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
            printf("%llu\n", samples[i]);
        }

    } else { // Measure inactive periods

        // fork if asked
        if (flag_c)
            childpid = fork();

        // both processes should go ahead and record their intervals...
        inactive_periods(num, threshold, samples);

        // parent waits for child to finish so the output is not intermingled
        // Both print, we sort it out later based on the CHILD column...
        if (flag_c && (childpid != 0)) {
            wait(NULL);
        } else {
            // child prints first so it puts out the headers:
            printf("%6s\t%6s\t%11s\t%11s\t%11s\t%11s\n",
//              ((flag_c == 1)?"CHILD":""),
              "CHILD", "ACTIVE", "START-CYCLE", "END-CYCLE", "LEN-CYCLE",
              ((flag_f == 1)?"LEN-MS":"N/A"));
        }

        i = 1;
        while ( i < (num - 2 )) {
            printf("%6s\t%6s\t%11llu\t%11llu\t%11llu\t%11.8f\n",
              (flag_c && (!childpid))?("yes"):("no"),
              ((i%2)==0)?("yes"):("no"),
              samples[i], samples[i+1], samples[i+1] - samples[i],
              ((flag_f == 1)?((samples[i+1] - samples[i]) / freq):0));
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

u_int64_t
forking_inactive_periods(int num, u_int64_t *samples) {





}
