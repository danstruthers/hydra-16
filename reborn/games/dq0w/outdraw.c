/*
** outdraw.c - (dq0w, swapped part 59: ../dq0/swap.inc, bank.s's stubs) an outfit's section drawn (outfit.h's: part
** 57's, the order the outfit's bought in): the section read from the outfits' file (outfits.js's layout), each line by
** its chance and its tags (if, not), its choices drawn by their weights (a tag's weight for a character with it) among
** those it can pay for (their things at a middling make: a free line's all), the one drawn bought (each thing's price
** the appraiser's of its make: its value times 2 plus its quality, a sixth; its own make half) and put in the list, or
** in the place of a thing drawn already (instead: a knife made a dagger, the price between them paid); a skill's tool
** it has already not drawn again.  Its own data in its image (it calls no other part), the draw's state the main
** RAM's (ost).
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "outfit.h"

#pragma code-name ("SW59CODE")
#pragma rodata-name ("SW59RODATA")
#pragma bss-name ("SW59RODATA")                 /* (Its own data: zeros each time it's copied in) */
#pragma local-strings (on)
#pragma static-locals (on)

#define LI_FREE         0x10                    /* (A line's info: given, */
#define LI_NEED         0x20                    /*   bought first, */
#define LI_IF           0x40                    /*   an if mask after it, */
#define LI_NOT          0x80                    /*   a not mask) */
#define CI_INSTEAD      0x40                    /* (A choice's info: the kind it takes the place of after its weights) */
#define SEC_MAX         255                     /* (A section's bytes at most: outfits.js's) */

static unsigned char sec[SEC_MAX];              /* The section, read in */
static unsigned char smake, smdie;              /* Its make */
static struct kind kk;                          /* A kind's record, copied out */
static struct ofit e;                           /* A list's entry, copied out or in */

/* The draw's die: 0 ... n - 1 */
static unsigned char __fastcall__ die (unsigned char n)
{
    ost.rs = ost.rs * 25173U + 13849U;
    return n ? (ost.rs >> 8) % n : 0;
}

/* Kind k's record copied out (kk) */
static void __fastcall__ kind_get (unsigned char k)
{
    hub_map (KIND_BANK);
    memcpy (&kk, CATALOGUE + (k < NKINDS ? k : 0), sizeof kk);
}

/* The first drawn of kind k: its entry (from 1), or 0 */
static unsigned char __fastcall__ drawn (unsigned char k)
{
    unsigned char j, n;

    hub_map (KIND_BANK);
    for (n = OF_N, j = 0; j < n; ++j) {
        if (OF_LIST[j].kind == k) {
            return j + 1;
        }
    }
    return 0;
}

/* A thing's make: the section's (its make and die), else the wealth's (a poor family's 2 + 1d3, an average one's
** 3 + 1d3, a rich one's 4 + 1d3); its own make a point less, a fine one a point more */
static unsigned char __fastcall__ quality (unsigned char tf)
{
    signed char g = smake != 0xFF ? smake + (smdie ? 1 + die (smdie) : 0) : 3 + ost.level + die (3);

    g += tf & TF_FINE ? 1 : tf & TF_OWN ? -1 : 0;
    return g < 0 ? 0 : g > 9 ? 9 : g;
}

/* What a thing of make g costs (kk its kind's record, q of a lot; its own make half), paid (back: taken back) if it
** isn't given */
static void __fastcall__ pay (unsigned char g, unsigned char q, unsigned char tf, unsigned char back)
{
    unsigned v = (unsigned) ((unsigned long) kk.value * (2 + g) / 6) * (kk.form & KF_COUNT ? q : 1);

    if (tf & TF_OWN) {
        v /= 2;
    }
    if (ost.freeall) {
        return;
    }
    ost.budget = back ? ost.budget + v : v > ost.budget ? 0 : ost.budget - v;
}

/* Treasure 5's jewelry by fives of the d100: its item, its material */
static const unsigned char jkind[20] = { KI_GEM, KI_GEM, KI_GEM, KI_RING, KI_RING, KI_RING, KI_RING, KI_RING,
    KI_PENDANT, KI_PENDANT, KI_PENDANT, KI_NECKLACE, KI_NECKLACE, KI_NECKLACE, KI_NECKLACE, KI_BROOCH, KI_BROOCH,
    KI_BROOCH, KI_BROOCH, KI_BROOCH };
static const unsigned char jmat[20] = { MT_BONE, MT_BONE, MT_BONE, MT_BRONZE, MT_BRONZE, MT_GLASS, MT_SILVER,
    MT_SILVER, MT_SILVER, MT_SILVER, MT_SILVER, MT_SILVER, MT_SILVER, MT_SILVER, MT_SILVER, MT_STONE, MT_GOLD, MT_GOLD,
    MT_GOLD, MT_GOLD };

/* A choice's things (n of them at p): drawn into the list, paid for; a thing in another (PL_IN) in the last that went
** elsewhere (the choice's container: a chest's pennies, its crowns, its goblet, all in it, 2026-10-29; it was in the
** thing just before it, the pennies) */
static void __fastcall__ things (const unsigned char* p, unsigned char n)
{
    static const unsigned char lost[20] = { 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 6, 5, 5, 4, 3, 2, 1, 0 };
    unsigned char k, q, j, many, tf, r, g, prev = 0, i = 0;

    for (; n--; p += 5, prev = (p[-4] & 15) != PL_IN ? i : prev) {
        k = p[0];
        tf = p[1] >> 4;
        if (p[2] & 0x80) {                      /* (The book's dice: 7d6 pennies, 2026-10-28) */
            for (q = 0, j = p[2] & 0x7F; j--; ) {
                q += 1 + die (p[3]);
            }
        } else {
            q = p[2] + (p[3] > p[2] ? die (p[3] - p[2] + 1) : 0);
        }
        kind_get (k);
        many = 1;
        if (!(kk.form & KF_COUNT)) {
            if (!(kk.flags & K_PLURAL) && q > 1) {
                many = q;                       /* (Several things, each its own: two shorkana) */
            }
            q = 1;
        } else if (!q) {
            q = 1;
        }
        for (j = 0; j < many; ++j) {
            e.kind = k;
            e.qty = q;
            e.quality = quality (tf);
            e.place = p[1] & 15;
            e.in = e.place == PL_IN ? prev : 0;
            e.flags = tf;
            if ((e.mat = p[4]) == MT_JEWEL) {   /* (A jewel: Treasure 5's item, narrowed to the catalogue's (a
                                                **   bracelet or an anklet a necklace, a comb or earrings a brooch),
                                                **   and its material (ivory with bone, amber with silver, jade
                                                **   stone, mithril gold: the house's), a gem in the pouch) */
                e.kind = jkind[die (100) / 5];  /* (Its tables by fives: 2026-10-29, the part's room) */
                r = jmat[die (100) / 5];
                e.mat = e.kind == KI_GEM ? 0 : r;
                if (!e.mat) {
                    e.place = PL_POUCH;
                }
            }
            kind_get (e.kind);
            r = (unsigned char) ((ost.rs >> 3) + i * 37 + j * 11) % 100;
            if (sec[0] & SF_FOUND) {            /* (A lair's leavings, lost or left: its condition by Treasure 1's
                                                **   Found column, silver and gold 30 the better on the roll (as the
                                                **   maker's found things are), an old one's (SF_OLD) 5 the worse a
                                                **   century of its stratum's age, the house's (2026-10-29); its
                                                **   life's tenths lost: broken 9 (a tenth left, as the Person
                                                **   column's), poor 7-8, fair 4-6, good 1-3, as new none) */
                g = e.mat ? e.mat : kk.mat;
                r += g == MT_SILVER || g == MT_GOLD ? 30 : 0;
                if (sec[0] & SF_OLD) {
                    r = r > ost.age ? r - ost.age : 0;
                }
                e.flags |= lost[r < 100 ? r / 5 : 19] << 4;
            } else if (sec[0] & SF_FREE) {      /* (A denizen's: its condition by Treasure 1's table, its Person
                                                **   column, its life's tenths lost in its flags' high four bits,
                                                **   part 58's: broken 1, poor 2-3, fair 4-6, good 7-9, as new 10; by
                                                **   the draw's state and the list's place, no die spent: the draws
                                                **   after it as they were) */
                e.flags |= (r < 1 ? 9 : r < 3 ? 7 + (r & 1) : r < 10 ? 4 + r % 3 : r < 90 ? 1 + r % 3 : 0) << 4;
            }
            if ((sec[0] & SF_OLD) && (kk.form & KF_SMALL) &&
                (ost.omint == 0xFF ? (ost.omint = old_mint ()) : ost.omint) != 0) {
                e.mat = 0x80 | ost.omint;       /* (Old coins, grave goods': the mint of the people of its stratum's
                                                **   oldest era, whole: part 57's, part 58's) */
            }
            kind_get (e.kind);
            pay (e.quality, q, tf, 0);
            hub_map (KIND_BANK);
            if ((i = OF_N) < NOFIT) {
                memcpy (OF_LIST + i, &e, sizeof e);
                OF_N = ++i;
            }
        }
    }
}

/* A choice at c: its weight, by the tags (0: it can't pay for it, or it has the tool already), its things' start (cw
** on from c), how many (cn), and what it takes the place of (ck) */
static unsigned char cw, cn, ck;
static unsigned char __fastcall__ weight (const unsigned char* c)
{
    unsigned char w = c[0], ci = c[1], k, n = ci >> 4 & 3, g;
    unsigned v = 0;
    const unsigned char* p;

    for (k = 0; k < n; ++k) {
        if (ost.tags & 1u << c[2 + 2 * k]) {
            w = c[3 + 2 * k];
            break;
        }
    }
    cw = 2 + 2 * n;
    ck = ci & CI_INSTEAD ? c[cw++] : 0;
    cn = ci & 15;
    g = smake != 0xFF ? smake + smdie : 5 + ost.level;  /* (Its price at the best make it may be: what it must have,
                                                **   strictly within its money, DECISIONS.md's answer 1) */
    for (p = c + cw, k = ck ? cn - 1 : 0, p += 5 * k; k < cn; ++k, p += 5) {   /* (In another's place: its last
                                                **   thing's, less the other's worth) */
        kind_get (p[0]);
        v += (unsigned) ((unsigned long) kk.value * (2 + g + ((p[1] >> 4) & TF_FINE ? 1 : 0)) / 6) *
            ((kk.form & KF_COUNT) && p[3] ? p[2] & 0x80 ? (p[2] & 0x7F) * p[3] : p[3] : 1);
    }
    if (ck) {
        kind_get (ck);
        v = v > kk.value ? v - kk.value : 0;
    }
    if (!ost.freeall && v > ost.budget) {
        return 0;
    }
    kind_get (c[cw]);
    return ost.tool && !ck && !(kk.form & KF_COUNT) && drawn (c[cw]) ? 0 : w;
}

/* A line (at p: its length first) drawn as mode says (MD_*) */
static void __fastcall__ line (const unsigned char* p, unsigned char mode)
{
    const unsigned char* c;
    const unsigned char* first;
    unsigned total, r;
    unsigned char info = p[2], n, j, w, f, v;

    if (mode == MD_NEED && !(info & LI_NEED) || mode == MD_CHANCE && (info & LI_NEED)) {
        return;
    }
    c = p + 3;
    if (info & LI_IF) {
        if (!(ost.tags & (c[0] | c[1] << 8))) {
            return;
        }
        c += 2;
    }
    if (info & LI_NOT) {
        if (ost.tags & (c[0] | c[1] << 8)) {
            return;
        }
        c += 2;
    }
    if (p[1] < 100 && die (100) >= p[1]) {
        return;
    }
    f = ost.freeall;
    ost.freeall |= (info & LI_FREE) || (sec[0] & SF_FREE);
    n = info & 15;
    for (total = 0, first = c, j = 0; j < n; ++j) {     /* (Its choices: each one's weight, of those it can pay for) */
        total += weight (c);
        c += cw + 5 * cn;
    }
    if (total) {
        r = die (total > 255 ? 255 : total);
        for (c = first, j = 0; j < n; ++j) {    /* (The one drawn) */
            w = weight (c);
            if (r < w) {
                break;
            }
            r -= w;
            c += cw + 5 * cn;
        }
        if (j < n && !ck) {
            things (c + cw, cn);
        } else if (j < n && (v = drawn (ck)) != 0) {    /* (In the place of a thing drawn: its last thing's kind) */
            c += cw + 5 * (cn - 1);
            hub_map (KIND_BANK);
            memcpy (&e, OF_LIST + v - 1, sizeof e);
            kind_get (e.kind);
            pay (e.quality, e.qty, e.flags, 1); /* (The old thing's price back) */
            e.kind = c[0];
            e.flags = c[1] >> 4;
            e.mat = c[4];
            e.quality = quality (e.flags);
            kind_get (c[0]);
            pay (e.quality, e.qty, e.flags, 0);
            hub_map (KIND_BANK);
            memcpy (OF_LIST + v - 1, &e, sizeof e);
        }
    }
    ost.freeall = f;
}

/* The section at off read, its head kept, drawn as mode says if its flags are those wanted: 1, or 0 */
unsigned char __fastcall__ s_draw_at (unsigned off, unsigned char mode)
{
    unsigned char k, n;
    const unsigned char* p = sec + 8;

    hub_map (0);                                /* (Its bytes alone, the index's length: its file read the less,
                                                **   2026-10-27) */
    if (!off || lseek (ost.fd, off, SEEK_SET) != (long) off || read (ost.fd, sec, ost.slen ? ost.slen : SEC_MAX) <= 8) {
        return 0;
    }
    ost.sflags = sec[0];
    ost.bent = sec[1];
    ost.keep = sec[2];
    smake = sec[3];
    smdie = sec[4];
    ost.pn = sec[5];
    ost.pd = sec[6];
    if (ost.want != 0xFF && (sec[0] & (SF_WEAPON | SF_SHIELD)) != ost.want) {
        return 0;
    }
    if (mode != MD_HEAD) {
        for (n = sec[7], k = 0; k < n && p < sec + SEC_MAX && *p; ++k, p += *p) {
            line (p, mode);
        }
    }
    return 1;
}
