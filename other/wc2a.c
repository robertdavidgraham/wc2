/*
    Implements the Unix command-line program 'wc' (word-count)
    Only ASCII parsing.
*/
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>


/**
 * Hold the configuration parsed from the command-line
 */
struct config {
    size_t file_count;
    int is_stdin;
    int is_counting_lines;
    int is_counting_words;
    int is_counting_bytes;
    int is_counting_chars;
    int is_printing_totals;
    unsigned column_width;
    int is_pointer_arithmetic;
};

/**
 * Holds the counts from reading a chunk, a file, or totals.
 */
struct results {
    unsigned long line_count;
    unsigned long word_count;
    unsigned long byte_count;
    unsigned long char_count;
};

/**
 * This table is initialized at startup to indicate which
 * bytes are spaces or not */
static unsigned char my_isspace[256];


/**
 * Print the results structure. We need to make sure there is a space
 * between each of the fields, though not before the first field, and
 * not after the last field. Every field is optional, depending upon
 * whether the corresponding parameter was specified on the command-line.
 * So that printing multiple results line up, we also have a consistent
 * column-width for all the columns.
 */
void
print_results(const char *filename, struct results *results, struct config *cfg)
{
    int needs_space = 0; /* space needed between output */
    unsigned width = cfg->column_width;

    /* -l */
    if (cfg->is_counting_lines)
        printf("%s%*lu", needs_space++?" ":"", width, results->line_count);
    
    /* -w */
    if (cfg->is_counting_words)
        printf("%s%*lu", needs_space++?" ":"", width, results->word_count);
    
    /* -c */
    if (cfg->is_counting_bytes)
        printf("%s%*lu", needs_space++?" ":"", width, results->byte_count);
    
    /* -m */
    if (cfg->is_counting_chars)
        printf("%s%*lu", needs_space++?" ":"", width, results->char_count);
    
    /* NULL if <stdin>, "total" for the last line showing totals, otherwise,
     * the name of the file that was processed */
    if (filename)
        printf("%s%s", needs_space++?" ":"", filename);
    printf("\n");    
}

static struct results 
parse_chunk_p(const unsigned char *buf, size_t length, unsigned *inout_state)
{
    unsigned was_space = !*inout_state;
    unsigned line_count = 0;
    unsigned word_count = 0;
    const unsigned char *end = buf + length;

    /* Run the inner loop. This is where 99.9% of the time is spent
     * in this program. */
    while (buf < end) {
        unsigned char c = *buf++;
        int is_space = my_isspace[c];

        line_count += (c == '\n');
        word_count += (was_space && !is_space);

        was_space = is_space;
    }

    /* Save the state, so that the next chunk knows whether this chunk
     * ended in a 'space' or a 'word'. */
    *inout_state = !was_space;

    /* Return the results */
    {
        struct results results;
        results.line_count = line_count;
        results.word_count = word_count;
        results.char_count = length;
        results.byte_count = length;
        
        return results;
    }
}

/** 
 * Parse a single 64k chunk. Since a word can cross a chunk
 * boundary, we have to remember the 'state' from a previous
 * chunk. 
 */
static struct results 
parse_chunk(const unsigned char *buf, size_t length, unsigned *inout_state)
{
    unsigned was_space = !*inout_state;
    unsigned line_count = 0;
    unsigned word_count = 0;
    size_t i;

    /* Run the inner loop. This is where 99.9% of the time is spent
     * in this program. */
    for (i = 0; i < length; i++) {
        unsigned char c = buf[i];
        int is_space = my_isspace[c];

        line_count += (c == '\n');
        word_count += (was_space && !is_space);

        was_space = is_space;
    }

    /* Save the state, so that the next chunk knows whether this chunk
     * ended in a 'space' or a 'word'. */
    *inout_state = !was_space;

    /* Return the results */
    {
        struct results results;
        results.line_count = line_count;
        results.word_count = word_count;
        results.char_count = length;
        results.byte_count = length;
        
        return results;
    }
}

/** 
 * Parse an individual file, or <stdin>, and print the results 
 */
static struct results
parse_file(FILE *fp, const struct config *cfg)
{
    enum {BUFSIZE=65536};
    struct results results = {0, 0, 0, 0};
    unsigned state = 0; /* state held between chunks */
    unsigned char *buf;

    buf = malloc(BUFSIZE);
    if (buf == NULL)
        abort();

    /* Process a 64k chunk at a time */
    for (;;) {
        size_t count;
        struct results x;

        /* Read the next chunk of data from the file */
        count = fread(buf, 1, BUFSIZE, fp);
        if (count <= 0)
            break;

        /* Do the word-count algorithm */
        if (cfg->is_pointer_arithmetic)
            x = parse_chunk_p(buf, count, &state);
        else    
            x = parse_chunk(buf, count, &state);

        /* Sum the results */
        results.line_count += x.line_count;
        results.word_count += x.word_count;
        results.byte_count += x.byte_count;
        results.char_count += x.char_count;
    }

    free(buf);
    return results;
}

/**
 * Calculate the width for the columns, so that when printing the
 * results from several files, all the columns will line up. The
 * width for all the columns is determined by the size of the files.
 */
static unsigned
get_column_width(int argc, char *argv[], int is_stdin)
{
    int i;
    off_t maxsize = 1;
    unsigned width = 0;

    for (i=1; i<argc; i++) {
        const char *filename = argv[i];
        struct stat st;

        if (filename[0] == '-')
            continue;
        
        if (stat(filename, &st) == 0) {
            if (S_ISREG(st.st_mode)) {
                if (maxsize <= st.st_size)
                    maxsize = st.st_size;
            } else if (maxsize <= 1000000)
                maxsize = 1000000;
        }
    }

    if (is_stdin) {
        if (maxsize <= 1000000)
            maxsize = 1000000;
    }

    while (maxsize) {
        width++;
        maxsize /= 10;
    }
    
    return width;
}

/**
 * Print a help message
 */
static void
print_help(void)
{
    printf("wc -- word, line, and byte or character count\n");
    printf("use:\n wc [-c|-m][-lw][file...]\n");
    printf("where:\n");
    printf(" -c\tPrint the number of bytes in each input file.\n");
    printf(" -l\tPrint the number of newlines in each input file.\n");
    printf(" -m\tPrint number of multibyte characters in each input file.\n");
    printf(" -w\tPrint the number of words in each input file.\n");
    printf("If no files specified, reads from stdin.\n");
    printf("If no options specified, -lwc will be used.\n");
}

/**
 * Parse the command-line options in order to get the configuration
 * for the program.
 */
static struct config
read_command_line(int argc, char *argv[])
{
    struct config cfg;
    int i;
    
    memset(&cfg, 0, sizeof(cfg));

    /* We set this as the errno so that 'perror()' will print a localized
     * error message, whatever "Invalid argument" is in the user's local
     * language */
    errno = EINVAL;

    for (i=1; i<argc; i++) {
        size_t j;
        size_t maxj;
        if (argv[i][0] != '-') {
            /* Assume anything not an -option is a filename */
            cfg.file_count++;
            continue;
        }
        if (argv[i][1] == '\0') {
            /* A bare dash '-' on the command-line means we should
             * also handle <stdin> */
            cfg.is_stdin = 1;
            continue;
        }
        if (argv[i][1] == '-') {
            if (argv[i][2] == '\0') {
                cfg.file_count += argc - i - 1;
                break;
            } else if (strcmp(argv[i], "--version") == 0) {
                fprintf(stderr, "--- wc-fast-ut8 1.0 by Robert Graham ---\n");
                exit(0);
            } else if (strcmp(argv[i], "--help") == 0) {
                print_help();
                exit(0);
            } else {
                perror(argv[i]);
                exit(1);
            }
        }

        maxj = strlen(argv[i]);
        for (j=1; j<maxj; j++) {
            char c = argv[i][j];
            const char *parm = NULL;
            switch (c) {
                case 'l': cfg.is_counting_lines++; break;
                case 'w': cfg.is_counting_words++; break;
                case 'c':
                    if (cfg.is_counting_chars) {
                        perror("-c");
                        exit(1);
                    }
                    cfg.is_counting_bytes++;
                    break;
                case 'm':
                    if (cfg.is_counting_bytes) {
                        perror("-m");
                        exit(1);
                    }
                    cfg.is_counting_chars++;
                    break;
                case 'W':
                    if (argv[i][j+1] == '\0') {
                        if (i+1 < argc)
                            parm = argv[++i];
                    } else {
                        parm = argv[i] + j + 1;
                    }
                    if (parm == NULL || !isdigit(*parm)) {
                        perror("-W");
                        exit(1);
                    } else
                        cfg.column_width = atoi(parm);
                    j = maxj;
                    break;
                case 'P':
                    cfg.is_pointer_arithmetic = 1;
                    break;
                default:
                    {
                        char foo[3];
                        foo[0] = '-';
                        foo[1] = c;
                        foo[2] = '\0';
                        perror(foo);
                        exit(1);
                    }
                    break;
            }
        }
    }

    /* If no files specified, then we do <stdin> instead */
    if (cfg.file_count == 0)
        cfg.is_stdin = 1;

    /* Default is -lwc if no options are given */
    if (cfg.is_counting_lines == 0 
        && cfg.is_counting_words == 0 
        && cfg.is_counting_bytes == 0 
        && cfg.is_counting_chars == 0) {
        cfg.is_counting_lines = 1;
        cfg.is_counting_words = 1;
        cfg.is_counting_bytes = 1;
    }

    /* Calculate the width for the columns */
    if (cfg.column_width == 0) {
        if (cfg.file_count > 0) {
            cfg.column_width = get_column_width(argc, argv, cfg.is_stdin);
        } else
            cfg.column_width = 1;
    }

    /* If there are more than one files, or if there are both files and
     * <stdin>, then we need to print totals at the end. Otherwise, if only
     * a single result is to be printed, then printing an additional result
     * would be redundant */
    if (cfg.file_count > 1 || (cfg.file_count && cfg.is_stdin))
        cfg.is_printing_totals = 1;

    return cfg;
}

int main(int argc, char *argv[])
{
    int i;
    struct results totals = {0};
    struct config cfg = {0};

    /* Force output to be an atomic line-at-a-time, so that other
     * programs reading the output never see a partial line */
    setvbuf(stdout, NULL, _IOLBF, 0);

    /* Read in the configuration parameters from the command-line */
    cfg = read_command_line(argc, argv);

    /* Initialize the table of spaces */
    for (i=0; i<256; i++)
        my_isspace[i] = isspace(i) != 0; 

    /* Process all the files specified on the command-line */
    for (i=1; i<argc; i++) {
        FILE *fp;
        const char *filename = argv[i];
        struct results results;

        if (argv[i][0] == '-')
            continue;

        fp = fopen(filename, "rb");
        if (fp == NULL) {
            perror(argv[i]);
            continue;
        }

        results = parse_file(fp, &cfg);
        print_results(filename, &results, &cfg);
        
        totals.line_count += results.line_count;
        totals.word_count += results.word_count;
        totals.byte_count += results.byte_count;
        totals.char_count += results.char_count;

        fclose(fp);
    }

    /* If no files specified, or the "-" file specified, then
     * we need to read in <stdin>. We need to change the mode
     * to "binary", to prevent the library from doing it's own
     * notions of text processing */
    if (cfg.is_stdin) {
        struct results results;
        FILE *fp;

        /* Make sure we read <stdin> in binary mode, because on some
         * platforms (Windows) it defaults to text-mode that will
         * chnage some characters */
        fp = freopen(NULL, "rb", stdin);
        if (fp == NULL) {
            perror("stdin");
            fp = stdin;
        }

        results = parse_file(fp, &cfg);
        print_results(NULL, &results, &cfg);

        totals.line_count += results.line_count;
        totals.word_count += results.word_count;
        totals.byte_count += results.byte_count;
        totals.char_count += results.char_count;
    }

    /* If we read more than one thing, then we also need to print an
     * additional totals line */
    if (cfg.is_printing_totals)
        print_results("total", &totals, &cfg);

    return 0;
}
