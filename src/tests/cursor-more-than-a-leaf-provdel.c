#include <db.h>
#include <sys/stat.h>
#include "test.h"

static DB_ENV *env;
static DB *db;
DB_TXN *txn;

const int num_insert = 25000;

void setup (void) {
    system("rm -rf " ENVDIR);
    int r;
    r=mkdir(ENVDIR, 0777);       CKERR(r);

    r=db_env_create(&env, 0); CKERR(r);
    env->set_errfile(env, stderr);
#if USE_BDB
    r=env->set_lk_max_objects(env, 2*num_insert); CKERR(r);
#endif
    r=env->set_lk_max_locks(env, 2*num_insert); CKERR(r);
    r=env->open(env, ENVDIR, DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|DB_INIT_TXN|DB_CREATE|DB_PRIVATE, 0777); CKERR(r);
    r=db_create(&db, env, 0); CKERR(r);

    r=env->txn_begin(env, 0, &txn, 0); assert(r==0);
    r=db->set_bt_compare(db, int_dbt_cmp); CKERR(r);
    r=db->set_dup_compare(db, int_dbt_cmp); CKERR(r);
    r=db->open(db, txn, "foo.db", 0, DB_BTREE, DB_CREATE, 0777); CKERR(r);
    r=txn->commit(txn, 0);    assert(r==0);
}

void shutdown (void) {
    int r;
    r= db->close(db, 0); CKERR(r);
    r= env->close(env, 0); CKERR(r);
}

void doit (BOOL committed_provdels) {
    DBT key,data;
    DBC *dbc;
    int r;
    int i;
    int j;

    r=env->txn_begin(env, 0, &txn, 0); CKERR(r);
    for (i = 0; i < num_insert; i++) {
        j = (i<<1) + 37;
        r=db->put(db, txn, dbt_init(&key, &i, sizeof(i)), dbt_init(&data, &j, sizeof(j)), DB_YESOVERWRITE);
    }
    r=txn->commit(txn, 0);    CKERR(r);
    r=env->txn_begin(env, 0, &txn, 0); CKERR(r);
    r = db->cursor(db, txn, &dbc, 0);                           CKERR(r);
    for (i = 0; i < num_insert; i++) {
        j = (i<<1) + 37;
        r = dbc->c_get(dbc, &key, &data, DB_NEXT); CKERR(r);
        assert(*(int*)key.data == i);
        assert(*(int*)data.data == j);
        r = dbc->c_del(dbc, 0); CKERR(r);
    }
    r = dbc->c_get(dbc, &key, &data, DB_NEXT); CKERR2(r, DB_NOTFOUND);
    r = dbc->c_get(dbc, &key, &data, DB_FIRST); CKERR2(r, DB_NOTFOUND);
    if (committed_provdels) {
        r = dbc->c_close(dbc);                                      CKERR(r);
        r=txn->commit(txn, 0);    CKERR(r);
        r=env->txn_begin(env, 0, &txn, 0); CKERR(r);
        r = db->cursor(db, txn, &dbc, 0);                           CKERR(r);
    }
    int ifirst, ilast, jfirst, jlast;
    ilast=2*num_insert;
    jlast=(ilast<<1)+37;
    ifirst=-1*num_insert;
    jfirst=(ifirst<<1)+37;
    r=db->put(db, txn, dbt_init(&key, &ifirst, sizeof(ifirst)), dbt_init(&data, &jfirst, sizeof(jfirst)), DB_YESOVERWRITE);
    CKERR(r);
    r=db->put(db, txn, dbt_init(&key, &ilast, sizeof(ilast)), dbt_init(&data, &jlast, sizeof(jlast)), DB_YESOVERWRITE);
    CKERR(r);

    r = dbc->c_get(dbc, dbt_init(&key, NULL, 0), dbt_init(&data, NULL, 0), DB_FIRST); CKERR(r);
    assert(*(int*)key.data == ifirst);
    assert(*(int*)data.data == jfirst);
    r = dbc->c_get(dbc, dbt_init(&key, NULL, 0), dbt_init(&data, NULL, 0), DB_NEXT); CKERR(r);
    assert(*(int*)key.data == ilast);
    assert(*(int*)data.data == jlast);
    r = dbc->c_get(dbc, dbt_init(&key, NULL, 0), dbt_init(&data, NULL, 0), DB_LAST); CKERR(r);
    assert(*(int*)key.data == ilast);
    assert(*(int*)data.data == jlast);
    r = dbc->c_get(dbc, dbt_init(&key, NULL, 0), dbt_init(&data, NULL, 0), DB_PREV); CKERR(r);
    assert(*(int*)key.data == ifirst);
    assert(*(int*)data.data == jfirst);
    r = dbc->c_close(dbc);                                      CKERR(r);
    r=txn->commit(txn, 0);    CKERR(r);
}

int main (int argc, const char *argv[]) {
    parse_args(argc, argv);

    setup();
    doit(TRUE);
    shutdown();
    setup();
    doit(FALSE);
    shutdown();

    return 0;
}
