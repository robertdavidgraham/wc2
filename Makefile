TIMEFORMAT=%U

CFLAGS = -Wall -Wpedantic -Wextra -O2

all: wc2a wc2b wc2c wc2z wc2u test-wc util-genx

wc2a: wc2a.c
	$(CC) $(CFLAGS) $< -o $@

wc2b: wc2b.c
	$(CC) $(CFLAGS) $< -o $@

wc2c: wc2c.c
	$(CC) $(CFLAGS) $< -o $@

wc2z: wc2z.c
	$(CC) $(CFLAGS) $< -o $@

wc2u: wc2u.c
	$(CC) $(CFLAGS) $< -o $@

test-wc: test-wc.c
	$(CC) $(CFLAGS) $< -o $@

util-genx: util-genx.c
	$(CC) $(CFLAGS) $< -o $@

pocorgtfo18.pdf:
	curl -L --retry 20 --retry-delay 2 -O https://github.com/angea/pocorgtfo/raw/master/releases/pocorgtfo18.pdf

ascii.txt: util-genx
	./util-genx a > ascii.txt

utf8.txt: util-genx
	./util-genx --utf8 > utf8.txt

bench: pocorgtfo18.pdf wc2a wc2u util-genx ascii.txt utf8.txt
	@./bench.sh wc -lwc pocorgtfo18.pdf
	@./bench.sh wc -lwc ascii.txt
	@./bench.sh wc -lwm utf8.txt
	@./bench.sh ./wc2a -lwc pocorgtfo18.pdf
	@./bench.sh ./wc2a -lwc ascii.txt
	@./bench.sh ./wc2a -lwm utf8.txt
	@./bench.sh ./wc2u -lwc pocorgtfo18.pdf
	@./bench.sh ./wc2u -lwc ascii.txt
	@./bench.sh ./wc2u -lwm utf8.txt

bench10: pocorgtfo18.pdf wc2a wc2u util-genx ascii.txt utf8.txt
	@./bench10.sh wc -lwc pocorgtfo18.pdf
	@./bench10.sh wc -lwc ascii.txt
	@./bench10.sh wc -lwm utf8.txt
	@./bench10.sh ./wc2a -lwc pocorgtfo18.pdf
	@./bench10.sh ./wc2a -lwc ascii.txt
	@./bench10.sh ./wc2a -lwm utf8.txt
	@./bench10.sh ./wc2u -lwc pocorgtfo18.pdf
	@./bench10.sh ./wc2u -lwc ascii.txt
	@./bench10.sh ./wc2u -lwm utf8.txt


test: wc2a
	@bash selftest

clean:
	rm -f wc2a wc2b wc2c wc2z wc2u test-wc util-genx

cleanall: 
	rm -f pocorgtfo18.pdf ascii.txt utf8.txt
