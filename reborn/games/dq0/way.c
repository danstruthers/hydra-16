/*
** way.c - Dingus Quest Zero's looking one way, and listening and sniffing one way (HOUSE.md's slice 4.0: "look
** north", "look left", "look at the door", "look through the window"; "listen at the door", "sniff at the grating"):
** the command carried out (part look_way), and a look one way's words (overlay 11).  The way is the one said, or the one a
** door, a window, a portcullis, a passage, the stairs the character sees lies (as it faces, never the compass); it
** turns to face it first (the turn's look), then looks, listens or sniffs, 3 seconds:
**
**   a look one way    the world's I_LOOK: the senses' look about, and twice as far in the quarter ahead (as far as
**                     the light and the eyes allow), and what lies beyond the openings that way (their struct
**                     wayseen); its words what it sees that way: through a doorway, an open door, a window, down a
**                     passage (a room there, how far), what's shut, the things, the denizens and the lights far off
**                     ahead (the place's own words not told again: a turn's look has them, and lbsaid.s says no
**                     sentence twice in an answer)
**   a listen          main.c's heard: what's heard that way twice as far (through a shut door muffled, its squares
**                     counted, the world's), as the time goes; "listen" alone, about it, as far as ever; nothing
**                     heard, told as it ends
**   a sniff           the world's (I_TRACE, slice 4.1's traces: the smell carried from that way, or about it), told
**                     at once (words.c's), the time after
**
** A thing looked at ("look at the table") is what anyone sees of it, no test: what it is, where, its state, what's in
** it or on it (the world's IO_LOOK, LM_AT: the user's, 2026-10-22); examined ("examine the sword"), a closer look, no
** test either (LM_CLOSE: what it's made of, its wear, a lock's make); the aspects asked for (the command's aux, EX_*:
** "assess the sword", "judge the quality of the chest", "appraise it", "heft the sack", "examine the lock of the
** chest", "check its condition", "check the chest for traps"), each its own: an appraisal the referee's test of the
** craft that knows it (value, quality, both), the rest no test but a door gone over for traps (the world's PA_CHECK,
** main.c's door work).  A denizen ("look at the rat") a look about, as before; a portal examined, its plain features,
** judged only if its quality's asked.
*/

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"
#include "tell.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's, an overlay's) */

#define WAY_T       30                          /* (A look one way's time, a listen's, a sniff's: tenths) */
#define WAY_OBJ     0x0ABE                      /* (The things looked at that are a way: a bit each, OB_DOOR,
                                                **   OB_PASSAGE, OB_STAIRS, OB_LADDER, OB_SHAFT, OB_WAY_OUT,
                                                **   OB_WINDOW, OB_PORTC: pick's) */

/* ==== Part look_way: the command (main.c's chores: the parser's VB_OBSERVE_DIRECTION, VB_OBSERVE_OBJECT,
** VB_OBSERVE_THROUGH, VB_LISTEN, VB_SMELL; and VB_TRACK, the traces read, slice 4.1's) */

#pragma code-name (push, "SW_look_way_CODE")    /* (Part names_said's till 2026-11-01) */
#pragma rodata-name (push, "SW_look_way_RODATA")

unsigned char __fastcall__ things (unsigned char op, const char* name);    /* (main.c's) */
extern struct ireq judged;
extern struct portal viewed;

/* A portal it sees looked at closely (the parser's thing, the way it lies): the world's record of it (the hub's
** wreq, WQ_EXAMINE: a doorway has none), in words (overlay 4), its aspects as ex asks (EX_*: 0 its plain features;
** its quality, judged by the craft that knows its material, the referee's test, P_APPRAISE: AP_WOOD, AP_METAL,
** AP_STONE; its lock, its condition, plain); a few seconds' looking (main.c's part commands's once, its room) */
extern unsigned long xs_seed;
static unsigned long judge_was;

/* The words' dice for a thing judged (k its key, ml the skill's mastery, the game's seed): the same again, the same
** words, till the skill improves (the user's, 2026-11-03: the referee's die the same way, rshouse.h's JUDGE_IN); the
** UI's own to put back after (judge_was) */
static void __fastcall__ judge_dice (unsigned k, unsigned ml)
{
    unsigned h = k * 40503U ^ ml * 257 ^ (unsigned) HUB->seed;

    judge_was = xs_seed;
    ((unsigned*) &xs_seed)[1] = h;
    ((unsigned*) &xs_seed)[0] = (h * 25173U + 13849U) | 1;
    random (2);
    random (2);
}

static void __fastcall__ inspect (unsigned char obj, unsigned char ex)
{
    struct req* r;
    signed char g = 0;
    unsigned k = 0;

    if (!feature (obj | PICK_TELL, FG_NEAR | FW_NONE)) {
        return;                                 /* (Found, walked up to beside it if it's out of reach, seen or remembered,
                                                **   then again; faced: main.c's feature path) */
    }
    HUB->wreq.kind = WQ_EXAMINE;
    HUB->wreq.x = tx;
    HUB->wreq.y = ty;
    hub_call (&HUB->wreq, T_WORLD);
    HUB->wreq.state = REQ_FREE;
    if (HUB->wreq.res) {
        msg ("An open doorway: there's no door in it.");
        return;
    }
    memcpy (&viewed, &HUB->pview, sizeof viewed);
    if (ex & EX_QUALITY) {                      /* (Its quality judged: the appraiser's test, the first answer kept, by
                                                **   a hash of the door's square: the user's, 2026-11-03) */
        RS_TEST_ITEM (HUB->slot[SLOT_UI]) = tx * 251 + ty;
        r = ask_b (RS_TEST, P_APPRAISE, viewed.mat == PM_STONE ? AP_STONE : viewed.mat == PM_IRON ? AP_METAL : AP_WOOD);
        g = RS_TEST_GRADE (*r);
        k = RS_TEST_MASTERY (*r);
        r->state = REQ_FREE;
    }
    judge_dice (tx * 251 + ty, k);
    portal_look (&viewed, g, ex);
    xs_seed = judge_was;
    msg (lb);
    if (!(ex & (EX_VALUE | EX_WEIGHT)) || (ex & EX_QUALITY)) {
        intend2 (I_WAIT, ex & (EX_LOCK | EX_COND) ? 20 : 50, I_WAIT_W2 (0, 0, 1));
    }
}

/* A thing (carried, or seen) looked at as anyone sees it, no test (the world's IO_LOOK, mode LM_*: "Your torch is in
** your hand. It's lit.", "The chest is beside you. It's closed, with a lock."), its time t tenths: 1; 0 if there's no
** such thing it carries or sees (out of reach, its weight felt or it gone over for traps: walked over to first, 1) */
static unsigned char __fastcall__ looked (unsigned char mode, unsigned char t)
{
    unsigned char r;

    if (!cmd.obj.word[0]) {
        return 0;
    }
    HUB->ireq.to = mode;
    if ((r = things (IO_LOOK | IO_GO, cmd.obj.word)) != IR_OK) {
        return r == IR_GONE;                    /* (The one path for things: out of reach, walked over to first, then
                                                **   again) */
    }
    msg (tt ());
    intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
    return 1;
}

/* A thing appraised (its value, its quality, both: ex's, EX_*): what it truly is (the world's IO_DESCRIBE, the one
** of several the command says), judged by the craft that knows it (the referee's P_APPRAISE), the aspects asked in
** words (overlay 1's appraise): 1; 0 if there's no such thing it carries or sees */
static unsigned char __fastcall__ judged_as (unsigned char ex)
{
    struct req* r;
    signed char g;

    if (!cmd.obj.word[0]) {
        return 0;
    }
    HUB->ireq.to = 1;                           /* (Out of reach: walked over to first) */
    if ((g = things (IO_DESCRIBE | IO_GO, cmd.obj.word)) != IR_OK) {
        return g == IR_GONE;                    /* (Out of reach: walked over to first, then again) */
    }
    memcpy (&judged, &HUB->ireq, sizeof judged);
    RS_TEST_ITEM (HUB->slot[SLOT_UI]) = judged.item;        /* (Its number: the first answer kept till the skill improves, the
                                                **   user's, 2026-11-03; the referee's and the words' dice its hash) */
    r = ask_b (RS_TEST, P_APPRAISE, judged.craft);
    g = RS_TEST_GRADE (*r);
    judge_dice (judged.item, RS_TEST_MASTERY (*r));
    r->state = REQ_FREE;
    appraise (&judged, g, ex);
    xs_seed = judge_was;
    msg (lb);
    return 1;
}

/* An opening looked at, or examined (stairs, a ladder, a shaft, the way out: "look at the stairs", COMMANDS-CRITIQUE.md's
** step 6, 2026-11-07): the one meant (choose.c's pick, which asked of several), worded as the room's look words it (words.c's
** opening: "You see a stairway leading down, ahead to your left."), not the room again: 1, told or asked; 0, none of
** those in sight now (one remembered: the look about's) */
#define OPENS       0x00B8                      /* (OB_STAIRS, OB_LADDER, OB_SHAFT, OB_WAY_OUT) */
static const char* const way_noun[12] = { "", "door", "passage", "stairway", "ladder", "shaft", "", "way out", "",
    "window", "", "portcullis" };               /* (WAY_OBJ's, each by its OB_*: a sniff toward it named) */
static unsigned char opened (void)
{
    struct sight* s;
    unsigned char i;

    if (pick (cmd.obj.kind | PICK_TELL, cmd.obj.lies) != 1) {
        return 1;                               /* (None, told; or which, asked) */
    }
    s = sight_get ();
    for (i = 0; i < s->nopen && (s->open[i].x != tx || s->open[i].y != ty); ++i) {
    }
    if (i >= s->nopen) {
        return 0;
    }
    sight_close = 64 | i;
    words ();
    sight_close = 0;
    msg (lb + 1);
    if ((s->open[i].kind & ~O_DAYLIT) == O_WAY_OUT) {
        way_words ();                           /* (The way out: shut, the theme's lock's words; or open) */
    }
    return 1;
}

/* A thing, a portal, the room looked at closely, or the aspects asked of it (main.c's VB_EXAMINE_OBJECT,
** VB_EXAMINE_LOCALE: a denizen's its own; a portal gone over for traps, main.c's door work): 0, or that door work,
** PA_CHECK */
static unsigned char examine (void)
{
    unsigned char ex = cmd.aux, k = cmd.obj.kind;

    if (!strcmp (cmd.obj.word, "money") || !strcmp (cmd.obj.word, "coins")) {
        strcpy (cmd.obj.word, "pennies");       /* (The money counted, its pennies: "count the money", 2026-10-28) */
    }
    if (cmd.obj.word[0]) {
        if (ex || !k) {
            walk_op = IO_DESCRIBE;              /* (Its first words before the world's asked: walk_ahead's, sway.c) */
            walk_ahead (cmd.obj.word);
        }
        if (ex & EX_BOTH ? judged_as (ex) : looked (ex & EX_TRAPS ? LM_TRAPS : ex & EX_WEIGHT ? LM_WEIGHT : ex & EX_LOCK ?
            LM_LOCK : ex & EX_COND ? LM_COND : LM_CLOSE, ex & EX_TRAPS ? 100 : ex ? 10 : 30)) {
            return 0;
        }
        if (!k) {
            say_no (A_SEE_NO, cmd.obj.word);
            return 0;
        }
    }
    if (!k && (ex & (EX_LOCK | EX_TRAPS))) {
        cmd.obj.kind = k = OB_DOOR;             /* (A lock, a trap, nothing named: the door in front) */
    }
    if (k == OB_DOOR || k == OB_WINDOW || k == OB_PORTC) {
        if (ex & EX_TRAPS) {
            return PA_CHECK;                    /* (Gone over for traps: the world's, main.c's door work) */
        }
        inspect (k, ex);
    } else if (ex) {
        msg ("Which thing? Name it (assess the sword, check the chest for traps ...).");
    } else if (k == OB_WALL) {
        walk_says ((const char*) W_DIG);
        intend2 (I_TRACE, I_TRACE_W1 (TRC_WALL, 0), 0);   /* (The wall ahead, its rock: the world's words, slice 4.10's) */
    } else if (!((OPENS >> (k & 15)) & 1) || !opened ()) {
        look (2);                               /* (A close look about: the room's figures) */
    }
    return 0;
}

/* Gone to a thing, a denizen, a body: anything it sees, or remembers seeing in this room though it's out of the light
** now (the user's, 2026-11-02: HOUSE.md's going places; main.c's move, VB_APPROACH_OBJECT its way here): the one found
** (resolve.c's: asked which, of several, its square in cmd.obj), else the nearest of that name (the world's IO_LOOK,
** LM_GO).  Walked to by the world's walk, stopping beside it (I_TAKE's a 2, walked on as it gets nearer: through a door
** it's seen through; a denizen, I_CLOSE's w1 1: up to it, as it moves, no blow); refused only for what it hasn't seen */
#pragma code-name (push, "SW_missiles_CODE")    /* (Called by its s_missiles: part look_way's room) */
#pragma rodata-name (push, "SW_missiles_RODATA")
void go_thing (void)
{
    unsigned x = cmd.obj.x, y = cmd.obj.y;
    unsigned char k;
    struct actor* a;

    if (!cmd.obj.word[0]) {
        tell (T_GO_WHICH_WAY);
        return;
    }
    if (!x) {
        HUB->ireq.to = LM_GO;
        k = things (IO_LOOK, cmd.obj.word);
        if (k == IR_OK) {
            msg (tt ());                        /* (Within reach already: where it lies) */
            return;
        }
        if (k != IR_NOT_HERE) {
            say_no (A_UNSEEN, cmd.obj.word);
            return;
        }
    } else if (x - HUB->pc_x + 1 <= 2 && y - HUB->pc_y + 1 <= 2) {
        sayf ("\nThe %s is beside you.", cmd.obj.word);
        return;
    } else {
        for (k = 0, a = HUB->actor; k < NACTORS; ++k, ++a) {
            if (a->state && a->x == x && a->y == y && a->sn == HUB->pc_stratum) {
                walk_says (cmd.obj.word);
                intend_a = k + 1;
                intend2 (I_CLOSE, 1, 0);        /* (A denizen's own: up to it as it moves, its walk not a thing's) */
                return;
            }
        }
        HUB->ireq.x = x;
        HUB->ireq.y = y;
    }
    HUB->ireq.item = 0;
    walk_a = 2;                                 /* (The one walk over, main.c's reach_go: told at once, the user's D17;
                                                **   walked on as it gets nearer, nothing after) */
    reach_go ();
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A closed door's little window or its spy-hole looked through (the user's, 2026-11-14: "look through the grille",
** "the little window", "the spy-hole", "look through the door"; "the window" with none seen but a door's): the door
** meant, its square pick's (pick_x, pick_y: the one with it, the nearest; or the door said, pick's own), the command the
** door's (OB_DOOR); 2 if it's closed with a little window or a spy-hole in it, to be walked up to (beside it, faced, the
** look one way looks through it: dq0p's view.c), 1 if not, 0 none (told: none with one; or which door, asked) */
static unsigned char peep (void)
{
    const char* w = cmd.obj.word;
    const struct sight* s;
    const struct sdoor* d;
    unsigned char i, g = 0, n = 0xFF, r = 1;
    unsigned a, b = 0xFFFF;

    if (strstr (w, "grill")) {
        g = PF_GRILLE;
    } else if (strstr (w, "spy") || strstr (w, "peep")) {
        g = PF_PEEP;
    } else if (cmd.obj.kind == OB_WINDOW && (!strcmp (w, "little") || !strcmp (w, "small") ||
        !pick (OB_WINDOW | PICK_ANY, cmd.obj.lies))) {
        g = PF_GRILLE | 0x80;                   /* ("The little window"; "the window", none seen: a door's, if there's
                                                **   one, else as ever) */
    } else if (cmd.action != VB_OBSERVE_THROUGH || cmd.obj.kind != OB_DOOR) {
        return 1;
    }
    s = sight_get ();
    if (g) {
        for (i = 0, d = s->door; i < s->ndoors; ++i, ++d) {
            if (d->kind == PK_DOOR && !(d->rel & MOUTH) && (d->parts & g & 0x7F) && (a = abs ((int) (d->x -
                s->x)) + abs ((int) (d->y - s->y))) < b) {
                b = a;
                n = i;
            }
        }
        if (n == 0xFF) {
            if (g & 0x80) {
                return 1;                       /* (No window, no door's: "You haven't seen a window here.", as ever) */
            }
            strcpy (lb, g == PF_PEEP ? "spy-hole" : "little window in a door");
            say_no (A_UNSEEN, lb);
            return 0;
        }
        tx = s->door[n].x;
        ty = s->door[n].y;
    } else if (pick (OB_DOOR | PICK_TELL, cmd.obj.lies) != 1) {
        return 0;                               /* ("Look through the door": none, told; which, asked) */
    }
    cmd.obj.kind = OB_DOOR;
    pick_x = tx;
    pick_y = ty;
    for (i = 0, d = s->door; i < s->ndoors; ++i, ++d) {
        if (d->x == tx && d->y == ty && d->kind == PK_DOOR && d->state != DOORWAY &&
            !(d->state & (PS_OPEN | PS_WRECKED)) && (d->parts & (PF_GRILLE | PF_PEEP))) {
            r = 2;
        }
    }
    return r;
}

unsigned char __fastcall__ s_look_way (unsigned char action)
{
    unsigned char h, k, j, e = 1;
    unsigned x0 = pick_x, y0 = pick_y;

    if (action == VB_EXAMINE_OBJECT) {
        return examine ();
    }
    if (action == VB_LEAVE) {                   /* ("Leave", "exit", "leave the room": by the door or the passage's mouth
                                                **   it means, which, asked; "exit north": that way) */
        tx = 0;
        if (cmd.dir > H_NW) {                   /* (The one meant: found, a door walked up to first, as a move through
                                                **   it is: main.c's feature path) */
            if (!feature ((cmd.obj.kind && cmd.obj.kind != OB_ROOM ? cmd.obj.kind : OB_ROOM) | PICK_TELL, FG_GO |
                FW_NONE)) {
                return 0;
            }
            cmd.dir = cmd.obj.lies;
        }
        intend_a = cmd.dir;
        intend2 (I_GO, tx, ty);
        return 0;
    }
    if (action == VB_NONE) {                    /* (A listen's or a sniff's end: what came of it) */
        if (lsn & 3) {
            bg_words (heard_at != 0);           /* (Nothing told as it listened, main.c's heard, or something: the
                                                **   background behind it, the sound layer's: part news's) */
        }
        lsn = 0;
        return 0;
    }
    if (action == VB_TRACK) {
        if (cmd.aux != TRC_TRACK) {             /* (The floor looked at, or the wall: its first words at once) */
            cmd.dir = cmd.aux ? 0 : GO_DOWN;    /* (TRC_FLOOR's 0) */
            walk_says ((const char*) W_DIG);
        }
        intend2 (I_TRACE, I_TRACE_W1 (cmd.aux, 0), 0);   /* (The traces read, the world's: the floor looked at, the Tracking
                                                **   test; worded as its E_VIEW comes, V_TRACED) */
        return 0;
    }
    k = action == VB_LISTEN ? 1 : action == VB_SMELL ? 4 : 0;
    h = cmd.dir;                                /* (A way: "look north", "listen left") */
    if (!k && h > H_NW && action == VB_OBSERVE_OBJECT && ((OPENS >> (cmd.obj.kind & 15)) & 1) && opened ()) {
        return 0;                               /* ("Look at the stairs": them, not the way they lie) */
    }
    if (!k && h > H_NW && !(e = peep ())) {
        return 0;                               /* (A closed door's little window, its spy-hole: none, or which) */
    }
    j = 0;
    if (h > H_NW) {
        if ((WAY_OBJ >> (cmd.obj.kind & 15)) & 1) {     /* (The one it sees) */
            j = cmd.obj.kind | PICK_TELL;
        } else if (k && h == 0xFF && (k == 4 || !cmd.obj.word[0])) {
            k += k & 1;                         /* ("listen", "sniff", "smell the air": about it) */
        } else {
            if (!k && cmd.obj.word[0] && !cmd.obj.kind) {
                walk_op = IO_DESCRIBE;          /* ("You look at the body.": its first words at once, examine's, the
                                                **   world's answer to come: the user's, 2026-11-17; a thing's, not a
                                                **   part of the room, "look in the far side") */
                walk_ahead (cmd.obj.word);
            }
            if (k || !looked (LM_AT, 10)) {     /* ("look at the table": what anyone sees of it, the user's) */
                look (0);                       /* ("look up", "look at the rat": a look about, as ever) */
            }
            return 0;
        }
    }
    if (!j) {
        cmd.obj.lies = h;                       /* (A way said: that way) */
    }
    if (j || hold_w[0] || (h <= H_NW && h != HUB->pc_face)) {
        j = feature (j, (e == 2 ? FG_NEAR : FG_FAR) | FW_NONE);   /* (A closed door's little window, its spy-hole:
                                                **   walked up to first) */
    } else {
        j = 1;                                  /* (None named, nothing in hand to take, the way it faces or none: its
                                                **   feature's 1 and nothing more, its part not swapped in for it, some
                                                **   50,000 cycles of a listen's first words, a look ahead's, 2026-11-16) */
    }
    pick_x = x0;
    pick_y = y0;
    if (!j) {
        return 0;                               /* (The one it sees, or the way said: turned to it first, the turn's look,
                                                **   main.c's feature path; none, told; or which, asked) */
    }
    h = cmd.obj.lies;
    if (!k) {
        intend2 (I_LOOK, WAY_T, e == 2);        /* (The world's: the senses' look one way, E_VIEW V_LOOKED; through the
                                                **   closed door's little window, its spy-hole: LK_PEEK.  Asked before
                                                **   its first words are said, the look begun as they are, its far
                                                **   part's the sooner: 2026-11-18) */
    }
    if (j == 1 && k < 4) {                      /* (Its first words at once, the look's or the listen's 4.8M cycles and
                                                **   more off: the user's, 2026-11-04; turned first, the turn's are) */
        walk_says ((const char*) (k ? W_LISTEN : W_PEER));
    }
    if (!k) {
        return 0;
    }
    if (k == 4) {
        if (cmd.obj.kind) {                 /* (A way's only: refuse.c's M_WAYS) */
            sayf ("\nYou sniff toward the %s.", way_noun[cmd.obj.kind & 15]);   /* ("Smell the stairs": named, its
                                                **   answer the air's that way: 2026-11-07, dq0sweep's) */
        }
        bg_words (2);                           /* (The draught, the air's: part news's, slice 4.5's) */
        intend2 (I_TRACE, I_TRACE_W1 (TRC_SMELL, h), WAY_T);   /* (The world's: the smell read, worded, then
                                                **   its time) */
        return 0;
    }
    heard_at = 0;                               /* (What it hears told, however soon after the last, and noted:
                                                **   main.c's heard) */
    lsn = k;
    intend2 (I_WAIT, WAY_T, I_WAIT_W2 (0, 0, 1));
    return 0;
}

/* A look one way asked from an overlay's code (hthings.c's H_LOOK_IN, overlay 1: "look into the passage"): look_way's,
** the hub's header in the window for it, then the window put back as it found it.  Its calls leave the hub's bank in
** the window (things (), a thing looked at): the overlay's code, returned to there, ran in it (2026-11-16: "look in the
** far side", "look in the side pocket" ended the game); and the hub's header written as it read, the overlay's bank
** in the window, written in that instead (othings' way, main.c's) */
void __fastcall__ s_look_ov (unsigned char action)
{
    unsigned w = hub_here ();

    hub_map (0);
    look_way (action);
    hub_back (w);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Overlay 13 (after part aliases's image): a look one way's words (words.c's o_words, as its look's E_VIEW is V_LOOKED:
** look.c's look (8)), from the senses' record (sight_copy) and their look one way's own (WAY_SEEN, copied into lb's
** end, words.c's not begun yet): the place it's in not told again; its own sentences said as they're made (none a
** glance says), the things' and the denizens' (those ahead, the record's then) in the glance's words, streamed
** (lbsaid.s's: none said twice in an answer).  What's near told first (the user's, 2026-11-17): a look one way's near
** part (WAY_NEAR's, its first E_VIEW) as its far part's worked out, but what the far part may change (an opening held
** back, WB_HOLD; "nothing more that way", WM_HOLDN); then its far part (WAY_SEEN's, WM_FAR, its second): only what's
** new, its things, denizens and lights far off the near part told left out (their sentences with them: none told
** twice, those it adds told on their own), nothing at all if it's all as the near part had it (WM_SAME) */

#pragma code-name (push, "OV13CODE")
#pragma rodata-name (push, "OV13RODATA")
#pragma data-name (push, "OV13RODATA")
#pragma bss-name (push, "OV13RODATA")
#pragma static-locals (push, on)              /* (Its locals static, in its bank: the code the less) */

#define S           sight_copy
#define W           (*(struct wayseen*) (lb + sizeof lb - sizeof (struct wayseen)))

typedef char w_fits[sizeof (struct wayseen) <= 300 ? 1 : -1];   /* (In lb's end: its first 100 bytes the theme's names,
                                                **   and the rows' words) */
static unsigned char ahpl, ahrel;               /* (A row's plural or choice, its way: UK_AHEAD's pl, rel) */
static unsigned ahfar;                          /* (Its squares, and a room's kind or a choice: far, room) */

/* A look ahead's words: said.txt's AHEAD row w >> 8 (UK_AHEAD: k w's low byte; pl, rel, far and room the statics
** above), one of focus_words' say ()s before (narration as data's rest, 2026-11-15) */
#pragma static-locals (push, off)               /* (Its event on the C stack, the main RAM: the words' overlay reads it) */
static void __fastcall__ ah (unsigned w)
{
    struct msg c;

    c.kind = UK_AHEAD;
    c.who = 0;
    c.a = ahpl;
    c.b = ahrel;
    c.w1 = w;
    c.w2 = ahfar;
    outcome_words (&c);
}
#pragma static-locals (pop)

/* Openings b and the next alike, the same way, and the same beyond them: told once ("Through the open doors ahead you
** see only darkness.", not the open door's twice) */
static unsigned char __fastcall__ twin (const struct beyond* b)
{
    return b[1].what == b->what && b[1].noun == b->noun && b[1].rel == b->rel && (b->what == WB_DARK ||
        (b[1].dist == b->dist && b[1].kind == b->kind && b[1].lit == b->lit));
}

static unsigned char nid;                       /* (The near part told last, its id; the things, the denizens and the */
static unsigned nit[SEE_ITEMS];                 /*   lights far off it told: their numbers, records, ways; and if it */
static unsigned char nni, nden[SEE_DENS], nnd, ngl[SEE_GLOWS], nng, nany;   /*   told anything at all; its openings, */
static struct beyond nb[SEE_BEYOND];            /*   what's beyond them told (0xFF, held back: none); and a part
                                                **   with its rest to come told (WM_REST's: what it told added, nrest) */
static unsigned char nrest, uc;                 /* (uc: the hub's ui_cmd set as a near part's told, till its far part's) */

/* Opening b, what's beyond it, as told before (its own byte by byte, told "so far" or not: the C library's memcmp not the
** UI's) */
static unsigned char __fastcall__ near_said (const struct beyond* b)
{
    unsigned char j, k;

    for (j = 0; j < SEE_BEYOND; ++j) {
        for (k = 1; k < sizeof *b && ((const unsigned char*) b)[k] == ((const unsigned char*) &nb[j])[k]; ++k) {
        }
        if (k == sizeof *b && !((b->what ^ nb[j].what) & ~WB_HOLD)) {
            return 1;
        }
    }
    return 0;
}

/* What a part with its rest to come told (WM_REST's: the look's own sentences, its things, denizens and lights far off),
** added to what's been told (a whole look's, all of it), so its rest tells only what's new */
static void told_add (void)
{
    unsigned char i;

    for (i = 0; i < W.nitems && nni < SEE_ITEMS; ++i) {
        nit[nni++] = W.item[i].n;
    }
    for (i = 0; i < W.ndens && nnd < SEE_DENS; ++i) {
        nden[nnd++] = W.den[i].den;
    }
    for (i = 0; i < W.nglows && nng < SEE_GLOWS; ++i) {
        ngl[nng++] = W.glow[i].rel;
    }
    nany |= W.nbeyond | W.nitems | W.ndens | W.nglows;
    memcpy (nb, W.beyond, sizeof nb);
    for (i = W.nbeyond; i < SEE_BEYOND; ++i) {
        nb[i].what = 0xFF;
    }
    nrest = 1;
}

/* The hub's ui_cmd, a near part's told (the user's, 2026-11-18: "a denizen's stir is held until a look's far part is
** told: the look reads as one piece"): a stir held till its far part's told (main.c's events, stir_held), the minds with
** it (the senses' sense_held); let go as the far part's told, if this set it */
static void __fastcall__ cmd_hold (unsigned char on)
{
    if (on | uc) {
        uc = on;
        hub_mem (0, (void*) &HUB->ui_cmd, &uc, 1);
    }
}

/* The far part's things (k 0), denizens (1), lights far off (2) at W's entry i: one the near part told */
static unsigned char __fastcall__ near_told (unsigned char k, unsigned char i)
{
    unsigned char j;

    for (j = k ? k == 1 ? nnd : nng : nni; j--; ) {
        if (k ? k == 1 ? nden[j] == W.den[i].den : ngl[j] == W.glow[i].rel : nit[j] == W.item[i].n) {
            return 1;
        }
    }
    return 0;
}

/* The far part's things, denizens, lights far off the near part told left out (a thing with those on it after it, hid
** 2, left out only if they all were) */
static void far_new (void)
{
    unsigned char i, j, n, a;

    for (i = n = 0; i < W.nitems; i = j) {
        for (a = near_told (0, i), j = i + 1; j < W.nitems && (W.item[j].hid & 2); ++j) {
            a &= near_told (0, j);
        }
        if (!a) {
            memmove (&W.item[n], &W.item[i], (j - i) * sizeof W.item[0]);
            n += j - i;
        }
    }
    W.nitems = n;
    for (i = n = 0; i < W.ndens; ++i) {
        if (!near_told (1, i)) {
            W.den[n++] = W.den[i];
        }
    }
    W.ndens = n;
    for (i = n = 0; i < W.nglows; ++i) {
        if (!near_told (2, i)) {
            W.glow[n++] = W.glow[i];
        }
    }
    W.nglows = n;
}

void o_focus_words (void)
{
    struct beyond* b;
    struct beyond* c;
    unsigned char i, j, k, n, h, hm, part, lead, told = 0;

    hub_mem (SIGHT_BANK, &W, WAY_NEAR, sizeof W);
    part = 0;                                   /* (0 the whole of it; 1 its near part; 2 its far part, after it) */
    if ((W.more & WM_NEAR) && W.id != nid) {
        part = 1;                               /* (Its near part, told first: what it told kept) */
        nid = W.id;
        for (i = nni = W.nitems; i--; ) {
            nit[i] = W.item[i].n;
        }
        for (i = nnd = W.ndens; i--; ) {
            nden[i] = W.den[i].den;
        }
        for (i = nng = W.nglows; i--; ) {
            ngl[i] = W.glow[i].rel;
        }
        nany = W.nbeyond | W.nitems | W.ndens | W.nglows;
        memcpy (nb, W.beyond, sizeof nb);
        for (i = 0; i < SEE_BEYOND; ++i) {
            if (i >= W.nbeyond || (nb[i].what & WB_HOLD)) {
                nb[i].what = 0xFF;              /* (None, or held back: matching no opening) */
            }
        }
        nrest = 0;
    } else {
        hub_mem (SIGHT_BANK, &W, WAY_SEEN, sizeof W);
        if ((W.more & WM_FAR) && W.id == nid) {
            if (W.more & WM_SAME) {
                nrest |= (W.more & WM_REST) != 0;
                cmd_hold (0);
                sight_get ();                   /* (All of it as its near part had it: nothing more to tell) */
                return;
            }
            part = 2;
            far_new ();                         /* (Only what's new) */
        } else if (W.seq != S.seq) {
            W.nbeyond = W.nitems = W.ndens = W.nglows = 0;  /* (Not this record's: a look about came after it) */
        }
    }
    for (i = hm = 0; i < W.nbeyond && part != 1; ++i) {
        if (W.beyond[i].what & WB_HOLD) {       /* (Its way on to ground not made yet: told "so far", but a near
                                                **   part's, held back for its far part) */
            W.beyond[i].what &= ~WB_HOLD;
            hm |= 1 << i;
        }
    }
    lead = (W.more & WM_RESTED) != 0;           /* (A look's rest, told after the prompt: its first sentence "Peering
                                                **   on, ...", the look's going on, the user's, 2026-11-19, L8) */
    S.eye = W.eye;                              /* (What it makes out, a look one way's: twice as far) */
    if (worded != 3 && part != 2) {
        ah (AH_PEER << 8 | (S.dark != 0));
    }
    for (i = 0, b = W.beyond; i < W.nbeyond; ++i, ++b) {    /* What's beyond the openings ahead (k: the opening,
                                                **   BN_PASSAGE 0): "You can't see past the door ahead: it's
                                                **   closed."; "Through the doorway ahead you see only darkness."; "...
                                                **   you see a passage, 20 feet of it."; "Down the passage ahead you
                                                **   see 30 feet of it."; "Through the open door ahead you see into the
                                                **   kitchen, lit by torches."; "Down the passage ahead you see into
                                                **   the hall, 30 feet on." */
        k = b->noun;
        if ((told & 1 << i) || (b->what & WB_HOLD) || (part == 2 && near_said (b))) {
            continue;                           /* (A way into a room told already, with the first; or what's beyond
                                                **   it held back for the far part; a far part's as its near part told
                                                **   it, none of the look's own said twice) */
        }
        h = hm >> i & 1;
        if (k >= BN_GRILLE && b->what == WB_SHUT) {
            ah (AH_COVER << 8);
            continue;                           /* (A closed door's spy-hole, the eye at it: dq0p's way.c, the user's,
                                                **   2026-11-14) */
        }
        if (b->what == WB_SHUT) {
            for (j = i + 1, c = b + 1, n = 0; j < W.nbeyond; ++j, ++c) {   /* (Alike the same way, shut: told once,
                                                **   "the doors ahead to your left", wherever they come) */
                if (c->what == WB_SHUT && c->noun == k && ((c->rel + 1) & 3) == ((b->rel + 1) & 3)) {
                    told |= 1 << j;
                    ++n;
                }
            }
            ahpl = !n ? 0 : k == BN_PORTCULLIS ? 2 : 1;     /* ("es", "s") */
            ahrel = (b->rel + 1) & 3;
            ahfar = (k == BN_WINDOW ? (n ? 5 : 4) : n ? 3 : b->kind & PS_BARRED_ ? 1 : b->kind & PS_LOCKED_ ? 2 : 0) << 8;
            ah (AH_SHUT << 8 | k);
            continue;
        }
        for (n = 0; i + 1 < W.nbeyond && twin (b); ++n, ++i, ++b) {
        }                                       /* (Alike the same way: told once, "the open doors") */
        if (k >= BN_GRILLE) {                   /* ("Through the little window in the door you see into the guardroom, lit
                                                **   by torches.") */
            ahpl = k != BN_GRILLE;              /* (The spy-hole's) */
            ah (AH_GRILLE << 8);
        } else {
            ahpl = !n ? 0 : k == BN_PORTCULLIS ? 2 : 1;
            ahrel = (b->rel + 1) & 3;
            ah ((lead ? AH_ONTHROUGH : AH_THROUGH) << 8 | k);
            lead = 0;
        }
        ahfar = b->kind << 8 | b->dist;         /* (Its squares; a room's kind) */
        if (b->what == WB_DARK) {
            ah (AH_DARK << 8);
        } else if (b->what == WB_SKY) {
            ah (AH_SKY << 8);
        } else if (b->what == WB_PASSAGE) {
            ahpl = n != 0;                      /* ("feet of them") */
            ah (AH_PASSAGE << 8 | k);
        } else {
            ah (AH_INTO << 8);                  /* (The room's name the theme's) */
            if (!k) {
                ah (AH_ON << 8);
            }
            if (b->lit) {
                ah (AH_LIT << 8);
            }
            for (j = i + 1, c = b + 1; j < W.nbeyond; ++j, ++c) {  /* (Another way ahead into the same room: told
                                                **   with it, not the room again, the user's, 2026-10-22) */
                if (c->what == WB_ROOM && c->kind == b->kind) {
                    told |= 1 << j;
                    ahrel = (c->rel + 1) & 3;
                    ah (AH_TOO << 8 | c->noun);
                }
            }
        }
        if (h) {
            ah (AH_SOFAR << 8);                 /* ("Down the passage ahead you see 20 feet of it so far.": its rest to
                                                **   come, the user's, 2026-11-18) */
        }
        ah (AH_END << 8);
    }
    for (i = 0; i < W.nglows; ++i) {            /* The lights far off ahead */
        ahrel = (W.glow[i].rel + 1) & 3;
        ah ((lead ? AH_ONGLOW : AH_GLOW) << 8 | (W.glow[i].how != GL_LIGHT));
        lead = 0;
    }
    if (!(W.nbeyond | W.nitems | W.ndens | W.nglows) && !(part == 1 && (W.more & WM_HOLDN)) && !(part == 2 && (nany | nrest))
        && !(W.more & WM_REST)) {
        ah (AH_NOTHING << 8);                   /* (Nothing: a near part's, unless the far part may yet see more; a far
                                                **   part's, unless its near part told something; none with its rest
                                                **   to come, nor its rest's) */
    }
    if (W.more & WM_REST) {
        if (part != 2) {
            nid = W.id;                         /* (A whole look's, its rest to come: what it tells kept) */
            nni = nnd = nng = nany = 0;
        }
        told_add ();
    }
    cmd_hold (part == 1);
    if (lead && (W.nitems | W.ndens)) {
        ah (AH_ONMORE << 8);                    /* (Its things and denizens in a glance's words after it) */
    }
    memcpy (&S.nitems, &W.nitems, sizeof S.nitems + sizeof S.item);   /* The things and the denizens ahead, in a */
    memcpy (&S.ndens, &W.ndens, sizeof S.ndens + sizeof S.den);       /*   glance's words (overlay 7's, 9's) */
    lbn = 1;                                    /* (Said after these, as a look's are) */
    *(lbe = lb) = 0;
    seen_words ();
    if (lb_stream) {
        lb_said ();                             /* (The last of it said) */
    }
    sight_get ();                               /* (The record whole again: the things and the denizens about) */
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma data-name (pop)
#pragma bss-name (pop)
