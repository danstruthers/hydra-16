/*
** harnfile.c - (dq0harn, swapped part 1: ../dq0/swap.inc) the C library's hmchar files (sdk/c/lib/harnfile.c: a
** character read and written), compiled here into the part with what calls them (dq0harn.c's load_pc, save_pc), so
** none of it is in the main RAM but while it's at work; the library's taken no more
*/

#pragma code-name ("SW1CODE")
#pragma rodata-name ("SW1RODATA")
#pragma local-strings (on)

#include "../../sdk/c/lib/harnfile.c"
