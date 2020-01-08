/*
    An Obfuscated-C version of 'wc' using state-machines
*/
#include <stdio.h>
int main(void)
{
    static const unsigned char table[4][4] = {
        {2,0,1,0,}, {2,0,1,0,}, {3,0,1,0,},  {3,0,1,0,}
    };
    static const unsigned char column[256] = {
        0,0,0,0,0,0,0,0,0,1,2,1,1,1,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
    };
    unsigned long counts[4] = {0,0,0,0};
    int state = 0;
    int c;

    while ((c = getchar()) != EOF)
        counts[state = table[state][column[c]]]++;

    printf("%lu %lu %lu\n", counts[1], counts[2], 
                counts[0] + counts[1] + counts[2] + counts[3]);
    return 0;
}
