#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "toku_assert.h"
#include "memory.h"
#include "fifo.h"
#include "ybt.h"

struct fifo {
    int n_items_in_fifo;
    char *memory;       // An array of bytes into which fifo_entries are embedded.
    int   memory_size;  // How big is fifo_memory
    int   memory_start; // Where is the first used byte?
    int   memory_used;  // How many bytes are in use? 
};

const int fifo_initial_size = 4096;
static void fifo_init(struct fifo *fifo) {
    fifo->n_items_in_fifo = 0;
    fifo->memory       = 0;
    fifo->memory_size  = 0;
    fifo->memory_start = 0;
    fifo->memory_used  = 0;
}

static int fifo_entry_size(struct fifo_entry *entry) {
    return sizeof (struct fifo_entry) + entry->keylen + entry->vallen;
}

static struct fifo_entry *fifo_peek(struct fifo *fifo) {
    if (fifo->n_items_in_fifo == 0) return NULL;
    else return (struct fifo_entry *)(fifo->memory+fifo->memory_start);
}

int toku_fifo_create(FIFO *ptr) {
    struct fifo *MALLOC(fifo);
    if (fifo == 0) return ENOMEM;
    fifo_init(fifo);
    *ptr = fifo;
    return 0;
}

void toku_fifo_free(FIFO *ptr) {
    FIFO fifo = *ptr;
    if (fifo->memory) toku_free(fifo->memory);
    fifo->memory=0;
    toku_free(fifo);
    *ptr = 0;
}

int toku_fifo_n_entries(FIFO fifo) {
    return fifo->n_items_in_fifo;
}

static int next_power_of_two (int n) {
    int r = 4096;
    while (r < n) {
	r*=2;
	assert(r>0);
    }
    return r;
}

int toku_fifo_enq(FIFO fifo, const void *key, unsigned int keylen, const void *data, unsigned int datalen, int type, TXNID xid) {
    int need_space_here = sizeof(struct fifo_entry) + keylen + datalen;
    int need_space_total = fifo->memory_used+need_space_here;
    if (fifo->memory == NULL) {
	fifo->memory_size = next_power_of_two(need_space_total);
	fifo->memory = toku_malloc(fifo->memory_size);
    }
    if (fifo->memory_start+need_space_total > fifo->memory_size) {
	// Out of memory at the end.
	int next_2 = next_power_of_two(need_space_total);
	if ((2*next_2 > fifo->memory_size)
	    || (8*next_2 < fifo->memory_size)) {
	    // resize the fifo
	    char *newmem = toku_malloc(next_2);
	    char *oldmem = fifo->memory;
	    if (newmem==0) return ENOMEM;
	    memcpy(newmem, oldmem+fifo->memory_start, fifo->memory_used);
	    fifo->memory_size = next_2;
	    fifo->memory_start = 0;
	    fifo->memory = newmem;
	    toku_free(oldmem);
	} else {
	    // slide things over
	    memmove(fifo->memory, fifo->memory+fifo->memory_start, fifo->memory_used);
	    fifo->memory_start = 0;
	}
    }
    struct fifo_entry *entry = (struct fifo_entry *)(fifo->memory + fifo->memory_start + fifo->memory_used);
    entry->type = type;
    entry->xid  = xid;
    entry->keylen = keylen;
    memcpy(entry->key, key, keylen);
    entry->vallen = datalen;
    memcpy(entry->key + keylen, data, datalen);
    fifo->n_items_in_fifo++;
    fifo->memory_used += need_space_here;
    return 0;
}

int toku_fifo_enq_cmdstruct (FIFO fifo, const BRT_CMD cmd) {
    return toku_fifo_enq(fifo, cmd->u.id.key->data, cmd->u.id.key->size, cmd->u.id.val->data, cmd->u.id.val->size, cmd->type, cmd->xid);
}

/* peek at the head (the oldest entry) of the fifo */
int toku_fifo_peek(FIFO fifo, bytevec *key, unsigned int *keylen, bytevec *data, unsigned int *datalen, u_int32_t *type, TXNID *xid) {
    struct fifo_entry *entry = fifo_peek(fifo);
    if (entry == 0) return -1;
    *key = entry->key;
    *keylen = entry->keylen;
    *data = entry->key + entry->keylen;
    *datalen = entry->vallen;
    *type = entry->type;
    *xid  = entry->xid;
    return 0;
}

// fill in the BRT_CMD, using the two DBTs for the DBT part.
int toku_fifo_peek_cmdstruct (FIFO fifo, BRT_CMD cmd, DBT*key, DBT*data) {
    u_int32_t type;
    bytevec keyb,datab;
    unsigned int keylen,datalen;
    int r = toku_fifo_peek(fifo, &keyb, &keylen, &datab, &datalen, &type, &cmd->xid);
    if (r!=0) return r;
    cmd->type=type;
    toku_fill_dbt(key, keyb, keylen);
    toku_fill_dbt(data, datab, datalen);
    cmd->u.id.key=key;
    cmd->u.id.val=data;
    return 0;
}

int toku_fifo_deq(FIFO fifo) {
    if (fifo->n_items_in_fifo==0) return -1;
    struct fifo_entry * e = fifo_peek(fifo);
    assert(e);
    int used_here = fifo_entry_size(e);
    fifo->n_items_in_fifo--;
    fifo->memory_start+=used_here;
    fifo->memory_used -=used_here;
    return 0;
}

int toku_fifo_iterate_internal_start(FIFO fifo) { return fifo->memory_start; }
int toku_fifo_iterate_internal_has_more(FIFO fifo, int off) { return off < fifo->memory_start + fifo->memory_used; }
int toku_fifo_iterate_internal_next(FIFO fifo, int off) {
    struct fifo_entry *e = (struct fifo_entry *)(fifo->memory + off);
    return off + fifo_entry_size(e);
}
struct fifo_entry * toku_fifo_iterate_internal_get_entry(FIFO fifo, int off) {
    return (struct fifo_entry *)(fifo->memory + off);
}

void toku_fifo_iterate (FIFO fifo, void(*f)(bytevec key,ITEMLEN keylen,bytevec data,ITEMLEN datalen,int type, TXNID xid, void*), void *arg) {
    FIFO_ITERATE(fifo,
		 key, keylen, data, datalen, type, xid,
		 f(key,keylen,data,datalen,type,xid, arg));
}

unsigned long toku_fifo_memory_size(FIFO fifo) {
    return sizeof(*fifo)+fifo->memory_size;
}