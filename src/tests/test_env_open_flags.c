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

void test_env_open_flags(int env_open_flags, int expectr) {
    if (verbose) printf("test_env_open_flags:%d\n", env_open_flags);

    DB_ENV *env;
    int r;

    r = db_env_create(&env, 0);
    assert(r == 0);
    env->set_errfile(env, 0);

    r = env->open(env, ENVDIR, env_open_flags, 0644);
    if (r != expectr && verbose) printf("env open flags=%x expectr=%d r=%d\n", env_open_flags, expectr, r);

    r = env->close(env, 0);
    assert(r == 0);
}

int main(int argc, const char *argv[]) {

    parse_args(argc, argv);
  
    system("rm -rf " ENVDIR);
    mkdir(ENVDIR, 0777);

#ifdef USE_TDB
    toku_set_trace_file(ENVDIR "/trace.tktrace");
#endif

    /* test flags */
    test_env_open_flags(0, ENOENT);
#ifdef TOKUDB
    // This one segfaults in BDB 4.6.21
    test_env_open_flags(DB_PRIVATE, ENOENT);
#endif
    test_env_open_flags(DB_PRIVATE+DB_CREATE, 0);
    test_env_open_flags(DB_PRIVATE+DB_CREATE+DB_INIT_MPOOL, 0);
    test_env_open_flags(DB_PRIVATE+DB_RECOVER, EINVAL);
    test_env_open_flags(DB_PRIVATE+DB_CREATE+DB_INIT_MPOOL+DB_RECOVER, EINVAL);

#ifdef USE_TDB
    toku_close_trace_file();
#endif

    return 0;
}