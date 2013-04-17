/* We are going to test whether create and close properly check their input. */

#include "test.h"

toku_range_tree* toku__lt_ifexist_selfwrite(toku_lock_tree* tree, TXNID txn);
toku_range_tree* toku__lt_ifexist_selfread(toku_lock_tree* tree, TXNID txn);

int r;
toku_lock_tree* lt  = NULL;
toku_ltm*       ltm = NULL;
DB*             db  = (DB*)1;
u_int32_t max_locks = 10;
BOOL duplicates = FALSE;
int  nums[10000];

DBT _key_left[2];
DBT _key_right[2];
DBT _data_left[2];
DBT _data_right[2];
DBT* key_left[2]   ;
DBT* key_right[2]  ;
DBT* data_left [2] ;
DBT* data_right[2] ;

toku_point qleft, qright;
toku_interval query;
toku_range* buf;
unsigned buflen;
unsigned numfound;

void init_query(BOOL dups) {  
    init_point(&qleft,  lt);
    init_point(&qright, lt);
    
    qleft.key_payload  = (void *) toku_lt_neg_infinity;
    qright.key_payload = (void *) toku_lt_infinity;

    if (dups) {
        qleft.data_payload  = qleft.key_payload;
        qright.data_payload = qright.key_payload;
    }

    memset(&query,0,sizeof(query));
    query.left  = &qleft;
    query.right = &qright;
}

void setup_tree(BOOL dups) {
    assert(!lt && !ltm);
    r = toku_ltm_create(&ltm, max_locks, dbpanic,
                        get_compare_fun_from_db, get_dup_compare_from_db,
                        toku_malloc, toku_free, toku_realloc);
    CKERR(r);
    assert(ltm);
    r = toku_lt_create(&lt, dups, dbpanic, ltm,
                       get_compare_fun_from_db, get_dup_compare_from_db,
                       toku_malloc, toku_free, toku_realloc);
    CKERR(r);
    assert(lt);
    init_query(dups);
}

void close_tree(void) {
    assert(lt && ltm);
    r = toku_lt_close(lt);
        CKERR(r);
    r = toku_ltm_close(ltm);
        CKERR(r);
    lt = NULL;
    ltm = NULL;
}

typedef enum { null = -1, infinite = -2, neg_infinite = -3 } lt_infty;

DBT* set_to_infty(DBT *dbt, lt_infty value) {
    if (value == infinite) return (DBT*)toku_lt_infinity;
    if (value == neg_infinite) return (DBT*)toku_lt_neg_infinity;
    if (value == null) return dbt_init(dbt, NULL, 0);
    assert(value >= 0);
    return                    dbt_init(dbt, &nums[value], sizeof(nums[0]));
}


void lt_insert(BOOL dups, int r_expect, char txn, int key_l, int data_l, 
               int key_r, int data_r, BOOL read_flag) {
    DBT _key_left;
    DBT _key_right;
    DBT _data_left;
    DBT _data_right;
    DBT* key_left   = &_key_left;
    DBT* key_right  = &_key_right;
    DBT* data_left  = dups ? &_data_left : NULL;
    DBT* data_right = dups ? &_data_right: NULL;

    key_left  = set_to_infty(key_left,  key_l);
    key_right = set_to_infty(key_right, key_r);
    if (dups) {
        if (key_left != &_key_left) data_left = key_left;
        else data_left = set_to_infty(data_left,  data_l);
        if (key_right != &_key_right) data_right = key_right;
        else data_right = set_to_infty(data_right,  data_r);
        assert(key_left  && data_left);
        assert(!read_flag || (key_right && data_right));
    } else {
        data_left = data_right = NULL;
        assert(key_left  && !data_left);
        assert(!read_flag || (key_right && !data_right));
    }

    TXNID local_txn = (TXNID) (size_t) txn;

    if (read_flag)
        r = toku_lt_acquire_range_read_lock(lt, db, local_txn,
                                            key_left,  data_left,
                                            key_right, data_right);
    else
        r = toku_lt_acquire_write_lock(lt, db, local_txn, key_left, data_left);
    CKERR2(r, r_expect);
}

void lt_insert_read(BOOL dups, int r_expect, char txn, int key_l, int data_l, 
                    int key_r, int data_r) {
    lt_insert(dups, r_expect, txn, key_l, data_l, key_r, data_r, TRUE);
}

void lt_insert_write(BOOL dups, int r_expect, char txn, int key_l, int data_l) {
    lt_insert(dups, r_expect, txn, key_l, data_l, 0, 0, FALSE);
}


void setup_payload_len(void** payload, u_int32_t* len, int val) {
    assert(payload && len);

    DBT temp;

    *payload = set_to_infty(&temp, val);
    
    if (val < 0) {
        *len = 0;
    }
    else {
        *len = sizeof(nums[0]);
        *payload = temp.data;
    }
}

void temporarily_fake_comparison_functions(void) {
    assert(!lt->db && !lt->compare_fun && !lt->dup_compare);
    lt->db = db;
    lt->compare_fun = get_compare_fun_from_db(db);
    lt->dup_compare = get_dup_compare_from_db(db);
}

void stop_fake_comparison_functions(void) {
    assert(lt->db && lt->compare_fun && lt->dup_compare);
    lt->db = NULL;
    lt->compare_fun = NULL;
    lt->dup_compare = NULL;
}

void lt_find(BOOL dups, toku_range_tree* rt,
                        unsigned k, int key_l, int data_l,
                                    int key_r, int data_r,
                                    char char_txn) {
temporarily_fake_comparison_functions();
    r = toku_rt_find(rt, &query, 0, &buf, &buflen, &numfound);
    CKERR(r);
    assert(numfound==k);

    TXNID find_txn = (TXNID) (size_t) char_txn;

    toku_point left, right;
    init_point(&left, lt);
    setup_payload_len(&left.key_payload, &left.key_len, key_l);
    if (dups) {
        if (key_l < null) left.data_payload = left.key_payload;
        else setup_payload_len(&left.data_payload, &left.data_len, data_l);
    }
    init_point(&right, lt);
    setup_payload_len(&right.key_payload, &right.key_len, key_r);
    if (dups) {
        if (key_r < null) right.data_payload = right.key_payload;
        else setup_payload_len(&right.data_payload, &right.data_len, data_r);
    }
    unsigned i;
    for (i = 0; i < numfound; i++) {
        if (toku__lt_point_cmp(buf[i].ends.left,  &left ) == 0 &&
            toku__lt_point_cmp(buf[i].ends.right, &right) == 0 &&
            buf[i].data == find_txn) { goto cleanup; }
    }
    assert(FALSE);  //Crash since we didn't find it.
cleanup:
    stop_fake_comparison_functions();
}

void lt_unlock(char ctxn) {
  int r;
  r = toku_lt_unlock(lt, (TXNID) (size_t) ctxn);
  CKERR(r);
}
              
void run_escalation_test(BOOL dups) {
    int i = 0;
/* ******************** */
/* 1 transaction request 1000 write locks, make sure it succeeds*/
    setup_tree(dups);
    assert(lt->lock_escalation_allowed);
    for (i = 0; i < 1000; i++) {
        lt_insert_write(dups, 0, 'a', i, i);
        assert(lt->lock_escalation_allowed);
    }
    close_tree();
/* ******************** */
/* interleaving transactions,
   TXN A grabs 1 3 5 7 9 
   TXN B grabs 2 4 6 8 10
   make sure lock escalation fails, and that we run out of locks */
    setup_tree(dups);
    // this should grab ten locks successfully
    for (i = 1; i < 10; i+=2) {
        lt_insert_write(dups, 0, 'a', i, i);
        lt_insert_write(dups, 0, 'b', i+1, i+1);
    }
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'a', 100, 100);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'b', 100, 100);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'c', 100, 100);
    close_tree();
/* ******************** */
/*
   test that escalation allowed flag goes from FALSE->TRUE->FALSE
   TXN A grabs 1 3 5 7 9 
   TXN B grabs 2 4 6 8 10
   try to grab another lock, fail, lock escalation should be disabled
   txn B gets freed
   lock escalation should be reenabled
   txn C grabs 60,70,80,90,100
   lock escalation should work
*/
    setup_tree(dups);
    assert(lt->lock_escalation_allowed);
    // this should grab ten locks successfully
    for (i = 1; i < 10; i+=2) {
        lt_insert_write(dups, 0, 'a', i, i);
        lt_insert_write(dups, 0, 'b', i+1, i+1);
    }
    assert(lt->lock_escalation_allowed);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'a', 100, 100);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'b', 100, 100);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'c', 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'a', 100, 100, 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'b', 100, 100, 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'c', 100, 100, 100, 100);
    lt_unlock('b');
    assert(lt->lock_escalation_allowed);
    for (i = 50; i < 1000; i++) {
        lt_insert_write(dups, 0, 'c', i, i);
        assert(lt->lock_escalation_allowed);
    }
    close_tree();
/* ******************** */
/*
   txn A grabs 0,1,2,...,8  (9 locks)
   txn B grabs read lock [5,7]
   txn C attempts to grab lock, escalation, and lock grab, should fail
   lock
*/
    setup_tree(dups);
    assert(lt->lock_escalation_allowed);
    // this should grab ten locks successfully
    for (i = 0; i < 10; i ++) { 
        if (i == 2 || i == 5) { continue; }
        lt_insert_write(dups, 0, 'a', i, i);
    }
    lt_insert_read (dups, 0, 'b', 5, 5, 5, 5);
    lt_insert_read (dups, 0, 'b', 2, 2, 2, 2);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'a', 100, 100);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'b', 100, 100);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'c', 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'a', 100, 100, 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'b', 100, 100, 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'c', 100, 100, 100, 100);
    lt_unlock('b');
    assert(lt->lock_escalation_allowed);
    for (i = 50; i < 1000; i++) {
        lt_insert_write(dups, 0, 'c', i, i);
        assert(lt->lock_escalation_allowed);
    }
    close_tree();
/* ******************** */
#if 0 //Only use when messy transactions are enabled.
/*
   txn A grabs 0,1,2,...,8  (9 locks)
   txn B grabs read lock [5,7]
   txn C attempts to grab lock, escalation, and lock grab, should fail
   lock
*/
    setup_tree(dups);
    assert(lt->lock_escalation_allowed);
    // this should grab ten locks successfully
    for (i = 0; i < 7; i++) { 
        lt_insert_write(dups, 0, 'a', i, i);
    }
    lt_insert_read (dups, 0, 'b', 5, 5, 6, 6);
    lt_insert_read (dups, 0, 'b', 2, 2, 3, 3);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'a', 100, 100);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'b', 100, 100);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'c', 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'a', 100, 100, 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'b', 100, 100, 100, 100);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'c', 100, 100, 100, 100);
    lt_unlock('b');
    assert(lt->lock_escalation_allowed);
    for (i = 50; i < 1000; i++) {
        lt_insert_write(dups, 0, 'c', i, i);
        assert(lt->lock_escalation_allowed);
    }
    close_tree();
#endif
/* ******************** */
/* escalate on read lock, */
    setup_tree(dups);
    for (i = 0; i < 10; i++) {
        lt_insert_write(dups, 0, 'a', i, i);
    }
    lt_insert_read(dups, 0, 'a', 10, 10, 10, 10);
    close_tree();
/* ******************** */
/* escalate on read lock of different transaction. */
    setup_tree(dups);
    for (i = 0; i < 10; i++) {
        lt_insert_write(dups, 0, 'a', i, i);
    }
    lt_insert_read(dups, 0, 'b', 10, 10, 10, 10);
    close_tree();
/* ******************** */
/* txn A grabs write lock 0,9
   txn A grabs read lock 1,2,3,4,5,6,7,8
   txn B grabs write lock 11, 12, should succeed */
    setup_tree(dups);
    for (i = 1; i < 9; i++) {
        lt_insert_read(dups, 0, 'a', i, i, i, i);
    }
    lt_insert_write(dups, 0, 'a', 0, 0);
    lt_insert_write(dups, 0, 'a', 9, 9);
    for (i = 50; i < 1000; i++) {
        lt_insert_write(dups, 0, 'b', i, i);
        assert(lt->lock_escalation_allowed);
    }
    close_tree();
/* ******************** */
/* [1-A-5]   [10-B-15]   [20-A-25]  BORDER WRITE
    [2B]  [6C] [12A]       [22A]    READ LOCKS
    check that only last borderwrite range is escalated */
    setup_tree(dups);
    lt_insert_write(dups, 0, 'a', 1, 1);
    lt_insert_write(dups, 0, 'a', 5, 5);
    lt_insert_write(dups, 0, 'b', 10, 10);
    lt_insert_write(dups, 0, 'b', 15, 15);
    lt_insert_write(dups, 0, 'a', 20, 20);
    lt_insert_write(dups, 0, 'a', 23, 23);
    lt_insert_write(dups, 0, 'a', 25, 25);

    lt_insert_read(dups, 0, 'b', 2, 2, 2, 2);
    lt_insert_read(dups, 0, 'a', 12, 12, 12, 12);
    lt_insert_read(dups, 0, 'a', 22, 22, 22, 22);
    
    lt_insert_read(dups, 0, 'a', 100, 100, 100, 100);

    lt_insert_write(dups, DB_LOCK_NOTGRANTED, 'b', 24, 24);
    lt_insert_write(dups, 0, 'a', 14, 14);
    lt_insert_write(dups, 0, 'b', 4, 4);
    close_tree();
/* ******************** */
/* Test read lock escalation, no writes. */
    setup_tree(dups);
    assert(lt->lock_escalation_allowed);
    for (i = 0; i < 1000; i ++) { 
        lt_insert_read (dups, 0, 'b', i, i, i, i);
    }
    close_tree();
/* ******************** */
/* Test read lock escalation, writes of same kind. */
    setup_tree(dups);
    assert(lt->lock_escalation_allowed);
    lt_insert_write(dups, 0, 'b', 5, 5);
    lt_insert_write(dups, 0, 'b', 10, 10);
    for (i = 0; i < 1000; i ++) {   
        lt_insert_read (dups, 0, 'b', i, i, i, i);
    }
    close_tree();
/* ******************** */
/* Test read lock escalation, writes of other kind. */
    setup_tree(dups);
    assert(lt->lock_escalation_allowed);
    lt_insert_write(dups, 0, 'a', 0, 0);
    lt_insert_write(dups, 0, 'b', 5, 5);
    lt_insert_write(dups, 0, 'a', 7, 7);
    lt_insert_write(dups, 0, 'c', 10, 10);
    lt_insert_write(dups, 0, 'a', 13, 13);
    for (i = 0; i < 1000; i ++) {  
        if (i % 5 == 0) { continue; }
        lt_insert_read (dups, 0, 'a', i, i, i, i);
    }
    close_tree();
/* ******************** */
/*
   txn A grabs 0,1,2,...,8  (9 locks) (all numbers * 10)
   txn B grabs read lock [5,7] but grabs many there
   txn C attempts to grab lock, escalation, and lock grab, should fail
   lock
*/
/*
    setup_tree(dups);
    assert(lt->lock_escalation_allowed);
    // this should grab ten locks successfully
    for (i = 0; i < 9; i ++) { 
        if (i == 2 || i == 5) { continue; }
        lt_insert_write(dups, 0, 'a', i*10, i*10);
    }
    for (i = 0; i < 10; i++) {
        lt_insert_read (dups, 0, 'b', 50+i, 50+i, 50+i, 50+i);
    }
    lt_insert_write(dups, 0, 'a', 9*10, 9*10);
    lt_insert_read (dups, 0, 'b', 20, 20, 20, 20);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'a', 1000, 1000);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'b', 1000, 1000);
    lt_insert_write(dups, TOKUDB_OUT_OF_LOCKS, 'c', 1000, 1000);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'a', 1000, 1000, 1000, 1000);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'b', 1000, 1000, 1000, 1000);
    lt_insert_read(dups, TOKUDB_OUT_OF_LOCKS, 'c', 1000, 1000, 1000, 1000);
    lt_unlock('b');
    assert(lt->lock_escalation_allowed);
    for (i = 100; i < 1000; i++) {
        lt_insert_write(dups, 0, 'c', i, i);
        assert(lt->lock_escalation_allowed);
    }
    close_tree();
*/
/* ******************** */
}

void init_test(void) {
    unsigned i;
    for (i = 0; i < sizeof(nums)/sizeof(nums[0]); i++) nums[i] = i;

    buflen = 64;
    buf = (toku_range*) toku_malloc(buflen*sizeof(toku_range));
    compare_fun = intcmp;
    dup_compare = intcmp;
}





int main(int argc, const char *argv[]) {
    parse_args(argc, argv);

    init_test();

    run_escalation_test(FALSE);
    run_escalation_test(TRUE);

    return 0;
}