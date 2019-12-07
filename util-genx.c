/* Generates random text with spaces to stress 'wc' programs,
 * in particular, to stress branch prediction units */
#include <stdio.h>
#include <ctype.h>
#include <string.h>

unsigned 
utf8_to_ucs4(unsigned char *buf, size_t sizeof_buf)
{
    if (sizeof_buf >= 1 && buf[0] < 0x80) {
        return buf[0];
    } else if (sizeof_buf >= 2 && (buf[0]&0xE0) == 0xC0) {
        return  (buf[0]&0x1F)<<6 
                | (buf[1] & 0x3F) << 0;
    } else if (sizeof_buf >= 3 && (buf[0]&0xF0) == 0xE0) {
        return (buf[0]&0x0F)<<12 
                | (buf[1] & 0x3F) << 6
                | (buf[2] & 0x3F) << 0;
    } else if (sizeof_buf >= 4 && (buf[0]&0xF1) == 0xF0) {
        return (buf[0]&0x07)<<18 
                | (buf[1] & 0x3F) << 12
                | (buf[2] & 0x3F) << 6
                | (buf[3] & 0x3F) << 0;
    } else
        return 0xFFFFFFFF;
}

size_t ucs4_to_utf8(unsigned char *buf, size_t sizeof_buf, unsigned wchar)
{
	#if 0
    if (0xD800 <= wchar && wchar <= 0xDFFF) {
        /* surrogates must not be encoded */
        return 0;
    }
	#endif

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
        "\xe7\x9a\x84",     /* U+7684 的 */
        "\xe4\xb8\x80",     /* U+4e00 一 */
        "\xe6\x98\xaf",     /* U+662f 是 */
        "\xe6\x96\x87",     /* U+6587 文 */
        "\xe3\x81\xaa",     /* U+306a な */
        "\xf0\x9f\x98\x82", /* U+1f602 😂 */
        "\xe2\x9d\xa4\xef\xb8\x8f", /* U+2764 ❤️ */
        "\xf0\x9f\x92\xa9", /* U+1f4a9 💩 */
        "\xe2\x80\x83",     /* U+2003   */
        "\xe1\xa0\x8e",     /* U+180e ᠎ */
        "\xe3\x80\x80",     /* U+3000 　 */
        "\n",               /* U+000a */
        " ",                /* U+0020   */
        "\xef\xbb\xbf",     /* U+feff ﻿ */
        "\xe1\x9a\x80",     /* U+1680   */
        "\t",               /* U+0009   */
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

