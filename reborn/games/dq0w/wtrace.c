/*
** wtrace.c - (dq0w) the traces on the floor (slice 4.1; DUNGEON.md's light kept, and the traces on the floor; the
** user's, 2026-10-16: "Dust and dirt on the floor may or may not be disturbed, depending on current movements of
** denizens or the player, and tracking ability can use that to understand how many creatures and how recently (and
** their types, possibly) have traveled the area."; "Traces of passage include smells as well, which dissipate with
** time (more quickly than fresh dust accumulates to cover tracks)."): every passing over a square kept, the world's,
** for it moves every body: the character's walk a square at a time (dq0w.c's step_on), the actors' steps (wbody.c's:
** their intents, a coarse move's too), the denizens out of the character's way (their task's roam: I_TROD), each
** journalled as it's made (the hub's traces' journal, TJ_AT: trod, the main RAM's, all a step costs) and taken into
** the store as the journal fills (tr_flush, the main loop's: a part's swap for many passings).
**
** The store: two banks of the world's own, records of eight bytes, four to a bucket, a square's by a hash of its
** chunk and its place in it: on a square, whose (a kind, 0 the character; smoke on the air), how many passings, when
** the last went (in fours of minutes: TQ) and its way.  A passing of one already there is counted in its record, a
** new one goes in a free place or over the oldest in its bucket (trodden out: old traces go as the place is busy, and
** the store never fills).  Its chunks in a directory (the world's own paging bank's, after ipage.c's table): those it
** holds, each wanted as a passing's or a reading's chunk (the nine round the character's always, as it comes into
** one), the one least lately wanted that isn't round the character put out to make room for another: its records
** written to its region's file in the save folder (trSXXYY.dat, a version of it each time: ../dq0/vfile.inc's, each
** chunk a group of its own there: its place in the region, its flags, its count) and freed; read again as it's
** wanted.  The save writes every chunk changed since (tr_flush's keep); a game carried on begins with none, and reads
** them as they're wanted; without a folder, a chunk put out is lost.  A chunk is kept swept (TC_SWEPT) as its keepers
** say (I_SWEPT, journalled: a lair of a kind that sweeps its floors placed).  The passings of those out of the
** character's way (a coarse move's, TS_NOWAY) are kept only where the store holds their chunk (none read in for them).
**
** And the traces read (I_TRACE, the UI's: the floor looked at, a track, a smell): what the floor shows of them
** (HOUSE.md's light and traces: its material, the maker's; the dust the stratum's age has laid on it; swept or not),
** each kind's about the character, how clearly they show (a print's hold on the floor, its tread, its passings,
** fading with time) or how strongly they smell (its smell, fading faster, carried a few squares through the open ways,
** not through rock or a shut door: the world's, as the dungeon is), the referee's test (Tracking, P_TRACK; a smell
** noticed, P_NOTICE_SMELL) at steps by the clearest, and as much as the grade reads: the UI bank's tread, and its words
** (the world's, as the things' are), E_VIEW V_TRACED.  Each kind's numbers and words are the denizens' data's
** (kinds.txt's tread, marks, smell, scent: dq0d/make.js's dq0tr.dat, its first record the character's own).
**
** Swapped part 27's (the store), 29's (its files), 28's (the reading) and 30's (its words: ../dq0/swap.inc, bank.s's
** stubs); trod the main RAM's.
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "trace.h"
#include "tstore.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)

/* ---- The journal (the main RAM's: a step's cost) */

unsigned char tr_n;                             /* (The journalling parts' own: trod.inc) */

/* ==== Part 27: the store, the journal taken in */
#pragma code-name (push, "SW27CODE")
#pragma rodata-name (push, "SW27RODATA")
#pragma bss-name (push, "SW27RODATA")           /* (Its own data, what's wanted only while it runs: the main RAM's room) */

#include "tput.inc"

/* (What's wanted through a call of part 29's, tr_file: the main RAM's, or the C stack's, this part's own data its
** image's again after it, as ../dq0/swap.inc has it) */
#pragma bss-name (push, "BSS")
static unsigned char rsn, lim, pcs, smoke;      /* (The chunk at hand: its stratum, its place; the chunks the store
                                                **   holds at most; the character's chunk; a flame's smoke) */
static unsigned rcx, rcy, pcx, pcy;
#pragma bss-name (pop)
#pragma static-locals (push, off)

static void own (void)
{
    hy_bank (ovbank[OVBANKS]);
}

/* The chunk at hand's entry plus 1, or 0 (the directory in the window after) */
static unsigned char find (void)
{
    struct tch* e;
    unsigned char h;

    own ();
    for (h = 0, e = THD->ch; h < NTCH; ++h, ++e) {
        if (e->used && e->sn == rsn && e->cx == rcx && e->cy == rcy) {
            e->stamp = ++THD->stamp;
            return h + 1;
        }
    }
    return 0;
}

/* The chunk at hand's entry plus 1: found, or made (a place for it: a free one, or the one least lately wanted that
** isn't round the character put out, its records written to its region's file; its own read in from it: part 29's);
** 0 if it can't be */
static unsigned char need (void)
{
    struct tch* e;
    unsigned char h, best;
    unsigned st = 0;

    if ((h = find ()) != 0) {
        return h;
    }
    for (best = 0, h = 1, e = THD->ch; h <= lim; ++h, ++e) {
        if (!e->used) {
            best = h;
            break;
        }
        if ((e->sn != pcs || e->cx - pcx + 1 > 2 || e->cy - pcy + 1 > 2 || lim < 9) && (!best || e->stamp - st > 0x8000U)) {
            best = h;
            st = e->stamp;
        }
    }
    if (!best) {
        return 0;
    }
    own ();
    if (THD->ch[best - 1].used) {
        tr_file (best, TF_OUT);                 /* (Its records out, freed: part 29's) */
        own ();
    }
    e = &THD->ch[best - 1];
    e->used = 1;
    e->sn = rsn;
    e->cx = rcx;
    e->cy = rcy;
    e->flags = 0;
    e->stamp = ++THD->stamp;
    tr_file (best, TF_IN);                      /* (Its own read in, if it has any) */
    return best;
}

/* A passing (s) taken into the store: its chunk wanted (read in, if it isn't: not for a coarse move's), its record
** counted; a lit flame's smoke with it; a chunk kept swept marked so */
static struct tstep s;
static void take (void)
{
    struct stratum* t = &STRATUM (s.sn);
    unsigned char h, k = s.kind, w = s.head;
    unsigned q, m;

    if (t->n != s.sn || !t->made || s.x - t->x0 >= t->w || s.y - t->y0 >= t->h) {
        return;                                 /* (Its stratum's record out of its slot: let go) */
    }
    rsn = s.sn;
    rcx = (s.x - t->x0) >> CW_BITS;
    rcy = (s.y - t->y0) >> CW_BITS;
    q = ((s.x - t->x0) & (CW - 1)) | ((s.y - t->y0) & (CW - 1)) << CW_BITS;
    m = (unsigned) (s.t / TQ);                  /* (All s wants first: a part's call puts this part's own data back as
                                                **   its image has it) */
    if (!(h = (w & TS_NOWAY) ? find () : need ())) {
        return;
    }
    own ();
    THD->ch[h - 1].flags |= k == TK_SWEPT ? TC_SWEPT | TC_DIRTY : TC_DIRTY;
    pc = h;
    pk = k;
    pn = 1;
    pt = m;
    if (k < TK_SMOKE) {
        psq = q | (w & TS_NOWAY ? 0 : (unsigned) (w & 7) << 10 | TW_WAY);
        put ();
    }
    if ((w & TS_LIT) && smoke) {
        pk = TK_SMOKE;
        psq = q | (unsigned) smoke << 10;
        put ();
    }
}

/* The journal's ring (the denizens' or the character's: k) taken in from its place so far to its count now (those
** written over since, lost) */
static void __fastcall__ ring (unsigned char k)
{
    unsigned at, n;

    own ();
    at = k ? THD->jpn : THD->jn;
    hub_map (DEN_BANK);
    n = k ? TJ_AT->pn : TJ_AT->n;
    if (n - at > (k ? NTRAIL : NJOURNAL)) {
        at = n - (k ? NTRAIL : NJOURNAL);
    }
    for (; at != n; ++at) {
        hub_map (DEN_BANK);
        memcpy (&s, k ? &TJ_AT->p[at % NTRAIL] : &TJ_AT->e[at % NJOURNAL], sizeof s);
        take ();
    }
    own ();
    if (k) {
        THD->jpn = n;
    } else {
        THD->jn = n;
    }
}

/* The journal taken into the store (the store begun, if it isn't: its directory's magic not there, the task just
** begun, the kinds' numbers read in); as the character comes into a chunk, the nine round it wanted (read in); keep 1,
** the store kept (the save's): every chunk changed since written to its region's file */
void __fastcall__ s_tr_flush (unsigned char keep)
{
    signed char x, y;
    unsigned char h;
    int fd;

    tr_n = 0;
    own ();
    if (THD->magic != TR_MAGIC) {
        for (h = 2; h < 4; ++h) {               /* (The store empty) */
            hy_bank (ovbank[OVBANKS + h]);
            memset ((void*) 0x8000, 0, 0x2000);
        }
        own ();
        memset (THD, 0, sizeof (struct thead));
        THD->magic = TR_MAGIC;
        strcpy (path, TR_DIR);                  /* (The kinds' numbers: the lib folder's dq0tr.dat) */
        strcat (path, "/dq0tr.dat");
        TKN->n = 0;
        if ((fd = open (path, O_RDONLY)) >= 0) {
            for (h = 0; h < NK && read (fd, buf, TREC) == TREC; ++h) {
                own ();
                TKN->tread[h] = ((char*) buf)[12];
                TKN->smell[h] = ((char*) buf)[13];
                TKN->size[h] = ((char*) buf)[15];
                TKN->n = h + 1;
            }
            close (fd);
        }
    }
    hub_map (0);
    lim = HUB->small ? 3 : NTCH;                /* (dq0 -C: a few, the tests' write-backs) */
    pcs = HUB->pc_stratum;
    pcx = (HUB->pc_x - STRATUM (pcs).x0) >> CW_BITS;
    pcy = (HUB->pc_y - STRATUM (pcs).y0) >> CW_BITS;
    smoke = memcmp (HUB->pc_lamp, "torch", 5) ? 2 : 5;     /* (A torch smokes; a lantern, a candle a little) */
    ring (0);
    ring (1);
    own ();
    if (THD->lsn != pcs + 1 || THD->lcx != pcx || THD->lcy != pcy) {
        THD->lsn = pcs + 1;
        THD->lcx = pcx;
        THD->lcy = pcy;
        for (y = -1; y <= 1; ++y) {
            for (x = -1; x <= 1; ++x) {
                rsn = pcs;
                rcx = pcx + x;
                rcy = pcy + y;
                if (rcx < (STRATUM (pcs).w + CW - 1) >> CW_BITS && rcy < (STRATUM (pcs).h + CW - 1) >> CW_BITS) {
                    need ();
                }
            }
        }
    }
    for (h = 1; keep && h <= NTCH; ++h) {
        own ();
        if (THD->ch[h - 1].used && (THD->ch[h - 1].flags & TC_DIRTY)) {
            tr_file (h, TF_KEEP);
        }
    }
    hub_map (0);
}

#pragma static-locals (pop)

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)

/* ==== Part 28: the traces read (I_TRACE's, the UI's: HOUSE.md's light and traces) */
#pragma code-name (push, "SW28CODE")
#pragma rodata-name (push, "SW28RODATA")
#pragma bss-name (push, "SW28RODATA")

#define NFOUND          6                       /* (The kinds read about the character, at most) */
#define SR              6                       /* (A smell's reach one way, squares) */

static struct tread R;
static unsigned char how, way, ws, dust, fdust, hold, nf, smk, pnose;
static unsigned wx, wy, now, life;
static unsigned char fk[NFOUND], fn[NFOUND], fh[NFOUND], fc[NFOUND];
static unsigned fa[NFOUND];                     /* (Each kind found: whose, its passings, the last's way, how clearly
                                                **   it shows, how long since the last went: TQs) */
#define NREACH          24
static unsigned char reach[NREACH];             /* (The squares a smell's carried from: x and y times 16, from the
                                                **   character's less SR) */
static struct trace rr;

/* The directory's entry plus 1 of square x, y's chunk (on the character's stratum), its place in it (sq), its flags
** (fl); 0 if the store hasn't it */
static unsigned sq;
static unsigned char fl;
static unsigned char __fastcall__ chunk_of (unsigned x, unsigned y)
{
    static struct tch* e;
    static unsigned cx, cy;
    static unsigned char h;

    x -= STRATUM (ws).x0;
    y -= STRATUM (ws).y0;
    cx = x >> CW_BITS;
    cy = y >> CW_BITS;
    sq = (x & (CW - 1)) | (y & (CW - 1)) << CW_BITS;
    hy_bank (ovbank[OVBANKS]);
    for (h = 0, e = THD->ch; h < NTCH; ++h, ++e) {
        if (e->used && e->sn == ws && e->cx == cx && e->cy == cy) {
            fl = e->flags;
            return h + 1;
        }
    }
    return 0;
}

/* Square x, y's floor: how well it holds a print (hold, its result: 0, no floor there) and how long one lasts (life,
** TQs); its material (R.floor, the maker's) and its dust (fdust: the stratum's age's; swept, none, its marks scattered
** too) */
static unsigned char __fastcall__ floor_at (unsigned x, unsigned y)
{
    static unsigned char* p;
    static unsigned char r, m;

    hold = 0;
    if (!(p = at (ws, x, y)) || (m = TILE (*p)) == T_ROCK || m == T_WALL || m == T_WINDOW) {
        return 0;
    }
    r = room_of (ws, x, y);
    R.floor = m = C->mat[r == 0xFF ? NROOMS : r][MP_FLOOR];
    fl = 0;
    chunk_of (x, y);
    fdust = fl & TC_SWEPT ? 0 : dust;
    life = 15;                                  /* (A scrape on stone: an hour) */
    if (m == MAT_EARTH || m == MAT_SOIL) {
        life = 1080;                            /* (A print in earth: three days; soft soil the better) */
        hold = 6 + (m == MAT_SOIL);
    } else if (m == MAT_STONE) {
        hold = 2 + fdust / 3;                   /* (Rough stone: little; its dust a little more) */
    } else {
        life = fdust >= 2 ? 32400 : fdust ? 360 : 15;   /* (Marks in dust: till new dust covers them, months) */
        hold = fdust ? 3 + fdust : 1;           /* (Flags, paving: nothing of themselves, their dust every mark) */
    }
    if (fl & TC_SWEPT) {
        fdust = TD_SWEPT;                       /* (Swept: the marks scattered) */
        return --hold;
    }
    return hold;
}

/* How far a scent's faded at its age a (TQs): a point each doubling of it (hub.h's SCENT_LIFE: its strength's
** halving; the one place rain and running water will speed it, slice 4.9) */
static unsigned char __fastcall__ faded (unsigned a)
{
    static unsigned char n;

    for (n = 0; a; a >>= 1) {
        ++n;
    }
    return n;
}

/* Square x, y's records taken in (its floor's hold and life; a smell's distance d): each kind's passings, the last's
** time and way, how clearly they show (a print's: its hold, its tread, its passings, less as it weathers) or how
** strongly they smell (its smell and the character's nose, hub.h's SCENT_LOG, less a point as its age doubles, faded's,
** and half a point a square off); the smoke on the air, the strongest (smk) */
static void __fastcall__ found (unsigned x, unsigned y, unsigned char d)
{
    static unsigned b, a;
    static unsigned char h, i, j, k, c, tr, sm;

    if (!(h = chunk_of (x, y))) {
        return;
    }
    b = BUCKET (h, sq);
    for (i = 0; i < 4; ++i) {
        hy_bank (TR_BANK (b));
        memcpy (&rr, TR_AT (b) + i, sizeof rr);
        if (rr.c != h || SQ_OF (rr.sq) != sq) {
            continue;
        }
        k = rr.k;
        a = now - rr.t;
        c = faded (a) + d / 2;
        if (k == TK_SMOKE) {                    /* (Smoke on the air: its strength, less as it fades) */
            j = rr.sq >> 10 & 7;
            if (how == TRC_SMELL && c < j && j - c > smk) {
                smk = j - c;
            }
            continue;
        }
        hy_bank (ovbank[OVBANKS]);              /* (Its kind's numbers: part 27's) */
        if (k >= TKN->n || (how == TRC_SMELL && !k)) {
            continue;                           /* (Its own sweat the character doesn't smell) */
        }
        tr = TKN->tread[k];
        sm = TKN->smell[k];
        if (how == TRC_SMELL) {
            sm = SCENT_LOG (sm, pnose) + 1;
            c = c >= sm ? 0 : sm - c;
        } else {
            c = hold + tr / 3 + (rr.n >= 4) + (rr.n >= 12);
            j = a >= life ? c : a / (life / 4 + 1);
            c = !tr || j >= c ? 0 : c - j;
        }
        for (j = 0; j < nf && fk[j] != k; ++j) {
        }
        if (j == nf) {
            if (nf == NFOUND) {
                continue;
            }
            fk[nf++] = k;
            fa[j] = 0xFFFF;
        }
        fn[j] = fn[j] + rr.n > 255 ? 255 : fn[j] + rr.n;
        if (a < fa[j]) {
            fa[j] = a;
            fh[j] = rr.sq & TW_WAY ? rr.sq >> 10 & 7 : 0xFF;
        }
        if (c > fc[j]) {
            fc[j] = c;
        }
    }
}

/* Square x, y lets the air through (a smell carried): not rock nor a wall; a door open (or broken down), a
** portcullis, a grating, an arrow slit, a window its shutters open and no glass in it */
static unsigned char __fastcall__ airy (unsigned x, unsigned y)
{
    static unsigned char* p;
    static struct portal* q;
    static unsigned char t;

    if (!(p = at (ws, x, y)) || (t = TILE (*p)) == T_ROCK || t == T_WALL) {
        return 0;
    }
    if ((t == T_DOOR || t == T_WINDOW) && (q = portal_at (cx_, cy_)) != 0) {
        t = q->kind;
        return t == PK_PORTCULLIS || t == PK_GRATE || t == PK_SLIT ? 1 : t == PK_WINDOW ?
            !(q->parts & PP_GLASS) && (!(q->parts & PP_SHUTTERS) || (q->state & PS_OPEN)) :
            (q->state & (PS_OPEN | PS_WRECKED)) != 0;
    }
    return 1;
}

/* The smell about the character: carried from the squares the air reaches, along each way from it as far as the
** air goes (SR one way, and half that the ways beside it; about it, SR / 3 every way), each one's records taken in by
** its distance; and the lights lying lit there (a sconce's torch, a fire), their smoke */
static void sniff (void)
{
    static unsigned char h, d, r, nr;
    static unsigned x, y, it;

    found (wx, wy, 0);
    for (nr = h = 0; h < 8; ++h) {
        r = way > H_NW ? SR / 3 : h == way ? SR : ((h - way + 1) & 7) < 3 ? SR / 2 : 0;
        for (d = 1; d <= r; ++d) {
            x = wx + head_x[h] * d;
            y = wy + head_y[h] * d;
            if (!airy (x, y)) {
                break;
            }
            found (x, y, d);
            if (nr < NREACH) {
                reach[nr++] = (unsigned char) (x - wx + SR) | (unsigned char) (y - wy + SR) << 4;
            }
        }
    }
    for (it = 0; (it = items (it, IN_SQUARE | IN_FLAGS, I_LIT)) != 0; ) {  /* (Lit things lying near: their smoke) */
        x = ip->x - wx + SR;
        y = ip->y - wy + SR;
        for (d = 0; ip->sn == ws && x < 16 && y < 16 && d < nr; ++d) {
            if (reach[d] == (unsigned char) (x | y << 4) && smk < 4) {
                smk = 4;
            }
        }
    }
    hub_map (0);
}

unsigned __fastcall__ s_traced (const struct msg* m)
{
    static const unsigned took[3] = { 50, 600, 30 };   /* (TRC_*'s time: a look; a minute's study; a sniff) */
    static const unsigned char clear[3] = { 4, 2, 1 };  /* (How clearly a trail shows, at least, to be told) */
    static struct trail* t;
    static unsigned w;
    static signed char st, dx, dy;
    static unsigned char i, j, k, best;

    tr_flush (0);                               /* (The journal in, the chunks round the character too: first, before
                                                **   this part's own data's set, its image put back after) */
    how = (unsigned char) m->w1;
    way = m->w1 >> 8;
    w = m->w2 ? m->w2 : took[how];
    hub_map (0);
    ws = HUB->pc_stratum;
    wx = HUB->pc_x;
    wy = HUB->pc_y;
    now = (unsigned) (HUB->now / TQ);
    pnose = HUB->pcsum.smell >> 2;              /* (The character's nose: a quarter of its smell sense, the
                                                **   referee's) */
    R.how = how;
    i = STRATUM (ws).age;
    dust = 3 + (i >= 3) + (i >= 6) + (i >= 12); /* (A century's dust a lot, the more the older) */
    floor_at (wx, wy);                          /* (Where it stands: its material, its dust) */
    R.dust = fdust;
    hub_map (SIGHT_BANK);
    k = SIGHT->dark;
    hub_map (0);
    if (how != TRC_SMELL && k) {
        R.result = TRR_DARK;                    /* (Too dark to see the floor: the eye's alone) */
        goto done;
    }
    if (how == TRC_SMELL && !HUB->pcsum.smell) {
        R.result = TRR_NOSE;
        goto done;
    }
    k = HUB->pc_light;                          /* (Its own flame's smoke about it: no other's told) */
    if (how == TRC_SMELL) {
        sniff ();
    } else {
        for (dy = -2; dy <= 2; ++dy) {          /* (The prints on the floor about it: two squares each way) */
            for (dx = -2; dx <= 2; ++dx) {
                if (floor_at (wx + dx, wy + dy)) {
                    found (wx + dx, wy + dy, 0);
                }
            }
        }
    }
    hub_map (0);
    for (best = j = 0; j < nf; ++j) {
        if (fc[j] > best) {
            best = fc[j];
        }
    }
    if (k) {
        smk = 0;
    }
    if (smk > best) {
        best = smk;
    }
    R.grade = G_MS;
    if (how != TRC_FLOOR) {                     /* (The referee's test, the world's slot: Tracking, or a smell noticed,
                                                **   at steps by the clearest) */
        st = how == TRC_SMELL ? 2 * best - 6 : best - 4;
        wsteps = st < -8 ? -8 : st > 6 ? 6 : st;
        R.grade = ask_w (RS_TEST, how == TRC_SMELL ? P_NOTICE_SMELL : P_TRACK, 0);
    }
    R.trod = best >= 4;
    if (how == TRC_SMELL && R.grade == G_CF) {
        nf = smk = 0;                           /* (Nothing noticed) */
    }
    while (R.ntr < NTRAILS) {                   /* (The clearest first: whose, its words, its passings, how long since,
                                                **   its way, part 30's to word) */
        for (best = 0, k = 0xFF, j = 0; j < nf; ++j) {
            if (fc[j] > best && fc[j] >= clear[how]) {
                best = fc[j];
                k = j;
            }
        }
        if (k == 0xFF) {
            break;
        }
        fc[k] = 0;
        t = &R.tr[R.ntr++];
        t->kind = i = fk[k];
        t->n = fn[k];
        t->age = fa[k];
        t->head = fh[k];
        hy_bank (ovbank[OVBANKS]);
        t->big = TKN->size[i] >= 5;
    }
    R.smoke = smk >= 4 ? 2 : smk ? 1 : 0;
    if (!R.ntr && !R.smoke && how != TRC_FLOOR) {
        R.result = TRR_NONE;
    }
done:
    hub_map (UI_BANK);
    memcpy (&UIREC->tread, &R, sizeof R);       /* (The reply, its words to come: part 30's, then the event) */
    hub_map (0);
    return tr_words (w);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)
