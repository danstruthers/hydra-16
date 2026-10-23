/*
** map.c - (dq0m) the map's window round the character (the UI asks: the hub's mapq), drawn into the UI bank (its rows,
** and each square's layer beside them, uirec's maplay: the UI's to colour) as the character knows the place
** (DUNGEON.md, the map kept, bearings underground): what it sees now as it is (a floor's height against its own
** marked, 1-9 tens of feet higher, a-i lower); the rest only as it's kept, faintly (a floor a comma, no heights): drawn
** on the map it carries (T_DRAWN, its chunk's kept.map that map's: as it was drawn), else as its mind keeps each chunk
** (struct chunk's kept: a square shown or not as the chunk's memory goes, its quality and forty more of a hundred, a
** square at a time, gone once the memory's gone; and for one that doesn't know north, the chunk's squares put off where
** they truly are, the more the more it's faded since, six squares at the most, the chunk's own way: directions and
** distances off); the character an @.  North up for one that knows north (the summary's northsense: a dwarf), the
** window kept in the stratum; for the rest the way it faces up (its body's, to the quarter turn: facing north or
** northeast, north up; east or southeast, east up; what it faces up, or up and to its right), the character in the
** middle, the rows turned (the UI puts a compass in the top right corner, the north it believes in: look.c's).  Only
** the chunks in the cache are looked at (a chunk
** that isn't there now was seen, if at all, before the cache made way for it: its memory's laid over it as it's made
** again, so the character's near ones are always there).
**
** All of it a swapped part's (part 1: ../dq0/swap.inc), draw_map called through its stub (bank.s's).
*/

#include <string.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"

#pragma code-name ("SW1CODE")
#pragma rodata-name ("SW1RODATA")
#pragma local-strings (on)
#pragma static-locals (on)

static struct chunk* K;
static unsigned char kx, ky, lk = 0xFF, lsn;
static unsigned lcx, lcy;
static unsigned char sn_, north, fade, lay;
static unsigned map_, now_;
static int myz;
static unsigned char mslot, mthr, moff, mway;   /* (The memory of the chunk in slot mslot as it is now: memory's) */

/* A square of a chunk in the cache, or NULL (outside the stratum, or not in the cache) */
static unsigned char* __fastcall__ peek (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s = &STRATUM (sn);
    unsigned ox = x - s->x0, oy = y - s->y0, cx, cy;
    unsigned char k;
    struct cslot* c;

    if (ox >= s->w || oy >= s->h) {
        return 0;
    }
    cx = ox / CW;
    cy = oy / CW;
    kx = ox % CW;
    ky = oy % CW;
    if (lk == 0xFF || lsn != sn || lcx != cx || lcy != cy) {
        hub_map (WORLD_BANK);                   /* (The cache's table the world bank's) */
        for (k = 0; k < NCSLOTS; ++k) {
            c = &WREC->cslot[k];
            if (c->used && c->sn == sn && c->cx == cx && c->cy == cy) {
                break;
            }
        }
        if (k == NCSLOTS) {
            return 0;
        }
        lk = k;
        lsn = sn;
        lcx = cx;
        lcy = cy;
    }
    hub_map (CACHE_BANK (lk));
    K = CHUNK_AT (lk);
    return &K->tile[ky][kx];
}

/* The memory of the chunk peek () found last (K, in slot lk) as it is now: mthr, a square shown if its hash is no more
** (0: none shown, the memory gone); moff, its squares put off that far (one that doesn't know north), mway, which way
** (its own, by its place and when it was seen) */
static void memory (void)
{
    struct ckept* k = &K->kept;
    unsigned age = now_ - k->m, u = KEPT_FADE (fade, k->n);
    unsigned char left = k->q;

    mslot = lk;
    while (age >= u && left) {                  /* (A tenth lost each fade) */
        age -= u;
        left = left > 10 ? left - 10 : 0;
    }
    mthr = !left ? 0 : left >= 62 ? 255 : (left + 40) * 5 / 2;   /* (Its quality and forty, of a hundred) */
    moff = north || !left ? 0 : (unsigned char) (6 * (k->q - left) / k->q);
    mway = (unsigned char) (K->cx * 3 + K->cy * 5 + k->m) & 7;
}

/* Square x, y's glyph (p: peek ()'s, its chunk K; faint: remembered, a floor a comma, no heights) */
static char __fastcall__ glyph (unsigned x, unsigned y, unsigned char* p, unsigned char faint)
{
    unsigned char t, n;
    int dz;

    t = TILE (*p);
    switch (t) {
    case T_FLOOR:
        if (faint) {
            return ',';
        }
        dz = K->elev[ky][kx] - myz;
        return dz >= 5 ? '1' + (dz - 5 >= 80 ? 8 : (dz - 5) / 10) : dz <= -5 ? 'a' + (-dz - 5 >= 80 ? 8 : (-dz - 5) / 10) :
            '.';
    case T_DOOR:
        for (n = 0; n < K->nportals; ++n) {     /* (A secret door: drawn as the wall it looks like) */
            if (K->portal[n].x == kx && K->portal[n].y == ky && (K->portal[n].state & PS_HIDDEN)) {
                return (p = peek (sn_, x, y - 1)) && T_PASS (*p) ? '|' : '-';
            }
        }
        return '+';
    case T_CORR:
        return '#';
    case T_STEPS:
        return '=';
    case T_UP:
        return '<';
    case T_DOWN:
        return '>';
    case T_WINDOW:
        return '"';
    case T_PIT:
        return 'O';
    case T_WALL:
        if (((p = peek (sn_, x, y - 1)) && T_INSIDE (*p)) || ((p = peek (sn_, x, y + 1)) && T_INSIDE (*p))) {
            return '-';
        }
        if (((p = peek (sn_, x - 1, y)) && T_INSIDE (*p)) || ((p = peek (sn_, x + 1, y)) && T_INSIDE (*p))) {
            return '|';
        }
        return '-';
    }
    return ' ';
}

/* What the map shows at x, y, its layer in lay (ML_*): seen now; drawn, its map carried; or as its mind keeps it */
static char __fastcall__ known (unsigned x, unsigned y)
{
    unsigned char* p = peek (sn_, x, y);
    unsigned char h;

    lay = ML_KEPT;
    if (!p) {
        return ' ';
    }
    if (*p & T_VIS) {
        lay = ML_NOW;
        return glyph (x, y, p, 0);
    }
    if ((*p & T_DRAWN) && map_ && K->kept.map == map_) {
        lay = ML_DRAWN;
        return glyph (x, y, p, 1);
    }
    if (mslot != lk) {
        memory ();
    }
    if (!(h = mthr)) {
        return ' ';
    }
    if (moff) {                                 /* (Put off: the square remembered here one that's truly further
                                                **   back its way) */
        x -= head_x[mway] * moff;
        y -= head_y[mway] * moff;
        if (!(p = peek (sn_, x, y)) || (*p & T_VIS)) {
            return ' ';
        }
    }
    return (*p & T_SEEN) && (unsigned char) (x * 7 + y * 13 + (x >> 2)) <= h ? glyph (x, y, p, 1) : ' ';
}

/* The window's squares, each way, as the map's turned (up: the quarter that's up): a step along a row, and down */
static const signed char ax[4] = { 1, 0, -1, 0 }, ay[4] = { 0, 1, 0, -1 };

void s_draw_map (void)
{
    static char row[MAP_W + 1];
    static unsigned char lays[MAP_W];
    struct stratum* s;
    unsigned left, top, px, py, x, y;
    unsigned char i, j, n, c, up, ci, cj;

    hub_map (0);
    sn_ = HUB->pc_stratum;
    px = HUB->pc_x;
    py = HUB->pc_y;
    north = HUB->pcsum.northsense;
    up = north ? 0 : HUB->pc_face >> 1;         /* (The quarter it faces up, for one that doesn't know north) */
    fade = HUB->pcsum.mapfade;
    map_ = HUB->pc_map;
    now_ = KEPT_CLOCK;
    mslot = 0xFF;
    s = &STRATUM (sn_);
    myz = peek (sn_, px, py) ? K->elev[ky][kx] : 0;
    ci = MAP_W / 2;                             /* (Turned: the character in the middle) */
    cj = MAP_H / 2;
    if (north) {                                /* (North up: the window kept in the stratum) */
        left = s->w <= MAP_W ? s->x0 : px - MAP_W / 2 < s->x0 ? s->x0 : px - MAP_W / 2 > s->x0 + s->w - MAP_W ?
            s->x0 + s->w - MAP_W : px - MAP_W / 2;
        top = s->h <= MAP_H ? s->y0 : py - MAP_H / 2 < s->y0 ? s->y0 : py - MAP_H / 2 > s->y0 + s->h - MAP_H ?
            s->y0 + s->h - MAP_H : py - MAP_H / 2;
        ci = px - left;
        cj = py - top;
    }
    for (j = 0; j < MAP_H; ++j) {
        x = px - ci * ax[up] + (j - cj) * ax[(up + 1) & 3];   /* (The row's first square: the window's top left,
                                                **   turned) */
        y = py - ci * ay[up] + (j - cj) * ay[(up + 1) & 3];
        for (i = 0, n = 0; i < MAP_W; ++i, x += ax[up], y += ay[up]) {
            if (x == px && y == py) {
                c = '@';
                lay = ML_PC;
            } else {
                c = known (x, y);
                if (up & 1) {                   /* (Turned a quarter: the walls' strokes too) */
                    c = c == '-' ? '|' : c == '|' ? '-' : c;
                }
            }
            row[i] = c;
            lays[i] = lay;
            if (c != ' ') {
                n = i + 1;
            }
        }
        row[n] = 0;
        hub_map (UI_BANK);                      /* (The map the UI bank's) */
        strcpy (UIREC->map[j], row);
        memcpy (UIREC->maplay[j], lays, MAP_W);
    }
    hub_map (0);
    lk = 0xFF;                                  /* (The cache may change before the next) */
}
