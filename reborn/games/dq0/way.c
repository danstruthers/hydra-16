/*
** way.c - Dingus Quest Zero's looking one way, and listening and sniffing one way (HOUSE.md's slice 4.0: "look
** north", "look left", "look at the door", "look through the window"; "listen at the door", "sniff at the grating"):
** the command carried out (part 6), and a look one way's words (overlay 11).  The way is the one said, or the one a
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
#include "dq0.h"
#include "hub.h"
#include "level.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's, an overlay's) */

#define WAY_T       30                          /* (A look one way's time, a listen's, a sniff's: tenths) */
#define WAY_OBJ     0x0ABE                      /* (The things looked at that are a way: a bit each, OB_DOOR,
                                                **   OB_PASSAGE, OB_STAIRS, OB_LADDER, OB_SHAFT, OB_WAY_OUT,
                                                **   OB_WINDOW, OB_PORTC: pick's) */

/* ==== Part 6: the command (main.c's chores: the parser's VB_OBSERVE_DIRECTION, VB_OBSERVE_OBJECT,
** VB_OBSERVE_THROUGH, VB_LISTEN, VB_SMELL; and VB_TRACK, the traces read, slice 4.1's) */

#pragma code-name (push, "SW6CODE")
#pragma rodata-name (push, "SW6RODATA")

unsigned char __fastcall__ things (unsigned char op, const char* name);    /* (main.c's) */
extern struct ireq judged;
extern struct portal viewed;

/* Turned to heading h (a look's, a close look's: the turn's look) */
static void __fastcall__ turn_to (unsigned char h)
{
    struct msg m;

    memset (&m, 0, sizeof m);
    m.kind = I_TURN;
    m.a = h;
    hub_send (&m);
}

/* A portal it sees looked at closely (the parser's thing, the way it lies): the world's record of it (the hub's
** wreq, WQ_EXAMINE: a doorway has none), in words (overlay 4), its aspects as ex asks (EX_*: 0 its plain features;
** its quality, judged by the craft that knows its material, the referee's test, P_APPRAISE: AP_WOOD, AP_METAL,
** AP_STONE; its lock, its condition, plain); a few seconds' looking (main.c's part 2's once, its room) */
static void __fastcall__ inspect (unsigned char obj, unsigned char orel, unsigned char ex)
{
    struct req* r;
    signed char g = 0;

    if (pick (obj | PICK_TELL, orel) != 1) {
        return;
    }
    if (REL_BEHIND (REL8 (g = bearing ((int) tx - HUB->pc_x, (int) ty - HUB->pc_y), HUB->pc_face))) {
        turn_to (g);                            /* (One behind it, that it knows of: turned to, to see it) */
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
    if (ex & EX_QUALITY) {                      /* (Its quality judged: the appraiser's test) */
        r = ask_b (RS_TEST, P_APPRAISE, viewed.mat == PM_STONE ? AP_STONE : viewed.mat == PM_IRON ? AP_METAL : AP_WOOD);
        g = r->grade;
        r->state = REQ_FREE;
    }
    portal_look (&viewed, g, ex);
    msg (lb);
    if (!(ex & (EX_VALUE | EX_WEIGHT)) || (ex & EX_QUALITY)) {
        intend2 (I_WAIT, ex & (EX_LOCK | EX_COND) ? 20 : 50, 1);
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
    if ((r = things (IO_LOOK, cmd.obj.word)) == IR_NOT_HERE) {
        reach_go ();
        return 1;
    }
    if (r != IR_OK) {
        return 0;
    }
    msg (tt ());
    intend2 (I_WAIT, t, 1);
    return 1;
}

/* A thing appraised (its value, its quality, both: ex's, EX_*): what it truly is (the world's IO_DESCRIBE, the one
** of several the command says), judged by the craft that knows it (the referee's P_APPRAISE), the aspects asked in
** words (overlay 1's appraise): 1; 0 if there's no such thing it carries or sees */
static unsigned char __fastcall__ judged_as (unsigned char ex)
{
    struct req* r;
    signed char g;

    if (!cmd.obj.word[0] || things (IO_DESCRIBE, cmd.obj.word) != IR_OK) {
        return 0;
    }
    memcpy (&judged, &HUB->ireq, sizeof judged);
    r = ask_b (RS_TEST, P_APPRAISE, judged.craft);
    g = r->grade;
    r->state = REQ_FREE;
    appraise (&judged, g, ex);
    msg (lb);
    return 1;
}

/* A thing, a portal, the room looked at closely, or the aspects asked of it (main.c's VB_EXAMINE_OBJECT,
** VB_EXAMINE_LOCALE: a denizen's its own; a portal gone over for traps, main.c's door work): 0, or that door work,
** PA_CHECK */
static unsigned char examine (void)
{
    unsigned char ex = cmd.aux, k = cmd.obj.kind;

    if (cmd.obj.word[0]) {
        if (ex & EX_BOTH ? judged_as (ex) : looked (ex & EX_TRAPS ? LM_TRAPS : ex & EX_WEIGHT ? LM_WEIGHT : ex & EX_LOCK ?
            LM_LOCK : ex & EX_COND ? LM_COND : LM_CLOSE, ex & EX_TRAPS ? 100 : ex ? 10 : 30)) {
            return 0;
        }
        if (!k) {
            sayf ("\nYou see no %s here.", cmd.obj.word);
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
        inspect (k, cmd.obj.lies, ex);
    } else if (ex) {
        msg ("Which thing? Name it (assess the sword, check the chest for traps ...).");
    } else {
        look (2);                               /* (A close look about: the room's figures) */
    }
    return 0;
}

unsigned char __fastcall__ s_look_way (unsigned char action)
{
    unsigned char h, k;

    if (action == VB_EXAMINE_OBJECT) {
        return examine ();
    }
    if (action == VB_NONE) {                    /* (A listen's or a sniff's end: what came of it) */
        if (!heard_at) {
            msg ("You hear nothing.");          /* (Nothing told as it listened: main.c's heard) */
        }
        lsn = 0;
        return 0;
    }
    if (action == VB_TRACK) {
        intend2 (I_TRACE, cmd.aux, 0);          /* (The traces read, the world's: the floor looked at, the Tracking
                                                **   test; worded as its E_VIEW comes, V_TRACED) */
        return 0;
    }
    k = action == VB_LISTEN ? 1 : action == VB_SMELL ? 4 : 0;
    h = cmd.dir;                                /* (A way: "look north", "listen left") */
    if (h > H_NW) {
        if ((WAY_OBJ >> (cmd.obj.kind & 15)) & 1) {     /* (The one it sees) */
            if (pick (cmd.obj.kind | PICK_TELL, cmd.obj.lies) != 1) {
                return 0;                       /* (None, told; or which, asked) */
            }
            h = bearing ((int) (tx - HUB->pc_x), (int) (ty - HUB->pc_y));
        } else if (k && h == 0xFF && (k == 4 || !cmd.obj.word[0])) {
            k += k & 1;                         /* ("listen", "sniff", "smell the air": about it) */
        } else {
            if (k || !looked (LM_AT, 10)) {     /* ("look at the table": what anyone sees of it, the user's) */
                look (0);                       /* ("look up", "look at the rat": a look about, as ever) */
            }
            return 0;
        }
    }
    if (h <= H_NW && h != HUB->pc_face) {       /* (Turned to it first: the turn's look) */
        turn_to (h);
    }
    if (!k) {
        intend2 (I_LOOK, WAY_T, 0);             /* (The world's: the senses' look one way, E_VIEW V_LOOKED) */
        return 0;
    }
    if (k == 4) {
        intend2 (I_TRACE, TRC_SMELL | (unsigned) h << 8, WAY_T);   /* (The world's: the smell read, worded, then
                                                **   its time) */
        return 0;
    }
    heard_at = 0;                               /* (What it hears told, however soon after the last, and noted:
                                                **   main.c's heard) */
    lsn = k;
    intend2 (I_WAIT, WAY_T, 1);
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Overlay 13 (after part 9's image): a look one way's words (words.c's o_words, as its look's E_VIEW is V_LOOKED:
** look.c's look (8)), from the senses' record (sight_copy) and their look one way's own (WAY_SEEN, copied into lb's
** end, words.c's not begun yet): the place it's in not told again; its own sentences said as they're made (none a
** glance says), the things' and the denizens' (those ahead, the record's then) in the glance's words, streamed
** (lbsaid.s's: none said twice in an answer) */

#pragma code-name (push, "OV13CODE")
#pragma rodata-name (push, "OV13RODATA")
#pragma data-name (push, "OV13RODATA")
#pragma bss-name (push, "OV13RODATA")
#pragma static-locals (push, on)              /* (Its locals static, in its bank: the code the less) */

#define S           sight_copy
#define W           (*(struct wayseen*) (lb + sizeof lb - sizeof (struct wayseen)))

static const char rel_w[3][20] = { "ahead to your left", "ahead", "ahead to your right" };
static const char noun[8][12] = { "passage", "doorway", "open door", "broken door", "door", "portcullis", "window",
    "arrow slit" };
typedef char w_fits[sizeof (struct wayseen) <= 300 ? 1 : -1];   /* (In lb's end: its first 100 bytes the theme's names) */

/* Openings b and the next alike, the same way, and the same beyond them: told once ("Through the open doors ahead you
** see only darkness.", not the open door's twice) */
static unsigned char __fastcall__ twin (const struct beyond* b)
{
    return b[1].what == b->what && b[1].noun == b->noun && b[1].rel == b->rel && (b->what == WB_DARK ||
        (b[1].dist == b->dist && b[1].kind == b->kind && b[1].lit == b->lit));
}

void o_focus_words (void)
{
    struct beyond* b;
    struct beyond* c;
    unsigned char i, j, k, n, sk = 0xFF, sr = 0xFF, told = 0;

    hub_mem (SIGHT_BANK, &W, WAY_SEEN, sizeof W);
    if (W.seq != S.seq) {
        W.nbeyond = W.nitems = W.ndens = W.nglows = 0;  /* (Not this record's: a look about came after it) */
    }
    msg (S.dark ? "You peer ahead into the dark." : "You peer ahead.");
    for (i = 0, b = W.beyond; i < W.nbeyond; ++i, ++b) {    /* What's beyond the openings ahead (k: the opening,
                                                **   BN_PASSAGE 0): "You can't see past the door ahead: it's
                                                **   closed."; "Through the doorway ahead you see only darkness."; "...
                                                **   you see a passage, 20 feet of it."; "Down the passage ahead you
                                                **   see 30 feet of it."; "Through the open door ahead you see into the
                                                **   kitchen, lit by torches."; "Down the passage ahead you see into
                                                **   the hall, 30 feet on." */
        k = b->noun;
        if (told & 1 << i) {
            continue;                           /* (A way into a room told already, with the first) */
        }
        if (b->what == WB_SHUT) {
            if (k == sk && b->rel == sr) {
                continue;                       /* (Two doors the same way, shut: said once) */
            }
            sk = k;
            sr = b->rel;
            sayf (" You can't see past the %s %s: %s.", noun[k], rel_w[(b->rel + 1) & 3], k == BN_WINDOW ?
                "its shutters are closed" : "it's closed");
            continue;
        }
        for (n = 0; i + 1 < W.nbeyond && twin (b); ++n, ++i, ++b) {
        }                                       /* (Alike the same way: told once, "the open doors") */
        sayf (" %s the %s%s %s you see ", k ? "Through" : "Down", noun[k], !n ? "" : k == BN_PORTCULLIS ? "es" : "s",
            rel_w[(b->rel + 1) & 3]);
        if (b->what == WB_DARK) {
            say ("only darkness");
        } else if (b->what == WB_PASSAGE) {
            sayf (k ? "a passage, %u feet of it" : n ? "%u feet of them" : "%u feet of it", SQ_FT * b->dist);
        } else {
            *lb = 0;
            theme_named (TN_ROOM, b->kind);     /* (The theme's, into lb) */
            sayf ("into the %s", lb);
            if (!k) {
                sayf (", %u feet on", SQ_FT * b->dist);
            }
            if (b->lit) {
                *lb = 0;
                theme_named (TN_LIGHT, 0);
                sayf (", %s", lb);
            }
            for (j = i + 1, c = b + 1; j < W.nbeyond; ++j, ++c) {  /* (Another way ahead into the same room: told
                                                **   with it, not the room again, the user's, 2026-10-22) */
                if (c->what == WB_ROOM && c->kind == b->kind) {
                    told |= 1 << j;
                    sayf ("; the %s %s leads there too", noun[c->noun], rel_w[(c->rel + 1) & 3]);
                }
            }
        }
        say (".");
    }
    for (i = 0; i < W.nglows; ++i) {            /* The lights far off ahead */
        sayf (" A %s far off %s.", W.glow[i].how == GL_LIGHT ? "light shows" : "faint glow shows",
            rel_w[(W.glow[i].rel + 1) & 3]);
    }
    if (!(W.nbeyond | W.nitems | W.ndens | W.nglows)) {
        say (" You see nothing more that way.");
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
