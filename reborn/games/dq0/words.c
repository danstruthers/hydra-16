/*
** words.c - (overlay 2) what the character perceives, in words (look.c's look: a copy of the world's record of it,
** struct sight, in the main RAM, for the hub isn't in the window while an overlay is).  The words are the
** character's: only what it sees now (or, in the dark, feels), each where it lies from it as it faces (one of eight:
** ahead, ahead to its right ... ahead to its left), and how far above or below it; nothing behind it (the user's: "you
** can't really see" it), though the senses mark the squares beside it there.  A room's size it knows when it's seen
** the whole of it; a ceiling when the light reaches it.  At a glance (the user's), in general words: a small hall, a
** long gallery, a high ceiling, a low, rough one (ceiling_glance); its figures (its size in feet, its
** ceiling's heights) only on a close look (look closely, examine the room: look's sight_close), and only for what it's
** seen (else its far side is lost in the dark).  What's further off is told in less detail, the further it is and the
** weaker the character's sight (the referee's: struct sight's eye): a thing too far to make out only by its size
** ("something small"), a denizen too far to tell what it is only as something there.  As a run or a sprint ends (look's
** SIGHT_HURRY), briefly: the place and its ways out, not its ceiling, its zones or its things.
**
** The words for a room and its light are the theme's (theme.h; theme.c's theme_named, added to lb).
**
** A look's words (lb_stream) are said as they're made: each sentence as the next begins (lb_out), so its first is out
** while the rest is worked out; and added at lb's end as it's kept (lbe), not looked for each time.
*/

#pragma code-name ("OV2CODE")
#pragma rodata-name ("OV2RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#define PS_OPEN_        0x01                    /* (A portal's state: open: dq0w/world.h's PS_OPEN) */

static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};
static const char* const count[5] = { "", "one", "two", "three", "four" };

static struct sight* S;

/* The words so far said, and out: lb begun again (look.c's lb_said: none a look's said already this answer) */
#define lb_out  lb_said

/* Words added to lb: a sentence begun (a look's, streamed), those before it said first */
#pragma register-vars (push, on)
static void __fastcall__ add (const char* s)
{
    register const char* q = s;
    register char* e = lbe;

    if (*q == ' ' && lb_stream && e != lb && e[-1] == '.') {
        lb_out ();
        e = lb;
    }
    while ((*e = *q) != 0) {
        ++e;
        ++q;
    }
    lbe = e;
}
#pragma register-vars (pop)

static void __fastcall__ addn (int v)
{
    char b[8];

    add (itoa (v, b, 10));
}

/* " 6 feet higher" or " 4 feet lower" (nothing if within a couple of feet) */
static void __fastcall__ height (signed char dz)
{
    if (dz >= 3 || dz <= -3) {
        add (" ");
        addn (dz > 0 ? dz : -dz);
        add (dz > 0 ? " feet higher" : " feet lower");
    }
}

/* ---- A room */

/* Seen door i one of the doors (mouths 0: but a portcullis, a window or a slit), or the passages' mouths (MOUTH),
** and not behind it */
static unsigned char __fastcall__ is (unsigned char i, unsigned char mouths)
{
    return (S->door[i].rel & MOUTH) == mouths && (mouths || S->door[i].kind == PK_DOOR) && !REL_BEHIND (S->door[i].rel);
}

/* The doors seen (mouths 0), or the passages' mouths (MOUTH) */
static void __fastcall__ doors (unsigned char mouths)
{
    unsigned char seen[8], d, k, n = 0, i;

    memset (seen, 0, sizeof seen);
    for (i = 0; i < S->ndoors; ++i) {
        if (is (i, mouths)) {
            ++seen[S->door[i].rel & 7];
        }
    }
    for (d = 0; d < 8; ++d) {
        n += seen[d] != 0;
    }
    for (d = 0, k = 0; d < 8; ++d) {
        if (!seen[d]) {
            continue;
        }
        if (!k++) {
            add (seen[d] == 1 ? (mouths ? " A passage leads off " : " There is a door ") : " There are ");
            if (seen[d] > 1) {
                add (count[seen[d] > 4 ? 4 : seen[d]]);
                add (mouths ? " passages leading off " : " doors ");
            }
        } else {
            add (k == n ? " and " : ", ");
            add (count[seen[d] > 4 ? 4 : seen[d]]);
            add (" ");
        }
        add (relname[d]);
        for (i = 0; i < S->ndoors; ++i) {       /* (The first that way's state and height) */
            if ((S->door[i].rel & 7) == d && is (i, mouths)) {
                if (!mouths) {
                    add (S->door[i].state == DOORWAY ? " (a doorway)" : S->door[i].state & PS_WRECKED ?
                         " (broken down)" : S->door[i].state & PS_OPEN_ ? " (open)" :
                         " (closed)");
                }
                if (S->door[i].dz >= 3 || S->door[i].dz <= -3) {
                    add (",");
                    height (S->door[i].dz);
                }
                break;
            }
        }
    }
    if (n) {
        add (".");
    }
}

/* The portcullises, windows and slits seen, each: "There is a barred window ahead to your left (its shutters
** open)." */
static void portals (void)
{
    unsigned char i, k, p;

    for (i = 0; i < S->ndoors; ++i) {
        k = S->door[i].kind;
        p = S->door[i].parts;
        if ((S->door[i].rel & MOUTH) || k == PK_DOOR || REL_BEHIND (S->door[i].rel)) {
            continue;
        }
        add (k == PK_PORTCULLIS ? " There is a portcullis " : k == PK_SLIT ? " There is an arrow slit " :
             p & PP_BARS ? " There is a barred window " : p & PP_GLASS ? " There is a glazed window " :
             p & PP_SHUTTERS ? " There is a shuttered window " : " There is an empty window frame ");
        add (relname[S->door[i].rel & 7]);
        if (k == PK_PORTCULLIS) {
            add (S->door[i].state & PS_WRECKED ? " (its bars forced apart)" : S->door[i].state & PS_OPEN_ ? " (raised)" :
                 " (down)");
        } else if (k == PK_WINDOW && (p & PP_SHUTTERS)) {
            add (S->door[i].state & PS_OPEN_ ? " (its shutters open)" : " (its shutters closed)");
        }
        height (S->door[i].dz);
        add (".");
    }
}

/* A trap's name, added to lb: "a scything blade, set off by a tripwire" (kind, trigger, state: world.h's TK_*,
** TT_*, TS_*: a pit that's given way an open one; overlay 10's, this one's room) */
#pragma code-name (push, "OV10CODE")
#pragma rodata-name (push, "OV10RODATA")
#pragma local-strings (push, on)
void __fastcall__ o_trap_name (unsigned char kind, unsigned char trigger, unsigned char state)
{
    static const char* const name[8] = {
        "an open pit", "a covered pit", "a covered pit, spikes in it", "a deadfall", "a scything blade",
        "a dart trap", "a gas trap", "an alarm"
    };

    if (kind <= TK_SPIKED) {
        strcat (lb, kind == TK_SPIKED ? (state & TS_SPRUNG ? "a pit, spikes in it" : name[kind]) :
            state & TS_SPRUNG ? name[TK_PIT] : name[kind]);
        return;
    }
    strcat (lb, name[kind & 7]);
    strcat (lb, trigger == TT_PLATE ? ", set off by a plate in the floor" : trigger == TT_WIRE ?
        ", set off by a tripwire" : ", set to go off as its door opens");
}
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* The traps it knows of and sees: "There is a dart trap, set off by a tripwire, ahead to your right." */
static void traps (void)
{
    unsigned char i;

    for (i = 0; i < S->ntraps; ++i) {
        if (REL_BEHIND (S->trap[i].rel)) {
            continue;
        }
        add (" There is ");
        trap_name (S->trap[i].kind, S->trap[i].trigger, S->trap[i].state);   /* (Its stub's: overlay 10) */
        lbe += strlen (lbe);
        add (S->trap[i].kind <= TK_SPIKED ? " " : ", ");
        add (S->trap[i].rel == 0xFF ? "at your feet" : relname[S->trap[i].rel & 7]);
        add (S->trap[i].state & TS_DISARMED ? " (disarmed)." : S->trap[i].kind > TK_SPIKED &&
             (S->trap[i].state & TS_SPRUNG) ? " (sprung)." : ".");
    }
}

/* The lights seen far off: "A light shows far off ahead." (the senses', struct sight's glows), and the eyes dazzled */
static void glows (void)
{
    unsigned char i;

    if (S->dazzled) {
        add (" The light dazzles you.");
    }
    for (i = 0; i < S->nglows; ++i) {
        add (S->glow[i].how == GL_LIGHT ? " A light shows far off " : " A faint glow shows far off ");
        add (relname[S->glow[i].rel & 7]);
        add (".");
    }
}

static void zones (void)
{
    unsigned char i;
    unsigned at;
    signed char dz;

    for (i = 0; i < S->nzones; ++i) {
        if (REL_BEHIND (S->zone[i].rel)) {
            continue;
        }
        dz = S->zone[i].dz;
        add (" ");
        at = lbe - lb;
        add (relname[S->zone[i].rel & 7]);
        lb[at] &= ~0x20;                        /* (Its first letter a capital) */
        if (dz >= 3 || dz <= -3) {
            add (dz > 0 ? " the floor rises " : " the floor falls ");
            addn (dz > 0 ? dz : -dz);
            add (dz > 0 ? " feet to a higher part of the room" : " feet to a lower part of the room");
            add (S->zone[i].how == Z_STEPS ? ", by steps." : S->zone[i].how == Z_SLOPE ? ", in a slope." :
                 ", at a ledge.");
        } else {
            add (" the room goes on, nearly level.");
        }
    }
}

/* ---- The ceiling in general words (the user's: a high ceiling, a low, rough one), as it's seen at a glance: its kind,
** and how high it is (low, under 8 feet; high, over 12; lofty, over 20), added to lb (its record sight_copy; seen.c's
** once, overlay 7's, its room) */
static void ceiling_glance (void)
{
#define S sight_copy
    static const char* const and[4] = { " and low.", ".", " and high.", " and very high." };
    static const char* const over[4] = { " low overhead.", " overhead.", " high overhead.", " far overhead." };
    unsigned char k = S.ceil != C_ROUGH && S.ceil != C_SLOPED && S.cwall >= S.cmid ? C_FLAT : S.ceil, h;

    h = S.cmid < 8 ? 0 : S.cmid <= 12 ? 1 : S.cmid <= 20 ? 2 : 3;
    add (k == C_FLAT ? " The ceiling is flat" : k == C_BARREL ? " A barrel vault arches" : k == C_DOME ?
         " The ceiling is a dome," : k == C_CATHEDRAL ? " A pointed vault rises" : k == C_SLOPED ?
         " The ceiling slopes" : " The ceiling is rough stone");
    add (k == C_SLOPED ? (S.cmid >= S.cwall + 6 ? " steeply, high on one side and low on the other." :
         ", higher on one side.") : k == C_FLAT || k == C_ROUGH ? and[h] : over[h]);
#undef S
}

/* The ceiling: at a glance, or (a close look) its figures */
static void ceiling (void)
{
    if (S->ceil == C_UNSEEN) {
        if (!S->dark && !S->lit) {
            add (" Above, the dark hides the ceiling.");
        }
        return;
    }
    if (!sight_close) {
        ceiling_glance ();
        return;
    }
    switch (S->ceil != C_ROUGH && S->cwall >= S->cmid ? C_FLAT : S->ceil) {   /* (A vault no higher in the middle: flat) */
    case C_FLAT:
        add (" The ceiling is flat, ");
        addn (S->cmid);
        add (" feet up.");
        return;
    case C_BARREL:
        add (" A barrel vault rises to ");
        break;
    case C_DOME:
        add (" The ceiling is a dome, ");
        break;
    case C_CATHEDRAL:
        add (" A pointed vault rises high overhead, to ");
        break;
    case C_SLOPED:
        add (" The ceiling slopes from ");
        addn (S->cmid);
        add (" feet down to ");
        addn (S->cwall);
        add (".");
        return;
    default:
        add (" The ceiling is rough stone, about ");
        addn (S->cmid);
        add (" feet up.");
        return;
    }
    addn (S->cmid);
    add (S->ceil == C_DOME ? " feet at its height and " : " feet, ");
    addn (S->cwall);
    add (" at the walls.");
}

static void __fastcall__ opening (unsigned char i)
{
    unsigned char k = S->open[i].kind, rel = S->open[i].rel;

    add (rel == 0xFF ? " You stand by " : " You see ");
    add (k == O_WAY_OUT ? "the stairs up to the way out" : k == O_STAIRS_UP ? "a stairway leading up" :
         k == O_STAIRS_DOWN ? "a stairway leading down" : k == O_LADDER_UP ? "a ladder going up" :
         k == O_LADDER_DOWN ? "a ladder going down" : k == O_SHAFT_UP ? "a shaft in the ceiling" :
         k == O_SHAFT_DOWN ? "a shaft in the floor" : "a window");
    if (rel != 0xFF) {
        add (" ");
        add (relname[rel & 7]);
        height (S->open[i].dz);
    }
    add (".");
}

static void room (void)
{
    unsigned char i;
    char* art;

    if (S->dark) {
        add ("It is dark. You can see nothing, but you feel the floor of a room about you.");
        return;
    }
    if (S->size != S_PART) {                    /* (Where in it, its size and shape: slice 4.0's, part 8's) */
        art = lbe + room_where ();              /* (Its words put in lb, at its end: part 8's) */
        lbe += strlen (lbe);
    } else {
        add ("You can see part of a");
        art = lbe;
        add (" ");
    }
    theme_named (TN_ROOM, S->kind);
    lbe += strlen (lbe);                        /* (The theme's, put at lb's end) */
    if (art[0] == ' ' && strchr ("aeiou", art[1])) {
        memmove (art + 1, art, strlen (art) + 1);   /* ("an ossuary") */
        *art = 'n';
        ++lbe;
    }
    if (S->lit || !sight_light) {
        if (S->lit) {
            add (", ");
            theme_named (TN_LIGHT, 0);
            lbe += strlen (lbe);
        }
        add (".");
    } else {
        add (", by the light of your ");
        add (sight_lamp);
        add (".");
    }
    if (sight_close && S->size != S_PART) {      /* (Its figures, a close look's: what it's seen of it) */
        add (" It is ");
        addn (SQ_FT * S->rw);
        add (" feet by ");
        addn (SQ_FT * S->rh);
        add (".");
    } else if (sight_close) {
        add (S->lit ? " You haven't seen all of it yet." : " Its far side is lost in the dark.");
    }
    if (S->up_ft) {                             /* (The slope's words, the user's: a slight rise, a rise, steep) */
        add (S->up_ft <= 2 ? " The floor rises gently " : S->up_ft <= 5 ? " The floor rises " : S->up_ft <= 9 ?
             " The floor rises steeply " : " The floor climbs sharply ");
        add (relname[S->up_rel & 7]);
        add (".");
    }
    if (!(S->first & SIGHT_HURRY)) {                            /* (A run's or a sprint's look: the place and its ways out) */
        ceiling ();
        zones ();
    }
    doors (0);
    doors (MOUTH);
    portals ();
    for (i = 0; i < S->nopen; ++i) {
        if (!REL_BEHIND (S->open[i].rel)) {
            opening (i);
        }
    }
}

/* ---- A passage */

static void passage (void)
{
    unsigned char i, n;
    signed char dz;

    for (i = n = 0; i < S->ndoors; ++i) {     /* (The doors beside it, but behind it) */
        n += !REL_BEHIND (S->door[i].rel);
    }
    add (S->dark ? "It is dark; you feel your way. " : "");
    add (S->nways <= 1 && n ? "The passage comes to a door; it goes back " : S->nways <= 1 ?
         "The passage ends here; it goes back " : S->nways == 2 ? "You are in a passage. It goes " :
         "The passage forks here. It goes ");
    for (i = 0; i < S->nways; ++i) {
        if (i) {
            add (i + 1 == S->nways ? " and " : ", ");
        }
        add (relname[S->way[i].rel & 7]);
        dz = S->way[i].dz;
        if (S->way[i].steps) {
            add (dz > 0 ? " up steps" : " down steps");
        } else if (dz >= 1 || dz <= -1) {
            add (dz == 1 ? ", rising gently" : dz == -1 ? ", falling gently" : dz > 0 ? ", rising" : ", falling");
        }
    }
    add (".");
    doors (0);                                  /* (A door beside it) */
    portals ();
}

/* What the character perceives, in words, into lb (from look.c's copy of the hub's record) */
void o_words (void)
{
    *(lbe = lb) = 0;
    lbn = 0;
    S = &sight_copy;
    if (sight_close & 16) {                     /* (The traces read: the world's words, I_TRACE's, the UI bank's
                                                **   tread) */
        hub_mem (UI_BANK, lb, UIREC->trtext, sizeof UIREC->trtext);
        lbe = lb + strlen (lb);
        if (lb_stream) {
            lb_out ();
        }
        return;
    }
    if (sight_close & 8) {
        focus_words ();                         /* (A look one way's: what it sees that way, overlay 11's) */
        return;
    }
    if (S->where == W_ROOM) {
        room ();
    } else {
        passage ();
    }
    traps ();
    glows ();
    seen_words ();                              /* (The things and the denizens it sees: overlay 12's) */
    if (lb_stream) {
        lb_out ();                              /* (The last of it said) */
    }
}

/* A thing worked (the things' IO_FILL, IO_SHUT, IO_HIDE: its words said), its time (the request's x): quietly; a
** timed work's (HOUSE.md's long tasks: the reply's qty its long task's kind, item its thing's kind), its first words (a
** long task's begun, task.c's) and a long task of the UI's (I_WAIT's kind), its command kept (the UI bank's) and
** carried out again as its time's up (main.c's reach 3: its end, 4, the things' IO_DONE, the test made then); done,
** nothing more */
static void __fastcall__ worked (unsigned char action)
{
    unsigned char k, n;
    unsigned t;

    hub_mem (0, &k, &HUB->ireq.qty, 1);
    hub_mem (0, &n, &HUB->ireq.item, 1);
    hub_mem (0, &t, &HUB->ireq.x, 2);
    if (action != IR_OK || !k || reach == 4) {
        msg (lb);
        if (action == IR_OK && reach != 4) {
            intend2 (I_WAIT, t, 1);
        }
        return;
    }
    task_words (TE_BEGUN | k << 8, n |= 0x80);
    hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
    reach = 3;
    intend2 (I_WAIT, t, k << 8 | (n & 0x7F) << 1 | 1);
}

/* The commands that want nothing but their action, past main.c's chores (its default's: part 2 has no room): 1 if
** it was one of them, carried out; each slice's own cases here, its work in its own part or overlay */
unsigned char __fastcall__ o_more (unsigned char action)
{
    unsigned char d = reach == 4 ? IO_DONE : 0;    /* (A timed work's end: main.c's reach) */

    switch (action) {
    case VB_FILL_OBJECT:                        /* (A light renewed, slice 4.1's: "fill the lantern", "dip the torch in
                                                **   the oil", the things' IO_FILL; its time, quietly: a timed work) */
        worked (othings (IO_FILL, cmd.obj.word, cmd.other.word, d));
        break;
    case VB_OBSERVE_IN:                         /* (Looked into a thing, onto it: "look in the sack", "look on the */
                                                /*   table", "look into the well", the user's, 2026-10-21; the */
                                                /*   world's IO_INSIDE; a way, "look into the passage", a look */
                                                /*   that way, way.c's) */
        if (cmd.obj.kind) {
            look_way (VB_OBSERVE_THROUGH);
            break;
        }
        action = othings (IO_INSIDE, cmd.obj.word, "", 0);
        if (action == IR_NOT_HERE) {
            reach_go ();                        /* (Out of reach: walked over to first, then again) */
        } else if (action != IR_OK) {
            sayf ("\nYou see no %s here.", cmd.obj.word);
        } else {
            msg (lb);
            intend2 (I_WAIT, 30, 1);            /* (A look's three seconds) */
        }
        break;
    case VB_OPEN_OBJECT:                        /* (A thing that closes, worked: "open the chest", "untie the */
    case VB_CLOSE_OBJECT:                       /*   sack", "pick the chest's lock", "force the chest", the */
    case VB_UNLOCK_OBJECT:                      /*   user's; the world's IO_SHUT, a lid, a tie, a stopper, a */
    case VB_FORCE_OBJECT:                       /*   lock; none of that name, a door's work: main.c's, 0) */
    case VB_LOCK_OBJECT:
        if (!cmd.obj.word[0] || cmd.obj.kind) {
            return 0;
        }
        action = othings (IO_SHUT, cmd.obj.word, "", (action == VB_LOCK_OBJECT ? PA_LOCK : cmd.aux) | d);
        if (action == IR_NONE) {
            if (cmd.action != VB_LOCK_OBJECT) {
                return 0;
            }
            sayf ("\nYou see no %s here.", cmd.obj.word);
        } else if (action == IR_NOT_HERE) {
            reach_go ();
        } else {
            worked (action);                    /* (Its lock picked, it forced: a timed work) */
        }
        break;
    case VB_HIDE:                               /* (A thing hidden, begun: main.c's hide, the things' IO_HIDE) */
        worked (IR_OK);
        break;
    case VB_TEND:                               /* (First aid: part 12's, slice 4.3's) */
        tend ();
        break;
    default:
        return 0;
    }
    return 1;
}
