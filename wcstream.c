/*
    Copies <stdin> to <stdout> one byte at a time. This is intended
    to expose a bug in `wc` whereby it gives different results 
    depending on how many bytes it reads at a time.
*/
#include <unistd.h>
#include <stdlib.h>

int main()
{
    for (;;) {
        char buf[65536];
        size_t length;
        size_t i;

        length = read(STDIN_FILENO, buf, sizeof(buf));
        if (length <= 0)
            break;
        for (i=0; i<length; i++) {
            size_t count;

            count = write(STDOUT_FILENO, buf+i, 1);
            if (count <= 0)
                exit(1);
        }
    }
    return 0;
}
