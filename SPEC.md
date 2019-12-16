wc - specification
===

This document attempts to document a specification for the `wc` program
as actually implemented in the real world, such as in the GNU `coreutils`,
BusyBox, macOS, FreeBSD, QNX, Solaris, and so on.

Unix/POSIX standard
---

The POSIX standard for 'wc' is given here:

    https://pubs.opengroup.org/onlinepubs/007904975/utilities/wc.html

The command-line parameters are:

    wc [-lw][-c|-m][file...]

Where:
    -l 
        Print the number newline '\n' characters in the file.
        This is not the number of lines, as a file containing some
        text but no newlines will print 0 here as the result.

    -w
        Print the number of words in the file. Specifically,
        this is the number of non-space characters preceeded
        by either a space character or the start of the file.

    -c
        Print the number of bytes in the file. This will be just
        the file size if it's a regular file, or the number of bytes
        if a stream like `stdin`. Exclusive wit the `-m` parameter.

    -m
        Print the number of multibyte characters in the file. A 
        two-byte sequence will therefore count as one character.
        This will be equal to or fewer than the number of bytes.
        This is mutually exclusive with `-c` parameter, only
        one or the other may be specified.

If no options are specified, then the default will be
"-lwc".

Normal syntax guidelines are followed. This means the options
can be specified separate, as in -l -w -c, or grouped,
as in "-lwc".


Simple tests
---

The number of *lines* is counted solely when the newline, '\n',
character is seen in the input stream. In other words:

    $ echo -n "test" | wc -l
    0

This is because even though there is a line of text, there is no 
newline character, and hence, the result is zero.

The number of *words* is counted solely when a two-character combination
is seen where where the first is a space (according to `iswspace()`), and
the second is not. For this purpose, the preceding character before
the start of input is assumed to have been a space character.

    $ echo -n "test" | wc -w
    1
    $ echo -n " test" | wc -w
    1
    $ echo -n "test " | wc -w
    1
    $ echo -n "test again" | wc -w
    2

The number of *bytes* in a file is simply the file size. However, this
can't be calculated solely by running a function like `stat` to determine
filesize, because the input may be piped in via `stdin`. Therefore, it
must reflect the total number of bytes read from the input.

    $ echo -n "test" | wc -c
    4

The number of *characters* is the same as the number of *bytes* for
single-byte character sets (ASCII, EBCDIC), but may be fewer than the
number of *bytes* for multi-byte character sets (UTF-8, ShiftJIS).
This is controlled by the *locale*.

Character set
---

The trivial understanding of word-count is that it operates on ASCII files.
In reality, the program operates on the currently configured character set.

Before the birth of the Internet in 1983, text files remained internal to
a computer. There was no expectation of transport such files between computers
of different manufacturers. Every manufacturer therefore used a different
character set. The character set would further be defined by the market
where the computer was sold, to conform to the local language.

The to most popular character-sets have been ASCII and EBCDIC. ASCII defines
the first 7-bits of a byte, and it the basis for most character sets, where
the 8th bit are custom characters, different for different vendors. EBCDIC is
an entirely different character set used by IBM mainframes, which once dominated
commercial computing, but are today little more than legacy that few people
have direct experince programming.

Asian languages in particular have used *multi-byte* character-sets. Some 
of these character-sets simply encode a single character in multiple bytes.
Others use "shift" states, where a symbol is encountered that changes the
interpretation of all future bytes until another "shift" happens. A good
example of this is "ShiftJIS" for Japanese.

When a C program starts up, the current character set is set to "C", which
is usually a single-byte character set related to either ASCII or EBCDIC.
Calling `setlocale(LC_CYPE,"")` causes the program to parse environmental
variables and reset the character-set accordingly.

Linux GNU coreutils always sets the locale this way, so the "word" count
reflects UTF-8 if that's the environment. However, macOS and FreeBSD only
does this if the `-m` parameter is set.

LC_CTYPE controls the behavior of `isspace()`/`iswspace()`,
changing which characters they may decide are whitespace or not. The
ISO 30112 standard specifies that POSIX Unicode whitespace characters
are U+0009..U+000D, U+0020, U+1680, U+180E, U+2000..U+2006, 
U+2008..U+200A, U+2028, U+2029, U+205F, and U+3000. This is discussed
at the following link:

    https://en.cppreference.com/w/c/string/wide/iswspace


UTF-8
--

UTF-8 is a *multi-byte* character-set that encodes a character in
1 to 4 bytes (inclusive).

The encoding table is shown below:

| bytes | bits |  first  |   last   |   byte1  |   byte2  |   byte3  |   byte4  |
|:-----:|:----:|:-------:|:--------:|:--------:|:--------:|:--------:|:--------:|
|   1   |    7 |  U+0000 |   U+007F | 0xxxxxxx |          |          |          |
|   2   |   11 |  U+0080 |   U+07FF | 110xxxxx | 10xxxxxx |          |          |
|   3   |   16 |  U+0800 |   U+FFFF | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
|   4   |   21 | U+10000 | U+10FFFF | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |

Encoding/decoding is supposed to be *strict*, rejecting invalid sequences.

Any Unicode value larger than 0x110000 is invalid and should not be encoded,
and an otherwise valid UTF-8 sequence that would decode to a larger number
should be rejected. Thus, there are 1,114,112 possible characters.

Redundant values should be rejected. A UTF-8 sequence of 0xC0 0x8A would otherwise
decode to 0x000A. Since this can be encoded using a 1-byte sequence, the 
redundant 2-byte sequence is invalid and should be rejected.

There are a set of Unicode valies between 0xD800 and 0xDFFF called surogates,
which not be either encoded into UTF-8 or decoded from UTF-8. An attempt to
encode these values in UTF-8 should be rejected.


Unicode Space Characters
--

Here is a list of various space characters, and whether they are recoginized
as spaces in the *iswspace* function on various platforms, where L=Linux,
M=macOS, W=Windows10:

    L M W 
    x x x    0x0009  ASCII tab '\t'
    x x x    0x000a  ASCII newline '\n'
    x x x    0x000b  ASCII vertical-tab '\v'
    x x x    0x000c  ASCII form-feed '\f'
    x x x    0x000d  ASCII carriage return '\r'
    x x x    0x0020  ASCII space ' ' '\b'
    . x x    0x0085  NEXT LINE (NEL)
    . x x    0x00a0  NO-BREAK SPACE
    x x x    0x1680  OGHAM SPACE MARK
    . . .    0x180E  MONGOLIAN VOWEL SEPARATOR
    x x x    0x2000  En Quad
    x x x    0x2001  Em Quad
    x x x    0x2002  En Space
    x x x    0x2003  Em Space
    x x x    0x2004  Three-per-Em Space
    x x x    0x2005  Four-per-Em Space
    x x x    0x2006  Six-per-Em Space
    x x x    0x2007  Figure Space
    x x x    0x2008  Punctuation Space
    x x x    0x2009  Thin Space
    x x x    0x200a  Hair Space
    . x x    0x200b  Zero Width Space  
    x x x    0x2028  Line Separator
    x x x    0x2029  Paragraph Separator
    . x x    0x202f  NNBSP - Narrow No-Break Space
    x x x    0x205f  MMSP - Medium Mathematical Space
    x x x    0x3000  Ideographic Space
    . . .    0xFEFF  ZERO WIDTH NO-BREAK SPACE

References:
    http://jkorpela.fi/chars/spaces.html

POSIXLY_CORRECT
---

The GNU coreutils deviate from the POSIX standard, but look for an
environement variable "POSIXLY_CORRECT", at which point they change
their behave to conform to the POSIX spec.

Line buffering of output
---

Programs consuming the ouput from `wc` typically expect to receive
it a line at a time, rathre than partial lines. Therefore, programs
should set output to line-buffering mode with the following
standard C library function:

    setvbuf(stdout, NULL, _IOLBF, 0);

Signals
---

The GNU coreutils use a function called `safe_read()` to read from files.
This is because the normal `read()` function can exit with an EINTR error
in the case of signals, which a program must always handle.

Pipes
---

The GNU coreutils register an atexit() function to cleanly close the `stdout`
pipe.


Binary
---

Input may be read in some "translated" mode, most famously on Windows which
translates CR/LF into LF. This needs to be disabled, running in "binary" mode.
