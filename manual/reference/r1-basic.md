# Chapter 1. HyBASIC Keywords

HyBASIC compiles a program whole before it runs it: every line is read, every name resolved and every block
matched, and only then does the first statement execute. An error the compiler finds (a `FOR` with no `NEXT`, a
label that isn't there) is therefore reported before anything has run, with the line it was found on. A line typed
at the prompt without a number is compiled and run the same way, as a program of one line.

An error is reported as its message, in small letters. `ERR` then holds its number, `ERR$` the message and `ERL`
the line; the numbers given in each entry are `ERR`'s. A failure reported by the system (a disk that is full, a
name that isn't there) has HyBASIC's own number where there is one that fits (53, file not found), and otherwise
256 plus the system's code (Appendix G).

A number is an integer of any length, a rational, a decimal or a complex number, and arithmetic on them is exact
(Chapter 18). A string is a sequence of bytes, each 0 to 255, up to 64K of them.
