# wc2 - optimizing wc

This tiny project is for exploring the performance of 'wc', the UNIX
word-count program.

This program is often a benchmark because it's trivial. When testing 
more complex code, you want to benchmark against the most trivial 
example you can think of. Counting the number of words in a file fits
this purpose very well.

Nobody wants to optimize this program, of course. There's no need to have
a fast word-count program. However, this will throw off your benchmarks.
The 'wc' program may not be written as trivially as you think, and thus,
may not be a good benchmark comparison.

This project contains an actual trivial implementation 'wc' optimized a
bit for speed. It's at least 5x (five times) faster than the 'wc' built-in
to most computers, and sometimes 50x faster.

