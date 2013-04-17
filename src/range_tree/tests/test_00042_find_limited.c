/* We are going to test whether we can insert and delete. */

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

void tests(BOOL allow_overlaps) {
    toku_interval query;
    toku_range insert;
    /*
        Limited/Unlimited Queries

        Limit of k does not produce all, but limit of 0 does.         Single point overlaps
    */

    /* Tree: {|0-1|,|2-3|,|4-5|,|6-7|,|8-9|}, query of |2-7|, limit 2 finds 2,
        limit 3 finds 3, limit 4 finds 3, limit 0 finds 3 */
    setup_tree(allow_overlaps, TRUE, 0, 1, 0);
    runinsert(0, init_range(&insert, 2, 3, 0)); 
    runinsert(0, init_range(&insert, 4, 5, 0)); 
    runinsert(0, init_range(&insert, 6, 7, 0)); 
    runinsert(0, init_range(&insert, 8, 9, 0));
    
    runlimitsearch(init_query(&query, 2, 7), 0, 3);
    runlimitsearch(init_query(&query, 2, 7), 1, 1);
    runlimitsearch(init_query(&query, 2, 7), 2, 2);
    runlimitsearch(init_query(&query, 2, 7), 3, 3);
    runlimitsearch(init_query(&query, 2, 7), 4, 3);
    close_tree();
    
    /* Tree is empty (return none) */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runlimitsearch(init_query(&query, 0, 0), 0, 0);
    close_tree();
    
    /* Tree contains only elements to the left. */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert, 1, 2, 0));
    runinsert(0, init_range(&insert, 3, 4, 0));
    runlimitsearch(init_query(&query, 8, 30), 0, 0);
    close_tree();
    
    /* Tree contains only elements to the right. */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert, 10, 20, 0));
    runinsert(0, init_range(&insert, 30, 40, 0));
    runlimitsearch(init_query(&query, 5, 7), 0, 0);
    close_tree();

    /* Tree contains only elements to the left and to the right. */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert, 10, 20, 0));
    runinsert(0, init_range(&insert, 30, 40, 0));
    runinsert(0, init_range(&insert, 70, 80, 0));
    runinsert(0, init_range(&insert, 90, 100, 0));
    runlimitsearch(init_query(&query, 60, 65), 0, 0);
    close_tree();
    
    /* Tree contains overlaps and elements to the left. */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert, 10, 20, 0));
    runinsert(0, init_range(&insert, 30, 40, 0));
    runinsert(0, init_range(&insert, 60, 80, 0));
    runinsert(0, init_range(&insert, 90, 100, 0));
    runlimitsearch(init_query(&query, 70, 95), 0, 2);
    close_tree();

    /* Tree contains overlaps and elements to the right. */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert, 110, 120, 0));
    runinsert(0, init_range(&insert, 130, 140, 0));
    runinsert(0, init_range(&insert, 60, 80, 0));
    runinsert(0, init_range(&insert, 90, 100, 0));
    runlimitsearch(init_query(&query, 70, 95), 0, 2);
    close_tree();

    /* Tree contains overlaps and elements to the left and to the right. */
    setup_tree(allow_overlaps, FALSE, 0, 0, 0);
    runinsert(0, init_range(&insert, 10, 20, 0));
    runinsert(0, init_range(&insert, 30, 40, 0));
    runinsert(0, init_range(&insert, 110, 120, 0));
    runinsert(0, init_range(&insert, 130, 140, 0));
    runinsert(0, init_range(&insert, 60, 80, 0));
    runinsert(0, init_range(&insert, 90, 100, 0));
    runlimitsearch(init_query(&query, 70, 95), 0, 2);
    close_tree();
}

int main(int argc, const char *argv[]) {
    parse_args(argc, argv);

    unsigned i;
    for (i = 0; i < sizeof(nums) / sizeof(nums[0]); i++) nums[i] = i; 
    buflen = 2;
    buf = (toku_range*)toku_malloc(2 * sizeof(toku_range));
    tests(FALSE);
#ifndef TOKU_RT_NOOVERLAPS
    tests(TRUE);
#endif
    tree = NULL;
    toku_free(buf);
    buf = NULL;
    return 0;
}