/*
** outfit.c - (dq0w, swapped part 57: ../dq0/swap.inc, bank.s's stubs) the outfits drawn (games/dq0/outfits.txt,
** compiled by games/dq0w/outfits.js into lib/dq0/dq0o.dat; the plan the user reviewed, and DECISIONS.md's "Outfits,
** reach and things used up"): the character's at a new game's start (s_kit's), the referee asked for its makings
** (RS_OUTFIT: its occupation, its open skills by their mastery, its family's wealth, its tags, a seed), its sections
** found and drawn (part 59's draw_at) in the order an outfit's bought (HOUSE.md's outfits): the universal kit; the
** occupation's needs; its first weapon (its highest weapon skill's); a tenth of what's left kept (its keep's share);
** the other weapons, three in all, and a shield; its bent's armour; its skills' tools; its likely and rare things; the
** money (prices and the wealth in farthings; what's left all of it silver pennies, in lots of 250).  The things
** drawn in the list (outfit.h's); part 58 makes them.  Its own data in its image (it calls part 59, which puts it out
** and in again: none that lasts across a call), the draw's state the main RAM's (ost).
*/

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "trace.h"
#include "outfit.h"

#pragma code-name ("SW57CODE")
#pragma rodata-name ("SW57RODATA")
#pragma bss-name ("SW57RODATA")                 /* (Its own data: none of it wanted across a call of part 59's,
                                                **   which puts this part out and in again) */
#pragma local-strings (on)

#define OF_MAGIC        0x464F                  /* (dq0o.dat's: "OF", its version) */
#define OF_VERSION      5                       /* (2: each entry its section's flags, 5 bytes; 3: a thing's
                                                **   material, its fifth byte, the denizens' places; 4: the sections'
                                                **   lengths after the entries, 2026-10-27; 5: a count's dice, a
                                                **   jewel, 2026-10-28) */

/* A section by its name (the hash outfits.js's): its offset in the file, 0 none; its flags (its index entry's) in fflags */
static unsigned char fflags;
static unsigned __fastcall__ find (const char* p)
{
    unsigned h = 0, k, n;
    unsigned char* q;

    for (; *p; ++p) {
        h = h * 31 + (unsigned char) *p;
    }
    hub_map (CAT_BANK);
    n = OF_INDEX[3];
    for (k = 0, q = OF_INDEX + 4; k < n; ++k, q += 5) {
        if ((q[0] | q[1] << 8) == h) {
            fflags = q[4];
            ost.slen = OF_INDEX[4 + 5 * n + k];  /* (Its length, the index's table after its entries: read whole, no
                                                **   more, 2026-10-27) */
            return q[2] | q[3] << 8;
        }
    }
    return 0;
}

/* Line n of the referee's reply (its occupation, then its skills, by their mastery) after pre, in b: 1, or 0 none */
static char b[256];                             /* (256: the index read in the fewer pieces, the start the quicker) */
static unsigned char __fastcall__ reply_line (unsigned char n, const char* pre)
{
    const char* t;
    unsigned char k;

    hub_map (0);
    for (t = HUB->slot[SLOT_UI].text; n && *t; ++t) {
        n -= *t == '\n';
    }
    if (!*t) {
        return 0;
    }
    strcpy (b, pre);
    for (k = strlen (b); *t && *t != '\n' && k < sizeof b - 3; ++k) {
        b[k] = *t++;
    }
    b[k] = 0;
    return 1;
}

/* The skills' sections (the reply's lines from line from on) whose flags are want, drawn, n at most: the line after
** the last drawn */
static unsigned char __fastcall__ skills (unsigned char from, unsigned char want, unsigned char n)
{
    unsigned char k;
    unsigned o;

    for (k = from; n && reply_line (k, "skill "); ++k) {
        ost.want = want;
        if ((o = find (b)) != 0 && (fflags & (SF_WEAPON | SF_SHIELD)) == want && draw_at (o, MD_ALL)) {
            --n;                                /* (A section none, or not of those wanted: not read, part 59 not
                                                **   called, its time) */
        }
    }
    ost.want = 0xFF;
    return k;
}

/* The outfits' file opened (ost's fd), its index in the kinds' bank (read the first time), the draw begun (ost's
** state but a denizen's, the list empty): 1, or 0 (none, or not this version's: closed) */
static unsigned char opened (void)
{
    unsigned char k;
    unsigned boff;
    int n;

    memset (&ost, 0, offsetof (struct ost, did));
    ost.want = 0xFF;
    hy_bank (ovbank[OVBANKS]);
    strcpy (b, TR_DIR);
    strcat (b, "/dq0o.dat");
    hub_map (0);
    if ((ost.fd = open (b, O_RDONLY)) < 0) {
        return 0;
    }
    hub_map (CAT_BANK);                         /* (Its index into the catalogue's bank, the first time) */
    if ((OF_INDEX[0] | OF_INDEX[1] << 8) != OF_MAGIC) {     /* (Its head, its entries, 5 bytes each, their
                                                **   sections' lengths) */
        for (boff = 0, k = 0; boff < 4 + 6 * (unsigned) k && (n = read (ost.fd, b, sizeof b)) > 0; boff += n) {
            hub_map (CAT_BANK);
            memcpy (OF_INDEX + boff, b, n);
            k = OF_INDEX[3];
        }
    }
    hub_map (KIND_BANK);
    OF_N = 0;                                   /* (The list empty) */
    hub_map (CAT_BANK);
    if ((OF_INDEX[0] | OF_INDEX[1] << 8) != OF_MAGIC || OF_INDEX[2] != OF_VERSION) {
        close (ost.fd);
        return 0;
    }
    hub_map (0);
    return 1;
}

/* ---- A denizen's things (I_KIT's: ost's did, hx, hy, hsn, fac), each section m's a asks drawn by its kind (the
** index's entries, outfits.js's, "#", "&" or "%" and its kind's number, a byte; none, no things): its role's (KT_ROLE,
** as its lair's placed), its lair's own (KT_LAIR, with its first denizen's: its beds, its stores, its hoard), its
** personal things (KT_PURSE, as it's first met: its kind's [purse KEY], else its standing's, m's x[0], [standing N]),
** every line given, each on its own die (its stratum's seed, its number and the section's: the same things each
** time), and made (part 58's: filed with its lair's home square, IW_DEN; its belongings in its lair's home room) */
void __fastcall__ s_den_kit (const struct msg* m)
{
    static const char pre[3] = { '#', '&', '%' };
    unsigned off;
    unsigned char k, r;
    struct room* q;

    if (!opened ()) {
        return;
    }
    if ((r = room_of (ost.hsn, ost.hx, ost.hy)) != 0xFF) {    /* (Its lair's home room's floor, the chunk in the
                                                **   window: its belongings' squares, part 58's) */
        q = &C->room[r];
        ost.bx0 = ost.hx - cx_ + q->x;
        ost.by0 = ost.hy - cy_ + q->y;
        ost.bw = q->w;
        ost.bh = q->h;
    } else {
        ost.bx0 = ost.hx;
        ost.by0 = ost.hy;
        ost.bw = ost.bh = 1;
    }
    ost.level = 1;
    for (k = 0; k < 3; ++k) {
        if (!(m->a & 1 << k)) {
            continue;
        }
        ost.rs = ((unsigned) STRATUM (ost.hsn).seed ^ ost.did * 40503U ^ k * 0x3B1DU) | 1;
        b[0] = pre[k];                          /* (Its name in the index: the section's mark and its kind's number) */
        b[1] = m->x[1];
        b[2] = 0;
        if (!(off = find (b)) && k == 2) {      /* (No purse of its kind's own: its standing's) */
            strcpy (b, "standing 0");
            b[9] += m->x[0];
            off = find (b);
        }
        draw_at (off, MD_ALL);
    }
    close (ost.fd);
    hub_map (KIND_BANK);
    if (OF_N) {
        outmake (1);
    }
    hub_map (0);
}

/* ---- The character's outfit: the referee asked (RS_OUTFIT, the UI's slot: the UI waits on the kit), drawn into the
** list; 0 if there's none (no file, a referee that hasn't one: the kit as it was) */
unsigned char s_outfit (void)
{
    struct req* r;
    unsigned char occ = 0, k, first, okeep = 0, obent = 0, olen;
    unsigned off = 0, boff;

    if (!opened ()) {
        return 0;
    }
    r = &HUB->slot[SLOT_UI];
    r->kind = RS_OUTFIT;
    r->actor = 0;
    r->text[0] = 0;
    r = hub_ask (SLOT_UI);
    if (r->result) {
        r->state = REQ_FREE;
        close (ost.fd);
        return 0;
    }
    ost.budget = r->w1;
    ost.rs = r->w2 | 1;
    ost.tags = r->x[0] | r->x[1] << 8;
    ost.level = r->a;
    if (reply_line (0, "")) {                   /* (Its occupation's section: the culture's own first, NAME/c) */
        k = strlen (b);
        for (first = 0; first < 6 && !(ost.tags & 1u << first); ++first) {
        }
        b[k] = '/';
        b[k + 1] = "fivtsk"[first < 6 ? first : 0];
        b[k + 2] = 0;
        if (!(off = find (b))) {
            b[k] = 0;
            off = find (b);
        }
    }
    olen = ost.slen;                            /* (Its section's length: find's, the others' after it) */
    if (off && draw_at (off, MD_HEAD)) {        /* (Its flags, its keep, its bent, kept: read once, 2026-10-27, each
                                                **   draw_at two parts' copies; its purse, not the family's: a
                                                **   cleric's 6d6) */
        occ = ost.sflags;
        okeep = ost.keep;
        obent = ost.bent;
        if (ost.pn) {
            for (ost.budget = 0, k = ost.pn; k--; ) {
                ost.rs = ost.rs * 25173U + 13849U;
                ost.budget += 1 + (ost.rs >> 8) % ost.pd;
            }
        }
    }
    ost.freeall = (occ & SF_KIT) != 0;          /* (The universal kit: its temple's, given) */
    draw_at (find ("universal"), MD_ALL);
    ost.freeall = 0;
    if (off) {
        ost.slen = olen;
        draw_at (off, MD_NEED);                 /* (Its occupation's needs) */
    }
    first = 1;
    if (!(occ & SF_ARMED)) {                    /* (Its first weapon, its highest weapon skill's) */
        first = skills (1, SF_WEAPON, 1);
    }
    k = okeep ? okeep : 10;
    ost.kept = ost.budget * k / 100;            /* (A tenth of what's left kept as money) */
    ost.budget -= ost.kept;
    if (!(occ & SF_ARMED)) {                    /* (Its other weapons, three in all, and a shield) */
        skills (first, SF_WEAPON, 2);
        skills (1, SF_SHIELD, 1);
    }
    if ((k = obent) != 0) {                     /* (Its bent's armour) */
        hub_map (CAT_BANK);
        boff = OF_INDEX[5 * k + 1] | OF_INDEX[5 * k + 2] << 8;    /* (Entry k - 1's offset) */
        ost.slen = OF_INDEX[4 + 5 * OF_INDEX[3] + k - 1];
        draw_at (boff, MD_ALL);
    }
    ost.tool = 1;                               /* (Its skills' tools) */
    skills (1, 0, 255);
    if (off) {
        ost.slen = olen;
        draw_at (off, MD_CHANCE);               /* (Its likely and rare things) */
    }
    hub_map (0);
    HUB->slot[SLOT_UI].state = REQ_FREE;
    close (ost.fd);
    ost.budget += ost.kept;                     /* (The money: in the pouch, silver pennies, the odd farthings none
                                                **   (no cut pennies made for change: HOUSE.md's money), in lots of 250
                                                **   at most (a lot's count a byte, joined to 250), all of it: told
                                                **   together, 2026-10-28) */
    hub_map (KIND_BANK);
    for (boff = ost.budget >> 2; boff && (k = OF_N) < NOFIT; boff -= olen) {
        olen = boff > 250 ? 250 : boff;
        memset (OF_LIST + k, 0, sizeof *OF_LIST);
        OF_LIST[k].kind = KI_PENNIES;
        OF_LIST[k].qty = olen;
        OF_LIST[k].quality = 4;
        OF_LIST[k].place = PL_POUCH;
        OF_N = k + 1;
    }
    return 1;
}
