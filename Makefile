
CFLAGS = -Wall -Wpedantic -Wextra -O2

all: wc2a wc2b wc2c wc2z wc-fast-utf8 test-wc

wc2a: wc2a.c
	$(CC) $(CFLAGS) $< -o $@

wc2b: wc2b.c
	$(CC) $(CFLAGS) $< -o $@

wc2c: wc2c.c
	$(CC) $(CFLAGS) $< -o $@

wc2z: wc2z.c
	$(CC) $(CFLAGS) $< -o $@

wc-fast-utf8: wc-fast-utf8.c
	$(CC) $(CFLAGS) $< -o $@

test-wc: test-wc.c
	$(CC) $(CFLAGS) $< -o $@

pocorgtfo18.pdf:
	curl https://www.alchemistowl.org/pocorgtfo/pocorgtfo18.pdf -o $@

time: pocorgtfo18.pdf wc2a wc2b wc2z
	/usr/bin/time wc pocorgtfo18.pdf
	/usr/bin/time ./wc2a pocorgtfo18.pdf
	/usr/bin/time ./wc2b pocorgtfo18.pdf
	/usr/bin/time ./wc2z pocorgtfo18.pdf

test: wc2a
	@bash selftest

clean:
	rm -f wc2a wc2b wc2c wc2z wc-fast-utf8 test-wc

