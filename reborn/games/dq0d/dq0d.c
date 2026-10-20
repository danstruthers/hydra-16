/*
** dq0d.c - Dingus Quest Zero's denizens (DENIZENS.md): a task of its own, started by the UI (dq0d SEG ROLE LIBDIR:
** the hub's segment, its role there, hub.h, and the game's lib folder).  As each stratum's plan is grown, it decides
** the stratum's populations, by its style and its depth, from the kinds a stand-in theme has (kinds.txt, made
** lib/dq0/dq0d.dat, read into a bank of its own as it starts): the lairs, where they lie in the plan, and the
** denizens of each, a record each from then on; so nothing is made up as the character turns a corner.  As the
** character comes near a lair (its chunk, or one beside it), its denizens are placed in it, each on a square of its
** own (the chunk made, the maker's, if it isn't yet); and those near the character are actors (the hub's: the world
** keeps the character out of their way, the senses see them), each deciding what it means to do by its kind and what
** it senses, saying so (its intents, the hub's di: the world moves their bodies, by the character's rules, and
** strikes their blows): to wander about its lair (a few squares, or a wide range), to keep to it, to watch the
** character, to keep its distance from it (a shy one), to come towards it (a bold one), to strike at it.  It senses
** through its own senses: it sees the character as far as its sight goes by the character's light (or its
** darkvision, with none), along a line clear of rock, walls and shut doors; it hears it moving, nearer.  At the turns
** pace it acts only in its turn (the hub's den_go: the world's, at each slice of the clock, hub.h's TURN_SLICE, and
** after the world's own work), and says when it's done (I_DONE), so they act through all the time what the
** character does takes, the same whatever the tasks' timing: those out of its sight and not sensing it coarsely, a
** few squares a while.  Those out but away from the character (not actors) move coarsely too, to squares about their
** lairs in chunks that are made (roam): their records are its own.  No two are on a square, nor any on the
** character's or where it's going.  Each has an attitude to the character, its faction's to begin with
** (factions.txt's, a stand-in theme's: a theme's will take their place), moved by what happens and by the character's
** words (the UI's request, the hub's sway: the referee's test), and acts by it: the hostile come at the character, an
** ally follows it and takes its requests; factions hostile to one another keep off one another.  (Their own exploring
** comes later.)
**
** Its records (the lairs, the denizens) are in a bank of its own, a record at a time copied out, a cache (move 2.8:
** no fixed count of them): each region's (the frame's, 256 squares each way: a lair's by its chunk's first square, a
** denizen's by its square, or, not placed yet, its lair's; no stratum's record wanted to find it) all in it or none, those round the character's always (one each way), the rest in
** the save folder (dnSXXYY.dat, a version each time it's written: ../dq0/vfile.inc's), read in as they're wanted,
** put out (the one least lately wanted) as a record's wanted and there's none free; each denizen and each lair known
** by a number of its own, never another's (a name given, a denizen's lair), its record's place the cache's.  Kept
** with the game (dq0 -d: the UI asks, the hub's dsave), the save folder's dens.dat: the bank as it is, its regions'
** table in its head.
**
** Its code that's seldom called is in swapped parts (../dq0/swap.inc: run in the main RAM's top, a part at a time,
** copied there from the overlays' file's banks, LIBDIR/dq0d.ovl, as it's called; bank.s's stubs): part 1, the kinds
** read in, the populations, the placing; part 2, the character's ways with a denizen, and harmed; part 3, the
** records paged; part 4, the regions round the character wanted, the records kept, the task's start, and what
** its actors seldom do (one made an actor or no longer one, killed, its kin dismayed, an ally come along, a
** cornered one's dart).
** The blows its actors strike and take are the world's, told as the denizens' events (E_FIGHT): it takes them to heart
** (fought: fleeing, dismay, harmed, a death recorded).
*/

#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "lore.h"

#pragma static-locals (on)                      /* (Its locals static: quicker, and nothing here is reentered) */
#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

/* ---- The swapped parts' stubs (bank.s's) */

int __fastcall__ kinds_load (const char* dir);  /* Part 1 */
void __fastcall__ populate (unsigned char n);
void __fastcall__ place (unsigned char i);
void __fastcall__ keep (unsigned char back);    /* Part 3 */
unsigned char __fastcall__ rec_new (unsigned char lair);
unsigned char __fastcall__ reg_need (unsigned char sn, unsigned char rx, unsigned char ry);
void dneed (void);
void __fastcall__ harmed (unsigned char k);     /* Part 2 */
void sway (void);
int __fastcall__ start (const char* dir);      /* Part 4 */
void __fastcall__ enlist (unsigned char i, unsigned char k);
void __fastcall__ retire (unsigned char k);
void __fastcall__ killed (unsigned char k);
void __fastcall__ dismay (unsigned char k);
unsigned char __fastcall__ along (unsigned char k);
unsigned char __fastcall__ dart (unsigned char k);
extern unsigned char ovbank[2];
extern char _SWAP_START__[];

/* ---- The kinds: the data file's (make.js's, from kinds.txt: the game's bestiary), in a bank of their own */

struct dkind {
    char            name[12];
    char            key[8];                     /* The referee's creature (its sheet, later: RS_NEW_ACTOR's key) */
    unsigned char   flags;                      /* K_* */
    unsigned char   styles;                     /* The strata's styles it lives in: a bit each (1 << ST_*) */
    unsigned char   deep0, deep1;               /* The strata it lives in, from the top down */
    unsigned char   nmin, nmax;                 /* How many in a lair */
    unsigned char   lairs;                      /* Lairs a stratum, at most */
    unsigned char   range;                      /* Squares from its lair it goes */
    unsigned char   sight, dark;                /* Squares it sees the character by its light, and with none */
    unsigned char   hear;                       /* Squares it hears it moving */
    unsigned char   step;                       /* Tenths of a second a square */
    unsigned char   faction;                    /* Its faction (factions.txt's, by number: 1 the first) */
    unsigned char   mind;                       /* M_* */
    signed char     sway;                       /* Steps for (or against) swaying it, besides its attitude and mind */
    unsigned char   eyes;                       /* Its eyes' adapting to the dark: tenths to a 16th of a level (the
                                                **   senses', who keep them: DUNGEON.md's perception as layers) */
    unsigned char   form;                       /* Its size, its nature, the light it keeps to (lore.h's KF_*) */
    unsigned char   land, climate;              /* Its habitat (LA_*, CL_*: the history's, choosing a region's) */
    unsigned char   arms;                       /* How it fights (AR_*) */
};
#define K_SHY           0x01                    /* It keeps its distance from the character */
#define K_LAIR          0x02                    /* It keeps to its lair */
#define K_WAYS          0x04                    /* Its lairs are by the stratum's ways in and out */
#define K_BOLD          0x08                    /* It comes towards the character */
#define K_BONES         0x10                    /* It leaves its bones, dead (else its body) */
enum { M_MINDLESS, M_BEAST, M_LOW, M_AVERAGE, M_CUNNING };   /* Its mind: nothing sways it; Animalcraft only; easily
                                                **   fooled, hard to reason with; neither; hard to fool */
#define KINDS           ((struct dkind*) 0x8000)
typedef char dkind_fits[sizeof (struct dkind) == 40 ? 1 : -1];     /* (make.js's records) */

/* The factions (factions.txt's: a stand-in theme's, after the kinds in the data file, the first none): each one's
** tongue, its attitude to the character as it first meets it, how it stands with each other one (AT_HOSTILE,
** AT_WARY, AT_INDIFFERENT, AT_FRIENDLY), copied out into the main RAM as it starts */
#define NFACT           8
struct faction {
    char            name[12];
    char            tongue[8];                  /* ("": none) */
    unsigned char   att;                        /* AT_* */
    unsigned char   rel[NFACT];                 /* AT_*: each faction's, by number */
    unsigned char   spare[3];
};
typedef char faction_fits[sizeof (struct faction) == 32 ? 1 : -1];
static unsigned char nfacts;
static char ftongue[NFACT][8];
static unsigned char fatt0[NFACT], frel[NFACT][NFACT];
static unsigned char kfact[SND_KINDS];          /* (Each kind's faction: for the records, without their kinds) */

/* ---- The records: the lairs and the denizens, in a bank of their own (a cache: its records' places free, kind 0,
** or a region's that's in it) */

#define NLAIRS          64
#define NDENS           240
struct lair {
    unsigned char   sn, kind, n;                /* Its stratum, its kind, its denizens */
    unsigned char   flags;                      /* L_* */
    unsigned char   link;                       /* The stratum's link it's by, or 0xFF (a room of its chunk) */
    unsigned        id;                         /* Its number (1 on: the order the lairs were decided in) */
    unsigned        cx, cy;                     /* Its chunk (from the stratum's x0, y0) */
    unsigned        hx, hy;                     /* Its home square (placed) */
    unsigned char   rx, ry;                     /* Its region (its chunk's first square's, the frame's) */
};
#define L_PLACED        0x01                    /* Its denizens placed */
struct den {
    unsigned char   kind;
    unsigned char   flags;                      /* D_* */
    unsigned char   sn, face;
    unsigned        id, lid;                    /* Its number (1 on), its lair's */
    unsigned        x, y;                       /* Its square (placed) */
    unsigned long   ready;                      /* When it can act again */
    unsigned char   attitude;                   /* To the character (hub.h's AT_*) */
    unsigned char   order;                      /* An ally's request (hub.h's OR_*) */
    unsigned char   lsn, lrx, lry;              /* Its lair's stratum, and its region (its own, not placed yet: the
                                                **   lair's rx, ry) */
    unsigned        hx, hy;                     /* Its lair's home square (placed) */
};
#define D_OUT           0x01                    /* On a square of its own */
#define D_ACTOR         0x02                    /* An actor now (the hub's entry the truth while it is) */
#define D_DEAD          0x04                    /* Dead (its body left where it fell: an item) */
#define NNAMES          8
#define NDREG           32
#define GIVE_WAY        20                      /* (Rounds the minds wait for the UI's command in hand: a second) */
#define DN_MAGIC        0x4E44                  /* "DN": dens.dat's */
struct head {
    unsigned        magic;
    unsigned char   nlairs, ndens;              /* The records' places used, up to the last in use */
    unsigned char   fatt[NFACT];                /* Each faction's attitude to the character now (its newcomers') */
    struct { unsigned id; char name[12]; } names[NNAMES];    /* The names the character's given (a denizen's
                                                **   number; 0: none) */
    unsigned        ids, lids;                  /* The denizens' and the lairs' numbers given */
    unsigned char   done[32];                   /* Each stratum's populations decided (a bit each: DONE) */
    struct {
        unsigned char sn, rx, ry;
        unsigned    stamp;
    }               reg[NDREG];                 /* The regions whose records are in the cache (sn its stratum plus 1:
                                                **   0 the entry free) */
    unsigned        rstamp;
};
#define DONE(n)         (hd.done[(n) >> 3] & 1 << ((n) & 7))
struct pop {
    struct head     h;
    struct lair     lair[NLAIRS];
    struct den      den[NDENS];
};
#define POP             ((struct pop*) 0x8000)
typedef char pop_fits[sizeof (struct pop) <= 0x2000 ? 1 : -1];

static unsigned char kb, pb;                    /* Its banks: the kinds', the records' */
static unsigned char nkinds;
static unsigned long tdens;                     /* The kinds that live in the game's theme's dungeon (theme.h's: a bit
                                                **   each, by kinds.txt's order): only those are populated */
static struct head hd;                          /* (The records' head, kept in the main RAM) */
static struct dkind K;                          /* (A kind, a lair, a denizen: copied out) */
static struct lair L;
static struct den D;
static unsigned char piece[256];                /* (The files' pieces, through the main RAM) */

/* Each copied out (or in) of its bank, the hub's header in the window after */
static void __fastcall__ kind_get (unsigned char k)
{
    hy_bank (kb);
    memcpy (&K, &KINDS[k < nkinds ? k : 0], sizeof K);
    hub_map (0);
}

static void __fastcall__ lair_get (unsigned char i)
{
    hy_bank (pb);
    memcpy (&L, &POP->lair[i], sizeof L);
    hub_map (0);
}

static void __fastcall__ lair_put (unsigned char i)
{
    hy_bank (pb);
    memcpy (&POP->lair[i], &L, sizeof L);
    hub_map (0);
}

static void __fastcall__ den_get (unsigned char i)
{
    hy_bank (pb);
    memcpy (&D, &POP->den[i], sizeof D);
    hub_map (0);
}

static void __fastcall__ den_put (unsigned char i)
{
    hy_bank (pb);
    memcpy (&POP->den[i], &D, sizeof D);
    hub_map (0);
}

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* The kinds read in (dir/dq0d.dat): 0, or -1 */
int __fastcall__ s_kinds_load (const char* dir)
{
    static char path[64];
    struct faction* f;
    unsigned off;
    int fd, n;

    strcpy (path, dir);
    strcat (path, "/dq0d.dat");
    if ((fd = open (path, O_RDONLY)) < 0) {
        return -1;
    }
    for (off = 0; off < 0x2000 && (n = read (fd, piece, sizeof piece)) > 0; off += n) {
        hy_bank (kb);
        memcpy ((char*) 0x8000 + off, piece, n);
    }
    close (fd);
    hy_bank (kb);                               /* (The first record's the counts; the factions after the kinds) */
    nkinds = KINDS[0].flags;
    nfacts = KINDS[0].styles < NFACT ? KINDS[0].styles : NFACT;
    for (n = 0; n < nkinds && n < SND_KINDS; ++n) {
        kfact[n] = KINDS[n].faction;
    }
    for (n = 0; n < nfacts; ++n) {
        f = (struct faction*) (KINDS + nkinds) + n;
        memcpy (ftongue[n], f->tongue, sizeof ftongue[0]);
        fatt0[n] = n ? f->att : AT_INDIFFERENT;
        memcpy (frel[n], f->rel, NFACT);
    }
    hub_map (0);
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* How factions f and g stand (AT_*: indifferent, either none) */
#define REL(f, g)       frel[(f) & (NFACT - 1)][(g) & (NFACT - 1)]

/* ---- The character, as the hub has it (each time round) */

static unsigned long now;
static unsigned pcx, pcy;
static unsigned char pcs, pclit, pcbusy, pcdead;

unsigned __fastcall__ apart (int dx, int dy);  /* |dx| or |dy|, the more (apart.s: in assembly, asked of every
                                                **   square the minds weigh) */

/* ---- Populations: stratum n's decided as its plan is grown (its record and links whole: the hub's STRATUM (n),
** copied): for each kind that lives in its style at its depth, its lairs (by the stratum's ways, if it lairs there;
** else in a region of halls, a chunk of it), and the denizens in each, a record each */
#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* Stratum n's plan's region idx's kind: in its plan bank, or (a big plan: more regions than PLAN_MAX, the maker's
** paged to the save folder) in its file there, plSS0000.dat, its regions in order */
static unsigned char __fastcall__ plan_kind (unsigned char n, unsigned idx)
{
    static const char hex[] = "0123456789abcdef";
    struct stratum* s = &STRATUM (n);
    struct region r;
    char* p;
    int fd;

    r.kind = RG_ROCK;
    if ((unsigned) s->rw * s->rh <= PLAN_MAX) {
        hub_map (PLAN_BANK (n));
        r.kind = (PLAN_AT (n) + idx)->kind;
    } else {
        hub_map (0);
        strcpy ((char*) piece, HUB->savedir);
        p = (char*) piece + strlen ((char*) piece);
        if (n > 15) {
            *p++ = '/';
            *p++ = 'p';
            *p++ = 'l';
            *p++ = hex[n >> 4];
        } else {
            *p++ = '/';
            *p++ = 'p';
            *p++ = 'l';
        }
        *p++ = hex[n & 15];
        strcpy (p, "0000.dat");
        if ((fd = open ((char*) piece, O_RDONLY)) >= 0) {
            lseek (fd, (long) idx * sizeof r, SEEK_SET);
            read (fd, &r, sizeof r);
            close (fd);
        }
    }
    hub_map (0);
    return r.kind;
}

void __fastcall__ s_populate (unsigned char n)
{
    struct stratum* s = &STRATUM (n);
    unsigned char k, j, m, i, rx = 0, ry = 0, t, u;
    unsigned w = (s->w + CW - 1) / CW, h = (s->h + CW - 1) / CW;

    rseed (s->seed ^ 0x44454E5AUL);
    for (k = 1; k < nkinds; ++k) {
        kind_get (k);
        if (!(K.styles & (1 << s->style)) || n < K.deep0 || n > K.deep1 || !(tdens & 1UL << k)) {
            continue;
        }
        m = K.flags & K_WAYS ? K.lairs : random (K.lairs + 1);
        for (j = 0; j < m; ++j) {
            memset (&L, 0, sizeof L);
            L.sn = n;
            L.kind = k;
            L.link = 0xFF;
            if (K.flags & K_WAYS) {             /* (By the stratum's ways, the first ones: not the dungeon's way in,
                                                **   the top's first, where the character begins) */
                if ((u = j + (n == 0)) >= s->nlinks) {
                    break;
                }
                L.link = u;
                L.cx = (s->link[u].x - s->x0) / CW;
                L.cy = (s->link[u].y - s->y0) / CW;
            } else {
                for (t = 0; t < 8; ++t) {       /* (A region of halls, a chunk of it) */
                    rx = random (s->rw);
                    ry = random (s->rh);
                    if (plan_kind (n, (unsigned) ry * s->rw + rx) == RG_HALLS) {
                        break;
                    }
                }
                hub_map (0);
                if (t == 8) {
                    continue;
                }
                L.cx = (unsigned) rx * RCH + random (RCH);
                L.cy = (unsigned) ry * RCH + random (RCH);
                L.cx = L.cx < w ? L.cx : w - 1;
                L.cy = L.cy < h ? L.cy : h - 1;
            }
            L.n = K.nmin + random (K.nmax - K.nmin + 1);
            L.rx = (s->x0 + (L.cx << CW_BITS)) >> 8;
            L.ry = (s->y0 + (L.cy << CW_BITS)) >> 8;
            if (!reg_need (n, L.rx, L.ry) || (t = rec_new (1)) == 0xFF) {
                break;                          /* (Its region in the cache first: no room, no folder) */
            }
            L.id = ++hd.lids;
            memset (&D, 0, sizeof D);
            D.kind = k;
            D.lid = L.id;
            D.sn = D.lsn = n;
            D.lrx = L.rx;
            D.lry = L.ry;
            D.attitude = hd.fatt[K.faction & (NFACT - 1)];    /* (Its faction's, as the character stands with it) */
            for (i = 0; i < L.n && (u = rec_new (0)) != 0xFF; ++i) {
                D.id = ++hd.ids;
                den_put (u);
            }
            L.n = i;
            lair_put (t);
        }
    }
    hd.done[n >> 3] |= 1 << (n & 7);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The strata grown since the last look, populated (none while the world's asking the maker: a plan may be half
** copied into the world bank) */
static void grow (void)
{
    unsigned char n, m, k;

    for (k = 0; k < MAXSTRATA; ++k) {           /* (The hub's slots: each one's stratum, its own number) */
        hub_map (WORLD_BANK);
        m = WREC->strata[k].made;
        n = WREC->strata[k].n;
        hub_map (0);
        if (m && !DONE (n) && !hub_posted (&HUB->mreq[ASK_WORLD])) {
            sync_strata ();
            populate (n);
        }
    }
}

/* ---- Placing: lair i's denizens put on squares of their own, as the character comes near it.  By its link: round
** the link's square (not on it: the character comes out there); else in a room of its chunk (or, a chunk with none,
** about its middle).  Its own numbers (the stratum's seed and the lair's), so it's the same whenever it's done */

static unsigned char __fastcall__ rec_on (unsigned char sn, unsigned x, unsigned y);
static unsigned char __fastcall__ taken (unsigned char me, unsigned x, unsigned y);

static unsigned char __fastcall__ open_sq (unsigned char* p)
{
    return p && (TILE (*p) == T_FLOOR || TILE (*p) == T_CORR || TILE (*p) == T_STEPS);
}

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

void __fastcall__ s_place (unsigned char i)
{
    struct stratum* s;
    struct room* q;
    unsigned x0, y0, x, y, ox, oy;
    unsigned char w = 7, h = 7, j, t, k;

    lair_get (i);
    s = &STRATUM (L.sn);
    rseed (s->seed + (unsigned long) (L.id - 1) * 7919UL);     /* (By its number: wherever its record is) */
    ox = s->x0 + L.cx * CW;
    oy = s->y0 + L.cy * CW;
    if (L.link != 0xFF) {
        L.hx = s->link[L.link].x;
        L.hy = s->link[L.link].y;
        x0 = L.hx - 3;
        y0 = L.hy - 3;
    } else if (at (L.sn, ox, oy) && C->nrooms) {
        q = &C->room[random (C->nrooms)];
        x0 = ox + q->x;
        y0 = oy + q->y;
        w = q->w;
        h = q->h;
        L.hx = x0 + w / 2;
        L.hy = y0 + h / 2;
    } else {
        x0 = ox + 8;
        y0 = oy + 8;
        w = h = 16;
        L.hx = ox + 16;
        L.hy = oy + 16;
    }
    for (j = 0, k = 0; j < L.n; ++j, ++k) {
        for (; k < hd.ndens && (den_get (k), D.lid != L.id || !D.kind); ++k) {
        }                                       /* (Its next denizen, in its records' order) */
        for (t = 0; t < 32; ++t) {
            x = x0 + random (w);
            y = y0 + random (h);
            if (open_sq (at (L.sn, x, y)) && (L.link == 0xFF || apart (x - L.hx, y - L.hy) > 1) &&
                apart (x - pcx, y - pcy) > 1 && !rec_on (L.sn, x, y) && !taken (0xFF, x, y)) {
                break;                          /* (Not on another's, nor on the way or the character's square or
                                                **   beside them: the character comes out there, not boxed in) */
            }
        }
        if (k == hd.ndens) {
            break;
        }
        den_get (k);
        D.hx = L.hx;                            /* (Its home, its record's own: its lair's may be out of the cache) */
        D.hy = L.hy;
        if (t < 32) {
            D.x = x;
            D.y = y;
            D.flags |= D_OUT;
            D.face = random (4) * 2;
            D.ready = now + random (50);
        }
        den_put (k);
    }
    L.flags |= L_PLACED;
    lair_put (i);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* Chunk (cx, cy) of stratum sn made already (in the cache: its table the world bank's; the hub's header in the
** window after) */
static unsigned char __fastcall__ made (unsigned char sn, unsigned cx, unsigned cy)
{
    struct cslot* q = WREC->cslot;
    unsigned char k;

    hub_map (WORLD_BANK);
    for (k = NCSLOTS; k && !(q->used && q->sn == sn && q->cx == cx && q->cy == cy); --k, ++q) {
    }
    hub_map (0);
    return k != 0;
}

/* Square x, y of stratum sn in a chunk that's made already */
static unsigned char __fastcall__ made_at (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s = &STRATUM (sn);

    return x - s->x0 < s->w && y - s->y0 < s->h && made (sn, (x - s->x0) >> CW_BITS, (y - s->y0) >> CW_BITS);
}

/* The lairs on the character's stratum whose chunks are its own or beside it, and made (the character's
** surroundings detailed: this task never makes a chunk to place a lair), placed: 1 if any was */
static unsigned char nearby (void)
{
    struct stratum* s = &STRATUM (pcs);
    unsigned cx = (pcx - s->x0) / CW, cy = (pcy - s->y0) / CW;
    unsigned char i, n = 0;

    for (i = 0; i < hd.nlairs; ++i) {
        lair_get (i);
        if (L.kind && L.sn == pcs && !(L.flags & L_PLACED) && L.cx + 1 - cx <= 2 && L.cy + 1 - cy <= 2 &&
            made (L.sn, L.cx, L.cy)) {
            place (i);
            n = 1;
        }
    }
    return n;
}

/* ---- The actors: the denizens near the character, in the hub for the world and the senses */

#define ACT_R           24                      /* Squares from the character a denizen's an actor */
#define NEAR            16

static struct actor a;
static unsigned char akind[NACTORS];            /* Each actor's kind, and its lair's home */
static unsigned ahx[NACTORS], ahy[NACTORS];
static unsigned char turns;                     /* The turns pace */
static unsigned char nnear;                     /* The denizens out near the character but not actors (more than
                                                **   there are entries for): their squares, each time round */
static unsigned nearx[NEAR], neary[NEAR];
static unsigned char sense_gen = 1;            /* (Bumped as the character changes: where it is, its light, busy) */
static unsigned char sgen[NACTORS], sres[NACTORS];  /* (Each actor's senses as they were last: of sense_gen, there) */
static unsigned sax[NACTORS], say[NACTORS];
static unsigned long alert[NACTORS];            /* (Each actor still and facing a noise it heard till then) */
static unsigned char steps[NACTORS];            /* (Each one's steps: a noise of its own every eighth) */
static unsigned char fled[NACTORS];             /* (Each one fleeing the character: its turns of it left) */
static unsigned wt;                             /* (The wait an actor's decided on, tenths) */
static unsigned dsx, dsy;                       /* (The square a cornered one darts to) */

/* ---- The referee, asked in this task's own slot (DSLOT, in the hub's header): the request (rq, in the main RAM:
** its kind, who and whom here, the rest the caller's) copied in whole, posted, waited for (asleep on the doorbell),
** the reply copied out; the hub's header in the window after */
#define RQ_SIZE (offsetof (struct req, text) + DTEXT)  /* (Its text DTEXT long: no more) */
static unsigned char rqb[RQ_SIZE];
#define rq      (*(struct req*) rqb)

static void __fastcall__ ask (unsigned char kind, unsigned char actor, unsigned char other)
{
    rq.kind = kind;
    rq.actor = actor;
    rq.other = other;
    hub_map (0);
    memcpy (DSLOT, &rq, RQ_SIZE);
    hub_call (DSLOT, T_REFEREE);                /* (Posted once it's whole) */
    if (HUB->quit) {
        rq.result = 0xFF;                       /* (The game's ending: the referee may be gone) */
        memset (rq.x, 0, sizeof rq.x);
        return;
    }
    memcpy (&rq, DSLOT, RQ_SIZE);
    DSLOT->state = REQ_FREE;
}

/* ---- Its intents (hub.h's the denizens' intents: the minds', for their actors' bodies, which are the world's, as the
** character's is): each posted in the hub's di, a ring (written whole, then shown; a full one waited on), and those
** posted waited for (settle: the world rung, and ringing back as it's carried them out), each actor's entry the
** world's then; the hub's header in the window after.  A wait (I_WAIT) changes nothing another actor's decision
** reads, so it needn't be waited for: it goes with the next that is, or with the turn's end (I_DONE, which rings).
** The intent at hand im (its own fields the caller's) */
static struct msg im;
static unsigned char stirs;                     /* (An intent posted since the last settle that isn't a wait) */
static unsigned char rface = 0xFF;             /* (The way a roused one faces: 0xFF as it does) */
#define ROUSE_SET       0x8000                  /* (rouse's: free then, whenever it was) */

static void __fastcall__ post (unsigned char kind, unsigned char who)
{
    unsigned char h;

    im.kind = kind;
    im.who = who;
    while (hub_map (0), ((h = HUB->di_head) + 1) % DI_SIZE == HUB->di_tail && !HUB->quit) {
        hub_ring (T_WORLD);
        hub_sleep (1);
    }
    memcpy (&HUB->di[h], &im, sizeof im);
    HUB->di_head = (h + 1) % DI_SIZE;
    stirs |= kind != I_WAIT;
    if (kind == I_DONE) {
        hub_ring (T_WORLD);
    }
}

static void settle (void)
{
    stirs = 0;
    if (hub_map (0), HUB->di_tail != HUB->di_head) {
        hub_ring (T_WORLD);
        while (hub_map (0), HUB->di_tail != HUB->di_head && !HUB->quit) {
            hub_sleep (1);
        }
    }
}

/* Actor k's (the copy a's) intent: what it's doing, and the way it faces, with it */
static void __fastcall__ act (unsigned char kind, unsigned char k)
{
    im.x[0] = a.doing;
    im.x[1] = a.face;
    post (kind, k + 1);
}

/* Actor k roused: free w tenths from now at the latest (| ROUSE_SET: then, whenever it was), facing rface */
static void __fastcall__ rouse (unsigned char k, unsigned w)
{
    im.x[0] = 0xFF;
    im.x[1] = rface;
    rface = 0xFF;
    im.w1 = w & ~ROUSE_SET;
    im.w2 = w >> 15;
    post (I_ROUSE, k + 1);
}

static unsigned char aorder[NACTORS];           /* (Each one's request, an ally's: its record's while it's an actor) */
static unsigned lsx[NACTORS], lsy[NACTORS];     /* (Where each last sensed the character: an ally follows it there) */
static unsigned char intr[NACTORS];             /* (Each one's lair intruded on, not yet left: its attitude fallen) */
static unsigned char seenfoe[NACTORS];          /* (Each one's seen the character with an enemy of its faction's) */
static unsigned char anear[NACTORS];            /* (Each one beside the character as it last acted: an ally comes
                                                **   along as the character goes up or down) */
static unsigned refx, refy;                     /* (The square step_for's S_AWAY and S_TOWARD go by: the character's,
                                                **   a foe's, the one an ally follows to) */

/* The name the character gave denizen number id (its entry in the head's names; id 0: a free one), or NNAMES */
static unsigned char __fastcall__ name_of (unsigned id)
{
    unsigned char n;

    for (n = 0; n < NNAMES && hd.names[n].id != id; ++n) {
    }
    return n;
}

#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* Actor k's place, time, attitude and request written back to its record */
static void __fastcall__ write_back (unsigned char k)
{
    hub_map (0);
    memcpy (&a, &HUB->actor[k], sizeof a);
    den_get (a.den);
    D.sn = a.sn;
    D.x = a.x;
    D.y = a.y;
    D.face = a.face;
    D.ready = a.ready;
    D.attitude = a.attitude;
    D.order = aorder[k];
}

/* Actor k no longer one (far from the character): its record its own again, its body gone from the world's keeping
** (I_LEAVE), the entry free */
void __fastcall__ s_retire (unsigned char k)
{
    write_back (k);
    D.flags &= ~D_ACTOR;
    if (D.attitude == AT_ALLIED && D.order == OR_WAIT) {
        D.ready = now + 36000;                  /* (An ally left waiting: it waits an hour, then goes its own way) */
    }
    den_put (a.den);
    im.x[0] = im.x[1] = 0xFF;
    post (I_LEAVE, k + 1);
    settle ();
    HUB->actor[k].name[0] = 0;
}

/* Denizen i made an actor, in free entry k (its time from now: the coarse moves have kept it while it wasn't one):
** its mind's fields written, its body given the world (I_ENTER: its place, its step, its time, bleeding or not); its
** sheet the referee's (RS_NEW_ACTOR, by its kind's key: made the first time, the same one after) */
void __fastcall__ s_enlist (unsigned char i, unsigned char k)
{
    struct rs_summary* s = (struct rs_summary*) rq.x;

    D.flags |= D_ACTOR;
    den_put (i);
    kind_get (D.kind);
    akind[k] = D.kind;
    ahx[k] = D.hx;
    ahy[k] = D.hy;
    memset (&a, 0, sizeof a);
    a.den = i;
    a.sn = D.sn;
    a.face = D.face;
    a.x = D.x;
    a.y = D.y;
    a.ready = D.ready > now ? D.ready : now;
    a.step = K.step;
    a.attitude = D.attitude;
    a.faction = K.faction;
    aorder[k] = D.order;
    sgen[k] = 0;
    alert[k] = 0;
    intr[k] = seenfoe[k] = anear[k] = 0;
    lsx[k] = lsy[k] = 0;
    fled[k] = 0;
    strcpy (rq.text, K.key);
    rq.w1 = i;
    ask (RS_NEW_ACTOR, k + 1, 0);
    im.b = (s->cond & C_BLEEDING) != 0;
    strcpy (a.name, (i = name_of (D.id)) < NNAMES ? hd.names[i].name : K.name);
    a.kind = D.kind;
    a.remains = K.flags & K_BONES ? KI_BONES : KI_BODY;
    a.timid = (K.flags & K_SHY) != 0;
    hub_map (0);
    memcpy (HUB->actor[k].name, a.name, sizeof a - offsetof (struct actor, name));
    im.x[0] = 0;
    im.x[1] = a.face;
    im.x[2] = a.sn;
    im.w1 = a.x;
    im.w2 = a.y;
    im.a = a.step;
    im.t = a.ready;
    post (I_ENTER, k + 1);
    settle ();
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The denizens out near the character made actors, as there are entries free; those left over noted (nearx ...) */
static void enlist_near (void)
{
    struct den* q = POP->den;
    unsigned char i, k = 0;

    nnear = 0;
    for (i = 0; i < hd.ndens; ++i, ++q) {
        hy_bank (pb);                           /* (Looked at in its bank; copied out only if it's near) */
        if ((q->flags & (D_OUT | D_ACTOR)) != D_OUT || q->sn != pcs || apart (q->x - pcx, q->y - pcy) > ACT_R + 16) {
            continue;
        }
        hub_map (0);
        while (k < NACTORS && HUB->actor[k].name[0]) {
            ++k;
        }
        den_get (i);
        if (k < NACTORS && apart (D.x - pcx, D.y - pcy) <= ACT_R) {
            enlist (i, k);
        } else if (nnear < NEAR) {
            nearx[nnear] = D.x;
            neary[nnear++] = D.y;
        }
    }
    hub_map (0);
}

/* Anyone but actor me on square x, y: the character (or where it's going), another actor, a denizen out near it (the
** hub's header in the window after) */
static unsigned char __fastcall__ taken (unsigned char me, unsigned x, unsigned y)
{
    struct actor* q = HUB->actor;
    unsigned char k;

    hub_map (0);
    if ((x == pcx && y == pcy) || (x == HUB->pc_tx && y == HUB->pc_ty)) {
        return 1;
    }
    for (k = 0; k < NACTORS; ++k, ++q) {
        if (k != me && q->name[0] && q->x == x && q->y == y) {
            return 1;
        }
    }
    for (k = 0; k < nnear; ++k) {
        if (nearx[k] == x && neary[k] == y) {
            return 1;
        }
    }
    return 0;
}

/* Its senses: whether actor k (the copy a, its kind K's) senses the character: the senses asked (the hub's dsense:
** DUNGEON.md's perception as layers, the senses answering for every actor): seen by the light layer and its eyes,
** heard, as they find it; and how far it may go with the answer standing (slk: a far one's, the coarse answer) */
static unsigned char slk[NACTORS];
static unsigned char __fastcall__ senses (unsigned char k)
{
    hub_map (0);
    HUB->dsense.who = k;
    HUB->dsense.sight = K.sight;
    HUB->dsense.dark = K.dark;
    HUB->dsense.hear = K.hear;
    HUB->dsense.eyes = K.eyes;
    hub_call (&HUB->dsense, T_SENSES);
    HUB->dsense.state = REQ_FREE;
    slk[k] = HUB->dsense.slack;
    return (HUB->dsense.res & (SN_SEEN | SN_HEARD)) != 0;
}

/* A step for actor k (the copy): a way it can walk to a square no one's on, within lim squares of its home; how:
** S_ANY (the way it faces, or another), S_AWAY (only one that takes it further from refx, refy (the character, or a
** foe): the furthest; close by it, cornered, one no nearer: a way round it, out), S_TOWARD (only one that takes it
** nearer, by the squares across and down together, so a step that's no nearer as the crow flies still counts: the
** nearest).  Its direction, or 0xFF */
enum { S_ANY, S_AWAY, S_TOWARD };
static unsigned __fastcall__ toward (int dx, int dy)
{
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

static unsigned char __fastcall__ step_for (unsigned char k, unsigned lim, unsigned char how)
{
    unsigned char d, j, best = 0xFF, s = random (2) ? a.face >> 1 : random (4);
    unsigned nx, ny, f, bf = how == S_TOWARD ? toward (a.x - refx, a.y - refy) : apart (a.x - refx, a.y - refy);

    for (j = 0; j < 4; ++j) {
        d = (s + j) & 3;
        nx = a.x + dir_x[d];
        ny = a.y + dir_y[d];
        f = how == S_TOWARD ? toward (nx - refx, ny - refy) : apart (nx - refx, ny - refy);   /* (The walk's own
                                                **   test last: it's the dearest) */
        if (apart (nx - ahx[k], ny - ahy[k]) > lim || (how == S_AWAY && (f < bf || (f == bf && bf > 2))) ||
            (how == S_TOWARD && f >= bf) || taken (k, nx, ny) || !step_ok (a.sn, a.x, a.y, d)) {
            continue;
        }
        if (how == S_ANY) {
            return d;
        }
        bf = f;
        best = d;
    }
    return best;
}

/* Actor a (the copy), out of the character's sight and not sensing it, moved coarsely (the turns pace's, catching
** up): a few squares' wander at a time, each a step of no time (I_STEP's STEP_COARSE: the world's), a while for them
** after (stopped as it comes into the character's sight) */
static void __fastcall__ coarse (unsigned char k, unsigned lim)
{
    unsigned char t, d;
    unsigned char* p;
    unsigned x, y;

    for (t = 0; t < 4; ++t) {
        d = random (4);
        x = a.x + dir_x[d];
        y = a.y + dir_y[d];
        if (apart (x - ahx[k], y - ahy[k]) > lim || !step_ok (a.sn, a.x, a.y, d) || taken (k, x, y)) {
            continue;
        }
        a.x = x;
        a.y = y;
        a.face = H_OF (d);
        im.w1 = H_OF (d) | STEP_COARSE;
        im.w2 = 1;
        act (I_STEP, k);
        if ((p = at (a.sn, x, y)) != 0 && (*p & T_VIS)) {
            break;
        }
    }
    wt = 60 + random (60);
}

#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* Actor a (the copy), shy and cornered beside the character: it darts past it, to the square beyond (open, no one
** on it: dsx, dsy), a moment's dash; 1 if it can */
unsigned char __fastcall__ s_dart (unsigned char k)
{
    dsx = pcx + (pcx - a.x);
    dsy = pcy + (pcy - a.y);
    return open_sq (at (a.sn, dsx, dsy)) && !taken (k, dsx, dsy);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Its events (DENIZENS.md: events): what its actors do, the world's to tell as it does it (the hub's denizens'
** events: nobody waits for them, and each perceives them or not by its own senses); and the noises they hear, their
** own and the world's (the character's: digging), each by every actor within its kind's hearing times the noise's
** loudness over two: it turns to face the noise, stands a while, and looks for the character again at once */

static unsigned wplace, dplace, fplace;         /* (Its places in the world's events, and in the denizens': the noises',
                                                **   the blows') */

static void hear (void)
{
    static struct msg m;
    static struct actor* q;
    static unsigned char k, src, sn, loud;

    for (;;) {
        hub_map (0);
        if (hub_next (&wplace, &m)) {           /* (The world's: a noise of the character's, on its stratum) */
            if (m.kind != E_NOISE) {
                continue;
            }
            src = 0;
            sn = pcs;
            loud = m.a;
        } else if (hub_dnext (&dplace, &m)) {   /* (The denizens': an actor's, not heard by itself) */
            if (m.kind != E_DEN || !m.b) {
                continue;
            }
            src = m.who;
            sn = m.x[0];
            loud = m.b;
        } else {
            break;
        }
        for (k = 0; k < NACTORS; ++k) {
            hub_map (0);
            q = &HUB->actor[k];
            if (!q->name[0] || k + 1 == src || q->sn != sn || (m.w1 == q->x && m.w2 == q->y) ||
                q->doing >= DO_FIGHT) {
                continue;                       /* (Not those fighting, fleeing, or senseless: nothing turns them) */
            }
            kind_get (akind[k]);
            if (apart (q->x - m.w1, q->y - m.w2) > K.hear * loud / 2) {
                continue;
            }
            rface = bearing (m.w1 - q->x, m.w2 - q->y);
            alert[k] = now + 100;
            sgen[k] = 0;                        /* (It senses again: the character, if it's there to be sensed) */
            rouse (k, 1);
        }
    }
    settle ();
}

/* ---- Fights (the referee's, RS_ATTACK: HarnMaster's melee): the world's to strike, every blow.  A hostile actor
** beside the character, sensing it, strikes at it a round at a time (its intent, I_ATTACK: the referee's round), as
** the character strikes at one (the world's own, I_ATTACK: the hub's pc_foe); each blow told (E_FIGHT, in the
** denizens' events: its words in their ring, FIGHT_AT, the world's, for the UI; played, the sound task's) and what it
** left the body in kept (senseless, fleeing, holding back, bleeding, dead: its remains the world's to put down); and
** here taken to heart (fought): one broken flees a while (or a shy one shaken), and fights after only if it's
** pressed; its kin, as one falls, test their courage too (dismay); one the character struck at turns on it
** (harmed), unless it flees; one dead, its record so, out of the dungeon for good */

#define FLEE_T          24                      /* Turns a broken one flees for (a step each, or a moment) */

#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* Actor k dead (the hub's entry as it is: its body the world's, gone): its record so, the entry free */
void __fastcall__ s_killed (unsigned char k)
{
    write_back (k);
    D.flags = (D.flags & ~(D_ACTOR | D_OUT)) | D_DEAD;
    den_put (a.den);
    HUB->actor[k].name[0] = 0;
}

/* Actor k fallen (dead, senseless): its kin among the actors test their courage (RS_MORALE, a bad situation: the book's
** "seeing a friend slain", Combat 18), and those it breaks (or shakes, a shy kind) flee */
void __fastcall__ s_dismay (unsigned char k)
{
    unsigned char j;

    for (j = 0; j < NACTORS; ++j) {
        if (j != k && akind[j] == akind[k] && !fled[j]) {
            rq.steps = -2;
            ask (RS_MORALE, j + 1, 0);
            if (rq.result == M_BROKEN || (rq.result == M_CAUTIOUS && (K.flags & K_SHY))) {
                fled[j] = FLEE_T;               /* (Its doing and its attitude its next turn's: fleeing) */
            }
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The blows the world's struck since the last look (E_FIGHT, the denizens' events), taken to heart, in turn: 1 if
** there were any */
static unsigned char fought (void)
{
    static struct msg m;
    unsigned char k, f, att, n = 0;

    while (hub_dnext (&fplace, &m)) {
        if (m.kind != E_FIGHT) {
            continue;
        }
        n = 1;
        f = m.x[2];
        att = m.who;
        k = ((att ? att : m.a) - 1) & 7;        /* (The actor in it: the character fights them one at a time) */
        if (m.x[1] != FO_BLED) {
            if (f & FF_FLEES) {
                fled[k] = FLEE_T;               /* (Broken: it flees, and won't seek the fight again) */
                HUB->actor[k].attitude = AT_UNFRIENDLY;
            }
            if (f & (att ? FF_A_DEAD | FF_A_DOWN : FF_D_DEAD | FF_D_DOWN)) {
                kind_get (akind[k]);
                dismay (k);
            }
            if (!att) {
                harmed (k);                     /* (Its own, and their foes, think again of the character) */
            }
        }
        if (f & (att ? FF_A_DEAD : FF_D_DEAD)) {
            killed (k);
        }
    }
    return n;
}

/* ---- Attitudes (DENIZENS.md): each denizen's to the character (hub.h's AT_*), its faction's to begin with, moved by
** what happens: its lair intruded on (a kind that keeps to it: the character beside its home), the character seen
** with an ally of a faction hostile to its own, harm done to it or its own (harmed), words (sway); and what it means
** to do by it.  A hostile one comes at the character (and fights it: the fights'); an unfriendly one keeps a few
** squares off, a bold one coming that near; a wary one watches (a shy one keeps its distance, as an indifferent one
** does); a friendly one doesn't shy from it; an ally follows it (by its own senses: to where it last sensed it),
** waits where it's asked, comes along up and down with it.  Denizens of factions hostile to one another keep off
** one another (the actor's foe: the fights' to fight, when they come) */

static int ddx, ddy;                            /* (The character from the actor deciding, and how far) */
static unsigned dist;

/* Actor k's (the copy's) attitude moved by what it senses of the character (it senses it now) */
static void __fastcall__ feel (unsigned char k)
{
    struct actor* q = HUB->actor;
    unsigned char j;

    if ((K.flags & K_LAIR) && apart (pcx - ahx[k], pcy - ahy[k]) <= 2) {
        if (!intr[k] && a.attitude > AT_HOSTILE && a.attitude < AT_FRIENDLY) {
            --a.attitude;                       /* (Its lair intruded on) */
        }
        intr[k] = 1;
    } else if (apart (pcx - ahx[k], pcy - ahy[k]) > 4) {
        intr[k] = 0;
    }
    hub_map (0);
    for (j = 0; j < NACTORS && !seenfoe[k] && a.attitude > AT_HOSTILE && a.attitude < AT_ALLIED; ++j, ++q) {
        if (j != k && q->name[0] && q->attitude == AT_ALLIED && q->sn == pcs && apart (q->x - pcx, q->y - pcy) <= 3 &&
            REL (a.faction, q->faction) == AT_HOSTILE) {
            seenfoe[k] = 1;                     /* (Seen with an enemy: once) */
            --a.attitude;
        }
    }
}

/* Actor k (the copy) near an actor of a faction hostile to its own (its foe: the nearest, within 4 squares): kept off
** it, a step away while it's within 3.  The step, or 0xFF */
static unsigned char __fastcall__ keep_off (unsigned char k)
{
    struct actor* q = HUB->actor;
    unsigned char j, d = 0xFF;
    unsigned f, best = 5;

    hub_map (0);
    a.foe = 0;
    for (j = 0; j < NACTORS; ++j, ++q) {
        if (j != k && q->name[0] && q->sn == a.sn && REL (a.faction, q->faction) == AT_HOSTILE &&
            (f = apart (q->x - a.x, q->y - a.y)) < best) {
            best = f;
            a.foe = j + 1;
            refx = q->x;
            refy = q->y;
        }
    }
    if (a.foe && best <= 3) {
        d = step_for (k, a.attitude == AT_ALLIED ? 0xFFFF : 2 * K.range, S_AWAY);
    }
    refx = pcx;
    refy = pcy;
    return d;
}

/* An ally's (actor k's, the copy's) turn: waiting where it was asked; else following the character, to where it last
** sensed it, till it's beside it.  The step, or 0xFE (its wait, wt) */
static unsigned char __fastcall__ ally (unsigned char k)
{
    unsigned char d = 0xFE;

    if (sres[k]) {
        a.face = bearing (ddx, ddy);
    }
    if (aorder[k] == OR_WAIT) {
        a.doing = DO_WAIT;
        wt = 20 + random (20);
        return d;
    }
    a.doing = DO_FOLLOW;
    if ((lsx[k] || lsy[k]) && apart (a.x - lsx[k], a.y - lsy[k]) > 1) {
        refx = lsx[k];
        refy = lsy[k];
        d = step_for (k, 0xFFFF, S_TOWARD);
        refx = pcx;
        refy = pcy;
    }
    if (d >= 4) {
        wt = 5 + random (10);
        d = 0xFE;
    }
    return d;
}

/* What actor k (the copy) does as it senses the character, by its attitude and its kind.  The step, 0xFD (darting
** past it: dart's square), or 0xFF (none) */
static unsigned char __fastcall__ meets (unsigned char k, unsigned char was)
{
    unsigned char d = 0xFF, t = a.attitude;

    a.face = bearing (ddx, ddy);
    a.doing = DO_WATCH;
    if (t == AT_HOSTILE || ((K.flags & K_BOLD) && t <= AT_INDIFFERENT)) {
        if (dist > (t == AT_HOSTILE ? 1 : 3) &&
            (d = step_for (k, 2 * K.range + (t == AT_HOSTILE ? 8 : 0), S_TOWARD)) != 0xFF) {
            a.doing = DO_COMING;                /* (At it, or to threaten it) */
        }
    } else if ((K.flags & K_SHY) && t <= AT_INDIFFERENT && dist < (was == DO_SHY ? 7 : 5)) {
        a.doing = DO_SHY;
        if ((d = step_for (k, 2 * K.range, S_AWAY)) == 0xFF && dist == 1 && dart (k)) {
            d = 0xFD;                           /* (Cornered: past the character) */
        }
    } else if (t == AT_UNFRIENDLY && dist < 3) {
        a.doing = DO_SHY;                       /* (A few squares off, threatening) */
        d = step_for (k, 2 * K.range, S_AWAY);
    }
    return d;
}

/* What actor k means to do, by its kind, its attitude and what it senses, said (its intent: the world does it, by the
** character's rules, or strikes its blow) and waited for, and the blows struck taken to heart (fought): kept off a
** foe; an ally's way; as it senses the character, meets's (a foe beside it: a blow); a noise heard, still a while;
** else it wanders about its lair (or keeps to it).  1 if all it said was a wait, not waited for (a.ready the time it
** will be free, as the world will have it) */
static unsigned char __fastcall__ decide (unsigned char k)
{
    unsigned char d = 0xFF, was, foe;

    kind_get (akind[k]);
    memcpy (&a, &HUB->actor[k], sizeof a);
    wt = 0;
    refx = pcx;
    refy = pcy;
    was = a.doing;
    ddx = pcx - a.x;
    ddy = pcy - a.y;
    dist = apart (ddx, ddy);
    a.doing = DO_ROAM;
    if (sgen[k] != sense_gen || apart (a.x - sax[k], a.y - say[k]) > slk[k]) {
        sgen[k] = sense_gen;                    /* (Sensed again only as the character has changed, or it's gone
                                                **   further than the senses' answer stands for) */
        sax[k] = a.x;
        say[k] = a.y;
        sres[k] = senses (k);
    }
    foe = !pcdead && (a.attitude == AT_HOSTILE || (a.attitude == AT_UNFRIENDLY && dist <= 1));
    if (sres[k]) {
        lsx[k] = pcx;
        lsy[k] = pcy;
        feel (k);
    }
    anear[k] = a.sn == pcs && dist <= 3;
    if (fled[k]) {
        --fled[k];
        a.doing = DO_FLEE;                      /* (Fleeing: away; cornered beside the character, it fights) */
        if ((d = step_for (k, 4 * K.range + 8, S_AWAY)) == 0xFF && dist == 1 && sres[k] && !pcdead) {
            d = 0xFC;
        }
    } else if (sres[k] && foe && dist == 1) {
        a.face = bearing (ddx, ddy);            /* (A foe beside it: it strikes) */
        d = 0xFC;
    } else if ((d = keep_off (k)) != 0xFF) {
        a.doing = sres[k] ? DO_WATCH : DO_ROAM;
    } else if (a.attitude == AT_ALLIED) {
        d = ally (k);
    } else if (sres[k]) {
        d = meets (k, was);
    } else if (alert[k] > now) {
        d = 0xFE;                               /* (A noise heard: still, facing the way it came, a while) */
        wt = 10 + random (20);
    } else if (turns && !(*at (a.sn, a.x, a.y) & T_VIS)) {
        coarse (k, K.flags & K_LAIR ? 2 : K.range);
        d = 0xFE;                               /* (Unseen, unaware: a coarse move) */
    } else if (K.flags & K_LAIR ? !random (4) : random (3)) {
        d = step_for (k, K.flags & K_LAIR ? 2 : K.range, S_ANY);
    }
    if (d == 0xFF) {                            /* (Watching: a while, unless it changes; fleeing, a moment) */
        wt = a.doing == DO_FLEE ? 10 + random (20) : a.doing ? 60 + random (60) : 10 + random (40);
    }
    hub_map (0);
    HUB->actor[k].attitude = a.attitude;        /* (Its mind's own, as it decides) */
    HUB->actor[k].foe = a.foe;
    if (d < 4) {                                /* (A step, now and then a noise of its own) */
        im.w1 = H_OF (d);
        im.w2 = 1;
        im.a = ++steps[k] & 7 ? DE_MOVED : DE_NOISE;
        im.b = steps[k] & 7 ? 1 : 3;
        act (I_STEP, k);
    } else if (d == 0xFD) {                     /* (A dash past the character, louder) */
        im.w1 = dsx;
        im.w2 = dsy;
        im.a = DE_MOVED;
        im.b = 2;
        act (I_GOTO, k);
    } else if (d == 0xFC) {                     /* (A round's blow: outnumbering it, later) */
        a.doing = DO_FIGHT;
        im.a = 0;
        im.b = Z_MID;
        act (I_ATTACK, k);
    }
    if (wt) {
        im.w1 = wt;
        act (I_WAIT, k);
    }
    if (!stirs) {
        a.ready += wt;
        return 1;
    }
    settle ();
    fought ();
    return 0;
}

/* ---- The denizens away from the character (out, but not actors): moved coarsely, a while at a time, each to a
** square of its range about its lair (in a chunk that's made: never made for them), so the dungeon goes on where
** the character isn't */

#define ROAM_T          300                     /* Tenths between their moves, at least (and as much again) */

/* A record (out, not an actor) on square x, y of stratum sn */
static unsigned char __fastcall__ rec_on (unsigned char sn, unsigned x, unsigned y)
{
    struct den* q = POP->den;
    unsigned char i, f = 0;

    hy_bank (pb);
    for (i = 0; i < hd.ndens; ++i, ++q) {
        if ((q->flags & (D_OUT | D_ACTOR)) == D_OUT && q->x == x && q->y == y && q->sn == sn) {
            f = 1;
            break;
        }
    }
    hub_map (0);
    return f;
}

/* Those due moved: 1 if any was */
static unsigned long roam_at;                   /* (The soonest one's due) */

static unsigned char roam (void)
{
    struct den* q = POP->den;
    unsigned char i, lim, n = 0;
    unsigned x, y;

    if (now < roam_at) {
        return 0;
    }
    roam_at = now + ROAM_T;
    for (i = 0; i < hd.ndens; ++i, ++q) {
        hy_bank (pb);
        if ((q->flags & (D_OUT | D_ACTOR)) != D_OUT) {
            continue;
        }
        if (q->ready > now) {
            if (q->ready < roam_at) {
                roam_at = q->ready;
            }
            continue;
        }
        den_get (i);
        kind_get (D.kind);
        if (D.attitude == AT_ALLIED && D.order == OR_WAIT) {
            D.order = OR_NONE;                  /* (Left waiting an hour: it goes its own way, an ally no more) */
            D.attitude = AT_FRIENDLY;
        }
        lim = D.sn == D.lsn ? (K.flags & K_LAIR ? 2 : K.range) : 0;  /* (Come along with the character off its
                                                **   lair's stratum: it stays where it is) */
        x = D.hx + random (2 * lim + 1) - lim;
        y = D.hy + random (2 * lim + 1) - lim;
        if (lim && made_at (D.sn, x, y) && open_sq (at (D.sn, x, y)) && !taken (0xFF, x, y) && !rec_on (D.sn, x, y)) {
            D.x = x;
            D.y = y;
            D.face = random (4) * 2;
            n |= D.sn == pcs && apart (x - pcx, y - pcy) <= ACT_R + 16;    /* (Near the character now) */
        }
        D.ready = now + ROAM_T + random (ROAM_T);
        if (D.ready < roam_at) {
            roam_at = D.ready;
        }
        den_put (i);
    }
    hub_map (0);
    return n;
}

#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* Ally k come along with the character, up or down: on a square beside it, open, no one's on it (its intent,
** I_CLIMB: the world puts it there).  1 if it is */
unsigned char __fastcall__ s_along (unsigned char k)
{
    unsigned char h;
    unsigned x, y;

    for (h = 0; h < 8; ++h) {
        x = pcx + head_x[h];
        y = pcy + head_y[h];
        if (open_sq (at (pcs, x, y)) && !taken (k, x, y)) {
            im.x[0] = im.x[1] = 0xFF;
            im.x[2] = pcs;
            im.w1 = x;
            im.w2 = y;
            post (I_CLIMB, k + 1);
            settle ();
            sgen[k] = 0;
            return HUB->actor[k].sn == pcs;
        }
    }
    hub_map (0);
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* Each actor tended: those far from the character retired, those free to act deciding (at the turns pace, each
** caught up with the clock: its intents carried out, the world waiting for them); the denizens near it placed (for a
** while after it moves: the chunks round it made as it looks about) and enlisted; those away from it moved,
** coarsely.  Nothing, if nothing's changed (the clock, the character, a blow) */
static unsigned char struck;                    /* (Blows taken to heart since: fought's) */

static unsigned char lrs = 0xFF;                /* (The region the character's in, as the cache last had those round
                                                **   it) */
static unsigned lrx, lry;
static void tend (void)
{
    static unsigned lx, ly;
    static unsigned long lnow;
    static unsigned char ls = 0xFF, fresh, lgen;
    struct actor* q;
    unsigned long r;
    unsigned char k, n, again = struck, woke = lgen != sense_gen;

    lgen = sense_gen;
    struck = 0;
    if (pcx != lx || pcy != ly || pcs != ls) {
        lx = pcx;
        ly = pcy;
        ls = pcs;
        again = 1;
        fresh = 10;
    }
    if (fresh) {
        --fresh;
        again |= nearby ();
    }
    if (!again && !woke && now == lnow) {
        return;
    }
    if (now != lnow) {
        again |= roam ();
    }
    lnow = now;
    for (k = 0, q = HUB->actor; k < NACTORS; ++k, ++q) {
        hub_map (0);
        if (!q->name[0]) {
            continue;
        }
        if (woke && q->doing && q->doing < DO_FIGHT && q->ready > now + 10) {
            rouse (k, 1 + random (10));         /* (The character's changed: a watcher sees to it soon; not one
                                                **   fighting, fleeing or senseless: its own time) */
            settle ();
        }
        if (q->sn != pcs && anear[k] && q->attitude == AT_ALLIED && aorder[k] != OR_WAIT && !along (k) && fresh) {
            continue;                           /* (An ally beside the character as it went up or down: with it,
                                                **   as soon as there's a square beside it) */
        }
        if (q->sn != pcs || apart (q->x - pcx, q->y - pcy) > ACT_R + 8) {
            retire (k);
            again = 1;
            continue;
        }
        for (n = 0, r = q->ready; r <= now && q->name[0] && n < (turns ? 64 : 1); ++n) {
            if (decide (k) && (r = a.ready) > now) {
                break;                          /* (A wait only, the world's to carry out with the rest) */
            }
            settle ();                          /* (Its record the world's, as it is now) */
            r = q->ready;
        }
    }
    if (again) {
        if (pcs != lrs || (pcx - STRATUM (pcs).x0) >> 8 != lrx || (pcy - STRATUM (pcs).y0) >> 8 != lry) {
            lrs = pcs;                          /* (Come into a region: those round it wanted, in the cache, the
                                                **   actors far from it retired first) */
            lrx = (pcx - STRATUM (pcs).x0) >> 8;
            lry = (pcy - STRATUM (pcs).y0) >> 8;
            dneed ();
        }
        enlist_near ();
    }
}

/* ---- The character's ways with a denizen (the UI's request, the hub's sway: hub.h): the one meant found among those
** it sees (or that are near), by a word of its name or its kind's; and words to sway it (the referee's test, in the
** UI's slot, lent while the UI waits), a request to an ally, a name given, a look at how it seems, a blow (harmed) */

static struct sway sw;

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* Whether name n (an actor's, or a kind's) is meant by w (the parser's word, in lower case): the start of one of its
** words ("rat", "giant", "grub") */
static unsigned char __fastcall__ called (const char* n, const char* w)
{
    static char b[12];
    unsigned char i, m = strlen (w);

    for (i = 0; i < sizeof b - 1 && n[i]; ++i) {
        b[i] = n[i] >= 'A' && n[i] <= 'Z' ? n[i] + 'a' - 'A' : n[i];
    }
    b[i] = 0;
    for (i = 0; b[i]; ++i) {
        if ((!i || b[i - 1] == ' ') && !strncmp (b + i, w, m)) {
            return 1;
        }
    }
    return 0;
}

/* The actor meant by w ("": any): the nearest of them on the character's stratum, on a square it sees or within four
** of it; its distance in dist.  The actor, or 0xFF */
static unsigned char __fastcall__ meant (const char* w)
{
    struct actor* q;
    unsigned char k, best = 0xFF;
    unsigned char* p;
    unsigned f, x, y;

    dist = 0xFFFF;
    for (k = 0; k < NACTORS; ++k) {
        hub_map (0);
        q = &HUB->actor[k];
        if (!q->name[0] || q->sn != pcs || (f = apart (q->x - pcx, q->y - pcy)) >= dist ||
            (*w && !called (q->name, w) && (kind_get (akind[k]), !called (K.name, w)))) {
            continue;
        }
        x = q->x;
        y = q->y;
        if (f > 4 && ((p = at (pcs, x, y)) == 0 || !(*p & T_VIS))) {
            continue;
        }
        dist = f;
        best = k;
    }
    hub_map (0);
    return best;
}

/* Actor k harmed by the character (a blow: the fights', and the stand-in's): it's hostile, an ally no more; its own
** turn against it, its lair's two steps, the rest of its faction one (and the faction's newcomers); those of factions
** hostile to its own that sense it think the better of it, a step (to friendly at most) */
void __fastcall__ s_harmed (unsigned char k)
{
    struct actor* q = HUB->actor;
    struct den* r = POP->den;
    unsigned char j, f, n;
    unsigned l;

    hub_map (0);
    f = q[k].faction;
    den_get (q[k].den);
    l = D.lid;
    for (j = 0; j < NACTORS; ++j, ++q) {
        if (!q->name[0]) {
            continue;
        }
        if (j == k) {
            q->attitude = q->doing == DO_FLEE ? AT_UNFRIENDLY : AT_HOSTILE;    /* (Fled from it: it won't seek it) */
        } else if (q->faction == f) {
            den_get (q->den);
            n = D.lid == l ? 2 : 1;
            q->attitude = q->attitude > n ? q->attitude - n : AT_HOSTILE;
        } else if (REL (f, q->faction) == AT_HOSTILE && sres[j] && q->attitude < AT_FRIENDLY) {
            ++q->attitude;
        } else {
            continue;
        }
        if (q->attitude < AT_ALLIED) {
            aorder[j] = OR_NONE;
        }
        if (q->doing < DO_FIGHT && q->ready > now) {
            rouse (j, 0);                       /* (At once: not one fighting, its own time; not put off) */
        }
        sgen[j] = 0;
    }
    settle ();
    for (j = 0; j < hd.ndens; ++j, ++r) {
        hy_bank (pb);
        if (r->kind && !(r->flags & D_ACTOR) && kfact[r->kind % SND_KINDS] == f) {
            n = r->lid == l ? 2 : 1;
            r->attitude = r->attitude > n ? r->attitude - n : AT_HOSTILE;
            if (r->attitude < AT_ALLIED) {
                r->order = OR_NONE;
            }
        }
    }
    hub_map (0);
    if (hd.fatt[f & (NFACT - 1)] > AT_HOSTILE) {
        --hd.fatt[f & (NFACT - 1)];
    }
}

/* Words to sway actor k (sw's how): by its mind (nothing sways the mindless; an animal only by Animalcraft, within
** reach), in its faction's tongue (but a threat), a test of the referee's (P_PERSUADE, P_DECEIVE, P_INTIMIDATE,
** P_ANIMALS) at steps by its attitude, its kind and its mind; the attitude moved by the grade, as far as that way of
** swaying goes (a critical success two steps, a success one, a critical failure a step back: a lie found out two, a
** threat laughed at hostile) */
static const signed char att_steps[AT_ALLIED + 1] = { -3, -2, -1, 0, 1, 2 };
static const unsigned char cap[SW_BEFRIEND + 1] = {
    0, AT_FRIENDLY, AT_ALLIED, AT_FRIENDLY, AT_WARY, AT_INDIFFERENT, AT_ALLIED
};

static void __fastcall__ influence (unsigned char k)
{
    struct req* r;
    signed char m, st;
    unsigned char t = sw.was, h = sw.kind, p;

    if (K.mind == M_MINDLESS) {
        sw.result = SR_MINDLESS;
        return;
    }
    if (sw.beast && h <= SW_DECEIVE) {
        sw.result = SR_BEAST;
        return;
    }
    if (dist > (sw.beast ? 8 : 12)) {
        sw.result = SR_FAR;
        return;
    }
    if (t >= cap[h]) {
        sw.result = SR_AS_FAR;
        return;
    }
    st = att_steps[t] + K.sway;
    p = sw.beast ? P_ANIMALS : h == SW_THREATEN ? P_INTIMIDATE : h == SW_DECEIVE ? P_DECEIVE : P_PERSUADE;
    if (h == SW_DECEIVE) {
        st += K.mind == M_LOW ? 2 : K.mind == M_CUNNING ? -2 : 0;
    } else if (!sw.beast && h != SW_THREATEN && K.mind == M_LOW) {
        --st;                                   /* (Hard to reason with) */
    }
    hub_map (0);
    r = DSLOT;                                  /* (This task's own slot: DTEXT of text, a tongue's name) */
    r->text[0] = 0;
    if (!sw.beast && h != SW_THREATEN) {        /* (Words: in its tongue, if it has one) */
        if (!ftongue[K.faction & (NFACT - 1)][0]) {
            sw.result = SR_TONGUE;
            return;
        }
        strcpy (r->text, ftongue[K.faction & (NFACT - 1)]);
    }
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = p;
    r->b = 0;
    r->steps = st;
    r = hub_call (r, T_REFEREE);
    sw.grade = r->grade;
    p = r->result;
    r->state = REQ_FREE;
    if (p == 2) {
        sw.result = SR_TONGUE;                  /* (Not one the character speaks) */
        sw.grade = 0;
        return;
    }
    sw.t = h == SW_THREATEN ? 50 : 100;
    m = sw.grade == G_CS ? 2 : sw.grade == G_MS ? 1 : sw.grade != G_CF ? 0 : h == SW_DECEIVE ? -2 :
        h == SW_THREATEN ? -AT_ALLIED : -1;
    m += t;
    t = m < 0 ? AT_HOSTILE : m > cap[h] ? cap[h] : m;
    hub_map (0);
    HUB->actor[k].attitude = sw.now = t;
    if (t == AT_ALLIED) {
        aorder[k] = OR_FOLLOW;                  /* (Won over: it comes with the character) */
    }
}

/* A request to ally k (sw's order): follow, wait, go its own way (an ally no more); fetching, not yet */
static void __fastcall__ order (unsigned char k)
{
    struct actor* q;

    hub_map (0);
    q = &HUB->actor[k];
    if (q->attitude != AT_ALLIED) {
        sw.result = SR_NOT_ALLY;
        return;
    }
    if (sw.order == OR_FETCH || !sw.order) {
        sw.result = SR_CANT;
        return;
    }
    sw.t = 20;
    if (sw.order == OR_HOME) {
        q->attitude = sw.now = AT_FRIENDLY;
        aorder[k] = OR_NONE;
    } else {
        aorder[k] = sw.order;
    }
}

/* A name given actor k (sw's word2, capitalised): a friend's or an ally's, kept in the head's names */
static void __fastcall__ give_name (unsigned char k)
{
    struct actor* q = &HUB->actor[k];
    unsigned char n;

    if (sw.was < AT_FRIENDLY || !sw.word2[0]) {
        sw.result = sw.word2[0] ? SR_NOT_ALLY : SR_CANT;
        return;
    }
    den_get (q->den);
    if ((n = name_of (D.id)) == NNAMES && (n = name_of (0)) == NNAMES) {
        sw.result = SR_CANT;                    /* (Names enough given) */
        return;
    }
    sw.word2[sizeof hd.names[0].name - 1] = 0;
    if (sw.word2[0] >= 'a' && sw.word2[0] <= 'z') {
        sw.word2[0] += 'A' - 'a';
    }
    hd.names[n].id = D.id;
    strcpy (hd.names[n].name, sw.word2);
    hub_map (0);
    strcpy (q->name, sw.word2);
}

/* The UI's request answered */
void s_sway (void)
{
    struct actor* q;
    unsigned char k, n = 0;

    hub_map (0);
    memcpy (&sw, &HUB->sway, sizeof sw);
    sw.result = SR_OK;
    sw.grade = 0;
    sw.t = 0;
    sw.name[0] = 0;
    sw.beast = 0;
    if (sw.kind == SW_ORDER && !sw.word[0]) {   /* (To every ally in earshot) */
        for (k = 0; k < NACTORS; ++k) {
            hub_map (0);
            q = &HUB->actor[k];
            if (q->name[0] && q->attitude == AT_ALLIED && q->sn == pcs && apart (q->x - pcx, q->y - pcy) <= 12) {
                strcpy (sw.name, ++n == 1 ? q->name : "");
                order (k);
                rouse (k, 1 | ROUSE_SET);
            }
        }
        sw.result = n ? sw.result : SR_NONE;
    } else if ((k = meant (sw.word)) == 0xFF) {
        sw.result = SR_NONE;
    } else {
        q = &HUB->actor[k];
        strcpy (sw.name, q->name);
        sw.was = sw.now = q->attitude;
        kind_get (akind[k]);
        sw.beast = K.mind == M_BEAST;
        switch (sw.kind) {
        case SW_LOOK:
            break;
        case SW_NAME:
            give_name (k);
            break;
        case SW_ORDER:
            order (k);
            break;
        default:
            influence (k);
            break;
        }
        rouse (k, 1 | ROUSE_SET);               /* (It sees to what's changed at once) */
        sgen[k] = 0;
    }
    settle ();
    memcpy (&HUB->sway, &sw, sizeof sw);       /* (The reply whole, then the state that shows it) */
    hub_reply (&HUB->sway);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The swapped parts' images read into banks of its own (dir/dq0d.ovl: dq0d.cfg's OV1, OV2): 0, or -1 */
static int __fastcall__ ovl_load (const char* dir)
{
    unsigned char* b = (unsigned char*) _SWAP_START__;  /* (Through the parts' area: none in it yet) */
    unsigned off;
    int fd, n;

    strcpy ((char*) b, dir);
    strcat ((char*) b, "/dq0d.ovl");
    if ((fd = open ((char*) b, O_RDONLY)) < 0 || (n = hy_banks_alloc (2)) < 0) {
        return -1;
    }
    ovbank[0] = n;
    ovbank[1] = n + 1;
    for (off = 0; off < 0x4000 && (n = read (fd, b, 512)) > 0; off += n) {
        hy_bank (ovbank[off >> 13]);
        memcpy ((void*) (0x8000 + (off & 0x1FFF)), b, n);
    }
    close (fd);
    return 0;
}

#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* The task begun, its banks had: the kinds read in (dir's), the theme's denizens, the records new or the game's
** kept ones read back: 0, or -1 */
int __fastcall__ s_start (const char* dir)
{
    if (kinds_load (dir)) {
        return -1;
    }
    hub_map (THEME_BANK);                       /* (The theme's denizens: which of the kinds) */
    tdens = THEME->dens;
    chunks_init (ASK_DENIZENS);
    hy_bank (pb);
    memset (POP, 0, 0x2000);
    hub_map (0);
    memcpy (hd.fatt, fatt0, sizeof hd.fatt);    /* (A new game's: the factions as the character first meets them) */
    hd.magic = DN_MAGIC;
    if (HUB->resume) {
        keep (1);
    }
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* A number's digits (the task's arguments) */
static unsigned char __fastcall__ num (const char* s)
{
    unsigned char n = 0;

    while (*s >= '0' && *s <= '9') {
        n = n * 10 + *s++ - '0';
    }
    return n;
}

int main (int argc, char* argv[])
{
    static unsigned lpx, lpy;
    static unsigned char lpl;
    static unsigned long done_at;
    static unsigned char gone, g, act, gw;
    unsigned char* p;
    int b;

    if (argc < 4 || ovl_load (argv[3]) || (b = hy_banks_alloc (2)) < 0 ||
        hub_attach (num (argv[1]), num (argv[2]))) {
        return 1;
    }
    kb = b;
    pb = b + 1;
    if (start (argv[3])) {
        return 1;
    }
    wplace = HUB->eq_head;                      /* (Its events from now on) */
    dplace = fplace = HUB->dq_head;
    while (hub_map (0), !HUB->quit) {
        while (hy_sem_try (HUB->sem[T_DENIZENS]) == 0) {
        }
        settle ();                              /* (Its intents the world's, as it goes on) */
        if (hub_posted (&HUB->dsave)) {         /* (The UI's asked for a save) */
            keep (0);
            hub_reply (&HUB->dsave);
        }
        turns = !HUB->tpr;
        g = HUB->den_go;
        act = !turns || g != gone;              /* (At the turns pace, only in its turn: the world's word, den_go) */
        if (!turns && HUB->ui_cmd && gw < GIVE_WAY) {
            ++gw;                               /* (At the real paces, the UI's command in hand answered first: the
            act = 0;                            **   minds wait, as the senses' look does, a second at most) */
        } else if (!HUB->ui_cmd) {
            gw = 0;
        }
        if (act || hub_posted (&HUB->sway)) {
            now = hub_now ();
            pcx = HUB->pc_x;
            pcy = HUB->pc_y;
            if (pcs != HUB->pc_stratum) {
                pcs = HUB->pc_stratum;
                sync_strata ();                 /* (Its stratum's record in its slot: the world may have made way for
                                                **   it there) */
                hub_map (0);
            }
            pcbusy = HUB->pc_ready > now;
            pcdead = (HUB->pcsum.cond & C_DEAD) != 0;
            if (!(pclit = HUB->pc_light != 0) && DONE (pcs) && (p = at (pcs, pcx, pcy)) != 0) {
                pclit = (*p & T_VIS) != 0;      /* (Lit where it stands: a room's light, a lamp put down) */
            }
            hub_map (0);
        }
        if (act && (pcx != lpx || pcy != lpy || (pclit | pcbusy << 1) != lpl)) {
            lpx = pcx;                          /* (The character changed: each actor senses it again) */
            lpy = pcy;
            lpl = pclit | pcbusy << 1;
            if (!++sense_gen) {
                sense_gen = 1;
            }
        }
        if (hub_posted (&HUB->sway)) {          /* (The UI's: the character's way with a denizen) */
            sway ();
        }
        if (act) {
            grow ();
            struck |= fought ();                /* (The character's blows: the slice's first, the world's) */
            if (DONE (pcs)) {
                hear ();
                tend ();
            }
            if (turns || now - done_at >= 50) {
                gone = g;                       /* (Its turn done, its intents in: the turns pace goes on; at the
                                                **   others, now and then, the world's bleeding seen to) */
                done_at = now;
                im.w1 = g;
                im.t = now;
                post (I_DONE, 0);
            }
        }
        hub_map (0);
        ++HUB->beat[T_DENIZENS];
        hub_sleep (10);                         /* (Asleep on the doorbell: a request, an event, the clock moved on
                                                **   at the turns pace, wakes it at once) */
    }
    return 0;
}

/* ==== Part 3: the records paged (see its head), and kept */
#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

#define VF_STATIC       static
#include "../dq0/vfile.inc"

unsigned char rsn, rrx, rry;                    /* (A region: found, written, read) */
static unsigned char ksn, krx, kry;             /* (The region at hand: not put out to make room for its own) */
char vpth[56];
static struct vfh vf;

/* The region of the denizen in D (its square's; not placed, its lair's) into rsn, rrx, rry */
static void den_region (void)
{
    if (D.flags & (D_OUT | D_ACTOR | D_DEAD)) {
        rsn = D.sn;
        rrx = D.x >> 8;
        rry = D.y >> 8;
    } else {
        rsn = D.lsn;
        rrx = D.lrx;
        rry = D.lry;
    }
}

/* Region rsn, rrx, rry's entry in the table, or 0xFF (a free one, rfree: 0xFF none) */
unsigned char rfree;
static unsigned char reg_find (void)
{
    unsigned char k, f = 0xFF, rlim;

    hub_map (0);
    rlim = HUB->small ? 2 : NDREG;              /* (dq0 -C: two, the tests' put out soon) */
    for (k = 0, rfree = 0xFF; k < NDREG; ++k) {
        if (!hd.reg[k].sn) {
            if (rfree == 0xFF && k < rlim) {
                rfree = k;
            }
        } else if (hd.reg[k].sn == rsn + 1 && hd.reg[k].rx == rrx && hd.reg[k].ry == rry) {
            f = k;
        }
    }
    return f;
}

/* The table's entry for region rsn ... made (its stamp the newest) */
static void __fastcall__ reg_add (unsigned char k)
{
    hd.reg[k].sn = rsn + 1;
    hd.reg[k].rx = rrx;
    hd.reg[k].ry = rry;
    hd.reg[k].stamp = ++hd.rstamp;
}

/* Region k of the table put out: its lairs and its denizens written (dnSXXYY.dat, a new version: each record after its
** tag, 0 a lair's, 1 a denizen's), their places freed, its entry too: 1, or 0 (no folder) */
static unsigned char __fastcall__ reg_out (unsigned char k)
{
    static unsigned char tag;
    unsigned char sn = hd.reg[k].sn - 1, rx = hd.reg[k].rx, ry = hd.reg[k].ry, i;
    unsigned n = 0;
    int fd;

    for (i = 0; i < hd.ndens; ++i) {
        den_get (i);
        den_region ();
        if ((D.flags & D_ACTOR) && rsn == sn && rrx == rx && rry == ry) {
            return 0;                           /* (An actor's: kept) */
        }
    }
    if (!vf_name (vpth, "dn", sn, rx, ry) || (fd = vf_make (vpth, 2)) < 0) {
        return 0;
    }
    for (i = 0, tag = 0; i < hd.nlairs; ++i) {
        lair_get (i);
        if (L.kind && L.sn == sn && L.rx == rx && L.ry == ry) {
            write (fd, &tag, 1);
            write (fd, &L, sizeof L);
            ++n;
            L.kind = 0;
            lair_put (i);
        }
    }
    for (i = 0, tag = 1; i < hd.ndens; ++i) {
        den_get (i);
        den_region ();
        if (D.kind && rsn == sn && rrx == rx && rry == ry) {
            write (fd, &tag, 1);
            write (fd, &D, sizeof D);
            ++n;
            D.kind = 0;
            den_put (i);
        }
    }
    vf_done (fd, n);
    hd.reg[k].sn = 0;
    return 1;
}

/* The table's entry least lately wanted that isn't round the character's region (one each way, on its stratum) nor
** the region at hand, put out: 1, or 0 */
/* (make_room's: the lair and the denizen at hand kept, the records put out read through L and D) */
static struct lair sl;
static struct den sd;
static unsigned char make_room (void)
{
    unsigned char k, b = 0xFF, cx = pcx >> 8, cy = pcy >> 8, r;
    unsigned t;

    hub_map (0);
    r = !HUB->small;                            /* (Those one each way kept; dq0 -C, its own alone) */
    for (k = 0; k < NDREG; ++k) {
        if (hd.reg[k].sn && (hd.reg[k].sn != pcs + 1 || (unsigned char) (hd.reg[k].rx - cx + r) > 2 * r ||
            (unsigned char) (hd.reg[k].ry - cy + r) > 2 * r) &&
            (hd.reg[k].sn != ksn + 1 || hd.reg[k].rx != krx || hd.reg[k].ry != kry) &&
            (b == 0xFF || hd.reg[k].stamp - t > 0x8000U)) {
            b = k;
            t = hd.reg[k].stamp;
        }
    }
    if (b == 0xFF) {
        return 0;
    }
    memcpy (&sl, &L, sizeof L);
    memcpy (&sd, &D, sizeof D);
    b = reg_out (b);
    memcpy (&L, &sl, sizeof L);
    memcpy (&D, &sd, sizeof D);
    return b;
}

/* A free record's place (lair 1: a lair's, else a denizen's), the first: its place (the records used up to it), or
** 0xFF (none to be made: no folder) */
unsigned char __fastcall__ s_rec_new (unsigned char lair)
{
    unsigned char i, n = lair ? NLAIRS : NDENS;

    do {
        for (i = 0; i < n; ++i) {
            hy_bank (pb);
            if (!(lair ? POP->lair[i].kind : POP->den[i].kind)) {
                hub_map (0);
                if (lair && i >= hd.nlairs) {
                    hd.nlairs = i + 1;
                } else if (!lair && i >= hd.ndens) {
                    hd.ndens = i + 1;
                }
                return i;
            }
        }
        hub_map (0);
    } while (make_room ());
    return 0xFF;
}

/* Region sn, rx, ry wanted: in the cache (its file's records read in, if it has one, and it's not in the cache yet;
** an entry for it in the table, one put out for it if it must), its stamp the newest: 1, or 0 (no room) */
unsigned char __fastcall__ s_reg_need (unsigned char sn, unsigned char rx, unsigned char ry)
{
    static unsigned char tag;
    unsigned char k, i;
    int fd;

    rsn = sn;
    rrx = rx;
    rry = ry;
    if ((k = reg_find ()) != 0xFF) {
        hd.reg[k].stamp = ++hd.rstamp;
        return 1;
    }
    ksn = sn;
    krx = rx;
    kry = ry;
    if (rfree == 0xFF && (!make_room () || (rsn = sn, rrx = rx, rry = ry, reg_find (), rfree == 0xFF))) {
        return 0;
    }
    k = rfree;
    rsn = sn;
    rrx = rx;
    rry = ry;
    reg_add (k);
    if (vf_name (vpth, "dn", sn, rx, ry) && (fd = vf_open (vpth, 2, &vf)) >= 0) {
        while (read (fd, &tag, 1) == 1) {
            if (tag) {
                if (read (fd, &D, sizeof D) != sizeof D || (i = s_rec_new (0)) == 0xFF) {
                    break;
                }
                D.flags &= ~D_ACTOR;
                den_put (i);
            } else {
                if (read (fd, &L, sizeof L) != sizeof L || (i = s_rec_new (1)) == 0xFF) {
                    break;
                }
                lair_put (i);
            }
        }
        close (fd);
    }
    hub_map (0);
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 4: the regions round the character wanted, and the records kept */
#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* (Part 3's, a copy of its own) */
/* The region of the denizen in D (its square's; not placed, its lair's) into rsn, rrx, rry */
static void den_region4 (void)
{
    if (D.flags & (D_OUT | D_ACTOR | D_DEAD)) {
        rsn = D.sn;
        rrx = D.x >> 8;
        rry = D.y >> 8;
    } else {
        rsn = D.lsn;
        rrx = D.lrx;
        rry = D.lry;
    }
}

/* Region rsn, rrx, rry's entry in the table, or 0xFF (a free one, rfree: 0xFF none) */
unsigned char rfree;
static unsigned char reg_find4 (void)
{
    unsigned char k, f = 0xFF, rlim;

    hub_map (0);
    rlim = HUB->small ? 2 : NDREG;              /* (dq0 -C: two, the tests' put out soon) */
    for (k = 0, rfree = 0xFF; k < NDREG; ++k) {
        if (!hd.reg[k].sn) {
            if (rfree == 0xFF && k < rlim) {
                rfree = k;
            }
        } else if (hd.reg[k].sn == rsn + 1 && hd.reg[k].rx == rrx && hd.reg[k].ry == rry) {
            f = k;
        }
    }
    return f;
}

/* The table's entry for region rsn ... made (its stamp the newest) */
static void __fastcall__ reg_add4 (unsigned char k)
{
    hd.reg[k].sn = rsn + 1;
    hd.reg[k].rx = rrx;
    hd.reg[k].ry = rry;
    hd.reg[k].stamp = ++hd.rstamp;
}

/* The regions round the character's (one each way, on its stratum) wanted, as it comes into one */
void s_dneed (void)
{
    unsigned char x, y, cx = pcx >> 8, cy = pcy >> 8, r;

    hub_map (0);
    r = !HUB->small;                            /* (dq0 -C: its own region alone) */
    for (y = cy >= r ? cy - r : 0; y <= cy + r && y >= cy - r; ++y) {
        for (x = cx >= r ? cx - r : 0; x <= cx + r && x >= cx - r; ++x) {
            reg_need (pcs, x, y);
        }
    }
}

/* ---- The records kept (back: read again), the save folder's dens.dat: the bank a piece at a time, through the
** main RAM (the actors' places written back first; read back, none an actor yet; none read but this version's) */
void __fastcall__ s_keep (unsigned char back)
{
    unsigned off;
    unsigned char k;
    int fd;

    hub_map (0);
    if (!HUB->savedir[0]) {
        return;
    }
    strcpy (vpth, HUB->savedir);
    strcat (vpth, "/dens.dat");
    if ((fd = back ? open (vpth, O_RDONLY) : open (vpth, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    if (!back) {
        for (k = 0; k < NACTORS; ++k) {
            hub_map (0);
            if (HUB->actor[k].name[0]) {
                write_back (k);
                den_put (a.den);
            }
        }
        hy_bank (pb);
        memcpy (POP, &hd, sizeof hd);
    } else if (read (fd, piece, 2) != 2 || *(unsigned*) piece != DN_MAGIC) {
        close (fd);
        return;
    } else {
        lseek (fd, 0, SEEK_SET);
    }
    for (off = 0; off < 0x2000; off += sizeof piece) {
        if (back) {
            if (read (fd, piece, sizeof piece) != sizeof piece) {
                memset (piece, 0, sizeof piece);
            }
            hy_bank (pb);
            memcpy ((char*) 0x8000 + off, piece, sizeof piece);
        } else {
            hy_bank (pb);
            memcpy (piece, (char*) 0x8000 + off, sizeof piece);
            write (fd, piece, sizeof piece);
        }
    }
    close (fd);
    if (back) {
        hy_bank (pb);
        memcpy (&hd, POP, sizeof hd);
        for (k = 0; k < hd.ndens; ++k) {
            den_get (k);
            D.flags &= ~D_ACTOR;
            den_put (k);
        }
    }
    hub_map (0);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
