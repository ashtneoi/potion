#include "common.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


size_t read_full(int fd, char* buf, size_t len)
{
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t count = read(fd, buf, remaining);
        E_EXPECTF(count >= 0, "read failed on stdin");
        if (count == 0) {
            break;
        }
        remaining -= count;
        buf += count;
    }
    return len - remaining;
}

char TABLE[24] = "bcdfghjkmnpqrstvwxz25678";

__attribute__((__noreturn__)) // for gcc
void exit_with_usage(void)
{
    printf("Usage:  base24 [-p]\n");
    printf("\n");
    printf("-p      truncate and format output to make it suitable for use in a path\n");
    exit(10);
}

int main(int argc, char** argv)
{
    int filename;

    EXPECT(argc > 0);

    if (argc == 1) {
        filename = 0;
    } else if (argc == 2 && 0 == strcmp(argv[1], "-p")) {
        filename = 1;
    } else if (argc == 2) {
        fprintf(stderr, "Error: invalid argument \"%s\"\n", argv[1]);
        exit_with_usage();
    } else {
        fprintf(stderr, "Error: too many arguments\n");
        exit_with_usage();
    }

    size_t digit_count = 0;

    while (1) {
        static uint8_t buf[4];
        memset(buf, 0, sizeof(buf));
        size_t count = read_full(STDIN_FILENO, (char*)buf, sizeof(buf));
        if (count == 0) {
            break;
        }
        uint32_t n = 0;
        for (int i = 3; i >= 0; i--) {
            n = (n << 8) + buf[i];
        }
        for (int i = 0; i < 7; i++) {
            putchar(TABLE[n % 24]);
            digit_count++;
            if (filename && digit_count == 2) {
                putchar('/');
            } else if (filename && digit_count >= 40) {
                putchar('\n');
                return 0;
            }
            n /= 24;
        }

        for (size_t i = 0; i < sizeof(buf) - count; i++) {
            putchar(TABLE[0]);
            if (filename && digit_count == 2) {
                putchar('/');
            } else if (filename && digit_count >= 40) {
                putchar('\n');
                return 0;
            }
        }
    }

    putchar('\n');
}
