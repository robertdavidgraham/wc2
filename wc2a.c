#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

enum {BUFSIZE=65536};

struct results {
    unsigned long long byte_count;
    unsigned long long word_count;
    unsigned long long line_count;
};

/* ASCII spaces = " \t\f\v\n\r" */
static unsigned char is_space[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/** Parse a single 64k chunk. Since a word can cross a chunk
 * boundary, we have to remember the 'state' from a previous
 * chunk. */
void parse_chunk(const char *buf, size_t length, struct results *results, int *in_state)
{
    /* Setup optimized variables */
    enum { Spaces=0, Word=1} state = *in_state;
    size_t i = 0;
    unsigned long long line_count = results->line_count;
    unsigned long long word_count = results->word_count;

    /* Run the inner loop. This is where 99.9% of the time is spent
     * in this program. */
    while (i < length) {
        char c = buf[i++];
        int is_not_space = !is_space[(unsigned char)c];

        line_count += (c == '\n');
        word_count += ((state == Spaces) & is_not_space);

        state = is_not_space;
    }

    /* Save the optimized variables */
    *in_state = state;
    results->line_count = line_count;
    results->word_count = word_count;
}

/** Parse an individual file, or <stdin>, and print the results */
void parse_file(char *buf, size_t bufsize, int fd, const char *filename)
{
    struct results results = {0, 0, 0};
    int state = 0; /* state held between chunks */

    for (;;) {
        ssize_t count;

        /* Process a 64k chunk at a time */
        count = read(fd, buf, bufsize);
        if (count <= 0)
            break;
        results.byte_count += count;
        parse_chunk(buf, count, &results, &state);
    }

    printf("%8llu %7llu %7llu %s\n", results.line_count, results.word_count, results.byte_count, filename);
}

int main(int argc, char *argv[])
{
    char *buf;
    
    /* Allocate a buffer. We don't read in the entire file, but
     * one small bit at a time. */
    buf = malloc(BUFSIZE);
    if (buf == NULL)
        abort();


    if (argc == 1) {
        /* If no files specified, then process <stdin> instead */
        parse_file(buf, BUFSIZE, STDIN_FILENO, "");
    } else {
        int i;

        /* Process all the files specified on the command-line */
        for (i=1; i<argc; i++) {
            int fd;

            fd = open(argv[1], O_RDONLY);
            if (fd == -1) {
                perror(argv[1]);
                continue;
            }

            parse_file(buf, BUFSIZE, fd, argv[1]);

            close(fd);
        }
    }

    return 0;
}
