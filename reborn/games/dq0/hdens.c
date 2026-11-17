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

/* A grapple's move g (rs.h's GM_*, | GT_TRY: blows.c's grapple, overlay 19's: HOUSE.md's grappling, 2026-11-09) sent
** (I_ATTACK, its b AIM_GRAPPLE | the move), or why not said; GT_TRY's none beside, 0, nothing said */
unsigned char __fastcall__ grip_go (unsigned char g)
{
    if ((g = grapple (g)) & GR_SEND) {
        intend (I_ATTACK, g & 0x0F);
    } else if (g) {
        msg (lb);
    }
    return g;
}

/* H_MISSILE: the missiles (part 13's, slice 4.7's; a throw at a denizen too): shot, loaded, gathered, thrown at the
** thing said or the target (the world's throw, its words overlay 11's); thrown into a well, let fall in it */
static void do_missile (void)
{
    unsigned char d;

    if (!strcmp (cmd.other.word, "well") || (cmd.prep == PR_IN && !cmd.other.word[0])) {
        put_in ();
        return;
    }
    if (cmd.action == VB_THROW_OBJECT && grip_go (GM_THROW | GT_TRY)) {
        return;                                 /* ("Throw the goblin": a denizen beside, thrown, a grapple's) */
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

/* ---- The fights' verbs not done till the plan's batch B1 (2026-11-10, DECISIONS.md's "Charge, a throw a way, engage,
** a split attack, and again"): part 24's, the commands seen to less often (part 2's room), through its stub fights.  Its
** words its own, said from it (the main RAM's sayf, msg); its state the part's own (none kept from one command to the
** next, and no other part called while it's needed) */
void __fastcall__ fights (unsigned char h);
#pragma code-name (push, "SW24CODE")
#pragma rodata-name (push, "SW24RODATA")
#pragma bss-name (push, "SW24RODATA")

static unsigned char k, d, e, n;
static int dx, dy;

/* How far a square is, dx, dy away (squares, 0xFE at most) */
static unsigned char span (void)
{
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    if (dy > dx) {
        dx = dy;
    }
    return dx > 0xFE ? 0xFE : dx;
}

/* H_CHARGE: a run at the nearest hostile denizen it sees ahead (ahead, or ahead to either side), a war cry ("Charge!",
** I_NOISE, 6, a yell's: the world's to hear, the noise's batch), and a blow as it gets there (I_CLOSE: its walk at a
** run, "fast charge" a sprint, its run; its engage test the walk's, as it comes into the foe's zone); one beside it
** struck at once.  None: "There's no foe ahead to charge." (the grammar's ERROR:NO_TARGET) */
static void charge (void)
{
    static struct sight* s;
    static struct actor* a;
    static struct sden* q;

    s = sight_get ();
    hub_map (0);
    for (e = k = 0, n = 0xFF, a = HUB->actor; k < NACTORS; ++k, ++a) {
        if (a->state != 1 || a->sn != HUB->pc_stratum || a->attitude != AT_HOSTILE || a->doing == DO_DOWN) {
            continue;
        }
        dx = a->x - HUB->pc_x;
        dy = a->y - HUB->pc_y;
        if (((REL8 (bearing (dx, dy), HUB->pc_face) + 1) & 7) > 2 || (d = span ()) >= n) {
            continue;                           /* (Not ahead, or no nearer) */
        }
        for (q = s->den, dx = s->ndens; dx && (q->x != a->x || q->y != a->y); --dx, ++q) {
        }
        if (dx) {                               /* (One it sees, the senses' last look's: the nearest) */
            n = d;
            e = k + 1;
        }
    }
    if (!e) {
        msg ("There's no foe ahead to charge.");
        return;
    }
    sayf (n > 1 ? "\n\"Charge!\"  You run at the %s." : "\n\"Charge!\"  You fall on the %s.", HUB->actor[e - 1].name);
    t_flush ();
    strike = SK_BEST;                           /* (With the best it holds, an attack's) */
    shand = 0;
    intend_a = 6;
    intend2 (I_NOISE, 0, 0);
    intend_a = e;
    if (n > 1) {
        reach = 6;                              /* (Closed with first, its arrival's words short) */
        intend2 (I_CLOSE, 0, strstr (cl, "fast") ? GA_SPRINT : GA_RUN);
    } else {
        intend2 (I_ATTACK, 0, 0);
    }
}

/* H_ENGAGE: closed with a denizen, walked up to into its zone, no blow (I_CLOSE's walk alone: the engage test the
** walk's, as it comes into the zone; engaged by another, refused, the world's TE_ENGAGED); beside it already, said so;
** a thing, refused */
static void engage (void)
{
    if ((d = fight_target (&cmd)) == FT_THING) {
        msg ("You can engage only a creature.");
    } else if (!d) {
        msg (lb);                               /* (None of that name it sees: the fight's words) */
    } else if (!(d & FT_CLOSE)) {
        sayf ("\nYou're already engaged with the %s.", cmd.obj.word);
    } else {
        sayf ("\nYou close with the %s.", cmd.obj.word);
        t_flush ();
        intend_a = d & 0x0F;
        intend2 (I_CLOSE, 1, 0);                /* (Up to it as it moves, no blow) */
    }
}

/* H_SPLIT: two denizens beside it attacked by turns ("attack the rat and the goblin", one line, parts.c's: the parser's
** thing the second, the last named; the first the word before the "and"): the first struck at, the other sent with the
** blow (I_ATTACK's split), the world's fighting on turning between them while both stand; the other not beside it, the
** first's an attack alone; the first not beside it, go up to it first */
static void split (void)
{
    static const char* p;

    p = strstr (cl, " and ");
    e = fight_target (&cmd);                    /* (The second, the parser's thing) */
    strcpy (lb + 360, cmd.obj.word);
    memset (&cmd.obj, 0, sizeof cmd.obj);
    for (n = 0; p && p > cl && p[-1] != ' ' && n < sizeof cmd.obj.word - 1; --p, ++n) {
    }
    memcpy (cmd.obj.word, p, n);                /* (The first, by its word) */
    if ((d = fight_target (&cmd)) == FT_THING || !d) {
        if (d) {
            unknown (U_CANT);
        } else {
            msg (lb);
        }
    } else if (d & FT_CLOSE) {
        hub_mem (0, &n, &HUB->folk[((d & 0x0F) - 1) & 7], 1);
        say_no (A_FAR | (n ? A_THEY : 0), cmd.obj.word);    /* ("The rat is out of reach: go up to it first."; a
                                                **   person's "them", 2026-11-17) */
    } else {
        if (e == FT_THING || (e & FT_CLOSE) || !((e ^ d) & 0x0F)) {
            e = 0;                              /* (No other foe beside it: the first alone) */
        } else {
            hub_map (0);
            sayf ("\nYou strike at the %s and the %s by turns.", HUB->actor[d - 1].name, lb + 360);
        }
        intend_a = d;
        intend2 (I_ATTACK, e, 0);
    }
}

/* A thing thrown a way ("throw the dagger north", "throw the dagger 3 north": the parser's throw with a heading, no
** mark: H_MISSILE's, VB_THROW_DIRECTION's row): its mark that many squares along the line (none said, a dozen: as far
** as it'll go), the world's throw at a place (IO_THROW, the referee's Throwing; stopped short on its line by a wall, a
** door shut, glass, it falls where it's stopped), told: how far it went */
static void throw_way (void)
{
    unsigned char was = HUB->pc_light;          /* (Not the part's: the things' reach may call another) */

    if (cmd.dir > H_NW) {
        cmd.dir = HUB->pc_face;                 /* (None said: ahead) */
    }
    n = cmd.n ? cmd.n : 12;
    HUB->ireq.x = HUB->pc_x + head_x[cmd.dir] * n;
    HUB->ireq.y = HUB->pc_y + head_y[cmd.dir] * n;
    HUB->ireq.to = cmd.manner;
    if ((d = things (IO_THROW | IO_STEPS, cmd.obj.word)) != IR_OK) {
        throw_said (d);                         /* (None carried, not a thing to throw, the reach's step first) */
        return;
    }
    hub_map (0);
    n = cmd.n ? cmd.n : 12;
    dx = HUB->ireq.x - HUB->pc_x;
    dy = HUB->ireq.y - HUB->pc_y;
    d = span ();
    sayf ("\nYou throw the %s %s", cmd.obj.word[0] ? cmd.obj.word : "thing", head_name[cmd.dir]);
    if (!d) {
        sayf (": it strikes the wall and falls at your feet.");
    } else {
        sayf (HUB->ireq.qty == TH_SHORT ? ", but it falls short, %u square%s off." : d < n ?
            ": it strikes the wall and falls %u square%s off." : ": it lands %u square%s off.", d, d == 1 ? "" : "s");
    }
    after (was, 20, IO_DROP);                   /* (Its time; a light thrown, the world's look in the new light) */
}

void __fastcall__ s_fights (unsigned char h)
{
    if (h == H_CHARGE) {
        charge ();
    } else if (h == H_ENGAGE) {
        engage ();
    } else if (h == H_SPLIT) {
        split ();
    } else {
        throw_way ();
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)

/* H_CHARGE (the plan's batch B1, the fights'): a run at the nearest hostile ahead, a war cry, a blow (part 24's) */
static void do_charge (void)
{
    fights (H_CHARGE);
}

/* H_THROW_WAY (B1's): a thing thrown a way, its mark squares along the line (part 24's) */
static void do_throw_way (void)
{
    fights (H_THROW_WAY);
}

/* H_ENGAGE (B1's): closed with a denizen, into its zone, no blow (part 24's) */
static void do_engage (void)
{
    fights (H_ENGAGE);
}

/* H_SPLIT (B1's): two foes attacked in turn (part 24's) */
static void do_split (void)
{
    fights (H_SPLIT);
}

/* H_SAY (the plan's batch B2, the noise's): words said aloud, heard by their loudness (I_NOISE); H_WAKE (B3's, the
** minds'): oneself woken, a sleeper woken (SW_WAKE); H_TAUNT: a denizen goaded from its post (SW_TAUNT); H_ORATE: a
** speech to all in earshot (P_ORATE).  minds.c's, part 27 */
static void do_say (void)
{
    minds (H_SAY);
}

static void do_wake (void)
{
    minds (H_WAKE);
}

static void do_taunt (void)
{
    minds (H_TAUNT);
}

static void do_orate (void)
{
    minds (H_ORATE);
}

/* The denizens' commands, by their handler's number (H_FIGHT ... H_ORATE) */
void __fastcall__ s_dens_do (unsigned h)
{
    switch ((unsigned char) h) {
    case H_CHARGE:
        do_charge ();
        break;
    case H_THROW_WAY:
        do_throw_way ();
        break;
    case H_ENGAGE:
        do_engage ();
        break;
    case H_SPLIT:
        do_split ();
        break;
    case H_SAY:
        do_say ();
        break;
    case H_WAKE:
        do_wake ();
        break;
    case H_TAUNT:
        do_taunt ();
        break;
    case H_ORATE:
        do_orate ();
        break;
    case H_FIGHT:
        if (cmd.action == VB_GRAPPLE_OBJECT) {
            grip_go (cmd.aux);                  /* (A grapple's move: blows.c's, HOUSE.md's grappling) */
            break;
        }
        do_fight ();
        break;
    case H_MISSILE:
        if (cmd.action == VB_THROW_OBJECT && cmd.dir <= H_NW) {
            do_throw_way ();                    /* ("Throw the dagger north": a way, not a mark, B1's) */
            break;
        }
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
