/*
** hooks.c - HarnMaster's part of the account of the time between two dungeons (../../dq0/rsmeta.h's acct_* hooks, the engine's
** account's: games/dq0/acct), in the main RAM: the character's sheet as the referee has it (dq0harn.h's names), and each hook,
** its work in an overlay of its own (hacct.h's) but where it's called from the account's overlays (acct_habit, acct_town,
** acct_learnt) or is a word long.
*/

#include <string.h>
#include <hydra.h>
#include <harn.h>
#include "../../dq0/hub.h"
#include "../../dq0/rs.h"
#include "../dq0harn.h"
#include "hacct.h"

#pragma static-locals (on)

/* ---- The sheet: the referee's names (dq0harn.h's), and the lines of the file that are the game's */

harn_char pc;
unsigned char pc_by[HARN_INJMAX];
unsigned char pc_dr[HARN_INJMAX];               /* (Its wounds' dressings, the referee's, read from its "dressed" line,
                                                **   soiled as the days pass, written back: body.c's) */
char dressed[64];                               /* (The "dressed" line, as read and as written: sheet.c's) */
char news[NEWS_N];
struct venom pv;
struct sick pd;
unsigned char afffl, aff_breath;
unsigned rest_m;
char calling[20];
unsigned char used[HARN_NSKILLS];
char tongue[NTONGUES][24];
unsigned char ntongues;

struct wwas was[HARN_INJMAX];
unsigned char wid[HARN_INJMAX], nwas;
unsigned char tended[HARN_INJMAX], ntended, selfnd, unpaid;

/* ---- The hooks */

unsigned char acct_sheet_read (void)
{
    unsigned char r;

    hy_bank (OV_SHEET);
    r = o_sheet_read ();
    if (r) {
        hy_bank (OV_BODY);
        o_dress (0);                            /* (Its dressings, its wounds read: soiled by the days, body.c's) */
    }
    ACCT_CATS ();
    return r;
}

void acct_sheet_write (void)
{
    hy_bank (OV_BODY);
    o_dress (1);
    hy_bank (OV_SHEET);
    o_sheet_write ();
    ACCT_CATS ();
}

/* The wounds as they come out: noted, the bleeding bound, a physician */
unsigned char acct_wounds (void)
{
    unsigned char r;

    hy_bank (OV_BODY);
    o_note ();
    if ((r = o_bind ()) != 0) {
        o_treat ();
    }
    ACCT_CATS ();
    return r;
}

unsigned char acct_mended (void)
{
    unsigned char r;

    hy_bank (OV_BODY);
    r = o_mended ();
    ACCT_CATS ();
    return r;
}

unsigned char acct_day (void)
{
    unsigned char r;

    hy_bank (OV_BODY);
    r = o_day ();
    ACCT_CATS ();
    return r;
}

const char* acct_death (void)
{
    return news;
}

/* Fatigue clear, but what a sickness holds (the plan's; hunger and thirst are the world's, begun afresh with the dungeon) */
void acct_rested (void)
{
    pc.fl = afffl;
    pc.state = HARN_CONSCIOUS;
}

const char* acct_calling (void)
{
    return calling;
}

unsigned char acct_wage (void)
{
    unsigned char r;

    hy_bank (OV_SKILL);
    r = o_wage ();
    ACCT_CATS ();
    return r;
}

/* A day's pence by its social class (the plan's table: slave, serf, unguilded, guilded, noble) */
unsigned char acct_habit (void)
{
    static const unsigned char habits[5] = { 1, 1, 2, 3, 8 };

    return habits[pc.social < 5 ? pc.social : 2];
}

unsigned acct_birthday (void)
{
    return pc.att[HARN_BD];
}

unsigned char acct_tool (unsigned char needs)
{
    unsigned char r = 0;

    hy_bank (OV_SKILL);
    if (needs) {
        r = o_needs ();
    } else {
        o_tool ();
    }
    ACCT_CATS ();
    return r;
}

void acct_first (void)
{
    hy_bank (OV_WORDS);
    o_first ();
    ACCT_CATS ();
}

void acct_worked (void)
{
    hy_bank (OV_SKILL);
    o_work ();
    ACCT_CATS ();
}

void acct_practice (void)
{
    hy_bank (OV_SKILL);
    o_practice ();
    ACCT_CATS ();
}

/* Rhetoric's and Intrigue's EMLs (the market's haggling, the inn's listening) */
void __fastcall__ acct_town (struct between* t)
{
    int e;

    e = harn_eml (&pc, HARN_RHE);
    t->rhet = e < 0 ? 0 : e;
    e = harn_eml (&pc, HARN_NTR);
    t->intr = e < 0 ? 0 : e;
}

/* A tongue or a script opened, a line of its own on the sheet ("script delvic=81", "language sindarin=11") */
unsigned char __fastcall__ acct_learnt (const char* what)
{
    if (ntongues == NTONGUES) {
        return 0;
    }
    strcpy (tongue[ntongues], *what == '*' ? "script " : "language ");
    strcat (tongue[ntongues++], what + (*what == '*'));
    return 1;
}
