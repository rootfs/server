#include <sys/types.h>
#include <assert.h>
typedef struct value *OMTVALUE;
#include "omt.h"

enum { N=10 };
struct value { int x; } vs[N];
OMTVALUE ps[N];

static void test (void) {
    OMT o;
    OMTCURSOR curs, curs2, curs3;
    int i, r;
    OMTVALUE v;
    for (i=0; i<N; i++) {
	vs[i].x=i;
	ps[i]=&vs[i];
    }

    // destroy the omt first
    r = toku_omt_create_from_sorted_array(&o, ps, 10);  assert(r==0);
    r = toku_omt_cursor_create(&curs);                      assert(r==0);
    r = toku_omt_fetch(o, 5, &v, curs);                     assert(r==0);
    toku_omt_destroy(&o);
    toku_omt_cursor_destroy(&curs);
    
    // destroy the cursor first
    r = toku_omt_create_from_sorted_array(&o, ps, 10);  assert(r==0);
    r = toku_omt_cursor_create(&curs);                      assert(r==0);
    r = toku_omt_fetch(o, 5, &v, curs);                     assert(r==0);
    assert(v->x==5);
    r = toku_omt_cursor_next(curs, &v);
    assert(r==0 && v->x==6);
    r = toku_omt_cursor_prev(curs, &v);
    assert(r==0 && v->x==5);
    toku_omt_cursor_destroy(&curs);
    toku_omt_destroy(&o);

    // Create two cursors, destroy omt first
    r = toku_omt_create_from_sorted_array(&o, ps, 10);  assert(r==0);
    r = toku_omt_cursor_create(&curs);                      assert(r==0);
    r = toku_omt_fetch(o, 5, &v, curs);                     assert(r==0);
    r = toku_omt_cursor_create(&curs2);                      assert(r==0);
    r = toku_omt_fetch(o, 4, &v, curs2);                     assert(r==0);    
    r = toku_omt_cursor_next(curs, &v);    assert(r==0 && v->x==6);
    toku_omt_destroy(&o);
    toku_omt_cursor_destroy(&curs);
    toku_omt_cursor_destroy(&curs2);

    // Create two cursors, destroy them first
    r = toku_omt_create_from_sorted_array(&o, ps, 10);  assert(r==0);
    r = toku_omt_cursor_create(&curs);                      assert(r==0);
    r = toku_omt_fetch(o, 5, &v, curs);                     assert(r==0);
    r = toku_omt_cursor_create(&curs2);                      assert(r==0);
    r = toku_omt_fetch(o, 4, &v, curs2);                     assert(r==0);    
    r = toku_omt_cursor_next(curs, &v);    assert(r==0 && v->x==6);
    r = toku_omt_cursor_prev(curs2, &v);   assert(r==0 && v->x==3);
    toku_omt_cursor_destroy(&curs);
    r = toku_omt_cursor_prev(curs2, &v);   assert(r==0 && v->x==2);
    toku_omt_cursor_destroy(&curs2);
    toku_omt_destroy(&o);
    
    // Create three cursors, destroy them first
    r = toku_omt_create_from_sorted_array(&o, ps, 10);  assert(r==0);
    r = toku_omt_cursor_create(&curs);                      assert(r==0);
    r = toku_omt_fetch(o, 5, &v, curs);                     assert(r==0);
    r = toku_omt_cursor_create(&curs2);                      assert(r==0);
    r = toku_omt_fetch(o, 4, &v, curs2);                     assert(r==0);    
    r = toku_omt_cursor_create(&curs3);                      assert(r==0);
    r = toku_omt_fetch(o, 9, &v, curs3);                     assert(r==0);    
    r = toku_omt_cursor_next(curs, &v);    assert(r==0 && v->x==6);
    r = toku_omt_cursor_prev(curs2, &v);   assert(r==0 && v->x==3);
    r = toku_omt_cursor_next(curs3, &v);   assert(r!=0 && !toku_omt_cursor_is_valid(curs3));
    toku_omt_cursor_destroy(&curs);
    r = toku_omt_cursor_prev(curs2, &v);   assert(r==0 && v->x==2);
    r = toku_omt_cursor_prev(curs2, &v);   assert(r==0 && v->x==1);
    r = toku_omt_fetch(o, 1, &v, curs3);                     assert(r==0);
    r = toku_omt_cursor_prev(curs3, &v);   assert(r==0 && v->x==0);
    r = toku_omt_cursor_prev(curs3, &v);   assert(r!=0 && !toku_omt_cursor_is_valid(curs3));
    toku_omt_cursor_destroy(&curs2);
    toku_omt_destroy(&o);
    toku_omt_cursor_destroy(&curs3);
}

int main (int argc __attribute__((__unused__)), char *argv[] __attribute__((__unused__))) {
    test();
    return 0;
}