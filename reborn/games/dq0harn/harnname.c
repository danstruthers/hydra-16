/*
** harnname.c - (dq0harn, swapped part 2: ../dq0/swap.inc) the C library's names (sdk/c/lib/harnname.c: an
** attribute's, a species', a sex's, a month's, a sunsign's ...), compiled here into the part, the library's taken no
** more; those the sheet's words call (sheet.c: overlay 1's) named s_*, each through its stub (bank.s's).  What they
** give is in the part (but a sunsign's, in the main RAM): used before another part's called
*/

#pragma code-name ("SW2CODE")
#pragma rodata-name ("SW2RODATA")
#pragma local-strings (on)

#define harn_attr_name      s_harn_attr_name
#define harn_species_name   s_harn_species_name
#define harn_sex_name       s_harn_sex_name
#define harn_month_name     s_harn_month_name
#define harn_sunsign_name   s_harn_sunsign_name

#include "../../sdk/c/lib/harnname.c"
