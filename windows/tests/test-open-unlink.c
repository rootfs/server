#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <sys/stat.h>

#ifndef S_IRUSR
#define S_IRUSR S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR S_IWRITE
#endif

const char TESTFILE[] = "test-open-unlink-file";

int main(void) {
    int r;
    int fd;

    system("rm -rf test-open-unlink-file");
    
    fd = open(TESTFILE, O_CREAT+O_RDWR, S_IRUSR+S_IWUSR);
    assert(fd != -1);

    r = unlink(TESTFILE);
    printf("%s:%d unlink %d %d\n", __FILE__, __LINE__, r, errno); fflush(stdout);
#if defined(__linux__)
    assert(r == 0);

    r = close(fd);
    assert(r == 0);
#endif
#if defined(_WIN32)
    assert(r == -1);
    
    r = close(fd);
    assert(r == 0);
    
    r = unlink(TESTFILE);
    printf("%s:%d unlink %d %d\n", __FILE__, __LINE__, r, errno); fflush(stdout);
    assert(r == 0);
#endif

    return 0;
}