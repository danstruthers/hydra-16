/*
** rock.h - Dingus Quest Zero's rock (DUNGEON.md's rock and old places, slice 4.10): the kinds of rock the dungeon is
** cut in, and where each lies, from the game's seed alone, so every task that wants it (the world's digging, the
** tunnellers' to come) works out the same rock at the same place and depth, and nothing of it is kept.
**
** The geology: beds of sedimentary rock (shale, sandstone, limestone), 25 feet thick, lying one on another down
** through every stratum, dipping gently one way across the frame (a few feet a region) and rising and falling a
** little region by region, so a stratum's rock changes as it's crossed and as it's dug down through; below 200 feet
** the shale pressed to slate and the limestone to marble.  Through them, a region's own (a region of the frame, 256
** squares each way): a granite boss in one region in six (round, 15 to 78 squares in radius, the beds' rock round
** it); in seven regions in 16 a line straight across it, its slope its own (its x by its y: near north and south): a
** vein a square wide (quartz three times in five, else a seam of ore, ironstone or copper) or a basalt dyke three
** squares wide.
**
** Each kind's hardness (RH_*: the dig's rate on it, the house's: HOUSE.md's digging) and whether it's poor (rotten
** rock: a roof that may come down on a dig, HOUSE.md's).  Its words the UI's (dq0/main.c's s_dig_used).
**
** A task defines ROCK_CODE before it includes this, where it wants the code (one file of it).
*/

#ifndef _ROCK_H
#define _ROCK_H

enum { RK_NONE, RK_SHALE, RK_SANDSTONE, RK_LIMESTONE, RK_SLATE, RK_MARBLE, RK_GRANITE, RK_BASALT, RK_QUARTZ,
    RK_IRONSTONE, RK_COPPER, NRK };
enum { RH_SOFT, RH_MIDDLING, RH_HARD, RH_VERY_HARD };
#define RK_HARDNESS     { RH_MIDDLING, RH_SOFT, RH_MIDDLING, RH_MIDDLING, RH_HARD, RH_MIDDLING, RH_HARD, RH_HARD, \
                          RH_VERY_HARD, RH_MIDDLING, RH_MIDDLING }
#define RK_QUARTERS     { 6, 4, 2, 1 }          /* (The rate in it, by its hardness: quarters of the tool's in
                                                **   stone, the world's, every ruleset's) */
#define RK_POOR(k)      ((k) == RK_SHALE)       /* (Rotten rock: it splits and falls) */
#define RK_VEIN(k)      ((k) >= RK_QUARTZ)      /* (A vein's, or a seam of ore's) */

/* The rock at square x, y of the frame, depth feet below the way in (the top stratum's datum less the floor's), from
** the game's seed (16 bits of it): RK_* */
unsigned char __fastcall__ rock_kind (unsigned seed, int depth, unsigned x, unsigned y);

#if defined (ROCK_CODE)

/* (16 bits throughout, the 6502's: no long arithmetic.  Every static given a value, so a task can put them in a
** part: its DATA a segment of the part's, used only within a call) */
static unsigned rk_h = 0, rk_seed = 0;

/* The dice: the next of them, and a region's first (what, which region) */
static unsigned char rk_next (void)
{
    rk_h ^= rk_h << 4;                          /* (Shifts the tasks link already: no runtime added to
                                                **   their main RAM) */
    rk_h ^= rk_h >> 3;
    rk_h ^= rk_h << 8;
    return (unsigned char) (rk_h ^ rk_h >> 8);
}

static unsigned char __fastcall__ rk_mix (unsigned char what, unsigned rx, unsigned ry)
{
    rk_h = (rk_seed ^ rx * 31 ^ ry * 977 ^ what * 7919) | 1;
    rk_next ();
    rk_next ();
    return rk_next ();
}

unsigned char __fastcall__ rock_kind (unsigned seed, int depth, unsigned x, unsigned y)
{
    static int d = 0, e = 0;
    static unsigned char k = 0, w = 0, rx = 0, ry = 0;

    rk_seed = seed;
    rx = x >> 8;
    ry = y >> 8;
    x &= 0xFF;
    y &= 0xFF;
    k = rk_mix (1, rx, ry) & 15;                /* (A vein across the region, five in 16; a dyke, two) */
    if (k < 7) {
        w = (rk_next () & 7) - 4;               /* (Its line: through a square of the region, its way w, 1 */
        d = (int) x - rk_next ();               /*   across; its width, times the longer, under e) */
        e = (int) y - rk_next ();
        d = (signed char) w * e - d;
        e = (k < 5 ? 1 : 3) * ((signed char) w < 0 ? 1 - (signed char) w : (signed char) w + 1);
        if (d < e && d > -e) {
            return k < 5 ? (k < 3 ? RK_QUARTZ : k < 4 ? RK_IRONSTONE : RK_COPPER) : RK_BASALT;
        }
    }
    if (rk_mix (3, rx, ry) % 6 == 0) {          /* (A boss in the region: its middle, its radius 15 to 78 squares) */
        d = (int) x - rk_next ();
        e = (int) y - rk_next ();
        k = 15 + (rk_next () & 0x3F);
        if (d > -80 && d < 80 && e > -80 && e < 80 && (unsigned) (d * d + e * e) < (unsigned) k * k) {
            return RK_GRANITE;
        }
    }
    /* The beds: the dip (feet a region each way, -16 to 15), the region's swell (0 to 7 feet), 25 feet each */
    rk_mix (4, 0, 0);
    d = (signed char) ((rk_next () & 0x1F) - 16);
    e = (signed char) ((rk_next () & 0x1F) - 16);
    d = depth + (((int) rx << 8 | x) >> 5) * d / 8 + (((int) ry << 8 | y) >> 5) * e / 8 + (rk_mix (5, rx, ry) & 7);
    k = rk_mix (6, (unsigned) (d + 30000) / 25, 0) % 10;   /* (The bed's kind: shale 4, sandstone 3, limestone 3
                                                            **   in 10) */
    k = k < 4 ? RK_SHALE : k < 7 ? RK_SANDSTONE : RK_LIMESTONE;
    if (depth > 200) {
        k = k == RK_SHALE ? RK_SLATE : k == RK_LIMESTONE ? RK_MARBLE : k;
    }
    return k;
}

#endif
#endif
