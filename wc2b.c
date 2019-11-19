/*
    A trivial implementation of the word count 'wc' program
    [pointer arithmetic]
*/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

/* Portability stuff for Windows, macOS, and Linux */
#ifdef WIN32
#include <io.h>
#define open _open
#define O_RDONLY _O_RDONLY
#else
#include <unistd.h>
#endif
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

/* Set this to larger size for slightly faster performance,
 * and samller size (like 1) to verify accuracy */
enum {BUFSIZE=65536};

/* We store the counts in this structure that we pass between
 * functions */
struct results {
    unsigned long long byte_count;
    unsigned long long word_count;
    unsigned long long line_count;
};

/* ASCII spaces = " \t\f\v\n\r" */
static unsigned char my_isspace[] = {
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

/** 
 * Parse a single 64k chunk. Since a word can cross a chunk
 * boundary, we have to remember the 'state' from a previous
 * chunk. 
 */
void parse_chunk(const char *restrict buf, size_t length, 
                 struct results * restrict results,  
                 int * restrict inout_state)
{
    /* Setup optimized variables, so that everything inside the
     * inner loop can be a register */
    int was_space = *inout_state;
    unsigned long long line_count = results->line_count;
    unsigned long long word_count = results->word_count;
    const char *end = buf + length;
    
    /* Run the inner loop. This is where 99.9% of the time is spent
     * in this program. */
    for (; buf < end; buf++) {
        char c = *buf;
        int is_space = my_isspace[(unsigned char)c];

        line_count += (c == '\n');
        word_count += (was_space && !is_space);

        was_space = is_space;
    }

    /* Save the state */
    *inout_state = was_space;
    results->line_count = line_count;
    results->word_count = word_count;
}

/** 
 * Parse an individual file, or <stdin>, and print the results 
 */
void parse_file(char *buf, size_t bufsize, int fd, const char *filename)
{
    struct results results = {0, 0, 0};
    int was_space = 1; /* state held between chunks */

    /* Process a 64k chunk at a time */
    for (;;) {
        ssize_t count;

        count = read(fd, buf, bufsize);
        if (count <= 0)
            break;
        results.byte_count += count;
        parse_chunk(buf, count, &results, &was_space);
    }

    if (filename && filename[0])
        printf("%8llu %7llu %7llu %s\n", results.line_count, results.word_count, results.byte_count, filename);
    else
        printf("%8llu %7llu %7llu\n", results.line_count, results.word_count, results.byte_count);
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
