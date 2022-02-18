# wc2 - optimizing wc with asynchronous state machine parsing

There have been multiple articles lately implementing the 
classic `wc` program in various programming languages, to
prove they can be "just as fast" as C.

In this project, instead of a different language we choose
a different *algorithm*. We implement it in both C and JavaScript,
both of which result in a program that's faster than the
built-in `wc` program.

The algorithm is known as an "asynchronous state-machine parser".
The code is written for correct parsing of UTF-8, though the concept
can be extended to support any character-set without changing
the fundamental algorithm or benchmark speed.

This projects contains three versions:
* `wc2o.c` is a simplified 25 line version highlighting the idea
* `wc2.c` is the full version in C
* `wc2.js` is the version in JavaScript

There are some additional bits of code:
* `wctool` to generate large test files
* `wcdiff` to find difference between two implementatins of `wc`
* `wcstream` to fragment iput files (demonstrates a bug in macOS's `wc`)


## The basic algorithm

The algorithm reads input and passes each byte one at a time
to a state-machine. It looks something like:

    length = fread(buf, 1, sizeof(buf), fp);
    for (i=0; i<length; i++) {
        c = buf[i];
        state = table[state][c];
        counts[state]++;
    }

No, you aren't suppose to be able to see how the word-count works
by looking at this code. The complexity happens elsewhere, setting
up the state-machine. Elswhere, we've created a large state-machine
for parsing UTF-8 characters, which takes about 250 lines of code.



## How `wc` works

None of those recent re-implementations of `wc` are doing what they claim.
They are writing a simple algortihm for counting the single-byte ASCII
character-sets. The real `wc` program supports arbitrary character
sets, such as UTF-8. The real programs spend most of their time
in functions like `mbrtowc()` to parse multi-byte characters and
`iswspace()` to test if they are spaces -- which re-implementations
of `wc` skip.

Therefore, to start with, I benchmark a bunch of different input files
against existing versions of `wc` under macOS and Linux on my laptop
(MacBook Air 2017).

The files are:
* `pocorgtfo18.pdf` a large 92-million byte PDF file that contains binary/illegal characters
* `ascii.txt` a file the same size containing random words, ASCII-only
* `utf8.txt` a file containing random UTF-8 sequences of 1, 2, 3, and 4 bytes
* `word.txt` a file containing 92-million 'x' characters
* `space.txt` a file containing 92-million ' ' (space) characters

Before running the benchmarks, the character-set is configured as:

    $ export LC_CTYPE=en_US.UTF-8

When running `wc`, either `-lwc` is configured for single-byte text,
or `-lwm` is configured for multibyte text.

The numbers reported come from the `time` command, the number of seconds for
`user` time (not `elapsed` or `system` time).


| Command | Input File    | macOS | Linux |
|---------|---------------|------:|------:|
| wc -lwc |pocorgtfo18.pdf|0.709  | 5.591 |
| wc -lwm |pocorgtfo18.pdf|0.693  | 5.419 |
| wc -lwc |ascii.txt      |0.296  | 2.509 |
| wc -lwm |utf8.txt       |0.532  | 1.840 |
| wc -lwc |space.txt      |0.296  | 0.284 |
| wc -lwm |space.txt      |0.295  | 0.298 |
| wc -lwc |word.txt       |0.302  | 1.268 |
| wc -lwm |word.txt       |0.294  | 1.337 |

These results tell us:

* Illegal characters (in `pocorgtfo18.pdf`) slow things down a lot,
  twice as slow on macOS, 10x slower on Linux.
* Text that randomly switches between spaces and words is much slower
  than text containing all the same character.
* On Linux, the code path that reads all spaces is significantly faster.
* The macOS program is in general much faster than the Linux version.
* Processing Unicode (the file `utf8.txt` with the `-m` option) is slower
  than processing ASCII (the file `ascii.txt` with the `-c` option).

## Our benchmarks

The time for our algorithm, in C and JavaScript, are the following.

| Program | Input File   | macOS | Linux |
|---------|--------------|------:|------:|
| wc2.c   | (all)        |0.206  | 0.278 |
| wc2.js  | (all)        |0.281  | 0.488 |

These results tell us:

* This state machine approach always results in the same speed, regardless
  of input.
* This state machine approach is faster than the built-in programs.
* Even written in JavaScript, the state machine approach is competitive in speed.
* The difference in macOS and Linux speed is actually the difference in `clang` and `gcc`
  speed. The LLVM `clang` compiler is doing better optimizations for x86 processors here.
* I don't know why Node.js behaves differently on macOS and Linux, it's probably just
  due to different versions.
* A JIT (like NodeJS) works well with simple compute algorithms. This tells
  us little about it's relative performance in larger programs. All languages
  that have a JIT should compile this sort of algorithm to roughly the same
  speed.

## Asynchronous

The legacy way of parsing couples *receiving input* with *parsing*. What we
are doing here is decoupling the two.

In other words, a common way of doing this would be to write a function
like `getline()` or `getword()` that reads input inside the parser to the next
line or word boundary. In our case, we read in 64k chunks at a time, regardless
of where boundaries might exist, and pass each chunk to the parser.

There's really no benefit doing it this way for `wc`, but there is a huge
benefit for network applications. The older Apache web-server does things
in the legacy, non-asynchronous way, but is steadily being replaced by
asynchronous servers like nginx and Lighthttpd that use asynchronous
techniques.

## State machine parsers

The minimalistic `wc2o.c` program is shown below in its entirety. We've hard-coded the
state-machine here.

```
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

    while ((c = getchar()) != EOF) {
        state = table[state][column[c]];
        counts[state]++;
    }

    printf("%lu %lu %lu\n", counts[1], counts[2], 
                counts[0] + counts[1] + counts[2] + counts[3]);
    return 0;
}
```

The key part that does all the word counting is in the two lines inside:
```
    while ((c = getchar()) != EOF) {
        state = table[state][column[c]];
        counts[state]++;
    }
```

This is only defined for ASCII, so you can see the state-machine on a
single-line in the code (`table`).

The program `wc2.c` has the same logic, the difference being that it 
generates a larger state-machine for parsing UTF-8.


## Pointer arithmetic

C has a peculiar idiom called "pointer arithmetic", where pointers can
be incremented. Looping through a buffer is done with an expression like
`*buf++` instead of `buf[i++]`. Many programmers think pointer-arithmetic
is faster.

To test this, the `wc2.c` program has an option `-P` that makes this
small change, to test the difference in speed.

