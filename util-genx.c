#include <stdio.h>

static unsigned
r_rand(unsigned *seed)
{
    static const unsigned a = 214013;
    static const unsigned c = 2531011;

    *seed = (*seed) * a + c;
    return (*seed)>>16 & 0x7fff;
}

int main(void)
{
    size_t i;
    unsigned seed = 0;

    for (i=0; i<92296537; i++) {
        putchar(" x\ty\rz\na"[r_rand(&seed)%8]);
    }
    return 0;
}
