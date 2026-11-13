/*
** meta.c - (dq0harn's, HarnMaster's) the character's sheet as the programs between dungeons see it (../../dq0/rsmeta.h):
** hmchar's file's lines (the library's harnfile.c's format: "att str 12", in harn.h's order; "ml lan 62"), the masteries
** a meta program asks for by what they're for, and what a master's teaching opens a tongue or a script at (the book's,
** Skills 11: a tongue at its skill base, an alien family's; a script at 70 and its skill base); and the scholar's house's
** hired readers (their skill, the book's EMLs for a scribe and a sage; their honesty, their Morality).  A meta program compiles
** it in (dq0schol's rsmeta.c), naming no HarnMaster skill of its own; the library's reader not linked (its room).
*/

#include <stdlib.h>
#include <dice.h>
#include <string.h>
#include <harn.h>
#include "../../dq0/rsmeta.h"

static harn_char pc;
static const unsigned char ml_of[NMK] = { HARN_LAN, HARN_SCR, HARN_FOL, HARN_MAT, HARN_AST, HARN_HRL };

/* Key k's place among keys (" a b c "): 0xFF none */
static unsigned char __fastcall__ key_at (const char* keys, const char* k)
{
    unsigned char n = strlen (k), i = 0;
    const char* p = keys + 1;

    while (*p) {
        if (!strncmp (p, k, n) && p[n] == ' ') {
            return i;
        }
        p = strchr (p, ' ') + 1;
        ++i;
    }
    return 0xFF;
}

void __fastcall__ meta_line (char* l, void __fastcall__ (*own) (char* l))
{
    static const char attrs[] = " sp sx bd ss ht fr owt wt cml str sta dex agl eye hrg sml voi int aur wil mor dty pty end ";
    static const char mls[] = " lan scr fol mat ast hrl ";
    unsigned char a = !strncmp (l, "att ", 4), k;
    char* e;

    if (!a && strncmp (l, "ml ", 3)) {
        own (l);
        return;
    }
    l = strchr (l, ' ') + 1;
    if (!(e = strchr (l, ' '))) {
        return;
    }
    *e++ = 0;
    if ((k = key_at (a ? attrs : mls, l)) == 0xFF) {
        return;
    }
    if (a) {
        pc.att[k] = atoi (e);
    } else {
        pc.ml[ml_of[k]] = atoi (e);
    }
}

unsigned char __fastcall__ meta_ml (unsigned char mk)
{
    return pc.ml[ml_of[mk]];
}

unsigned char __fastcall__ meta_taught (unsigned char mk)
{
    return mk == MK_SCRIPT ? 70 + harn_get_sb (&pc, HARN_SCR) : harn_get_sb (&pc, HARN_LAN);
}

/* A hired reader's skill: a scribe's Script EML 45 + 4d10 (a sage of a dead tongue's 50 + 4d10: the house's figures in the
** book's terms) */
unsigned char __fastcall__ meta_hired (unsigned char sage)
{
    return (sage ? 50 : 45) + dice (4, 10);
}

/* A hired reader's honesty: its Morality (the book's attribute, 3d6), by Character 9's bands: diabolical (4 or less) lies,
** unscrupulous (7 or less) asks half again, corruptible (10 or less) sells a copy on */
unsigned char __fastcall__ meta_honest (unsigned char set)
{
    unsigned char m = set != 0xFF ? set : dice (3, 6);

    return m <= 4 ? MH_LIES : m <= 7 ? MH_DEAR : m <= 10 ? MH_SELLS : MH_HONEST;
}
