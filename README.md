# wc2 - asynchronous state machine parsing

There have been multiple articles lately implementing the 
classic `wc` program in various programming *languages*, to
"prove" their favorite language can be "just as fast" as C.

This project does something different.
Instead of a different *language* it uses a different *algorithm*.
The new algorithm is significantly faster -- implementing in a
slow language like JavaScript is still faster than the original
`wc` program written in C.

The algorithm is known as an "asynchronous state-machine parser".
It's a technique for *parsing* that you don't learn in college.
It's more *efficient*, but more importantly, it's more *scalable*.
That's why your browser uses a state-machine to parse GIFs,
and most web servers use state-machines to parse incoming HTTP requests.

This projects contains three versions:
* `wc2o.c` is a simplified 25 line version highlighting the idea
* `wc2.c` is the full version in C, supporting Unicode
* `wc2.js` is the version in JavaScript

## The basic algorithm

The algorithm reads input and passes each byte one at a time
to a state-machine. It looks something like:

```c
    length = fread(buf, 1, sizeof(buf), fp);
    for (i=0; i<length; i++) {
        c = buf[i];
        state = table[state][c];
        counts[state]++;
    }
```

No, you aren't suppose to be able to see how the word-count works
by looking at this code. The complexity happens elsewhere, setting
up the state-machine.

The state-machine table is the difference between the simple version
(`wc2o.c`) and complex version (`wc2.c`) of the program. The algorithm
is the same, the one shown above, the difference is in how they setup
the table. The simple program creates a table for ASCII, the complex
program creates a much larger table supporting UTF-8.


## How `wc` works

The `wc` word-count program counts the number of words in a file. A "word"
is some non-space characters separated by space.

Those who re-implement `wc` simplify the problem by only doing ASCII instead
of the full UTF-8 Unicode. This is cheating, because much of the speed of
`wc` comes from its need to handle character-sets like UTF-8.
The real programs spend most of their time
in functions like `mbrtowc()` to parse multi-byte characters and
`iswspace()` to test if they are spaces -- which re-implementations
of `wc` skip.

For this reason, we've implemented a full UTF-8 version in this project, to
prove that it works without cheating. Now the real `wc` works with a lot
more character-sets, and we don't do that. But by implementing UTF-8, we've
shown that it's possible, and that the speed for any character-set is the same.

Another simplification is how invalid input is handled. The original `wc` program
largley ignores errors, but it's still an important factor in making sure you
are doing things correctly.

## Benchmark input files

This project uses a number of large input files for benchmarking.
The traditional `wc` program has wildly different performance depending
upon input, such as whether the file is full of illegal characters, or
whether UTF-8 is being handled. The first test file is downloaded
from the Internet as "real-world data", while the others are generated
using a program built with this project (`wctool`).
* `pocorgtfo18.pdf` a large 92-million byte PDF file that contains binary/illegal characters
* `ascii.txt` a file the same size containing random words, ASCII-only
* `utf8.txt` a file containing random UTF-8 sequences of 1, 2, 3, and 4 bytes
* `word.txt` a file containing 92-million 'x' characters
* `space.txt` a file containing 92-million ' ' (space) characters

Before benchmarking the old `wc`, set the character-set to UTF-8. It's
probably already set to this on new systems, but do this to make sure:

    $ export LC_CTYPE=en_US.UTF-8

When running `wc`, the `-lwc` is the default for counting words in ASCII text.
To convert it into UTF-8 "multi-byte" mode, change `c` o `m`, as in `-lwm`.

The numbers reported come from the Unix `time` command, the number of seconds for
`user` time. In other words, `elapsed` time or `system` time aren't reported.

The following table shows benchmarking a 2019 x86 MacBook Air with the old
`wc` program. As you can see, it has a wide variety of speeds depending
on input.

The `wc` program included with macOS and Linux are completely different.
Therefore, the following table shows them benchmarked against each other
on the same hardware.


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

The times for our algorithm, in C and JavaScript, are the following.
The state-machine parser is immune to input type, all the input files
show the same results.

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

## Asynchronous scalability

The algorithm is *faster*, but more importantly, it's more *scalable*.

Such scalability isn't usefull for `wc`, but is incredibly important for network
programs. Consider an HTTP web-server. The traditional way that the Apache web-server
worked was by reading the entire header in and buffering it, before then parsing
the header. This need to buffer the entire header caused an enormous scalability
problem. In contrast, asynchronous web-servers like Nginx use a state-machine
parser. They parse the bytes as they arrive, and discard them.

This is analogous to NFA and DFA regular-expressions. If you use the NFA
approach, you need to buffer the entire chunk of data, so that the regex
can backtrack. Using the DFA approach, input can be provided as a stream,
one byte at a time, without needing buffering. DFAs are more scalable than NFAs.

## State machine parsers

This project contains a minimalistic `wc2o.c` program to highlight
the algorithm, without all the fuss of building UTF-8 tables, supporting
only ASCII.

```c
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

```c
    while ((c = getchar()) != EOF) {
        state = table[state][column[c]];
        counts[state]++;
    }
```

This is only defined for ASCII, so you can see the state-machine on a
single-line in the code (`table`).

## Additional tools

This project includes additional tools:
 * `wctool` to generate large test files
 * `wcdiff` to find difference between two implementatins of `wc`
 * `wcstream` to fragment input files (demonstrates a bug in macOS's `wc`)


The program `wc2.c` has the same logic, the difference being that it 
generates a larger state-machine for parsing UTF-8.


## Pointer arithmetic

C has a peculiar idiom called "pointer arithmetic", where pointers can
be incremented. Looping through a buffer is done with an expression like
`*buf++` instead of `buf[i++]`. Many programmers think pointer-arithmetic
is faster.

To test this, the `wc2.c` program has an option `-P` that makes this
small change, to test the difference in speed.

