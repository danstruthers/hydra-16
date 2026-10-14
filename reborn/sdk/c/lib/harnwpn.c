/*
** harnwpn.c - harn.h's weapons (Combat 3).
*/

#include <harn.h>

/* skill, weight (tenths of a pound), quality, attack and defence classes, one-handed
** penalty, blunt, edge and point impact (-1: none; a point the book has in brackets, an option, is none here),
** price.  (The morningstar's impacts are the book's as printed: B0, P5) */

#define N       (-1)
#define W(name, sk, wt, wq, wac, wdc, hm, b, e, p, cost)   { name, HARN_ ## sk, wt, wq, wac, wdc, hm, { b, e, p }, cost }

const harn_weapon harn_weapons[] = {
    W ("hand",          UNA, 0, 0, 0, 15, 0, 0, N, N, 0),
    W ("foot",          UNA, 0, 0, 5, 5, 0, 1, N, N, 0),
    W ("head",          UNA, 0, 0, 0, 0, 0, 1, N, N, 0),
    W ("buckler",       SHD, 30, 12, 5, 15, 0, 1, N, N, 24),
    W ("knight shield", SHD, 50, 13, 5, 20, 0, 2, N, N, 60),
    W ("round shield",  SHD, 60, 13, 5, 20, 0, 2, N, N, 42),
    W ("kite shield",   SHD, 70, 14, 5, 25, 0, 3, N, N, 72),
    W ("tower shield",  SHD, 80, 14, 5, 25, 0, 3, N, N, 96),
    W ("knife",         DAG, 10, 10, 5, 0, 0, 0, 1, 4, 6),
    W ("dagger",        DAG, 10, 11, 5, 5, 0, 1, 2, 5, 24),
    W ("taburi",        DAG, 10, 10, 5, 0, 0, 0, N, 4, 20),
    W ("keltan",        DAG, 20, 12, 5, 10, 0, 2, 0, 3, 36),
    W ("longknife",     SWD, 10, 12, 10, 15, 0, 1, 3, 5, 96),
    W ("shortsword",    SWD, 20, 12, 10, 5, 0, 2, 4, 4, 90),
    W ("mankar",        SWD, 20, 11, 10, 5, 0, 2, 5, 0, 84),
    W ("mang",          SWD, 30, 11, 15, 10, 5, 3, 6, 0, 110),
    W ("broadsword",    SWD, 30, 12, 15, 10, 0, 3, 5, 3, 150),
    W ("estoc",         SWD, 30, 11, 15, 10, 0, 3, 0, 6, 150),
    W ("falchion",      SWD, 40, 12, 15, 5, 0, 4, 6, 1, 120),
    W ("bastard sword", SWD, 50, 12, 20, 10, 10, 4, 7, 4, 180),
    W ("battlesword",   SWD, 80, 13, 25, 10, 20, 5, 8, 4, 230),
    W ("stick",         CLU, 20, 9, 5, 5, 0, 2, N, N, 0),
    W ("club",          CLU, 30, 9, 15, 5, 0, 4, N, N, 12),
    W ("mace",          CLU, 40, 11, 15, 5, 0, 6, N, N, 84),
    W ("morningstar",   CLU, 50, 11, 20, 5, 10, 0, N, 5, 48),
    W ("maul",          CLU, 70, 9, 20, 5, 20, 7, N, N, 24),
    W ("sickle",        AXE, 10, 9, 5, 5, 0, 1, 4, 3, 10),
    W ("shorkana",      AXE, 20, 10, 5, 5, 0, 3, 5, N, 48),
    W ("hatchet",       AXE, 20, 9, 5, 5, 0, 3, 4, N, 12),
    W ("handaxe",       AXE, 30, 11, 10, 5, 0, 4, 6, N, 72),
    W ("warhammer",     AXE, 50, 11, 15, 5, 5, 6, N, N, 90),
    W ("battleaxe",     AXE, 60, 12, 20, 10, 15, 6, 9, N, 100),
    W ("nachakas",      FLA, 10, 10, 15, 10, 0, 4, N, N, 12),
    W ("grainflail",    FLA, 20, 9, 20, 5, 0, 5, N, N, 12),
    W ("ball & chain",  FLA, 40, 12, 20, 10, 0, 8, N, N, 60),
    W ("warflail",      FLA, 50, 11, 25, 10, 20, 9, N, N, 60),
    W ("staff",         SPR, 40, 11, 20, 15, 10, 4, N, N, 36),
    W ("javelin",       SPR, 30, 10, 15, 5, 10, 2, N, 6, 48),
    W ("spear",         SPR, 50, 11, 20, 10, 10, 4, N, 7, 60),
    W ("trident",       SPR, 60, 12, 20, 15, 10, 4, N, 5, 72),
    W ("lance",         POL, 80, 11, 25, 5, 15, 4, N, 8, 120),
    W ("glaive",        POL, 80, 11, 25, 10, 20, 6, 7, 6, 84),
    W ("jousting pole", POL, 80, 8, 25, 5, 25, 3, N, N, 40),
    W ("poleaxe",       POL, 80, 11, 25, 5, 15, 6, 9, 6, 96),
    W ("pike",          POL, 120, 12, 25, 5, 25, 4, N, 8, 96),
    W ("net",           NET, 40, 9, 20, 0, 0, 2, N, N, 48),
    W ("whip",          WHP, 20, 9, 25, 5, 0, 2, 1, N, 12),
    W ("isagra",        WHP, 40, 11, 25, 5, 0, 3, 3, N, 20),
    W ("crossbow",      BOW, 50, 10, 5, 5, 0, 3, N, N, 60),
    W ("shortbow",      BOW, 20, 10, 5, 5, 0, 1, N, N, 24),
    W ("longbow",       BOW, 30, 11, 5, 5, 0, 2, N, N, 36),
    W ("hartbow",       BOW, 20, 13, 5, 5, 0, 1, N, N, 96),
    W ("blowgun",       BLO, 10, 8, 5, 5, 0, 1, N, N, 12),
    W ("sling",         SLG, 1, 9, 0, 0, 0, N, N, N, 6),
    W ("staff sling",   SLG, 10, 10, 5, 0, 10, 1, N, N, 12),
};
const unsigned char harn_nweapons = sizeof harn_weapons / sizeof harn_weapons[0];
