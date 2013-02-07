#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>

/* A period [start, end]. */
typedef struct {
  u_int64_t start;
  u_int64_t end;
} period_t;

static u_int64_t random_periods(int num, period_t* samples);
static void print_periods(int num, period_t* samples);

int main(int argc, char** argv) {
  int num;
  period_t* periods;

  /* Parse the argument. */
  assert(argc == 2);
  num = atoi(argv[1]);
  assert(num > 0);

  periods = (period_t*) malloc(sizeof(period_t) * num);
  assert(periods != NULL);

  random_periods(num, periods);
  print_periods(num, periods);

  free(periods);
  return 0;
}

static void print_periods(int num, period_t* periods) {
  int i;
  for (i = 0; i < num; i++) {
    /* On 64-bit machines, the format specifier is %lu whereas on 32-bit
     * machines the format specifier is %llu. PRIu64 is set to the appropriate
     * format specifier in <inttypes.h>.
     */
    printf("start=%" PRIu64 ",end=%" PRIu64 "\n", periods[i].start, periods[i].end);
  }
}

/* Creates num periods of random duration. The periods are non-overlapping and
 * have monotonically increasing start times.
 */
static u_int64_t random_periods(int num, period_t* periods) {
  int i;
  u_int64_t current = 0;
  srand(time(NULL));
  for (i = 0; i < num; i++) {
    current += rand() % 50;
    periods[i].start = current;
    current += rand() % 50;
    periods[i].end = current;
  }
  return 0;
}
