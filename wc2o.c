/*
    An Obfuscated-C version of 'wc' using state-machines
*/
#include <stdio.h>
int main(void)
{
    static const unsigned char table[4][3] = {
        {2,0,1}, {2,0,1}, {3,0,1}, {3,0,1}
    };
    static const unsigned char column[256] = {
        0,0,0,0,0,0,0,0,0,1,2,1,1,1,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
    };
    unsigned long counts[4] = {0,0,0,0};
    int state = 0;
    int c;

    while ((c = getchar()) != EOF) {
        state = table[state][column[c]];
        counts[state]++;
    }

    printf("%7lu %7lu %7lu\n", counts[1], counts[2], 
                counts[0] + counts[1] + counts[2] + counts[3]);
    return 0;
}
