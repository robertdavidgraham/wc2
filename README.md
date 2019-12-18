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


## Pointer arithmetic

C has a peculiar idiom called "pointer arithmetic", where pointers can
be incremented. Many use this idiom under the false belief that it's 
faster. While in some cases it can be, in the general case, it is not.

In our code samples, when we parse the bytes in a buffer, we do something
like the following, which is valid C and JavaScript code:

    while (i < length) {
        c = buf[i++];
    }

The pointer arithmetic version would look like the following:

    end = buf + length;
    while (buf < end) {
        c = *buf++;
    }

To benchmark the difference, our programs have a switch `-P` that
uses pointer-arithmetic instead. We then benchmark the results across
a range of CPUs and compilers. The units here are "clock cycles". The
inner loop of the program takes from 3 to 12 clock cycles.


| CPU            | GHz  | compiler            | wc2a | wc2a -P | Diff |
|----------------|------|---------------------|------|---------|------|
| Intel i7-5650U | 3.1  | gcc v8.3.0          | 6.0  | 6.2     |  -4% |
|                | 3.1  | clang v7.0.1-8      | 4.8  | 5.5     | -15% |
|                | 3.1  | clang v11.0         | 3.6  | 4.2     | -19% |
|                | 3.1  | gcc v9.2.0-1        | 4.2  | 4.2     |   0% |
| Intel x5-Z8350 | 1.44 | gcc v4.8.4          | 9.3  | 9.3     |  -1% |
|                | 1.44 | clang v3.4          | 11.6 | 11.6    |   0% |
|                | 1.44 | gcc v9.2.1          | 9.5  | 9.6     |   0% |
|                | 1.44 | clang v9.0.0-2      | 8.3  | 9.6     | -16% |
| ARM Cortex A53 | 1.2  | gcc v6.3.0          | 11.2 | 11.2    |   0% |
|                | 1.2  | clang 3.8.1-24+rpi1 | 12.0 | 12.9    |  -8% |
| ARM Cortex A72 | 1.5  | gcc v9.2.1          | 4.2  | 4.2     |   0% |
|                | 1.5  | clang v9.0.0-2      | 5.6  | 5.6     |   0% |
| ARM Cortex A73 | 1.8  | gcc v7.4.0          | 5.5  | 5.1     |   8% |

From these results we can see:
* Half the time, there is no difference.
* Only once is there a slight increase in speed, much more often there
  is a decrease in speed.

The amount of time we can expect to save by replacing an *index* with
*pointer arithmetic* is one clock cycle per byte. Therefore, we report
benchmark results in terms of clock cycles.

As we can see, even with short loops of just a few clock cycles in 
length, we still do not see the benefit of pointer arithmetic that
programmers think they should achieve.

Programmers are encouraged to program in a language's own idioms.
Pointer arithmetic is idiomatic C. However, like `goto` or global
variables, it's a bad idiom. It's something that's useful in isolated
cases, but it not a good idiom for general purpose programming. It 
certainly doesn't improve speed in the general case.

