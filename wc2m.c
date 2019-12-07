/*
    A trivial implementation of the word count 'wc' program

*/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

/* Set this to larger size for slightly faster performance,
 * and samller size (like 1) to verify accuracy */
enum {BUFSIZE=65536};

/* We store the counts in this structure that we pass between
 * functions */
struct results {
    unsigned long byte_count;
    unsigned long word_count;
    unsigned long line_count;
    unsigned long char_count;
};

/** 
 * Parse a single 64k chunk. Since a word can cross a chunk
 * boundary, we have to remember the 'state' from a previous
 * chunk. 
 */
void parse_chunk(const char * buf, size_t length, 
                 struct results *results,  
                 int *inout_state,
                 mbstate_t *mbstate)
{
    /* Setup optimized variables, so that everything inside the
     * inner loop can be a register */
    int was_space = *inout_state;
    unsigned long line_count = results->line_count;
    unsigned long word_count = results->word_count;
    unsigned long char_count = results->char_count;
    size_t i;
    
    /* Run the inner loop. This is where 99.9% of the time is spent
     * in this program. */
    for (i = 0; i < length; i++) {
        size_t len;
        wchar_t c;
        int is_space;

        len = mbrtowc(&c, buf + i, length - i, mbstate);
        if (len == (size_t)-1 || len == 0) {
            i++;
            continue;
        }
        if (len == (size_t)-2)
            break;

        char_count++;
        is_space = iswspace(c);

        line_count += (c == '\n');
        word_count += (was_space && !is_space);

        was_space = is_space;
    }

    /* Save the state */
    *inout_state = was_space;
    results->line_count = line_count;
    results->word_count = word_count;
    results->char_count = char_count;
}

/** 
 * Parse an individual file, or <stdin>, and print the results 
 */
void parse_file(char *buf, size_t bufsize, FILE *fp, const char *filename)
{
    struct results results = {0, 0, 0, 0};
    int was_space = 1; /* state held between chunks */
    mbstate_t mbstate;

    memset(&mbstate, 0, sizeof(mbstate));
    
    /* Process a 64k chunk at a time */
    for (;;) {
        size_t count;

        count = fread(buf, 1, bufsize, fp);
        if (count <= 0)
            break;
        results.byte_count += (unsigned long)count;
        parse_chunk(buf, count, &results, &was_space, &mbstate);
    }

    if (filename && filename[0])
        printf("%8lu %7lu %7lu %s\n", results.line_count, results.word_count, results.char_count, filename);
    else
        printf("%8lu %7lu %7lu\n", results.line_count, results.word_count, results.char_count);
}

int main(int argc, char *argv[])
{
    char *buf;
    
    setlocale(LC_ALL, "");
    fprintf(stderr, "locale=%s\n", setlocale(LC_CTYPE,NULL));

    /* Allocate a buffer. We don't read in the entire file, but
     * one small bit at a time. */
    buf = malloc(BUFSIZE);
    if (buf == NULL)
        abort();

    if (argc == 1) {
        /* If no files specified, then process <stdin> instead */
        parse_file(buf, BUFSIZE, stdin, "");
    } else {
        int i;

        /* Process all the files specified on the command-line */
        for (i=1; i<argc; i++) {
            FILE *fp;

            fp = fopen(argv[i], "rb");
            if (fp == NULL) {
                perror(argv[i]);
                continue;
            }

            parse_file(buf, BUFSIZE, fp, argv[i]);

            fclose(fp);
        }
    }

    free(buf);
    return 0;
}
