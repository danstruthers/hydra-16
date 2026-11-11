/*
** fixt.c - (overlay 4) the words of the dungeon's fixtures (DUNGEON.md: portals; traps, pits and other nasty
** surprises): a denizen in the way, and a portal looked at closely, judged if its quality's asked (or from afar, only
** seen).  What came of working a portal or of a trap (an E_PORTAL's, an E_TRAP's: an opening told with what it shows
** beyond, "You open the door, revealing ...") is said.txt's since 2026-11-11 (said.c, narration as data)
*/

#pragma code-name ("OV4CODE")
#pragma rodata-name ("OV4RODATA")

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

/* A denizen in the character's way (an E_NOWAY's N_DEN: its name in lb): "A giant rat is in the way." */
void o_den_way (void)
{
    static char b[32];

    strcpy (b, lb[0] < 'a' ? "" : strchr ("aeiou", lb[0]) ? "An " : "A ");    /* (A name: Grub) */
    strcat (b, lb);
    strcat (b, " is in the way.");
    msg (b);
}

/* ---- Words added to lb */

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

static void __fastcall__ addn (unsigned v)
{
    char b[6];

    add (utoa (v, b, 10));
}

static const char* const matname[4] = { "wood", "iron-bound wood", "iron", "stone" };

/* ---- A portal looked at from further off than NEAR_SQ (d squares; p the world's record of it): only what's seen
** from afar (the user's: "its relative size and maybe its construction material"), its kind, its size against the
** character's (low, tall; narrow, wide), a door's material if its sight is sharp enough (the referee's, sight_copy's
** eye), whether it stands open; the rest wants a closer look */
static void __fastcall__ portal_far (const struct portal* p, unsigned char d)
{
    unsigned char k = p->kind, w = p->size & 15, h = p->size >> 4;
    const char* a;

    add (k == PK_SLIT ? "An arrow slit" : "A");  /* ("A tall, narrow door of wood") */
    if (k == PK_DOOR || k == PK_PORTCULLIS) {
        a = h <= 5 ? " low" : h >= 8 ? " tall" : "";
        add (a);
        add (w <= 2 ? (*a ? ", narrow" : " narrow") : w >= 5 && !(p->feat & PF_DOUBLE) ? (*a ? ", wide" : " wide") :
             "");
    }
    add (k == PK_PORTCULLIS ? " portcullis" : k == PK_WINDOW ? " window" : k == PK_SLIT ? "" : p->feat & PF_DOUBLE ?
         " double door" : " door");
    if (k == PK_DOOR && d <= 3 + sight_copy.eye) {
        add (" of ");
        add (matname[p->mat & 3]);
    }
    add (", some ");
    addn (SQ_FT * d);
    add (" feet off");
    add (p->state & PS_WRECKED ? ", broken down." : k == PK_PORTCULLIS ? (p->state & PS_OPEN ? ", raised." :
         ", down.") : p->state & PS_OPEN ? ", standing open." : ".");
    add (" You'd have to go closer to make out more.");
}

/* ---- A portal looked at closely (p: the world's record of it, copied, its bar 1 if the character's on its bar's
** side): its kind, its size, its material, its thickness and its features (a grille, a spy-hole, studs, its hinges, a
** bar's brackets on this side, a lock, its make) plain to see; and, its quality asked (ex's EX_QUALITY, the
** appraiser's grade g: the referee's test of the craft that knows its material), its state (rotten ... new) and how
** stout it is (its body left), judged: a critical success right, a marginal one near, a marginal failure well off, a
** critical failure wild (and said as surely).  Its other aspects alone (ex, EX_*, 2026-10-22): its lock, its
** condition (the blows' marks), its value and its weight (none: it's part of the place).  Further off than NEAR_SQ
** (from the character's square at its last look to main.c's find's), only its look from afar */
void __fastcall__ o_portal_look (const struct portal* p, signed char g, unsigned char ex)
{
    static const char* const low[3][4] = {      /* (Its state, the worse half: wood's, iron's, stone's) */
        { "rotten", "rotting", "decayed", "worn" }, { "rusted through", "rusting away", "badly rusted", "rusty" },
        { "crumbling", "crumbling", "cracked", "weathered" }
    };
    static const char* const high[6] = { "sound", "sound", "good", "fine", "fine", "new" };
    unsigned char k = p->kind, m = p->mat & 3, mine = p->bar, f = p->feat, h = k == PK_DOOR ? p->harm : p->harm & 15;
    signed char q = p->qual;
    unsigned body = p->tough > h ? p->tough - h : 1;   /* (Its body left: a window's, its shutters') */
    unsigned dx = tx - sight_copy.x, dy = ty - sight_copy.y;

    lb[0] = 0;
    dx = (int) dx < 0 ? -dx : dx;
    dy = (int) dy < 0 ? -dy : dy;
    if ((dx = dx > dy ? dx : dy) > NEAR_SQ) {
        portal_far (p, dx > 99 ? 99 : dx);
        return;
    }
    if (ex & (EX_VALUE | EX_WEIGHT) && !(ex & EX_QUALITY)) {
        add (ex & EX_VALUE ? "There's no price to put on it: it's part of the place." :
            "It's part of the place: there's no hefting it.");
        return;
    }
    if (ex & EX_LOCK) {                         /* (Its lock alone: its make, how hard it looks to pick) */
        add (!(p->lock || (p->state & PS_LOCKED)) ? "It has no lock." : p->state & PS_BROKEN ?
            "Its lock is broken: it was forced once." : p->lock <= 4 ?
            "It has a simple lock: easy to pick, by the look of it." : p->lock <= 10 ?
            "It has a sound lock: a fair test of a lockpick." : "It has an intricate lock: a hard one to pick.");
        return;
    }
    if (ex & EX_COND) {                         /* (Its condition alone: the blows' marks) */
        add (p->state & PS_WRECKED ? "It's been broken down." : !p->harm ? "It shows no damage." : m == PM_IRON ||
            k == PK_PORTCULLIS ? "It's dented with blows." : "It's splintered with blows.");
        return;
    }
    add (k == PK_PORTCULLIS ? "A portcullis of iron bars, " : k == PK_SLIT ? "An arrow slit, a hand's breadth wide" :
         k == PK_WINDOW ? "A window, " : f & PF_DOUBLE ? "A double door of " : "A door of ");
    if (k == PK_DOOR) {
        add (matname[m]);
        add (", ");
    }
    if (k != PK_SLIT) {
        addn (p->size & 15);
        add (" feet wide");
    }
    add (" and ");
    addn (p->size >> 4);
    add (" high");
    if (k == PK_DOOR || k == PK_PORTCULLIS || (k == PK_WINDOW && (p->parts & PP_SHUTTERS))) {
        add (k == PK_WINDOW ? "; its shutters of wood, " : k == PK_PORTCULLIS ? "; its bars " : ", ");
        addn (p->thick / TH_IN);
        add (p->thick == TH_IN ? " inch thick." : " inches thick.");
    } else {
        add (".");
    }
    if (k == PK_WINDOW) {
        add (p->parts & PP_GLASS ? " It's glazed, in small leaded panes." : "");
        add (p->parts & PP_BARS ? " Iron bars an inch thick are set across it." : "");
        add (p->parts & (PP_GLASS | PP_BARS | PP_SHUTTERS) ? "" : " It's an empty frame.");
        if (p->parts & PP_SKY) {
            add (" It looks on the sky.");
        }
    }
    if (k == PK_DOOR) {
        add (f & PF_STUDS ? " It's studded with iron." : "");
        add (f & PF_GRILLE ? " A little window is cut in it, a grille across it." : f & PF_PEEP ? (mine ?
             " A spy-hole is cut in it, its cover on this side." : " A spy-hole is cut in it, its cover on the far side.") :
             "");
        add (!(f & PF_HINGES) == !mine ? " Its hinges are on this side." : " Its hinges are on the far side.");
        if ((f & PF_BRACKETS) && mine) {
            add (p->state & PS_BARRED ? " Brackets for a bar are fixed on this side, the bar in them." :
                 " Brackets for a bar are fixed on this side, empty.");
        }
        if (p->lock || (p->state & PS_LOCKED)) {
            add (" It has a lock: ");
            add (p->lock <= 4 ? "a simple one." : p->lock <= 10 ? "a sound one." : "an intricate one.");
        }
        add (p->state & PS_WRECKED ? " It's been broken down, and hangs in pieces." : p->state & PS_OPEN ?
             " It stands open." : "");
    }
    if (k == PK_PORTCULLIS && (p->state & PS_WRECKED)) {
        add (" Its bars are forced apart: there's a gap to get through.");
    }
    if (p->harm && !(p->state & PS_WRECKED)) {
        add (m == PM_IRON || k == PK_PORTCULLIS ? " It's dented with blows." : " It's splintered with blows.");
    }
    if (!(ex & EX_QUALITY)) {
        return;                                 /* (A closer look: no judgement, no test) */
    }
    if (p->state & PS_WRECKED) {
        add (" You judge it ruined, past mending.");   /* (Judged by its state, its make no matter now: D13's) */
        return;
    }
    switch (g) {
    case G_WELL:
        break;
    case G_PASS:
        q += random (3) - 1;
        body = body * (8 + random (5)) / 10;
        break;
    case G_FAIL:
        q += random (7) - 3;
        body = body * (4 + random (13)) / 10;
        break;
    default:                                    /* (Wild, and as sure of it) */
        q = random (10);
        body = 1 + random (60);
        break;
    }
    q = q < 0 ? 0 : q > 9 ? 9 : q;
    add (" You judge it ");
    add (q < 4 ? low[m == PM_IRON || k == PK_PORTCULLIS ? 1 : m == PM_STONE ? 2 : 0][q] : high[q - 4]);
    add (body <= 3 ? ", and flimsy: a few blows would break it." : body <= 10 ?
         ": it would take some battering to break." : body <= 30 ?
         ", and stout: it would take a deal of battering to break." : ", and very stout: it would stand a long battering.");
}
