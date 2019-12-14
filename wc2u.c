/*
    The Unix standard 'wc' program, optimized to process UTF-8 input
    as fast as possible.

    https://pubs.opengroup.org/onlinepubs/007904975/utilities/wc.html
*/
#define _FILE_OFFSET_BITS   64

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


/** Hold the configuration parsed from the command-line */
struct config {
    size_t file_count;
    int is_stdin;
    int is_counting_lines;
    int is_counting_words;
    int is_counting_bytes;
    int is_counting_chars;
    int is_printing_totals;
    unsigned column_width;
};

/** Holds the results from parsing a file */
struct results {
    unsigned long line_count;
    unsigned long word_count;
    unsigned long char_count;
    unsigned long byte_count;
    unsigned long illegal_count;
    int is_space;
};


/*
| bytes | bits |  first  |   last   |   byte1  |   byte2  |   byte3  |   byte4  |
|:-----:|:----:|:-------:|:--------:|:--------:|:--------:|:--------:|:--------:|
|   1   |    7 |  U+0000 |   U+007F | 0xxxxxxx |          |          |          |
|   2   |   11 |  U+0080 |   U+07FF | 110xxxxx | 10xxxxxx |          |          |
|   3   |   16 |  U+0800 |   U+FFFF | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
|   4   |   21 | U+10000 | U+10FFFF | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |
 */
enum {  
    DUO2_xx,
    DUO2_C2,
    TRI2_E0,
    TRI2_E1,
    TRI2_E2,
    TRI2_E3,
    TRI2_ED,
    TRI2_EE,
    TRI2_xx,
    TRI3_E0_xx,
    TRI3_E1_xx,
    TRI3_E1_9a,
    TRI3_E2_80,
    TRI3_E2_81,
    TRI3_E2_xx,
    TRI3_E3_80,
    TRI3_E3_81,
    TRI3_E3_xx,
    TRI3_Ed_xx,
    TRI3_Ee_xx,
    TRI3_xx_xx,
    QUAD2_xx,
    QUAD2_F0,
    QUAD2_F4,
    QUAD3_xx_xx,
    QUAD3_F0_xx,
    QUAD3_F4_xx,
    QUAD4_xx_xx_xx,
    QUAD4_F0_xx_xx,
    QUAD4_F4_xx_xx,
    ILLEGAL
};

enum {
    WASSPACE = 0,
    NEWLINE,
    NEWWORD,
    WASWORD,
    USPACE,
    UWORD=USPACE+ILLEGAL+1,
    STATE_MAX=UWORD+ILLEGAL+1
};

unsigned char table[256][256];



const char *name(unsigned state)
{
    static char buf[64];
    switch (state) {
        case WASSPACE: return "(WASSPACE)";
        case NEWLINE: return "(NEWLINE)";
        case NEWWORD: return "-NEWWORD-";
        case WASWORD: return "-WASWORD-";
        case USPACE+DUO2_xx: return "(DUO2_xx)";
        case USPACE+DUO2_C2: return "(DUO2_C2)";
        case USPACE+TRI2_E0: return "(TRI2_E0)";
        case USPACE+TRI2_E1: return "(TRI2_E1)";
        case USPACE+TRI2_E2: return "(TRI2_E2)";
        case USPACE+TRI2_E3: return "(TRI2_E3)";
        case USPACE+TRI2_ED: return "(TRI2_EE)";
        case USPACE+TRI2_EE: return "(TRI2_EE)";
        case USPACE+TRI2_xx: return "(TRI2_xx)";
        case USPACE+TRI3_E0_xx: return "(TRI3_E0_xx)";
        case USPACE+TRI3_E1_xx: return "(TRI3_E1_xx)";
        case USPACE+TRI3_E1_9a: return "(TRI3_E1_9a)";
        case USPACE+TRI3_E2_80: return "(TRI3_E2_80)";
        case USPACE+TRI3_E2_81: return "(TRI3_E2_81)";
        case USPACE+TRI3_E2_xx: return "(TRI3_E2_xx)";
        case USPACE+TRI3_E3_80: return "(TRI3_E3_80)";
        case USPACE+TRI3_E3_81: return "(TRI3_E3_81)";
        case USPACE+TRI3_E3_xx: return "(TRI3_E3_xx)";
        case USPACE+TRI3_Ed_xx: return "(TRI3_Ed_xx)";
        case USPACE+TRI3_Ee_xx: return "(TRI3_Ee_xx)";
        case USPACE+TRI3_xx_xx: return "(TRI3_xx_xx)";
        case USPACE+QUAD2_xx: return "(QUAD2_xx)";
        case USPACE+QUAD3_xx_xx: return "(QUAD3_xx_xx)";
        case USPACE+QUAD4_xx_xx_xx: return "(QUAD4_xx_xx_xx)";
        case USPACE+ILLEGAL: return "(ILLEGAL)";
        
        case UWORD+DUO2_xx: return "-DUO2_xx-";
        case UWORD+DUO2_C2: return "-DUO2_C2-";
        case UWORD+TRI2_E0: return "-TRI2_E0-";
        case UWORD+TRI2_E1: return "-TRI2_E1-";
        case UWORD+TRI2_E2: return "-TRI2_E2-";
        case UWORD+TRI2_E3: return "-TRI2_E3-";
        case UWORD+TRI2_ED: return "-TRI2_ED-";
        case UWORD+TRI2_EE: return "-TRI2_EE-";
        case UWORD+TRI2_xx: return "-TRI2_xx-";
        case UWORD+TRI3_E0_xx: return "-TRI3_E0_xx-";
        case UWORD+TRI3_E1_xx: return "-TRI3_E1_xx-";
        case UWORD+TRI3_E1_9a: return "-TRI3_E1_9a-";
        case UWORD+TRI3_E2_80: return "-TRI3_E2_80-";
        case UWORD+TRI3_E2_81: return "-TRI3_E2_81-";
        case UWORD+TRI3_E2_xx: return "-TRI3_E2_xx-";
        case UWORD+TRI3_E3_80: return "-TRI3_E3_80-";
        case UWORD+TRI3_E3_81: return "-TRI3_E3_81-";
        case UWORD+TRI3_E3_xx: return "-TRI3_E3_xx-";
        case UWORD+TRI3_Ed_xx: return "-TRI3_Ed_xx-";
        case UWORD+TRI3_Ee_xx: return "-TRI3_Ee_xx-";
        case UWORD+TRI3_xx_xx: return "-TRI3_E0_xx-";
        case UWORD+QUAD2_xx: return "-QUAD2_xx-";
        case UWORD+QUAD3_xx_xx: return "-QUAD3_xx_xx-";
        case UWORD+QUAD4_xx_xx_xx: return "-QUAD4_xx_xx_xx-";
        case UWORD+ILLEGAL: return "-ILLEGAL-";
        
        default:
            snprintf(buf, sizeof(buf), "%u", state);
            return buf;
    }
}

/**
 * Build an ASCII row. This configures low-order 7-bits, which should
 * be roughly the same for all states
 */
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
                    row[c] = ubase + DUO2_C2;
                else
                    row[c] = ubase + DUO2_xx;
            } else if ((c & 0xF0) == 0xE0) {
                /* 1110 xxxx - unicode 3 byte sequence */
                switch (c) {
                    case 0xE0:
                        row[c] = ubase + TRI2_E0;
                        break;
                    case 0xE1:
                        row[c] = ubase + TRI2_E1;
                        break;
                    case 0xE2:
                        row[c] = ubase + TRI2_E2;
                        break;
                    case 0xE3:
                        row[c] = ubase + TRI2_E3;
                        break;
                    case 0xEd:
                        row[c] = ubase + TRI2_ED;
                        break;
                    case 0xEe:
                        row[c] = ubase + TRI2_EE;
                        break;
                    default:
                        row[c] = ubase + TRI2_xx;
                        break;
                }
            } else if ((c & 0xF8) == 0xF0) {
                if (c >= 0xF5)
                    row[c] = ubase + ILLEGAL;
                else if (c == 0xF0)
                    row[c] = ubase + QUAD2_F0;
                else if (c == 0xF4)
                    row[c] = ubase + QUAD2_F4;
                else
                    row[c] = ubase + QUAD2_xx;
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
    build_urow(ubase, DUO2_xx, 0);
    build_urow(ubase, DUO2_C2, 0);
 
    /*
     * Three byte
     */
    build_urow(ubase, TRI2_E0, TRI3_E0_xx);
    build_urow(ubase, TRI2_E1, TRI3_E1_xx);
    build_urow(ubase, TRI2_E2, TRI3_E2_xx);
    build_urow(ubase, TRI2_E3, TRI3_E3_xx);
    build_urow(ubase, TRI2_ED, TRI3_Ed_xx);
    build_urow(ubase, TRI2_EE, TRI3_Ee_xx);
    build_urow(ubase, TRI2_xx, TRI3_xx_xx);
    
    build_urow(ubase, TRI3_E0_xx, 0);
    build_urow(ubase, TRI3_E1_xx, 0);
    build_urow(ubase, TRI3_E1_9a, 0);
    build_urow(ubase, TRI3_E2_80, 0);
    build_urow(ubase, TRI3_E2_81, 0);
    build_urow(ubase, TRI3_E2_xx, 0);
    build_urow(ubase, TRI3_E3_80, 0);
    build_urow(ubase, TRI3_E3_81, 0);
    build_urow(ubase, TRI3_E3_xx, 0);
    build_urow(ubase, TRI3_Ed_xx, 0);
    build_urow(ubase, TRI3_Ee_xx, 0);
    build_urow(ubase, TRI3_xx_xx, 0);

    table[ubase + TRI2_E1][0x9a] = ubase + TRI3_E1_9a;
    table[ubase + TRI2_E2][0x80] = ubase + TRI3_E2_80;
    table[ubase + TRI2_E2][0x81] = ubase + TRI3_E2_81;
    table[ubase + TRI2_E3][0x80] = ubase + TRI3_E3_80;
    table[ubase + TRI2_E3][0x81] = ubase + TRI3_E3_81;
    
    
    /*
     * Four byte
     */
    build_urow(ubase, QUAD2_xx, QUAD3_xx_xx);
    build_urow(ubase, QUAD2_F0, QUAD3_F0_xx);
    build_urow(ubase, QUAD2_F4, QUAD3_F4_xx);

    build_urow(ubase, QUAD3_xx_xx, QUAD4_xx_xx_xx);
    build_urow(ubase, QUAD3_F0_xx, QUAD4_F0_xx_xx);
    build_urow(ubase, QUAD3_F4_xx, QUAD4_F4_xx_xx);

    build_urow(ubase, QUAD4_xx_xx_xx, 0);
    build_urow(ubase, QUAD4_F0_xx_xx, 0);
    build_urow(ubase, QUAD4_F4_xx_xx, 0);

    /*
     * Mark Unicode spaces
     */
    if (iswspace(0x0085))
        table[ubase + DUO2_C2][0x85] = WASSPACE;
    if (iswspace(0x00A0))
        table[ubase + DUO2_C2][0xA0] = WASSPACE;
    if (iswspace(0x1680)) /* 0x1680 = 0xe1 0x9a 0x80 = OGHAM SPACE MARK*/
        table[ubase + TRI3_E1_9a][0x80] = WASSPACE;
    for (i=0x2000; i<0x200b+1; i++) {
        if (iswspace(i))
            table[ubase + TRI3_E2_80][0x80 + (i&0x6F)] = WASSPACE;
    }
    if (iswspace(0x2028))
        table[ubase + TRI3_E2_80][0xA8] = WASSPACE;
    if (iswspace(0x2029))
        table[ubase + TRI3_E2_80][0xA9] = WASSPACE;
    if (iswspace(0x202F))
        table[ubase + TRI3_E2_80][0xAF] = WASSPACE;
    if (iswspace(0x205F))
        table[ubase + TRI3_E2_81][0x9F] = WASSPACE;
    if (iswspace(0x3000))
        table[ubase + TRI3_E3_80][0x80] = WASSPACE;


    /*
     * Mark illegal sequences
     *
     * The following need to be marked as illegal because they can
     * be represented with a shorter string. In other words,
     * 0xC0 0x81 is the same as 0x01, so needs to be marked as an
     * illegal sequence.
     */
    for (i=0x80; i<0xA0; i++) {
        table[ubase + TRI2_E0][i] = ubase + ILLEGAL;
    }
    for (i=0x80; i<0x90; i++) {
        table[ubase + QUAD2_F0][i] = ubase + ILLEGAL;
    }
    /* Exceeds max possible size of unicode character */
    for (i=0x90; i<0xC0; i++) {
        table[ubase + QUAD2_F4][i] = ubase + ILLEGAL;
    }
    /* Surrogate space */
    for (i=0xA0; i<0xC0; i++) {
        table[ubase + TRI2_ED][i] = ubase + ILLEGAL;
    }
    
}

void **table_p;

unsigned PSTATE(void **p)
{
    return ((char*)p - (char*)table_p)/256;
}
void **compile_pointers()
{
    void **pp;
    size_t i;
    size_t j;

    pp = malloc(sizeof(*pp) * 256 * STATE_MAX);
    if (pp == NULL)
        abort();
    table_p = pp;
    for (i=0; i<STATE_MAX; i++) {
        for (j=0; j<256; j++) {
            pp[i*256 + j] = (char*)table_p + table[i][j]*256;
        }
    }
    
    return pp;
}

void
parse_chunk_p(const unsigned char *buf, size_t length, struct results *results, unsigned *inout_state)
{
    void **state = (void**)((char*)table_p + *inout_state);
    size_t i;
    unsigned long counts[STATE_MAX];
    if (table_p == NULL)
        table_p = compile_pointers();
    
    state = (void**)((char*)table_p + *inout_state);
        
    /* We only care about the first four states, so these will be initialized to zero.
     * Since we don't use the other ~100 counts for the other states, we won't initialize them */
    counts[NEWLINE] = 0;
    counts[NEWWORD] = 0;
    counts[WASSPACE] = 0;
    counts[WASWORD] = 0;

    /* This is the inner-loop where 99.9% of the execution time of this program will
     * be spent. */
    for (i=0; i<length; i += 1) {
        unsigned char c = buf[i];
        state = state[c];
        counts[((char*)state - (char*)table_p)/256]++;
    }

    /* Now update the results with what we found in the inner-loop */
    results->line_count += counts[NEWLINE];
    results->word_count += counts[NEWWORD];
    results->char_count += counts[NEWLINE] + counts[WASSPACE] + counts[WASWORD] + counts[NEWWORD];
    results->byte_count += length;
    *inout_state = (char*)state - (char*)table_p;
}

static void
parse_chunk(const unsigned char *buf, size_t length, struct results *results, unsigned *inout_state)
{
    unsigned state = *inout_state;
    size_t i;
    unsigned long counts[STATE_MAX];
    unsigned char c;

    /* We only care about the first four states, so these will be initialized to zero.
     * Since we don't use the other ~100 counts for the other states, we won't initialize them */
    counts[NEWLINE] = 0;
    counts[NEWWORD] = 0;
    counts[WASSPACE] = 0;
    counts[WASWORD] = 0;

    /* This is the inner-loop where 99.9% of the execution time of this program will
     * be spent. */
    for (i=0; i<length; i++) {
        c = buf[i];
        state = table[state][c];
        counts[state]++;
    }

    /* Now update the results with what we found in the inner-loop */
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
    unsigned state = 0;
    unsigned long line_count = 0;
    unsigned long word_count = 0;
    unsigned long char_count = 0;
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

    return 1;
}


int selftest_legal(void)
{
    unsigned i;
    unsigned char buf[4];

    for (i=0; i<0xFFFFFFFF; i++) {
        size_t length;
        int x;
        int y;
        wchar_t wc;
        mbstate_t state;
       
	memset(&state, 0, sizeof(state));
        
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
        y = is_legal(buf, length);
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

/**
 * This function compiles a DFA-style state-machine for parsing UTF-8 
 * variable-length byte sequences.
 */
void compile_utf8_statemachine(int is_multibyte)
{
    if (is_multibyte) {
        setlocale(LC_ALL, "");
        build_WASSPACE(table[WASSPACE]);
        build_WASSPACE(table[NEWLINE]);
        build_WASWORD(table[WASWORD]);
        build_WASWORD(table[NEWWORD]);
        build_unicode(NEWWORD, USPACE);
        build_unicode(WASWORD, UWORD);
    } else {
        int c;
        setlocale(LC_ALL, "");
        for (c=0; c<256; c++) {
            if (c == '\n') {
                table[WASSPACE][c] = NEWLINE;
                table[NEWLINE][c] = NEWLINE;
                table[NEWWORD][c] = NEWLINE;
                table[WASWORD][c] = NEWLINE;
            } else if (isspace(c)) {
                table[WASSPACE][c] = WASSPACE;
                table[NEWLINE][c] = WASSPACE;
                table[NEWWORD][c] = WASSPACE;
                table[WASWORD][c] = WASSPACE;
            } else {
                table[WASSPACE][c] = NEWWORD;
                table[NEWLINE][c] = NEWWORD;
                table[NEWWORD][c] = WASWORD;
                table[WASWORD][c] = WASWORD;
            }
        }
    }
}

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

/** 
 * Parse an individual file, or <stdin>, and print the results 
 */
struct results 
parse_file(FILE *fp, struct results *totals)
{
    enum {BUFSIZE=65536};
    struct results results = {0};
    unsigned state = 0; /* state held between chunks */
    unsigned char buf[BUFSIZE];

    /* Process a 64k chunk at a time */
    for (;;) {
        ssize_t count;

        count = fread(buf, 1, BUFSIZE, fp);
        if (count <= 0)
            break;
        parse_chunk(buf, count, &results, &state);
    }

    totals->line_count += results.line_count;
    totals->word_count += results.word_count;
    totals->byte_count += results.byte_count;
    totals->char_count += results.char_count;

    return results;
}

/**
 * Calculate the width for the columns, so that when printing the
 * results from several files, all the columns will line up. The
 * width for all the columns is determined by the size of the files.
 */
unsigned
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

    /* Read in the configuration parameters from the command-line */
    cfg = read_command_line(argc, argv);

    /* Compile the ASCII/UTF8 state-machine that we'll use to
     * parse multi-byte characters */
    compile_utf8_statemachine(cfg.is_counting_chars);

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

        results = parse_file(fp, &totals);
        print_results(filename, &results, &cfg);

        fclose(fp);
    }

    /* If no files specified, or the "-" file specified, then
     * we need to read in <stdin>. We need to change the mode
     * to "binary", to prevent the library from doing it's own
     * notions of text processing */
    if (cfg.is_stdin) {
        struct results results;
        FILE *fp;
        fp = freopen(NULL, "rb", stdin);
        if (fp == NULL) {
            perror("stdin");
            fp = stdin;
        }
        results = parse_file(fp, &totals);
        print_results(NULL, &results, &cfg);
    }

    /* If we read more than one thing, then we also need to print an
     * additional totals line */
    if (cfg.is_printing_totals)
        print_results("total", &totals, &cfg);

    return 0;
}
