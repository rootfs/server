#ifndef BRTTYPES_H
#define BRTTYPES_H

#ident "Copyright (c) 2007, 2008 Tokutek Inc.  All rights reserved."

#include <sys/types.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#define _FILE_OFFSET_BITS 64

#include "../include/db.h"
#include <inttypes.h>

typedef struct brt *BRT;
struct brt_header;
struct wbuf;

typedef unsigned int ITEMLEN;
typedef const void *bytevec;
//typedef const void *bytevec;

typedef int64_t DISKOFF;  /* Offset in a disk. -1 is the NULL pointer. */
typedef u_int64_t TXNID;
typedef struct s_blocknum { int64_t b; } BLOCKNUM; // make a struct so that we will notice type problems.

static inline BLOCKNUM make_blocknum(int64_t b) { BLOCKNUM result={b}; return result; }
static const BLOCKNUM header_blocknum = {0};

typedef struct {
    u_int32_t len;
    char *data;
} BYTESTRING;

/* Make the LSN be a struct instead of an integer so that we get better type checking. */
typedef struct __toku_lsn { u_int64_t lsn; } LSN;
#define ZERO_LSN ((LSN){0})

/* Make the FILEID a struct for the same reason. */
typedef struct __toku_fileid { u_int32_t fileid; } FILENUM;

typedef enum __toku_bool { FALSE=0, TRUE=1} BOOL;

typedef struct tokulogger *TOKULOGGER;
#define NULL_LOGGER ((TOKULOGGER)0)
typedef struct tokutxn    *TOKUTXN;
#define NULL_TXN ((TOKUTXN)0)

// The data that appears in the log to encode a brtheader. */
typedef struct loggedbrtheader {
    u_int32_t size;
    u_int32_t flags;
    u_int32_t nodesize;
    BLOCKNUM  free_blocks;
    BLOCKNUM  unused_blocks;
    int32_t n_named_roots; // -1 for the union below to be "one".
    union {
	struct {
	    char **names;
	    BLOCKNUM *roots;
	} many;
	struct {
	    BLOCKNUM  root;
	} one;
    } u;
} LOGGEDBRTHEADER; 

typedef struct intpairarray {
    u_int32_t size;
    struct intpair {
	u_int32_t a,b;
    } *array;
} INTPAIRARRAY;

typedef struct cachetable *CACHETABLE;
typedef struct cachefile *CACHEFILE;

/* tree command types */
enum brt_cmd_type {
    BRT_NONE = 0,
    BRT_INSERT = 1,
    BRT_DELETE_ANY = 2,  // Delete any matching key.  This used to be called BRT_DELETE.
    BRT_DELETE_BOTH = 3,
    BRT_ABORT_ANY = 4,   // Abort any commands on any matching key.
    BRT_ABORT_BOTH  = 5, // Abort commands that match both the key and the value
    BRT_COMMIT_ANY  = 6,
    BRT_COMMIT_BOTH = 7
};

/* tree commands */
struct brt_cmd {
    enum brt_cmd_type type;
    TXNID xid;
    union {
        /* insert or delete */
        struct brt_cmd_insert_delete {
            DBT *key;
            DBT *val;
        } id;
    } u;
};
typedef struct brt_cmd BRT_CMD_S, *BRT_CMD;

#define UU(x) x __attribute__((__unused__))

typedef struct leafentry *LEAFENTRY;

#endif