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
** (roam).  No two are on a square, nor any on the character's or where it's going.  (Attitudes, factions and
** influence, and the denizens' own exploring come later.)
**
** Its records (the lairs, the denizens) are in a bank of its own, a record at a time copied out; kept with the game
** (dq0 -d: the UI asks, the hub's den_state), the save folder's dens.dat.
*/

#include <fcntl.h>
#include <stddef.h>
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
};
#define K_SHY           0x01                    /* It keeps its distance from the character */
#define K_LAIR          0x02                    /* It keeps to its lair */
#define K_WAYS          0x04                    /* Its lairs are by the stratum's ways in and out */
#define K_BOLD          0x08                    /* It comes towards the character */
#define KINDS           ((struct dkind*) 0x8000)

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
    signed char     att;                        /* Its attitude to the character (hub.h's AT_*) */
};
#define D_OUT           0x01                    /* On a square of its own */
#define D_ACTOR         0x02                    /* An actor now (the hub's entry the truth while it is) */
#define D_DEAD          0x04                    /* Dead (its body left where it fell: an item) */
struct head {
    unsigned char   done[MAXSTRATA];            /* Each stratum's populations decided */
    unsigned char   nlairs, ndens;
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
    hub_map (0);
    nkinds = off / sizeof (struct dkind);
    return 0;
}

/* ---- The character, as the hub has it (each time round) */

static unsigned long now;
static unsigned pcx, pcy;
static unsigned char pcs, pclit, pcbusy, pcdead;

static unsigned __fastcall__ apart (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dx > dy ? dx : dy;
}

/* A kind's attitude to the character as it first meets it (K the kind's: till the theme's factions say, the bold
** hostile, those that keep to their lairs unfriendly, the rest wary) */
static signed char first_attitude (void)
{
    return K.flags & K_BOLD ? AT_HOSTILE : K.flags & K_LAIR ? AT_UNFRIENDLY : AT_WARY;
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
            D.att = first_attitude ();
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
static unsigned long fled[NACTORS];             /* (Each one fleeing the character till then) */
static unsigned long bled[NACTORS];             /* (Each one bleeding: its bloodloss reckoned to then; 0 not) */

/* ---- The referee, asked in this task's own slot (in the denizens' bank, DSLOT_AT): the request (rq, in the main
** RAM: its kind, who and whom here, the rest the caller's) copied in whole, posted, the referee rung and waited for
** (the CPU left to it a tick at a time), the reply copied out; the hub's header in the window after */
static struct req rq;
#define RQ_SIZE (offsetof (struct req, text) + DTEXT)  /* (Its text DTEXT long: no more) */

static void __fastcall__ ask (unsigned char kind, unsigned char actor, unsigned char other)
{
    rq.kind = kind;
    rq.actor = actor;
    rq.other = other;
    rq.state = REQ_FREE;
    hub_map (DEN_BANK);
    memcpy (DSLOT_AT, &rq, RQ_SIZE);
    DSLOT_AT->state = REQ_POSTED;               /* (Posted once it's whole) */
    hub_map (0);
    hub_ring (T_REFEREE);
    for (;;) {
        hub_map (DEN_BANK);
        if (DSLOT_AT->state == REQ_DONE) {
            break;
        }
        hub_map (0);
        if (HUB->quit) {
            rq.result = 0xFF;                   /* (The game's ending: the referee may be gone) */
            memset (rq.x, 0, sizeof rq.x);
            return;
        }
        hy_sleep_ticks (1);
    }
    memcpy (&rq, DSLOT_AT, RQ_SIZE);
    DSLOT_AT->state = REQ_FREE;
    hub_map (0);
}

/* Actor k's place and time written back to its record */
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
    D.att = a.attitude;
}

/* Actor k no longer one (far from the character): its record its own again, the entry free */
static void __fastcall__ retire (unsigned char k)
{
    write_back (k);
    D.flags &= ~D_ACTOR;
    den_put (a.den);
    HUB->actor[k].name[0] = 0;
    ++HUB->den_seq;
}

/* Denizen i made an actor, in free entry k (its time from now: the coarse moves have kept it while it wasn't one);
** its sheet the referee's (RS_NEW_ACTOR, by its kind's key: made the first time, the same one after) */
static void __fastcall__ enlist (unsigned char i, unsigned char k)
{
    struct rs_summary* s = (struct rs_summary*) rq.x;

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
    a.attitude = D.att;
    sgen[k] = 0;
    alert[k] = 0;
    fled[k] = 0;
    strcpy (rq.text, K.key);
    rq.w1 = i;
    ask (RS_NEW_ACTOR, k + 1, 0);
    a.health = s->health;
    bled[k] = s->cond & C_BLEEDING ? now : 0;
    if (s->cond & C_UNCONSCIOUS) {
        a.doing = DO_DOWN;                      /* (Senseless still: a while yet) */
        a.ready = now + 300;
    }
    strcpy (a.name, K.name);
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
** S_ANY (the way it faces, or another), S_AWAY (only one that takes it further from the character: the furthest;
** close by it, cornered, one no nearer: a way round it, out), S_TOWARD (only one that takes it nearer: the nearest).
** Its direction, or 0xFF */
enum { S_ANY, S_AWAY, S_TOWARD };
static unsigned char __fastcall__ step_for (unsigned char k, unsigned lim, unsigned char how)
{
    unsigned char d, j, best = 0xFF, s = random (2) ? a.face >> 1 : random (4);
    unsigned nx, ny, f, bf = apart (a.x - pcx, a.y - pcy);

    for (j = 0; j < 4; ++j) {
        d = (s + j) & 3;
        nx = a.x + dir_x[d];
        ny = a.y + dir_y[d];
        f = apart (nx - pcx, ny - pcy);         /* (The walk's own test last: it's the dearest) */
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
        } else if (hub_dnext (&dplace, &m)) {   /* (Its own: an actor's, not heard by itself; a fight's, not by
                                                **   those in it) */
            if ((m.kind != E_DEN && m.kind != E_FIGHT) || !m.b) {
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
                q->doing >= DO_FIGHT || (m.kind == E_FIGHT && k + 1 == m.a)) {
                continue;                       /* (Not those fighting, fleeing, or senseless: nothing turns them) */
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

/* ---- Fights (the referee's, RS_ATTACK: HarnMaster's melee).  A hostile actor beside the character, sensing it,
** strikes at it a round at a time (the referee's round: the hub's), as the character strikes at one (its blow
** posted by the world, I_ATTACK: the hub's pc_foe, struck here as it's seen); each blow told (E_FIGHT, in this task's
** queue: the referee's words in the ring, FIGHT_AT, for the UI), heard (loud: the others turn to it), and what it
** did taken: a denizen dead (its record so, its body left where it fell: an item), knocked senseless (it lies there
** till it comes round: the referee's time), wounded (its morale tested, RS_MORALE: broken, or a shy one cautious, it
** flees a minute, and fights after only if it's pressed; cautious, it holds back a round), its tactical advantage (it
** strikes again at once).  One struck at turns on the character, unless it flees.  One bleeding has its bloodloss
** reckoned a minute at a time (RS_TIME), and may die of it */

#define FLEE_T          600                     /* Tenths a broken one flees for */
#define LOUD_FIGHT      5

static unsigned char fring;                     /* (The ring's next) */
static struct fight fw;                         /* (A blow's words, as they go into the ring) */

/* The referee's words of a blow (its reply in rq) kept for the ring (fw) */
static void keep_words (void)
{
    memset (&fw, 0, sizeof fw);
    fw.out = rq.w2;
    fw.which = rq.a;
    strncpy (fw.text, rq.text, sizeof fw.text - 1);
}

/* A blow told: attacker att, defender def (0 the character, else an actor's entry plus 1; actor a, the copy, the one
** in it), how it came out (O_*, FO_GONE, FO_BLED), what it left them in (FF_*); its words fw's */
static void __fastcall__ told (unsigned char att, unsigned char def, unsigned char out, unsigned char f)
{
    strcpy (att ? fw.att : fw.def, a.name);
    hub_map (DEN_BANK);
    memcpy (FIGHT_AT + fring, &fw, sizeof fw);
    hub_map (0);
    ev.kind = E_FIGHT;
    ev.who = att;
    ev.a = def;
    ev.b = LOUD_FIGHT;
    ev.w1 = a.x;
    ev.w2 = a.y;
    ev.t = now;
    ev.x[0] = a.sn;
    ev.x[1] = out;
    ev.x[2] = f;
    ev.x[3] = fring;
    hub_devent (&ev);
    fring = (fring + 1) % NFIGHTS;
}

/* A body put down where actor a (the copy) fell: an item of the store's (under its lock, as the maker's are) */
static void __fastcall__ body (unsigned char kind)
{
    struct item* p;
    unsigned i;

    hub_map (0);
    hy_sem_acquire (HUB->item_lock);
    for (i = 1; i < NITEMS; ++i) {
        hub_map (ITEM_BANK (i));
        p = ITEM_AT (i);
        if (!p->kind) {
            memset (p, 0, sizeof *p);
            p->kind = kind;
            p->where = IW_SQUARE;
            p->sn = a.sn;
            p->x = a.x;
            p->y = a.y;
            p->qty = 1;
            break;
        }
    }
    hub_map (0);
    hy_sem_release (HUB->item_lock);
}

/* Actor k dead (the hub's entry as it is): its record so, out of the dungeon for good, its body where it fell (a
** skeleton's its bones: the stand-in theme's), the entry free */
static void __fastcall__ kill (unsigned char k)
{
    write_back (k);
    D.flags = (D.flags & ~(D_ACTOR | D_OUT)) | D_DEAD;
    den_put (a.den);
    kind_get (akind[k]);
    body (strcmp (K.key, "skel") ? KI_BODY : KI_BONES);
    HUB->actor[k].name[0] = 0;
    ++HUB->den_seq;
    HUB->den_look = 1;                          /* (The world's to look about again: the body seen) */
}

/* What a blow did (the referee's reply in rq), taken for actor k (a, the copy; K its kind): att and def as told's,
** and told; 1 if the actor's dead */
static unsigned char __fastcall__ fought (unsigned char att, unsigned char def, unsigned char k)
{
    const struct rs_effect* e = (const struct rs_effect*) rq.x;
    unsigned char i, f = 0, hurt = 0, me = k + 1, out = rq.result, dead;

    keep_words ();
    for (i = 0; i < RS_EFFECTS && e->kind; ++i, ++e) {
        if (e->kind == X_DEATH) {
            f |= e->target == att ? FF_A_DEAD : FF_D_DEAD;
        } else if (e->kind == X_COND_ON) {
            f |= e->cond & C_UNCONSCIOUS ? (e->target == att ? FF_A_DOWN : FF_D_DOWN) : FF_PRONE;
        } else if (e->kind == X_DROP) {
            f |= FF_FUMBLE;
        }
        if (e->target != me) {
            continue;
        }
        if (e->kind == X_WOUND) {
            hurt = 1;
            if ((e->cond & C_BLEEDING) && !bled[k]) {
                bled[k] = now ? now : 1;
            }
        } else if (e->kind == X_COND_ON && (e->cond & C_UNCONSCIOUS)) {
            a.doing = DO_DOWN;                  /* (Senseless: it lies there till it comes round) */
            a.ready = now + rq.w2;
        }
    }
    if (rq.c & (AF_STOPPED | AF_STOPPED_D)) {
        f |= FF_STOPPED;
    }
    a.health = att ? rq.w1 : rq.w1 >> 8;
    if ((rq.c & AF_TA) && def == me) {
        a.ready = now;                          /* (Its tactical advantage: it strikes again at once) */
    }
    if (!att && a.attitude > AT_HOSTILE) {
        a.attitude = AT_HOSTILE;                /* (Struck at: it turns on the character) */
    }
    dead = (f & (me == att ? FF_A_DEAD : FF_D_DEAD)) != 0;
    if (hurt && !dead && a.doing != DO_DOWN) {
        rq.steps = 0;
        ask (RS_MORALE, me, 0);
        if (rq.result == M_BROKEN || (rq.result == M_CAUTIOUS && (K.flags & K_SHY))) {
            fled[k] = now + FLEE_T;             /* (Broken: it flees, and won't seek the fight again) */
            a.doing = DO_FLEE;
            a.attitude = AT_UNFRIENDLY;
            f |= FF_FLEES;
        } else if (rq.result == M_CAUTIOUS) {
            a.ready += HUB->round;              /* (Cautious: it holds back a round) */
        }
    }
    told (att, def, out, f);
    return dead;
}

/* The character's blow at actor k (the world's, posted: the hub's pc_foe), struck if the actor's still beside it;
** else told it's out of reach */
static void __fastcall__ pc_strike (unsigned char k)
{
    unsigned char dead;

    hub_map (0);
    memcpy (&a, &HUB->actor[k], sizeof a);
    if (!a.name[0] || a.sn != pcs || apart (a.x - pcx, a.y - pcy) > 1) {
        memset (&fw, 0, sizeof fw);
        told (0, k + 1, FO_GONE, 0);
        return;
    }
    kind_get (akind[k]);
    rq.a = HUB->pc_aim;
    rq.b = AS_BEST;
    rq.c = 0;
    rq.steps = 0;
    ask (RS_ATTACK, 0, k + 1);
    if (rq.result == 0xFF) {
        return;                                 /* (It can't: senseless, dead) */
    }
    dead = fought (0, k + 1, k);
    hub_map (0);
    memcpy (&HUB->actor[k], &a, sizeof a);
    ++HUB->den_seq;
    if (dead) {
        kill (k);
    }
}

/* Actor a (the copy, entry k) strikes at the character beside it (the others beside it fighting it too: it's
** outnumbered), a round's turn: 1 if it died of it (a counterstrike) */
static unsigned char __fastcall__ den_strike (unsigned char k)
{
    struct actor* q = HUB->actor;
    unsigned char j, n = 0;

    hub_map (0);
    for (j = 0; j < NACTORS; ++j, ++q) {
        if (j != k && q->name[0] && q->doing == DO_FIGHT && q->sn == pcs && apart (q->x - pcx, q->y - pcy) <= 1) {
            ++n;
        }
    }
    rq.a = Z_MID;
    rq.b = AS_BEST;
    rq.c = n;
    rq.steps = 0;
    ask (RS_ATTACK, k + 1, 0);
    a.doing = DO_FIGHT;
    a.ready += HUB->round;
    return rq.result != 0xFF && fought (k + 1, 0, k);
}

/* Actor k (the hub's entry) bleeding: its bloodloss reckoned a minute at a time (RS_TIME); dead of it, told if the
** character sees it */
static void __fastcall__ bleeding (unsigned char k)
{
    unsigned char* p;
    unsigned m;

    if (!bled[k] || (m = (unsigned) ((now - bled[k]) / 600)) == 0) {
        return;
    }
    bled[k] += m * 600UL;
    hub_map (0);
    memcpy (&a, &HUB->actor[k], sizeof a);
    rq.w1 = m;
    ask (RS_TIME, k + 1, 0);
    HUB->actor[k].health = rq.b;
    if (rq.result == 1) {
        if ((p = at (a.sn, a.x, a.y)) != 0 && (*p & T_VIS)) {
            memset (&fw, 0, sizeof fw);
            told (k + 1, 0, FO_BLED, FF_A_DEAD);
        }
        kill (k);
    }
    hub_map (0);
}

/* Actor a (the copy) can strike the character dx, dy off, beside it: square on, or a corner between them open */
static unsigned char __fastcall__ reach (int dx, int dy)
{
    unsigned char* p;

    if (!dx || !dy) {
        return 1;
    }
    return ((p = at (a.sn, a.x + dx, a.y)) != 0 && T_CLEAR (*p)) || ((p = at (a.sn, a.x, a.y + dy)) != 0 &&
        T_CLEAR (*p));
}

/* What actor k means to do, by its kind, its attitude and what it senses, and done (or a while waited), by the
** character's rules (step_here: the world's own, as they were when it moved them).  Fleeing, it goes away from the
** character (cornered beside it, it fights).  Watching the character: a foe beside it (hostile; unfriendly, and
** pressed) strikes at it; a shy one keeps its distance (till it's a few squares further; cornered beside it, it darts
** past), a bold or hostile one comes towards it; else it wanders about its lair (or keeps to it) */
static void __fastcall__ decide (unsigned char k)
{
    unsigned char d = 0xFF, was, foe, dead = 0;
    unsigned dist;
    int dx, dy;

    kind_get (akind[k]);
    memcpy (&a, &HUB->actor[k], sizeof a);
    if (!turns && a.ready + CATCHUP < now) {
        a.ready = now - CATCHUP;
    }
    was = a.doing;
    dx = pcx - a.x;
    dy = pcy - a.y;
    dist = apart (dx, dy);
    a.doing = DO_ROAM;
    if (sgen[k] != sense_gen || sax[k] != a.x || say[k] != a.y) {
        sgen[k] = sense_gen;                    /* (Sensed again only as it or the character has changed) */
        sax[k] = a.x;
        say[k] = a.y;
        sres[k] = senses (dist, dx, dy);
    }
    foe = !pcdead && (a.attitude <= AT_HOSTILE || (a.attitude == AT_UNFRIENDLY && dist <= 1));
    if (fled[k] > now) {
        a.doing = DO_FLEE;                      /* (Fleeing: away; cornered beside the character, it fights) */
        if ((d = step_for (k, 4 * K.range + 8, S_AWAY)) == 0xFF && dist == 1 && sres[k] && !pcdead &&
            reach (dx, dy)) {
            dead = den_strike (k);
            d = 0xFC;
        }
    } else if (sres[k]) {
        a.face = bearing (dx, dy);
        a.doing = DO_WATCH;
        if (foe && dist == 1 && reach (dx, dy)) {
            dead = den_strike (k);
            d = 0xFC;
        } else if ((K.flags & K_SHY) && !foe && dist < (was == DO_SHY ? 7 : 5)) {
            a.doing = DO_SHY;
            if ((d = step_for (k, 2 * K.range, S_AWAY)) == 0xFF && dist == 1 && dart (k)) {
                d = 0xFD;                       /* (Cornered: past the character) */
            }
        } else if (((K.flags & K_BOLD) || (foe && a.attitude <= AT_HOSTILE)) && dist > 1 &&
            (d = step_for (k, 2 * K.range, S_TOWARD)) != 0xFF) {
            a.doing = DO_COMING;
        }
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
        a.ready += a.doing == DO_FLEE ? 10 + random (20) : a.doing ? 60 + random (60) : 10 + random (40);
    } else if (d < 4) {                         /* (Watching: a while, unless it changes; fleeing, a moment) */
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
    if (a.doing != was || d < 4 || d >= 0xFC) {
        ++HUB->den_seq;                         /* (Changed: the world looks again at what the character sees) */
    }
    if (dead) {
        kill (k);
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
        lim = K.flags & K_LAIR ? 2 : K.range;
        x = L.hx + random (2 * lim + 1) - lim;
        y = L.hy + random (2 * lim + 1) - lim;
        if (made_at (D.sn, x, y) && open_sq (at (D.sn, x, y)) && !taken (0xFF, x, y) && !rec_on (D.sn, x, y)) {
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
    hub_map (0);
    if ((k = HUB->pc_foe) != 0) {               /* (The character's blow, posted: struck first) */
        HUB->pc_foe = 0;
        pc_strike ((k - 1) & 7);
        again = 1;
    }
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
        if (woke && q->doing && q->doing < DO_FIGHT && q->ready > now + 10) {
            q->ready = now + 1 + random (10);   /* (The character's changed: a watcher sees to it soon; not one
                                                **   fighting, fleeing or senseless: its own time) */
        }
        if (q->sn != pcs || apart (q->x - pcx, q->y - pcy) > ACT_R + 8) {
            retire (k);
            again = 1;
            continue;
        }
        bleeding (k);
        if (!q->name[0]) {
            continue;                           /* (Dead of it) */
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
        pcdead = (HUB->pcsum.cond & C_DEAD) != 0;
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
