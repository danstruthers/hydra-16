/*
** dq0d.c - Dingus Quest Zero's denizens (DENIZENS.md): a task of its own, started by the UI (dq0d SEG ROLE LIBDIR:
** the hub's segment, its role there, hub.h, and the game's lib folder).  As each stratum's plan is grown, it decides
** the stratum's populations: as the history left them, if the theme has inhabitants of its own (the save folder's
** theme file, its faction lines: pop.h), each faction's lairs as popgen gives them (bank/popgen, a bank program,
** bankprog.h's: their kinds and how many, its leader in its home); else the stand-in's, by the stratum's style
** and its depth, from the kinds the theme names (kinds.txt, the game's bestiary, made lib/dq0/dq0d.dat, read into a
** bank of its own as it starts): the lairs, where they lie in the plan, and the denizens of each, a record each from
** then on; so nothing is made up as the character turns a corner.  As the
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
** character's or where it's going.  Each has an attitude to the character, its faction's to begin with (its
** record's: the theme's, or the stand-in's, factions.txt's), moved by what happens and by the character's
** words (the UI's request, the hub's sway: the referee's test), and acts by it: the hostile come at the character, an
** ally follows it and takes its requests; factions hostile to one another keep off one another.  (Their own exploring
** comes later.)
**
** As it decides a stratum's populations it says which places are kept lit (DUNGEON.md: light kept): each lair of a
** kind that keeps lights (kinds.txt's f), its chunk and its keepers' range, in the hub's places kept (KEPT_AT), the
** stratum marked done there (the world waits for that before its first chunk's made, the maker stocks by it); and as
** a lair's last keeper dies, its keeping's gone (its torches burn down from then: the world's).
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
** copied there from the overlays' file's banks, LIBDIR/dq0d.ovl, as it's called; bank.s's stubs): part 1, the
** populations, the placing; part 2, the character's ways with a denizen, and harmed; part 3, the records paged; part
** 4, the regions round the character wanted, the records kept, the task's start (the kinds read in, popgen loaded),
** and what its actors seldom do (one made an actor or no longer one, killed, an ally come along, a cornered one's
** dart); part 5, the senses' own (a hunter's stalk, a scent followed, one drawn on); part 6, morale (HOUSE.md's: the
** field about an actor and its bonds to its leaders, reckoned as it engages and as one near it falls; its kin
** dismayed, a leader's loss known; the bonds aged by the clock).
** The blows its actors strike and take are the world's, told as the denizens' events (E_FIGHT): it takes them to heart
** (fought: fleeing, dismay, harmed, a death recorded).
*/

#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bankprog.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "lore.h"
#include "pop.h"

#pragma static-locals (on)                      /* (Its locals static: quicker, and nothing here is reentered) */
#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

/* ---- The swapped parts' stubs (bank.s's) */

void __fastcall__ populate (unsigned char n);   /* Part 1 */
void __fastcall__ place (unsigned char i);
void __fastcall__ keep (unsigned char back);    /* Part 3 */
unsigned char __fastcall__ rec_new (unsigned char lair);
unsigned char __fastcall__ reg_need (unsigned char sn, unsigned char rx, unsigned char ry);
void dneed (void);
void __fastcall__ harmed (unsigned char k);     /* Part 2 */
unsigned char __fastcall__ stalks (unsigned char k);
unsigned char __fastcall__ scent (unsigned char k);
unsigned char __fastcall__ draw_on (unsigned char k);
void sway (void);
int __fastcall__ start (const char* dir);      /* Part 4 */
void __fastcall__ enlist (unsigned char i, unsigned char k);
void __fastcall__ retire (unsigned char k);
void __fastcall__ killed (unsigned char k);
unsigned char __fastcall__ along (unsigned char k);
unsigned char __fastcall__ dart (unsigned char k);
void __fastcall__ dismay (unsigned char k);    /* Part 6 (morale) */
void __fastcall__ heart (unsigned char k);
void age (void);
extern unsigned char ovbank[3];
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
#define K_LIGHTS        0x20                    /* It keeps lights: a mind that uses fire, needing light (its lair's
                                                **   rooms kept lit: DUNGEON.md's light kept, slice 4.1) */
#define K_SWEEPS        0x40                    /* It keeps its lair's floors swept (no dust there, its traces
                                                **   scattered: DUNGEON.md's traces, slice 4.1) */
#define K_NOSE          0x80                    /* A hunter by smell (its nose keen, kinds.txt's 6 or more): hostile,
                                                **   it follows the character's scent where it can't sense it (scent) */
#define K_HELP(k)       ((k).styles >> 4)       /* Its way with a foe it's brought down (the character helpless:
                                                **   senseless, or in shock), by its nature (kinds.txt's ways k and r,
                                                **   make.js's, in its styles' high bits: the flags are full): */
#define H_KILLS         0x01                    /*   it finishes it (the referee's dispatch); */
#define H_ROBS          0x02                    /*   it robs it (I_ROB: the world's), and leaves it; neither, it only
                                                **   defended itself, and goes */
enum { M_MINDLESS, M_BEAST, M_LOW, M_AVERAGE, M_CUNNING };   /* Its mind: nothing sways it; P_ANIMALS only; easily
                                                **   fooled, hard to reason with; neither; hard to fool */
#define KINDS           ((struct dkind*) 0x8000)
typedef char dkind_fits[sizeof (struct dkind) == 40 ? 1 : -1];     /* (make.js's records) */

/* The factions (factions.txt's: a stand-in theme's, after the kinds in the data file, the first none): each one's
** tongue, its attitude to the character as it first meets it, how it stands with each other one (AT_HOSTILE,
** AT_WARY, AT_INDIFFERENT, AT_FRIENDLY), copied out into the main RAM as it starts */
#define NFACT           8
struct faction {
    char            name[12];
    char            tongue[9];                  /* ("": none; 8 letters, 2026-10-29: "sindarin") */
    unsigned char   att;                        /* AT_* */
    unsigned char   rel[NFACT];                 /* AT_*: each faction's, by number (a stand-in's: the first 7) */
    unsigned char   spare[2];
};
typedef char faction_fits[sizeof (struct faction) == 32 ? 1 : -1];
static unsigned char nfacts;
static char ftongue[NFACT][9];
static unsigned char fatt0[NFACT], frel[NFACT][NFACT];

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
#define L_LEADER        0x02                    /* Its faction's leader with them (a theme's home: popgen's, 2026-10-29), its
                                                **   faction the high bits (L_FAC) */
#define L_FAC(f)        (((f) >> 4) & 7)
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
    unsigned char   fac;                        /* Its faction (its kind's, the stand-in's; or the theme's) */
    unsigned char   serve;                      /* The weeks it had served its leaders as the game began (a leader's,
                                                **   it had led: its band's age), */
    unsigned char   contact;                    /*   and how much it's with them, 0-255 (part 6's: its bonds' growth) */
};
#define D_OUT           0x01                    /* On a square of its own */
#define D_ACTOR         0x02                    /* An actor now (the hub's entry the truth while it is) */
#define D_DEAD          0x04                    /* Dead (its body left where it fell: an item) */
#define D_MET           0x08                    /* Met (made an actor once: its personal things made, I_KIT's KT_PURSE,
                                                **   the outfits' phase 3, 2026-10-27) */
#define D_LOST          0x10                    /* It knows a leader of its is lost (dead): no bond to a dead one after
                                                **   (part 6's; one it doesn't know of holds) */
#define NNAMES          8
#define NDREG           32
#define GIVE_WAY        30                      /* (Tenths the minds wait for the UI's command in hand, at most: its
                                                **   answer's first words before anything they do, its parsing a
                                                **   second or so as the tasks share the CPU; by the clock, not
                                                **   their rounds, every event waking them) */
static unsigned char gw;                        /* (Waiting for it: main's, and tend's between actors) */
static unsigned long gwt;                       /* (Since when) */
static unsigned char gsq;                       /* (Its command line's: the parser's request's seq, each line's own) */
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
static unsigned char knose[SND_KINDS];          /* Each kind's nose (kinds.txt's, dq0tr.dat's: the traces'), and the
                                                **   character's smell (its sweat's: the scent a hunter follows) */
static unsigned char pcsmell = 3;
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

/* A kind's belongings' ways (kinds.txt's keeps, standing and home: make.js's, after the factions in the kinds' bank,
** 2026-10-27), copied out (X) */
struct dkx {
    unsigned char   keeps;                      /* KP_*: what it doesn't carry, where */
    unsigned char   standing;                   /* 0-4: its means */
    unsigned char   home[3];                    /* Its home room's uses, the best liked first (world.h's U_*; 0xFF none) */
    unsigned char   body;                       /* Its body (hub.h's BD_*: how it gets up, and falls, worded) */
    unsigned char   rank;                       /* Its rank in its faction (RK_*: kinds.txt's, 2026-10-30) */
};
enum { KP_BODY, KP_OPEN, KP_HIDDEN, KP_BOX, KP_TO_LEADER, KP_LEADER };
enum { RK_FILE, RK_CAPTAIN, RK_LORD };          /* (The rank and file; a band's head; a chieftain or a spiritual
                                                **   leader: part 6's morale) */
static struct dkx X;
static void __fastcall__ kx_get (unsigned char k)
{
    hy_bank (kb);
    memcpy (&X, (struct dkx*) ((struct faction*) (KINDS + nkinds) + KINDS[0].styles) + (k < nkinds ? k : 0), sizeof X);
    hub_map (0);
}

/* The room of the chunk at ox, oy (stratum sn: a lair's, kind k's) its home's in: one of the uses its kind likes best
** (its home's, kinds.txt's: of the first of them the chunk has, one by the dice), else any; the chunk in the window */
static unsigned char __fastcall__ home_room (unsigned char k, unsigned char sn, unsigned ox, unsigned oy)
{
    unsigned char u, r, n, j;

    kx_get (k);
    at (sn, ox, oy);
    for (u = 0; u < 3 && X.home[u] != 0xFF; ++u) {
        for (n = 0, r = 0; r < C->nrooms; ++r) {
            n += C->room[r].kind == X.home[u];
        }
        if (n) {
            for (j = random (n), r = 0; C->room[r].kind != X.home[u] || j--; ++r) {
            }
            return r;
        }
    }
    return random (C->nrooms);
}

#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* The kinds read in (dir/dq0d.dat): 0, or -1 (part 4's: the start's) */
static int __fastcall__ kinds_load (const char* dir)
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
    strcpy (path, dir);                         /* (The kinds' noses, the character's smell: the traces' file) */
    strcat (path, "/dq0tr.dat");
    if ((fd = open (path, O_RDONLY)) >= 0) {
        for (n = 0; n < SND_KINDS && read (fd, piece, 64) == 64; ++n) {
            knose[n] = piece[14];
            pcsmell = n ? pcsmell : piece[13];
        }
        close (fd);
    }
    hy_bank (kb);                               /* (The first record's the counts; the factions after the kinds) */
    nkinds = KINDS[0].flags;
    nfacts = KINDS[0].styles < NFACT ? KINDS[0].styles : NFACT;
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
static unsigned char pcrest;                    /* (The character in a long wait or asleep, not asked of: the actors'
                                                **   idle ways the coarser, decide's and coarse's) */
static unsigned char pchelp;                    /* (Helpless: in shock, or senseless as it lies, busy till it comes
                                                **   round; the referee's conditions) */
static unsigned char pcstay;                    /* (Its stay where it stands, since it came onto the square: its
                                                **   scent's points more, hub.h's TR_MORE of a passing each TQ) */
static unsigned char nowq, pcq;                 /* (The TQs gone by, the traces' clock: its low byte; and as the
                                                **   character came onto its square) */
static unsigned long nextq;                     /* (When the next TQ's gone by) */

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

/* A lair of stratum n's, of kind k (K its record, copied out), the j'th of its kind there, faction lf's: by the
** stratum's ways if its kind lairs there (the first ones: not the dungeon's way in, the top's first, where the
** character begins), else in a region of halls, a chunk of it; ln denizens of it (0: as many as its kind's group,
** drawn), and one of kind ll with them (its faction's leader, in its home: 0 none).  0 if no more of its kind can be
** had there (its ways all taken, but lspill puts it in the halls then; no room for its records), else 1 (made, or no
** halls found for it).  Its numbers the stratum's seed's, in turn */
static unsigned char lf, ln, ll, lspill;

static unsigned char __fastcall__ lair_add (unsigned char n, unsigned char k, unsigned char j)
{
    struct stratum* s = &STRATUM (n);
    struct keep* q;
    unsigned char i, rx = 0, ry = 0, t, u;
    unsigned w = (s->w + CW - 1) / CW, h = (s->h + CW - 1) / CW;

    memset (&L, 0, sizeof L);
    L.sn = n;
    L.kind = k;
    L.link = 0xFF;
    if ((K.flags & K_WAYS) && (u = j + (n == 0)) < s->nlinks) {
        L.link = u;
        L.cx = (s->link[u].x - s->x0) / CW;
        L.cy = (s->link[u].y - s->y0) / CW;
    } else if ((K.flags & K_WAYS) && !lspill) {
        return 0;
    } else {
        for (t = 0; t < 8; ++t) {               /* (A region of halls, a chunk of it) */
            rx = random (s->rw);
            ry = random (s->rh);
            if (plan_kind (n, (unsigned) ry * s->rw + rx) == RG_HALLS) {
                break;
            }
        }
        hub_map (0);
        if (t == 8) {
            return 1;
        }
        L.cx = (unsigned) rx * RCH + random (RCH);
        L.cy = (unsigned) ry * RCH + random (RCH);
        L.cx = L.cx < w ? L.cx : w - 1;
        L.cy = L.cy < h ? L.cy : h - 1;
    }
    L.n = ln ? ln : K.nmin + random (K.nmax - K.nmin + 1);
    L.rx = (s->x0 + (L.cx << CW_BITS)) >> 8;
    L.ry = (s->y0 + (L.cy << CW_BITS)) >> 8;
    if (!reg_need (n, L.rx, L.ry) || (t = rec_new (1)) == 0xFF) {
        return 0;                               /* (Its region in the cache first: no room, no folder) */
    }
    L.id = ++hd.lids;
    memset (&D, 0, sizeof D);
    D.kind = k;
    D.fac = lf;
    D.lid = L.id;
    D.sn = D.lsn = n;
    D.lrx = L.rx;
    D.lry = L.ry;
    D.attitude = hd.fatt[lf & (NFACT - 1)];    /* (Its faction's, as the character stands with it) */
    D.contact = ll ? 200 : 64;                  /* (With its leader in its lair, often; else seldom: part 6's bonds) */
    rx = 8 + ((unsigned char) (L.id * 5 ^ L.id >> 1) & 127);  /* (The band's age, weeks: by its number, not the dice, so the
                                                **   populations' dice are as they were) */
    for (i = 0; i < L.n + (ll != 0) && (u = rec_new (0)) != 0xFF; ++i) {
        if (i == L.n) {
            D.kind = ll;                        /* (Its leader, the last) */
        }
        D.id = ++hd.ids;
        D.serve = rx - ((unsigned char) D.id & rx >> 3);   /* (Each its own, about the band's age) */
        den_put (u);
    }
    L.n = i;
    if (ll) {
        L.flags |= L_LEADER | (lf & 7) << 4;
    }
    lair_put (t);
    if ((K.flags & K_LIGHTS) && i) {            /* (Its keepers' lights: the places kept, the hub's) */
        hub_map (DEN_BANK);
        if ((u = KEPT_AT->n) < NKEPT) {
            q = &KEPT_AT->at[u];
            q->sn = n;
            q->range = K.range;
            q->live = i;
            q->cx = L.cx;
            q->cy = L.cy;
            q->lid = L.id;
            KEPT_AT->n = u + 1;
        }
        hub_map (0);
    }
    return 1;
}

/* popgen (a bank program: pop.h), if the theme has inhabitants of its own: its entry for a stratum's lairs (0: none,
** the stand-in's), its arguments, the lairs it gives, the paths it's called by (bankprog.h keeps them) */
static struct bank_fn* pgs;
static struct pgarg pga;
static struct pglair pgl[POP_LAIRS];
static char pgexp[48], pgbp[48], pgthm[48];

void __fastcall__ s_populate (unsigned char n)
{
    static unsigned char ways[SND_KINDS];       /* (Each kind's lairs by the ways, so far) */
    struct stratum* s = &STRATUM (n);
    struct pglair* p;
    unsigned char k, j, m;

    rseed (s->seed ^ 0x44454E5AUL);
    if (pgs) {                                  /* (The theme's factions, as popgen places them) */
        pga.sn = n;
        pga.seed = (unsigned) s->seed;
        m = bank_call (pgs, unsigned char __fastcall__ (*) (struct pgarg*), &pga);
        memset (ways, 0, sizeof ways);
        for (j = 0, p = pgl; j < m && j < POP_LAIRS; ++j, ++p) {
            if (p->flags & PL_LEADER) {
                continue;                       /* (With its home, the lair before it) */
            }
            kind_get (k = p->kind);
            lf = p->fac;
            ln = p->n;
            ll = j + 1 < m && (p[1].flags & PL_LEADER) ? p[1].kind : 0;
            lspill = 1;
            if (!lair_add (n, k, ways[k & (SND_KINDS - 1)]++)) {
                break;
            }
        }
    } else {
        for (k = 1; k < nkinds; ++k) {          /* (The stand-in's: each kind that lives in its style at its depth) */
            kind_get (k);
            if (!(K.styles & (1 << s->style)) || n < K.deep0 || n > K.deep1 || !(tdens & 1UL << k)) {
                continue;
            }
            m = K.flags & K_WAYS ? K.lairs : random (K.lairs + 1);
            lf = K.faction;
            ln = ll = lspill = 0;
            for (j = 0; j < m && lair_add (n, k, j); ++j) {
            }
        }
    }
    hd.done[n >> 3] |= 1 << (n & 7);
    hub_map (DEN_BANK);                         /* (Its places kept whole: so said, the world's to go on) */
    KEPT_AT->done[n >> 3] |= 1 << (n & 7);
    hub_map (0);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The strata grown since the last look, populated (none while the world's asking the maker: a plan may be half
** copied into the world bank), whatever the pace (the world waits for them: KEPT_AT) */
static void grow (void)
{
    static unsigned char ms[MAXSTRATA], ns[MAXSTRATA];
    const struct stratum* p;
    unsigned char k;

    hub_map (WORLD_BANK);                       /* (The hub's slots: each one's stratum, its own number; at once, as
                                                **   it's looked at every time round: walked, not indexed, no
                                                **   multiplying) */
    for (k = 0, p = WREC->strata; k < MAXSTRATA; ++k, ++p) {
        ms[k] = p->made;
        ns[k] = p->n;
    }
    hub_map (0);
    for (k = 0; k < MAXSTRATA; ++k) {
        if (ms[k] && !DONE (ns[k]) && !hub_posted (&HUB->mreq[ASK_WORLD])) {
            sync_strata ();
            populate (ns[k]);
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

static struct msg im;                           /* (Its intents: below) */
static void __fastcall__ post (unsigned char kind, unsigned char who);
static void trodden (void);
static void kitted (void);

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

void __fastcall__ s_place (unsigned char i)
{
    struct stratum* s;
    struct room* q;
    unsigned x0, y0, x, y, ox, oy;
    unsigned char w = 7, h = 7, j, t, k, kd = 0;

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
        q = &C->room[home_room (L.kind, L.sn, ox, oy)];  /* (By its use, its kind's liking: 2026-10-27) */
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
        kd = D.kind;
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
        im.a = (j && kd == L.kind ? 0 : KT_LAIR) | ((kind_get (kd), KF_NATURE (K.form)) >= NA_FOLK ? KT_ROLE : 0);  /* (Its
                                                **   lair's own with its first: its stores, its hoard; a beast's den's,
                                                **   the dead's lair's, 2026-10-29; folk's and the dead's roles; a
                                                **   theme's leader with its home its own too, its hoard, 2026-10-29) */
        if (im.a == KT_LAIR && (hub_map (0), HUB->gear & GE_OLD)) {
            im.a = 0;                           /* (dq0 -K's, the tests': a beast's den none, as before, not even asked
                                                **   of the world) */
        }
        if (im.a) {                             /* (Its things made, the world's draw by its kind's role and its
                                                **   lair's, filed at its home, the outfits' gear) */
            im.b = K.faction;
            im.x[0] = 0;
            im.x[1] = kd;
            im.x[2] = L.sn;
            im.w1 = L.hx;
            im.w2 = L.hy;
            im.t = D.id;
            kitted ();
        }
        im.x[1] = kd;                           /* (Its scent where it lives, at its lair's home: a passing, its comings
                                                **   and goings, and its stays, the most: the world's traces told) */
        im.x[2] = L.sn;
        im.w1 = L.hx;
        im.w2 = L.hy;
        im.a = 0;
        im.b = 255;
        trodden ();
    }
    if (kd && (kind_get (kd), K.flags & K_SWEEPS)) {
        im.x[2] = L.sn;                         /* (A kind that sweeps its floors: the lair's chunk kept swept, the
                                                **   world's traces told) */
        im.w1 = L.hx;
        im.w2 = L.hy;
        post (I_SWEPT, 0);
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
static unsigned char stalk[NACTORS];            /* (Each one stalking the character: its steps' loudness plus 1) */
static unsigned char stq[NACTORS], stn[NACTORS];   /* (Each one's square, its x and 16 times its y (a step's
                                                **   always another), and the TQ its stay there last journalled) */
static unsigned char fled[NACTORS];             /* (Each one fleeing the character: its turns of it left) */
unsigned char __fastcall__ shoots (unsigned char k, unsigned char ag);   /* (Part 2's: slice 4.7's) */
static unsigned char shat[NACTORS];             /* (Where the character was as each last shot, or meant to: AG_NOSHOT's
                                                **   worth while it's there still, slice 4.7's) */
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

/* A passing posted (im's: I_TROD, the world's traces), not waited for: nothing an actor's decision reads */
static void trodden (void)
{
    unsigned char st = stirs;

    post (I_TROD, 0);
    stirs = st;
}

/* A denizen's things made (I_KIT, the world's: the outfits' gear), its fields the caller's: not a stir, as a passing
** isn't */
static void kitted (void)
{
    unsigned char st = stirs;

    post (I_KIT, 0);
    stirs = st;
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
/* Where actor k (a robber, its kind's record K) takes what it robs (I_ROB's w1, w2, x[2], b): its faction's hoard, the
** home of a lair of its faction's leader (kinds.txt's keeps H; or a theme's home, its leader with it: L_LEADER,
** 2026-10-29, its theme's faction the robber's) placed on its stratum (RB_HOARD: the world puts the takings in the
** hoard there, b its kind's faction); else its own home, hidden there if its kind hides its things (RB_HIDE), else
** lying there (HOUSE.md's the robbers' takings, 2026-10-27); K its own again after */
static void __fastcall__ rob_to (unsigned char k)
{
    unsigned char i, f = K.faction;

    im.w1 = ahx[k];
    im.w2 = ahy[k];
    im.b = f;
    kx_get (akind[k]);
    im.x[2] = X.keeps == KP_HIDDEN ? RB_HIDE : 0;
    for (i = 0; i < hd.nlairs; ++i) {
        lair_get (i);
        if (L.kind && (L.flags & L_PLACED) && L.sn == a.sn && ((L.flags & L_LEADER) ?
            L_FAC (L.flags) == HUB->actor[k].faction :
            (kx_get (L.kind), X.keeps == KP_LEADER) && (kind_get (L.kind), K.faction == f))) {
            im.w1 = L.hx;
            im.w2 = L.hy;
            im.x[2] = RB_HOARD;
            break;
        }
    }
    kind_get (akind[k]);
}

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
#define DRAWN_N         8                       /* (A hostile one drawn on there, its decisions: drawn) */
static unsigned char drawn[NACTORS];
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

/* Denizen D (its kind's record K) first met, as it's first made an actor: its personal things made (I_KIT's
** KT_PURSE, the world's draw: its purse, its trinkets, its valuables by its standing, the user's answer 6: made as it's
** first met, then kept), folk's only, if it has any standing (kinds.txt's) */
static void met (void)
{
    unsigned char k = D.kind, f = K.faction;

    if (KF_NATURE (K.form) != NA_FOLK || (kx_get (k), !X.standing)) {
        return;
    }
    im.a = KT_PURSE;
    im.b = f;
    im.x[0] = X.standing;
    im.x[1] = k;
    im.x[2] = D.lsn;
    im.w1 = D.hx;
    im.w2 = D.hy;
    im.t = D.id;
    kitted ();
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
    D.serve = a.serve;                          /* (Its bonds: part 6's) */
    D.contact = a.contact;
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

    k |= D.flags & D_MET ? 0 : 0x80;          /* (First met: its personal things, met's) */
    D.flags |= D_ACTOR | D_MET;
    den_put (i);
    kind_get (D.kind);
    if (k & 0x80) {
        k &= 0x7F;
        met ();
    }
    akind[k] = D.kind;
    stn[k] = nowq;                              /* (Its stay from now, if it's the square the entry's last was on) */
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
    a.faction = D.fac;
    a.did = KF_NATURE (K.form) >= NA_FOLK ? D.id : 0;  /* (Its things': filed at its lair's home, the outfits' gear;
                                                **   folk's and the dead's, s_place's I_KIT; a beast none) */
    a.hx = D.hx;
    a.hy = D.hy;
    a.hsn = D.lsn;
    aorder[k] = D.order;
    sgen[k] = 0;
    alert[k] = 0;
    intr[k] = seenfoe[k] = anear[k] = 0;
    lsx[k] = lsy[k] = 0;
    fled[k] = 0;
    strcpy (rq.text, K.key);
    rq.w1 = i;
    ask (RS_NEW_ACTOR, k + 1, 0);
    a.lead = s->lead;                           /* (Its influence and might, the referee's; its bonds its record's: part
                                                **   6's morale) */
    a.power = s->power;
    a.serve = D.serve;
    a.contact = D.contact;
    im.b = (s->cond & C_BLEEDING) != 0;
    strcpy (a.name, (i = name_of (D.id)) < NNAMES ? hd.names[i].name : K.name);
    a.kind = D.kind;
    kx_get (D.kind);
    a.body = X.body;                            /* (The words for its getting up and falling: the UI's) */
    a.remains = K.flags & K_BONES ? KI_BONES : KI_BODY;
    a.timid = (K.flags & K_SHY) | (K_HELP (K) & H_KILLS ? RS_KILLER : 0);   /* (K_SHY the referee's 1) */
    hub_map (0);
    memcpy (HUB->actor[k].name, a.name, sizeof a - offsetof (struct actor, name));
    im.x[0] = s->cond & C_UNCONSCIOUS ? DO_DOWN : 0;   /* (Left senseless, and senseless still: it lies there) */
    im.x[1] = a.face;
    im.x[2] = a.sn;
    im.x[3] = s->cond >> 12 & 7;                /* (Its legs, the referee's C_MOVE: slice 4.1b's) */
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
static unsigned char loud;                      /* (A noise of the character's asked of: hear's) */
static unsigned char __fastcall__ senses (unsigned char k)
{
    hub_map (0);
    HUB->dsense.who = k;
    HUB->dsense.sight = K.sight;
    HUB->dsense.dark = K.dark;
    HUB->dsense.hear = K.hear;
    HUB->dsense.eyes = K.eyes;
    HUB->dsense.nose = (knose[akind[k] % SND_KINDS] + pcstay) / 3;  /* (Its smell's reach in still air: a third of
                                                **   its nose, squares, the character's stay a point or two more) */
    HUB->dsense.noise = loud;                   /* (0: the character as it is; a noise of its: heard, hear's) */
    hub_call (&HUB->dsense, T_SENSES);
    HUB->dsense.state = REQ_FREE;
    if (!loud) {
        slk[k] = HUB->dsense.slack;
    }
    return (HUB->dsense.res & (SN_SEEN | SN_HEARD | SN_SMELT)) != 0;
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
    wt = pcrest ? 6000 + random (6000) : 60 + random (60);  /* (The character resting: a coarse move ten or twenty
                                                **   minutes apart, as those away from it move, roam's: the long
                                                **   durations' coarser ticks, 2026-10-28) */
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
** loudness over two (the character's by the senses' sound layer, slice 4.5's: round corners, muffled by a door, a
** wall, masked by the background where it is): it turns to face the noise, stands a while, and looks for the
** character again at once */

static unsigned wplace, dplace, fplace;         /* (Its places in the world's events, and in the denizens': the noises',
                                                **   the blows') */

static void hear (void)
{
    static struct msg m;
    static struct actor* q;
    static unsigned char k, src, sn;

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
            if (src ? apart (q->x - m.w1, q->y - m.w2) > K.hear * loud / 2 : !(senses (k) & 1)) {
                continue;                       /* (The character's: the senses' to say, loud as it is) */
            }
            hub_map (0);
            rface = bearing (m.w1 - q->x, m.w2 - q->y);
            alert[k] = now + 100;
            if (!src) {
                sgen[k] = 0;                    /* (It senses again: the character, if it's there to be sensed; not
                                                **   at another's noise, a fellow's every step: the senses' work
                                                **   for each, slice 4.5's, a rat's steps a storm of it) */
            }
            rouse (k, 1);
        }
    }
    loud = 0;
    settle ();
}

/* ---- Fights (the referee's, RS_ATTACK: the ruleset's melee): the world's to strike, every blow.  A hostile actor
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

/* Actor k dead (the hub's entry as it is: its body the world's, gone): its record so, the entry free; its lair's
** keeping one keeper the fewer (none left: gone, kept no longer, the world's to see to its torches) */
void __fastcall__ s_killed (unsigned char k)
{
    struct keep* q;
    unsigned char i;

    write_back (k);
    D.flags = (D.flags & ~(D_ACTOR | D_OUT)) | D_DEAD;
    den_put (a.den);
    HUB->actor[k].name[0] = 0;
    hub_map (DEN_BANK);
    for (i = KEPT_AT->n, q = KEPT_AT->at; i; --i, ++q) {
        if (q->lid == D.lid && q->live && !--q->live) {
            ++KEPT_AT->gen;
        }
    }
    hub_map (0);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The blows the world's struck since the last look (E_FIGHT, the denizens' events), taken to heart, in turn: 1 if
** there were any */
static struct msg fm;                           /* (The blow at hand: harmed reads it) */
#define m fm
static unsigned char fought (void)
{
    unsigned char k, f, att, n = 0, g;

    while (hub_dnext (&fplace, &m)) {
        if (m.kind != E_FIGHT) {
            continue;
        }
        n = 1;
        f = m.x[2];
        att = m.who;
        k = ((att ? att : m.a) - 1) & 7;        /* (The actor in it: the character fights them one at a time) */
        if (m.x[1] != FO_BLED && (f & FF_FLEES)) {
            fled[k] = FLEE_T;                   /* (Broken: it flees, and won't seek the fight again) */
            HUB->actor[k].attitude = AT_UNFRIENDLY;
        }
        if ((g = f & (att ? FF_A_DEAD | FF_A_DOWN : FF_D_DEAD | FF_D_DOWN)) != 0) {
            dismay (k | g << 4);                /* (Its kin's courage, how it fell in the high bits: part 6's; bled to
                                                **   death too, a fall all the same) */
        }
        if (m.x[1] != FO_BLED && !att) {
            harmed (k);                         /* (Its own, and their foes, think again of the character; its
                                                **   wound seen) */
        }
        if (f & (att ? FF_A_DEAD : FF_D_DEAD)) {
            killed (k);
        }
    }
    return n;
}
#undef m

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

/* The way actor k (the copy a, its kind K's) follows the character's scent where it doesn't sense it (a hunter by
** smell, K_NOSE: DUNGEON.md's traces): the character's trail, the world's journal of its passings (the hub's traces'
** journal, TJ_AT: its last NTRAIL squares), the freshest square of it beside the actor (or under it) its nose can
** still smell (hub.h's SCENT_LIFE, of the character's smell, by its nose: a hound's days, a rat's most of a day), and
** on along it to the next: the step toward that (step_for's); or, an older trail, the world's to find (STEP_SCENT:
** the traces' store, which keeps it all, each square's scent the longer for its passings and the character's stays
** there: hub.h's TR_MORE), 0xFB, at once if the last one led it on, else once each ten seconds (sniffed: till then);
** 0xFF, neither (part 5's, the main RAM's room: slice 4.5's) */
static unsigned long sniffed[NACTORS];
#pragma code-name (push, "SW5CODE")
#pragma rodata-name (push, "SW5RODATA")
unsigned char __fastcall__ s_scent (unsigned char k)
{
    struct tstep* e;
    unsigned n, i;
    unsigned long life;
    unsigned char d = 0xFF;

    life = (unsigned long) SCENT_LIFE (pcsmell, knose[akind[k] % SND_KINDS]) * TQ;
    hub_map (DEN_BANK);
    n = TJ_AT->pn;
    for (i = 0; i < NTRAIL && i < n; ++i) {     /* (The newest first) */
        e = &TJ_AT->p[(n - 1 - i) % NTRAIL];
        if (now - e->t > life) {
            break;
        }
        if (e->sn == a.sn && apart (e->x - a.x, e->y - a.y) <= 1) {
            if (i) {
                e = &TJ_AT->p[(n - i) % NTRAIL];   /* (The next it went to) */
                refx = e->x;
                refy = e->y;
                d = step_for (k, 2 * K.range + 8, S_TOWARD);
                refx = pcx;
                refy = pcy;
            }
            break;
        }
    }
    hub_map (0);
    if (d == 0xFF && (now >= sniffed[k] || (HUB->actor[k].doing == DO_COMING && !sres[k]))) {
        sniffed[k] = now + 100;
        d = 0xFB;                               /* (The store sniffed: an older trail, if there's one) */
    }
    return d;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* Actor k's stay where it stands (the copy a): a passing for each TQ on its square, journalled as each goes by (the
** world's traces, I_TROD's TS_STAY: its scent the stronger there, hub.h's TR_MORE), not waited for */
static void __fastcall__ stayed (unsigned char k)
{
    unsigned char q, c;

    q = (unsigned char) a.x + ((unsigned char) a.y << 4);
    if (stq[k] != q) {
        stq[k] = q;                             /* (Come onto it: its stay begun) */
        stn[k] = nowq;
    } else if ((c = nowq - stn[k]) != 0) {
        stn[k] = nowq;
        im.x[1] = akind[k];
        im.x[2] = a.sn;
        im.w1 = a.x;
        im.w2 = a.y;
        im.a = TS_STAY;
        im.b = c;
        trodden ();
    }
}

/* Whether actor k shoots at the character it senses further off than beside it (slice 4.7's): a launcher of its own
** and its missiles (the world's agear, AG_SHOOTS) and a shot to be had (AG_NOSHOT, the world's: none from where it
** stands, but the character's moved since it last shot or meant to, shat's); 1, facing it, the world's to say if it
** can.  Part 2's (the main RAM's room) */
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")
unsigned char __fastcall__ s_shoots (unsigned char k, unsigned char ag)
{
    unsigned char h = (unsigned char) (pcx + 7 * pcy);

    if (shat[k] != h) {
        ag &= ~AG_NOSHOT;
    }
    if ((ag & (AG_SHOOTS | AG_NOSHOT)) != AG_SHOOTS) {
        return 0;
    }
    shat[k] = h;
    a.face = bearing (ddx, ddy);
    return 1;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* What actor k means to do, by its kind, its attitude and what it senses, said (its intent: the world does it, by the
** character's rules, or strikes its blow) and waited for, and the blows struck taken to heart (fought): kept off a
** foe; an ally's way; as it senses the character, meets's (a foe beside it: a blow); a noise heard, still a while;
** else it wanders about its lair (or keeps to it).  1 if all it said was a wait, not waited for (a.ready the time it
** will be free, as the world will have it) */
static unsigned char __fastcall__ decide (unsigned char k)
{
    unsigned char d = 0xFF, was, foe, ag;

    kind_get (akind[k]);
    memcpy (&a, &HUB->actor[k], sizeof a);
    ag = HUB->agear[k];                         /* (Its things, the world's: a launcher, a shot, slice 4.7's) */
    stayed (k);
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
        drawn[k] = DRAWN_N;
        feel (k);
    }
    anear[k] = a.sn == pcs && dist <= 3;
    if (foe && pchelp && !(K_HELP (K) & H_KILLS)) {
        foe = 0;                                /* (Its foe down, helpless: by its nature (the user's, 2026-10-21), a
                                                **   killer strikes on (the referee finishes one senseless); a robber
                                                **   comes up to it and robs it, then goes; one that only defended
                                                **   itself goes, unfriendly after: kinds.txt's ways) */
        if (sres[k] && !fled[k] && (!(K_HELP (K) & H_ROBS) || dist <= 1)) {
            if (K_HELP (K) & H_ROBS) {
                a.doing = DO_FLEE;              /* (Off with what it takes, to its lair: the world's) */
                rob_to (k);
                act (I_ROB, k);
            }
            a.attitude = AT_UNFRIENDLY;
            fled[k] = FLEE_T;
        }
    }
    if (fled[k]) {
        --fled[k];
        a.doing = DO_FLEE;                      /* (Fleeing: away; cornered beside the character, it fights, unless
                                                **   it's helpless) */
        if ((d = step_for (k, 4 * K.range + 8, S_AWAY)) == 0xFF && dist == 1 && sres[k] && !pcdead && !pchelp) {
            d = 0xFC;
        }
    } else if (sres[k] && foe && dist == 1) {
        a.face = bearing (ddx, ddy);            /* (A foe beside it: it strikes) */
        d = 0xFC;
    } else if (sres[k] && foe && (ag & AG_SHOOTS) && a.sn == pcs && shoots (k, ag)) {
        d = 0xFA;                               /* (A foe it senses further off, a shot: part 2's, slice 4.7's; asked
                                                **   only of one with a launcher, the part's copy the dearer) */
    } else if ((d = keep_off (k)) != 0xFF) {
        a.doing = sres[k] ? DO_WATCH : DO_ROAM;
    } else if (a.attitude == AT_ALLIED) {
        d = ally (k);
    } else if (sres[k]) {
        d = meets (k, was);
    } else if (foe && (K.flags & K_NOSE) && (d = scent (k)) != 0xFF) {
        a.doing = DO_COMING;                    /* (Its nose to the character's trail: after it, the way the scent
                                                **   goes; the world's, an older trail: DO_ROAM, if there's none) */
    } else if (foe && !(K.flags & K_SHY) && drawn[k] && (d = draw_on (k)) != 0xFF) {
        a.doing = DO_COMING;                    /* (Drawn on to where it last sensed the character, heard it, a while:
                                                **   slice 4.5's, HOUSE.md's: part 5's) */
    } else if (alert[k] > now) {
        d = 0xFE;                               /* (A noise heard: still, facing the way it came, a while) */
        wt = 10 + random (20);
    } else if (turns && !(*at (a.sn, a.x, a.y) & T_VIS)) {
        coarse (k, K.flags & K_LAIR ? 2 : K.range);
        d = 0xFE;                               /* (Unseen, unaware: a coarse move) */
    } else if (!pcrest && (K.flags & K_LAIR ? !random (4) : random (3))) {
        d = step_for (k, K.flags & K_LAIR ? 2 : K.range, S_ANY);
    }                                           /* (The character resting: no idle wandering in its sight) */
    if (d == 0xFF) {                            /* (Watching: a while, unless it changes; fleeing, a moment; the
                                                **   character resting, thirty times the while, a watcher's or an idler's:
                                                **   roused all the same as it changes, tend's) */
        wt = a.doing == DO_FLEE ? 10 + random (20) : a.doing ? 60 + random (60) : 10 + random (40);
        if (pcrest && a.doing != DO_FLEE) {
            wt *= 30;                           /* (A watcher's three to six minutes, an idler's one or two) */
        }
    }
    hub_map (0);
    HUB->actor[k].attitude = a.attitude;        /* (Its mind's own, as it decides) */
    HUB->actor[k].foe = a.foe;
    if (!foe || !(K.flags & K_NOSE) || a.doing != DO_COMING) {
        stalk[k] = 0;                           /* (A hunter coming at the character stalks it: its Stealth tested as
                                                **   it begins, D15's, every actor's; its grade the steps' loudness,
                                                **   no noise of its own: HOUSE.md's sound and stealth) */
    } else if (!stalk[k]) {
        stalk[k] = stalks (k);
    }
    if (d < 4 || d == 0xFB) {                   /* (A step, now and then a noise of its own; its kind with no
                                                **   darkvision, feeling its way in the dark: the world's, STEP_FEELS;
                                                **   the way the scent goes, the world's to find) */
        im.w1 = d == 0xFB ? STEP_SCENT : H_OF (d);
        im.w2 = K.dark ? 1 : 1 | STEP_FEELS;
        if (d == 0xFB) {
            im.x[2] = knose[akind[k] % SND_KINDS];
        }
        im.a = ++steps[k] & 7 || stalk[k] ? DE_MOVED : DE_NOISE;
        im.b = stalk[k] ? stalk[k] - 1 : steps[k] & 7 ? 1 : 3;
        act (I_STEP, k);
    } else if (d == 0xFD) {                     /* (A dash past the character, louder) */
        im.w1 = dsx;
        im.w2 = dsy;
        im.a = DE_MOVED;
        im.b = 2;
        act (I_GOTO, k);
    } else if (d >= 0xFA && d <= 0xFC && d != 0xFB) {   /* (A round's blow, or a shot: outnumbering it, later) */
        if (was != DO_FIGHT && K.mind >= M_LOW) {
            heart (k);                          /* (A mind's heart as it engages: the field about it, its bonds; part
                                                **   6's.  A beast's is reckoned as one near it falls: it feels only a
                                                **   foe's might, and its fellows' falls) */
        }
        a.doing = DO_FIGHT;
        im.a = 0;
        im.b = d == 0xFA ? Z_MID | AIM_SHOT : Z_MID;
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
#define NOW_HI          (((unsigned*) &now)[1]) /* (The clock's high word: 65536 tenths, near enough two hours) */
#define DAY_HI          13                      /* (A day of them, near enough) */
static unsigned aged;                           /* (The bonds aged to then, the high word: part 6's age) */

static unsigned char roam (void)
{
    struct den* q = POP->den;
    unsigned char i, lim, n = 0;
    unsigned x, y;

    if (now < roam_at) {
        return 0;
    }
    if (NOW_HI - aged >= DAY_HI) {
        age ();                                 /* (The bonds aged by the clock, a day at a time: part 6's) */
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
            im.x[1] = D.kind;                   /* (Its passing: the world's traces, kept where it holds the chunk) */
            im.x[2] = D.sn;
            im.w1 = x;
            im.w2 = y;
            im.a = D.face;
            im.b = 0;                           /* (A passing, no stay) */
            trodden ();
        }
        x = pcrest ? 10 * ROAM_T : ROAM_T;      /* (The character resting: ten times the while, the long durations'
                                                **   coarser ticks) */
        D.ready = now + x + random (x);
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
    static unsigned char ls = 0xFF, fresh, lgen, lrest;
    struct actor* q;
    unsigned long r;
    unsigned char k, n, again = struck, woke = lgen != sense_gen, up = lrest > pcrest;

    lgen = sense_gen;
    lrest = pcrest;                             /* (up: the character's rest over, the idle roused too: their waits a
                                                **   rest's long ones) */
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
        if (!turns && HUB->ui_cmd && gw != 2) {
            break;                              /* (The UI's command come in meanwhile, as they decide, though not
                                                **   seen yet: the rest decide next time round, its answer first) */
        }
        if (!q->name[0]) {
            continue;
        }
        if (woke && (q->doing || up) && q->doing < DO_FIGHT && q->ready > now + 10) {
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

#pragma code-name (push, "SW5CODE")
#pragma rodata-name (push, "SW5RODATA")

/* A hunter's stalk begun (decide's, slices 4.5 and 4.6: HOUSE.md's sound, the air and moving quietly): its Stealth
** tested (the referee's, by its sheet, in the denizens' slot), its steps' loudness plus 1 by the grade: a success 0,
** a marginal failure 1, a critical one 2 */
unsigned char __fastcall__ s_stalks (unsigned char k)
{
    rq.a = P_STEALTH;
    rq.steps = 0;
    ask (RS_TEST, k + 1, 0);
    return (signed char) rq.grade > 0 ? 1 : rq.grade == G_MF ? 2 : 3;
}

/* Actor k (the copy a), hostile, drawn on to where it last sensed the character (decide's: heard it, seen it, smelt
** it, and lost it), a while (DRAWN_N decisions): its step that way, or 0xFF (there, or no way nearer) */
unsigned char __fastcall__ s_draw_on (unsigned char k)
{
    --drawn[k];
    refx = lsx[k];
    refy = lsy[k];
    return step_for (k, K.flags & K_LAIR ? 2 : 2 * K.range + 8, S_TOWARD);   /* (Not far from its lair: one that
                                                **   keeps to it, a step or two) */
}

#pragma code-name (pop)
#pragma rodata-name (pop)

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
    if ((fm.x[1] == O_HIT || fm.x[1] == O_BOTH_HIT) && !(fm.x[2] & FF_STOPPED)) {
        q[k].mood |= AM_HURT;                   /* (Wounded, as its fellows see it: part 6's field) */
    }
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
        if (r->kind && !(r->flags & D_ACTOR) && r->fac == f) {
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

/* Words to sway actor k (sw's how): by its mind (nothing sways the mindless; an animal only by P_ANIMALS, within
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
        if (!ftongue[HUB->actor[k].faction & (NFACT - 1)][0]) {
            sw.result = SR_TONGUE;
            return;
        }
        strcpy (r->text, ftongue[HUB->actor[k].faction & (NFACT - 1)]);
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
            if (q->name[0] && q->attitude == AT_ALLIED && q->sn == pcs && apart (q->x - pcx, q->y - pcy) <= 12 &&
                q->doing != DO_DOWN) {          /* (Not one lying senseless: it hears nothing) */
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
        switch (q->doing == DO_DOWN && sw.kind ? 0xFF : sw.kind) {
        case 0xFF:
            sw.result = SR_DOWN;                /* (Senseless: it hears nothing, it takes no request, till it comes
                                                **   round, 2026-10-29) */
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

/* The swapped parts' images read into banks of its own (dir/dq0d.ovl: dq0d.cfg's OV1, OV2, OV3): 0, or -1 */
static int __fastcall__ ovl_load (const char* dir)
{
    unsigned char* b = (unsigned char*) _SWAP_START__;  /* (Through the parts' area: none in it yet) */
    unsigned off;
    int fd, n;

    strcpy ((char*) b, dir);
    strcat ((char*) b, "/dq0d.ovl");
    if ((fd = open ((char*) b, O_RDONLY)) < 0 || (n = hy_banks_alloc (3)) < 0) {
        return -1;
    }
    ovbank[0] = n;
    ovbank[1] = n + 1;
    ovbank[2] = n + 2;
    for (off = 0; off < 0x6000 && (n = read (fd, b, 512)) > 0; off += n) {
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
    struct bank_fn* h;
    unsigned char f, k;
    int fd;

    if (kinds_load (dir)) {
        return -1;
    }
    hub_map (0);
    if (HUB->savedir[0] && strlen (HUB->savedir) + sizeof THEME_FILE < sizeof pgthm && strlen (dir) + 12 < sizeof pgbp &&
        (fd = open (strcat (strcat (strcpy (pgthm, HUB->savedir), "/"), THEME_FILE), O_RDONLY)) >= 0) {
        close (fd);                             /* (A theme file: its inhabitants, if it has any, popgen's) */
        pga.lib = dir;
        pga.theme = pgthm;
        pga.att = piece;                        /* (Its factions' attitudes, relations, first kinds: through the
                                                **   files' pieces, taken only if there are any) */
        pga.rel = piece + POP_NFACT;
        pga.first = piece + POP_NFACT * (POP_NFACT + 1);
        pga.lair = pgl;
        if (bank_setup (strcat (strcpy (pgexp, dir), "/dq0d.exp"), 0x60, 16) ||
            (h = bank_fn (strcat (strcpy (pgbp, dir), "/popgen.bp"), "pg_load")) == 0 ||
            !bank_call (h, unsigned char __fastcall__ (*) (struct pgarg*), &pga) ||
            (pgs = bank_fn (pgbp, "pg_stratum")) == 0) {
            bank_flush ();                      /* (None: its bank given back, the stand-in's populations) */
        } else {
            memcpy (fatt0, piece, NFACT);
            memcpy (frel, piece + POP_NFACT, NFACT * NFACT);
            for (f = 1; f < NFACT; ++f) {       /* (Each its first kind's stand-in faction's tongue, from its record:
                                                **   the history's factions past a stand-in's 7 too, 2026-10-29) */
                kind_get (piece[POP_NFACT * (POP_NFACT + 1) + f]);
                hy_bank (kb);
                k = K.faction < KINDS[0].styles ? K.faction : 0;
                memcpy (ftongue[f], ((struct faction*) (KINDS + nkinds) + k)->tongue, sizeof ftongue[0]);
                hub_map (0);
            }
        }
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
    static unsigned char lpl, lpn;
    static unsigned long done_at;
    static unsigned char gone, g, act;
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
        if (HUB->cq.seq != gsq) {
            gsq = HUB->cq.seq;                  /* (A new line: given way to again, though the last's outlasted
                                                **   the wait, its command not let go in between) */
            gw = 0;
        }
        if (!turns && HUB->ui_cmd && gw != 2) {
            if (!gw) {
                gw = 1;                         /* (At the real paces, the UI's command in hand answered first: the
                                                **   minds wait, three seconds at most) */
                gwt = hub_now ();
            } else if (hub_now () - gwt > GIVE_WAY) {
                gw = 2;                         /* (Waited long enough: on, till the command's let go) */
            }
            act = gw == 2;
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
            pcbusy = HUB->pc_ready > now;       /* (Busy: not pc_noise + 1, 0 as it's 0xFF, the senses' to reckon by
                                                **   its pace, slice 4.5's fault: a busy character taken for idle,
                                                **   one senseless not helpless, struck on, never robbed) */
            pcrest = pcbusy && (HUB->pc_task.kind == LT_WAIT || HUB->pc_task.kind == LT_SLEEP) && !HUB->pc_task.asking;
            pcdead = (HUB->pcsum.cond & C_DEAD) != 0;
            pchelp = (HUB->pcsum.cond & C_STUNNED) || ((HUB->pcsum.cond & C_UNCONSCIOUS) && pcbusy);
            if (!(pclit = HUB->pc_light != 0) && DONE (pcs) && (p = at (pcs, pcx, pcy)) != 0) {
                pclit = (*p & T_VIS) != 0;      /* (Lit where it stands: a room's light, a lamp put down) */
            }
            hub_map (0);
        }
        if (act) {
            if (now >= nextq) {                 /* (The traces' clock: a TQ gone by; a long wait's coarse turn's,
                                                **   ten minutes, each of them) */
                nowq += 1 + ((unsigned) now - (unsigned) nextq) / TQ;   /* (Sixteen bits: a turn ten minutes on
                                                **   at most) */
                nextq = now + TQ;
            }
            if (pcx != lpx || pcy != lpy) {
                pcq = nowq;                     /* (Come onto a square: its stay there begun) */
                pcstay = 0;
            } else if (pcstay < 2) {
                pcstay = TR_MORE ((unsigned char) (nowq - pcq + 1));
            }
        }
        if (act && (pcx != lpx || pcy != lpy || (pclit | pcstay << 1 | (HUB->pc_going != 0) << 3 | (HUB->pc_task.kind == LT_SLEEP) << 4) != lpl ||
            (HUB->pc_going && HUB->pc_noise != lpn))) {   /* (Moving or not, and how loud: what the senses hear of
                                                **   it, asleep the quieter, slice 4.5's; not its being busy, a turn's, a word's: its
                                                **   senses' answer the same, the senses not asked again) */
            lpx = pcx;                          /* (The character changed: each actor senses it again; its stay's
                                                **   scent too) */
            lpy = pcy;
            lpl = pclit | pcstay << 1 | (HUB->pc_going != 0) << 3 | (HUB->pc_task.kind == LT_SLEEP) << 4;
            lpn = HUB->pc_noise;
            if (!++sense_gen) {
                sense_gen = 1;
            }
        }
        if (hub_posted (&HUB->sway)) {          /* (The UI's: the character's way with a denizen) */
            sway ();
        }
        grow ();                                /* (In its turn or not: the world waits for a new stratum's) */
        if (act) {
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

/* (The main RAM's: part 4's room) The places kept lit (the hub's KEPT_AT) written to the save's dens.dat, after the
** records' bank (fd), a piece at a time through the main RAM; or read back (back: nothing there, none) */
#pragma code-name (push, "CODE")
static void __fastcall__ kept_file (int fd, unsigned char back)
{
    unsigned off, n;

    for (off = 0; off < sizeof (struct kept); off += n) {
        n = sizeof (struct kept) - off < sizeof piece ? sizeof (struct kept) - off : sizeof piece;
        if (back && read (fd, piece, n) != n) {
            memset (piece, 0, n);
        }
        hub_map (DEN_BANK);
        if (back) {
            memcpy ((char*) KEPT_AT + off, piece, n);
        } else {
            memcpy (piece, (char*) KEPT_AT + off, n);
            write (fd, piece, n);
        }
    }
    hub_map (0);
}
#pragma code-name (pop)

/* ---- The records kept (back: read again), the save folder's dens.dat: the bank a piece at a time, through the
** main RAM (the actors' places written back first; read back, none an actor yet; none read but this version's); and
** after it the places kept lit (the hub's KEPT_AT) */
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
    kept_file (fd, back);
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

/* ==== Part 6: morale (HOUSE.md's morale, for review; DECISIONS.md's the user's of 2026-10-29: "any intelligent
** creature should modify the morale field around itself").  Each intelligent actor (a mind low, average or cunning:
** no beast, nothing mindless) sways the courage of those about it by its influence: its rank in its faction (kinds.txt's:
** the rank and file, a captain, a lord or a spiritual leader) and its influence skill (the referee's summary's lead:
** Oratory or Rhetoric), for its friends (its kind, its faction) and against its foes (a faction hostile to theirs: and
** any foe mightier than one, by its power, its fearsomeness), less with distance, by how it's doing (wounded, half;
** berserk, twice; fleeing, a friend's turned against them; fallen, gone; a foe's either, none); the character too, by
** its own (its allies its friends, those hostile to it its foes).  A friend's counts the more as one has been long with its
** band (its time served and its contact: the bonds about it).  And bonds upward: each subordinate's to its leaders
** (its band's head in its lair, its faction's above that, wherever they are), by the leader's rank, the time they've
** had together (the leader's own a new leader's less) and how much they're together (contact: a band living with its
** leader every day, others each time they're near; aged by the clock).  All of it is reckoned only as a test is made
** (or may be: an actor's heart, as it engages, and as one near it falls), never kept up as things go: the field and
** the bonds held to the book's band (twenty points either way), the situation besides (a friend fallen about it, the
** book's bad situation; none left standing, desperate).  A fall (dismay): each of the fallen's kin that knows of it
** (hears the fight's din: its hearing, a fight as loud as the world's blows; not one senseless) tests at once (not one
** fleeing already), the fallen's influence gone, its bond to it gone if it was its leader (dead: known lost from then,
** its record's D_LOST; one that doesn't know of it keeps its bond, the fall unfelt till it's known: told, the hook). */
#pragma code-name (push, "SW6CODE")
#pragma rodata-name (push, "SW6RODATA")
#pragma bss-name (push, "SW6RODATA")            /* (Its locals the part's own: the main RAM's room) */
#pragma register-vars (push, on)                /* (Its records' pointers in the zero page) */

#define FIELD_R         8                       /* Squares an actor's influence reaches (all of it beside one, half at
                                                **   five, falling off with distance: falloff) */
#define FALL_LOUD       5                       /* A fall's din (the world's blows', wfight.c's LOUD_FIGHT) */
#define WEEK_HI         92                      /* A week, the clock's high words (65536 tenths each) */
#define FULL_WEEKS      128                     /* The weeks together that make a bond's time whole (some two and a
                                                **   half years: the bonds' scale, 0-128) */
#define CONTACT_UP      16                      /* A bond's contact the more each time an actor's near its leader */
#define FIELD_MAX       20                      /* The field and the bonds held to the book's band (points) */
#define SEEN_SLAIN      10                      /* A friend seen to fall: the book's bad situation (points), */
#define DESPERATE       20                      /*   none of its friends left standing about it, desperate */

static const unsigned char rankw[3] = { 2, 5, 10 };    /* A rank's weight (points): the rank and file, a captain, a lord
                                                **   or a spiritual leader (its influence, a sixteenth of its skill
                                                **   more; and its bonds' whole) */
static const unsigned char falloff[FIELD_R + 1] = { 255, 255, 205, 171, 146, 128, 114, 102, 93 };   /* (An
                                                **   influence's 256ths at each distance: four over the squares and
                                                **   three) */
static unsigned tb;                             /* (times') */
static unsigned char krank[SND_KINDS];          /* (Each kind's rank: ranks') */
static unsigned weeks;                          /* (The game's weeks gone) */
static unsigned char l1, l2;                    /* (A record's leaders, chain's: its band's head, its faction's above it;
                                                **   0xFF none) */
static struct actor tp, tq;                     /* (The one reckoned for, and another about it: copied out) */
static unsigned char gi;                        /* (The actor grab copies) */
static unsigned char rj, rfallen, rgone;        /* (reckon's: the one reckoned for, the one fallen, the bond gone) */
static unsigned char js, jc, jq;                /* (Its time served and its contact, 0-128 each, and its band's
                                                **   familiar: their mean) */
static unsigned char fw, fd;                    /* (An influence's weight, points, and its distance) */
static int rf;                                  /* (The field as it's reckoned) */
static signed char mf, mb;                      /* (The last reckoning's field and bonds, points) */
static unsigned char nfr, ndown;                /* (And its friends standing about it, and down) */

/* Each kind's rank, and the weeks gone (each entry's first: the part's data are its own while it's in) */
static void ranks (void)
{
    static unsigned char k;

    for (k = 0; k < nkinds && k < SND_KINDS; ++k) {
        kx_get (k);
        krank[k] = X.rank;
    }
    weeks = NOW_HI / WEEK_HI;
}

/* a times tb (16 bits: no library multiplying for the main RAM to keep) */
static unsigned __fastcall__ times (unsigned char a)
{
    static unsigned r, s;

    for (r = 0, s = a; tb; tb >>= 1, s <<= 1) {
        if (tb & 1) {
            r += s;
        }
    }
    return r;
}

/* A bond's time (0-128): w weeks served as the game began, the game's weeks since on them, FULL_WEEKS at most */
static unsigned char __fastcall__ tpart (unsigned char w)
{
    static unsigned t;

    t = w + weeks;
    return t >= FULL_WEEKS ? FULL_WEEKS : t;
}

/* Actor gi copied out (into t), the hub's header in the window */
static void __fastcall__ grab (struct actor* t)
{
    hub_map (0);
    memcpy (t, &HUB->actor[gi], sizeof *t);
}

/* Record d's leaders (l1, l2): the highest above its rank in its own lair (its band's head), and its faction's highest
** above that elsewhere (wherever it is); a dead one counted while it doesn't know of a loss (D_LOST); D its record's
** after, the hub's header in the window */
static void __fastcall__ chain (unsigned char d)
{
    register struct den* q = POP->den;
    static unsigned char i, b1, b2, r, dead;

    den_get (d);
    b1 = b2 = krank[D.kind & (SND_KINDS - 1)];
    dead = D.flags & D_LOST ? D_DEAD : 0;       /* (Dead and known: no leader) */
    l1 = l2 = 0xFF;
    hy_bank (pb);
    for (i = 0; i < hd.ndens; ++i, ++q) {
        if (q->kind && q->fac == D.fac && i != d && !(q->flags & dead)) {
            r = krank[q->kind & (SND_KINDS - 1)];
            if (q->lid == D.lid) {
                if (r > b1) {
                    b1 = r;
                    l1 = i;
                }
            } else if (r > b2) {
                b2 = r;
                l2 = i;
            }
        }
    }
    hub_map (0);
    if (b2 <= b1) {
        l2 = 0xFF;                              /* (Not above its band's head) */
    }
}

/* The bond (points) to leader record l (0xFF none, or rgone: 0) of the one reckoned for (js, jc): the leader's rank's
** weight by the time they've had together (the lesser of its service and the leader's own) and how much they're
** together */
static unsigned char __fastcall__ bond (unsigned char l)
{
    register struct den* q;
    static unsigned char t;

    if (l == 0xFF || l == rgone) {
        return 0;
    }
    hy_bank (pb);
    q = &POP->den[l];
    t = q->serve;
    fw = rankw[krank[q->kind & (SND_KINDS - 1)]];
    hub_map (0);
    t = tpart (t);
    tb = (t < js ? t : js) + jc;
    return times (fw) >> 8;
}

/* A friend's influence fw (points) at fd, for the one reckoned for (turned: against it), its band's familiar the more */
static void __fastcall__ by (unsigned char turned)
{
    tb = 128 + jq;
    fw = times (fw) >> 8;
    tb = falloff[fd];
    fw = times (fw) >> 8;
    rf += turned ? -fw : fw;
}

/* A foe's weight (points) against the one reckoned for: its influence fw, and an eighth of its might (power pw) over
** the other's; wounded, half */
static void __fastcall__ against (unsigned char pw)
{
    fw += pw > tp.power ? (unsigned char) (pw - tp.power) >> 3 : 0;
    tb = falloff[fd];
    fw = times (fw) >> 8;
    rf -= fw;
}

/* Actor rj's heart (points): the field about it and its bonds, held to the book's band, the situation besides (a friend
** fallen: rfallen the actor fallen, 0xFF none, or one lying senseless); rgone a leader's record whose bond's taken away
** (0xFF none).  mf, mb, nfr after; K its kind's, the hub's header in the window */
static signed char reckon (void)
{
    static unsigned char smart, nl, b;

    gi = rj;
    grab (&tp);
    js = tpart (tp.serve);
    jc = tp.contact >> 1;
    jq = (js + jc) >> 1;                        /* (Its bonds about it: how familiar its band is) */
    kind_get (akind[rj]);
    l1 = l2 = 0xFF;
    if ((smart = K.mind >= M_LOW) != 0) {
        chain (tp.den);
    }
    rf = nfr = ndown = nl = 0;
    for (gi = 0; gi < NACTORS; ++gi) {
        grab (&tq);
        if (gi == rj || !tq.name[0] || tq.sn != tp.sn || (fd = apart (tq.x - tp.x, tq.y - tp.y)) > FIELD_R) {
            continue;
        }
        kind_get (akind[gi]);
        fw = smart && K.mind >= M_LOW ? rankw[krank[akind[gi] & (SND_KINDS - 1)]] + (tq.lead >> 4) : 0;   /* (A mind
                                                **   sways a mind: a beast feels only a foe's might, and its fellows'
                                                **   falls) */
        if (tq.mood & AM_BERSERK) {
            fw <<= 1;                           /* (Berserk, twice; wounded, half) */
        }
        if (tq.mood & AM_HURT) {
            fw >>= 1;
        }
        b = gi == rfallen || tq.doing == DO_DOWN ? 2 : tq.doing == DO_FLEE || fled[gi];   /* (Fallen, 2; fleeing) */
        if (tq.faction == tp.faction || (akind[gi] == akind[rj] && REL (tp.faction, tq.faction) != AT_HOSTILE)) {
            if (b == 2) {
                ++ndown;                        /* (A friend fallen: its influence gone, the book's bad situation) */
            } else {
                by (b);                         /* (A friend: fleeing, its influence turned) */
                if (!b) {
                    ++nfr;
                    nl |= tq.den == l1 || tq.den == l2;
                }
            }
        } else if (!b && REL (tp.faction, tq.faction) == AT_HOSTILE) {
            against (tq.power);
        }
    }
    hub_map (0);
    if (!pcdead && pcs == tp.sn && (fd = apart (pcx - tp.x, pcy - tp.y)) <= FIELD_R) {
        fw = smart ? rankw[RK_FILE] + (HUB->pcsum.lead >> 4) : 0;   /* (The character's: the rank and file's, its own
                                                **   skill) */
        if (tp.attitude == AT_ALLIED) {
            by (0);
        } else if (tp.attitude <= AT_UNFRIENDLY && !pchelp) {
            against (HUB->pcsum.power);
        }
    }
    b = 0;
    if (smart) {
        if (nl && tp.contact < 255 - CONTACT_UP) {
            HUB->actor[rj].contact = tp.contact + CONTACT_UP;  /* (With its leader again: the bond the stronger) */
        }
        b = bond (l1) + bond (l2);
    }
    rf = rf < -FIELD_MAX ? -FIELD_MAX : rf > FIELD_MAX ? FIELD_MAX : rf;
    mf = rf;
    mb = b;
    rf += b;
    rf = rf < -FIELD_MAX ? -FIELD_MAX : rf > FIELD_MAX ? FIELD_MAX : rf;
    if (ndown) {
        rf -= nfr ? SEEN_SLAIN : DESPERATE;     /* (A friend fallen: the book's bad situation; none left standing about
                                                **   it, desperate: beyond the field's band) */
    }
    kind_get (akind[rj]);
    return rf;
}

/* Every actor's heart anew (rfallen the one fallen: 0xFF none) */
static void hearts (void)
{
    rgone = 0xFF;
    for (rj = 0; rj < NACTORS; ++rj) {
        if (rj != rfallen && (hub_map (0), HUB->actor[rj].name[0])) {
            HUB->actor[rj].heart = reckon () / 5;
        }
    }
}

/* Actor rj's courage tested (RS_MORALE, in the task's own slot): its heart (rfallen, rgone: reckon's), why (MR_*): broken
** (or shaken, a shy kind), it flees; berserk, so its fellows see it */
static void __fastcall__ courage (unsigned char why)
{
    rq.steps = reckon () / 5;
    rq.a = why;
    rq.b = mf;
    rq.c = mb;
    rq.w1 = rgone != 0xFF ? rgone : rfallen;
    rq.w2 = l1 | l2 << 8;
    ask (RS_MORALE, rj + 1, 0);
    if (rq.result == M_BROKEN || (rq.result == M_CAUTIOUS && (K.flags & K_SHY))) {
        fled[rj] = FLEE_T;                      /* (Its doing and its attitude its next turn's: fleeing) */
    } else if (rq.result == M_BERSERK) {
        HUB->actor[rj].mood |= AM_BERSERK;
    }
}

/* Actor k fallen (its high bits how, FF_* << 4: dead, else senseless): each of its kin among the actors (its kind, its
** faction) that knows of it (hears the fight's din: within its hearing of a fight's loudness, as it hears another's
** noise; its sight never reaches further; not one senseless) tests its courage (not one fleeing already): its friend's
** fall the book's bad situation (desperate, none left standing about it), the fallen's influence gone; its leader's
** (its band's head, its faction's), its bond to it gone (dead: known lost from then).  Then every actor's heart anew.
** K its own after */
void __fastcall__ s_dismay (unsigned char k)
{
    static unsigned char j, dead, why;
    static struct actor f;

    dead = k & (FF_A_DEAD | FF_D_DEAD) << 4;   /* (Dead, else senseless) */
    rfallen = gi = k & 7;
    ranks ();
    grab (&f);
    for (j = 0; j < NACTORS; ++j) {
        gi = j;
        grab (&tp);
        if (j == rfallen || !tp.name[0] || (akind[j] != akind[rfallen] && tp.faction != f.faction) || tp.sn != f.sn ||
            tp.doing == DO_DOWN || (kind_get (akind[j]), apart (tp.x - f.x, tp.y - f.y) > (unsigned) K.hear * FALL_LOUD / 2)) {
            continue;                           /* (Unheard, or senseless: unfelt, its bond to it holding, till it's
                                                **   known) */
        }
        why = MR_FELL;
        if (K.mind >= M_LOW) {
            chain (tp.den);
            if (f.den == l1 || f.den == l2) {
                why = MR_LEADER;
                if (dead) {
                    D.flags |= D_LOST;          /* (Known lost: no bond to it from now) */
                    den_put (tp.den);
                }
            }
        }
        if (!fled[j]) {
            rj = j;                             /* (Fleeing already: it knows, and flees on) */
            rgone = f.den;
            courage (why);
        }
    }
    hearts ();
    kind_get (akind[rfallen]);
}

/* Actor k's heart as it engages (decide's: the field about it and its bonds, its steps the world's blow carries to the
** referee, its courage tested with them as it's wounded); K its own after */
void __fastcall__ s_heart (unsigned char k)
{
    rj = k;
    rfallen = rgone = 0xFF;
    ranks ();
    HUB->actor[k].heart = reckon () / 5;
}

/* Record l (0xFF none) if it's dead, else 0xFF */
static unsigned char __fastcall__ dead_of (unsigned char l)
{
    if (l != 0xFF) {
        hy_bank (pb);
        l = POP->den[l].flags & D_DEAD ? l : 0xFF;
        hub_map (0);
    }
    return l;
}

/* The hook for a loss told (a runner's word, a survivor's, to come: through a stub, SWFN told, 6): actor j learns of
** its leaders' deaths it didn't know of: known lost from then (D_LOST), its courage tested at once (MR_TOLD), its
** bond to the first of them gone.  K its own after */
void __fastcall__ s_told (unsigned char j)
{
    static unsigned char d;

    rj = j;
    rfallen = 0xFF;
    ranks ();
    hub_map (0);
    chain (d = HUB->actor[j].den);
    if ((rgone = dead_of (l1)) == 0xFF) {
        rgone = dead_of (l2);
    }
    if (rgone != 0xFF) {
        D.flags |= D_LOST;
        den_put (d);
        courage (MR_TOLD);
    }
    kind_get (akind[j]);
}

/* The bonds aged by the clock (roam's, a day or more gone since they last were): each record not an actor (an
** actor's its own while it is, each time it's near its leader), living with its leader (a lair with its faction's
** leader in it: L_LEADER) its contact whole (with it every day); else the less, an eighth a day (eight days at most) */
void s_age (void)
{
    register struct den* q;
    static unsigned lids[8];
    static unsigned char i, j, m, dn;

    if (!aged) {
        aged = NOW_HI;                          /* (The first time: from now) */
        return;
    }
    for (dn = 0; NOW_HI - aged >= DAY_HI; ++dn) {
        aged += DAY_HI;
    }
    dn = dn > 8 ? 8 : dn;
    for (i = m = 0; i < hd.nlairs; ++i) {
        lair_get (i);
        if (L.kind && (L.flags & L_LEADER) && m < 8) {
            lids[m++] = L.id;
        }
    }
    hy_bank (pb);
    for (i = 0, q = POP->den; i < hd.ndens; ++i, ++q) {
        if (q->kind && !(q->flags & (D_DEAD | D_ACTOR))) {
            for (j = 0; j < m && lids[j] != q->lid; ++j) {
            }
            if (j < m) {
                q->contact = 255;               /* (With its leader every day: as much as can be) */
            } else {
                for (j = dn; j; --j) {
                    q->contact -= q->contact >> 3;
                }
            }
        }
    }
    hub_map (0);
}

#pragma register-vars (pop)
#pragma bss-name (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
