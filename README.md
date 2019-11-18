# wc2 - optimizing wc

This tiny project is for exploring the slowness of 'wc', the Unix word count program.
The 'wc' program is often a benchmark target because it's trivially easy. However,
despite supposedly written in optimized C, it's fairly slow. Despite being trivial,
the source code for existing programs are actually fairly complex, and it's hard to
see exactly where the slowdown happens.

In this project, I write my own version of 'wc' as an optimized target, then work
backward to see where the performance issues come from.

