#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE
#define _ATFILE_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
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


void serialize_prime(const char* path);

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

int dirent_name_strcmp(const struct dirent** a, const struct dirent** b)
{
    return strcmp((*a)->d_name, (*b)->d_name);
}

void serialize_u64(uint64_t n)
{
    for (int i = 0; i < 8; i++) {
        uint8_t b = n & 0xFF;
        n >>= 8;
        E_EXPECTF(1 == write(STDOUT_FILENO, &b, 1), "write failed");
    }
}

void serialize_str(const char* s)
{
    size_t remaining = strlen(s); // don't write the null terminator
    serialize_u64(remaining);
    while (remaining > 0) {
        ssize_t count = write(STDOUT_FILENO, s, remaining);
        E_EXPECTF(count >= 0, "write failed");
        remaining -= (size_t)count;
        s += count;
    }
    // TODO: pad it!
}

// The caller must free the returned string.
char* path_join(const char* a, const char* b)
{
    char* joined = malloc(strlen(a) + 1 + strlen(b) + 1);
    E_EXPECT(joined);
    int a_is_root = a[0] == '/' && a[1] == 0;
    sprintf(joined, "%s/%s", a_is_root ? "" : a, b);
    return joined;
}

void serialize_entry(const char* path, const char* name)
{
    serialize_str("entry");
    serialize_str("(");
    serialize_str("name");
    serialize_str(name);
    serialize_str("node");
    char* child_path = path_join(path, name);
    serialize_prime(child_path);
    free(child_path);
    serialize_str(")");
}

void serialize_prime_prime(const char* path)
{
    struct stat stat_buf;
    E_EXPECT(0 == lstat(path, &stat_buf));

    serialize_str("type");
    switch (stat_buf.st_mode & S_IFMT) { // file type
        case S_IFREG:
            serialize_str("regular");
            if (stat_buf.st_mode & S_IXUSR) {
                serialize_str("executable");
                serialize_str("");
            }
            serialize_str("contents");
            serialize_str("TODO"); // TODO
            break;
        case S_IFLNK:
            serialize_str("symlink");
            serialize_str("target");
            serialize_str("TODO"); // TODO
            break;
        case S_IFDIR:
            serialize_str("directory");

            struct dirent** names; // scandir allocates the list and its contents
            int names_len = scandir(path, &names, NULL, dirent_name_strcmp);
            E_EXPECTF(names_len >= 0, "scandir failed on \"%s\"", path);

            int dir = open(path, O_RDONLY);
            E_EXPECTF(dir >= 0, "open failed on \"%s\"", path);

            for (int i = 0; i < names_len; i++) {
                if (0 == strcmp(names[i]->d_name, ".") || 0 == strcmp(names[i]->d_name, "..")) {
                    continue;
                }

                serialize_entry(path, names[i]->d_name);
            }

            for (int i = 0; i < names_len; i++) {
                free(names[i]);
            }
            free(names);
            break;
        default:
            fprintf(stderr, "Error: file \"%s\" has unknown or illegal type", path);
            exit(1);
    }
}

void serialize_prime(const char* path)
{
    serialize_str("(");
    serialize_prime_prime(path);
    serialize_str(")");
}

void serialize(const char* path)
{
    serialize_str("nix-archive-1");
    serialize_prime(path);
}

int main(int argc, char** argv)
{
    EXPECT(argc == 2);

    int is_tty = isatty(STDOUT_FILENO);
    E_EXPECT(is_tty >= 0);
    if (is_tty) {
        fprintf(stderr, "Error: stdout is a TTY but NAR archives are binary data\n");
        exit(1);
    }

    char* path = argv[1];
    size_t path_len = strlen(path);
    while (path_len > 1 && path[path_len - 1] == '/') {
        path[path_len - 1] = 0;
        path_len--;
    }
    EXPECT(strlen(path) != 0);
    serialize(path);
}
