/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "Copyright (c) 2007 Tokutek Inc.  All rights reserved."

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

int db_put(DB *db, DB_TXN *txn, int k, int v) {
    DBT key, val;
    return db->put(db, txn, dbt_init(&key, &k, sizeof k), dbt_init(&val, &v, sizeof v), DB_NOOVERWRITE);
}

void test_txn_recover3(int nrows) {
    if (verbose) printf("test_txn_recover1:%d\n", nrows);

    system("rm -rf " ENVDIR);
    mkdir(ENVDIR, 0777);
    mkdir(ENVDIR "/" "t.tokudb", 0777);

    DB_ENV *env;
    DB *mdb, *sdb;
    DB_TXN * const null_txn = 0;
    const char * const fname = "t.tokudb/main.brt";
    const char * const sname = "t.tokudb/status.brt";
    int r;

    r = db_env_create(&env, 0);        assert(r == 0);
    env->set_errfile(env, stderr);
    r = env->open(env, ENVDIR, DB_CREATE|DB_INIT_MPOOL|DB_INIT_TXN|DB_INIT_LOCK|DB_INIT_LOG |DB_THREAD |DB_PRIVATE | DB_RECOVER, 0777); CKERR(r);

    r = db_create(&mdb, env, 0); assert(r == 0);
    mdb->set_errfile(mdb,stderr); // Turn off those annoying errors
    r = mdb->open(mdb, null_txn, fname, NULL, DB_BTREE, DB_CREATE+DB_THREAD+DB_AUTO_COMMIT, 0666); assert(r == 0);
    r = mdb->close(mdb, 0); assert(r == 0);

    r = db_create(&sdb, env, 0); assert(r == 0);
    sdb->set_errfile(sdb,stderr); // Turn off those annoying errors
    r = sdb->open(sdb, null_txn, sname, NULL, DB_BTREE, DB_CREATE+DB_THREAD+DB_AUTO_COMMIT, 0666); assert(r == 0);
    r = sdb->close(sdb, 0); assert(r == 0);

    r = db_create(&mdb, env, 0); assert(r == 0);
    mdb->set_errfile(mdb,stderr); // Turn off those annoying errors
    r = mdb->open(mdb, null_txn, fname, NULL, DB_BTREE, DB_CREATE+DB_THREAD+DB_AUTO_COMMIT, 0666); assert(r == 0);

    r = db_create(&sdb, env, 0); assert(r == 0);
    sdb->set_errfile(sdb,stderr); // Turn off those annoying errors
    r = sdb->open(sdb, null_txn, sname, NULL, DB_BTREE, DB_CREATE+DB_THREAD+DB_AUTO_COMMIT, 0666); assert(r == 0);


    DB_TXN *txn;
    r = env->txn_begin(env, null_txn, &txn, 0); assert(r == 0);

    int i;
    for (i=0; i<nrows; i++) {
        int k = htonl(i);
        int v = htonl(i);
        DBT key, val;
        r = mdb->put(mdb, txn, dbt_init(&key, &k, sizeof k), dbt_init(&val, &v, sizeof v), 0);
        assert(r == 0); 
        r = sdb->put(sdb, txn, dbt_init(&key, &k, sizeof k), dbt_init(&val, &v, sizeof v), 0);
        assert(r == 0); 
    }
   
    r = txn->commit(txn, 0); assert(r == 0);

    r = mdb->close(mdb, 0); assert(r == 0);
    r = sdb->close(sdb, 0); assert(r == 0);

    r = env->txn_checkpoint(env, 0, 0, 0); assert(r == 0);

    char **names;
    r = env->log_archive(env, &names, 0); assert(r == 0);
    if (names) {
        for (i=0; names[i]; i++)
            printf("%d:%s\n", i, names[i]);
        free(names);
    }

    r = env->close(env, 0); assert(r == 0);

    r = db_env_create(&env, 0);        assert(r == 0);
    env->set_errfile(env, stderr);
    r = env->open(env, ENVDIR, DB_CREATE|DB_INIT_MPOOL|DB_INIT_TXN|DB_INIT_LOCK|DB_INIT_LOG |DB_THREAD |DB_PRIVATE | DB_RECOVER, 0777); CKERR(r);
    r = env->close(env, 0); assert(r == 0);
}

int main(int argc, const char *argv[]) {

    parse_args(argc, argv);
  
    test_txn_recover3(1);

    return 0;
}