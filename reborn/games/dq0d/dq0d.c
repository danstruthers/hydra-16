/*
** dq0d.c - Dingus Quest Zero's denizens (DENIZENS.md): a task of its own, started by the UI (dq0d SEG ROLE LIBDIR:
** the hub's segment, its role there, hub.h, and the game's lib folder).  As each stratum's plan is grown, it decides
** the stratum's populations, by its style and its depth, from the kinds a stand-in theme has (kinds.txt, made
** lib/dq0/dq0d.dat, read into a bank of its own as it starts): the lairs, where they lie in the plan, and the
** denizens of each, a record each from then on; so nothing is made up as the character turns a corner.  As the
** character comes near a lair (its chunk, or one beside it), its denizens are placed in it, each on a square of its
** own (the chunk made, the maker's, if it isn't yet); and those near the character are actors (the hub's: the world
** keeps the character out of their way, the senses see them), each deciding what it means to do by its kind and what
** it senses, and doing it by the character's rules: to wander about its lair (a few squares, or a wide range), to keep to it, to watch the character,
** to keep its distance from it (a shy one), to come towards it (a bold one).  It senses through its own senses: it
** sees the character as far as its sight goes by the character's light (or its darkvision, with none), along a line
** clear of rock, walls and shut doors; it hears it moving, nearer.  At the turns pace it keeps up with the clock, as
** the world waits for it (a slice of the clock at a time: hub.h's TURN_SLICE), so they act through all the time what
** the character does takes: those out of its sight and not sensing it coarsely, a few squares a while.  Those out
** but away from the character (not actors) move coarsely too, to squares about their lairs in chunks that are made
** (roam).  No two are on a square, nor any on the character's or where it's going.  Each has an attitude to the
** character, its faction's to begin with (factions.txt's, a stand-in theme's: a theme's will take their place),
** moved by what happens and by the character's words (the UI's request, the hub's sway: the referee's test), and acts
** by it: the hostile come at the character, an ally follows it and takes its requests; factions hostile to one
** another keep off one another.  (Their own exploring comes later.)
**
** Its records (the lairs, the denizens) are in a bank of its own, a record at a time copied out; kept with the game
** (dq0 -d: the UI asks, the hub's den_state), the save folder's dens.dat.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"

#pragma static-locals (on)                      /* (Its locals static: quicker, and nothing here is reentered) */

/* ---- The kinds: the data file's (make.js's, from kinds.txt), in a bank of their own */

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
    unsigned char   spare[5];
};
#define K_SHY           0x01                    /* It keeps its distance from the character */
#define K_LAIR          0x02                    /* It keeps to its lair */
#define K_WAYS          0x04                    /* Its lairs are by the stratum's ways in and out */
#define K_BOLD          0x08                    /* It comes towards the character */
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

/* ---- The records: the lairs and the denizens, in a bank of their own */

#define NLAIRS          64
#define NDENS           240
struct lair {
    unsigned char   sn, kind, n, first;         /* Its stratum, its kind, its denizens (n records from first) */
    unsigned char   flags;                      /* L_* */
    unsigned char   link;                       /* The stratum's link it's by, or 0xFF (a room of its chunk) */
    unsigned        cx, cy;                     /* Its chunk (from the stratum's x0, y0) */
    unsigned        hx, hy;                     /* Its home square (placed) */
};
#define L_PLACED        0x01                    /* Its denizens placed */
struct den {
    unsigned char   kind, lair;
    unsigned char   flags;                      /* D_* */
    unsigned char   sn, face;
    unsigned        x, y;                       /* Its square (placed) */
    unsigned long   ready;                      /* When it can act again */
    unsigned char   attitude;                   /* To the character (hub.h's AT_*) */
    unsigned char   order;                      /* An ally's request (hub.h's OR_*) */
};
#define D_OUT           0x01                    /* On a square of its own */
#define D_ACTOR         0x02                    /* An actor now (the hub's entry the truth while it is) */
#define NNAMES          8
struct head {
    unsigned char   done[MAXSTRATA];            /* Each stratum's populations decided */
    unsigned char   nlairs, ndens;
    unsigned char   fatt[NFACT];                /* Each faction's attitude to the character now (its newcomers') */
    struct { unsigned char den; char name[12]; } names[NNAMES];    /* The names the character's given (den 0xFF:
                                                **   none) */
};
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

/* The kinds read in (dir/dq0d.dat): 0, or -1 */
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

/* How factions f and g stand (AT_*: indifferent, either none) */
#define REL(f, g)       frel[(f) & (NFACT - 1)][(g) & (NFACT - 1)]

/* ---- The character, as the hub has it (each time round) */

static unsigned long now;
static unsigned pcx, pcy;
static unsigned char pcs, pclit, pcbusy;

static unsigned __fastcall__ apart (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dx > dy ? dx : dy;
}

/* ---- Populations: stratum n's decided as its plan is grown (its record and links whole: the hub's strata[n],
** copied): for each kind that lives in its style at its depth, its lairs (by the stratum's ways, if it lairs there;
** else in a region of halls, a chunk of it), and the denizens in each, a record each */
static void __fastcall__ populate (unsigned char n)
{
    struct stratum* s = &strata[n];
    struct region* r;
    unsigned char k, j, m, i, rx = 0, ry = 0, t, u;
    unsigned w = (s->w + CW - 1) / CW, h = (s->h + CW - 1) / CW;

    rseed (s->seed ^ 0x44454E5AUL);
    for (k = 1; k < nkinds; ++k) {
        kind_get (k);
        if (!(K.styles & (1 << s->style)) || n < K.deep0 || n > K.deep1 || !(tdens & 1UL << k)) {
            continue;
        }
        m = K.flags & K_WAYS ? K.lairs : random (K.lairs + 1);
        for (j = 0; j < m && hd.nlairs < NLAIRS; ++j) {
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
                    hub_map (PLAN_BANK (n));
                    r = PLAN_AT (n) + (unsigned) ry * s->rw + rx;
                    if (r->kind == RG_HALLS) {
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
            if (hd.ndens + L.n > NDENS) {
                break;
            }
            L.first = hd.ndens;
            memset (&D, 0, sizeof D);
            D.kind = k;
            D.lair = hd.nlairs;
            D.sn = n;
            D.attitude = hd.fatt[K.faction & (NFACT - 1)];    /* (Its faction's, as the character stands with it) */
            for (i = 0; i < L.n; ++i) {
                den_put (hd.ndens++);
            }
            lair_put (hd.nlairs++);
        }
    }
    hd.done[n] = 1;
}

/* The strata grown since the last look, populated (none while the world's asking the maker: a plan may be half
** copied into the hub) */
static void grow (void)
{
    unsigned char n;

    for (n = 0; n < MAXSTRATA; ++n) {
        hub_map (0);
        if (HUB->strata[n].made && !hd.done[n] && HUB->mreq[ASK_WORLD].state != REQ_POSTED) {
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

static void __fastcall__ place (unsigned char i)
{
    struct stratum* s;
    struct room* q;
    unsigned x0, y0, x, y, ox, oy;
    unsigned char w = 7, h = 7, j, t;

    lair_get (i);
    s = &strata[L.sn];
    rseed (s->seed + (unsigned long) i * 7919UL);
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
    for (j = 0; j < L.n; ++j) {
        for (t = 0; t < 32; ++t) {
            x = x0 + random (w);
            y = y0 + random (h);
            if (open_sq (at (L.sn, x, y)) && (L.link == 0xFF || apart (x - L.hx, y - L.hy) > 1) &&
                apart (x - pcx, y - pcy) > 1 && !rec_on (L.sn, x, y) && !taken (0xFF, x, y)) {
                break;                          /* (Not on another's, nor on the way or the character's square or
                                                **   beside them: the character comes out there, not boxed in) */
            }
        }
        den_get (L.first + j);
        if (t < 32) {
            D.x = x;
            D.y = y;
            D.flags |= D_OUT;
            D.face = random (4) * 2;
            D.ready = now + random (50);
            den_put (L.first + j);
        }
    }
    L.flags |= L_PLACED;
    lair_put (i);
}

/* Chunk (cx, cy) of stratum sn made already (in the cache: the hub's header in the window) */
static unsigned char __fastcall__ made (unsigned char sn, unsigned cx, unsigned cy)
{
    struct cslot* q = HUB->cslot;
    unsigned char k;

    hub_map (0);
    for (k = 0; k < NCSLOTS; ++k, ++q) {
        if (q->used && q->sn == sn && q->cx == cx && q->cy == cy) {
            return 1;
        }
    }
    return 0;
}

/* Square x, y of stratum sn in a chunk that's made already */
static unsigned char __fastcall__ made_at (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s = &strata[sn];

    return x - s->x0 < s->w && y - s->y0 < s->h && made (sn, (x - s->x0) >> CW_BITS, (y - s->y0) >> CW_BITS);
}

/* The lairs on the character's stratum whose chunks are its own or beside it, and made (the character's
** surroundings detailed: this task never makes a chunk to place a lair), placed: 1 if any was */
static unsigned char nearby (void)
{
    struct stratum* s = &strata[pcs];
    unsigned cx = (pcx - s->x0) / CW, cy = (pcy - s->y0) / CW;
    unsigned char i, n = 0;

    for (i = 0; i < hd.nlairs; ++i) {
        lair_get (i);
        if (L.sn == pcs && !(L.flags & L_PLACED) && L.cx + 1 - cx <= 2 && L.cy + 1 - cy <= 2 &&
            made (L.sn, L.cx, L.cy)) {
            place (i);
            n = 1;
        }
    }
    return n;
}

/* ---- The actors: the denizens near the character, in the hub for the world and the senses */

#define ACT_R           24                      /* Squares from the character a denizen's an actor */
#define CATCHUP         30                      /* Tenths an actor makes up, at most, that it's fallen behind the
                                                **   clock (the real paces': this task's late) */
#define NEAR            16

static struct actor a;
static unsigned char akind[NACTORS];            /* Each actor's kind, and its lair's home */
static unsigned ahx[NACTORS], ahy[NACTORS];
static unsigned char turns;                     /* The turns pace: this task moves its actors itself */
static unsigned char nnear;                     /* The denizens out near the character but not actors (more than
                                                **   there are entries for): their squares, each time round */
static unsigned nearx[NEAR], neary[NEAR];
static unsigned char sense_gen = 1;            /* (Bumped as the character changes: where it is, its light, busy) */
static unsigned char sgen[NACTORS], sres[NACTORS];  /* (Each actor's senses as they were last: of sense_gen, there) */
static unsigned sax[NACTORS], say[NACTORS];
static unsigned long alert[NACTORS];            /* (Each actor still and facing a noise it heard till then) */
static unsigned char steps[NACTORS];            /* (Each one's steps: a noise of its own every eighth) */
static unsigned char aorder[NACTORS];           /* (Each one's request, an ally's: its record's while it's an actor) */
static unsigned lsx[NACTORS], lsy[NACTORS];     /* (Where each last sensed the character: an ally follows it there) */
static unsigned char intr[NACTORS];             /* (Each one's lair intruded on, not yet left: its attitude fallen) */
static unsigned char seenfoe[NACTORS];          /* (Each one's seen the character with an enemy of its faction's) */
static unsigned char anear[NACTORS];            /* (Each one beside the character as it last acted: an ally comes
                                                **   along as the character goes up or down) */
static unsigned refx, refy;                     /* (The square step_for's S_AWAY and S_TOWARD go by: the character's,
                                                **   a foe's, the one an ally follows to) */

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

/* The name the character gave denizen i (its entry in the head's names), or NNAMES */
static unsigned char __fastcall__ name_of (unsigned char i)
{
    unsigned char n;

    for (n = 0; n < NNAMES && hd.names[n].den != i; ++n) {
    }
    return n;
}

/* Actor k no longer one (far from the character): its record its own again, the entry free */
static void __fastcall__ retire (unsigned char k)
{
    write_back (k);
    D.flags &= ~D_ACTOR;
    if (D.attitude == AT_ALLIED && D.order == OR_WAIT) {
        D.ready = now + 36000;                  /* (An ally left waiting: it waits an hour, then goes its own way) */
    }
    den_put (a.den);
    HUB->actor[k].name[0] = 0;
    ++HUB->den_seq;
}

/* Denizen i made an actor, in free entry k (its time from now: the coarse moves have kept it while it wasn't one) */
static void __fastcall__ enlist (unsigned char i, unsigned char k)
{
    D.flags |= D_ACTOR;
    den_put (i);
    kind_get (D.kind);
    lair_get (D.lair);
    akind[k] = D.kind;
    ahx[k] = L.hx;
    ahy[k] = L.hy;
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
    strcpy (a.name, (i = name_of (i)) < NNAMES ? hd.names[i].name : K.name);
    hub_map (0);
    memcpy (&HUB->actor[k], &a, sizeof a);      /* (Its name last: the entry used once it's whole) */
    ++HUB->den_seq;
}

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

/* Its senses: whether actor a (the copy) senses the character, dist squares off (dx, dy): seen, by its light (or
** with none, its darkvision), along a line clear of rock, walls and shut doors; or heard, moving, nearer */
static unsigned char __fastcall__ senses (unsigned dist, int dx, int dy)
{
    unsigned char* p;
    struct portal* q;
    unsigned i;

    if (a.sn != pcs) {
        return 0;
    }
    if (dist <= K.hear && pcbusy) {
        return 1;
    }
    if (dist > (pclit ? K.sight : K.dark)) {
        return 0;
    }
    for (i = 1; i < dist; ++i) {
        p = at (a.sn, a.x + dx * (int) i / (int) dist, a.y + dy * (int) i / (int) dist);
        if (!p || !T_CLEAR (*p) || (TILE (*p) == T_DOOR && (q = portal_at (cx_, cy_)) != 0 &&
            !(q->state & PS_OPEN))) {
            return 0;
        }
    }
    return 1;
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

/* Actor a (the copy) a square on heading d, as the world would move the character: a step step_for's found it can
** walk, to a square no one's on; its time by its pace and the slope (the world's cost ()) */
static void __fastcall__ step_here (unsigned char d)
{
    unsigned nx = a.x + dir_x[d], ny = a.y + dir_y[d], c = a.step;
    unsigned char* p;
    int z;

    z = elev_at (a.sn, a.x, a.y);
    p = at (a.sn, nx, ny);
    z = C->elev[cy_][cx_] - z;
    if (z > 0) {
        c += a.step * z / 2;
    } else if (z < -1) {
        c += a.step * (-z - 1) / 4;
    }
    if (TILE (*p) == T_STEPS) {
        c += a.step / 2;
    }
    a.ready += c;
    a.x = nx;
    a.y = ny;
    a.face = H_OF (d);
}

/* Actor a (the copy), out of the character's sight and not sensing it, moved coarsely (the turns pace's, catching
** up): a few squares' wander at a time, a while for them (stopped as it comes into the character's sight) */
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
        if ((p = at (a.sn, x, y)) != 0 && (*p & T_VIS)) {
            break;
        }
    }
    a.ready += 60 + random (60);
}

/* Actor a (the copy), shy and cornered beside the character: it darts past it, to the square beyond (open, no one
** on it), a moment's dash; 1 if it did */
static unsigned char __fastcall__ dart (unsigned char k)
{
    unsigned x = pcx + (pcx - a.x), y = pcy + (pcy - a.y);

    if (!open_sq (at (a.sn, x, y)) || taken (k, x, y)) {
        return 0;
    }
    a.x = x;
    a.y = y;
    a.ready += 2 * a.step;
    return 1;
}

/* ---- Its events (DENIZENS.md: events): what its actors do, published (the hub's denizens' events: nobody waits
** for them, and each perceives them or not by its own senses); and the noises they hear, their own and the world's
** (the character's: digging), each by every actor within its kind's hearing times the noise's loudness over two:
** it turns to face the noise, stands a while, and looks for the character again at once */

static struct msg ev;
static unsigned wplace, dplace;                 /* (Its places in the world's events, and in its own) */

/* What lies between actor a (the copy) and the character, for those who hear it (SOUND.md: E_DEN's x[3]): the squares
** of wall and rock on the line (15 at most), and the shut doors times 16; 0 off its stratum, or more than 24 squares
** off (beyond anyone's hearing of a step) */
static unsigned char between (void)
{
    unsigned char* p;
    struct portal* q;
    unsigned char solid = 0, doors = 0;
    unsigned i, dist;
    int dx = pcx - a.x, dy = pcy - a.y;

    if (a.sn != pcs || (dist = apart (dx, dy)) > 24) {
        return 0;
    }
    for (i = 1; i < dist; ++i) {
        p = at (a.sn, a.x + dx * (int) i / (int) dist, a.y + dy * (int) i / (int) dist);
        if (!p || !T_CLEAR (*p)) {
            if (solid < 15) {
                ++solid;
            }
        } else if (TILE (*p) == T_DOOR && (q = portal_at (cx_, cy_)) != 0 && !(q->state & PS_OPEN) && doors < 15) {
            ++doors;
        }
    }
    return solid | doors << 4;
}

/* Actor k (the copy, a) did what, how loud (0-9): published, where it is (and whether the character sees its square
** now, the senses' marks; what lies between, if it's heard), the hub's header in the window after */
static void __fastcall__ publish (unsigned char k, unsigned char what, unsigned char loud)
{
    unsigned char* p;

    ev.x[3] = loud ? between () : 0;
    p = at (a.sn, a.x, a.y);
    ev.x[2] = p && (*p & T_VIS);
    ev.kind = E_DEN;
    ev.who = k + 1;
    ev.a = what;
    ev.b = loud;
    ev.w1 = a.x;
    ev.w2 = a.y;
    ev.t = now;
    ev.x[0] = a.sn;
    ev.x[1] = akind[k];
    hub_map (0);
    hub_devent (&ev);
}

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
        } else if (hub_dnext (&dplace, &m)) {   /* (Its own: an actor's, not heard by itself) */
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
            if (!q->name[0] || k + 1 == src || q->sn != sn || (m.w1 == q->x && m.w2 == q->y)) {
                continue;
            }
            kind_get (akind[k]);
            if (apart (q->x - m.w1, q->y - m.w2) > K.hear * loud / 2) {
                continue;
            }
            q->face = bearing (m.w1 - q->x, m.w2 - q->y);
            alert[k] = now + 100;
            sgen[k] = 0;                        /* (It senses again: the character, if it's there to be sensed) */
            if (q->ready > now + 1) {
                q->ready = now + 1;
            }
        }
    }
    hub_map (0);
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
** sensed it, till it's beside it.  The step, or 0xFE (its time added) */
static unsigned char __fastcall__ ally (unsigned char k)
{
    unsigned char d = 0xFE;

    if (sres[k]) {
        a.face = bearing (ddx, ddy);
    }
    if (aorder[k] == OR_WAIT) {
        a.doing = DO_WAIT;
        a.ready += 20 + random (20);
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
        a.ready += 5 + random (10);
        d = 0xFE;
    }
    return d;
}

/* What actor k (the copy) does as it senses the character, by its attitude and its kind.  The step, 0xFD (darted
** past it), or 0xFF (none) */
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

/* What actor k means to do, by its kind, its attitude and what it senses, and done (or a while waited), by the
** character's rules (step_here: the world's own, as they were when it moved them): kept off a foe; an ally's way;
** as it senses the character, meets's; a noise heard, still a while; else it wanders about its lair (or keeps to it) */
static void __fastcall__ decide (unsigned char k)
{
    unsigned char d = 0xFF, was;

    kind_get (akind[k]);
    memcpy (&a, &HUB->actor[k], sizeof a);
    if (!turns && a.ready + CATCHUP < now) {
        a.ready = now - CATCHUP;
    }
    refx = pcx;
    refy = pcy;
    was = a.doing;
    ddx = pcx - a.x;
    ddy = pcy - a.y;
    dist = apart (ddx, ddy);
    a.doing = DO_ROAM;
    if (sgen[k] != sense_gen || sax[k] != a.x || say[k] != a.y) {
        sgen[k] = sense_gen;                    /* (Sensed again only as it or the character has changed) */
        sax[k] = a.x;
        say[k] = a.y;
        sres[k] = senses (dist, ddx, ddy);
    }
    if (sres[k]) {
        lsx[k] = pcx;
        lsy[k] = pcy;
        feel (k);
    }
    anear[k] = a.sn == pcs && dist <= 3;
    if ((d = keep_off (k)) != 0xFF) {
        a.doing = sres[k] ? DO_WATCH : DO_ROAM;
    } else if (a.attitude == AT_ALLIED) {
        d = ally (k);
    } else if (sres[k]) {
        d = meets (k, was);
    } else if (alert[k] > now) {
        d = 0xFE;                               /* (A noise heard: still, facing the way it came, a while) */
        a.ready += 10 + random (20);
    } else if (turns && !(*at (a.sn, a.x, a.y) & T_VIS)) {
        coarse (k, K.flags & K_LAIR ? 2 : K.range);
        d = 0xFE;                               /* (Unseen, unaware: a coarse move) */
    } else if (K.flags & K_LAIR ? !random (4) : random (3)) {
        d = step_for (k, K.flags & K_LAIR ? 2 : K.range, S_ANY);
    }
    if (d == 0xFF) {
        a.ready += a.doing ? 60 + random (60) : 10 + random (40);    /* (Watching: a while, unless it changes) */
    } else if (d < 4) {
        step_here (d);
    }
    if (d < 4 || d == 0xFD) {                   /* (Published: a step, now and then a noise of its own; a dash) */
        publish (k, d == 0xFD || ++steps[k] & 7 ? DE_MOVED : DE_NOISE, d == 0xFD ? 2 : steps[k] & 7 ? 1 : 3);
    }
    if (a.doing != was && (!was || !a.doing)) {
        publish (k, a.doing ? DE_NOTICED : DE_LOST, 0);
    }
    hub_map (0);
    memcpy (&HUB->actor[k], &a, sizeof a);
    if (a.doing != was || d < 4 || d == 0xFD) {
        ++HUB->den_seq;                         /* (Changed: the world looks again at what the character sees) */
    }
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
        lair_get (D.lair);
        if (D.attitude == AT_ALLIED && D.order == OR_WAIT) {
            D.order = OR_NONE;                  /* (Left waiting an hour: it goes its own way, an ally no more) */
            D.attitude = AT_FRIENDLY;
        }
        lim = D.sn == L.sn ? (K.flags & K_LAIR ? 2 : K.range) : 0;    /* (Come along with the character off its
                                                **   lair's stratum: it stays where it is) */
        x = L.hx + random (2 * lim + 1) - lim;
        y = L.hy + random (2 * lim + 1) - lim;
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

/* Ally k come along with the character, up or down: on a square beside it, open, no one's on it.  1 if it is */
static unsigned char __fastcall__ along (unsigned char k)
{
    struct actor* q;
    unsigned char h;
    unsigned x, y;

    for (h = 0; h < 8; ++h) {
        x = pcx + head_x[h];
        y = pcy + head_y[h];
        if (open_sq (at (pcs, x, y)) && !taken (k, x, y)) {
            hub_map (0);
            q = &HUB->actor[k];
            q->sn = pcs;
            q->x = x;
            q->y = y;
            q->ready = now + 10;
            sgen[k] = 0;
            ++HUB->den_seq;
            return 1;
        }
    }
    hub_map (0);
    return 0;
}

/* Each actor tended: what the world's done taken, those far from the character retired, those free to act set
** going (at the turns pace, each caught up with the clock: moved here, the world waiting); the denizens near it
** placed (for a while after it moves: the chunks round it made as it looks about) and enlisted; those away from it
** moved, coarsely.  Nothing, if nothing's changed (the clock, the character, the world's work) */
static void tend (void)
{
    static unsigned lx, ly;
    static unsigned long lnow;
    static unsigned char ls = 0xFF, fresh, lgen;
    struct actor* q;
    unsigned char k, n, again = 0, woke = lgen != sense_gen;

    lgen = sense_gen;
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
    hub_map (0);
    for (k = 0, q = HUB->actor; k < NACTORS && !again && now == lnow; ++k, ++q) {
        if (q->state == REQ_DONE) {
            again = 1;
        }
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
        if (q->state == REQ_DONE) {             /* (Done; or it couldn't, and it waits a moment) */
            if (q->result) {
                q->ready += 10;
            }
            q->state = REQ_FREE;
        }
        if (!q->name[0] || q->state != REQ_FREE) {
            continue;
        }
        if (woke && q->doing && q->ready > now + 10) {
            q->ready = now + 1 + random (10);   /* (The character's changed: a watcher sees to it soon) */
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
        for (n = 0; q->ready <= now && n < (turns ? 64 : 1); ++n) {
            decide (k);
            hub_map (0);
        }
    }
    if (again) {
        enlist_near ();
    }
}

/* ---- The character's ways with a denizen (the UI's request, the hub's sway: hub.h): the one meant found among those
** it sees (or that are near), by a word of its name or its kind's; and words to sway it (the referee's test, in the
** UI's slot, lent while the UI waits), a request to an ally, a name given, a look at how it seems, a blow (harmed) */

static struct sway sw;

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
static void __fastcall__ harmed (unsigned char k)
{
    struct actor* q = HUB->actor;
    struct den* r = POP->den;
    unsigned char j, f, l, n;

    hub_map (0);
    f = q[k].faction;
    den_get (q[k].den);
    l = D.lair;
    for (j = 0; j < NACTORS; ++j, ++q) {
        if (!q->name[0]) {
            continue;
        }
        if (j == k) {
            q->attitude = AT_HOSTILE;
        } else if (q->faction == f) {
            den_get (q->den);
            n = D.lair == l ? 2 : 1;
            q->attitude = q->attitude > n ? q->attitude - n : AT_HOSTILE;
        } else if (REL (f, q->faction) == AT_HOSTILE && sres[j] && q->attitude < AT_FRIENDLY) {
            ++q->attitude;
        } else {
            continue;
        }
        if (q->attitude < AT_ALLIED) {
            aorder[j] = OR_NONE;
        }
        q->ready = now + 1;
        sgen[j] = 0;
    }
    for (j = 0; j < hd.ndens; ++j, ++r) {
        hy_bank (pb);
        if (!(r->flags & D_ACTOR) && kfact[r->kind % SND_KINDS] == f) {
            n = r->lair == l ? 2 : 1;
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
    ++HUB->den_seq;
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
    unsigned char t = sw.was, h = sw.how, p;

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
    r = &HUB->slot[SLOT_UI];
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
    r = hub_ask (SLOT_UI);
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
    if ((n = name_of (q->den)) == NNAMES && (n = name_of (0xFF)) == NNAMES) {
        sw.result = SR_CANT;                    /* (Names enough given) */
        return;
    }
    sw.word2[sizeof hd.names[0].name - 1] = 0;
    if (sw.word2[0] >= 'a' && sw.word2[0] <= 'z') {
        sw.word2[0] += 'A' - 'a';
    }
    hd.names[n].den = q->den;
    strcpy (hd.names[n].name, sw.word2);
    hub_map (0);
    strcpy (q->name, sw.word2);
}

/* The UI's request answered */
static void sway (void)
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
    if (sw.how == SW_ORDER && !sw.word[0]) {    /* (To every ally in earshot) */
        for (k = 0; k < NACTORS; ++k) {
            hub_map (0);
            q = &HUB->actor[k];
            if (q->name[0] && q->attitude == AT_ALLIED && q->sn == pcs && apart (q->x - pcx, q->y - pcy) <= 12) {
                strcpy (sw.name, ++n == 1 ? q->name : "");
                order (k);
                q->ready = now + 1;
            }
        }
        sw.result = n ? sw.result : SR_NONE;
    } else if (sw.how == SW_ATTACK && !sw.word[0]) {
        sw.result = SR_CANT;                    /* (A blow wants the one meant named) */
    } else if ((k = meant (sw.word)) == 0xFF) {
        sw.result = SR_NONE;
    } else {
        q = &HUB->actor[k];
        strcpy (sw.name, q->name);
        sw.was = sw.now = q->attitude;
        kind_get (akind[k]);
        sw.beast = K.mind == M_BEAST;
        switch (sw.how) {
        case SW_LOOK:
            break;
        case SW_ATTACK:                         /* (The stand-in for the fights': harm done, its attitude's fall) */
            harmed (k);
            sw.now = AT_HOSTILE;
            sw.t = 20;
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
        hub_map (0);
        HUB->actor[k].ready = now + 1;          /* (It sees to what's changed at once) */
        sgen[k] = 0;
    }
    hub_map (0);
    ++HUB->den_seq;
    memcpy (&HUB->sway, &sw, sizeof sw);       /* (The reply whole, then the state that shows it) */
    HUB->sway.state = REQ_DONE;
}

/* ---- The records kept (back: read again), the save folder's dens.dat: the bank a piece at a time, through the
** main RAM (the actors' places written back first; read back, none an actor yet) */
static void __fastcall__ keep (unsigned char back)
{
    static char p[56];
    unsigned off;
    unsigned char k;
    int fd;

    hub_map (0);
    if (!HUB->savedir[0]) {
        return;
    }
    strcpy (p, HUB->savedir);
    strcat (p, "/dens.dat");
    if ((fd = back ? open (p, O_RDONLY) : open (p, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
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

int main (int argc, char* argv[])
{
    static unsigned lpx, lpy;
    static unsigned char lpl;
    unsigned char* p;
    int b;

    if (argc < 4 || (b = hy_banks_alloc (2)) < 0 || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    kb = b;
    pb = b + 1;
    if (kinds_load (argv[3])) {
        return 1;
    }
    hub_map (THEME_BANK);                       /* (The theme's denizens: which of the kinds) */
    tdens = THEME->dens;
    chunks_init (ASK_DENIZENS);
    hy_bank (pb);
    memset (POP, 0, 0x2000);
    hub_map (0);
    memcpy (hd.fatt, fatt0, sizeof hd.fatt);    /* (A new game's: the factions as the character first meets them) */
    memset (hd.names, 0xFF, sizeof hd.names);
    if (HUB->resume) {
        keep (1);
    }
    wplace = HUB->eq_head;                      /* (Its events from now on) */
    dplace = HUB->dq_head;
    while (hub_map (0), !HUB->quit) {
        while (hy_sem_try (HUB->sem[T_DENIZENS]) == 0) {
        }
        if (HUB->den_state == REQ_POSTED) {     /* (The UI's asked for a save) */
            keep (0);
            hub_map (0);
            HUB->den_state = REQ_DONE;
        }
        now = hub_now ();
        pcx = HUB->pc_x;
        pcy = HUB->pc_y;
        pcs = HUB->pc_stratum;
        pcbusy = HUB->pc_ready > now;
        if (!(pclit = HUB->pc_light != 0) && hd.done[pcs] && (p = at (pcs, pcx, pcy)) != 0) {
            pclit = (*p & T_VIS) != 0;          /* (Lit where it stands: a room's light, a lamp put down) */
        }
        hub_map (0);
        if (pcx != lpx || pcy != lpy || (pclit | pcbusy << 1) != lpl) {
            lpx = pcx;                          /* (The character changed: each actor senses it again) */
            lpy = pcy;
            lpl = pclit | pcbusy << 1;
            if (!++sense_gen) {
                sense_gen = 1;
            }
        }
        if (HUB->sway.state == REQ_POSTED) {    /* (The UI's: the character's way with a denizen) */
            sway ();
        }
        turns = !HUB->tpr;
        grow ();
        if (hd.done[pcs]) {
            hear ();
            tend ();
        }
        hub_map (0);
        HUB->den_at = now;                      /* (Caught up: the turns pace's clock goes on) */
        ++HUB->beat[T_DENIZENS];
        if (turns && HUB->pc_ready > now) {     /* (The turns pace's clock moving: kept up with, a tick at a time,
                                                **   never spinning: the others' work goes on meanwhile) */
            if (hub_now () != now) {
                hy_yield ();
            } else {
                hy_sleep_ticks (1);
            }
        } else {
            hy_sleep_ticks (10);
        }
    }
    return 0;
}
