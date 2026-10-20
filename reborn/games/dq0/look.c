/*
** look.c - Dingus Quest Zero's places told (the UI's): what the character perceives, in words (look: words.c's, an
** overlay, from the world's record of it in the hub, level.h's struct sight: the UI never reads the dungeon); and the
** map (the maker draws its window into the hub, on asking), with the character's state beside it.
**
** The words are the character's: only what it sees now (or, in the dark, feels), each where it lies from it as it
** faces (one of eight: ahead, ahead to its right ... ahead to its left), and how far above or below it.  A room's
** size it knows when it's seen the whole of it; a ceiling when the light reaches it.
**
** The words for the places are the theme's (theme.h: a stratum's name, a room's by its kind, a lit room's light;
** theme.c's).
**
** The map's code is a swapped part's (part 3: swap.inc), called through its stub (bank.s's map).
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

char lb[400];                                   /* The words, as they're made (words.c's), */
char* lbe;                                      /*   their end, */
unsigned lbn;                                   /*   and how many a look's said already (streamed: lb_stream) */
unsigned char lb_stream;                        /* A look's words said a sentence at a time as they're made (words.c's
                                                **   add): its first out while the rest is worked out */
unsigned told[TOLD];                            /* The sentences the looks have said this answer, each one's hash
                                                **   (words.c's lb_out: none said twice; none yet as a command's taken), */
unsigned char ntold;                            /*   and how many */
struct sight sight_copy;                        /* The hub's record, copied for the overlay */
unsigned char sight_light;                      /*   and the character's light, */
char sight_lamp[14];                            /*   what gives it, named */
unsigned char sight_close;                      /*   and a close look (look's first 2) */

/* The senses' record (their bank's) copied out, into sight_copy: what reads it reads the copy */
struct sight* sight_get (void)
{
    hub_mem (SIGHT_BANK, &sight_copy, SIGHT, sizeof sight_copy);
    return &sight_copy;
}

/* The things' words (the UI bank's things_text) copied out, into lb */
const char* tt (void)
{
    hub_mem (UI_BANK, lb, UIREC->things_text, sizeof lb);
    return lb;
}

/* What the character perceives (first: the first time there; 2, a close look, the room's figures with it; | 4, as a
** run or a sprint ends: briefly; 8, a look one way's: what it sees that way, way.c's): the senses' record copied,
** worded (overlay 2) */
void __fastcall__ look (unsigned char first)
{
    sight_close = first & 10;                   /* (The theme's words for a place's first sight: later) */
    sight_get ();
    sight_copy.first |= first & 4 ? SIGHT_HURRY : 0;
    sight_light = HUB->pc_light;
    strcpy (sight_lamp, HUB->pc_lamp);
    pc_face = HUB->pc_face;
    if (names_n) {
        names_look (0);                         /* (Its things named marked; then the room's name: names.c's) */
    }
    lb_stream = !names_n;                       /* (Said as it's made, but with names: made whole, the room's name
                                                **   put in it after) */
    words ();
    if (!lb_stream) {
        names_look (1);
        msg (lb);
    }
    lb_stream = 0;
}

/* A denizen the character sees has moved (the world's E_VIEW, V_STIR): the senses' record copied, worded (overlay
** 2), and told if what it sees of them has changed */
void stir (void)
{
    sight_get ();
    pc_face = HUB->pc_face;
    stir_words ();
    if (lb[0]) {
        msg (lb);
    }
}

/* ---- The map, and beside it (under it, if the screen's too narrow) the character's state: where it is (its
** stratum, its depth, its square), the way it faces, its light, its senses as they are now (the summary's, whatever
** lowers them in them), and the ruleset's short lines (the referee's: the attributes, the penalties) */

#define PANEL       14                          /* The panel's width */
#define PLINES      18
#define panel       ((char (*)[PANEL + 1]) lb)  /* (In lb: no look's being made as the map's shown; 270 bytes of the
                                                **   main RAM) */
typedef char panel_fits[PLINES * (PANEL + 1) <= sizeof lb ? 1 : -1];
static unsigned char np;

#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

static void __fastcall__ pline (const char* s)
{
    if (np < PLINES) {
        strncpy (panel[np], s, PANEL);
        panel[np++][PANEL] = 0;
    }
}

/* A line of the panel: words, a number, words */
static void __fastcall__ num (const char* a, int v, const char* z)
{
    static char b[PANEL + 1];
    char n[8];

    strcpy (b, a);
    strcat (b, itoa (v, n, 10));
    strcat (b, z);
    pline (b);
}

/* The panel's lines, from the hub's header, and the referee's (rules: its lines, each ended by a new line) */
static void __fastcall__ make_panel (const char* rules)
{
    static char b[PANEL + 1];
    static struct { unsigned char sname, style; int z; } w;
    const char* e;
    unsigned char n;

    np = 0;
    hub_mem (SIGHT_BANK, &w, &SIGHT->sname, sizeof w);   /* (The senses' record's: the stratum's name, the depth) */
    theme_named (TN_STRATUM, w.sname);
    pline (tname);
    num ("Depth ", -w.z, " ft");
    num ("Sq ", (int) (HUB->pc_x - 32768U), ",");
    strcat (panel[np - 1], itoa ((int) (HUB->pc_y - 32768U), b, 10));
    strcpy (b, "Facing ");
    strcat (b, head_abbr[HUB->pc_face & 7]);
    pline (b);
    if (HUB->pc_light) {
        num ("Light ", HUB->pc_light * SQ_FT, " ft");
    } else {
        pline ("No light");
    }
    num ("Sight ", HUB->pcsum.sight, "");
    num ("Hearing ", HUB->pcsum.hearing, "");
    num ("Smell ", HUB->pcsum.smell, "");
    pline ("1-9 up a-i dn");
    while (*rules) {                            /* (The referee's, a line each) */
        e = strchr (rules, '\n');
        n = e ? e - rules : strlen (rules);
        strncpy (b, rules, n > PANEL ? PANEL : n);
        b[n > PANEL ? PANEL : n] = 0;
        pline (b);
        rules += e ? n + 1 : n;
    }
}

void __fastcall__ s_map (const char* rules)
{
    static char line[MAP_W + PANEL + 3];
    unsigned char x, y, n, side, k;

    make_panel (rules);
    hub_call (&HUB->mapq, T_MAKER);             /* (The maker draws it into the UI bank) */
    blank ();
    sayf ("The map (you face %s; heights in tens of feet against yours):", head_name[HUB->pc_face & 7]);
    hub_map (UI_BANK);                          /* (The map's the UI bank's: its rows copied out one by one) */
    for (n = 0; n < MAP_H - 1 && !UIREC->map[n][0]; ++n) {
    }
    for (y = MAP_H - 1; y > n && !UIREC->map[y][0]; --y) {
    }
    hub_map (0);
    side = tw > MAP_W + PANEL + 1;
    for (k = 0; n + k <= y || (side && k < np); ++k) {  /* (The rows from the first seen to the last, the panel's
                                                **   beside them) */
        line[0] = 0;
        if (n + k <= y && n + k < MAP_H) {
            hub_mem (UI_BANK, line, UIREC->map[n + k], MAP_W + 1);
        }
        if (side && k < np) {
            for (x = strlen (line); x < MAP_W + 1; ++x) {
                line[x] = ' ';
            }
            strcpy (line + MAP_W + 1, panel[k]);
        }
        say_raw (line);
    }
    HUB->mapq.state = REQ_FREE;
    if (!side) {
        blank ();
        for (k = 0; k < np; ++k) {
            say_raw (panel[k]);
        }
    }
    blank ();
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 8: where in its room the character is, and the things it sees there, in words (HOUSE.md's where you are:
** by the facing alone, the user's, never the compass; only what's known, the senses' record's copy) */

char where_w[34];

#pragma code-name (push, "SW8CODE")
#pragma rodata-name (push, "SW8RODATA")

#define SC          sight_copy

/* An actor's square (x, y) in sight, the senses' last look's (blows.c's: one closed with) */
unsigned char __fastcall__ s_den_seen (unsigned x, unsigned y)
{
    unsigned at[2];
    unsigned char n, k;

    hub_mem (SIGHT_BANK, &n, &SIGHT->ndens, 1);
    for (k = 0; k < n; ++k) {
        hub_mem (SIGHT_BANK, at, &SIGHT->den[k].x, sizeof at);
        if (at[0] == x && at[1] == y) {
            return 1;
        }
    }
    return 0;
}

/* The character's place in its room (the floor in thirds each way, far, middle and near, left, middle and right) and
** the room's size and shape (its floor: small under 150 square feet, large to 2,500, vast beyond; long, twice as long
** as it's wide, narrow too if 10 feet across or less; wide, 40 feet across or more), into where_w: "You are in the far
** left corner of a large, long "; the offset just after its article's a (words.c's "an" before a vowel) */
unsigned char s_room_where (void)
{
    static const char* const at_[9] = { "in the far left corner of", "at the far end of", "in the far right corner of",
                                        "on the left side of", "in the middle of", "on the right side of",
                                        "in the near left corner of", "at the near end of", "in the near right corner of" };
    unsigned char dx, dy, q, w, d, f, r, n, mn, lng;
    unsigned area;
    char* o = lbe;                              /* (Put at lb's end: words.c's room) */

    dx = SC.x - SC.rx;
    dy = SC.y - SC.ry;
    strcpy (o, "You are ");
    if (dx >= SC.rw || dy >= SC.rh) {
        strcat (o, "in");                       /* (Not on its floor: in a doorway) */
    } else {
        q = ((pc_face + 1) & 7) >> 1;           /* (The nearest of the four it faces) */
        w = q & 1 ? SC.rh : SC.rw;              /* (Across it, as it faces; its depth) */
        d = q & 1 ? SC.rw : SC.rh;
        f = q == 0 ? dy : q == 1 ? SC.rw - 1 - dx : q == 2 ? SC.rh - 1 - dy : dx;     /* (From the far wall, the right) */
        r = q == 0 ? SC.rw - 1 - dx : q == 1 ? SC.rh - 1 - dy : q == 2 ? dx : dy;
        strcat (o, at_[(d < 3 ? 1 : f < d / 3 ? 0 : d - 1 - f < d / 3 ? 2 : 1) * 3 +
            (w < 3 ? 1 : w - 1 - r < w / 3 ? 0 : r < w / 3 ? 2 : 1)]);
    }
    strcat (o, " a");
    n = strlen (o);
    area = SC.rw * SC.rh;
    mn = SC.rw < SC.rh ? SC.rw : SC.rh;
    lng = (SC.rw ^ SC.rh ^ mn) >= 2 * mn;
    if (lng && mn <= 2) {
        strcat (o, " long, narrow ");
    } else {
        strcat (o, area < 6 ? " small" : area < 24 ? "" : area < 100 ? " large" : " vast");
        strcat (o, area >= 6 && area < 24 ? (lng ? " long " : " ") : lng ? ", long " : mn >= 8 ? ", wide " : " ");
    }
    return n;
}

/* A thing seen lying in the character's room (sight_copy's item i), not hidden nor at its feet: on the floor's edge,
** its words for seen.c into where_w (a space before them: "In the far left corner ", "Against the wall to your left "):
** 1; else 0 */
unsigned char __fastcall__ s_lie_words (unsigned char i)
{
    static const char* const wall_[4] = { "wall ahead ", "wall to your right ", "wall behind you ", "wall to your left " };
    unsigned char dx, dy, q, k;

    dx = SC.item[i].x - SC.rx;
    dy = SC.item[i].y - SC.ry;
    if ((SC.item[i].hid & 1) || SC.item[i].rel == 0xFF || SC.where != W_ROOM || SC.size == S_PART || SC.rw < 3 || SC.rh < 3 || dx >= SC.rw || dy >= SC.rh) {
        return 0;
    }
    q = ((pc_face + 1) & 7) >> 1;
    k = !dy | (dx == SC.rw - 1) << 1 | (dy == SC.rh - 1) << 2 | !dx << 3;   /* (Against the wall to the north, east,
                                                **   south, west) */
    k = (k >> q | k << (4 - q)) & 15;           /* (As it faces: ahead, to its right, behind it, to its left) */
    if (!k) {
        return 0;
    }
    if ((k & 5) && (k & 10)) {
        strcpy (where_w, k & 1 ? " In the far " : " In the near ");
        strcat (where_w, k & 8 ? "left corner " : "right corner ");
        return 1;
    }
    strcpy (where_w, " Against the ");
    strcat (where_w, wall_[k & 1 ? 0 : k & 2 ? 1 : k & 4 ? 2 : 3]);
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
