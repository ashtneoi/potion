CFLAGS=-std=c99 -pedantic -Wall -Wextra -Werror

bin/sha3-256sum:
	cd lib-external/libkeccak && $(MAKE) libkeccak.a
	cd lib-external/sha3sum && $(MAKE) sha3-256sum
	mkdir -p bin
	cp lib-external/sha3sum/sha3-256sum $@

bin/nar: src/nar.c
	$(CC) $(CFLAGS) -o $@ $^
