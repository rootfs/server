/* How fast can we do insertions when there are many files? */

#include <db.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "test.h"

#define NFILES 1000
#define NINSERTS_PER 1000

static DB_ENV *env;
static DB *dbs[NFILES];
DB_TXN *txn;

void setup (void) {
    system("rm -rf " ENVDIR);
    int r;
    r=mkdir(ENVDIR, 0777);       CKERR(r);

    r=db_env_create(&env, 0); CKERR(r);
    env->set_errfile(env, stderr);
    r=env->open(env, ENVDIR, DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN|DB_CREATE|DB_PRIVATE, 0777); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0); assert(r==0);

    int i;

    for (i=0; i<NFILES; i++) {
	char fname[20];
	snprintf(fname, sizeof(fname), "foo%d.db", i);
	r=db_create(&dbs[i], env, 0); CKERR(r);
	r = dbs[i]->set_pagesize(dbs[i], 4096);
	r=dbs[i]->open(dbs[i], txn, fname, 0, DB_BTREE, DB_CREATE, 0777); CKERR(r);
    }
    r=txn->commit(txn, 0);    assert(r==0);
}

void shutdown (void) {
    int i;
    int r;
    for (i=0; i<NFILES; i++) {
	r= dbs[i]->close(dbs[i], 0); CKERR(r);
    }
    r= env->close(env, 0); CKERR(r);
}

void doit (void) {
    int j;
    int r;
    struct timeval startt, endt;
    gettimeofday(&startt, 0);
    r=env->txn_begin(env, 0, &txn, 0); assert(r==0);
    for (j=0; j<NINSERTS_PER; j++) {
	int i;
	DBT key,data;
	char str[10];
	snprintf(str, sizeof(str), "%08d", j);
	dbt_init(&key, str, 1+strlen(str));
	dbt_init(&data, str, 1+strlen(str));
	for (i=0; i<NFILES; i++) {
	    r = dbs[i]->put(dbs[i], txn, &key, &data, DB_YESOVERWRITE);
	    assert(r==0);
	}
    }
    r=txn->commit(txn, 0); assert(r==0);
    gettimeofday(&endt, 0);
    long long ninserts = NINSERTS_PER * NFILES;
    double diff = (endt.tv_sec - startt.tv_sec) + 1e-6*(endt.tv_usec-startt.tv_usec);
    printf("%lld insertions in %9.6fs, %9.3f ins/s \n", ninserts, diff, ninserts/diff);
}

int main (int argc, const char *argv[]) {
    parse_args(argc, argv);

    setup();
    doit();
    shutdown();

    return 0;
}