/*
** task.c - (dq0) the long tasks' words (HOUSE.md's long tasks, slice 4.0: the world keeps them, the hub's pc_task the
** character's): what came of one (E_TASK: stopped, asked, carried on, refused), a walk's beats as it goes (E_BEAT),
** the task a stop names ("stop digging": its LT_*), a question's answer read, and a line typed as the character's
** busy carried out at once or held (fighting on's: a command taking over); a light burning low and going out (E_TASK's
** too), and the status line's light left (slice 4.1's); robbed as it lay helpless (E_TASK's too); a timed work's first
** words, a battering's as it goes, a bare part stopped short of hurting itself (the user's, 2026-10-22).  Overlay 3's
** (the stop's word, a question's answer, the state's words, a wait's length, a line typed as it's busy and the
** game paused or kept overlay 7's).
*/

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "tell.h"

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

static const char* const doing[] = {           /* (By LT_*) */
    "doing anything", "walking", "climbing", "digging", "searching", "waiting", "sleeping", "talking", "battering",
    "fighting", "travelling", "studying the mechanism", "disarming the trap", "picking the lock", "forcing", "heaving at",
    "bending the bars", "winding the winch", "working the pane free", "tracking", "sniffing the air", "filling",
    "hiding", "tending the wound", "taking off", "taking off", "reaching into", "holding open", "getting out", "putting on",
    "letting fall", "letting fall", "putting away", "blowing out", "blowing out", "lighting", "setting down", "taking up", "sharpening",
    "striking a spark", "getting out", "putting away", "taking up", "setting down", "blowing out", "lighting", "dipping"
};
#define NDOING          (sizeof doing / sizeof doing[0])

/* An enemy the character's met (E_TASK's b, TE_ENGAGED's, TE_FAR's: its entry plus 1), as it's said: "the giant
** rat" (the first letter a capital, cap), or "something" if it isn't seen */
static char fn[20];
static const char* __fastcall__ foe (unsigned char b, unsigned char cap)
{
    static char at[12], seen[12];
    unsigned char k, n;

    hub_mem (0, at, HUB->actor[(b - 1) & 7].name, sizeof at);
    hub_mem (SIGHT_BANK, &n, &SIGHT->ndens, 1);
    for (k = n; k--; ) {
        hub_mem (SIGHT_BANK, seen, SIGHT->den[k].name, sizeof seen);
        if (!strcmp (seen, at)) {
            break;
        }
    }
    strcpy (fn, k == 0xFF ? "something" : at[0] >= 'A' && at[0] <= 'Z' ? "" : "the ");
    if (k != 0xFF) {
        strcat (fn, at);
    }
    if (cap && fn[0] >= 'a') {
        fn[0] -= 'a' - 'A';
    }
    return fn;
}

/* A light's kind's name (E_TASK's b, TE_LOW's, TE_OUT's: the catalogue's), in fn; and whether it's a
** plural (K_PLURAL: candle stubs) */
static unsigned char __fastcall__ lamp (unsigned char b)
{
    unsigned char f;

    hub_mem (KIND_BANK, fn, CATALOGUE[b].name, sizeof CATALOGUE[0].name);
    hub_mem (KIND_BANK, &f, &CATALOGUE[b].flags, 1);
    return f & K_PLURAL;
}

#define FW_BATTER       0xFF                    /* (felt_words' own: a battering's first words, struck with the thing
                                                **   named, its what times 256) */
#pragma data-name (push, "OV3RODATA")          /* (Its words made in the overlay's bank: the main RAM's none) */
static char nb[20] = "", dn[36] = "", pn[24] = "";
#pragma data-name (pop)

/* A journey's place (E_TASK's b, I_TRAVEL's: a name's entry, TRV_IN, TRV_KIND | a room's kind; TRV_ON aside), as it's
** said: "the Mausoleum" (the name as given), "the kitchen" (the theme's word); the way in "where you came in" */
static const char* __fastcall__ place (unsigned char b)
{
    char t[NAME_LEN];                           /* (On the C stack: the hub's bank in the window as it's copied, not
                                                **   the overlay's) */
    b &= ~TRV_ON;
    if (b == TRV_IN) {
        return "where you came in";
    }
    *pn = 0;
    if (b & TRV_KIND) {
        b &= ~TRV_KIND;
        strcpy (pn, "the ");
        hub_mem (THEME_BANK, t, b < 8 ? THEME->room[b] : THEME->uses[(b - 8) % TUSES], 16);
        t[15] = 0;
    } else {
        hub_mem (THEME_BANK, t, NAMES_AT[b % TNAMES].name, NAME_LEN);
        t[NAME_LEN - 1] = 0;
    }
    strcat (pn, t);
    return pn;
}

/* What a long task's done to (the long task's what, n: hub.h's), its words: "the door", "the chest"; "it" */
static const char* __fastcall__ noun (unsigned char n)
{
    static const char* const pk[3] = { "the door", "the window", "the portcullis" };    /* (By PK_*) */

    if (n & 0x80) {
        hub_mem (KIND_BANK, fn, CATALOGUE[(n & 0x7F) | what_hi].name, sizeof CATALOGUE[0].name);   /* (A thing's kind's name: the
                                                **   catalogue's: past 127, the UI's top bit) */
        strcpy (nb, "the ");
        strcat (nb, fn);
        return nb;
    }
    n = (n & 15) - 1;
    return n < 3 ? pk[n] : "it";
}

/* A long task of kind k as it's said ("picking the lock", "kicking the door"), n its what (the striker, its thing) */
static const char* __fastcall__ doing_of (unsigned char k, unsigned char n)
{
    static const char* const struck[4] = { "battering", "punching", "kicking", "butting" };    /* (By SK_*) */

    strcpy (dn, k == LT_BATTER ? struck[n >> 4 & 3] : doing[k < NDOING ? k : 0]);
    if (n && (k == LT_BATTER || k == LT_FORCE || k == LT_HEAVE || k == LT_FILL || k == LT_HIDE || (k >= LT_PACKOFF && k != LT_KINDLE))) {
        strcat (dn, " ");
        strcat (dn, noun (n));
    }
    return dn;
}

/* What came of the character's long task: E_TASK's w (TE_*, its kind times 256); named, the kind its stop named (b),
** or the foe (TE_FIGHT's), or the health (TE_ASK_HURT's), or a light's kind (TE_LOW's, TE_OUT's), or what it was done
** to (a stop's, a question's, a timed work begun: the long task's what).  1 if a question's put; 2 one answered yes
** unasked (dq0 -a's, the tests': not put); 4 fighting on begun (the character busy) */
static const char* const fetched[8] = {         /* (TE_FETCHED's: FT_*) */
    "\n%s picks up the %s.", "\n%s hands you the %s.", "\n%s can't find the %s where it lay.",
    "\n%s can't carry the %s.", "\n%s has its hands full: it can't take the %s.",
    "\n%s sets the %s down at your feet.", "\n%s takes the %s.", "\n%s can't get to the %s."
};

unsigned char __fastcall__ o_task_words (unsigned w, unsigned char named)
{
    unsigned char k = w >> 8, p = (unsigned char) w, n = named, one = 0;
    const char* d;
    char t[20];                                 /* (On the C stack: a thing's name) */

    if (p == TE_OUT) {
        one = k;                                /* (Another of its name burning still; one it sees: the world's) */
        k &= 0x3F;
    }

    if (p == TE_NOT || p == TE_ENGAGED || p == TE_OUT) {
        hub_mem (0, &n, &HUB->pc_task.what, 1);     /* (Its b another's: what it's done to the hub's) */
    }
    d = doing_of (k, n);
    if (p <= TE_NEAR || p == TE_ENGAGED || (p == TE_OUT && k)) {
        parts_op (PO_STOP);                     /* (The rest of its line with it: parts.c's lists, 2026-11-04) */
        if (k != LT_FIGHT || reach < 5) {          /* (A fight begun as the character came free, its line's first step in hand: that stopped, not the step) */
            reach = 0;                          /* (A walk over to a thing stopped: its command dropped; a thing's
                                                **   timed work stopped: its end dropped, words.c's) */
        }
    }
    k = p;
    if (k == TE_ASK_SHORT && w >> 8 != LT_DIG) {  /* (A dig's nothing to show: its words its own, E_DIG's.  A battering's: a bare part's blow that would hurt it stopped short
                                                **   of it, once a door; told, dq0 -a's too, then asked) */
        p = named >> 4 & 3;
        sayf ("\nYou %s ", p == SK_KICK ? "kick" : p == SK_HEAD ? "butt" : "punch");
        sayf ("%s with your bare %s, but stop short of hurting yourself.", noun (named),
              p == SK_KICK ? "feet" : p == SK_HEAD ? "head" : "hands");
    }
    if (k == TE_ASK_SEEN || k == TE_ASK_TIRED || k == TE_ASK_HEARD || k == TE_ASK_HURT || k == TE_ASK_SHORT) {
        if (autogo) {
            return 2;
        }
        parts_op (PO_STOP);                     /* (Asked: the rest of a list isn't its answer, and goes as a stop's does) */
    }
    switch (k) {
    case TE_BODY:                               /* (What time did to its body: the referee's words, part 12's; 8 dead
                                                **   of it, slice 4.3's) */
        return body_words ();
    case TE_STOPPED:
    case TE_HIT:
    case TE_NEAR:
        sayf ("\nYou stop %s.", d);
        break;
    case TE_ASK_SEEN:
    case TE_ASK_HEARD:
        sayf ("\nKeep %s?", d);
        break;
    case TE_ASK_TIRED:
        sayf ("\nYou're tiring.  Keep %s?", d);
        break;
    case TE_NOT:
        sayf ("\nYou aren't %s: you're %s.", doing[named < NDOING ? named : 0], d);
        break;
    case TE_IDLE:
        msg ("You aren't doing anything to stop.");
        if (reach >= 5) {
            reach_said (TE_IDLE, 0);            /* (Between two steps of the reach: they stop, part 19's) */
        }
        break;
    case TE_CANT:
        msg ("There's no stopping part way up: you cling on, and climb.");
        break;
    case TE_ENGAGED:                            /* (HOUSE.md's engagement: "The giant rat engages you: you stop
                                                **   walking."; engaged already, "You're engaged with the giant rat.") */
        if (named & TE_STILL) {
            sayf ("\nYou're engaged with %s", foe (named, 0));
        } else {
            sayf ("\n%s engages you", foe (named, 1));
        }
        if (k) {
            sayf (": you stop %s.", d);
        } else {
            say (".");
        }
        break;
    case TE_FAR:
        sayf ("\nYou can't get near enough to %s.", foe (named, 0));
        break;
    case TE_AWAY:                               /* (The engage initiative test's: an enemy newly engaged the quicker,
                                                **   its tactical advantage a disengage) */
        sayf ("\nQuicker, %s backs away from you.", foe (named, 0));
        break;
    case TE_ASK_HURT:                           /* (Fighting on's: its health's band worse, H_BADLY, H_CRITICAL) */
        sayf ("\nYou're %s hurt.  Keep fighting?", named == H_BADLY ? "badly" : "critically");
        break;
    case TE_FIGHT:                              /* (Fighting on: begun on a hostile engaging it, or by an attack, its
                                                **   blow's words enough; turned to the next) */
        if (named & TE_STILL) {
            sayf ("\nYou turn to %s.", foe (named, 0));
        } else if (!(named & TE_QUIET)) {
            sayf ("\nYou fight back against %s.", foe (named, 0));
        }
        return 4;
    case TE_BREAK:                              /* (A disengage, the book's: w's high byte 0 backing away, 1 a flight,
                                                **   2 a run from one not engaging it) */
        sayf ("\nYou %s %s.", !(w >> 8) ? "back away from" : w >> 8 == 1 ? "break away from" : "run from",
            foe (named, 0));
        break;
    case TE_FREE:                               /* (None: b 0 not engaged, 1 nothing near to flee, 2 no way back) */
        msg (named == 2 ? "There's no way back." : named ? "You see nothing to flee from." : "You aren't engaged.");
        break;
    case TE_NO_FOE:                             /* (b 1: in shock, in no state to fight on; 2, a missile's fight) */
        msg (named == 1 ? "You're in no state to fight." : named ? "No foe is left in your sights." :
            "No foe is left beside you.");
        break;
    case TE_NO_SHOT:                            /* (The missiles', slice 4.7's: overlay 4's, shots.c) */
    case TE_LOADED:
    case TE_GATHERED:
    case TE_READIED:
        shot_words (w, named);
        break;
    case TE_LOW:                                /* (A light it carries in its last ten minutes: slice 4.1's; not in its sleep) */
        hub_mem (0, &p, &HUB->pc_task.kind, 1);
        if (p == LT_SLEEP) {
            break;
        }
        p = lamp (named);
        sayf ("\nYour %s %s burning low.", fn, p ? "are" : "is");
        break;
    case TE_OUT:                                /* (Its fuel burnt: its own, its only light, its task stopped with
                                                **   it; or one it sees lying there, TO_SEEN) */
        p = lamp (named);
        if (one & TO_SEEN) {                    /* (One lying about or in a sconce: told once an answer, a long rest's two
                                                **   the soak's, 2026-11-04; harm_told's bit, the prone's it doesn't read) */
            if (harm_told & 1) {
                break;
            }
            harm_told |= 1;
        }
        if (!(one & TO_SEEN) && !p && (one & TO_ANOTHER)) {     /* (Another of its name burning still, the world's: "One of
                                                **   your torches", not "Your torch" twice) */
            sayf ("\nOne of your %s%s gutters and goes out", fn, strchr ("sxh", fn[strlen (fn) - 1]) ? "es" : "s");
        } else {
            sayf ("\n%s %s gutter%s and go%s out", one & TO_SEEN ? "The" : "Your", fn, p ? "" : "s", p ? "" : "es");
        }
        if (w >> 8 & 0x3F) {                    /* (Its long task stopped with it: k is the code here) */
            sayf (": you stop %s.", d);
        } else {
            say (".");
        }
        break;
    case TE_FETCHED:                            /* (An ally's errand, a gift taken: slice 4.8's) */
        lamp (w >> 8);
        strcpy (t, fn);
        sayf (fetched[(n >> 4) & 7], foe (n & 15, 1), t);
        break;
    case TE_ROBBED:                             /* (Helpless, robbed: a robber's way, the denizens') */
        sayf ("\n%s goes through your things as you lie helpless, and takes what it finds.", foe (named, 1));
        break;
    case TE_PACKED:                             /* (The reach's: part 19's) */
    case TE_FUMBLE:
    case TE_KINDLED:
        reach_said (w, named);
        break;
    case TE_LEARNT:                             /* (A skill opened as it's used: Drawing, the first map, slice 4.2's) */
        msg ("You start to learn drawing.");
        break;
    case TE_BEGUN:                              /* (A timed work, a battering, begun: its first words at once, its
                                                **   outcome as its time's done, HOUSE.md's long tasks; the user's,
                                                **   2026-10-22) */
        p = w >> 8;
        if (p >= LT_PACKOFF) {
            reach_said (w, named);              /* (A step of the reach: part 19's) */
            break;
        }
        if (p == LT_TRAVEL) {                   /* (A journey: slice 4.2's) */
            sayf ("\nYou head back toward %s.", place (named));
            break;
        }
        if (p == LT_BATTER && (n & SK_WITH)) {
            felt_words (FW_BATTER | n << 8);    /* (Struck with the thing named: "with the bar", overlay 7's room) */
            break;
        }
        sayf (p == LT_STUDY ? "\nYou study the mechanism." : p == LT_TRACK ? "\nYou cast about for tracks." :
              p == LT_SNIFF ? "\nYou sniff the air." : p == LT_CLIMB ? "\nYou set to climbing through the window." :
              "\nYou set to %s.", d);
        break;
    case TE_ASK_SHORT:
        say ("  Keep at it?");
        break;
    case TE_THERE:                              /* (A journey's: slice 4.2's, HOUSE.md's travel; b its place, | TRV_ON
                                                **   under way) */
        p = (named & ~TRV_ON) == TRV_IN;
        sayf (named & TRV_ON ? "\nYou're back %s%s." : "\nYou're %s%s already.", p ? "" : "in ", place (named));
        break;
    case TE_FORGOT:
        sayf (named & TRV_ON ? "\nYou no longer remember the way to %s." : "\nYou don't remember the way to %s.",
              place (named));
        break;
    case TE_UNSURE:
        msg ("You've lost your bearings: you can't tell which way to go.");
        break;
    case TE_ASTRAY:
        sayf ("\nYou've lost your way: you can't find %s.", place (named));
        break;
    case TE_ELSEWHERE:
        if ((named & ~TRV_ON) == TRV_IN) {
            msg ("You came in on a level above this one.");
        } else {
            place (named);
            if (pn[0] >= 'a' && pn[0] <= 'z') {
                pn[0] -= 'a' - 'A';
            }
            sayf ("\n%s isn't on this level.", pn);
        }
        break;
    }
    return k == TE_ASK_SEEN || k == TE_ASK_TIRED || k == TE_ASK_HEARD || k == TE_ASK_HURT || k == TE_ASK_SHORT;
                                                /* (1: a question put) */
}

/* (Overlay 7's, overlay 3's room the timed work's: the state's words, a wait's length, the stop's word, a question's
** answer, a line typed as it's busy, the game paused or kept) */
#pragma code-name (push, "OV7CODE")
#pragma rodata-name (push, "OV7RODATA")
#pragma local-strings (push, on)               /* (Their words with them) */

/* The character's state after a harm, its words (shock, bleeding, a leg or an arm useless: rs.h's C_*, the low
** byte): each once an answer, then "still" (a second harm's: a door battered, its trap after) */
void __fastcall__ o_state_words (unsigned char cond)
{
    static const unsigned char c_[] = { C_STUNNED, C_BLEEDING, C_LEG_USELESS, C_ARM_USELESS };
    static const char* const now_[] = { " You are in shock.", " You are bleeding badly.", " Your leg won't bear you.",
                                        " Your arm hangs useless." };
    static const char* const still_[] = { " You are still in shock.", " You are still bleeding badly.",
                                          " Your leg still won't bear you.", " Your arm still hangs useless." };
    unsigned char i;

    for (i = 0; i < sizeof c_; ++i) {
        if (cond & c_[i]) {
            say (harm_told & c_[i] ? still_[i] : now_[i]);
        }
    }
    harm_told |= cond & ~1;
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* The status line's right side so far (s), and after it: the bar of a long task's part done, ten cells; a task of
** no length its words (a fight's, its foe: "Fighting the giant rat"); then the light the character carries, its
** minutes left (slice 4.1's player aid, the world's words, the hub's pc_left: "torch 40m", "lantern 5h59m"), if
** there's room for it beside the referee's words, the facing and the clock (the status line's 72) */
void __fastcall__ o_bar_text (char* s, unsigned done, unsigned len)
{
    unsigned char i, b;
    char* e;
    char t[sizeof HUB->pc_left];                /* (On the C stack: the main RAM's none) */

    hub_mem (0, &b, &HUB->pc_task.kind, 1);
    hub_mem (0, &i, &HUB->pc_task.foe, 1);
    if (b && !len && i) {
        strcat (s, "Fighting ");
        strcat (s, foe (i, 0));
        strcat (s, "  ");
    } else if (b && len) {
        hub_mem (0, &i, &HUB->pc_going, 1);
        if ((b == LT_WALK || b == LT_TRAVEL) && (i & GOING_DARK)) {
            strcat (s, (i & 0x7F) <= GA_SLOW + 1 ? "by feel " : "blind ");   /* (In the dark: feeling its way, or
                                                **   not, said outright) */
        }
        b = done >= len ? 10 : (unsigned char) ((unsigned long) done * 10 / len);
        e = s + strlen (s);
        *e++ = '[';
        for (i = 0; i < 10; ++i) {
            *e++ = i < b ? '#' : '.';
        }
        strcpy (e, "]  ");
    }
    hub_mem (0, t, HUB->pc_left, sizeof t);     /* (Its words the world's, as it burns: "" none, or none that burns
                                                **   down) */
    if (*t && strlen (s) + strlen (t) <= 32) {  /* (Room left for the referee's 15, the facing's 13, the clock's 9) */
        strcat (s, t);
        strcat (s, "  ");
    }
    hub_mem (0, &i, &HUB->pc_want, 1);          /* (Hunger and thirst once they matter: slice 4.9's) */
    if (i && strlen (s) <= 24) {
        strcat (s, i >= 0x10 ? "thirsty  " : "hungry  ");
    }
}

#pragma code-name (push, "OV8CODE")       /* (A walk's beats, a battering's: once overlay 8's, linked in overlay 3,
                                                **   overlay 8's room part 1's, 2026-10-27; in overlay 16 since
                                                **   2026-11-01, overlay 3's room, dq0.cfg's) */
#pragma rodata-name (push, "OV8RODATA")
#pragma local-strings (push, on)

/* A walk's beat as it goes (E_BEAT's w: WB_*, its side times 256; a passage's bend, its run's new bearing in
** sixteenths | 0x10, the world's, by the north the character believes in: "The passage bends away north-northeast",
** HOUSE.md's a passage's heading) */
void __fastcall__ o_beat_words (unsigned w)
{
    unsigned char r, sk;

    switch ((unsigned char) w) {
    case WB_QUIET:
        hub_mem (SIGHT_BANK, &r, &SIGHT->dark, 1);
        msg (r ? "You feel your way on." : "You go on down the passage.");
        break;
    case WB_TURN:
        hub_mem (SIGHT_BANK, &sk, &SIGHT->dark, 1);
        if (sk) {                               /* (In the dark: as its feet go, no passage's name, no bearing) */
            r = w >> 8;
            msg (r & 0x10 ? "The way bends." : r == WS_LEFT ? "The way turns left." : "The way turns right.");
            break;
        }
        if ((r = w >> 8) & 0x10) {
            hub_mem (0, &sk, &HUB->pc_skew, 1);
            r = (r + (sk << 1)) & 15;
            sayf ("\nThe passage bends away %s%s%s.", head_name[r & 1 ? ((r >> 1) + 1) & 6 : r >> 1], r & 1 ? "-" : "",
                r & 1 ? head_name[(r >> 1) | 1] : "");
            break;
        }
        msg (r == WS_LEFT ? "The passage turns left." : "The passage turns right.");
        break;
    case WB_DOOR:
        msg ("You go through a doorway.");
        break;
    case WB_UP:
        msg ("The way rises.");
        break;
    case WB_DOWN:
        msg ("The way falls away.");
        break;
    case WB_CROSS:
        hub_mem (SIGHT_BANK, &r, &SIGHT->dark, 1);
        if (!r)                                 /* (In the dark, no room's told) */
        tell (T_YOU_CROSS_THE);                 /* (A room crossed to a door, a passage: "You cross the room.") */
        break;
    case WB_TELLS:                              /* (A battering's as it goes: the user's, 2026-10-22) */
        msg ("Your blows begin to tell on it.");
        break;
    case WB_GIVES:
        msg ("It's starting to give.");
        break;
    case WB_ON:
        msg ("You keep at it.");
        break;
    default:
        felt_words (w);                         /* (The dark's: overlay 7's) */
        break;
    }
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* (Overlay 7's, its room: the dark's words, 2026-10-22) */
#pragma code-name (push, "OV17CODE")            /* (Overlay 17's since 2026-11-02: overlay 7's room, what can be named) */
#pragma rodata-name (push, "OV17RODATA")
#pragma local-strings (push, on)

/* A walk in the dark's beat (E_BEAT's w, WB_FEEL ... WB_SHORT: HOUSE.md's walking in the dark): set out on, feeling
** its way or at a pace said outright; what its hands met; its foot on a thing in its way (the catalogue's kind), over
** it fallen or stumbling; what it ran into; an edge gone over, stopped short at.  And a blow at a denizen with a soft
** thing (WB_SWAT: its actor plus 1, the thing the hub's pc_strike's), one attempt; a battering's first words, struck
** with the thing named (FW_BATTER, task_words': its what), "You set to battering the door with the bar." (the user's,
** 2026-10-22) */
/* A walk over for a command just done (look's LOOK_NEW, E_ARRIVE's): the things and the denizens it sees that it didn't
** as the command began (UIREC's before, resolve.c's), and not the thing it came for; nothing else: the command's outcome
** comes next (the user's, 2026-11-02) */
static void new_words (void)
{
    struct sight* s = &sight_copy;
    unsigned before[SEE_ITEMS + SEE_DENS];      /* (On the C stack: the hub's bank in the window as it's copied) */
    unsigned char i, j, k, n;

    hub_mem (UI_BANK, before, UIREC->before, sizeof before);
    for (i = n = 0, j = 0; i < s->nitems; ++i) {
        for (k = 0; k < SEE_ITEMS && before[k] != s->item[i].n; ++k) {
        }
        if (s->item[i].hid & 2 ? j : k == SEE_ITEMS && !(cmd.obj.word[0] && strstr (s->item[i].name, cmd.obj.word))) {
            memcpy (&s->item[n++], &s->item[i], sizeof s->item[0]);
            j = 1;                              /* (One on the surface before it told with that, and only so) */
        } else {
            j = 0;
        }
    }
    s->nitems = n;
    for (i = n = 0; i < s->ndens; ++i) {
        for (k = 0; k < SEE_DENS && before[SEE_ITEMS + k] != *(unsigned*) s->den[i].name; ++k) {
        }
        if (k == SEE_DENS) {
            memcpy (&s->den[n++], &s->den[i], sizeof s->den[0]);
        }
    }
    s->ndens = n;
    lbn = 1;                                    /* (Said as a look's are) */
    *(lbe = lb) = 0;
    seen_words ();
    if (lb_stream) {
        lb_said ();
    }
}

/* In the dark, what its hands and feet can tell (the look's, every walk's end; the user's, 2026-11-02: HOUSE.md's walking
** in the dark): "You feel a wall ahead and to your left; nothing to your right."  No name for the place, no way told,
** unless it's been on this square before and not forgotten it ("This should be the kitchen.") */
static void touch_words (void)
{
    static const char* const kind_w[4] = { "nothing", "a wall", "a door", "iron bars" };
    static const char* const side_w[3] = { "ahead", "to your left", "to your right" };
    static const unsigned char order_[4] = { 1, 2, 3, 0 };
    struct sight* s = &sight_copy;
    unsigned char c = s->cmid, i, k, n, m, said = 0;

    hub_mem (0, &k, (char*) &HUB->pcsum.cond + 1, 1);   /* (C_BLINDED, its high byte: the window's not the hub's here) */
    strcat (lb, k & C_BLINDED >> 8 ? "You can't see. You feel " : "It is dark. You feel ");
    for (i = 0; i < 4; ++i) {
        k = order_[i];
        for (n = m = 0; m < 3; ++m) {
            n += TOUCHED (c, m) == k;
        }
        if (!n) {
            continue;
        }
        if (said++) {
            strcat (lb, "; ");
        }
        strcat (lb, kind_w[k]);
        for (m = 0; m < 3; ++m) {
            if (TOUCHED (c, m) == k) {
                strcat (lb, " ");
                strcat (lb, side_w[m]);
                if (--n) {
                    strcat (lb, n == 1 ? " and" : ",");
                }
            }
        }
    }
    strcat (lb, ".");
    lbe = lb + strlen (lb);
    if (s->cwall) {
        if (s->where == W_ROOM) {
            strcat (lb, " This should be the ");
            lbe = lb + strlen (lb);
            theme_named (TN_ROOM, s->kind);
            strcat (lb, ".");
        } else {
            strcat (lb, " You've been along this passage.");
        }
        lbe = lb + strlen (lb);
    }
}

void __fastcall__ o_felt_words (unsigned w)
{
    static const char* const met[FA_PIT + 1] = {
        "Your hands find no wall within reach that way.", "Your hands meet a wall.", "You feel a door before you.",
        "Your hands meet iron bars.",
        "The floor falls away before you.", "The floor rises steeply before you.",
        "The floor falls away before you: the edge of a pit."
    };
    static const char* const into[FA_PIT + 1] = { "", "a wall", "a door", "iron bars", "", "a rise in the floor", "" };
    static const char* const gait[4] = { "walk", "jog", "run", "sprint" };
    unsigned char b = w >> 8, k = b & 7, g = (b >> 4) - GA_WALK & 3, sk, r_;

    if (w == FW_NEW) {
        new_words ();
        return;
    }
    if (w == FW_TOUCH) {
        touch_words ();
        return;
    }
    switch ((unsigned char) w) {
    case WB_SIDES:                              /* (A dark walk, as what its hands find on either side changes: its low
                                                **   four bits now, left then right; its high four as they were) */
        for (k = 0; k < 2; ++k, b >>= 2) {
            sk = (b >> 4) & 3;
            r_ = b & 3;
            if (sk != r_) {
                if (!r_) {
                    sayf ("\nThe wall to your %s ends.", k ? "right" : "left");
                } else if (r_ == 2) {
                    sayf ("\nYou feel a door to your %s.", k ? "right" : "left");
                } else if (sk != 2) {
                    sayf ("\nA wall begins to your %s.", k ? "right" : "left");
                }
            }
        }
        return;
    case WB_FEEL:
        hub_mem (0, &sk, &HUB->pc_skew, 1);     /* (The way by the north it believes in: slice 4.2's) */
        k = (k + sk) & 7;
        if (reach == 2) {                       /* (Its command again, walked over first: told as the walk began) */
            return;
        }
        if (b >> 4) {
            sayf ("\nYou %s %s into the dark.", gait[g], head_name[k]);
        } else {
            sayf ("\nYou feel your way %s.", head_name[k]);
        }
        return;
    case WB_FELT:
        msg (met[k]);
        return;
    case WB_THING:
    case WB_TRIP:
    case WB_STUMBLE:
        hub_mem (KIND_BANK, lb, CATALOGUE[b].name, sizeof CATALOGUE[0].name);
        hub_mem (KIND_BANK, &k, &CATALOGUE[b].flags, 1);
        sayf ((unsigned char) w == WB_THING ? "\nYour foot finds something on the floor: %s %s." :
            (unsigned char) w == WB_TRIP ?
            "\nYou trip over something on the floor (%s %s), and go sprawling; you pick yourself up." :
            "\nYou stumble over something on the floor (%s %s), but keep your feet.",
            k & K_PLURAL ? "some" : *lb == 'a' || *lb == 'e' || *lb == 'i' || *lb == 'o' || *lb == 'u' ? "an" : "a", lb);
        return;
    case WB_HIT:
        sayf (g ? "\nYou %s headlong into %s!" : "\nYou %s into %s.", gait[g], into[k]);
        return;
    case WB_OFF:
        msg (k == FA_PIT ? "The floor drops away under your feet: a pit!" : "You blunder over the edge of a drop!");
        return;
    case WB_SHORT:
        msg (k == FA_PIT ? "You stop short, teetering, at the edge of a pit." :
            "You stop short, teetering, at the edge of a drop.");
        return;
    case WB_SWAT:
    case FW_BATTER:
        if ((unsigned char) w == WB_SWAT) {
            hub_mem (0, lb, HUB->actor[(b - 1) & 7].name, sizeof HUB->actor[0].name);
        } else {
            k = (b & 15) - 1;                   /* (A portal's kind plus 1: hub.h's long task's what) */
            strcpy (lb, k == PK_PORTCULLIS ? "portcullis" : k == PK_DOOR ? "door" : "window");
        }
        hub_mem (0, &k, &HUB->pc_strike, 1);
        hub_mem (KIND_BANK, lb + 32, CATALOGUE[SK_KIND (k)].name, sizeof CATALOGUE[0].name);
        sayf ((unsigned char) w == WB_SWAT ? "\nYou swat at %s%s with the %s, but it seems ineffectual." :
            "\nYou set to battering %s%s with the %s.", *lb >= 'a' ? "the " : "", lb, lb + 32);
        return;
    case WB_SPENT:                              /* (A thing used up, its kind: HOUSE.md's things used up) */
        if (b >= KI_ANTIDOTE) {
            return;                             /* (A remedy drunk: told as it's drunk, 2026-11-02) */
        }
        hub_mem (KIND_BANK, lb, CATALOGUE[b].name, sizeof CATALOGUE[0].name);
        sayf (b == KI_RATIONS || b == KI_BANDAGES ? "\nThat was the last of your %s." : b == KI_WATER ? "\nYour waterskin is empty." :
            "\nYour %s is %s.", lb, b == KI_INK ? "dry" :
            b == KI_QUILL ? "worn out" : "worn to nothing");    /* (Rations eaten: slice 4.9's) */
        return;
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "OV19CODE")            /* (Overlay 19's since 2026-11-03: overlay 1's room) */
#pragma rodata-name (push, "OV19RODATA")

/* A turn's first words (V_FACING's: the user's, 2026-11-03, "I guess the turn info is OK keeping both"): said by its way
** as it faces ("turn r", the parser's own reply, CQ_REL | n), "You turn right, to face west."; by the compass, or another's
** reason to turn, "You turn to face west." */
void __fastcall__ o_turned_says (unsigned char go)
{
    static const char* const tw[8] = { "", "half right", "right", "well to the right", "around", "well to the left", "left",
                                       "half left" };
    static const char* const st[8] = { "forward", "ahead to your right", "to your right", "back to your right", "back",
                                       "back to your left", "to your left", "ahead to your left" };
    unsigned char f, d, b[4];

    hub_mem (0, &f, &HUB->pc_face, 1);
    hub_mem (0, &d, &HUB->pc_skew, 1);
    if (go) {                                   /* (A way set out on, V_SETOFF's: 8 a step, by the way it faces; a walk, by
                                                **   the north it believes in) */
        if (go & 8) {
            sayf ("\nYou step %s.", st[(go - f) & 7]);
        } else {
            sayf ("\nYou set off %s.", head_name[(go + d) & 7]);
        }
        return;
    }
    hub_mem (CMD_BANK, b, HUB_WINDOW + CQ_CMD, 4);   /* (Its action, its actor, its flags, its way: as it was said) */
    f = (f + d) & 7;
    d = b[3] & 7;
    if (b[0] != VB_FACE_DIRECTION || b[3] == 0xFF || !(b[3] & CQ_REL) || !d) {
        sayf ("\nYou turn to face %s.", head_name[f]);
    } else {
        sayf ("\nYou turn %s, to face %s.", tw[b[2] & CF_HALF ? (d == 2 ? 1 : d == 6 ? 7 : d) : d], head_name[f]);
    }
}

/* The way the character believes it faces, said (the status command's, after the referee's words of its state: slice
** 4.2's bearings, the north it believes in; unsure of it, said so); and first, every referee's, what the hub has of its
** body that the referee's Condition hasn't (the user's, 2026-11-03: "When running status, I would like to see my hunger and
** thirst state as well"): its hunger and thirst, in the game's words for them; a limb useless, a limp; how it lies, sits or
** crouches; the water it's in */
void o_facing_words (void)
{
    static const char* const hw[5] = { "not hungry", "hungry", "very hungry", "starving", "wasting away with hunger" };
    static const char* const tw[5] = { "not thirsty", "thirsty", "very thirsty", "parched", "dying of thirst" };
    static const char* const pw[8] = { "", "lying down", "crouching", "kneeling", "sitting", "", "", "" };
    static const char* const lw[4] = { "left arm", "right arm", "left leg", "right leg" };
    unsigned char f, b[2], w;

    hub_mem (0, &w, &HUB->pc_want, 1);
    f = WANT_HUNGER (w);
    w = WANT_THIRST (w);
    sayf ("\nHunger and thirst: %s, %s.", hw[f > 4 ? 4 : f], tw[w > 4 ? 4 : w]);
    hub_mem (0, b, &HUB->pcsum.limbs, 1);
    hub_mem (0, b + 1, &HUB->pcsum.limp, 1);
    for (f = 0; f < 4; ++f) {
        if (b[0] & LB_ARM_L << f) {             /* (LB_ARM_L, LB_ARM_R, LB_LEG_L, LB_LEG_R: bits in order) */
            sayf ("\nYour %s is useless.", lw[f]);
        }
    }
    if (b[1]) {
        sayf ("\nYou limp.");
    }
    hub_mem (0, &f, &HUB->pc_pos, 1);
    if (pw[f & 7][0]) {
        sayf ("\nPosition: %s.", pw[f & 7]);
    }
    hub_mem (0, &f, &HUB->pc_wet, 1);
    hub_mem (0, b, &HUB->pc_swim, 1);
    hub_mem (0, b + 1, &HUB->pc_under, 1);
    if (b[1]) {
        sayf ("\nWater: under it, holding your breath.");
    } else if (b[0]) {
        sayf ("\nWater: swimming.");
    } else if (f & ~WET_RUNS) {
        sayf ("\nWater: %u feet deep where you stand.", f & ~WET_RUNS);
    }
    hub_mem (0, &f, &HUB->pc_face, 1);
    hub_mem (0, b, &HUB->pc_skew, 2);           /* (pc_skew and pc_lost) */
    sayf ("\nFacing: %s%s.", head_name[(f + b[0]) & 7], b[1] ? ", you think; you've lost your bearings" : "");
}
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "OV7CODE")
#pragma rodata-name (push, "OV7RODATA")


/* The task a stop names, by its word ("digging"; "": none, LT_NONE) */
unsigned char __fastcall__ o_stop_kind (const char* word)
{
    static const char* const word_[] = { "walking", "going", "moving", "climbing", "digging", "searching",
                                         "waiting", "resting", "sleeping", "talking", "battering", "breaking",
                                         "smashing", "fighting", "attacking", "kicking", "punching", "butting",
                                         "studying", "disarming", "picking", "forcing", "heaving", "bending",
                                         "winding", "tracking", "sniffing", "filling", "soaking", "dipping", "hiding",
                                         "travelling", "returning", "heading", "tending", "treating", "bandaging",
                                         "prising", "freeing", "sharpening", "honing" };
    static const unsigned char kind_[] = { LT_WALK, LT_WALK, LT_WALK, LT_CLIMB, LT_DIG, LT_SEARCH, LT_WAIT,
                                           LT_SLEEP, LT_SLEEP, LT_TALK, LT_BATTER, LT_BATTER, LT_BATTER, LT_FIGHT,
                                           LT_FIGHT, LT_BATTER, LT_BATTER, LT_BATTER, LT_STUDY, LT_DISARM, LT_PICK,
                                           LT_FORCE, LT_HEAVE, LT_BEND, LT_WIND, LT_TRACK, LT_SNIFF, LT_FILL,
                                           LT_FILL, LT_FILL, LT_HIDE, LT_TRAVEL, LT_TRAVEL, LT_TRAVEL, LT_TEND, LT_TEND, LT_TEND,
                                           LT_PRISE, LT_PRISE, LT_HONE, LT_HONE };   /* ("working" none: whatever the work is) */
    unsigned char i;

    for (i = 0; i < sizeof kind_; ++i) {
        if (!strcmp (word, word_[i])) {
            return kind_[i] == LT_FILL && HUB->pc_task.kind == LT_DIPOLD ? LT_DIPOLD : kind_[i];   /* (A dip's the old torch's own) */
        }
    }
    return LT_NONE;
}

/* A question's answer (a soft interruption's, "Keep walking?"): 1 yes, go on; 2 no, stop; 0 neither (a command) */
unsigned char __fastcall__ o_answer_of (const char* l)
{
    static const char* const yes[] = { "y", "yes", "yeah", "ok", "sure", "go on", "carry on", "continue" };
    static const char* const no[] = { "n", "no", "nope", "stop", "halt" };
    unsigned char i;

    if (!strnicmp (l, "keep ", 5)) {
        return 1;                               /* ("Keep at it", "keep going", "keep kicking": whatever it's doing) */
    }
    for (i = 0; i < sizeof yes / sizeof yes[0]; ++i) {
        if (!stricmp (l, yes[i])) {
            return 1;
        }
    }
    for (i = 0; i < sizeof no / sizeof no[0]; ++i) {
        if (!stricmp (l, no[i])) {
            return 2;
        }
    }
    return 0;
}

/* A line typed as the character's busy (main.c's at_once: its command the parser's, cmd): 1 if it's carried out at
** once, taking no time (what it carries, its sheet, the map, the help, a stop, its state, a look about, a stance),
** or the fight's own as it fights (an attack at a denizen: its target changed, the world's); else 0, held till the
** character's free, or 2, held and the fight it's in stopped for it (HOUSE.md's fighting on: a command that takes
** time takes over, its round out first) */
unsigned char __fastcall__ o_free_cmd (const char* l)
{
    static const unsigned char now_[] = { VB_META_INVENTORY, VB_META_SHEET, VB_META_MAP, VB_META_HELP, VB_META_TIME, VB_STOP,
                                          VB_OBSERVE_LOCALE, VB_OBSERVE_AROUND, VB_FIND_OBJECT, VB_DEFEND };
    unsigned char a = cmd.action, i;

    for (i = 0; i < sizeof now_; ++i) {
        if (a == now_[i]) {
            return 1;
        }
    }
    if (!strcmp (l, "status")) {
        return 1;
    }
    hub_mem (0, &i, &HUB->pc_task.kind, 1);
    if (i == LT_TRAVEL) {
        return 2;                               /* (A journey: any command ends it, HOUSE.md's travel) */
    }
    return i != LT_FIGHT ? 0 : a == VB_ATTACK_OBJECT && !cmd.obj.kind ? 1 : 2;
}

/* The game paused, or kept (VB_META_PAUSE, VB_META_SAVE: main.c's chores) */
void __fastcall__ o_meta (unsigned char action)
{
    if (action == VB_META_PAUSE) {
        wset (WS_PAUSE, paused = 1, 0);
        tell (T_PAUSED);
        status ();
    } else if (!savedir) {
        tell (T_NOT_KEPT);
    } else {
        save_game ();
        tell (T_KEPT);
    }
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)


/* ---- The long durations (the user's, 2026-10-28: "waits should be able to last a full night's sleep, so 8 hours at
** least.  Some spells may take longer than that to cast, so 16-bit minutes would be better."): a wait's, a sleep's,
** a rest's length read from the line, its intent (I_WAIT: in minutes past 100 of them, WA_MIN), and told back
** (E_TIME's).  Overlay 13's (its room) */
#pragma code-name (push, "OV13CODE")
#pragma rodata-name (push, "OV13RODATA")
#pragma data-name (push, "OV13RODATA")
#pragma bss-name (push, "OV13RODATA")
#pragma static-locals (push, on)              /* (Its locals static, in its bank: the code the less) */
#pragma local-strings (push, on)

static const char nwords[] = "\0one\0two\0three\0four\0five\0six\0seven\0eight\0nine\0ten\0eleven\0twelve\0thirteen\0"
    "fourteen\0fifteen\0sixteen\0seventeen\0eighteen\0nineteen\0twenty\0thirty\0forty\0fifty\0sixty\0seventy\0eighty\0"
    "ninety";                                   /* (0 to 19, then the tens from twenty: 20 on) */
static char nb_[24];
static unsigned char rest_;                     /* (A rest said, not a sleep: its words) */

/* The nth of nwords */
static const char* __fastcall__ nth (unsigned char n)
{
    const char* p = nwords;

    while (n--) {
        p += strlen (p) + 1;
    }
    return p;
}

/* A number, n, in words: "eight", "twenty-five"; past 99 its figures */
static const char* __fastcall__ num_words (unsigned n)
{
    if (n >= 100) {
        return utoa (n, nb_, 10);
    }
    if (n < 20) {
        return nth (n);
    }
    strcpy (nb_, nth (n / 10 + 18));
    if (n % 10) {
        strcat (nb_, "-");
        strcat (nb_, nth (n % 10));
    }
    return nb_;
}

/* A wait's length, minutes: line l's duration, its spans added up, each a number and a unit ("wait 10 minutes", "an
** hour", "3 hours 20 minutes", "3 hours and 20 minutes", "half an hour", "eight hours": the parser's numbers, figures
** and one to ten), n the parser's first number (one of its words not known here); 0 none said (a round); 65,535 at
** most */
static unsigned __fastcall__ wait_of (const char* l, unsigned char n)
{
    register const char* p = l;
    const char* w;
    unsigned long t = 0;
    unsigned v = 0;
    unsigned char k, c, half = 0, said = 0;

    for (; *p; ++p) {
        if (p != l && p[-1] != ' ') {
            continue;                           /* (A word's first letter, each) */
        }
        c = *p | 0x20;
        if (c >= '0' && c <= '9') {
            v = dnum (p);
        } else if (c == 'h' && p[1] == 'a') {
            half = 1;                           /* ("half") */
        } else if (c == 'h' || (c == 'm' && p[1] == 'i')) {
            if (!v) {                           /* ("hour", "hours", "minute", "minutes") */
                v = said || !n ? 1 : n;         /* (Its number the parser's: a word not known here) */
            }
            t += c == 'h' ? (half ? 30 : 60UL * v) : half ? 0 : v;
            v = half = 0;
            said = 1;
        } else if (c == 'a' && (p[1] == ' ' || p[2] == ' ')) {
            v = 1;                              /* ("a", "an") */
        } else {
            for (k = 1; k <= 10; ++k) {         /* ("one" ... "ten") */
                w = nth (k);
                c = strlen (w);
                if (!strnicmp (p, w, c) && (p[c] | 0x20) == ' ') {
                    v = k;
                }
            }
        }
    }
    return !said ? 0 : t > 65535UL ? 65535U : t ? (unsigned) t : 1;
}

/* A wait (VB_WAIT, VB_PASS) or a sleep or a rest (VB_SLEEP) begun (chores'): as long as the line says (wait_of); a
** wait or a pass with none said a round; a sleep or a rest a night's, eight hours, or a short one's, half an hour
** (the house's); its intent in tenths, or in minutes past 100 of them (WA_MIN), asleep for a sleep (WA_SLEEP) */
void __fastcall__ o_wait_go (unsigned char action)
{
    unsigned m = time_ui (TU_UNTIL);            /* ("until dawn", "watch the sun": time.c's, 2026-10-31) */
    unsigned char a = 0;

    if (!m) {
        m = wait_of (cl, cmd.n);
    }
    if (action == VB_SLEEP) {
        rest_ = strstr (cl, "rest") != 0;
        if (!m) {
            m = strstr (cl, "short") ? 30 : 480;
        }
        a = rest_ ? WA_SLEEP | WA_REST : WA_SLEEP;
    }
    if (m > 100) {
        a |= WA_MIN;
    } else {
        m *= 600;
    }
    intend_a = a;
    intend2 (I_WAIT, m, 0);
}

/* Time passed for the character (E_TIME: w its tenths, or its minutes, b's WA_MIN; asleep, WA_SLEEP), told: a
** round's (none said), "Time passes."; a wait's as long as it is, "You wait eight hours.", "You wait three hours and
** twenty minutes."; a sleep's, "You settle down to sleep for eight hours." (a rest's, to rest) */
void __fastcall__ o_time_said (unsigned w, unsigned char b)
{
    unsigned m = b & WA_MIN ? w : w / 600, h = m / 60, r = m % 60;

    if (!(b & (WA_MIN | WA_SLEEP)) && (w < 600 || w % 600)) {
        tell (T_TIME_PASSES);                   /* (A round, or less than a minute's: as it was) */
        return;
    }
    sayf (b & WA_SLEEP ? "\nYou settle down to %s for " : "\nYou wait ", rest_ ? "rest" : "sleep");
    if (m == 30) {
        say ("half an hour");
    } else {
        if (h) {
            say (h == 1 ? "an" : num_words (h));
            say (h == 1 ? " hour" : " hours");
            if (r) {
                say (" and ");
            }
        }
        if (r || !h) {
            say (r == 1 ? "a" : num_words (r));
            say (r == 1 ? " minute" : " minutes");
        }
    }
    say (".");
}

#pragma local-strings (pop)
#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma data-name (pop)
#pragma bss-name (pop)
