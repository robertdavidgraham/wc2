/*
    11/27　たらとかぼちゃの煮込み
*/
#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>

enum {BUFSIZE=65536};

/*
| bytes | bits |  first  |   last   |   byte1  |   byte2  |   byte3  |   byte4  |
|:-----:|:----:|:-------:|:--------:|:--------:|:--------:|:--------:|:--------:|
|   1   |    7 |  U+0000 |   U+007F | 0xxxxxxx |          |          |          |
|   2   |   11 |  U+0080 |   U+07FF | 110xxxxx | 10xxxxxx |          |          |
|   3   |   16 |  U+0800 |   U+FFFF | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
|   4   |   21 | U+10000 | U+10FFFF | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |
 */
enum {  
    U2_xx,
    U2_C2,
    U3_E0,
    U3_E1,
    U3_E2,
    U3_E3,
    U3_Ed,
    U3_Ee,
    U3_Ex,
    U3_E0_xx,
    U3_E1_xx,
    U3_E1_9a,
    U3_E2_80,
    U3_E2_81,
    U3_E2_xx,
    U3_E3_80,
    U3_E3_81,
    U3_E3_xx,
    U3_Ed_xx,
    U3_Ee_xx,
    U3_Ex_xx,
    U4_xx,
    U4_F0,
    U4_F4,
    U4_xx_xx,
    U4_F0_xx,
    U4_F4_xx,
    U4_xx_xx_xx,
    U4_F0_xx_xx,
    U4_F4_xx_xx,
    ILLEGAL,
    
};

enum {
    WASSPACE = 0,
    NEWLINE,
    NEWWORD,
    WASWORD,


    USPACE,
    UWORD=USPACE+ILLEGAL+1,
    STATE_MAX=UWORD+ILLEGAL+1,
};
const char *name(unsigned state)
{
    static char buf[64];
    switch (state) {
        case WASSPACE: return "(WASSPACE)";
        case NEWLINE: return "(NEWLINE)";
        case NEWWORD: return "-NEWWORD-";
        case WASWORD: return "-WASWORD-";
        case USPACE+U2_xx: return "(U2_xx)";
        case USPACE+U2_C2: return "(U2_C2)";
        case USPACE+U3_E0: return "(U3_E0)";
        case USPACE+U3_E1: return "(U3_E1)";
        case USPACE+U3_E2: return "(U3_E2)";
        case USPACE+U3_E3: return "(U3_E3)";
        case USPACE+U3_Ed: return "(U3_Ee)";
        case USPACE+U3_Ee: return "(U3_Ee)";
        case USPACE+U3_Ex: return "(U3_Ex)";
        case USPACE+U3_E0_xx: return "(U3_E0_xx)";
        case USPACE+U3_E1_xx: return "(U3_E1_xx)";
        case USPACE+U3_E1_9a: return "(U3_E1_9a)";
        case USPACE+U3_E2_80: return "(U3_E2_80)";
        case USPACE+U3_E2_81: return "(U3_E2_81)";
        case USPACE+U3_E2_xx: return "(U3_E2_xx)";
        case USPACE+U3_E3_80: return "(U3_E3_80)";
        case USPACE+U3_E3_81: return "(U3_E3_81)";
        case USPACE+U3_E3_xx: return "(U3_E3_xx)";
        case USPACE+U3_Ed_xx: return "(U3_Ed_xx)";
        case USPACE+U3_Ee_xx: return "(U3_Ee_xx)";
        case USPACE+U3_Ex_xx: return "(U3_Ex_xx)";
        case USPACE+U4_xx: return "(U4_xx)";
        case USPACE+U4_xx_xx: return "(U4_xx_xx)";
        case USPACE+U4_xx_xx_xx: return "(U4_xx_xx_xx)";
        case USPACE+ILLEGAL: return "(ILLEGAL)";
        
        case UWORD+U2_xx: return "-U2_xx-";
        case UWORD+U2_C2: return "-U2_C2-";
        case UWORD+U3_E0: return "-U3_E0-";
        case UWORD+U3_E1: return "-U3_E1-";
        case UWORD+U3_E2: return "-U3_E2-";
        case UWORD+U3_E3: return "-U3_E3-";
        case UWORD+U3_Ed: return "-U3_Ed-";
        case UWORD+U3_Ee: return "-U3_Ee-";
        case UWORD+U3_Ex: return "-U3_Ex-";
        case UWORD+U3_E0_xx: return "-U3_E0_xx-";
        case UWORD+U3_E1_xx: return "-U3_E1_xx-";
        case UWORD+U3_E1_9a: return "-U3_E1_9a-";
        case UWORD+U3_E2_80: return "-U3_E2_80-";
        case UWORD+U3_E2_81: return "-U3_E2_81-";
        case UWORD+U3_E2_xx: return "-U3_E2_xx-";
        case UWORD+U3_E3_80: return "-U3_E3_80-";
        case UWORD+U3_E3_81: return "-U3_E3_81-";
        case UWORD+U3_E3_xx: return "-U3_E3_xx-";
        case UWORD+U3_Ed_xx: return "-U3_Ed_xx-";
        case UWORD+U3_Ee_xx: return "-U3_Ee_xx-";
        case UWORD+U3_Ex_xx: return "-U3_E0_xx-";
        case UWORD+U4_xx: return "-U4_xx-";
        case UWORD+U4_xx_xx: return "-U4_xx_xx-";
        case UWORD+U4_xx_xx_xx: return "-U4_xx_xx_xx-";
        case UWORD+ILLEGAL: return "-ILLEGAL-";
        
        default:
            snprintf(buf, sizeof(buf), "%u", state);
            return buf;
    }
}


unsigned char table[256][256];


void build_basic(unsigned char *row, unsigned char default_state, unsigned char ubase)
{
    unsigned c;
    for (c=0; c<256; c++) {
        if ((c & 0x80)) {
            if ((c & 0xE0) == 0xC0) {
                /* 110x xxxx - unicode 2 byte sequence */
                if (c < 0xC2)
                    row[c] = ubase + ILLEGAL;
                else if (c == 0xC2)
                    row[c] = ubase + U2_C2;
                else
                    row[c] = ubase + U2_xx;
            } else if ((c & 0xF0) == 0xE0) {
                /* 1110 xxxx - unicode 3 byte sequence */
                switch (c) {
                    case 0xE0:
                        row[c] = ubase + U3_E0;
                        break;
                    case 0xE1:
                        row[c] = ubase + U3_E1;
                        break;
                    case 0xE2:
                        row[c] = ubase + U3_E2;
                        break;
                    case 0xE3:
                        row[c] = ubase + U3_E3;
                        break;
                    case 0xEd:
                        row[c] = ubase + U3_Ed;
                        break;
                    case 0xEe:
                        row[c] = ubase + U3_Ee;
                        break;
                    default:
                        row[c] = ubase + U3_Ex;
                        break;
                }
            } else if ((c & 0xF8) == 0xF0) {
                if (c >= 0xF5)
                    row[c] = ubase + ILLEGAL;
                else if (c == 0xF0)
                    row[c] = ubase + U4_F0;
                else if (c == 0xF4)
                    row[c] = ubase + U4_F4;
                else
                    row[c] = ubase + U4_xx;
            } else
                row[c] = ubase + ILLEGAL;
        } else if (c == '\n')
            row[c] = NEWLINE;
        else if (isspace(c))
            row[c] = WASSPACE;
        else
            row[c] = default_state;
    }
}

void build_WASSPACE(unsigned char *row)
{
    build_basic(row, NEWWORD, USPACE);
}

void build_WASWORD(unsigned char *row)
{
    build_basic(row, WASWORD, UWORD);
}

/*
| bytes | bits |  first  |   last   |   byte1  |   byte2  |   byte3  |   byte4  |
|:-----:|:----:|:-------:|:--------:|:--------:|:--------:|:--------:|:--------:|
|   1   |    7 |  U+0000 |   U+007F | 0xxxxxxx |          |          |          |
|   2   |   11 |  U+0080 |   U+07FF | 110xxxxx | 10xxxxxx |          |          |
|   3   |   16 |  U+0800 |   U+FFFF | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
|   4   |   21 | U+10000 | U+10FFFF | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |
*/
size_t ucs4_to_utf8(unsigned char *buf, size_t sizeof_buf, unsigned wchar)
{
    if (0xD800 <= wchar && wchar <= 0xDFFF) {
        /* surrogates must not be encoded */
        return 0;
    }

    if (0x10000 <= wchar && wchar <= 0x10FFFF && sizeof_buf >= 4) {
        buf[0] = 0xF0 | ((wchar >> 18) & 0x07);
        buf[1] = 0x80 | ((wchar >> 12) & 0x3F);
        buf[2] = 0x80 | ((wchar >>  6) & 0x3F);
        buf[3] = 0x80 | ((wchar >>  0) & 0x3F);
        return 4;
    } else if (0x0800 <= wchar && wchar <= 0xFFFF && sizeof_buf >= 3) {
        buf[0] = 0xE0 | ((wchar >> 12) & 0x0F);
        buf[1] = 0x80 | ((wchar >>  6) & 0x3F);
        buf[2] = 0x80 | ((wchar >>  0) & 0x3F);
        return 3;
    } else if (0x0080 <= wchar && wchar <= 0x7FF && sizeof_buf >= 2) {
        buf[0] = 0xC0 | ((wchar >>  6) & 0x1F);
        buf[1] = 0x80 | ((wchar >>  0) & 0x3F);
        return 2;
    } else if (wchar <= 0x7F && sizeof_buf >= 1) {
        buf[0] = wchar;
        return 1;
    } else
        return 0;
}

int selftest_ucs4_to_utf8(void)
{
    unsigned c;
    unsigned char buf1[64];
    unsigned char buf2[64];
    unsigned max_errors = 10;
    unsigned was_last_error = 0;

    for (c=0; c<=0x10FFFF; c++) {
        size_t len1 = ucs4_to_utf8(buf1, sizeof(buf1), c);
        int len2 = wctomb((char*)buf2, c);

        

        if (len2 < 0 && len1 == 0)
            continue;
        
        if ((int)len1 != len2) {
            if (!was_last_error)
                printf("0x%04x failed, len1=%d len2=%d\n", c, (int)len1, (int)len2);
            was_last_error = 1;
            continue;
        }

        if (memcmp(buf1, buf2, len1) != 0) {
            printf("0x%04x failed, len=%d \nbuf1=%02x %02x %02x %02x \nbuf2=%02x %02x %02x %02x\n", c, len2,
                    buf1[0], buf1[1], buf1[2], buf1[3], 
                    buf2[0], buf2[1], buf2[2], buf2[3] 
                    );
            if (--max_errors == 0)
                exit(1);
        }

        if (was_last_error) {
            printf("0x%04x succeeded\n", c);
            was_last_error = 0;
        }
    }

    return 0;
}

void print_row(unsigned char row)
{
    unsigned i;

    printf("--row = %s\n", name(row));
    for (i=0; i<256; i++) {
        printf("0x%02x -> %s\n", i, name(table[row][i]));
    }

}

void build_urow(unsigned ubase, unsigned id, unsigned next)
{
    size_t i;
    unsigned default_state;

    default_state = table[ubase + ILLEGAL][0];

    if (next == 0)
        next = default_state;
    else
        next = ubase + next;

    memcpy(table[ubase + id], table[ubase + ILLEGAL], 256);

    //assert(default_state == table[ubase + ILLEGAL][0]);

    //build_basic(table[ubase + id], default_state, ubase);
    for (i=0x80; i<0xC0; i++) {
        table[ubase + id][i] = next;
    }
    for (i=0xC0; i<0x100; i++) {
        table[ubase + id][i] = ubase + ILLEGAL;
    }

}
void build_unicode(unsigned char default_state, unsigned ubase)
{
    size_t i;

    build_basic(table[ubase + ILLEGAL], default_state, ubase);
    
    /*
     * Two byte
     */
    build_urow(ubase, U2_xx, 0);
    build_urow(ubase, U2_C2, 0);
 
    /*
     * Three byte
     */
    build_urow(ubase, U3_E0, U3_E0_xx);
    build_urow(ubase, U3_E1, U3_E1_xx);
    build_urow(ubase, U3_E2, U3_E2_xx);
    build_urow(ubase, U3_E3, U3_E3_xx);
    build_urow(ubase, U3_Ed, U3_Ed_xx);
    build_urow(ubase, U3_Ee, U3_Ee_xx);
    build_urow(ubase, U3_Ex, U3_Ex_xx);
    
    build_urow(ubase, U3_E0_xx, 0);
    build_urow(ubase, U3_E1_xx, 0);
    build_urow(ubase, U3_E1_9a, 0);
    build_urow(ubase, U3_E2_80, 0);
    build_urow(ubase, U3_E2_81, 0);
    build_urow(ubase, U3_E2_xx, 0);
    build_urow(ubase, U3_E3_80, 0);
    build_urow(ubase, U3_E3_81, 0);
    build_urow(ubase, U3_E3_xx, 0);
    build_urow(ubase, U3_Ed_xx, 0);
    build_urow(ubase, U3_Ee_xx, 0);
    build_urow(ubase, U3_Ex_xx, 0);

    table[ubase + U3_E1][0x9a] = ubase + U3_E1_9a;
    table[ubase + U3_E2][0x80] = ubase + U3_E2_80;
    table[ubase + U3_E2][0x81] = ubase + U3_E2_81;
    table[ubase + U3_E3][0x80] = ubase + U3_E3_80;
    table[ubase + U3_E3][0x81] = ubase + U3_E3_81;
    
    
    /*
     * Four byte
     */
    build_urow(ubase, U4_xx, U4_xx_xx);
    build_urow(ubase, U4_F0, U4_F0_xx);
    build_urow(ubase, U4_F4, U4_F4_xx);

    build_urow(ubase, U4_xx_xx, U4_xx_xx_xx);
    build_urow(ubase, U4_F0_xx, U4_F0_xx_xx);
    build_urow(ubase, U4_F4_xx, U4_F4_xx_xx);

    build_urow(ubase, U4_xx_xx_xx, 0);
    build_urow(ubase, U4_F0_xx_xx, 0);
    build_urow(ubase, U4_F4_xx_xx, 0);

    /*
     * Unicode spaces
     */
    if (iswspace(0x0085))
        table[ubase + U2_C2][0x85] = WASSPACE;
    if (iswspace(0x00A0))
        table[ubase + U2_C2][0xA0] = WASSPACE;
    if (iswspace(0x1680)) /* 0x1680 = 0xe1 0x9a 0x80 = OGHAM SPACE MARK*/
        table[ubase + U3_E1_9a][0x80] = WASSPACE;
    for (i=0x2000; i<0x200b+1; i++) {
        if (iswspace(i))
            table[ubase + U3_E2_80][0x80 + (i&0x6F)] = WASSPACE;
    }
    if (iswspace(0x2028))
        table[ubase + U3_E2_80][0xA8] = WASSPACE;
    if (iswspace(0x2029))
        table[ubase + U3_E2_80][0xA9] = WASSPACE;
    if (iswspace(0x202F))
        table[ubase + U3_E2_80][0xAF] = WASSPACE;
    if (iswspace(0x205F))
        table[ubase + U3_E2_81][0x9F] = WASSPACE;
    if (iswspace(0x3000))
        table[ubase + U3_E3_80][0x80] = WASSPACE;


    /*
     * Illegal sequences
     *
     * The following need to be marked as illegal because they can
     * be represented with a shorter string. In other words,
     * 0xC0 0x81 is the same as 0x01, so needs to be marked as an
     * illegal sequence.
     */
    for (i=0x80; i<0xA0; i++) {
        table[ubase + U3_E0][i] = ubase + ILLEGAL;
    }
    for (i=0x80; i<0x90; i++) {
        table[ubase + U4_F0][i] = ubase + ILLEGAL;
    }
    /* Exceeds max possible size of unicode character */
    for (i=0x90; i<0xC0; i++) {
        table[ubase + U4_F4][i] = ubase + ILLEGAL;
    }
    /* Surrogate space */
    for (i=0xA0; i<0xC0; i++) {
        table[ubase + U3_Ed][i] = ubase + ILLEGAL;
    }
    
}

struct results {
    unsigned long line_count;
    unsigned long word_count;
    unsigned long char_count;
    unsigned long byte_count;
    unsigned long illegal_count;
    int is_space;
};


//parse_chunk(buf, count, &results, &was_space);
void
parse_chunk(const unsigned char *buf, size_t length, struct results *results, unsigned *inout_state)
{
    unsigned state = *inout_state;
    size_t i;
    unsigned long counts[STATE_MAX];
    
    counts[NEWLINE] = 0;
    counts[NEWWORD] = 0;
    counts[WASSPACE] = 0;
    counts[WASWORD] = 0;

    for (i=0; i<length; i++) {
        unsigned char c = buf[i];
        state = table[state][c];
        counts[state]++;
    }

    results->line_count += counts[NEWLINE];
    results->word_count += counts[NEWWORD];
    results->char_count += counts[NEWLINE] + counts[WASSPACE] + counts[WASWORD] + counts[NEWWORD];
    results->byte_count += length;
    results->is_space = (state == NEWLINE || state == WASSPACE) && counts[NEWWORD] == 0;
    *inout_state = state;
}


int is_space(const unsigned char *str, size_t length)
{
    struct results results = {0};
    unsigned state = 0;

    parse_chunk(str, length, &results, &state);
    return results.is_space && results.illegal_count == 0;
}

int is_legal(const unsigned char *str, size_t length)
{
    struct results results;
    unsigned state = 0;
    unsigned long line_count = 0;
    unsigned long word_count = 0;
    unsigned long char_count = 0;
    unsigned long illegal_count = 0;
    size_t i;

    for (i=0; i<length; i++) {
        unsigned char c = str[i];

        state = table[state][c];

        line_count += state == NEWLINE; /* new line */
        word_count += state == NEWWORD; /* new word */
        char_count += state < USPACE; /* char count */
        if (state == USPACE+ILLEGAL)
            return 0;
        if (state == UWORD+ILLEGAL)
            return 0;
        if (char_count)
            return 1;
    }

    results.line_count = line_count;
    results.word_count = word_count;
    results.char_count = char_count;
    results.illegal_count = illegal_count;
    results.is_space = (state == NEWLINE || state == WASSPACE) && word_count == 0;
    return 1;
}


int selftest_legal(void)
{
    unsigned i;
    unsigned char buf[4];

    //print_row(USPACE + U3_E0_xx);
    //exit(1);
    for (i=0; i<0xFFFFFFFF; i++) {
        size_t length;
        int x;
        int y;
        wchar_t wc;
        mbstate_t state = {{0}};
        
        if ((i&0xFFFFFF) == 0) {
            fprintf(stderr, "0x%08x\b\b\b\b\b\b\b\b\b\b", i);
            fflush(stderr);
        }

        buf[0] = (i>>0) & 0xFF;
        buf[1] = (i>>8) & 0xFF;
        buf[2] = (i>>16) & 0xFF;
        buf[3] = (i>>24) & 0xFF;


        if (i <= 0xFF)
            length = 1;
        else if (i <= 0xFFFF)
            length = 2;
        else if (i <= 0xFFFFFF)
            length = 3;
        else
            length = 4;

        if ((buf[0] & 0x80) == 0)
            continue;
        if (length >= 2 && (buf[1] & 0x80) == 0)
            continue;
        if (length >= 3 && (buf[2] & 0x80) == 0)
            continue;
        if (length >= 4 && (buf[3] & 0x80) == 0)
            continue;

        x = mbrtowc(&wc, (char*)buf, length, &state);
        //printf("x=%d\n", x);
        y = is_legal(buf, length);
        //printf("y=%d\n", y);
        if (x == -1 && y == 0)
            continue;
        else if (x != -1 && y != 0)
            continue;
        else {
            unsigned j;
            printf("\nfail\n");
            for (j=0; j<length; j++)
                printf("%02x ", buf[j]);
             printf(": mbtowc()=%s uparse()=%s\n",
                        (x==-1)?"illegal":"legal", (y==1)?"legal":"illegal");
            exit(1);
        }
    }
    return 0;
}

int selftest_isspace(void)
{
    unsigned i;
    for (i=0; i<0xFFFF; i++) {
        unsigned char buf[64];
        size_t len;

        len = ucs4_to_utf8(buf, sizeof(buf), i);
        if (len == 0)
            continue;
        if (is_space(buf, len) != iswspace(i)) {
            fprintf(stdout, "0x%04x: is_space()=%s, iswspace()=%s\n",
                    i,
                    is_space(buf, len)?"true":"false",
                    iswspace(i)?"true":"false");
            exit(1);
        }
    }
    return 0; /* success */
}

void compile_utf8_statemachine(void)
{
    setlocale(LC_ALL, "");
    //printf("LC_CTYPE = %s\n", setlocale(LC_CTYPE, 0));

    build_WASSPACE(table[WASSPACE]);
    build_WASSPACE(table[NEWLINE]);
    build_WASWORD(table[WASWORD]);
    build_WASWORD(table[NEWWORD]);
    build_unicode(NEWWORD, USPACE);
    build_unicode(WASWORD, UWORD);
}

/** 
 * Parse an individual file, or <stdin>, and print the results 
 */
void parse_file(unsigned char *buf, size_t bufsize, FILE *fp, const char *filename)
{
    struct results results = {0};
    unsigned state = 0; /* state held between chunks */

    /* Process a 64k chunk at a time */
    for (;;) {
        ssize_t count;

        count = fread(buf, 1, bufsize, fp);
        if (count <= 0)
            break;
        parse_chunk(buf, count, &results, &state);
    }

    if (filename && filename[0])
        printf("%lu %lu %lu %lu %s\n", results.line_count, results.word_count, results.char_count, results.byte_count, filename);
    else
        printf("%lu %lu %lu %lu\n", results.line_count, results.word_count, results.char_count, results.byte_count);
}

int main(int argc, char *argv[])
{
    unsigned char *buf;
    
    compile_utf8_statemachine();

    /* Allocate a buffer. We don't read in the entire file, but
     * one small bit at a time. */
    buf = malloc(BUFSIZE);
    if (buf == NULL)
        abort();

    if (argc == 1) {
        /* If no files specified, then process <stdin> instead */
        const char *filename = "";
        parse_file(buf, BUFSIZE, stdin, filename);
    } else {
        int i;

        /* Process all the files specified on the command-line */
        for (i=1; i<argc; i++) {
            FILE *fp;
            const char *filename = argv[i];

            fp = fopen(filename, "rb");
            if (fp == NULL) {
                perror(argv[i]);
                continue;
            }

            parse_file(buf, BUFSIZE, fp, filename);

            fclose(fp);
        }
    }

    free(buf);
    return 0;
}
