/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "Copyright (c) 2007, 2008 Tokutek Inc.  All rights reserved."

/* Insert a bunch of stuff */
#include "brt.h"
#include "key.h"
#include "memory.h"
#include "toku_assert.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static const char fname[]="sinsert.brt";

enum { SERIAL_SPACING = 1<<6 };
enum { ITEMS_TO_INSERT_PER_ITERATION = 1<<20 };
//enum { ITEMS_TO_INSERT_PER_ITERATION = 1<<14 };
enum { BOUND_INCREASE_PER_ITERATION = SERIAL_SPACING*ITEMS_TO_INSERT_PER_ITERATION };

enum { NODE_SIZE = 1<<20 };

static int nodesize = NODE_SIZE;
static int keysize = sizeof (long long);
static int valsize = sizeof (long long);
static int do_verify =0; /* Do a slow verify after every insert. */

static int verbose = 1;

static CACHETABLE ct;
static BRT t;

static void setup (void) {
    int r;
    unlink(fname);
    r = toku_brt_create_cachetable(&ct, 0, ZERO_LSN, NULL_LOGGER);         assert(r==0);
    r = toku_open_brt(fname, 0, 1, &t, nodesize, ct, NULL_TXN, toku_default_compare_fun, (DB*)0); assert(r==0);
}

static void shutdown (void) {
    int r;
    r = toku_close_brt(t, 0); assert(r==0);
    r = toku_cachetable_close(&ct); assert(r==0);
}
static void long_long_to_array (unsigned char *a, unsigned long long l) {
    int i;
    for (i=0; i<8; i++)
	a[i] = (l>>(56-8*i))&0xff;
}

static void insert (long long v) {
    unsigned char kc[keysize], vc[valsize];
    DBT  kt, vt;
    memset(kc, 0, sizeof kc);
    long_long_to_array(kc, v);
    memset(vc, 0, sizeof vc);
    long_long_to_array(vc, v);
    toku_brt_insert(t, toku_fill_dbt(&kt, kc, keysize), toku_fill_dbt(&vt, vc, valsize), 0);
    if (do_verify) toku_cachetable_verify(ct);
}

static void serial_insert_from (long long from) {
    long long i;
    for (i=0; i<ITEMS_TO_INSERT_PER_ITERATION; i++) {
	insert((from+i)*SERIAL_SPACING);
    }
}

static long long llrandom (void) {
    return (((long long)(random()))<<32) + random();
}

static void random_insert_below (long long below) {
    long long i;
    assert(0 < below);
    for (i=0; i<ITEMS_TO_INSERT_PER_ITERATION; i++) {
	insert(llrandom()%below);
    }
}

static double tdiff (struct timeval *a, struct timeval *b) {
    return (a->tv_sec-b->tv_sec)+1e-6*(a->tv_usec-b->tv_usec);
}

static void biginsert (long long n_elements, struct timeval *starttime) {
    long long i;
    struct timeval t1,t2;
    int iteration;
    for (i=0, iteration=0; i<n_elements; i+=ITEMS_TO_INSERT_PER_ITERATION, iteration++) {
	gettimeofday(&t1,0);
	serial_insert_from(i);
	gettimeofday(&t2,0);
	if (verbose) {
	    printf("serial %9.6fs %8.0f/s    ", tdiff(&t2, &t1), ITEMS_TO_INSERT_PER_ITERATION/tdiff(&t2, &t1));
	    fflush(stdout);
	}
	gettimeofday(&t1,0);
	random_insert_below((i+ITEMS_TO_INSERT_PER_ITERATION)*SERIAL_SPACING);
	gettimeofday(&t2,0);
	if (verbose) {
	    printf("random %9.6fs %8.0f/s    ", tdiff(&t2, &t1), ITEMS_TO_INSERT_PER_ITERATION/tdiff(&t2, &t1));
	    printf("cumulative %9.6fs %8.0f/s\n", tdiff(&t2, starttime), (ITEMS_TO_INSERT_PER_ITERATION*2.0/tdiff(&t2, starttime))*(iteration+1));
	}
    }
}

static void usage() {
    printf("benchmark-test [-v] [--nodesize NODESIZE] [--keysize KEYSIZE] [--valsize VALSIZE] [--verify] [ITERATIONS]\n");
}

int main (int argc, char *argv[]) {
    /* parse parameters */
    int i;
    for (i=1; i<argc; i++) {
        char *arg = argv[i];
        if (arg[0] != '-')
            break;
        if (strcmp(arg, "--nodesize") == 0) {
            if (i+1 < argc) {
                i++;
                nodesize = atoi(argv[i]);
            }
        } else if (strcmp(arg, "--keysize") == 0) {
            if (i+1 < argc) {
                i++;
                keysize = atoi(argv[i]);
            }
        } else if (strcmp(arg, "--valsize") == 0) {
            if (i+1 < argc) {
                i++;
                valsize = atoi(argv[i]);
            }
        } else if (strcmp(arg, "--verify")==0) {
	    do_verify = 1;
	} else if (strcmp(arg, "-v")==0) {
	    verbose++;
	} else if (strcmp(arg, "-q")==0) {
	    verbose = 0;
	} else {
	    usage();
	    return 1;
	}
    }

    struct timeval t1,t2,t3;
    long long total_n_items;
    if (i < argc) {
	char *end;
	errno=0;
	total_n_items = ITEMS_TO_INSERT_PER_ITERATION * (long long) strtol(argv[i], &end, 10);
	assert(errno==0);
	assert(*end==0);
	assert(end!=argv[i]);
    } else {
	total_n_items = 1LL<<22; // 1LL<<16
    }

    if (verbose) {
	printf("nodesize=%d\n", nodesize);
	printf("keysize=%d\n", keysize);
	printf("valsize=%d\n", valsize);
	printf("Serial and random insertions of %d per batch\n", ITEMS_TO_INSERT_PER_ITERATION);
    }
    setup();
    gettimeofday(&t1,0);
    biginsert(total_n_items, &t1);
    gettimeofday(&t2,0);
    shutdown();
    gettimeofday(&t3,0);
    if (verbose) {
	printf("Shutdown %9.6fs\n", tdiff(&t3, &t2));
	printf("Total time %9.6fs for %lld insertions = %8.0f/s\n", tdiff(&t3, &t1), 2*total_n_items, 2*total_n_items/tdiff(&t3, &t1));
    }
    if (verbose>1) {
	toku_malloc_report();
    }
    toku_malloc_cleanup();
    return 0;
}
