#include "common.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


void serialize_prime(const char* path);


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

void write_all_to_stdout(const char* buf, size_t size)
{
    size_t remaining = size;
    while (remaining > 0) {
        ssize_t count = write(STDOUT_FILENO, buf, remaining);
        E_EXPECTF(count >= 0, "write failed on stdout");
        remaining -= (size_t)count;
        buf += count;
    }
}

static char EIGHT_ZEROES[8] = { 0 };

void serialize_str(const char* s)
{
    size_t size = strlen(s); // don't write the null terminator
    serialize_u64(size);
    write_all_to_stdout(s, size);
    write_all_to_stdout(EIGHT_ZEROES, (8 - (size % 8)) % 8);
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

void dump_file(const char* path, size_t size)
{
    size_t read_remaining = size;
    int f = open(path, O_RDONLY);
    E_EXPECTF(f >= 0, "open failed on \"%s\"", path);
    static char buf[4096];
    while (read_remaining > 0) {
        ssize_t read_count = read(f, buf, 4096);
        E_EXPECTF(read_count >= 0, "read failed on \"%s\"", path);
        read_remaining -= read_count;

        size_t write_remaining = read_count;
        const char* write_buf = buf;
        while (write_remaining > 0) {
            ssize_t write_count = write(STDOUT_FILENO, write_buf, write_remaining);
            E_EXPECTF(write_count >= 0, "write failed on stdout");
            write_remaining -= write_count;
            write_buf += write_count;
        }
    }
    (void)close(f);
    write_all_to_stdout(EIGHT_ZEROES, (8 - (size % 8)) % 8);
}

void serialize_prime_prime(const char* path)
{
    struct stat stat_buf;
    E_EXPECTF(0 == lstat(path, &stat_buf), "stat failed on \"%s\"", path);

    size_t size = stat_buf.st_size;

    serialize_str("type");
    switch (stat_buf.st_mode & S_IFMT) { // file type
        case S_IFREG:
            serialize_str("regular");
            if (stat_buf.st_mode & S_IXUSR) {
                serialize_str("executable");
                serialize_str("");
            }
            serialize_str("contents");
            serialize_u64(size);
            dump_file(path, size);
            break;
        case S_IFLNK:
            serialize_str("symlink");
            serialize_str("target");
            serialize_u64(size);
            char* buf = malloc(size);
            E_EXPECT(buf);
            ssize_t count = readlink(path, buf, size);
            E_EXPECTF(count >= 0, "readlink failed on \"%s\"", path);
            EXPECT((size_t)count == size);
            write_all_to_stdout(buf, size);
            break;
        case S_IFDIR:
            serialize_str("directory");

            struct dirent** names; // scandir allocates the list and its contents
            int names_len = scandir(path, &names, NULL, dirent_name_strcmp);
            E_EXPECTF(names_len >= 0, "scandir failed on \"%s\"", path);

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
