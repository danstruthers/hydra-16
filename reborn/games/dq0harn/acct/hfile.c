/*
** hfile.c - (HarnMaster's part of the account, its sheet overlay) the library's hmchar files (sdk/c/lib/harnfile.c, harnkey.c) compiled into the overlay, so none of it is in
** the main RAM (games/dq0harn/harnfile.c's way)
*/

#pragma code-name ("SHEETCODE")
#pragma rodata-name ("SHEETRODATA")
#pragma bss-name ("SHEETRODATA")
#pragma local-strings (on)

#include "../../../sdk/c/lib/harnkey.c"
#include "../../../sdk/c/lib/harnfile.c"
