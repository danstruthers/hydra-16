/*
** hfall.c - (dq0harn) HarnMaster's falls (the library's, ../../sdk/c/lib/harnfall.c: harn_fall, Skills 8), overlay
** 10's, the referee's main RAM's room (GAME-CRITIQUE.md's E10, 2026-11-19): its one caller, sheet.c's harm (overlay 1),
** calls it through its stub (bank.s's harn_fall); every pointer it's given the main RAM's
*/

#pragma code-name ("OV10CODE")
#pragma rodata-name ("OV10RODATA")

#define harn_fall       o_harn_fall
#define harn_fall_dice  o_harn_fall_dice

#include "../../sdk/c/lib/harnfall.c"
