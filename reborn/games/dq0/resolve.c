/*
** resolve.c - (overlay 11) the things a command names found among those the character perceives, and the thing its
** verb takes when none is said (the plan's command interpreter: defaults, it, selectors, the target, questions only
** when it's unclear).  main.c calls it with the parser's command (cmd: hub.h's struct cmd) before it's carried out:
**   - an answer to its question ("Which do you mean: 1. the sword ahead, or 2. the shield to your left?"): a number,
**     a way ("left"), a selector ("the closest"), a name ("shield"): the command asked about made again, on that one
**     (the user's answer 13); any other line lets the question go;
**   - "it" ("take it", "them"): the last thing named, of any kind (the user's answer 7);
**   - a thing unsaid, its verb's (commands.verbs' unsaid): the last thing named (drop, put ...); that, else the things
**     near (take: one taken, several asked which, none said so: answer 8); the target (attack, throw);
**   - which of several of those it sees: the closest (the lead: the closest), the largest, the smallest, the second
**     (as they lie from its left round to its right), any;
**   - the target: set ("target the rat", or any "at the ...": "throw the apple at the rat"), shown, cleared ("clear
**     target"; as it's down while others fight on; as it's dropped or thrown away; as it's out of sight; as the world's
**     events make the character turn: a denizen come into sight, a blow from one it doesn't see, a trap set off,
**     main.c's untarget);
**   - "take all": each thing it sees lying about, taken in turn (taken_next);
**   - a thing's aspect asked, its thing unsaid ("check its condition", "examine the lock"): the last thing named;
**   - what it's done with ("with the bar", "using the pick": the instrument, the user's, 2026-10-22): to hand
**     (carried, or lying within reach: the things' IO_FIND, as a fill's fuel and a door's bar are), else refused,
**     the things' words ("You have no bar to hand."), never done with bare hands or another thing instead; a blow's
**     taken in hand first, as a named weapon is (names.c's), and struck with that thing and no other (strike's
**     SK_THING, its kind: the user's, 2026-10-22, HOUSE.md's fights and portals);
**   - a blow with nothing named, asked about first, its hands empty and a better thing carried (choose.c's: CF_ASKED,
**     the command carried out as it's answered).
** A thing found has its square (struct cthing's x, y) and its name as its word.  Its own state (the last thing named,
** the target, the question, the things to take) is kept in its overlay's bank (its data there: the code's only).
** The fixtures (doors, windows, stairs ...) are choose.c's to find and to ask about (overlay 7: pick, choose).
*/

#pragma code-name ("OV11CODE")
#pragma rodata-name ("OV11RODATA")
#pragma data-name ("OV11RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#define NASK        8                           /* (The choices a question lists, the things "take all" takes) */
#define NAMELEN     14

static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};

static struct cthing last = { 0 };              /* The last thing named ("it") */
static struct cthing target = { 0 };            /* The target (its word "": none) */
static struct cmd kept = { 0 };                 /* The command a question was asked about, */
static unsigned char asked = 0;                 /*   its choices (0: none asked), */
static unsigned char role = 0;                  /*   the thing it asked about (0 the command's, 1 the other), */
static char cn[NASK][NAMELEN] = { { 0 } };      /* The things found: each one's name, */
static unsigned cx[NASK] = { 0 }, cy[NASK] = { 0 };   /*   its square, */
static unsigned char ch[NASK] = { 0 };          /*   the heading it lies (0xFF: where the character stands), */
static unsigned char cs[NASK] = { 0 };          /*   its size (an item's; a denizen's 5), */
static unsigned char cd[NASK] = { 0 };          /*   and whether it's a denizen (1) and down (2) */
static unsigned char nc = 0;
static char all[NASK][NAMELEN] = { { 0 } };     /* "take all": the things still to take up, */
static unsigned char nall = 0, iall = 0;        /*   how many, the next */
static char dens[SEE_DENS][12] = { { 0 } };     /* The denizens seen at the last command (a new one: the target let
                                                **   go) */
static unsigned char ndens = 0;

/* Squares from the character to square (x, y) (the more of the two ways) */
static unsigned char __fastcall__ apart (unsigned x, unsigned y)
{
    x -= sight_copy.x;
    y -= sight_copy.y;
    x = (int) x < 0 ? -x : x;
    y = (int) y < 0 ? -y : y;
    x = x > y ? x : y;
    return x > 255 ? 255 : x;
}

/* Does name have word w as one of its words ("giant rat": "rat"; "iron key": "key", "keys"; "bones": "bone")? */
static unsigned char __fastcall__ has (const char* name, const char* w)
{
    unsigned char n = strlen (w), m;
    const char* p = name;

    if (!n) {
        return 1;
    }
    m = n > 1 && w[n - 1] == 's' ? n - 1 : n;  /* (The word a plural: its name's word without the s) */
    while (*p) {
        if ((!strncmp (p, w, n) && (p[n] == 0 || p[n] == ' ' || (p[n] == 's' && p[n + 1] == 0))) ||
            (m < n && !strncmp (p, w, m) && (p[m] == 0 || p[m] == ' '))) {
            return 1;
        }
        while (*p && *p != ' ') {
            ++p;
        }
        while (*p == ' ') {
            ++p;
        }
    }
    return 0;
}

/* One found: its name, square, size, whether it's a denizen */
static void __fastcall__ add (const char* name, unsigned x, unsigned y, unsigned char size, unsigned char den)
{
    if (nc < NASK) {
        strncpy (cn[nc], name, NAMELEN - 1);
        cn[nc][NAMELEN - 1] = 0;
        cx[nc] = x;
        cy[nc] = y;
        ch[nc] = x == sight_copy.x && y == sight_copy.y ? 0xFF :
            bearing ((int) x - sight_copy.x, (int) y - sight_copy.y);
        cs[nc] = size;
        cd[nc++] = den;
    }
}

/* The things it sees that the thing said might be (its word in their names: none said, the things lying about):
** items (what = 1), denizens (2), or both (3).  How many */
static unsigned char __fastcall__ gather (const char* w, unsigned char what)
{
    struct sight* s = &sight_copy;
    unsigned char i;

    nc = 0;
    if (what & 1) {
        for (i = 0; i < s->nitems; ++i) {
            if (!(s->item[i].hid & 1) && has (s->item[i].name, w)) {    /* (Not one it hid: it knows where those are,
                                                **   but they're out of sight) */
                add (s->item[i].name, s->item[i].x, s->item[i].y, s->item[i].hid >> 4, 0);
            }
        }
    }
    if (what & 2) {
        for (i = 0; i < s->ndens; ++i) {
            if (*w ? has (s->den[i].name, w) : s->den[i].doing != DO_DOWN) {
                add (s->den[i].name, s->den[i].x, s->den[i].y, 5, s->den[i].doing == DO_DOWN ? 3 : 1);
            }
        }
    }
    return nc;
}

/* A choice's place in a list: from the character's left round to its right, then where it stands */
static unsigned char __fastcall__ order (unsigned char k)
{
    return ch[k] == 0xFF ? 8 : (ch[k] - pc_face + 2) & 7;
}

/* The one of those found that the selector picks (SL_*, n the nth's): its number, or 0xFF (none: several, and the
** selector doesn't choose) */
static unsigned char __fastcall__ choose_one (unsigned char sel, unsigned char n)
{
    unsigned char k, best = 0xFF, b = 0, d, j;

    if (nc == 1) {
        return 0;
    }
    for (k = 0; k < nc; ++k) {
        d = sel == SL_LARGEST ? 255 - cs[k] : sel == SL_SMALLEST ? cs[k] : apart (cx[k], cy[k]);
        if (sel == SL_NTH) {                    /* (The nth from its left round to its right) */
            for (d = j = 0; j < nc; ++j) {
                d += order (j) < order (k) || (order (j) == order (k) && j < k);
            }
            if (d == n - 1) {
                return k;
            }
        } else if (best == 0xFF || d < b) {
            best = k;
            b = d;
        }
    }
    return sel == SL_THE || sel == SL_NTH || sel == SL_ALL || sel == SL_IT || sel == SL_ME ? 0xFF : best;
}

/* Thing t made the one found, k: its name its word, its square */
static void __fastcall__ found (struct cthing* t, unsigned char k)
{
    strcpy (t->word, cn[k]);
    t->x = cx[k];
    t->y = cy[k];
    t->sel = SL_THE;
}

/* Is thing t (a denizen's, or a thing's found) still among those it sees?  Its entry found's (k), or 0xFF */
static unsigned char __fastcall__ seen (const struct cthing* t)
{
    unsigned char k;

    if (!t->word[0] || !gather (t->word, 3)) {
        return 0xFF;
    }
    for (k = 0; k < nc; ++k) {
        if (!t->x || (cx[k] == t->x && cy[k] == t->y)) {
            return k;
        }
    }
    return nc == 1 ? 0 : 0xFF;                  /* (One of its name, moved: that one) */
}

/* How far choice k lies to the character's right, across the way it faces (less: its left) */
static int __fastcall__ side (unsigned char k)
{
    return head_x[pc_face] * ((int) cy[k] - sight_copy.y) - head_y[pc_face] * ((int) cx[k] - sight_copy.x);
}

/* Entry j and the one before it, of a list of words or of bytes, swapped */
static void __fastcall__ swap (unsigned* p)
{
    unsigned u = p[0];

    p[0] = p[-1];
    p[-1] = u;
}
static void __fastcall__ swapc (unsigned char* p)
{
    unsigned char u = p[0];

    p[0] = p[-1];
    p[-1] = u;
}

/* The question, which of those found: "Which do you mean: 1. the sword ahead, or 2. the sword to your left?", the
** command kept, the thing it's about its role */
static void __fastcall__ ask_which (unsigned char r)
{
    unsigned char k, j;
    char b[NAMELEN];

    for (k = 1; k < nc; ++k) {                  /* (In their order: a few, a simple sort, the ties as they were) */
        for (j = k; j && order (j) < order (j - 1); --j) {
            memcpy (b, cn[j], NAMELEN);
            memcpy (cn[j], cn[j - 1], NAMELEN);
            memcpy (cn[j - 1], b, NAMELEN);
            swap (cx + j);
            swap (cy + j);
            swapc (ch + j);
            swapc (cs + j);
            swapc (cd + j);
        }
    }
    strcpy (lb, "Which do you mean:");
    for (k = 0; k < nc; ++k) {
        strcat (lb, k ? (k + 1 == nc ? ", or " : ", ") : " ");
        b[0] = '1' + k;
        b[1] = 0;
        strcat (lb, b);
        strcat (lb, ". the ");
        strcat (lb, cn[k]);
        strcat (lb, " ");
        strcat (lb, ch[k] == 0xFF ? "where you stand" : relname[(ch[k] - pc_face) & 7]);
        for (j = 0; j < nc; ++j) {              /* (Another the same way: how far off each is; as far, which side) */
            if (j != k && ch[j] == ch[k] && ch[k] != 0xFF) {
                if (apart (cx[j], cy[j]) == apart (cx[k], cy[k])) {
                    strcat (lb, side (k) < side (j) ? ", on the left" : ", on the right");
                } else {
                    strcat (lb, ", ");
                    strcat (lb, utoa (5 * apart (cx[k], cy[k]), b, 10));
                    strcat (lb, " feet off");
                }
                break;
            }
        }
    }
    strcat (lb, "?");
    msg (lb);
    memcpy (&kept, &cmd, sizeof kept);
    asked = nc;
    role = r;
}

/* The line an answer to the question?  Then cmd the command asked about, on the one chosen: 1; else 0 (the question
** let go) */
static unsigned char answered (void)
{
    unsigned char n = asked, k = 0xFF, j, d, bd = 0xFF;

    asked = 0;
    if (!n) {
        return 0;
    }
    nc = n;
    if (!(cmd.flags & CF_NOVERB)) {
        return 0;                               /* (A command of its own: "attack the closest rat") */
    }
    if (cmd.obj.sel == SL_CLOSEST || cmd.obj.sel == SL_LARGEST || cmd.obj.sel == SL_SMALLEST || cmd.obj.sel == SL_NTH ||
        cmd.obj.sel == SL_LEAD) {
        k = choose_one (cmd.obj.sel, cmd.obj.n);    /* ("the closest", "the second") */
    } else if (cmd.n && cmd.n <= n && !cmd.obj.word[0]) {
        k = cmd.n - 1;                          /* ("2") */
    } else if (cmd.obj.word[0]) {
        for (j = 0; j < n && k == 0xFF; ++j) {  /* ("the shield") */
            if (has (cn[j], cmd.obj.word)) {
                k = j;
            }
        }
    } else if (cmd.dir <= H_NW) {
        for (j = 0; j < n; ++j) {               /* ("left": the nearest that way) */
            if (ch[j] == cmd.dir && (d = apart (cx[j], cy[j])) < bd) {
                bd = d;
                k = j;
            }
        }
    }
    if (k == 0xFF) {
        return 0;
    }
    memcpy (&cmd, &kept, sizeof cmd);
    found (role ? &cmd.other : &cmd.obj, k);
    cmd.flags &= ~CF_UNSAID;                    /* (Its thing said now: the one chosen) */
    return 1;
}

/* Thing t (the command's, or its other) found among those it sees, by its word and its selector (what: items 1,
** denizens 2, both 3): 1 found (or not one it sees, or several of the name and none chosen: the command's to see
** to, as ever); 0 a question asked (ask: where it must be one, the target) */
static unsigned char __fastcall__ find_thing (struct cthing* t, unsigned char what, unsigned char r, unsigned char ask)
{
    unsigned char k;

    if (t->kind || t->x || t->sel == SL_ME || t->sel == SL_IT || (!t->word[0] && t->sel == SL_THE) ||
        !gather (t->word, what)) {
        return 1;                               /* (A fixture, found already, the character, nothing, none it sees) */
    }
    for (k = 0; k < nc && !cd[k]; ++k) {
    }
    if (k == nc && (t->sel == SL_NTH || t->sel >= SL_OTHER)) {
        return 1;                               /* (Things alone, the nth, the other, by their state: the world's to
                                                **   choose, the carried among them, slice 4.1's) */
    }
    if ((k = choose_one (t->sel, t->n)) != 0xFF) {
        found (t, k);
        return 1;
    }
    if (!ask || t->sel != SL_THE) {
        return 1;
    }
    ask_which (r);
    return 0;
}

/* The actions done with a thing ("with the bar"): what it's done with must be to hand */
static const unsigned char tooled[] = { VB_ATTACK_OBJECT, VB_BREAK_OBJECT, VB_FORCE_OBJECT, VB_BEND_OBJECT,
    VB_RAISE_OBJECT, VB_LOWER_OBJECT, VB_DISARM_OBJECT, VB_DIG, VB_LIGHT_OBJECT, VB_EXTINGUISH_OBJECT, VB_OPEN_OBJECT,
    VB_CLOSE_OBJECT, VB_UNLOCK_OBJECT, VB_LOCK_OBJECT, VB_FILL_OBJECT, VB_KNOCK_OBJECT, VB_PUSH_OBJECT, VB_PULL_OBJECT,
    VB_TOUCH_OBJECT, VB_BAR_OBJECT, VB_CLIMB, VB_SEARCH, VB_EXAMINE_OBJECT, VB_JUMP };

/* What the command's done with (its "with": the other thing's word, or, nothing else said, the thing's), to hand?
** 1, it the other thing now (the thing unsaid, if that was it), a blow's to take in hand first, once its target's
** found (main.c's hold_w: the hold, its words, the referee told, its time, as the blow's sent: slice 4.1b's); 0,
** refused (the things' words) */
static unsigned char tool_ok (void)
{
    char* w = cmd.other.word[0] ? cmd.other.word : cmd.obj.word;
    unsigned char k, a = cmd.action;

    for (k = 0; k < sizeof tooled && tooled[k] != a; ++k) {
    }
    if (k == sizeof tooled || !*w || cmd.obj.kind == OB_WAY_OUT) {
        return 1;                               /* (Not done with a thing; the way out's own, the puzzle's) */
    }
    k = w == cmd.obj.word ? cmd.obj.sel | cmd.obj.n << 4 : cmd.other.sel | cmd.other.n << 4;
    hub_mem (0, &HUB->ireq.sel, &k, 1);         /* (Which of several: the world's to choose) */
    if (othings (IO_FIND, w, "", 0) != IR_OK) {
        msg (lb);                               /* ("You have no bar to hand.", "The bar is out of reach: ...") */
        return 0;
    }
    if (w == cmd.obj.word) {                    /* ("Attack with the sword": what it's done with, the thing unsaid) */
        strcpy (cmd.other.word, w);
        *w = 0;
        if (!cmd.obj.kind && !cmd.obj.sel) {
            cmd.flags |= CF_UNSAID;
        }
    }
    if (a == VB_ATTACK_OBJECT || a == VB_BREAK_OBJECT) {
        strike = (unsigned char) othings_x < KI_HIGH ? SK_THING | (unsigned char) othings_x : SK_BEST;  /* (That thing
                                                **   strikes, and no other: its kind, IO_FIND's (one past seven bits,
                                                **   items.h's KI_HIGH, the best held); in
                                                **   hand first, once there's something to strike) */
        strcpy (hold_w, cmd.other.word);
    }
    if (a != VB_FILL_OBJECT) {
        cmd.other.word[0] = 0;                  /* (Checked: the action's own rules use what's to hand; a fill's fuel
                                                **   its own, named) */
    }
    return 1;
}

/* The target, if it's still one (let go as it's out of sight, or down while others are up): its entry among those
** found plus 1 (its square now the target's), or 0 */
static unsigned char has_target (void)
{
    struct sight* s = &sight_copy;
    unsigned char k, i, up = 0;

    if (!target.word[0]) {
        return 0;
    }
    for (i = 0; i < s->ndens; ++i) {
        up += s->den[i].doing != DO_DOWN;
    }
    if ((k = seen (&target)) == 0xFF || ((cd[k] & 2) && up)) {
        target.word[0] = 0;                     /* (Gone from sight; down, and others still up) */
        return 0;
    }
    target.x = cx[k];                           /* (Where it is now) */
    target.y = cy[k];
    return k + 1;
}

/* Overlay 11: the line (cmd) an answer to the question asked (chosen: choose.c's, a fixture's, already answered: the
** question let go)?  Then cmd the command asked about, on the one chosen: 1; else 0 */
unsigned char __fastcall__ o_answer (unsigned char chosen)
{
    if (chosen || !asked) {
        asked = 0;
        return 0;
    }
    hub_mem (SIGHT_BANK, &sight_copy, SIGHT, sizeof sight_copy);   /* (Only if one was asked: the copy's cost) */
    return answered ();
}

/* Overlay 11: the command (cmd) resolved: it, its unsaid thing, which of several, the target.  RV_ON: carried on
** with; RV_DONE: seen to (a question asked, the target shown or set, said why not) */
unsigned char o_resolve (void)
{
    struct sight* s = &sight_copy;
    unsigned char k, n, a = cmd.action;

    if (cmd.flags & CF_ASKED) {
        return RV_DONE;                         /* (A blow's question put: choose.c's) */
    }
    hub_mem (0, &k, &HUB->pcsum.cond, 1);       /* (C_STUNNED its low byte's) */
    if (((k & C_STUNNED) || a == VB_TEND) && (k = early ()) != 0) {
        return k - 1;                           /* (In shock, refused; first aid its own: part 12's, slice 4.3's) */
    }
    hub_mem (SIGHT_BANK, s, SIGHT, sizeof *s);  /* (The senses' record as it is now) */
    if (untarget) {                             /* (The world's events: the character turned) */
        untarget = 0;
        target.word[0] = 0;
    }
    for (k = 0; k < s->ndens; ++k) {            /* (A denizen come into sight since: the target let go) */
        for (n = 0; n < ndens && strcmp (dens[n], s->den[k].name); ++n) {
        }
        if (n == ndens) {
            target.word[0] = 0;
        }
    }
    for (ndens = 0; ndens < s->ndens; ++ndens) {
        strcpy (dens[ndens], s->den[ndens].name);
    }
    if (cmd.obj.sel == SL_IT && !cmd.obj.word[0] && !cmd.obj.kind) {   /* ("it": the last thing named) */
        if (!last.word[0] && !last.kind) {
            msg ("You haven't named anything yet.");
            return RV_DONE;
        }
        memcpy (&cmd.obj, &last, sizeof last);
    }
    if (cmd.other.sel == SL_IT && !cmd.other.word[0]) {
        memcpy (&cmd.other, &last, sizeof last);
    }
    if (cmd.prep == PR_WITH && !tool_ok ()) {   /* (What it's done with: to hand, else refused) */
        return RV_DONE;
    }
    if (cmd.aux && (a == VB_EXAMINE_LOCALE || (a == VB_EXAMINE_OBJECT && (cmd.flags & CF_UNSAID))) && !cmd.obj.kind &&
        last.word[0] && !last.kind) {           /* (An aspect asked, nothing named: "check its condition", the last
                                                **   thing named) */
        memcpy (&cmd.obj, &last, sizeof last);
        cmd.action = a = VB_EXAMINE_OBJECT;
        cmd.flags &= ~CF_UNSAID;
    }
    switch (a) {
    case VB_TARGET_SHOW:
        if ((k = has_target ()) != 0) {
            sayf ("\nYour target: the %s %s.", target.word, ch[k - 1] == 0xFF ? "where you stand" :
                relname[(ch[k - 1] - pc_face) & 7]);
        } else {
            msg ("You have no target.");
        }
        return RV_DONE;
    case VB_TARGET_CLEAR:
        target.word[0] = 0;
        msg ("You have no target now.");
        return RV_DONE;
    case VB_TARGET_SET:
        if (!find_thing (&cmd.obj, 3, 0, 1)) {
            return RV_DONE;
        }
        if (seen (&cmd.obj) == 0xFF) {
            sayf ("\nYou see no %s here.", cmd.obj.word);
            return RV_DONE;
        }
        memcpy (&target, &cmd.obj, sizeof target);
        sayf ("\nYour target: the %s.", cmd.obj.word);
        goto named;
    }
    nall = 0;
    if (cmd.flags & CF_UNSAID) {                /* (Its thing unsaid: its verb's) */
        switch (cmd.dflt) {
        case DF_LAST:
            if (last.word[0] && !last.kind) {
                memcpy (&cmd.obj, &last, sizeof last);
            }
            break;
        case DF_NEAR:
            if (last.word[0] && !last.kind && gather (last.word, 1)) {
                memcpy (&cmd.obj, &last, sizeof last);    /* ("examine book", then "take": the book) */
            } else if (!gather ("", 1)) {
                msg ("There's nothing here to take.");
                return RV_DONE;
            } else if (nc == 1) {
                found (&cmd.obj, 0);
            } else {
                ask_which (0);
                return RV_DONE;
            }
            break;
        case DF_TARGET:
            if (has_target ()) {
                memcpy (&cmd.obj, &target, sizeof target);
            }
            break;
        }
    } else {
        if (a == VB_RETRIEVE_OBJECT && cmd.obj.sel == SL_ALL && !cmd.obj.word[0]) {
            if (!gather ("", 1)) {              /* ("take all": everything it sees lying about, in turn) */
                msg ("There's nothing here to take.");
                return RV_DONE;
            }
            for (nall = 0; nall < nc; ++nall) { /* (The nearest first: those in reach before those walked to) */
                for (k = n = 0; k < nc; ++k) {
                    if (apart (cx[k], cy[k]) < apart (cx[n], cy[n])) {
                        n = k;
                    }
                }
                strcpy (all[nall], cn[n]);
                cx[n] = cy[n] = 0xFFFF;
            }
            iall = 1;
            strcpy (cmd.obj.word, all[0]);
            cmd.obj.sel = SL_THE;               /* (Each by its name in turn: not all of a name at once) */
        } else if (!find_thing (&cmd.obj, a == VB_ATTACK_OBJECT ? 2 : a == VB_THROW_OBJECT ? 1 : 3, 0, 0)) {
            return RV_DONE;
        }
    }
    if (cmd.other.word[0] || cmd.other.sel) {   /* (The other: in it, from it, at it ...) */
        if (!find_thing (&cmd.other, 3, 1, a == VB_THROW_OBJECT)) {
            return RV_DONE;
        }
        if (cmd.prep == PR_AT && seen (&cmd.other) != 0xFF) {
            memcpy (&target, &cmd.other, sizeof target);   /* ("at the rat": the target, till it's cleared) */
        }
    }
    if (a == VB_THROW_OBJECT && !cmd.other.word[0] && has_target ()) {
        memcpy (&cmd.other, &target, sizeof target);    /* (Thrown at the target, none said) */
        cmd.prep = PR_AT;
    }
    if ((a == VB_DROP_OBJECT || a == VB_THROW_OBJECT) && target.word[0] && !strcmp (cmd.obj.word, target.word)) {
        target.word[0] = 0;                     /* (The target dropped, or thrown away) */
    }
    if (cmd.flags & CF_UNSAID) {
        return RV_ON;
    }
named:
    if (cmd.obj.word[0] || cmd.obj.kind) {
        memcpy (&last, &cmd.obj, sizeof last);  /* (The last thing named: "it") */
    }
    return a == VB_TARGET_SET ? RV_DONE : RV_ON;
}

/* Overlay 11: "take all"'s next thing to take up, into w ("": none) */
void __fastcall__ o_taken_next (char* w)
{
    *w = 0;
    if (iall < nall) {
        strcpy (w, all[iall++]);
    }
}

#pragma code-name (push, "OV10CODE")           /* (Overlay 10's: this one's room) */
#pragma rodata-name (push, "OV10RODATA")
#pragma data-name (push, "OV10RODATA")
#pragma local-strings (push, on)

/* Overlay 10: a throw told (the things' IO_THROW's result r, how it went, TH_*, times 256): "You throw the dagger hard
** at the giant rat: it strikes it." */
void __fastcall__ o_throw_words (unsigned r)
{
    static const char* const how[3] = { ": it strikes it.", ", and miss: it lands beside it.",
        ", but it's too far: it falls short." };
    const char* w = cmd.obj.word[0] ? cmd.obj.word : "it";

    switch ((unsigned char) r) {
    case IR_OK:
        sayf ("\nYou throw the %s%s at the %s%s", w, cmd.manner & MN_HARD ? " hard" : "", cmd.other.word,
            how[r >> 8 & 3]);
        return;
    case IR_NO:
        sayf ("\nThe %s isn't a thing you can throw.", w);
        return;
    case IR_NOT_HERE:
        sayf ("\nYou see no %s here.", cmd.other.word);
        return;
    case IR_FULL:
        msg ("Throw it at what? (throw the dagger at the rat; or name a target: target the rat)");
        return;
    default:
        msg (cmd.obj.word[0] ? "You carry none of those." : "You have nothing in your hands to throw.");
    }
}

#pragma local-strings (pop)
#pragma data-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)
