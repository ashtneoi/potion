#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE
#define _ATFILE_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


// STR(s) does macro-expansion of s before stringifying it. (See "Argument Prescan"
// in the gcc manual.)
#define STR(s) #s

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
        case EOVERFLOW: return "EOVERFLOW";
        default: return NULL;
    }
}

int dirent_name_strcmp(const struct dirent** a, const struct dirent** b)
{
    return strcmp((*a)->d_name, (*b)->d_name);
}

void scandir_test(const char* path)
{
    struct dirent** names; // scandir initializes this
    int names_len = scandir(path, &names, NULL, dirent_name_strcmp);
    E_EXPECTF(names_len >= 0, "scandir failed on \"%s\"", path);

    int dir = open(path, O_RDONLY);
    E_EXPECTF(dir >= 0, "open failed on \"%s\"", path);

    for (int i = 0; i < names_len; i++) {
        if (0 == strcmp(names[i]->d_name, ".") || 0 == strcmp(names[i]->d_name, "..")) {
            continue;
        }

        struct stat stat_buf;
        E_EXPECT(0 == fstatat(dir, names[i]->d_name, &stat_buf, AT_SYMLINK_NOFOLLOW));

        char t;
        switch (stat_buf.st_mode & S_IFMT) { // file type
            case S_IFREG: t = 'r'; break;
            case S_IFLNK: t = 's'; break;
            case S_IFDIR: t = 'd'; break;
            default: t = '?'; break;
        }

        char p;
        if (!(stat_buf.st_mode & S_IFDIR) && stat_buf.st_mode & S_IXUSR) {
            p = 'x';
        } else {
            p = '-';
        }

        printf("%s/%s %c%c\n", path, names[i]->d_name, t, p);

        if (S_ISDIR(stat_buf.st_mode)) {
            char* dir_path = malloc(strlen(path) + 1 + strlen(names[i]->d_name) + 1);
            E_EXPECT(dir_path);
            sprintf(dir_path, "%s/%s", path, names[i]->d_name);
            scandir_test(dir_path);
        }
    }

    for (int i = 0; i < names_len; i++) {
        free(names[i]);
    }
    free(names);
}

int main(int argc, char** argv)
{
    EXPECT(argc == 2);

    char* path = argv[1];
    size_t path_len = strlen(path);
    while (path_len > 1 && path[path_len - 1] == '/') {
        path[path_len - 1] = 0;
        path_len--;
    }
    scandir_test(path);
}
