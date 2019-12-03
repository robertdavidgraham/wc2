/* Generates random text with spaces to stress 'wc' programs,
 * in particular, to stress branch prediction units */
#include <stdio.h>
#include <ctype.h>
#include <string.h>

static unsigned
r_rand(unsigned *seed)
{
    static const unsigned a = 214013;
    static const unsigned c = 2531011;

    *seed = (*seed) * a + c;
    return (*seed)>>16 & 0x7fff;
}

void gen_utf8(void)
{
    size_t i;
    unsigned seed = 0;
    static const char *list[] = { 
        "的",
        "一",
        "是",
        "文",
        "な",
        "😂",
        "❤️",
        "💩",
        " ",
        "᠎",
        "　",
        "\n",
        " ",
        "﻿", /* U+FEFF ZERO WIDTH NO-BREAK SPACE */
        " ", /* U+1680 OGHAM SPCE MARK */
        " ", /* U+00A0 NO-BREAK SPACE */
        0};

        
    fprintf(stderr, "[+] generating utf8 file\n");
    for (i=0; i<92296537; ) {
        const char *out = list[r_rand(&seed)%16];
        printf("%s", out);
        i += strlen(out);
    }
}

void gen_ascii(void)
{
    size_t i;
    unsigned seed = 0;

    fprintf(stderr, "[+] generating ascii file\n");
    for (i=0; i<92296537; i++) {
        putchar(" x\ty\rz\na"[r_rand(&seed)%8]);
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1 && (tolower(argv[1][0]) == 'u' || strcmp(argv[1], "-u") == 0 || strcmp(argv[1], "--utf8") == 0))
        gen_utf8();
    else
        gen_ascii();

    return 0;
}

