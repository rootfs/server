/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "Copyright (c) 2007, 2008 Tokutek Inc.  All rights reserved."

#include "log-internal.h"
#include "toku_assert.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define dname __FILE__ ".dir"
#define rmrf "rm -rf " dname "/"

int main (int argc __attribute__((__unused__)),
	  char *argv[] __attribute__((__unused__))) {
    int r;
    long long lognum;
    system(rmrf);
    r = mkdir(dname, 0700);    assert(r==0);
    r = toku_logger_find_next_unused_log_file(dname,&lognum);
    assert(r==0 && lognum==0LL);

    r = creat(dname "/log01.tokulog", 0700); assert(r>=0);
    r = close(r); assert(r==0);

    r = toku_logger_find_next_unused_log_file(dname,&lognum);
    assert(r==0 && lognum==2LL);
    
    r = creat(dname "/log123456789012345.tokulog", 0700); assert(r>=0);
    r = close(r); assert(r==0);
    r = toku_logger_find_next_unused_log_file(dname,&lognum);
    assert(r==0 && lognum==123456789012346LL);

    r = creat(dname "/log3.tokulog", 0700); assert(r>=0);
    r = close(r); assert(r==0);
    r = toku_logger_find_next_unused_log_file(dname,&lognum);
    assert(r==0 && lognum==123456789012346LL);

    return 0;
}
