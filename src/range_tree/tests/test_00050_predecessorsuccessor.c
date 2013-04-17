#include "test.h"

int nums[200];
char letters[2] = {'A','B'};

toku_range_tree *tree;
toku_range* buf;
unsigned buflen;

toku_interval* init_query(toku_interval* range, int left, int right) {
    range->left = (toku_point*)&nums[left];
    range->right = (toku_point*)&nums[right];
    return range;
}

toku_range* init_range(toku_range* range, int left, int right, int data) {
    init_query(&range->ends, left, right);
    if (data < 0)   range->data = 0;
    else            range->data = (TXNID)letters[data];
    return range;
}

void* init_point(unsigned left) {
    assert(left < sizeof(nums) / sizeof(nums[0]));
    return ((toku_point*)&nums[left]);
}

void setup_tree(BOOL allow_overlaps, BOOL insert, int left, int right, int data) {
    int r;
    toku_range range;
    r = toku_rt_create(&tree, int_cmp, char_cmp, allow_overlaps, malloc, free, realloc);
    CKERR(r);

    if (insert) {
        r = toku_rt_insert(tree, init_range(&range, left, right, data));
        CKERR(r);
    }
}
void close_tree(void) {
    int r;
    r = toku_rt_close(tree);    CKERR(r);
}

void runsearch(int rexpect, toku_interval* query, toku_range* expect) {
    int r;
    unsigned found;
    r = toku_rt_find(tree, query, 0, &buf, &buflen, &found);
    CKERR2(r, rexpect);
    
    if (rexpect != 0) return;
    assert(found == 1);
    assert(int_cmp(buf[0].ends.left, expect->ends.left) == 0 &&
           int_cmp(buf[0].ends.right, expect->ends.right) == 0 &&
           char_cmp(buf[0].data, expect->data) == 0);
}

void runinsert(int rexpect, toku_range* toinsert) {
    int r;
    r = toku_rt_insert(tree, toinsert);
    CKERR2(r, rexpect);
}

void runlimitsearch(toku_interval* query, unsigned limit, unsigned findexpect) {
    int r;
    unsigned found;
    r=toku_rt_find(tree, query, limit, &buf, &buflen, &found);  CKERR(r);
    verify_all_overlap(query, buf, found);
    
    assert(found == findexpect);
}

typedef enum {PRED=0, SUCC=1} predsucc;
void runtest(predsucc testtype, toku_point* query, BOOL findexpect,
             unsigned left, unsigned right, unsigned data) {
    int r;
    BOOL found;
    toku_range out;
    assert(data < sizeof(letters) / sizeof(letters[0]));
    assert(left < sizeof(nums) / sizeof(nums[0]));
    assert(right < sizeof(nums) / sizeof(nums[0]));
    if (testtype == PRED) {
        r = toku_rt_predecessor(tree, query, &out, &found);

    }
    else {
        assert(testtype == SUCC);
        r = toku_rt_successor(tree, query, &out, &found);
    }
    CKERR(r);
    assert(found == findexpect);
    if (findexpect) {
        assert(int_cmp(out.ends.left, (toku_point*)&nums[left]) == 0);
        assert(int_cmp(out.ends.right, (toku_point*)&nums[right]) == 0);
        assert(char_cmp(out.data, (TXNID)letters[data]) == 0);
    }
}


void tests(BOOL allow_overlaps) {
    toku_range insert;

    /*
        Empty
            Only empty space test.
        1 element
        standard tree
        Pred/Succ:
            1: In empty space
                * Nothing to the left/Right (pred/succ) respectively.
                * something to the left/right (pred/succ) respectively.
            2: On left endpoint.
                * Nothing to the left/Right (pred/succ) respectively.
                * something to the left/right (pred/succ) respectively.
            3: On right endpoint.
                * Nothing to the left/Right (pred/succ) respectively.
                * something to the left/right (pred/succ) respectively.
            4: In middle of range.
                * Nothing to the left/Right (pred/succ) respectively.
                * something to the left/right (pred/succ) respectively.
    */

    /* Empty tree. */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runtest(PRED, init_point(5), FALSE, 0, 0, 0);
    runtest(SUCC, init_point(5), FALSE, 0, 0, 0);
    close_tree();

    /* Single element tree.  Before left, left end point, middle,
       right end point, after right. */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert, 10, 20, 0)); 
    runtest(PRED, init_point(5),  FALSE, 0, 0, 0);
    runtest(PRED, init_point(10), FALSE, 0, 0, 0);
    runtest(PRED, init_point(15), FALSE, 0, 0, 0);
    runtest(PRED, init_point(20), FALSE, 0, 0, 0);
    runtest(PRED, init_point(25), TRUE, 10, 20, 0);
    runtest(SUCC, init_point(5),  TRUE, 10, 20, 0);
    runtest(SUCC, init_point(10), FALSE, 0, 0, 0);
    runtest(SUCC, init_point(15), FALSE, 0, 0, 0);
    runtest(SUCC, init_point(20), FALSE, 0, 0, 0);
    runtest(SUCC, init_point(25), FALSE, 0, 0, 0);
    close_tree();

    /*
        Swap left and right for succ.
        Multi element tree.
         * In empty space.
          * Something on left.
          * Nothing on left.

         * At a left end point.
          * Something on left.
          * Nothing on left.

         * Inside a range.
          * Something on left.
          * Nothing on left.

         * At a right end point.
          * Something on left.
          * Nothing on left.
    */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert, 10, 20, 0)); 
    runinsert(0, init_range(&insert, 30, 40, 0)); 

    /*
     * In empty space.
      * Something on left.
      * Nothing on left.
    */
    runtest(PRED, init_point(25), TRUE, 10, 20, 0);
    runtest(PRED, init_point(5), FALSE, 0, 0, 0);

    /*
     * At a left end point.
      * Something on left.
      * Nothing on left.
    */
    runtest(PRED, init_point(30), TRUE, 10, 20, 0);
    runtest(PRED, init_point(10), FALSE, 0, 0, 0);

    /*
     * Inside a range.
      * Something on left.
      * Nothing on left.
     */
    runtest(PRED, init_point(35), TRUE, 10, 20, 0);
    runtest(PRED, init_point(15), FALSE, 0, 0, 0);

    /*
     * At a right end point.
      * Something on left.
      * Nothing on left.
    */
    runtest(PRED, init_point(40), TRUE, 10, 20, 0);
    runtest(PRED, init_point(20), FALSE, 0, 0, 0);

    /*
     * In empty space.
      * Something on right.
      * Nothing on right.
    */
    runtest(SUCC, init_point(25), TRUE, 30, 40, 0);
    runtest(SUCC, init_point(45), FALSE, 0, 0, 0);

    /*
     * At a right end point.
      * Something on right.
      * Nothing on right.
    */
    runtest(SUCC, init_point(20), TRUE, 30, 40, 0);
    runtest(SUCC, init_point(40), FALSE, 0, 0, 0);

    /*
     * Inside a range.
      * Something on right.
      * Nothing on right.
     */
    runtest(SUCC, init_point(15), TRUE, 30, 40, 0);
    runtest(SUCC, init_point(35), FALSE, 0, 0, 0);

    /*
     * At a right end point.
      * Something on right.
      * Nothing on right.
    */
    runtest(SUCC, init_point(20), TRUE, 30, 40, 0);
    runtest(SUCC, init_point(40), FALSE, 0, 0, 0);

    close_tree();


    /*
        With other interval that cannot be the predecessor
        or the successor, but that need to be looked at.  */

    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert,  5, 7, 0)); 
    runinsert(0, init_range(&insert, 50, 60, 0)); 
    runinsert(0, init_range(&insert, 10, 20, 0)); 
    runinsert(0, init_range(&insert, 30, 40, 0)); 
    runinsert(0, init_range(&insert,  2, 4, 0)); 
    runinsert(0, init_range(&insert, 70, 80, 0)); 

    runtest(PRED, init_point(25), TRUE, 10, 20, 0);
    runtest(PRED, init_point(4), FALSE, 0, 0, 0);
    runtest(SUCC, init_point(25), TRUE, 30, 40, 0);
    runtest(SUCC, init_point(95), FALSE, 0, 0, 0);
    
    close_tree();
}

int main(int argc, const char *argv[]) {
    parse_args(argc, argv);

    unsigned i;
    
    for (i = 0; i < sizeof(nums) / sizeof(nums[0]); i++) nums[i] = i; 
    buflen = 2;
    buf = (toku_range*)toku_malloc(2 * sizeof(toku_range));
    tests(FALSE);

    tree = NULL;
    toku_free(buf);
    buf = NULL;
    return 0;
}