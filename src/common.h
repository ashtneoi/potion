#ifndef COMMON_H
#define COMMON_H


#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#define _ATFILE_SOURCE

#include <errno.h>
#include <stdio.h>
#include <unistd.h>


// STR(s) does macro-expansion of s before stringifying it. (See "Argument Prescan"
// in the gcc manual.)
#define STR(s) #s

// TODO: add E_WANTF (like E_EXPECTF except for foreseeable errors)

// Example output:
//  Error (at src/nar.c:52): expectation `strlen(path) < 40` failed
#define EXPECT(cond) \
    do { \
        if (!(cond)) { \
            fprintf( \
                stderr, "Error (at %s:%d): expectation `%s` failed\n", __FILE__, __LINE__, \
                STR(cond) \
            ); \
            fflush(stderr); \
            exit(1); \
        } \
    } while(0)

// Example output:
//   Error (ENOENT at src/nar.c:52): expectation `0 < open(path, O_RDONLY)` failed
//   Error (errno 17 at src/nar.c:52): expectation `0 < open(path, O_RDONLY)` failed
#define E_EXPECT(cond) \
    do { \
        if (!(cond)) { \
            const char* n_str = errno_name(errno); \
            fprintf(stderr, "Error ("); \
            if (n_str) { \
                fprintf(stderr, "%s", n_str); \
            } else { \
                fprintf(stderr, "errno %d", errno); \
            } \
            fprintf( \
                stderr, " at %s:%d): expectation `%s` failed\n", __FILE__, __LINE__, STR(cond) \
            ); \
            fflush(stderr); \
            exit(1); \
        } \
    } while(0)

#define E_EXPECTF(cond, ...) \
    do { \
        if (!(cond)) { \
            const char* n_str = errno_name(errno); \
            fprintf(stderr, "Error ("); \
            if (n_str) { \
                fprintf(stderr, "%s", n_str); \
            } else { \
                fprintf(stderr, "errno %d", errno); \
            } \
            fprintf(stderr, " at %s:%d): ", __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
            fflush(stderr); \
            exit(1); \
        } \
    } while(0)

#define MIN(a, b) ((a) <= (b) ? (a) : (b))


const char* errno_name(int err)
{
    switch (err) {
        case EACCES: return "EACCES";
        case EBADF: return "EBADF";
        case EFAULT: return "EFAULT";
        case ELOOP: return "ELOOP";
        case ENAMETOOLONG: return "ENAMETOOLONG";
        case ENOENT: return "ENOENT";
        case ENOMEM: return "ENOMEM";
        case ENOTDIR: return "ENOTDIR";
        case ENOTTY: return "ENOTTY";
        case EOVERFLOW: return "EOVERFLOW";
        default: return NULL;
    }
}


#endif // COMMON_H
