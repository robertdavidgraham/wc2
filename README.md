# wc2 - optimizing wc with asynchronous state machine parsing

This project implements the classic Unix `wc` command-line program 
using *asynchronous state machine* parsers, in both C and JavaScript.
Such parsers are an important part of how the modern Internet works,
but at the same time, rarely taught in school or covered in textbooks.
This project is for learning about such parsers.

## The code

This projects contains a number of programs:

* `wc2o`
    This is a minimalistic state-machine program, parsing only ASCII,
    and only `stdin`. The 'o' stands for 'obfuscate C version', as
    it's really hard to read to code to determine what it does. It
    does show the essence of state-machines, however.

* `wc2a`
    An implementation of an ASCII-only version of `wc` in the fastest,
    simplest manner.

* `wc2u`
    Includes Unicode UTF-8 parsing. This builds a state-machine with
    around 70 states. However, the evaluation is still very simple.

* `wctool`
    Used to create various test files.

* `wcdiff`
    Generates random data then 'diffs' it between the built-in `wc`
    program and one of the programs in this project. It searches for
    the minimal pattern that produces a difference between the two
    programs. For legal text input, the results should be the same.
    For illegal input, the results are undefined. Each different
    version of `wc` will produce different results with illegal input.

## Performance

The `wc` program is a popular benchmark target, because it's perceived as the
most minimal, trivial program that does useful processing on input.

However, it's not so simple. For one thing, because of the need to parse
UTF-8 text, it's not so simple -- UTF-8 parsing is a much more difficult problem
than simple word counting. Secondly, the speed of the program varies widely
for different types of input.

* Illegal characters cause `wc` to slow down.
* Random spaces/non-spaces stress the CPU branch prediction, slowing things down.
* All-space text is faster than all-word (non-space) text.

## Benchmarks

On macOS, some benchmarks for the built-in program are as follows. Two versions
of the command are run "-lwc" is the default mode, counting bytes, and "-lwm" 
is the mode that counts UTF-8 multi-byte characters.

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

* Illegal characters (in `pocorgtfo18.pdf`) slows things down a lot,
  twice as slow on macOS, 10x slower on Linux.
* Text that randomly switches between spaces and words is much slower
  than text containing all the same character.
* On Linux, the code path that reads all spaces is significantly faster.
* The macOS program is in general much faster than the Linux version.
* Processing Unicode (the file `utf8.txt` with the `-m` option) is slower
  than processing ASCII (the file `ascii.txt` with the `-c` option).

The time for our two programs are as follows:

| Program | Input File    | macOS | Linux |
|---------|---------------|------:|------:|
| wc2a.c  | (all)         |0.110  | 0.182 |
| wc2a.js | (all)         |0.281  | 0.392 |
| wc2u.c  | (all)         |0.206  | 0.278 |
| wc2u.js | (all)         |0.281  | 0.488 |

These results tell us:

* This state machine approach always results in the same speed, regardless
  of input. Constant-time parsing is considered a useful feature for security
  reasons.
* This state machine approach is faster than the built-in programs.
* Even written in JavaScript, the state machine approach is competitive in speed.

## Asynchronous

The legacy way of parsing is to combine the part that gets input (reading from
a file or receiving data from the network) with the part that does the parsing.
We see that in the various implementations of `wc`, where reading from the files
is integrated with the parsers.

The asynchronous approach is where one part of the program reads input, then
passes that input to a parser. This is the technique used in our parsers.

In particular, the key part of our code is a function

    struct results 
    parse_chunk(    const unsigned char *buf,
                    size_t length,
                    unsigned *state);

Some other part of the code reads from a file, a chunk at a time, then passes
is to the parser function. The parser maintains state from one chunk to the next
in the "state" variable. It returns counts (line-count, word-count, char-count)
in the results structure.

Let's say that we wanted to optimize the program so that it could simultaneously
parsing many files at the same time. With the sequential approach, we'd have to 
spawn a thread/process per file. With the asynchronous approach, we can do this
in a single thread (or single thread-per-core) using the AIO feature (asynchronous
input output).

This isn't important for files, but is important for network processes. The legacy
Apache method of writing a web server spawns a thread/process per TCP connection.
Almost all major alternatives to Apache use the asynchronous approach, using
an asynchronous function like `epoll()` or `kqueue()` to read network traffic,
then passing it to the parser asynchronously.

## State machine parsers

The minimalistic `wc2o.c` program is shown below:

    #include <stdio.h>
    int main(void)
    {
        static const unsigned char table[4][4] = {
            {2,0,1,0,}, {2,0,1,0,}, {3,0,1,0,},  {3,0,1,0,}
        };
        static const unsigned char trans[256] = {
            0,0,0,0,0,0,0,0,0,1,2,1,1,1,0,0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            1,0,
        };
        unsigned long counts[4] = {0,0,0,0};
        int state = 0;
        int c;
        while ((c = getchar()) != EOF)
            counts[state = table[state][trans[c]]]++;
        printf("%lu %lu %lu\n", counts[1], counts[2], 
                    counts[0] + counts[1] + counts[2] + counts[3]);
        return 0;
    }

The key part that does all the word counting is in the two lines:

        while ((c = getchar()) != EOF)
            counts[state = table[state][trans[c]]]++;

It's not obvious from these two lines that word-counting is going on,
which is why we call this an "obfuscated C" version of the word-count.
The logic is driven by the tables at the top of the program.

The program `wc2u.c` is essentially the same, except that it builds
very complicated state-machine tables to handle the complexities 
of UTF-8 decoding.

In the above code, the `trans` table is used to compress the state-machine
table. Instead of 256 columns per row in `table`, we can reduce this to
merely 3 columns per row, because all way care about is whether a character
is a non-space(0), space(1), or newline(2).

Our `wc2u.c` program doesn't have this step. Also, we don't try to
compact things as in our obfuscated version. So it's inner loop looks
like:

    for (i=0; i<length; i++) {
        c = buf[i];
        state = table[state][c];
        counts[state]++;
    }

JavaScript doesn't do two-dimensional tables well, so we manually unroll
the table into a single dimension in `wc2u.js`:

    for (i=0; i<length; i++) {
        c = buf[i];
        state = table[state  * 256 + c];
        counts[state]++;
    }

Despite these syntactic differences, all the versions of this state-machine 
inner-loop are the same.
