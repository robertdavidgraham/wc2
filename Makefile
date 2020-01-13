TIMEFORMAT=%U

CFLAGS += -Wall -Wpedantic -Wextra -O2

all: wc2 wc2o wcdiff wctool wcstream

wc2: wc2.c
	$(CC) $(CFLAGS) $< -o $@

wc2o: wc2o.c
	$(CC) $(CFLAGS) $< -o $@

wcdiff: wcdiff.c
	$(CC) $(CFLAGS) $< -o $@

wctool: wctool.c
	$(CC) $(CFLAGS) $< -o $@

wcstream: wcstream.c
	$(CC) $(CFLAGS) $< -o $@

pocorgtfo18.pdf:
	curl -L --retry 20 --retry-delay 2 -O https://github.com/angea/pocorgtfo/raw/master/releases/pocorgtfo18.pdf

ascii.txt: wctool
	@./wctool --ascii > ascii.txt

utf8.txt: wctool
	@./wctool --utf8 > utf8.txt

space.txt: wctool
	@./wctool --allspace > space.txt

word.txt: wctool
	@./wctool --allword > word.txt

bench: wc2 pocorgtfo18.pdf ascii.txt utf8.txt space.txt word.txt
	@./bench-3.sh
	
test: wc2
	@bash selftest

clean:
	rm -f wc2 wc2o wcdiff wctool wcstream

cleanall:
	rm -f pocorgtfo18.pdf ascii.txt utf8.txt word.txt
