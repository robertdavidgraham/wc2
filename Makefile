
CFLAGS = -Wall -Wpedantic -Wextra -O2

all: wc2a wc2b wc2z 

wc2a: wc2a.c
	$(CC) $(CFLAGS) $< -o $@

wc2b: wc2b.c
	$(CC) $(CFLAGS) $< -o $@

wc2z: wc2z.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f wc2a wc2b wc2z

