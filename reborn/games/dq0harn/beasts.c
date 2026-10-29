/*
** beasts.c - (overlay 5's read-only data, the referee's) the bestiary: each kind's sheet to be, by its key (the
** denizens' kinds', kinds.txt's): the first six a stand-in of the game's own (their numbers the game's); the rest the
** HarnWorld Bestiary's averages for the like creature (each entry's page given), or for folk HarnMaster's for men
** (about 10 to 12, their skills by their trade), the peoples' by the book's (2026-10-29: Phase 4's peoples, the
** user's answer 11).  Moved out of overlay 2 for the peoples' room (fight.c reads an entry with bread, its copy in the
** main RAM: dq0harn.h's struct bkind).  The order is the sheets' (a sheet keeps its entry's number: new ones only at
** the end).
**
** The peoples' (the book's numbers, in the game's units):
** - The Gargun (HarnMaster 3.5's Gargun 4-8): the five sub-species' attributes, Initiative, Dodge, natural armour
**   (B4 E3 P1 F3: their fur and hide, the better of it and what they wear: not BF_FOLK), their usual weapon and its
**   mastery (a small orc's dagger, a brown orc's mankar, a black orc's mang, a white orc's spear, a red orc's
**   handaxe); their weight the book's (72 to 105 lb), smaller than a man; a black orc sooner berserk than fled (Gargun
**   6's: "more likely to become berserk than flee"); the orc king the house's, a black orc's best.
** - The Khuzdul and the Sindarin: a man's (11) with the book's species modifiers (Character 7-8: a dwarf's Strength and
**   Stamina 4 more, Dexterity 1, Will 3; an elf's Strength and Stamina 1, Dexterity and Agility 2), their weight by
**   their build (a dwarf some 150 lb for 54", an elf some 130 lb), their trade's weapon and mastery the house's (a
**   warrior's 65, a miner's 50, an elder's 70; an elf warden's longknife 65).
** - Men-at-arms and their captain: a man's, as the bandit and its lord are, a soldier's spear, a captain's sword.
*/

#pragma rodata-name ("OV5RODATA")

#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

const struct bkind bestiary[NBEASTS] = {
    { "rat", { 5, 8, 10, 14, 6 }, 3, 2, 30, 50, { 1, 1, 0, 1 }, BF_QUAD,
      { { "bite", 45, 2, HARN_POINT, HARN_FACE }, { "" } } },
    { "spider", { 6, 9, 12, 15, 8 }, 4, 3, 40, 55, { 2, 3, 1, 0 }, 0,
      { { "bite", 50, 3, HARN_POINT, HARN_FACE }, { "" } } },
    { "ghoul", { 13, 14, 11, 11, 12 }, 35, 4, 70, 40, { 1, 1, 1, 0 }, BF_BIPED,
      { { "claws", 55, 4, HARN_EDGE, HARN_HAND }, { "bite", 45, 4, HARN_POINT, HARN_FACE } } },
    { "skel", { 10, 10, 10, 10, 10 }, 10, 4, 90, 35, { 0, 4, 6, 2 }, BF_MINDLESS | BF_BIPED,
      { { "sword", 50, 4, HARN_EDGE, 0xFF }, { "" } } },
    { "bear", { 22, 18, 8, 10, 12 }, 150, 5, 60, 30, { 4, 3, 2, 3 }, BF_QUAD,
      { { "claws", 65, 6, HARN_EDGE, HARN_HAND }, { "bite", 55, 7, HARN_POINT, HARN_FACE } } },
    { "kobold", { 8, 10, 12, 13, 9 }, 15, 3, 40, 55, { 2, 2, 1, 1 }, BF_BIPED | BF_FOLK,
      { { "spear", 45, 5, HARN_POINT, 0xFF }, { "" } } },
    { "bat", { 3, 8, 10, 10, 8 }, 1, 1, 45, 50, { 1, 1, 0, 1 }, 0,                  /* (Bats 1, p10) */
      { { "bite", 45, 1, HARN_POINT, HARN_FACE }, { "" } } },
    { "adder", { 4, 10, 12, 12, 8 }, 1, 2, 45, 60, { 1, 1, 0, 1 }, 0,               /* (Snakes 3, p88) */
      { { "bite", 55, 3, HARN_POINT, HARN_FACE }, { "" } } },
    { "basilsk", { 16, 13, 14, 14, 10 }, 28, 5, 55, 70, { 6, 6, 4, 4 }, 0,         /* (Dragons 13, p36) */
      { { "bite", 65, 5, HARN_POINT, HARN_FACE }, { "" } } },
    { "lindwrm", { 22, 18, 4, 18, 16 }, 200, 6, 85, 90, { 5, 9, 4, 5 }, 0,         /* (Dragons 14, p37) */
      { { "claws", 85, 7, HARN_EDGE, HARN_HAND }, { "bite", 68, 7, HARN_POINT, HARN_FACE } } },
    { "wight", { 12, 12, 6, 10, 1 }, 40, 4, 60, 35, { 1, 1, 1, 0 }, BF_MINDLESS | BF_BIPED,
      { { "claws", 55, 3, HARN_POINT, HARN_HAND }, { "" } } },                      /* (Morvrin 5, p78: a man's,
                                                                                    **   its will gone) */
    { "shade", { 11, 12, 11, 11, 15 }, 38, 4, 70, 50, { 2, 2, 1, 0 }, BF_BIPED,
      { { "sword", 60, 5, HARN_EDGE, 0xFF }, { "" } } },                            /* (Morvrin 3, p76: a priest's) */
    { "koblord", { 11, 12, 12, 13, 12 }, 20, 3, 60, 55, { 3, 3, 2, 1 }, BF_BIPED | BF_FOLK,
      { { "axe", 55, 6, HARN_EDGE, 0xFF }, { "" } } },
    { "bandit", { 11, 11, 11, 11, 11 }, 40, 4, 50, 45, { 2, 2, 1, 1 }, BF_BIPED | BF_FOLK,
      { { "sword", 50, 5, HARN_EDGE, 0xFF }, { "" } } },
    { "banlord", { 14, 13, 12, 12, 13 }, 45, 4, 65, 55, { 4, 5, 3, 2 }, BF_BIPED | BF_FOLK,
      { { "sword", 65, 6, HARN_EDGE, 0xFF }, { "" } } },
    { "cultist", { 10, 10, 11, 10, 12 }, 38, 4, 55, 40, { 1, 1, 0, 0 }, BF_BIPED | BF_FOLK,
      { { "dagger", 45, 3, HARN_POINT, 0xFF }, { "" } } },
    { "priest", { 10, 11, 12, 11, 16 }, 38, 4, 70, 45, { 2, 2, 1, 1 }, BF_BIPED | BF_FOLK,
      { { "mace", 55, 5, HARN_BLUNT, 0xFF }, { "" } } },
    /* (Phase 4's peoples, 2026-10-29) */
    { "arak", { 10, 10, 11, 11, 10 }, 18, 3, 50, 55, { 4, 3, 1, 3 }, BF_BIPED,     /* (Gargun 4: the small orc) */
      { { "dagger", 60, 5, HARN_POINT, 0xFF }, { "" } } },
    { "hyeka", { 11, 11, 11, 11, 11 }, 24, 3, 55, 55, { 4, 3, 1, 3 }, BF_BIPED,    /* (Gargun 5: the brown orc) */
      { { "mankar", 55, 5, HARN_EDGE, 0xFF }, { "" } } },
    { "khanu", { 13, 12, 11, 11, 12 }, 26, 3, 72, 55, { 4, 3, 1, 3 }, BF_BIPED | BF_BERSERK,  /* (Gargun 6: the black
                                                                                    **   orc) */
      { { "mang", 60, 6, HARN_EDGE, 0xFF }, { "" } } },
    { "kyani", { 11, 11, 11, 11, 11 }, 24, 3, 55, 55, { 4, 3, 1, 3 }, BF_BIPED,    /* (Gargun 7: the white orc) */
      { { "spear", 55, 7, HARN_POINT, 0xFF }, { "" } } },
    { "viasal", { 12, 11, 11, 11, 11 }, 25, 3, 66, 55, { 4, 3, 1, 3 }, BF_BIPED,   /* (Gargun 8: the red orc) */
      { { "axe", 60, 6, HARN_EDGE, 0xFF }, { "" } } },
    { "garking", { 15, 14, 12, 12, 14 }, 28, 3, 80, 60, { 4, 3, 1, 3 }, BF_BIPED | BF_BERSERK,   /* (The house's: a
                                                                                    **   tribe's king, its strongest) */
      { { "mang", 70, 6, HARN_EDGE, 0xFF }, { "" } } },
    { "khuzwar", { 15, 15, 12, 11, 14 }, 38, 4, 65, 50, { 2, 2, 1, 1 }, BF_BIPED | BF_FOLK,   /* (A Khuzan warrior) */
      { { "axe", 65, 6, HARN_EDGE, 0xFF }, { "" } } },
    { "khuzmin", { 15, 15, 12, 11, 13 }, 37, 4, 50, 45, { 2, 2, 1, 1 }, BF_BIPED | BF_FOLK,   /* (A miner) */
      { { "pick", 50, 5, HARN_POINT, 0xFF }, { "" } } },
    { "khuzeld", { 14, 14, 12, 11, 16 }, 36, 4, 75, 50, { 3, 3, 2, 1 }, BF_BIPED | BF_FOLK,   /* (An elder) */
      { { "hammer", 70, 6, HARN_BLUNT, 0xFF }, { "" } } },
    { "sindwar", { 12, 12, 13, 13, 12 }, 32, 4, 65, 65, { 2, 2, 1, 1 }, BF_BIPED | BF_FOLK,   /* (A Sindarin warden) */
      { { "blade", 65, 3, HARN_EDGE, 0xFF }, { "" } } },
    { "manarms", { 12, 12, 11, 11, 11 }, 40, 4, 55, 50, { 2, 2, 1, 1 }, BF_BIPED | BF_FOLK,   /* (A man-at-arms) */
      { { "spear", 55, 7, HARN_POINT, 0xFF }, { "" } } },
    { "captain", { 14, 13, 12, 12, 13 }, 44, 4, 70, 55, { 4, 5, 3, 2 }, BF_BIPED | BF_FOLK,   /* (Its captain) */
      { { "sword", 65, 5, HARN_EDGE, 0xFF }, { "" } } },
};
