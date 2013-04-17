#ifndef TOKU_ASSERT_H
#define TOKU_ASSERT_H
/* The problem with assert.h:  If NDEBUG is set then it doesn't execute the function, if NDEBUG isn't set then we get a branch that isn't taken. */
/* This version will complain if NDEBUG is set. */
/* It evaluates the argument and then calls a function  toku_do_assert() which takes all the hits for the branches not taken. */

#ifdef NDEBUG
#error NDEBUG should not be set
#endif

void toku_do_assert(int,const char*/*expr_as_string*/,const char */*fun*/,const char*/*file*/,int/*line*/);

// Define SLOW_ASSERT if you want to get test-coverage information that ignores the assert statements.
#ifdef SLOW_ASSERT
#define assert(expr) toku_do_assert((expr) != 0, #expr, __FUNCTION__, __FILE__, __LINE__)
#else
#define assert(expr) ({ if ((expr)==0) toku_do_assert(0, #expr, __FUNCTION__, __FILE__, __LINE__); })
#endif
#endif