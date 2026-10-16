/*
** map.c - (dq0m) the map's window round the character (the UI asks: the hub's mapq), drawn into the UI bank: each
** square the character has seen, a floor's height against its own marked (1-9 tens of feet higher, a-i lower), the
** character an @.  Only the chunks in the cache are looked at (a chunk that isn't there now was seen, if at all,
** before the cache made way for it: its map's laid over it as it's made again, so the character's near ones are
** always there).
**
** All of it a swapped part's (part 1: ../dq0/swap.inc), draw_map called through its stub (bank.s's).
*/

#include <string.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"

#pragma code-name ("SW1CODE")
#pragma rodata-name ("SW1RODATA")
#pragma local-strings (on)

static struct chunk* K;
static unsigned char kx, ky, lk = 0xFF, lsn;
static unsigned lcx, lcy;

/* A square of a chunk in the cache, or NULL (outside the stratum, or not in the cache) */
static unsigned char* __fastcall__ peek (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s = &strata[sn];
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

static char __fastcall__ glyph (unsigned char sn, unsigned x, unsigned y, int myz)
{
    unsigned char* p = peek (sn, x, y);
    unsigned char t, n;
    int dz;

    if (!p || !(*p & T_SEEN)) {
        return ' ';
    }
    t = TILE (*p);
    switch (t) {
    case T_FLOOR:
        dz = K->elev[ky][kx] - myz;
        return dz >= 5 ? '1' + (dz - 5 >= 80 ? 8 : (dz - 5) / 10) : dz <= -5 ? 'a' + (-dz - 5 >= 80 ? 8 : (-dz - 5) / 10) :
            '.';
    case T_DOOR:
        for (n = 0; n < K->nportals; ++n) {     /* (A secret door: drawn as the wall it looks like) */
            if (K->portal[n].x == kx && K->portal[n].y == ky && (K->portal[n].state & PS_HIDDEN)) {
                return (p = peek (sn, x, y - 1)) && T_PASS (*p) ? '|' : '-';
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
        if (((p = peek (sn, x, y - 1)) && T_INSIDE (*p)) || ((p = peek (sn, x, y + 1)) && T_INSIDE (*p))) {
            return '-';
        }
        if (((p = peek (sn, x - 1, y)) && T_INSIDE (*p)) || ((p = peek (sn, x + 1, y)) && T_INSIDE (*p))) {
            return '|';
        }
        return '-';
    }
    return ' ';
}

void s_draw_map (void)
{
    static char row[MAP_W + 1];
    struct stratum* s;
    unsigned left, top, i, j, x, y;
    unsigned char n, c, sn;
    unsigned char* p;
    int myz;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    s = &strata[sn];
    myz = (p = peek (sn, x, y)) != 0 ? K->elev[ky][kx] : 0;
    left = s->w <= MAP_W ? s->x0 : x - MAP_W / 2 < s->x0 ? s->x0 : x - MAP_W / 2 > s->x0 + s->w - MAP_W ?
        s->x0 + s->w - MAP_W : x - MAP_W / 2;
    top = s->h <= MAP_H ? s->y0 : y - MAP_H / 2 < s->y0 ? s->y0 : y - MAP_H / 2 > s->y0 + s->h - MAP_H ?
        s->y0 + s->h - MAP_H : y - MAP_H / 2;
    for (j = 0; j < MAP_H; ++j) {
        for (i = 0, n = 0; i < MAP_W; ++i) {
            c = left + i == x && top + j == y ? '@' : glyph (sn, left + i, top + j, myz);
            row[i] = c;
            if (c != ' ') {
                n = i + 1;
            }
        }
        row[n] = 0;
        hub_map (UI_BANK);                      /* (The map the UI bank's) */
        strcpy (UIREC->map[j], row);
    }
    hub_map (0);
    lk = 0xFF;                                  /* (The cache may change before the next) */
}
