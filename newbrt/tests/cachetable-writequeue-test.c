#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

int verbose;

typedef struct ctpair *PAIR;
struct ctpair {
    PAIR next_wq;
};

PAIR new_pair() {
    PAIR p = (PAIR) malloc(sizeof *p); assert(p);
    return p;
}

void destroy_pair(PAIR p) {
    free(p);
}

#include "cachetable-writequeue.h"

// test simple create and destroy

void test_create_destroy() {
    struct writequeue writequeue, *wq = &writequeue;
    writequeue_init(wq);
    assert(writequeue_empty(wq));
    writequeue_destroy(wq);
} 

// verify that the wq implements FIFO ordering

void test_simple_enq_deq(int n) {
    struct writequeue writequeue, *wq = &writequeue;
    int r;
    pthread_mutex_t mutex; 

    r = pthread_mutex_init(&mutex, 0); assert(r == 0);
    writequeue_init(wq);
    assert(writequeue_empty(wq));
    PAIR pairs[n];
    int i;
    for (i=0; i<n; i++) {
        pairs[i] = new_pair();
        writequeue_enq(wq, pairs[i]);
        assert(!writequeue_empty(wq));
    }
    for (i=0; i<n; i++) {
        PAIR p;
        r = writequeue_deq(wq, &mutex, &p); 
        assert(r == 0 && p == pairs[i]);
        destroy_pair(p);
    }
    assert(writequeue_empty(wq));
    writequeue_destroy(wq);
    r = pthread_mutex_destroy(&mutex); assert(r == 0);
}

// setting the wq closed should cause deq to return EINVAL

void test_set_closed() {
    struct writequeue writequeue, *wq = &writequeue;
    writequeue_init(wq);
    writequeue_set_closed(wq);
    int r = writequeue_deq(wq, 0, 0);
    assert(r == EINVAL);
    writequeue_destroy(wq);
}

// closing a wq with a blocked reader thread should cause the reader to get EINVAL

struct writequeue_with_mutex {
    struct writequeue writequeue;
    pthread_mutex_t mutex;
};

void writequeue_with_mutex_init(struct writequeue_with_mutex *wqm) {
    writequeue_init(&wqm->writequeue);
    int r = pthread_mutex_init(&wqm->mutex, 0); assert(r == 0);
}

void writequeue_with_mutex_destroy(struct writequeue_with_mutex *wqm) {
    writequeue_destroy(&wqm->writequeue);
    int r = pthread_mutex_destroy(&wqm->mutex); assert(r == 0);
}

void *test_set_closed_waiter(void *arg) {
    struct writequeue_with_mutex *wqm = arg;
    int r;

    r = pthread_mutex_lock(&wqm->mutex); assert(r == 0);
    PAIR p;
    r = writequeue_deq(&wqm->writequeue, &wqm->mutex, &p);
    assert(r == EINVAL);
    r = pthread_mutex_unlock(&wqm->mutex); assert(r == 0);
    return arg;
}

void test_set_closed_thread() {
    struct writequeue_with_mutex writequeue_with_mutex, *wqm = &writequeue_with_mutex;
    int r;

    writequeue_with_mutex_init(wqm);
    pthread_t tid;
    r = pthread_create(&tid, 0, test_set_closed_waiter, wqm); assert(r == 0);
    sleep(1);
    writequeue_set_closed(&wqm->writequeue);
    void *ret;
    r = pthread_join(tid, &ret);
    assert(r == 0 && ret == wqm);
    writequeue_with_mutex_destroy(wqm);
}

// verify writer reader flow control
// the write (main) thread writes as fast as possible until the wq is full. then it
// waits.
// the read thread reads from the wq slowly using a random delay.  it wakes up any
// writers when the wq size <= 1/2 of the wq limit

struct rwfc {
    pthread_mutex_t mutex;
    struct writequeue writequeue;
    int current, limit;
};

void rwfc_init(struct rwfc *rwfc, int limit) {
    int r;
    r = pthread_mutex_init(&rwfc->mutex, 0); assert(r == 0);
    writequeue_init(&rwfc->writequeue);
    rwfc->current = 0; rwfc->limit = limit;
}

void rwfc_destroy(struct rwfc *rwfc) {
    int r;
    writequeue_destroy(&rwfc->writequeue);
    r = pthread_mutex_destroy(&rwfc->mutex); assert(r == 0);
}

void *rwfc_reader(void *arg) {
    struct rwfc *rwfc = arg;
    int r;
    while (1) {
        PAIR ctpair;
        r = pthread_mutex_lock(&rwfc->mutex); assert(r == 0);
        r = writequeue_deq(&rwfc->writequeue, &rwfc->mutex, &ctpair);
        if (r == EINVAL) {
            r = pthread_mutex_unlock(&rwfc->mutex); assert(r == 0);
            break;
        }
        if (2*rwfc->current-- > rwfc->limit && 2*rwfc->current <= rwfc->limit) {
            writequeue_wakeup_write(&rwfc->writequeue);
        }
        r = pthread_mutex_unlock(&rwfc->mutex); assert(r == 0);
        destroy_pair(ctpair);
        usleep(random() % 100);
    }
    return arg;
}       

void test_flow_control(int limit, int n) {
    struct rwfc my_rwfc, *rwfc = &my_rwfc;
    int r;
    rwfc_init(rwfc, limit);
    pthread_t tid;
    r = pthread_create(&tid, 0, rwfc_reader, rwfc); assert(r == 0);
    sleep(1);     // this is here to block the reader on the first deq
    int i;
    for (i=0; i<n; i++) {
        PAIR ctpair = new_pair();
        r = pthread_mutex_lock(&rwfc->mutex); assert(r == 0);
        writequeue_enq(&rwfc->writequeue, ctpair);
        rwfc->current++;
        while (rwfc->current >= rwfc->limit) {
            // printf("%d - %d %d\n", i, rwfc->current, rwfc->limit);
            writequeue_wait_write(&rwfc->writequeue, &rwfc->mutex);
        }
        r = pthread_mutex_unlock(&rwfc->mutex); assert(r == 0);
        // usleep(random() % 1);
    }
    writequeue_set_closed(&rwfc->writequeue);
    void *ret;
    r = pthread_join(tid, &ret); assert(r == 0);
    rwfc_destroy(rwfc);
}

int main(int argc, char *argv[]) {
    int i;
    for (i=1; i<argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "-v") == 0) {
            verbose++;
            continue;
        }
    }
    test_create_destroy();
    test_simple_enq_deq(0);
    test_simple_enq_deq(42);
    test_set_closed();
    test_set_closed_thread();
    test_flow_control(8, 10000);
    return 0;
}