/*
** fixt.c - (overlay 4) the words of the dungeon's fixtures (DUNGEON.md: portals; traps, pits and other nasty
** surprises): what came of working a portcullis, a window or a slit (an E_PORTAL's: a door's are intro.c's), what a
** trap did, or what came of finding or disarming one (an E_TRAP's: a trap's name is words.c's); what came of blows
** at a portal, of a part lifted out of one; and a portal looked at closely, judged (or from afar, only seen)
*/

#pragma code-name ("OV4CODE")
#pragma rodata-name ("OV4RODATA")

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include "dq0.h"
#include "hub.h"

/* What a trap did, or what came of it (w: an E_TRAP's) */
void __fastcall__ o_trap_words (unsigned w)
{
    static const char* const set_off[4] = {
        "A plate sinks under your foot with a click.", "A wire parts against your shin.",
        "As the door opens, something clicks.", ""
    };
    static const char* const does[8] = {
        "", "", "", " Stones crash down from above!", " A blade scythes out of the wall!",
        " Darts hiss out of the wall!", " A cloud of choking gas billows up!",
        " Somewhere near, a bell clangs, loud and long."
    };
    unsigned char kind = w & 0x0F, trigger = w >> 4 & 3, o = w >> 8;

    lb[0] = 0;
    switch (o) {
    case TO_SPRUNG:
    case TO_DODGED:
    case TO_RESISTED:
    case TO_CHOKED:
        if (kind <= TK_SPIKED) {
            msg (kind == TK_SPIKED ? "The floor gives way beneath you: a pit, spikes at its foot!" :
                "The floor gives way beneath you: a pit!");
            return;
        }
        strcpy (lb, set_off[trigger]);
        strcat (lb, does[kind] + (trigger == 3));  /* (Set off as it was being disarmed: no trigger's words) */
        strcat (lb, o == TO_DODGED ? " You throw yourself clear." : o == TO_RESISTED ?
            " You hold your breath and stagger clear of it." : o == TO_CHOKED ?
            " You choke on it; everything goes black, and you come to some time later." : "");
        break;
    case TO_OPENED:
        strcpy (lb, "The pit's cover falls in.");
        break;
    case TO_NOTICED:
    case TO_FOUND:
        strcpy (lb, o == TO_NOTICED ? "You stop short: just ahead is a trap, " : "You find a trap: ");
        trap_name (kind, trigger, 0);
        strcat (lb, ".");
        break;
    case TO_DISARMED:
        strcpy (lb, "You work at the trap a while, and disarm it.");
        break;
    case TO_NOT_YET:
        strcpy (lb, "You work at the trap a while, but it defeats you, for now.");
        break;
    case TO_FUMBLED:
        strcpy (lb, "Your hand slips: the trap goes off!");
        break;
    case TO_ASSESSED:
        strcpy (lb, "You study the mechanism a while, and make out how it works: now to disarm it.");
        break;
    case TO_PUZZLED:
        strcpy (lb, "You study the mechanism a while, but can't make out how it works.");
        break;
    case TO_CANT:
        strcpy (lb, "It's set by magic: you've no means to undo it.");
        break;
    case TO_PIT:
        strcpy (lb, "There's no disarming a pit: you'll have to go round it.");
        break;
    default:
        strcpy (lb, "There's no trap you know of within reach.");
        break;
    }
    msg (lb);
}

/* A denizen in the character's way (an E_NOWAY's N_DEN: its name in lb): "A giant rat is in the way." */
void o_den_way (void)
{
    static char b[32];

    strcpy (b, lb[0] < 'a' ? "" : strchr ("aeiou", lb[0]) ? "An " : "A ");    /* (A name: Grub) */
    strcat (b, lb);
    strcat (b, " is in the way.");
    msg (b);
}

/* What came of working a portal (w: an E_PORTAL's, its kind times 256): a portcullis's, a window's or a slit's
** words, or a door's (intro.c's) */
void __fastcall__ o_fixt_words (unsigned w)
{
    static const char* const said[PO_FOUND_TRAP - PO_RAISED] = {
        "You wind the winch beside it, and the portcullis grinds up.",
        "You heave the portcullis up by its bars, and it catches, raised.",
        "You loose the winch: the portcullis crashes down.",
        "The portcullis is raised already.", "The portcullis is down already.",
        "Its winch is on the other side: you can't lower it from here.",
        "You heave at the portcullis, but it won't rise.", "That isn't a thing raised or lowered.",
        "It has no shutters to open or close.",
        "You strain at the bars, and bend them apart: there's room to get through.", "The bars won't give.",
        "The glass shatters with a crash.", "The shutters splinter and give way.", "There's nothing in it to break.",
        "You can't climb through: it's shut, glazed or barred.",
        "An arrow slit: only light and arrows pass it, and it's too narrow to climb through.",
        "You climb up onto the sill, and through.", "You can't get up onto the sill.",
        "You slip from the sill, and fall.", "That isn't a window: walk through it.",
        "There's a trap on that door you know of: disarm it before you open it.",
    };
    static const char* const shutters[PO_HELD + 1] = {
        "There's no window that way.", "You open the shutters.", "You close the shutters.",
        "The shutters stand open already.", "The shutters are shut already.", "The shutters are locked.",
        "The shutters are barred from the other side.", "You lift the bar and open the shutters.", 0, 0, 0,
        "There's no lock on them to pick.", "With a crack the shutters burst open.", "It holds."
    };
    static const char* const blows[PO_JAMMED + 1 - PO_BROKE_DOWN] = {
        "You batter at it till it gives way, and it crashes open.", "You batter at it a while: it's starting to give.",
        "You batter at it a while: it's marked, but it holds.", "Your blows make no mark on it.",
        "It's broken: it won't shut.", "You lift the bar out of its brackets, and set it down.",
        "The bar's on the far side: you can't get at it from here.", "There's nothing there to lift out.",
        "The bars are set fast in the stone: bend them, or break them, first.",
        "You work the pane free of its frame, and lift it out whole.", "You work the bent bars free, and lift them out.",
        "The bars break out of the stone with a clang.",
        "You set the bar in its brackets: the door is barred.", "There are no brackets for a bar on this side.",
        "It's barred already.", "It stands open: shut it first, then bar it.",
        "It was burst open once: its brackets won't hold a bar now.",
        "You have no bar to put in its brackets (carry one, or set one down by it).",
        "That bar is too short to span the door.",
        "The bars buckle and break under your blows: there's a gap to get through.",
        "Its bars are forced apart, and it's jammed where it is: there's a gap to get through."
    };
    unsigned char out = w & 0xFF, kind = w >> 8;

    if (out > PO_FOUND_TRAP) {
        msg (blows[out - PO_BROKE_DOWN]);
        return;
    }
    if (out >= PO_RAISED) {
        if (out < PO_FOUND_TRAP) {
            msg (out == PO_NOT_PORTC && kind == PK_PORTCULLIS ? "It's a portcullis: it's raised or lowered." :
                said[out - PO_RAISED]);
        }
        return;
    }
    if ((kind == PK_WINDOW || kind == PK_SLIT) && out <= PO_HELD && shutters[out]) {
        msg (shutters[out]);
        return;
    }
    portal_words (out);
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
** side), judged at the appraiser's grade g (the referee's test of the craft that knows its material): its kind, its
** size, its material, its thickness and its features (a grille, a spy-hole, studs, its hinges, a bar's brackets on
** this side, a lock) plain to see; its state (rotten ... new) and how stout it is (its body left), judged: a critical
** success right, a marginal one near, a marginal failure well off, a critical failure wild (and said as surely).
** Further off than NEAR_SQ (from the character's square at its last look to main.c's find's), only its look from
** afar */
void __fastcall__ o_portal_look (const struct portal* p, signed char g)
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
            add (g < G_MS ? "of what sort, you can't tell." : p->lock <= 4 ? "a simple one." : p->lock <= 10 ?
                 "a sound one." : "an intricate one.");
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
    switch (g) {
    case G_CS:
        break;
    case G_MS:
        q += random (3) - 1;
        body = body * (8 + random (5)) / 10;
        break;
    case G_MF:
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
