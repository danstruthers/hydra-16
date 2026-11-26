/*
** outfit.c - (dq0w, swapped part outfit: ../dq0/swap.inc, bank.s's stubs) the outfits drawn (games/dq0/outfits.txt,
** compiled by games/dq0w/outfits.js into lib/dq0/dq0o.dat; the plan the user reviewed, and DECISIONS.md's "Outfits,
** reach and things used up"): the character's at a new game's start (s_kit's), the referee asked for its makings
** (RS_OUTFIT: its occupation, its open skills by their mastery, its family's wealth, its tags, a seed), its sections
** found and drawn (part outdraw's draw_at) in the order an outfit's bought (HOUSE.md's outfits): the universal kit; the
** occupation's needs; its first weapon (its highest weapon skill's); a tenth of what's left kept (its keep's share);
** the other weapons, three in all, and a shield; its bent's armour; its skills' tools; its likely and rare things; the
** money (prices and the wealth in farthings; what's left all of it silver pennies, in lots of 250).  The things
** drawn in the list (outfit.h's); part outmake makes them.  Its own data in its image (it calls part outdraw, which puts it out
** and in again: none that lasts across a call), the draw's state the main RAM's (ost).
*/

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "trace.h"
#include "outfit.h"
void __fastcall__ time_kept (unsigned n);       /* (Part time_drift's: past any minute, wpool.c's, H16's flasks filled: wbody.h's
                                                **   POOL_MARK, POOL_FILL) */

#pragma code-name ("SW_outfit_CODE")
#pragma rodata-name ("SW_outfit_RODATA")
#pragma bss-name ("SW_outfit_RODATA")                 /* (Its own data: none of it wanted across a call of part outdraw's,
                                                **   which puts this part out and in again) */
#pragma local-strings (on)

#define OF_MAGIC        0x464F                  /* (dq0o.dat's: "OF", its version) */
#define OF_VERSION      5                       /* (2: each entry its section's flags, 5 bytes; 3: a thing's
                                                **   material, its fifth byte, the denizens' places; 4: the sections'
                                                **   lengths after the entries, 2026-10-27; 5: a count's dice, a
                                                **   jewel, 2026-10-28) */

/* A section by its name (the hash outfits.js's): its offset in the file, 0 none; its flags (its index entry's) in fflags */
static unsigned char fflags;
static unsigned char fk, fn, fl, fh;            /* (find's: static, the entries' walk the quicker) */
#pragma register-vars (push, on)                /* (Its entry's pointer in the zero page's register bank: an entry a
                                                **   few cycles, not some 160, a denizen's kind's found past all the
                                                **   named sections, 32,000 cycles in a climb's first letter's window:
                                                **   cmd2's, 2026-11-26) */
static unsigned __fastcall__ find (const char* p)
{
    unsigned h = 0;
    register const unsigned char* q;

    for (; *p; ++p) {
        h = h * 31 + (unsigned char) *p;
    }
    fl = (unsigned char) h;
    fh = h >> 8;
    hub_map (CAT_BANK);
    fn = OF_INDEX[3];
    for (fk = 0, q = OF_INDEX + 4; fk < fn; ++fk, q += 5) {
        if (q[0] == fl && q[1] == fh) {
            fflags = q[4];
            ost.slen = OF_INDEX[4 + 5 * fn + fk];    /* (Its length, the index's table after its entries: read whole,
                                                **   no more, 2026-10-27) */
            return q[2] | q[3] << 8;
        }
    }
    return 0;
}
#pragma register-vars (pop)

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
            --n;                                /* (A section none, or not of those wanted: not read, part outdraw not
                                                **   called, its time) */
        }
    }
    ost.want = 0xFF;
    return k;
}

/* The outfits' file opened (ost's fd: opened the first time and kept open, as the world goes on: an open of the
** game's lib folder on the card some 700,000 cycles, a denizen's kit's, 2026-10-30), its index in the kinds' bank
** (read the first time), the draw begun (ost's state but a denizen's, the list empty): 1, or 0 (none, or not this
** version's: closed) */
static int ofd = -1;
static unsigned char opened (void)
{
    unsigned char k;
    unsigned boff;
    int n;

    memset (&ost, 0, offsetof (struct ost, did));
    ost.want = 0xFF;
    if (ofd < 0) {
        hy_bank (ovbank[OVBANKS]);
        strcpy (b, TR_DIR);
        strcat (b, "/dq0o.dat");
        hub_map (0);
        if ((ofd = open (b, O_RDONLY)) < 0) {
            return 0;
        }
        for (k = 0; k < 2; ++k) {               /* (The file whole into its two banks as it's opened: as the game
                                                **   begins (outfits_in); each section then read from them, part
                                                **   outdraw's, not the card, fix2, 2026-11-24) */
            hy_bank (OF_BANK (k));
            read (ofd, (void*) 0x8000, 0x2000);
        }
        hub_map (0);
    }
    ost.fd = ofd;
    hub_map (CAT_BANK);                         /* (Its index into the catalogue's bank, the first time) */
    if ((OF_INDEX[0] | OF_INDEX[1] << 8) != OF_MAGIC) {     /* (Its head, its entries, 5 bytes each, their
                                                **   sections' lengths) */
        lseek (ofd, 0, SEEK_SET);
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
        close (ofd);
        ofd = -1;
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
** time), and made (part outmake's: filed with its lair's home square, IW_DEN; its belongings in its lair's home room) */
unsigned char ui_long (void);                   /* (dq0w.c's) */
void kit_give (void);                           /* (jobs.s's) */
void __fastcall__ s_den_kit (const struct msg* m)
{
    static const char pre[3] = { '#', '&', '%' };
    unsigned off;
    unsigned char k, r;
    struct room* q;

    if (!m) {                                   /* (0: the file opened and read into its banks, if the start's outfit
                                                **   didn't: dq0w.c's start, a game carried on drawing no outfit of the
                                                **   character's, its first denizen's kit read it whole behind a command,
                                                **   fix2, 2026-11-24) */
        if (ofd < 0) {
            opened ();
        }
        return;
    }
    if (!opened ()) {
        return;
    }
    if ((r = room_of (ost.hsn, ost.hx, ost.hy)) != 0xFF) {    /* (Its lair's home room's floor, the chunk in the
                                                **   window: its belongings' squares, part outmake's) */
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
    ost.kit = 1;
    ost.level = 1;
    ost.omint = 0xFF;                           /* (Its stratum's grave goods' coins' mint, not looked for yet; their
                                                **   age, five a century: part outdraw's) */
    k = STRATUM (ost.hsn).age;
    ost.age = k > 50 ? 250 : 5 * k;
    for (k = 0; k < 4; ++k) {
        off = 0;
        if (k < 3) {
            if (!(I_KIT_DRAW (*m) & 1 << k)) {
                continue;
            }
            ost.rs = ((unsigned) STRATUM (ost.hsn).seed ^ ost.did * 40503U ^ k * 0x3B1DU) | 1;
            b[0] = pre[k];                      /* (Its name in the index: the section's mark and its kind's number) */
            b[1] = I_KIT_DKIND (*m);
            b[2] = 0;
            if (!(off = find (b)) && k == 2) {  /* (No purse of its kind's own: its standing's) */
                strcpy (b, "standing 0");
                b[9] += I_KIT_STANDING (*m);
                off = find (b);
            }
            if (!off) {
                continue;                       /* (None: part outdraw not copied in for it) */
            }
        }
        hub_map (0);
        if (ost.kit == 2 || ui_long ()) {
            ost.kit = 2;                        /* (A command come: all drawn again after its answer: before a section's
                                                **   drawn, its part copied in, its section found first, the line come
                                                **   as it was looked for (cmd2's, 2026-11-26); and before anything's
                                                **   made) */
            kit_give ();                        /* (In kit_run's call: out of it at once, jobs.s's) */
            return;
        }
        if (off) {
            draw_at (off, MD_ALL);
        }
    }
    ost.kit = 0;
    hub_map (KIND_BANK);
    if (OF_N) {
        time_kept (0xFFFC);
        outmake (1);
        time_kept (0xFFFD);                     /* (Its flasks of oil filled: H16's, POOL_MARK and POOL_FILL) */
    }
    hub_map (0);
}

/* A carried-on game's kit (kit.dat above the dungeon's folder, the world's save's: things.c's kit_keep, slice 4.4's, D8's), in the
** list in place of the one drawn, its coins the new region's at the money-changer's cut, and the file left open at its start
** (ost's kept: part kit lays each thing's state on it, once made): 1; or 0 (none there, or empty: a new game's, dq0life's emptied it) */
static struct kitrec kr;
static unsigned char kit_read (void)
{
    char p[56];
    int fd;
    unsigned char n;

    hub_map (0);
    if (!HUB->savedir[0]) {
        return 0;
    }
    strcpy (p, HUB->savedir);
    strcat (p, "/../kit.dat");
    if ((fd = open (p, O_RDONLY)) < 0) {
        return 0;
    }
    for (n = 0; n < NOFIT && read (fd, &kr, sizeof kr) == sizeof kr; ++n) {
        hub_map (KIND_BANK);
        if ((CATALOGUE[kr.o.kind].form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT)) {
            kr.o.qty -= ((unsigned) kr.o.qty * MONEY_CUT + 50) / 100;
        }
        memcpy (OF_LIST + n, &kr, sizeof (struct ofit));
    }
    if (!n) {
        close (fd);
        return 0;
    }
    hub_map (KIND_BANK);
    OF_N = n;
    lseek (fd, 0, SEEK_SET);
    ost.kept = fd;
    return 1;
}

void __fastcall__ den_fetch (const struct msg* m);   /* (Part errands's: 0, the outfit fitted to its strength, wtake.c's fit) */

/* ---- The character's outfit: the referee asked (RS_OUTFIT, the UI's slot: the UI waits on the kit), drawn into the
** list; 0 if there's none (no file, a referee that hasn't one: the kit as it was) */
unsigned char s_outfit (void)
{
    struct req* r;
    unsigned char occ = 0, k, first, okeep = 0, obent = 0, olen, wv;
    unsigned off = 0, boff, cap;

    if (!opened ()) {
        return 0;
    }
    r = &HUB->slot[SLOT_UI];
    r->kind = RS_OUTFIT;
    r->actor = 0;
    r->text[0] = 0;
    r = hub_ask (SLOT_UI);
    if (RS_OUTFIT_NONE (*r)) {
        r->state = REQ_FREE;
        return 0;
    }
    ost.budget = RS_OUTFIT_MONEY (*r);
    ost.rs = RS_OUTFIT_SEED (*r) | 1;
    wv = RS_OUTFIT_HEFT (*r);                               /* (Its weapons' version, by its Strength: the referee's, D13's) */
    cap = RS_OUTFIT_BEARS (*r) << 2;                         /* (What it bears, a little burdened at most: quarter pounds) */
    ost.tags = RS_OUTFIT_TAGS (*r)[0] | RS_OUTFIT_TAGS (*r)[1] << 8;
    ost.level = RS_OUTFIT_LEVEL (*r);
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
    ost.budget += ost.kept;                     /* (The money: s_kit's, part kit's, the room of this one's image) */
    if (kit_read ()) {
        ost.budget = 0;                         /* (Its kit carried on: its own money in it, slice 4.4's) */
        return OF_CARRIED;
    }
    ost.kept = cap;                             /* (What it bears: part take_rule's, no stub of its own, the main RAM's room) */
    den_fetch (0);
    return 1 + (wv < 3 ? wv : 0);               /* (Drawn; its weapons' version too, the kit's to make: s_kit's) */
}
