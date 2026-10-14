/*
** ovharn.c - (overlay 1) harn.h's names and its files, compiled into the game's first overlay rather than linked
** from the C library into its main RAM: only the title, the profile and loading a character use them.  (The
** game's definitions take the library's places: the linker brings a library module only for what's undefined.)
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

#include "../../sdk/c/lib/harnname.c"
#include "../../sdk/c/lib/harnfile.c"
#include "../../sdk/c/lib/harnkey.c"
