/*
** hself.c - the body's commands and the meta commands (COMMANDS-CRITIQUE.md's §5.2, step L, stage 3: the command
** handlers by meaning, cmds.h): the hands and the limbs (swap hands, sit, stand, lie down, crouch), a feel about in
** the dark, a look about, a wait or a sleep, a long task stopped, first aid, a dig; and what's carried, the sheet and
** the map, the time, pause and save, help, quit.  Part 2's, the commands' own: each verb's work as it was in main.c's
** s_command (part 2's), rarer (part 24's: the dig) and words.c's more (overlay 16's: the time, first aid).
*/

#include <string.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "cmds.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's in it) */
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* H_POWERS: a spell, a talent, an invocation: refused, true to the character (2026-11-09, the user's: the refusal the
** row's request, A_NO_MAGIC, A_NO_PSI, A_NO_GODS: actions.c's); an invocation's names the character's deity (the
** referee's, RS_DESCRIBE's RD_DEITY: "" none, its form without a name).  Part 24's, the commands seen to less often,
** through its stub powers (2026-11-11: part 2's room) */
void powers (void);
#pragma code-name (push, "SW24CODE")
#pragma rodata-name (push, "SW24RODATA")
void s_powers (void)
{
    struct req* r;

    lb[0] = 0;
    if (cmd.aux == A_NO_GODS) {
        r = ask_b (RS_DESCRIBE, 0, RD_DEITY);
        strcpy (lb, r->text);
        r->state = REQ_FREE;
    }
    say_no (cmd.aux, lb);
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* H_PRETEND (the plan's batch B3, the minds'): dead or asleep feigned, an Acting contest (P_ACT); minds.c's, part 27 */
static void do_pretend (void)
{
    minds (H_PRETEND);
}

/* H_PRAY (the plan's batch B5, 2026-11-10): an hour's prayer to the character's deity, a Ritual test (P_RITUAL):
** part 27's, below */
void pray_go (void);
static void do_pray (void)
{
    pray_go ();
}

/* H_FAST (B5's): a vow of fasting, eating refused for a day or till "stop" (part 27's, below) */
void fast_go (void);
unsigned char fast_stop (void);
static void do_fast (void)
{
    fast_go ();
}

/* H_AGAIN (the plan's batch B7, the line's): the last line typed that didn't say "again" run again ("again",
** "repeat"): its parts queued first, before any typed ahead (parts.c's PO_REPEAT, term.c's t_last), each carried out
** as the character's free, as if typed */
void again_go (void);
static void do_again (void)
{
    again_go ();
}

/* The last line typed that didn't say "again" (term.c's t_last) queued in its parts (parts.c's, as typed: lb keeps the
** line being typed meanwhile), then put first, before any typed ahead; none, or no room, said so.  Part 24's (part 2's
** room), through its stub again_go */
#pragma code-name (push, "SW24CODE")
#pragma rodata-name (push, "SW24RODATA")
void s_again_go (void)
{
    unsigned d = qlen, e;

    strcpy (lb, inl);
    strcpy (inl, t_last);
    e = t_last[0] && parts_in (PO_LINE);
    strcpy (inl, lb);
    if (!e) {
        msg ("There's nothing to do again.");
        return;
    }
    e = qlen - d;
    memcpy (lb + 128, queue + d, e);
    memmove (queue + e, queue, d);
    memcpy (queue, lb + 128, e);
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* The body's and the meta commands, by their handler's number (H_NOTHING ... H_AGAIN) */
void __fastcall__ s_self_do (unsigned h)
{
    unsigned char d;

    switch ((unsigned char) h) {
    case H_STATUS:                              /* (The character's state: pace.c's "status", described (0); the line in
                                                **   the main RAM, this part's own gone as part 4 comes in) */
        strcpy (lb, "status");
        pace_cmd (lb);
        break;
    case H_POWERS:
        powers ();
        break;
    case H_PRETEND:
        do_pretend ();
        break;
    case H_PRAY:
        do_pray ();
        break;
    case H_FAST:
        do_fast ();
        break;
    case H_AGAIN:
        do_again ();
        break;
    case H_LIMBS:                               /* (The hands' and limbs': part 11's, slice 4.1b's; the body's position,
                                                **   slice 4.6's) */
        limbs_cmd ();
        break;
    case H_FEEL:                                /* (A look about, or a search for a thing; in the dark, or blind, the
                                                **   character free, its hands: what's told at once, then the feel around
                                                **   (search.c's: the user's, 2026-11-03) */
        if (!busy && !(HUB->pcsum.cond & C_STUNNED) && sight_get ()->dark) {
            if (cmd.action == VB_OBSERVE_LOCALE) {
                look (0);
            }
            cmd.aux = SR_FOR;
            search_go ();
            break;
        }
    case H_LOOK:
        look (0);
        break;
    case H_WAIT:                                /* (A round, or as long as it says: "wait 10 minutes"; engaged, "pass": a
                                                **   round given up, defending; a sleep or a rest, a night's or as long as
                                                **   it says: task.c's) */
        wait_go (cmd.action);
        break;
    case H_STOP:                                /* (The long task: "stop", "stop digging"; a fast, "stop fasting", or
                                                **   "stop" with no long task: part 27's fast_stop) */
        if (!fast_stop ()) {
            intend (I_STOP, stop_kind (cmd.obj.word));
        }
        break;
    case H_TEND:                                /* (First aid: part 12's, slice 4.3's) */
        tend ();
        break;
    case H_DIG:                                 /* (A wall, the rock, a shaft: the world's, a heading 8 on, a diagonal
                                                **   too) */
        d = cmd.dir;
        if (d != GO_DOWN && d > H_NW) {
            tell (T_DIG_WHICH_WAY);
            break;
        }
        walk_says ((const char*) W_DIG);
        intend (I_DIG, d == GO_DOWN ? DIR_DOWN : d + 8);
        break;
    case H_INVENTORY:                           /* (What's carried, and its load) */
        list_say (1);
        things (IO_LIST, "");
        list_say (0);
        if ((d = HUB->pcsum.load) != 0) {
            tell (T_BURDENED - 1 + d);          /* (Its load, as the referee has it: its encumbrance) */
        }
        break;
    case H_SHEET:                               /* (The map, or the sheet: part 3's) */
        sheet_map (cmd.action == VB_META_MAP);
        break;
    case H_TIME:                                /* (The time it believes: time.c's, 2026-10-31) */
        time_ui (TU_TELL);
        break;
    case H_META:                                /* (Paused, or kept: overlay 3's) */
        meta (cmd.action);
        break;
    case H_HELP:
        help ();
        break;
    case H_QUIT:
        quit = 1;                               /* (H_NOTHING: nothing) */
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Prayer and fasting (the plan's batch B5, 2026-11-10: the user's; DECISIONS.md's "Prayer, fasting, swimming and
** following"): part 27's, its own (part 2's room; the banks full: overlay 25 its image's), its stubs the build's */
#pragma code-name (push, "SW27CODE")
#pragma rodata-name (push, "SW27RODATA")

unsigned long fasting;                          /* (The vow's end, the clock's tenths: 0 none; feed.c's refusal.  The main
                                                **   RAM's, not kept in a saved game) */

/* A prayer (VB_PRAY, VB_PRAY_FOR the same): to the character's own deity (the referee's RS_DESCRIBE RD_DEITY: "" none),
** an hour (a timed work, LT_WAIT's: its command kept, main.c's reach 3), its end (reach 4) the Ritual test (P_RITUAL)
** and a calming line naming the deity, its words alone, no other effect (the user's); with no deity, the hour the same
** and a line of its own, no test */
void s_pray_go (void)
{
    static const char* const calm[4] = {        /* (By the grade: CF, MF, MS, CS) */
        "\nYou rise from your prayers. The rites slipped from you and %s seemed far away; still, the quiet hour has eased your mind.",
        "\nYou rise from your prayers. The words came haltingly, but you feel steadier for the hour given to %s.",
        "\nYou rise from your prayers, calmer, sure that %s has heard you.",
        "\nYou rise from your prayers. %s's peace settles on you, deep and sure."
    };
    struct req* r;
    signed char g;
    char d[20];                                 /* (On the C stack: the deity's name, the slot freed) */

    r = ask_b (RS_DESCRIBE, 0, RD_DEITY);
    memcpy (d, r->text, sizeof d - 1);
    d[sizeof d - 1] = 0;
    r->state = REQ_FREE;
    if (reach != 4) {
        if (*d) {
            sayf ("\nYou kneel and pray to %s.", d);
        } else {
            msg ("You kneel and pray, though you know no god to pray to.");
        }
        hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
        reach = 3;
        intend2 (I_WAIT, 36000U, I_WAIT_W2 (LT_WAIT, 0, 1));   /* (An hour) */
        return;
    }
    if (!*d) {
        msg ("You rise from your prayers. If anyone heard, no sign of it came; still, the quiet hour has steadied you.");
        return;
    }
    g = RS_TEST_GRADE (*ask_b (RS_TEST, P_RITUAL, 0));
    HUB->slot[SLOT_UI].state = REQ_FREE;
    sayf (calm[g == G_BADLY ? 0 : g == G_FAIL ? 1 : g == G_PASS ? 2 : 3], d);
}

/* The vow ended ("break my fast", "stop fasting", "stop" with no long task): told; or none to end */
static void unvow (void)
{
    hub_map (0);
    if (fasting > HUB->now) {
        msg ("You break your fast: you may eat again.");
    } else {
        msg ("You aren't fasting.");
    }
    fasting = 0;
}

/* A vow of fasting (VB_FASTING: "fast"): a day from now no food (feed.c's refusal), hunger counting on as ever (the
** referee's, rf/want.c); "break my fast" (the parser's, the line's word) ends it */
void s_fast_go (void)
{
    unsigned long now;

    if (strstr (cl, "break")) {
        unvow ();
        return;
    }
    hub_map (0);
    now = HUB->now;
    if (fasting > now) {
        sayf ("\nYou're fasting already, with %u hours of your vow to go.", (unsigned) ((fasting - now + 35999UL) / 36000UL));
        return;
    }
    fasting = now + 864000UL;                   /* (A day) */
    msg ("You vow to fast for a day: no food shall pass your lips till this hour tomorrow. (\"break my fast\" ends the vow.)");
}

/* A stop (H_STOP's) that ends the vow: "stop fasting" (said), or "stop" with no long task while it fasts.  1 if so
** (told), else 0 (the long task's stop, as ever) */
unsigned char s_fast_stop (void)
{
    const char* w = cmd.obj.word;

    if (!strcmp (w, "fasting") || !strcmp (w, "fast")) {
        unvow ();
        return 1;
    }
    hub_map (0);
    if (*w || HUB->pc_task.kind || fasting <= HUB->now) {
        return 0;
    }
    unvow ();
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
