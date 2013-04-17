/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "Copyright (c) 2007,2008 Tokutek Inc.  All rights reserved."

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <db.h>

#include "test.h"

static DB_ENV *dbenv;
static DB *db;
static DB_TXN * txn;
static DBC *cursor;

void test_cursor_delete2 () {
    int r;
    DBT key,val;

    r = db_env_create(&dbenv, 0);                                                            CKERR(r);
    r = dbenv->open(dbenv, ENVDIR, DB_PRIVATE|DB_INIT_MPOOL|DB_CREATE|DB_INIT_TXN, 0);       CKERR(r);

    r = db_create(&db, dbenv, 0);                                                            CKERR(r);
    r = db->set_flags(db, DB_DUP|DB_DUPSORT);                                                CKERR(r);
    r = dbenv->txn_begin(dbenv, 0, &txn, 0);                                                 CKERR(r);
    r = db->open(db, txn, "primary.db", NULL, DB_BTREE, DB_CREATE, 0600);                    CKERR(r);
    r = txn->commit(txn, 0);                                                                 CKERR(r);

    r = dbenv->txn_begin(dbenv, 0, &txn, 0);                                                 CKERR(r);
    r = db->put(db, txn, dbt_init(&key, "a", 2), dbt_init(&val, "b", 2), DB_YESOVERWRITE);   CKERR(r);
    r = txn->commit(txn, 0);                                                                 CKERR(r);

    r = dbenv->txn_begin(dbenv, 0, &txn, 0);                                                 CKERR(r);
    r = db->del(db, txn, dbt_init(&key, "a", 2), 0);                                         CKERR(r);
    r = txn->commit(txn, 0);                                                                 CKERR(r);

    r = dbenv->txn_begin(dbenv, 0, &txn, 0);                                                 CKERR(r);
    r = db->put(db, txn, dbt_init(&key, "a", 2), dbt_init(&val, "c", 2), DB_YESOVERWRITE);   CKERR(r);

    cursor=cursor;

    r = db->cursor(db, txn, &cursor, 0);                                                     CKERR(r);
    r = cursor->c_get(cursor, dbt_init_malloc(&key), dbt_init_malloc(&val), DB_FIRST);       CKERR(r);
    assert(strcmp(key.data, "a")==0);  free(key.data);
    assert(strcmp(val.data, "c")==0);  free(val.data);
    r = cursor->c_del(cursor, 0);                                                            CKERR(r);
    r = cursor->c_del(cursor, 0);                                                            assert(r==DB_KEYEMPTY);
    r = cursor->c_get(cursor, dbt_init_malloc(&key), dbt_init_malloc(&val), DB_NEXT);        assert(r==DB_NOTFOUND);

    r = cursor->c_close(cursor);                                                             CKERR(r);
    r = txn->commit(txn, 0);                                                                 CKERR(r);



    r = db->close(db, 0);                                                                    CKERR(r);
    r = dbenv->close(dbenv, 0);                                                              CKERR(r);
}

int main(int argc, const char *argv[]) {

    parse_args(argc, argv);
  
    system("rm -rf " ENVDIR);
    mkdir(ENVDIR, 0777);
    
    test_cursor_delete2();

    return 0;
}