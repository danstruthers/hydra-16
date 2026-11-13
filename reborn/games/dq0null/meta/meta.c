/*
** meta.c - (dq0null's, the null ruleset's) the character's sheet as the programs between dungeons see it, and the scholar's
** house's hired readers (../../dq0/rsmeta.h).  Its rules are flat (dq0null.c's): no attributes and no masteries on its sheet
** (name, occupation, pace, wounds, its tongues and scripts: every line the program's own), every skill at its flat chance
** (BASE, dq0null.c's: its own tongue, its scripts, the lore, the stars ...), a tongue or a script a master opens put on its
** sheet with no mastery ("script delvic", as its referee reads one); a hired reader's skill the same flat chance, and every one
** of them honest (it has no Morality).  A meta program compiles it in (dq0nschol's rsmeta.c).
*/

#include "../../dq0/rsmeta.h"

#define BASE            60                      /* (A test's chance with no steps, percent: dq0null.c's) */

void __fastcall__ meta_line (char* l, void __fastcall__ (*own) (char* l))
{
    own (l);
}

unsigned char __fastcall__ meta_ml (unsigned char mk)
{
    (void) mk;
    return BASE;
}

unsigned char __fastcall__ meta_taught (unsigned char mk)
{
    (void) mk;
    return 0;
}

unsigned char __fastcall__ meta_hired (unsigned char sage)
{
    (void) sage;
    return BASE;
}

unsigned char __fastcall__ meta_honest (unsigned char set)
{
    (void) set;
    return MH_HONEST;
}
