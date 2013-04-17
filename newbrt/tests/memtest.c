/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "Copyright (c) 2007, 2008 Tokutek Inc.  All rights reserved."

#include <stdio.h>
#include <stdlib.h>
#include "toku_assert.h"
#include "memory.h"
#include "mempool.h"

void test_mempool_limits(size_t size) {
    void *base = malloc(size);
    struct mempool mempool;
    toku_mempool_init(&mempool, base, size);

    size_t i;
    for (i=0;; i++) {
        void *vp = toku_mempool_malloc(&mempool, 1, 1);
        if (vp == 0) 
            break;
    }
    assert(i == size);

    toku_mempool_fini(&mempool);
    free(base);
}

void test_mempool_malloc_mfree(size_t size) {
    void *base = malloc(size);
    struct mempool mempool;
    toku_mempool_init(&mempool, base, size);

    void *vp[size];
    size_t i;
    for (i=0;; i++) {
        vp[i] = toku_mempool_malloc(&mempool, 1, 1);
        if (vp[i] == 0) 
            break;
    }
    assert(i == size);

    for (i=0; i<size; i++) 
        toku_mempool_mfree(&mempool, vp[i], 1);
    assert(toku_mempool_get_frag_size(&mempool) == size);

    toku_mempool_fini(&mempool);
    free(base);
}

int main() {
    test_mempool_limits(0);
    test_mempool_limits(256);
    test_mempool_malloc_mfree(0);
    test_mempool_malloc_mfree(256);
    return 0;
}