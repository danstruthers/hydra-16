/*
** wworn.c - (dq0w, swapped part worn: ../dq0/swap.inc) where a thing's worn (the user's, 2026-11-10: "Wearing should
** extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should keep track of where worn (as
** should all worn items that can be worn in more than one place)"; HOUSE.md's worn places).  A thing of a kind worn in
** more than one place (kinds.c's wplaces) is worn at a place, its record's sn (items.h's WP_*: a place on the body,
** PB_*, its right side, a pair), as many there as the place holds (kinds.c's wroom: four rings a hand); its place said (the
** UI's worn.c: the request's into, "@" and the place), else its kind's (a ring the off hand's, then the other's when
** that's full).  What's worn another way (clothes, a pack, a belt) is one of a kind, as it was.  Its words where it's
** worn: the inventory's, a look's, a denizen's (its place drawn by its own number: a denizen's things' sn is the
** stratum they're filed in); which one's taken off, asked by place
*/

#include <string.h>
#include <stdlib.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "../dq0/rs.h"
#include "world.h"
#include "things.h"

void __fastcall__ put (const char* s);          /* (tlist.s's: the hub's text, the words put after what's there) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;
unsigned char __fastcall__ named (const char* k, const char* w);
extern struct ireq iq;
extern const struct wplace wplaces[];           /* (kinds.c's, in this part's image) */
extern const unsigned char wroom[];

unsigned char wplace = WP_NONE;                 /* (The place a thing's to be worn at, worn_at's: things.c's don's, WP_NONE
                                                **   one worn the one way; the main RAM's, the catalogue's code reads it) */

#pragma code-name ("SW_worn_CODE")
#pragma rodata-name ("SW_worn_RODATA")
#pragma bss-name ("SW_worn_RODATA")                /* (Its own data, wanted only while it runs: not the main RAM's) */
#pragma local-strings (on)
#pragma static-locals (on)

static char kn[14];                             /* (A kind's name, copied out) */

/* Kind k's place's record (wplaces'), or 0: one worn the one way; its name in kn */
static const struct wplace* __fastcall__ wp_of (unsigned char k)
{
    unsigned char j;

    hub_map (KIND_BANK);
    memcpy (kn, CATALOGUE[k < NKINDS ? k : 0].name, sizeof kn);
    for (j = 0; j < NWPLACES; ++j) {
        if (wplaces[j].kind == k) {
            return wplaces + j;
        }
    }
    return 0;
}

/* Thing i's place (WP_*), or WP_NONE (one worn the one way): its sn; a denizen's (its sn its stratum), drawn by its own
** number from those its kind's worn at; one with none yet (an outfit's, the kit's), its kind's own, on the left */
static unsigned char __fastcall__ place_of (unsigned i)
{
    const struct wplace* w;
    struct item* p = item_at (i);
    unsigned id = p->id;
    unsigned char s = p->sn, k = p->kind, den = p->where == IW_DEN, l, n;

    if (!(w = wp_of (k))) {
        return WP_NONE;
    }
    if (den) {
        for (n = l = 0; l < NPB; ++l) {
            n += (w->allow >> l) & 1;
        }
        for (n = id % n, l = 0; !((w->allow >> l) & 1) || n--; ++l) {
        }
        return l | (w->dflt & WP_PAIR) | (PB_SIDED (l) && (id & 2) ? WP_RIGHT : 0);
    }
    l = s & WP_LOC;
    return (s & 0x30) != WP_AT || !((w->allow >> l) & 1) || ((s ^ w->dflt) & WP_PAIR) ? w->dflt : s & ~WP_AT;
}

/* Place s in words, how says whose (SY_ITS: a denizen's, else yours) and whether both sides (SY_BOTH, a hand's either):
** "on your left hand", "about its neck", "in your ears", "on your hands" */
#define SY_ITS          1
#define SY_BOTH         2
#define SY_THEY         4                       /* (SY_ITS's, a person's: "their", 2026-11-17) */
static void __fastcall__ say_at (unsigned char s, unsigned char how)
{
    static const char* const prep[4] = { "on", "over", "about", "at" };
    static const unsigned char pi[NPB] = { 0, 1, 2, 0, 0, 3, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0 };
    static const char* const part[NPB] = { "head", "face", "neck", "shoulder", "arm", "elbow", "wrist",
        "hand", "breast", "waist", "groin", "hip", "thigh", "knee", "calf", "foot" };
    unsigned char l = s & WP_LOC;

    put (s & WP_PAIR ? "in" : prep[pi[l]]);
    put (how & SY_ITS ? (how & SY_THEY ? " their " : " its ") : " your ");
    if (s & WP_PAIR) {
        put ("ears");
        return;
    }
    if (PB_SIDED (l) && !(how & SY_BOTH)) {
        put (s & WP_RIGHT ? "right " : "left ");
    }
    put (part[l]);
    if (how & SY_BOTH) {
        put ("s");
    }
}

/* How many things the character wears at place s */
static unsigned char __fastcall__ worn_n (unsigned char s)
{
    unsigned i = 0;
    unsigned char c = 0;

    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        if ((ip->flags & I_WORN) && place_of (i) == s) {
            ++c;
        }
    }
    return c;
}

/* Thing n (of kind k: carried, or within reach) to be worn (things.c's don's, the catalogue's code's): 0, its place in
** wplace (WP_NONE, one of a kind); else the words why not, SA_* (the catalogue's), or 0xFF (said here): it worn already;
** one of its kind worn already (one worn the one way); the place said not one it's worn at; no room left there (unsaid,
** a hand's: on either) */
unsigned char __fastcall__ s_worn_at (unsigned n, unsigned k)
{
    const struct wplace* w;
    unsigned i = 0;
    unsigned char s, l, said, t;

    wplace = WP_NONE;
    if (item_at (n)->flags & I_WORN) {
        return SA_WORN;
    }
    if (!(w = wp_of (k))) {
        while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {  /* (One of a kind worn: a second pack, a second cloak, no) */
            if (ip->kind == k && (ip->flags & I_WORN)) {
                return SA_WEAR_ONE;
            }
        }
        return 0;
    }
    said = iq.into[0] == '@' ? iq.into[2] : 0;  /* (The place said: worn.c's; its side, l or r, else -) */
    l = said ? (iq.into[1] - 'A') & WP_LOC : w->dflt & WP_LOC;
    s = l | (w->dflt & WP_PAIR);
    hub_map (0);
    t = !PB_SIDED (l) ? 0 : said == 'l' ? 0 : said == 'r' ? WP_RIGHT : HUB->pcsum.limbs & LB_LEFT ? WP_RIGHT : 0;
    s |= t;                                     /* (Its side: the one said; else the off hand's, then the other's) */
    if (said && !((w->allow >> l) & 1)) {
        put ("You can't wear the ");
        put (kn);
        put (" ");
        say_at (s, 0);
        put (".");
        return 0xFF;
    }
    t = PB_SIDED (l) && said != 'l' && said != 'r';
    if (worn_n (s) < wroom[l] || (t && worn_n (s ^= WP_RIGHT) < wroom[l])) {
        wplace = s | WP_AT;
        return 0;
    }
    put ("There's no room left ");
    say_at (s, t ? SY_BOTH : 0);
    put (" for the ");
    put (kn);
    put (".");
    return 0xFF;
}

/* Where thing n's worn, in words after its name (how: WH_LIST the inventory's, ", on your left hand", else ", worn";
** WH_LOOK a look's, after "worn", " on your left hand"; WH_DEN a denizen's, " on its left hand"), one worn the one way
** none but the inventory's ", worn" */
void __fastcall__ s_worn_put (unsigned n, unsigned char how)
{
    unsigned char s = place_of (n);

    if (s == WP_NONE) {
        if (how == WH_LIST) {
            put (", worn");
        }
        return;
    }
    put (how == WH_LIST ? ", " : " ");
    say_at (s, how == WH_DEN ? SY_ITS : how == WH_THEY ? SY_ITS | SY_THEY : 0);
}

/* "Take off the ring" with rings worn in two places or more (IO_DOFF's, things.c's serve: its thing named by its word,
** not its letter): which one, asked by place, each place's first by its id, kind, count and state (WS_AT: the place's
** words after it), "12 104 1 32 , on your left hand|", as the take's which asks (caps.c's o_which_ask): IR_WHICH; else 0
** (one place, or none: the handling's own) */
#define NWHICH          4
unsigned char s_worn_which (void)
{
    static unsigned id[NWHICH];
    static unsigned char pl[NWHICH], kd[NWHICH];
    unsigned i = 0;
    unsigned char m = 0, j, s;
    char b[6];

    if (iq.name[0] == '#' || iq.sel) {
        return 0;                               /* (By its letter, or a selector said: the chooser's) */
    }
    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        if (!(ip->flags & I_WORN) || !ip->id || (s = place_of (i)) == WP_NONE || !named (kn, iq.name)) {
            continue;                           /* (place_of's kind's name in kn) */
        }
        for (j = 0; j < m && pl[j] != s; ++j) {
        }
        if (j == m && m < NWHICH) {
            id[m] = item_at (i)->id;
            kd[m] = item_at (i)->kind;
            pl[m++] = s;
        }
    }
    if (m < 2) {
        return 0;
    }
    put_bank = UI_BANK;                         /* (The text begun again: things.c's text_new) */
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
    *tp = 0;
    for (j = 0; j < m; ++j) {
        put (utoa (id[j], b, 10));
        put (" ");
        put (utoa (kd[j], b, 10));
        put (" 1 32 , ");               /* (WS_AT's: its place's words after it) */
        say_at (pl[j], 0);
        put ("|");
    }
    return IR_WHICH;
}
