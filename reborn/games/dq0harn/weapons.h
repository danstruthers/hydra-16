/*
** weapons.h - (dq0harn, and dq0acct's what's to practise with) the referee's table of the weapons.  Included by each, as course.h and watch.h
** are.
*/

/* The weapons, by the tools' keys: a word, its skill, its attack and defence classes, its blunt, edge and point
** impacts (-1: none), the part it is (0xFF: a thing held; TOOL: a tool, the one that hits a thing hardest offered for
** it, its hands empty: a weapon a thing would spoil), its penalty one-handed (the book's HM, a two-handed thing's
** always in both: the things'); the unarmed first (SK_PUNCH, SK_KICK, SK_HEAD's: the hand, the
** foot, the head); the book's stick, club and staff (Combat 3: their numbers, a point's optional aspect left out) what
** a thing swung is treated as (a theme's own kinds by these keys too); the shield last */
#define TOOL            0xFE
static const struct weapon {
    char            key[8];
    char            word[14];                   /* (A catalogue's name: the world finds a weapon let fall by it) */
    unsigned char   sk, wac, wdc;
    signed char     imp[3];
    unsigned char   part;
    unsigned char   hm;
} weapons[] = {
    { "hands", "fists", HARN_UNA, 0, 15, { 0, -1, -1 }, HARN_HAND },
    { "", "foot", HARN_UNA, 5, 5, { 1, -1, -1 }, HARN_FOOT },
    { "", "head", HARN_UNA, 0, 0, { 1, -1, -1 }, HARN_SKULL },
    { "dagger", "dagger", HARN_DAG, 5, 5, { 1, 2, 5 }, 0xFF },
    { "knife", "knife", HARN_DAG, 5, 0, { 0, 1, 4 }, 0xFF },
    { "taburi", "taburi", HARN_DAG, 5, 0, { 0, -1, 4 }, 0xFF },
    { "blade", "sword", HARN_SWD, 15, 10, { 3, 5, 3 }, 0xFF },
    { "shortsw", "shortsword", HARN_SWD, 10, 5, { 2, 4, 4 }, 0xFF },
    { "falchin", "falchion", HARN_SWD, 15, 5, { 4, 6, 1 }, 0xFF },
    { "longknf", "longknife", HARN_SWD, 10, 15, { 1, 3, 5 }, 0xFF },
    { "mankar", "mankar", HARN_SWD, 10, 5, { 2, 5, 0 }, 0xFF },     /* (The Gargun's, Combat 3's (G): 2026-10-29) */
    { "mang", "mang", HARN_SWD, 15, 10, { 3, 6, 0 }, 0xFF, 5 },
    { "bastard", "bastard sword", HARN_SWD, 20, 10, { 4, 7, 4 }, 0xFF, 10 },
    { "hatchet", "hatchet", HARN_AXE, 5, 5, { 3, 4, -1 }, 0xFF },
    { "sickle", "sickle", HARN_AXE, 5, 5, { 1, 4, 3 }, 0xFF },
    { "shorkan", "shorkana", HARN_AXE, 5, 5, { 3, 5, -1 }, 0xFF },
    { "battlax", "battleaxe", HARN_AXE, 20, 10, { 6, 9, -1 }, 0xFF, 15 },
    { "warhamm", "warhammer", HARN_AXE, 15, 5, { 6, -1, 5 }, 0xFF, 5 },
    { "poleaxe", "poleaxe", HARN_POL, 25, 5, { 6, 9, 6 }, 0xFF },
    { "net", "net", HARN_NET, 20, 0, { 2, -1, -1 }, 0xFF },
    { "whip", "whip", HARN_WHP, 25, 5, { 2, 1, -1 }, 0xFF },
    { "mace", "mace", HARN_CLU, 15, 5, { 6, -1, -1 }, 0xFF },
    { "spear", "spear", HARN_SPR, 20, 10, { 4, -1, 7 }, 0xFF, 10 },
    { "axe", "axe", HARN_AXE, 10, 5, { 4, 6, -1 }, 0xFF },  /* (The book's handaxe: a creature's let fall, slice
                                                **   4.1b's) */
    { "pick", "pick", HARN_AXE, 10, 5, { 2, -1, 5 }, TOOL },
    { "mattock", "mattock", HARN_AXE, 15, 5, { 3, 4, 4 }, TOOL },
    { "shovel", "shovel", HARN_SPR, 10, 10, { 2, 3, -1 }, TOOL },
    { "spade", "spade", HARN_SPR, 10, 10, { 2, 3, -1 }, TOOL },
    { "chisel", "chisel", HARN_DAG, 0, 0, { 1, -1, 2 }, TOOL },
    { "torch", "torch", HARN_CLU, 5, 5, { 2, -1, -1 }, 0xFF },
    { "stick", "stick", HARN_CLU, 5, 5, { 2, -1, -1 }, 0xFF },
    { "club", "club", HARN_CLU, 15, 5, { 4, -1, -1 }, 0xFF },
    { "staff", "staff", HARN_SPR, 20, 15, { 4, -1, -1 }, 0xFF, 10 },
    { "shield", "shield", HARN_SHD, 5, 20, { 2, -1, -1 }, 0xFF },
    { "buckler", "buckler", HARN_SHD, 5, 15, { 1, -1, -1 }, 0xFF },     /* (The shields last: Combat 3) */
    { "kshield", "knight shield", HARN_SHD, 5, 20, { 2, -1, -1 }, 0xFF },
    { "kite", "kite shield", HARN_SHD, 5, 25, { 3, -1, -1 }, 0xFF },
    { "tower", "tower shield", HARN_SHD, 5, 25, { 3, -1, -1 }, 0xFF },
};
