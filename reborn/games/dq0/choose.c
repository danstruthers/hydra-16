/*
** choose.c - (overlay 7) the thing the player means, of those the character perceives (main.c's find: the parser's
** thing, and the way it lies if one was said: "the door on my right", "the north door"): the one there is; of
** several, the one directly ahead (the nearest that way); else (the user's, 2026-10-16: "If I say open door and there
** are three closed doors in the room, it should give me a numbered list of choices if one is not directly in front
** of me") they're listed, numbered, where they lie as it faces ("Which door: 1. the door ahead to your left (closed),
** 2. the one to your right (closed), or 3. the double door behind you to your right (open)?"), and the command kept:
** the next line chooses one by its number, or by the way it lies ("left"), and the command's carried out on it
** (choose; any other line, the choice let go).  From the hub's record of what the character perceives (copied into
** look.c's sight_copy: the hub isn't in the window while an overlay is), and its facing as the command was made
** (pc_face).  Its own state is kept in its overlay's bank (its data there: the code's only).
**
** And a blow with nothing named (the user's, 2026-10-22: "I don't think it should auto-fallback to something better
** unless you have nothing at hand, and then it should ask for confirmation"; HOUSE.md's fights and portals): what's
** in its hands strikes (of two, the referee's choice: wield's, overlay 14's, RS_WIELD), as a thing named does; its hands
** empty and a better thing carried (the referee's, by the reckoning a blow goes by), asked ("You can try
** battering it with your hands, but would you rather use your pick?"), the command kept: yes, with that thing (taken
** in hand as a thing named is, resolve.c's); no, or any other answer, bare-handed; a command instead, the blow let go.
*/

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")
#pragma data-name ("OV7RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#define NPICK       8

static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};

static const char* const thing[] = { "", "door", "passage", "stairway", "ladder", "shaft", "ledge", "way out", "wall",
    "window", "trap", "portcullis", "exit", "corner", "pillar" };   /* (OB_ROOM's: leaving it, way.c's) */

static struct cmd kept = { 0 };                 /* The blow asked about (wielded's) */
static unsigned px[NPICK] = { 0 }, py[NPICK] = { 0 };   /*   each one's square, */
static unsigned char ph[NPICK] = { 0 };         /*   the heading it lies, */
static unsigned char pi[NPICK] = { 0 };         /*   and its record (a door's, or ndoors on: an opening's) */
static unsigned char tasked = 0;                /* A blow's question put (kept the blow, its other thing the better
                                                **   one carried) */
unsigned char __fastcall__ o_answer_of (const char* l);    /* (This overlay's: task.c's) */

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

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

static const char* first;                      /* (The first choice's noun) */

/* How far choice k lies to the character's right, across the way it faces (less, its left); or (ahead) how far
** ahead of it */
static int __fastcall__ side (unsigned char k, unsigned char ahead)
{
    int dx = px[k] - sight_copy.x, dy = py[k] - sight_copy.y;

    return ahead ? head_x[pc_face] * dx + head_y[pc_face] * dy : head_x[pc_face] * dy - head_y[pc_face] * dx;
}

/* Choice k in words, added to lb: "the door ahead to your left (closed)" (and how far, if another lies the same way) */
static void __fastcall__ named (unsigned char k, unsigned char n, char* wd)
{
    static const char* const open[8] = {
        "stairway up", "stairway down", "ladder up", "ladder down", "window", "shaft overhead", "shaft in the floor",
        "stairs up to the way out"
    };
    struct sight* s = &sight_copy;
    unsigned char i = pi[k], r, j, st;
    const char* w;
    char b[6];

    add (k ? ", (" : " (");
    b[0] = '1' + k;
    b[1] = 0;
    add (b);
    add (") the ");
    if (i < s->ndoors) {
        r = s->door[i].rel;
        st = s->door[i].state;
        w = r & MOUTH ? "passage" : s->door[i].kind == PK_PORTCULLIS ? "portcullis" : s->door[i].kind == PK_WINDOW ?
            "window" : s->door[i].kind == PK_SLIT ? "arrow slit" : s->door[i].parts & SD_DOUBLE ? "double door" : "door";
    } else {
        r = s->open[i - s->ndoors].rel;
        w = s->open[i - s->ndoors].kind & 8 ? (s->open[i - s->ndoors].kind & 15) == O_SKYLIGHT ? "skylight" :
            (s->open[i - s->ndoors].kind & 15) == O_SKYSHAFT ? "shaft overhead" : open[s->open[i - s->ndoors].kind & 7] :
            open[s->open[i - s->ndoors].kind & 7];
    }
    strncpy (wd, w, 14);                        /* (Its words for which_in: the noun, its state) */
    wd[14] = 0;
    add (k && w == first ? "one" : w);           /* ("the door ..., the one ...") */
    first = k ? first : w;
    add (" ");
    add (r == 0xFF ? "where you stand" : relname[r & 7]);
    for (j = 0; j < n; ++j) {                   /* (Another the same way: how far each is; as far, which side) */
        if (j != k && ph[j] == ph[k]) {
            if (apart (px[j], py[j]) == apart (px[k], py[k])) {
                add ((r & 3) == 2 ? (side (k, 1) > side (j, 1) ? ", further ahead" : ", further back") :
                     side (k, 0) < side (j, 0) ? ", on the left" : ", on the right");
                break;
            }
            add (", ");
            add (utoa (SQ_FT * apart (px[k], py[k]), b, 10));
            add (" feet off");
            break;
        }
    }
    if (i < s->ndoors && !(r & MOUTH)) {
        strncat (wd, st == DOORWAY ? " doorway" : st & PS_WRECKED ? " broken" : st & PS_OPEN ? " open" : " closed", 14 - strlen (wd));
        add (st == DOORWAY ? " (a doorway)" : st & PS_WRECKED ? " (broken down)" : s->door[i].kind == PK_PORTCULLIS ?
             (st & PS_OPEN ? " (raised)" : " (down)") : st & PS_OPEN ? " (open)" : st & PS_BARRED_ ? " (closed, barred)" :
             st & PS_LOCKED_ ? " (closed, locked)" : " (closed)");
    }
}

/* Choice k's place in the list: from the character's left round to its right, then behind it (where it stands last) */
static unsigned char __fastcall__ order (unsigned char k)
{
    return ph[k] == 0xFF ? 8 : (ph[k] - pc_face + 2) & 7;
}

/* The n choices put in their order (a few: a simple sort) */
static void __fastcall__ sort (unsigned char n)
{
    unsigned char k, j, t;
    unsigned u;

    for (k = 1; k < n; ++k) {
        for (j = k; j && order (j) < order (j - 1); --j) {
            u = px[j]; px[j] = px[j - 1]; px[j - 1] = u;
            u = py[j]; py[j] = py[j - 1]; py[j - 1] = u;
            t = ph[j]; ph[j] = ph[j - 1]; ph[j - 1] = t;
            t = pi[j]; pi[j] = pi[j - 1]; pi[j - 1] = t;
        }
    }
}

/* Square dx, dy off not on the side of heading h */
static unsigned char toward (int dx, int dy, unsigned char h)
{
    return dx * head_x[h] + dy * head_y[h] <= 0;
}

/* Those it remembers (KNOWN_REL, after the n it sees of the m in the record, sz apart, the first's rel at p) made as
** those it sees, in the copy: their number and those it sees */
static unsigned char known (unsigned char* p, unsigned char n, unsigned char m, unsigned char sz)
{
    for (p += n * sz; n < m && (*p & KNOWN_REL); ++n, p += sz) {
        if (*p != 0xFF) {
            *p &= ~KNOWN_REL;
        }
    }
    return n;
}

unsigned char __fastcall__ o_pick (unsigned char obj, unsigned char orel)
{
    struct sight* s = &sight_copy;
    unsigned char i, k, h, r, n = 0, how = obj & (PICK_TELL | PICK_ANY), best = 0xFF, bd = 0xFF, fit = 0, bars = 0;
    unsigned x, y;
    char wd[16];                                /* (A choice's words: on the C stack, for which.c) */

    obj &= ~(PICK_TELL | PICK_ANY);
    hub_mem (SIGHT_BANK, s, SIGHT, sizeof *s);  /* (The senses' record as it is now; in the copy, those it remembers,
                                                **   KNOWN_REL, after those it sees: one rule for what can be named, the
                                                **   user's, 2026-11-02) */
    if (!(how & PICK_ANY)) {                    /* (PICK_ANY: is there one it sees) */
        s->ndoors = known (&s->door[0].rel, s->ndoors, SEE_DOORS, sizeof s->door[0]);
        s->nopen = known (&s->open[0].rel, s->nopen, SEE_OPEN, sizeof s->open[0]);
    }
again:
    for (i = n = 0; i < s->ndoors + s->nopen && n < NPICK; ++i) {
        if (i < s->ndoors) {
            r = s->door[i].rel;
            k = s->door[i].kind == PK_WINDOW || s->door[i].kind == PK_SLIT;    /* (A window, or a slit) */
            if (!(obj == OB_ROOM ? !k : obj == OB_DOOR ? !(r & MOUTH) && !k && (bars || s->door[i].kind != PK_PORTCULLIS) :
                obj == OB_WINDOW ? k : obj == OB_PORTC ?
                s->door[i].kind == PK_PORTCULLIS : obj == OB_PASSAGE && (r & MOUTH))) {
                continue;
            }
            x = s->door[i].x;
            y = s->door[i].y;
        } else {
            r = s->open[i - s->ndoors].rel;
            k = s->open[i - s->ndoors].kind;
            if (!((obj == OB_STAIRS && (k == O_STAIRS_UP || k == O_STAIRS_DOWN || k == O_WAY_OUT)) ||
                (obj == OB_LADDER && (k == O_LADDER_UP || k == O_LADDER_DOWN)) ||
                (obj == OB_SHAFT && ((k & 15) == O_SHAFT_UP || k == O_SHAFT_DOWN || (k & 15) >= O_SKYLIGHT)) ||
                (obj == OB_WAY_OUT && k == O_WAY_OUT))) {
                continue;
            }
            x = s->open[i - s->ndoors].x;
            y = s->open[i - s->ndoors].y;
        }
        h = r == 0xFF ? 0xFF : bearing ((int) x - s->x, (int) y - s->y);    /* (By its square: the facing now's) */
        if (orel != 0xFF && (fit ? r == 0xFF || toward (x - s->x, y - s->y, orel) : h != orel)) {
            continue;                           /* (Not that way: first exactly as the look's words have it; none, any
                                                **   on that side, "the door on the left" ahead of it to its left) */
        }
        if (pick_x && x == pick_x && y == pick_y) {   /* (The one chosen in answer to the question: which_in's) */
            best = n;
        }
        px[n] = x;
        py[n] = y;
        ph[n] = h;
        pi[n++] = i;
    }
    if (!n && !fit && orel <= H_NW) {
        fit = 1;
        goto again;
    }
    if (!n && !bars && obj == OB_DOOR) {        /* (A door: one that is, as the look's words have it; none, a portcullis) */
        bars = 1;
        fit = 0;
        goto again;
    }
    if (!n) {
        if (how & PICK_TELL) {
            strcpy (lb, thing[obj]);            /* (Its name in the main RAM: the refusals' overlay's to read) */
            say_no (orel == 0xFF ? A_UNSEEN : A_UNSEEN_WAY, lb);
        }
        return 0;
    }
    if (best == 0xFF && n > 1 && !(how & PICK_ANY)) {
        for (k = 0; k < n; ++k) {               /* (The nearest directly ahead) */
            if (ph[k] == pc_face && (h = apart (px[k], py[k])) < bd) {
                bd = h;
                best = k;
            }
        }
        if (best == 0xFF) {                     /* (None: which, asked, from its left round to its right, then
                                                **   behind it) */
            sort (n);
            strcpy (lb, "Which ");
            add (thing[obj]);
            add ("?");
            for (k = 0; k < n && k < 4; ++k) {   /* ("Which door? (1) the door ahead to your left (closed), (2) ...":
                                                **   asked as the things' are, which.c's, answered by which_in, 2026-11-07) */
                named (k, n, wd);
                which_put (k, wd, px[k], py[k], ph[k]);
            }
            which_place (k);
            return 2;
        }
    }
    best = best == 0xFF ? 0 : best;
    tx = px[best];
    ty = py[best];
    return 1;
}

/* A blow with nothing named (c a fresh command, or one made again: main.c's cmd, wield's): what's in its hands named,
** or asked about (the command kept, CF_ASKED: resolve's to leave it be) */
static void __fastcall__ wielded (struct cmd* c)
{
    unsigned char w;

    if (c->action == VB_ATTACK_OBJECT || c->action == VB_BREAK_OBJECT) {
        worded = 0x80;                          /* (Its target's first words held, its hands empty: blows.c's found) */
        w = wield ();
        worded &= 1;
        if (w & 2) {
            memcpy (&kept, c, sizeof kept);
            tasked = w | worded << 3;           /* (| 4: a light in hand, "no" its fist; | 8: its first words said) */
            c->flags |= CF_ASKED;
        }
    }
}

/* A command (main.c's, the parser's, before it's carried out): a blow's question answered (cl's line: yes, no), the
** blow kept made again; any other command lets it go.  (A door's or an opening's question is which.c's, answered by
** letters.c's which_in, the line made again, its square chosen in pick_x and pick_y: 2026-11-07) */
unsigned char __fastcall__ o_choose (struct cmd* c)
{
    unsigned char j;

    strike = SK_BEST;                           /* (What a blow strikes with: the verb's, or the thing named, resolve.c's,
                                                **   or what's in its hands, wield's) */
    shand = 0;                                  /* (No hand said yet: main.c's hand_said; nothing to take in hand
                                                **   for a blow, resolve.c's) */
    hold_w[0] = 0;
    if (tasked) {
        if ((j = o_answer_of (cl)) != 0) {
            memcpy (c, &kept, sizeof kept);
            worded = tasked >> 3;               /* (A blow's first words said as it was asked, fight_target's: not
                                                **   again; held as it was, its hands empty, said now, 2026-11-17) */
            if (j == 1) {
                c->prep = PR_WITH;              /* (Yes: with the better thing, taken in hand) */
            } else {
                c->other.word[0] = 0;           /* (No: bare-handed; a light in hand, its fist: s_wield's) */
                if (tasked & 4) {
                    strike = SK_PUNCH;
                }
            }
            tasked = 0;
            return 1;
        }
        tasked = 0;
    }
    wielded (c);
    return 0;
}

#pragma code-name (push, "OV14CODE")
#pragma rodata-name (push, "OV14RODATA")
#pragma local-strings (push, on)

/* (Overlay 14's, through its stub wield: part aliases's till speed13, 2026-11-23, whose copy over part commands and part commands's back
** were 91,000 cycles of an attack's first letter) A blow with nothing named (the command, cmd:
** VB_ATTACK_OBJECT's at a denizen there to strike, but a kick's, a punch's, a head butt's; VB_BREAK_OBJECT's at a door,
** a window, a portcullis): what's in its hands strikes (the things' IO_HANDS; of two, the referee's choice,
** RS_WIELD's), strike that thing: 1; its hands empty, a better thing carried (RS_WIELD's key, its name the
** things' IO_WEAR's, worn by nothing), its name its other thing and the question put: 2 (dq0 -a's: yes, unasked, with
** it: 1); else 0, its bare hands.  At a door, a window, a portcullis a light in hand never
** strikes (the user's, 2026-11-17: "nothing is battered with a light"): its hand as if empty, the other's thing, or the
** better thing asked about (| 4: "no" its fist), or its bare hands, SK_PUNCH (its fist, the light kept in the other).
** The hub read and written by hub_mem (the window's this overlay's), the things and the referee asked by wield_ask
** (part commands's: o_choose's caller's, s_command's, there already) */
#define USLOT           (HUB->slot[SLOT_UI])
unsigned char __fastcall__ wield_ask (unsigned char op, unsigned char b);
unsigned char o_wield (void)
{
    unsigned char a = cmd.action, t, k, lit = 0;
    unsigned char x[2];

    if ((a == VB_ATTACK_OBJECT && cmd.aux != SK_BEST) || cmd.prep == PR_WITH || (a == VB_BREAK_OBJECT &&
        !cmd.obj.kind && cmd.obj.word[0])) {
        return 0;                               /* (A part of it said; a thing named; a thing broken, not a portal) */
    }
    k = cmd.obj.kind;
    t = a == VB_BREAK_OBJECT || k == OB_DOOR || k == OB_WINDOW || k == OB_PORTC;
    if ((!t && !(a = fight_target (&cmd))) || (t && k && !pick (k | PICK_ANY, cmd.obj.lies))) {
        return 0;                               /* (No denizen there to strike, nor a door, a window, a portcullis
                                                **   seen: why not told as it's carried out, unasked, slice 4.1b's) */
    }
    wield_ask (IO_HANDS, 0);
    if (t) {
        hub_mem (0, &lit, &HUB->ireq.mat, 1);   /* (A door, a window, a portcullis: never battered with a light, the
                                                **   user's, 2026-11-17; a light in hand as if the hand were empty: its
                                                **   bits the world's) */
    }
    hub_mem (0, &x[0], &HUB->ireq.x, 1);        /* (Their low bytes: the kinds) */
    hub_mem (0, &x[1], &HUB->ireq.y, 1);
    if (lit & 1) {
        x[0] = 0;
    }
    if (lit & 2) {
        x[1] = 0;
    }
    hub_mem (0, USLOT.x, x, 2);
    if (lit) {
        lit = 4;
    }
    wield_ask (0, t);                           /* (RS_WIELD) */
    hub_mem (0, &k, &USLOT.result, 1);
    if (k != 0) {                               /* (What's in its hands strikes: held already, nothing to take) */
        hub_mem (0, &k, &USLOT.x[k - 1], 1);
        strike = SK_OF (k);
        k = 1;
    } else if (hub_mem (0, &k, USLOT.text, 1), k) {  /* (Its hands empty, a better thing carried: asked about) */
        wield_ask (IO_WEAR, 0);
        hub_mem (0, cmd.other.word, HUB->ireq.name, sizeof cmd.other.word);
        k = 2 | lit;                            /* (A light in hand: no, its fist, o_choose's) */
        if (autogo) {
            cmd.prep = PR_WITH;
            k = 1;
        } else {
            if (!t) {
                hub_mem (0, &lit, &HUB->folk[((a & 0x0F) - 1) & 7], 1);
            }
            sayf ("\nYou can %s, but would you rather %s your %s?", t ? "try battering it with your hands" :
                lit ? "fight them with your fists" : "fight it with your fists",
                t ? "use" : "draw", cmd.other.word);    /* (A person "them": the hub's folk, 2026-11-17) */
        }
    } else if (lit) {
        strike = SK_PUNCH;                      /* (A light in hand, nothing better: its bare hands, as with none, the
                                                **   light kept in the other; SK_BEST the referee's would take the torch) */
    }
    return k;
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW_commands_CODE")
#pragma rodata-name (push, "SW_commands_RODATA")
#pragma local-strings (push, on)

/* (Part commands's, o_wield's: the commands' part, there as a command's carried out) The things asked from o_wield's overlay,
** what's in its hands (IO_HANDS) or the tool with the referee's key worn by nothing (IO_WEAR, its key the slot's text:
** its name back in the request's), or (op 0) the referee's choice (RS_WIELD, b its a door's, a window's, a
** portcullis's), its reply let go; the hub's header in the window for it, then the window put back */
unsigned char __fastcall__ s_wield_ask (unsigned char op, unsigned char b)
{
    unsigned w = hub_here ();

    hub_map (0);
    if (op == IO_WEAR) {
        HUB->ireq.to = 0;
        b = things (IO_WEAR, USLOT.text);
    } else if (op) {
        b = things (op, "");
    } else {
        ask_b (RS_WIELD, 0, b)->state = REQ_FREE;
    }
    hub_back (w);
    return b;
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
