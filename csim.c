#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "cachelab.h"

typedef struct {
    int valid;
    unsigned long long tag;
    unsigned long long last_used;
} cache_line_t;

/*
 * Simulate one data-cache access.
 *
 * The cache is stored as a flat array.  The lines for set i start at
 * cache[i * E].  last_used is a monotonically increasing timestamp used
 * to select the least-recently-used line on eviction.
 */
static void access_cache(cache_line_t *cache, int s, int E, int b,
                         unsigned long long address,
                         unsigned long long *time,
                         int *hit_count, int *miss_count,
                         int *eviction_count, int verbose)
{
    unsigned long long set_mask = (1ULL << s) - 1ULL;
    unsigned long long set_index = (address >> b) & set_mask;
    unsigned long long tag = address >> (s + b);
    cache_line_t *set = cache + (set_index * E);
    int victim = 0;
    int i;

    for (i = 0; i < E; i++) {
        if (set[i].valid && set[i].tag == tag) {
            (*hit_count)++;
            set[i].last_used = ++(*time);
            if (verbose) {
                printf(" hit");
            }
            return;
        }
    }

    (*miss_count)++;
    if (verbose) {
        printf(" miss");
    }

    for (i = 0; i < E; i++) {
        if (!set[i].valid) {
            victim = i;
            break;
        }
        if (set[i].last_used < set[victim].last_used) {
            victim = i;
        }
    }

    if (set[victim].valid) {
        (*eviction_count)++;
        if (verbose) {
            printf(" eviction");
        }
    }

    set[victim].valid = 1;
    set[victim].tag = tag;
    set[victim].last_used = ++(*time);
}

static void print_usage(char *program)
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n",
           program);
}

int main(int argc, char **argv)
{
    int s = -1;
    int E = -1;
    int b = -1;
    int verbose = 0;
    char *trace_file = NULL;

    FILE *trace;
    cache_line_t *cache;
    int S;
    int hit_count = 0;
    int miss_count = 0;
    int eviction_count = 0;
    unsigned long long time = 0;
    char op;
    unsigned long long address;
    int size;
    int opt;

    while ((opt = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (opt) {
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (s == -1 || E == -1 || b == -1 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_usage(argv[0]);
        return 1;
    }

    trace = fopen(trace_file, "r");
    if (trace == NULL) {
        perror(trace_file);
        return 1;
    }

    S = 1 << s;
    cache = calloc(S * E, sizeof(cache_line_t));
    if (cache == NULL) {
        perror("calloc");
        fclose(trace);
        return 1;
    }

    while (fscanf(trace, " %c %llx,%d", &op, &address, &size) == 3) {
        if (op == 'I') {
            continue;
        }

        if (verbose) {
            printf("%c %llx,%d", op, address, size);
        }

        if (op == 'L' || op == 'S') {
            access_cache(cache, s, E, b, address, &time,
                         &hit_count, &miss_count, &eviction_count, verbose);
        } else if (op == 'M') {
            access_cache(cache, s, E, b, address, &time,
                         &hit_count, &miss_count, &eviction_count, verbose);
            access_cache(cache, s, E, b, address, &time,
                         &hit_count, &miss_count, &eviction_count, verbose);
        }

        if (verbose) {
            printf("\n");
        }
    }

    fclose(trace);
    free(cache);

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
