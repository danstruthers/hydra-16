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
**   a sniff           no smell to read yet (the trace layer's, slice 4.1): nothing out of the ordinary, as it ends
**
** A thing or a denizen looked at ("look at the rat") is a look about, as before.
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
** VB_OBSERVE_THROUGH, VB_LISTEN, VB_SMELL) */

#pragma code-name (push, "SW6CODE")
#pragma rodata-name (push, "SW6RODATA")

void __fastcall__ s_look_way (unsigned char action)
{
    struct msg m;
    unsigned char h, k;

    if (action == VB_NONE) {                    /* (A listen's or a sniff's end: what came of it) */
        if (lsn & 4) {
            msg ("You smell nothing out of the ordinary.");
        } else if (!heard_at) {
            msg ("You hear nothing.");          /* (Nothing told as it listened: main.c's heard) */
        }
        lsn = 0;
        return;
    }
    k = action == VB_LISTEN ? 1 : action == VB_SMELL ? 4 : 0;
    h = cmd.dir;                                /* (A way: "look north", "listen left") */
    if (h > H_NW) {
        if ((WAY_OBJ >> (cmd.obj.kind & 15)) & 1) {     /* (The one it sees) */
            if (pick (cmd.obj.kind | PICK_TELL, cmd.obj.lies) != 1) {
                return;                         /* (None, told; or which, asked) */
            }
            h = bearing ((int) (tx - HUB->pc_x), (int) (ty - HUB->pc_y));
        } else if (k && h == 0xFF && !cmd.obj.word[0]) {
            k += k & 1;                         /* ("listen", "sniff": about it) */
        } else {
            look (0);                           /* ("look up", "look at the rat": a look about, as ever) */
            return;
        }
    }
    if (h <= H_NW && h != HUB->pc_face) {       /* (Turned to it first: the turn's look) */
        memset (&m, 0, sizeof m);
        m.kind = I_TURN;
        m.a = h;
        hub_send (&m);
    }
    if (!k) {
        intend2 (I_LOOK, WAY_T, 0);             /* (The world's: the senses' look one way, E_VIEW V_LOOKED) */
        return;
    }
    heard_at = 0;                               /* (What it hears told, however soon after the last, and noted:
                                                **   main.c's heard) */
    lsn = k;
    intend2 (I_WAIT, WAY_T, 1);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Overlay 11: a look one way's words (words.c's o_words, as its look's E_VIEW is V_LOOKED: look.c's look (8)),
** from the senses' record (sight_copy) and their look one way's own (WAY_SEEN, copied into lb's end, words.c's not
** begun yet): the place it's in not told again; its own sentences said as they're made (none a glance says), the
** things' and the denizens' (those ahead, the record's then) in the glance's words, streamed (lbsaid.s's: none said
** twice in an answer) */

#pragma code-name (push, "OV11CODE")
#pragma rodata-name (push, "OV11RODATA")
#pragma data-name (push, "OV11RODATA")
#pragma bss-name (push, "OV11RODATA")
#pragma static-locals (push, on)              /* (Its locals static, in its bank: the code the less) */

#define S           sight_copy
#define W           (*(struct wayseen*) (lb + sizeof lb - sizeof (struct wayseen)))

static const char rel_w[3][20] = { "ahead to your left", "ahead", "ahead to your right" };
static const char noun[8][12] = { "passage", "doorway", "open door", "broken door", "door", "portcullis", "window",
    "arrow slit" };
typedef char w_fits[sizeof (struct wayseen) <= 300 ? 1 : -1];   /* (In lb's end: its first 100 bytes the theme's names) */

void o_focus_words (void)
{
    struct beyond* b;
    unsigned char i, k;

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
        if (b->what == WB_SHUT) {
            sayf (" You can't see past the %s %s: %s.", noun[k], rel_w[(b->rel + 1) & 3], k == BN_WINDOW ?
                "its shutters are closed" : "it's closed");
            continue;
        }
        sayf (" %s the %s %s you see ", k ? "Through" : "Down", noun[k], rel_w[(b->rel + 1) & 3]);
        if (b->what == WB_DARK) {
            say ("only darkness");
        } else if (b->what == WB_PASSAGE) {
            sayf (k ? "a passage, %u feet of it" : "%u feet of it", SQ_FT * b->dist);
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
