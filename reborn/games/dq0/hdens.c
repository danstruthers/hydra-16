/*
** hdens.c - the denizens' commands (COMMANDS-CRITIQUE.md's §5.2, step L, stage 3: the command handlers by meaning,
** cmds.h): a blow (at a denizen, or a thing broken by it), the stance; the missiles (a shot, a throw, a launcher
** loaded, the missiles gathered); away from the foes; a denizen swayed (talk, tell, persuade, threaten, an ally's
** requests, a name given), a gift and the market.  Part 2's, the commands' own: each verb's work as it was in main.c's
** s_command (part 2's) and rarer (part 24's: the sway, the gift).
*/

#include <string.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "cmds.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's in it) */
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* H_MISSILE: the missiles (part 13's, slice 4.7's; a throw at a denizen too): shot, loaded, gathered, thrown at the
** thing said or the target (the world's throw, its words overlay 11's); thrown into a well, let fall in it */
static void do_missile (void)
{
    unsigned char d;

    if (!strcmp (cmd.other.word, "well") || (cmd.prep == PR_IN && !cmd.other.word[0])) {
        put_in ();
        return;
    }
    d = HUB->pc_light;
    if (missiles () == 2) {                     /* (Thrown at a thing: part 13's) */
        after (d, 20, IO_DROP);                 /* (Its time; a light thrown, the world's look in the new light) */
    }
}

/* H_FIGHT: a blow at a denizen beside it (overlay 6's choice), or closed with first; the stance set (the referee's
** defence for the character); a blow at a thing, broken by it */
static void do_fight (void)
{
    unsigned char d;

    if ((d = fight_target (&cmd)) == FT_THING) {
        portal (PA_BREAK);                      /* (A blow at a thing: broken by it) */
        return;
    }
    if (d) {
        if (!hold_w[0] || hold_first ()) {      /* (Its thing in hand first, slice 4.1b's) */
            if (d & FT_CLOSE) {
                reach = 6;                      /* (Closed with first, its arrival's words short) */
                walk_says (0);
                intend (I_CLOSE, d & 0x0F);
                return;
            }
            intend (I_ATTACK, d);               /* (Beside it: d its number alone) */
        }
        return;
    }
    HUB->ireq.into[0] = 0;                      /* (A blow, no foe of that name: a thing of it to hand, refused, */
    HUB->ireq.to = 0;                           /*   said back, "You can't kick the pot."; else the fight's words, */
    if (cmd.action == VB_ATTACK_OBJECT && cmd.obj.word[0] && !cmd.obj.kind &&   /* lb's (IO_FIND's IR_NO: none */
        things (IO_FIND, cmd.obj.word) == IR_OK) {   /* such, or seen out of reach, its words in the things') */
        unknown (U_CANT);
    } else {
        msg (lb);
    }
}

/* The denizens' commands, by their handler's number (H_FIGHT ... H_GIVE) */
void __fastcall__ s_dens_do (unsigned h)
{
    switch ((unsigned char) h) {
    case H_FIGHT:
        do_fight ();
        break;
    case H_MISSILE:
        do_missile ();
        break;
    case H_DISENGAGE:                           /* (Away from the foes engaging it, the book's disengage; a flight at a
                                                **   run once free: the world's) */
        intend2 (I_DISENGAGE, cmd.dir, cmd.action == VB_FLEE);
        break;
    default:                                    /* (H_SWAY: a denizen swayed, an ally asked, a name given: the denizens'
                                                **   sway the verb's, SW_*; H_GIVE: a gift, the market, entertaining:
                                                **   part 15's whole, 2026-11-02) */
        if (errand ()) {                        /* (A thing to fetch found first, a gift's: part 15's, slice 4.8's) */
            deal ((unsigned char) h == H_GIVE ? SW_GIVE : cmd.aux);
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
