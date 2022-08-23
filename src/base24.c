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

int main(int argc, char** argv)
{
    (void)argv;
    EXPECT(argc == 1);

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
            n /= 24;
        }

        (void)count;
    }

    putchar('\n');
}
