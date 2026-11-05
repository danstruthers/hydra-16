/*
** hfile.c - (dq0acct, overlay 3) the library's hmchar files (sdk/c/lib/harnfile.c, harnkey.c) compiled into the overlay, so none of it is in
** the main RAM (games/dq0harn/harnfile.c's way)
*/

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")
#pragma bss-name ("OV3RODATA")
#pragma local-strings (on)

#include "../../sdk/c/lib/harnkey.c"
#include "../../sdk/c/lib/harnfile.c"
