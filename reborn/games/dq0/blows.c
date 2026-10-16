/*
** blows.c - (overlay 6) a fight's words: a blow struck (an E_FIGHT: the character's at a denizen, or one's at it), how
** it came out (rs.h's O_*), what the strikes did (the referee's words: the wounds, or the armour that stopped them),
** and what it left them in (fallen, senseless, dead, fleeing): "You strike at the giant rat with your sword: a
** serious cut to its left thigh (S2). The giant rat falls senseless."  Into lb, for main.c to say (the blow's record
** copied into lb's last FIGHT_LB bytes: the words never as long as the rest).
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")

#include <string.h>
#include "hub.h"
#include "rs.h"
#include "dq0.h"

static char* t;                                 /* (Where the next words go in lb) */
static const char* line[4];                     /* The referee's lines: the attacker's weapon, the defender's, what the
                                                **   attacker's strike did, what the defender's did */
static const char* aname;                       /* The attacker's name, the defender's (0: the character) */
static const char* dname;

static void __fastcall__ add (const char* s)
{
    strcpy (t, s);
    t += strlen (t);
}

/* One of them: "you" (or "You"), "the giant rat" (or "The") */
static void __fastcall__ who (const char* n, unsigned char cap)
{
    if (!n) {
        add (cap ? "You" : "you");
        return;
    }
    if (*n == '?') {
        add (cap ? "Something" : "something");  /* (Not seen) */
        return;
    }
    add (cap ? "The " : "the ");
    add (n);
}

/* A strike's words (its line: a wound, "a ..."; or the armour that stopped it, "your ...", "its ..."; or none: no
** harm) after what led to it: ": a serious cut ...", ", but your leather tunic turns the blow", ", but does no harm"
** (by the striker: the character's "do") */
static void __fastcall__ struck (unsigned char n, unsigned char you)
{
    const char* l = line[n];

    if (*l == 'a' && l[1] == ' ') {
        add (": ");
        add (l);
    } else if (*l) {
        add (", but ");
        add (l);
        add (" turns the blow");
    } else {
        add (you ? ", but do no harm" : ", but does no harm");
    }
}

void __fastcall__ o_fight_words (const struct msg* m)
{
    static unsigned char n;                     /* (What the character sees: whether it sees the denizen) */
    static unsigned at[2];
    struct fight* f = FIGHT_LB;
    unsigned char out = m->x[1], fl = m->x[2], pc = !m->who, k;
    const char* aw;
    char* s;

    hub_mem (DEN_BANK, f, FIGHT_AT + m->x[3] % NFIGHTS, sizeof *f);
    hub_mem (0, &n, &HUB->sight.ndens, 1);
    for (k = n; k--; ) {
        hub_mem (0, at, &HUB->sight.den[k].x, sizeof at);
        if (at[0] == m->w1 && at[1] == m->w2) {
            break;
        }
    }
    if (k == 0xFF) {
        strcpy (pc ? f->def : f->att, "?");      /* (Not seen: something) */
    }
    t = lb;
    *t = 0;
    aname = f->att[0] ? f->att : 0;
    dname = f->def[0] ? f->def : 0;
    f->text[sizeof f->text - 1] = 0;
    for (k = 0, s = f->text; k < 4; ++k) {      /* (Its four lines, cut apart where they are) */
        line[k] = s;
        while (*s && *s != '\n') {
            ++s;
        }
        if (*s) {
            *s++ = 0;
        }
    }
    aw = line[0];
    if (out == FO_GONE) {
        add (dname ? "The " : "It");
        add (dname ? dname : "");
        add (dname ? " is out of reach." : " is out of reach.");
        return;
    }
    if (out == FO_BLED) {
        who (aname, 1);
        add (" collapses, bled white.");
        return;
    }
    if (out == O_RISE) {
        who (aname, 1);
        add (pc ? " get to your feet." : " gets to its feet.");
        return;
    }
    if (out == O_DISPATCH) {
        add ("You finish ");
        who (dname, 0);
        add (" where it lies.");
    } else {
        who (aname, 1);                         /* (The blow: "You strike at the giant rat with your sword", "The
                                                **   ghoul claws at you", "The kobold strikes at you with its spear") */
        if (pc) {
            add (" strike at ");
            who (dname, 0);
            add (" with your ");
            add (aw);
        } else if (!strcmp (aw, "bite") || !strcmp (aw, "claws")) {
            add (*aw == 'b' ? " bites at you" : " claws at you");
        } else {
            add (" strikes at you with its ");
            add (aw);
        }
        switch (out) {
        case O_MISS:
            add (pc ? ", and miss" : ", and misses");
            break;
        case O_DODGED:
            add (pc ? "; it dodges" : "; you dodge");
            break;
        case O_BLOCKED:
            add (pc ? "; it fends off the blow" : "; you fend it off with your ");
            if (!pc) {
                add (line[1]);
            }
            if (*line[2]) {                     /* (A bare arm struck as it blocked) */
                struck (2, pc);
            } else if (*line[3]) {              /* (A claw, a bite, struck by what blocked it) */
                add (pc ? "; it strikes you as it does" : "; your blow lands on it");
                struck (3, 0);
            }
            break;
        case O_HIT:
            struck (2, pc);
            break;
        case O_COUNTER_HIT:
            add (pc ? "; it strikes first" : "; you strike first");
            struck (3, !pc);
            break;
        case O_BOTH_HIT:
            add (pc ? "; you strike each other. Your blow" : "; you strike each other. Its blow");
            struck (2, 0);
            add (pc ? "; its" : "; yours");
            struck (3, 0);
            break;
        case O_FUMBLE:
        case O_STUMBLE:
            k = f->which;
            if (k & 1) {
                add (out == O_FUMBLE ? (pc ? ", but fumble the blow" : ", but fumbles the blow") :
                    pc ? ", but stumble" : ", but stumbles");
            }
            if (k & 2) {
                add (out == O_FUMBLE ? (pc ? "; it falters" : "; you fumble your guard") :
                    pc ? "; it stumbles" : "; you stumble");
            }
            if ((fl & FF_PRONE) && out == O_STUMBLE) {
                add (" and fall");
            }
            break;
        case O_ADVANTAGE:
            add (pc ? "; it slips inside your guard" : "; you see an opening");
            break;
        }
        add (".");
    }
    if (fl & (FF_A_DEAD | FF_D_DEAD)) {         /* (What it left them in) */
        add (" ");
        who (fl & FF_A_DEAD ? aname : dname, 1);
        add ((fl & FF_A_DEAD ? pc : !pc) ? " are dead." : " falls dead.");
    } else if (fl & (FF_A_DOWN | FF_D_DOWN)) {
        add ((fl & FF_A_DOWN ? pc : !pc) ? " Everything goes black." : " It falls senseless.");
    }
    if (fl & FF_FLEES) {
        add (" It flees.");
    }
}

/* A word of a fight's, into lb: the stance taken (rs.h's D_*), or why there's no blow (FN_*) */
enum { FN_AWAY = 5, FN_NONE_LIKE, FN_NONE };

static void __fastcall__ note (unsigned char n)
{
    static const char* const say[8] = {
        "You will defend yourself as best you can.", "You will block.", "You will dodge.",
        "You will strike back first.", "You will not defend yourself.", "It's out of reach: go up to it first.",
        "There's nothing like that here to fight.", "There's nothing beside you to fight."
    };

    strcpy (lb, say[n & 7]);
}

/* The denizen beside the character to strike at (one whose name has the word w, if it's given; the one that way,
** dir, a heading, if that is): its entry plus 1; or 0, and why not in lb */
unsigned char __fastcall__ o_fight_target (const char* w, unsigned char dir)
{
    static struct actor q;
    static struct { unsigned char sn; unsigned x, y; } pc;
    unsigned char k, away = 0;
    int dx, dy;

    hub_mem (0, &pc.sn, (const void*) &HUB->pc_stratum, 1);
    hub_mem (0, &pc.x, (const void*) &HUB->pc_x, 4);
    for (k = 0; k < NACTORS; ++k) {
        hub_mem (0, &q, &HUB->actor[k], sizeof q);
        dx = q.x - pc.x;
        dy = q.y - pc.y;
        if (q.name[0] && q.sn == pc.sn && (!*w || strstr (q.name, w)) && (dir > H_NW || bearing (dx, dy) == dir)) {
            if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) {
                return k + 1;
            }
            away = 1;
        }
    }
    note (away ? FN_AWAY : *w ? FN_NONE_LIKE : FN_NONE);
    return 0;
}

/* The character's stance (the hub's pc_stance, rs.h's D_*: the referee's defence for it) set by its word (dodge,
** block, counter, none; else the best), told in lb */
void __fastcall__ o_fight_stance (const char* w)
{
    static unsigned char s;
    unsigned char d = *w;

    s = d == 'd' ? D_DODGE : d == 'b' && w[1] == 'l' ? D_BLOCK : d == 'c' ? D_COUNTER : d == 'n' ? D_NONE : D_AUTO;
    hub_mem (0, &HUB->pc_stance, &s, 1);
    note (s);
}
