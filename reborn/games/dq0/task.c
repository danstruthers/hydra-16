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
#include "dq0said.h"

unsigned pick_x, pick_y;                        /* (A door's or an opening's square chosen in answer to choose.c's
                                                **   question: letters.c's which_in sets it, choose.c's o_pick takes it;
                                                **   here, the main RAM's, 2026-11-07) */

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

static const char* const doing[] = {           /* (By LT_*; then by SV_*, a step's verb, from NLT) */
    "doing anything", "walking", "climbing", "digging", "searching", "waiting", "sleeping", "talking", "battering",
    "fighting", "travelling", "studying the mechanism", "disarming the trap", "picking the lock", "forcing", "heaving at",
    "bending the bars", "winding the winch", "working the pane free", "tracking", "sniffing the air", "filling",
    "hiding", "tying the rope", "tending the wound", "turning a key", "dipping",
    "taking off", "taking off", "putting on", "letting fall", "setting down", "setting down", "setting down", "dipping",
    "reaching into", "holding open", "getting out", "letting fall", "putting away", "putting out", "putting out", "lighting",
    "setting down", "taking up", "sharpening", "striking a spark", "sheathing", "slinging", "setting down", "stowing",
    "stowing", "leaving"
};
#define NDOING          (sizeof doing / sizeof doing[0])
#define NLT             (LT_DIP + 1)
typedef char doing_all[NDOING == NLT + SV_LEAVE + 1 - SV_OFF ? 1 : -1];   /* (Every long task's words: "You stop doing anything the torch",
                                                **   2026-11-04) */
#define DOING(k)        doing[(k) & LT_STEP ? NLT - SV_OFF + ((k) & ~SM_ALL) : (k) < NLT ? (k) : 0]   /* (A step's by its verb: its
                                                **   word, SM_*, its first words' alone) */

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

    strcpy (dn, k == LT_BATTER ? struck[n >> 4 & 3] : DOING (k));
    if (n && (k == LT_BATTER || k == LT_FORCE || k == LT_HEAVE || k == LT_FILL || k == LT_DIP || k == LT_HIDE ||
              ((k & LT_STEP) && (k & ~SM_ALL) != SV_KINDLE))) {
        strcat (dn, " ");
        strcat (dn, noun (n));
    }
    return dn;
}

/* What came of the character's long task: E_TASK's w (TE_*, its kind times 256); named, the kind its stop named (b),
** or the foe (TE_FIGHT's), or the health (TE_ASK_HURT's), or a light's kind (TE_LOW's, TE_OUT's), or what it was done
** to (a stop's, a question's, a timed work begun: the long task's what).  1 if a question's put; 2 one answered yes
** unasked (dq0 -a's, the tests': not put); 4 fighting on begun (the character busy).  Its words said.txt's TASK rows
** (said.c's: narration as data, stage 3, 2026-11-11): what's worked out for them here (the foe as it's said, what it
** was doing, the journey's place, a light's plural: their words copied into the main RAM, lb's end, said_s; a choice,
** said_n); the stop, the question, the robbery's purse its own */
static unsigned char pursed;                    /* (Its coins taken as it lay, not told yet: robbed's, overlay 15's before) */
extern unsigned char fol_still;                 /* (hmove.c's: a follow's state, 6 its first walk after its quarry) */

unsigned char __fastcall__ o_task_words (unsigned w, unsigned char named)
{
    unsigned char k = w >> 8, p = (unsigned char) w, n = named, one = 0;
    const char* d;
    struct msg c;                               /* (An event for the table's rows: said.c's) */

    if (p == TE_OUT) {
        one = k;                                /* (Another of its name burning still; one it sees: the world's) */
        k &= 0x3F;
    }

    if (p == TE_NOT || p == TE_ENGAGED || p == TE_OUT) {
        hub_mem (0, &n, &HUB->pc_task.what, 1);     /* (Its b another's: what it's done to the hub's) */
    }
    d = doing_of (k, n);
    if (p <= TE_NEAR || p == TE_ENGAGED || (p == TE_OUT && k)) {
        parts_in (PO_STOP);                     /* (The rest of its line with it: parts.c's lists, 2026-11-04) */
        if (k != LT_FIGHT || reach < 5) {          /* (A fight begun as the character came free, its line's first step in hand: that stopped, not the step) */
            reach = 0;                          /* (A walk over to a thing stopped: its command dropped; a thing's
                                                **   timed work stopped: its end dropped, words.c's; "take all"'s
                                                **   later things with it, 2026-11-06) */
            pending[0] = 0;
        } else if (cmd.action == VB_STOP && p == TE_STOPPED) {
            pending[0] = 0;                     /* (The player's stop of a fight that took over a reach's steps: the reach
                                                **   ends with it, where it stands, the pack's place told, as a flight's
                                                **   does, main's; the user's, 2026-11-17: "'stop' ends everything under
                                                **   way, a fight's part and the reach it took over") */
            reach_said (TE_IDLE, 0);
        }
    }
    k = p;
    said_s[0] = said_s[1] = 0;                  /* (The row's words: what it was doing its text, in the main RAM) */
    said_s[2] = strcpy (lb + 300, d);
    said_n = 0;
    c.kind = E_TASK;
    c.who = 0;
    c.b = named;
    c.w1 = w;
    if (k == TE_ASK_SHORT && w >> 8 != LT_DIG) {  /* (A dig's nothing to show: its words its own, E_DIG's.  A battering's: a bare part's blow that would hurt it stopped short
                                                **   of it, once a door; told, dq0 -a's too, then asked: TASK.bare) */
        said_n = named >> 4 & 3;
        said_s[0] = strcpy (lb + 340, noun (named));
        c.w1 = SAID_TASK_BARE;
        outcome_words (&c);
        c.w1 = w;
    }
    if (k == TE_ASK_SEEN || k == TE_ASK_TIRED || k == TE_ASK_HEARD || k == TE_ASK_HURT || k == TE_ASK_SHORT) {
        if (autogo) {
            return 2;
        }
        parts_in (PO_STOP);                     /* (Asked: the rest of a list isn't its answer, and goes as a stop's does) */
    }
    switch (k) {
    case TE_BODY:                               /* (What time did to its body: the referee's words, part 12's; 8 dead
                                                **   of it, slice 4.3's; then a robbery at rest, as it's up: its coins
                                                **   gone told, TASK.coins) */
        if (!(p = body_words ()) && pursed) {
            hub_mem (0, &p, &HUB->pc_task.kind, 1);
            if (p) {
                news_due |= 2;                  /* (At rest still: as it's up) */
                return 0;
            }
            said_n = pursed >> 1;
            pursed = 0;
            c.w1 = SAID_TASK_COINS;
            outcome_words (&c);
        }
        return p;
    case TE_NOT:
        said_s[0] = strcpy (lb + 340, DOING (named));
        break;
    case TE_IDLE:                               /* (TASK.idle: its arg 0, nothing to stop; 1, between two steps of the
                                                **   reach, they stop, the pack's place told, part 19's; "take all"'s
                                                **   later things with them, 2026-11-06; 2, engaged, a step a round's
                                                **   action, no long task: that step not done) */
        if (reach >= 5) {
            said_n = 1;
            if (busy && step_k) {
                said_n = 2;
                strcpy (lb + 300, doing_of (step_k, step_n));
            }
            outcome_words (&c);
            pending[0] = 0;
            reach_said (TE_IDLE, 0);
            return 0;
        }
        break;
    case TE_ENGAGED:                            /* (HOUSE.md's engagement: "The giant rat engages you: you stop
                                                **   walking."; engaged already, "You're engaged with the giant rat.") */
    case TE_AWAY:                               /* (The engage initiative test's: an enemy newly engaged the quicker,
                                                **   its tactical advantage a disengage) */
    case TE_FIGHT:                              /* (Fighting on: begun on a hostile engaging it, or by an attack, its
                                                **   blow's words enough; turned to the next) */
    case TE_BREAK:                              /* (A disengage, the book's: w's high byte 0 backing away, 1 a flight,
                                                **   2 a run from one not engaging it) */
    case TE_FETCHED:                            /* (An ally's errand, a gift taken: slice 4.8's) */
    case TE_ROBBED:                             /* (Helpless, robbed: a robber's way, the denizens'.  At rest, 2026-11-06:
                                                **   caught at it, or its coins taken unawares, told as it's up) */
        said_s[0] = foe (k == TE_FETCHED ? n & 15 : named, 0);
        if (k == TE_ROBBED && !(w >> 8 & RB_CAUGHT) && ((w >> 8 & RB_PURSE) || out_told)) {
            pursed = out_told ? 2 : 1;          /* (Senseless, robbed: not seen, found as it comes round, the user's,
                                                **   2026-11-14: TASK.coins' arg 1) */
            news_due |= 2;
            if (out_told) {
                return 0;
            }
        }
        break;
    case TE_FAR:
        if (cmd.action != VB_FOLLOW_OBJECT || fol_still == 6) {   /* (Following: re-aimed each time, out of reach as often as
                                                **   it keeps its distance; the follow's own words its end, "You can't keep
                                                **   up", "You've lost", hmove.c's: 2026-11-17, a rat keeping its distance at
                                                **   the denizens' new timing told it four times over in one answer.  Said
                                                **   once, as the follow begins, its first walk after it: fol_still 6, the
                                                **   user's, 2026-11-17, "A follow out of reach says so once, when it
                                                **   starts") */
            say_no (A_NEARER, foe (named, 0));
        }
        return 0;
    case TE_LOW:                                /* (A light it carries in its last ten minutes: slice 4.1's; not in its sleep) */
        hub_mem (0, &p, &HUB->pc_task.kind, 1);
        if (p == LT_SLEEP || out_told) {
            return 0;                           /* (Nor as it lies senseless: unseen, the user's, 2026-11-14) */
        }
        said_n = lamp (named) != 0;
        break;
    case TE_OUT:                                /* (Its fuel burnt: its own, its only light, its task stopped with
                                                **   it; or one it sees lying there, TO_SEEN.  Its arg: the light's
                                                **   plural, one seen (2), one of several of its name (4, 5 "es") */
        if (out_told) {
            return 0;                           /* (Senseless: unseen, the place looked at as it comes round, numb's) */
        }
        p = lamp (named);
        if (one & TO_SEEN) {                    /* (One lying about or in a sconce: told once an answer, a long rest's two
                                                **   the soak's, 2026-11-04; harm_told's bit, the prone's it doesn't read) */
            if (harm_told & 1) {
                return 0;
            }
            harm_told |= 1;
        }
        said_n = (p != 0) | (one & TO_SEEN ? 2 : 0);
        if (!(one & TO_SEEN) && !p && (one & TO_ANOTHER)) {     /* (Another of its name burning still, the world's: "One of
                                                **   your torches", not "Your torch" twice) */
            said_n = strchr ("sxh", fn[strlen (fn) - 1]) ? 5 : 4;
        }
        if (!(w >> 8 & 0x3F)) {                 /* (Its long task stopped with it: what it was doing; or none) */
            lb[300] = 0;
        }
        break;
    case TE_PACKED:                             /* (The reach's: part 19's) */
    case TE_FUMBLE:
    case TE_KINDLED:
        reach_said (w, named);
        return 0;
    case TE_BEGUN:                              /* (A timed work, a battering, begun: its first words at once, its
                                                **   outcome as its time's done, HOUSE.md's long tasks; the user's,
                                                **   2026-10-22) */
        p = w >> 8;
        if (p & LT_STEP) {
            reach_said (w, named);              /* (A step of the reach: part 19's) */
            return 0;
        }
        if ((p == LT_FORCE || p == LT_HEAVE) && n) {    /* (What's forced or heaved at, its words obj: the piece
                                                **   every actor's, a denizen's "the door", 2026-11-11) */
            said_s[0] = strcpy (lb + 340, noun (n));
        }
        if (p == LT_TRAVEL) {                   /* (A journey: slice 4.2's) */
            strcpy (lb + 300, place (named));
        } else if (p == LT_BATTER && (n & SK_WITH)) {   /* (Struck with the thing named: "with the bar", the hub's
                                                **   pc_strike's; its what the portal's kind plus 1) */
            p = (n & 15) - 1;
            said_s[0] = strcpy (lb + 340, p == PK_PORTCULLIS ? "the portcullis" : p == PK_DOOR ? "the door" : "the window");
            hub_mem (0, &p, &HUB->pc_strike, 1);
            hub_mem (KIND_BANK, lb + 370, CATALOGUE[SK_KIND (p)].name, sizeof CATALOGUE[0].name);
            lb[370 + sizeof CATALOGUE[0].name] = 0;
            said_s[1] = lb + 370;
        }
        break;
    case TE_THERE:                              /* (A journey's: slice 4.2's, HOUSE.md's travel; b its place, | TRV_ON
                                                **   under way.  Its arg: under way 2, the way in 1) */
        said_n = (named & TRV_ON ? 2 : 0) | ((named & ~TRV_ON) == TRV_IN);
        /* (Fall through: its place) */
    case TE_FORGOT:
    case TE_ASTRAY:
        if (k == TE_FORGOT) {
            said_n = (named & TRV_ON) != 0;
        }
        strcpy (lb + 300, place (named));
        break;
    case TE_ELSEWHERE:                          /* (Its text the place, none the way in) */
        lb[300] = 0;
        if ((named & ~TRV_ON) != TRV_IN) {
            strcpy (lb + 300, place (named));
        }
        break;
    }
    outcome_words (&c);
    return k == TE_FIGHT ? 4 : k == TE_ASK_SEEN || k == TE_ASK_TIRED || k == TE_ASK_HEARD || k == TE_ASK_HURT ||
        k == TE_ASK_SHORT;                      /* (1: a question put) */
}


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


/* (Overlay 7's, its room: the look's words in the dark, 2026-10-22) */
#pragma code-name (push, "OV17CODE")            /* (Overlay 17's since 2026-11-02: overlay 7's room, what can be named) */
#pragma rodata-name (push, "OV17RODATA")
#pragma local-strings (push, on)

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
        if (s->item[i].hid & 2 ? j : k == SEE_ITEMS && !(cmd.obj.word[0] && name_is (s->item[i].name, cmd.obj.word))) {
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
** unless it's been on this square before and not forgotten it ("This should be the kitchen.").  Its kinds of wall a
** list, each with its sides (said.txt's LIST.touch and LI.touch: the look's lists, the user's, 2026-11-16) */
static void touch_words (void)
{
    static const unsigned char order_[4] = { 1, 2, 3, 0 };
    struct sight* s = &sight_copy;
    unsigned char c = s->cmid, i, k, n, m;
    struct li v[4];                             /* (On the C stack: the list, said_li) */
    struct msg f;

    hub_mem (0, &k, (char*) &HUB->pcsum.cond + 1, 1);   /* (C_BLINDED, its high byte: the window's not the hub's here) */
    f.kind = UK_LIST;
    f.who = 0;
    f.b = s->kind;
    f.w1 = LF_TOUCH << 8 | (k & C_BLINDED >> 8 ? 1 : 0);
    f.w2 = s->cwall ? (s->where == W_ROOM ? 1 << 4 : 2 << 4) : 0;
    for (i = f.a = 0; i < 4; ++i) {
        k = order_[i];
        for (n = m = 0; m < 3; ++m) {
            n |= TOUCHED (c, m) == k ? 1 << m : 0;
        }
        if (n) {
            v[f.a].cap[0] = 0;
            v[f.a].w1 = LI_TOUCH << 8;
            v[f.a++].a = k | n << 2;
        }
    }
    said_li = v;
    lbe = lb + strlen (lb);
    list_words (&f);
}

/* A walk over for a command just done (FW_NEW), or what its hands and feet can tell in the dark (FW_TOUCH): the
** look's (words.c's).  (A walk's beats in the dark were its too: said.txt's BEAT rows since 2026-11-11, stage 3) */
void __fastcall__ o_felt_words (unsigned w)
{
    if (w == FW_NEW) {
        new_words ();
    } else {
        touch_words ();
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "OV19CODE")            /* (Overlay 19's since 2026-11-03: overlay 1's room) */
#pragma rodata-name (push, "OV19RODATA")

/* The character's own words (UK_SELF's row MY_*, w's high byte; v its low, n2 b): said.txt's SELF rows, narration as
** data's rest, 2026-11-15 */
static void __fastcall__ my (unsigned w, unsigned char b)
{
    struct msg c;

    c.kind = UK_SELF;
    c.who = 0;
    c.b = b;
    c.w1 = w;
    outcome_words (&c);
}

/* A turn's first words (V_FACING's): "You turn to face west.", always, whatever's typed after it (the user's,
** 2026-11-16: the way it turned, "You turn right, to face west.", was read from the command queue as the words were
** told, and a command typed after it was found there instead; the event's b now the heading it believes it faces, | 32,
** the queue not read).  A step's or a walk's (V_SETOFF's: b its way).  (Their words said.txt's SELF rows: step,
** setoff, face) */
void __fastcall__ o_turned_says (unsigned char go)
{
    unsigned char f, d;

    if (go & 32) {                              /* (A turn: the heading it believes it faces, the world's) */
        said_s[2] = head_name[go & 7];
        my (MY_FACE << 8, 0);
        return;
    }
    if (worded == 2) {                          /* (A way set out on, V_SETOFF's: 8 a step, by the way it faces; a walk,
                                                **   by the north it believes in) */
        return;
    }
    hub_mem (0, &f, &HUB->pc_face, 1);
    hub_mem (0, &d, &HUB->pc_skew, 1);
    if (go & 8) {
        my (MY_STEP << 8 | ((go - f) & 7), 0);
    } else {
        said_s[2] = head_name[(go + d) & 7];
        my (MY_SETOFF << 8, 0);
    }
}

/* The way the character believes it faces, said (the status command's, after the referee's words of its state: slice
** 4.2's bearings, the north it believes in; unsure of it, said so); and first, every referee's, what the hub has of its
** body that the referee's Condition hasn't (the user's, 2026-11-03: "When running status, I would like to see my hunger and
** thirst state as well"): its hunger and thirst, in the game's words for them; a limb useless, a limp; how it lies, sits or
** crouches; the water it's in.  (Their words said.txt's SELF rows) */
void o_facing_words (void)
{
    unsigned char f, b[2], w;

    hub_mem (0, &w, &HUB->pc_want, 1);
    my (MY_WANT << 8 | WANT_HUNGER (w), WANT_THIRST (w));
    hub_mem (0, b, &HUB->pcsum.limbs, 1);
    hub_mem (0, b + 1, &HUB->pcsum.limp, 1);
    for (f = 0; f < 4; ++f) {
        if (b[0] & LB_ARM_L << f) {             /* (LB_ARM_L, LB_ARM_R, LB_LEG_L, LB_LEG_R: bits in order) */
            my (MY_LIMB << 8 | f, 0);
        }
    }
    if (b[1]) {
        my (MY_LIMP << 8, 0);
    }
    hub_mem (0, &f, &HUB->pc_pos, 1);
    if ((f &= 7) != 0 && f <= 4) {              /* (Lying down, crouching, kneeling, sitting) */
        my (MY_POS << 8 | f, 0);
    }
    hub_mem (0, &f, &HUB->pc_hid, 1);
    if (f) {
        my (MY_HIDDEN << 8, 0);
    }
    hub_mem (0, &f, &HUB->pc_wet, 1);
    hub_mem (0, b, &HUB->pc_swim, 1);
    hub_mem (0, b + 1, &HUB->pc_under, 1);
    if (b[1]) {
        my (MY_UNDER << 8, 0);
    } else if (b[0]) {
        my (MY_SWIM << 8, 0);
    } else if (f & ~WET_RUNS) {
        my (MY_DEEP << 8 | (f & ~WET_RUNS), 0);
    }
    hub_mem (0, &f, &HUB->pc_face, 1);
    hub_mem (0, b, &HUB->pc_skew, 2);           /* (pc_skew and pc_lost) */
    said_s[2] = head_name[(f + b[0]) & 7];
    my (MY_FACING << 8 | b[1], 0);
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
                                           LT_FILL, LT_DIP, LT_HIDE, LT_TRAVEL, LT_TRAVEL, LT_TRAVEL, LT_TEND, LT_TEND, LT_TEND,
                                           LT_PRISE, LT_PRISE, SV_HONE, SV_HONE };   /* ("working" none: whatever the work is) */
    unsigned char i, k;

    for (i = 0; i < sizeof kind_; ++i) {
        if (!strcmp (word, word_[i])) {
            hub_mem (0, &k, &HUB->pc_task.kind, 1);   /* (Filling, soaking, dipping: whichever of them it's at, a
                                                **   lantern's, a torch's, the old torch's: each its own words,
                                                **   2026-11-06; the hub's byte copied, as this overlay's in the window
                                                **   as it runs, not the hub's header) */
            return (kind_[i] == LT_FILL || kind_[i] == LT_DIP) && (k == LT_FILL || k == LT_DIP || k == SV_DIP) ? k : kind_[i];
        }
    }
    return LT_NONE;
}

/* A question's answer (a soft interruption's, "Keep walking?"): 1 yes, go on; 2 no, stop; 0 neither (a command) */
unsigned char __fastcall__ o_answer_of (const char* l)
{
    static const char* const yes[] = { "y", "yes", "yeah", "ok", "sure", "go on", "carry on", "continue" };
    static const char* const no[] = { "n", "no", "nope", "stop", "halt", "enough" };
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
** time takes over, its round out first); 3, a line that breaks off (the stop's kind, the user's, 2026-11-06: "`stop`
** and other similar commands should be acted on immediately"): held, whatever long task it's at stopped for it at
** once, a reach's steps with it (main's), then carried out.  The class by the parser's meanings, not its words: a stop
** (stop, halt, enough: carried out at once, 1), a flight or a disengage (flee, escape, run away, back away, withdraw ...) */
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
    if (!strcmp (l, "status") || (a == VB_WAKE && !cmd.obj.word[0])) {
        return 1;                               /* ("wake", "wake up": asleep, a stop's, minds.c's, 2026-11-10) */
    }
    if (a == VB_FLEE || a == VB_DISENGAGE) {
        return 3;                               /* (Breaks off: any long task, a reach's steps, a fight, a journey) */
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

static unsigned char time_told = 0;              /* (Its words said already, o_wait_go's: not as its E_TIME comes) */
void __fastcall__ o_time_said (unsigned w, unsigned char b);

/* A wait (VB_WAIT, VB_PASS) or a sleep or a rest (VB_SLEEP) begun (chores'): as long as the line says (wait_of); a
** wait or a pass with none said a round; a sleep or a rest a night's, eight hours, or a short one's, half an hour
** (the house's); its intent in tenths, or in minutes past 100 of them (WA_MIN), asleep for a sleep (WA_SLEEP).  A sleep
** or a rest with a light burning: its words as its E_TIME comes, after the world's of the light put out first (the
** user's, 2026-11-17: "A rest puts a burning light out first"; its locals on the C stack, the main RAM's: the hub's
** pc_light copied into one, not into the overlay's bank, the window's as hub_mem copies) */
#pragma static-locals (push, off)
void __fastcall__ o_wait_go (unsigned char action)
{
    unsigned m = time_ui (TU_UNTIL);            /* ("until dawn", "watch the sun": time.c's, 2026-10-31) */
    unsigned char a = 0, lit = 0;

    if (!m) {
        m = wait_of (cl, cmd.n);
    }
    if (action == VB_SLEEP) {
        rest_ = cmd.aux & 1;                    /* (A rest, a short one: the verbs' own, commands.verbs' rest, shortrest,
                                                **   shortsleep) */
        if (!m) {
            m = cmd.aux & 2 ? 30 : 480;
        }
        a = rest_ ? WA_SLEEP | WA_REST : WA_SLEEP;
        hub_mem (0, &lit, (const void*) &HUB->pc_light, 1);
    }
    if (m > 100) {
        a |= WA_MIN;
    } else {
        m *= 600;
    }
    if (!busy && m && !lit) {
        o_time_said (m, a);                     /* (As long as it says: told at once, the world's E_TIME to come saying
                                                **   nothing, the user's, 2026-11-04; a round's in time as it is) */
        t_flush ();
        time_told = 1;
    }
    intend_a = a;
    intend2 (I_WAIT, m, 0);
}
#pragma static-locals (pop)

/* Time passed for the character (E_TIME: w its tenths, or its minutes, b's WA_MIN; asleep, WA_SLEEP), told: a
** round's (none said), "Time passes."; a wait's as long as it is, "You wait eight hours.", "You wait three hours and
** twenty minutes."; a sleep's, "You settle down to sleep for eight hours." (a rest's, to rest) */
void __fastcall__ o_time_said (unsigned w, unsigned char b)
{
    unsigned m = b & WA_MIN ? w : w / 600, h = m / 60, r = m % 60;

    if (time_told) {
        time_told = 0;
        return;
    }
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

