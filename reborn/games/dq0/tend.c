/*
** tend.c - (dq0, swapped part 12: swap.inc) wounds over time, the UI's part (slice 4.3: HOUSE.md's wounds over time):
** first aid ("tend my wounds", "bandage my arm", "first aid", "treat the bandit": VB_TEND), the referee's plan for it
** (RS_TREAT: the wound, the treatment, its time) carried out as a timed work (LT_TEND: its first words at once, its
** outcome as its time's up, the referee's; a bleeder's tourniquet a minute at a time till it holds), a bandage it
** used up (the things' IO_USE); what a character in shock can't do refused (the book's: rest, be led away, a half
** move, Combat 14); and what time did to its body told (E_TASK's TE_BODY: the referee's words, RS_DESCRIBE b 4's),
** held as it sleeps and told as it wakes (main.c's E_READY: news_due).
*/

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"
#include "rs.h"
#include "actions.h"

#pragma code-name ("SW12CODE")
#pragma rodata-name ("SW12RODATA")
#pragma local-strings (on)

/* ---- In shock (the book's, Combat 14: it may rest, be led away, or make a half move): what it may still do, every
** other command refused before its things are looked for (resolve.c's) */
static const unsigned char may[] = {
    VB_NONE, VB_OBSERVE_LOCALE, VB_OBSERVE_DIRECTION, VB_OBSERVE_OBJECT, VB_OBSERVE_THROUGH, VB_OBSERVE_AROUND,
    VB_EXAMINE_OBJECT, VB_EXAMINE_LOCALE, VB_DROP_OBJECT, VB_MOVE_DIRECTION, VB_MOVE_TOWARD, VB_FACE_DIRECTION,
    VB_STEP_DIRECTION, VB_SET_PACE, VB_PACE_SHOW, VB_SAY, VB_FIND_OBJECT, VB_LISTEN, VB_SMELL, VB_TARGET_SET,
    VB_TARGET_CLEAR, VB_TARGET_SHOW, VB_WAIT, VB_SLEEP, VB_SIT, VB_LIE_DOWN, VB_STOP, VB_META_STATUS, VB_META_HELP,
    VB_META_INVENTORY, VB_META_MAP, VB_META_SAVE, VB_META_QUIT, VB_ENTER, VB_LEAVE, VB_DEFEND, VB_FLEE,
    VB_FOLLOW_OBJECT, VB_REPEAT, VB_WHERE, VB_DISENGAGE, VB_PASS, VB_SEQUENCE, VB_APPROACH_OBJECT, VB_ALLY_COME,
    VB_ALLY_STAY, VB_ALLY_DISMISS, VB_META_SHEET, VB_META_PAUSE, VB_OBSERVE_IN
};

/* Before a command's things are looked for (resolve.c's): 2, refused, in shock (told); 1, first aid's (its own
** patient found here, tend's); 0, on */
unsigned char s_early (void)
{
    unsigned b = hub_here ();
    unsigned char a = cmd.action, k, r = 0;

    hub_map (0);
    if (HUB->pcsum.cond & C_STUNNED) {
        for (k = 0; k < sizeof may && may[k] != a; ++k) {
        }
        if (k == sizeof may) {
            msg (a == VB_TELL_OBJECT || (a >= VB_INTIMIDATE && a <= VB_JUDGE_SINCERITY) || a >= VB_DECEIVE &&
                a <= VB_BEFRIEND ? "You're in shock: you can't string the words together." :
                "You're in shock: you can do no more than rest, or walk slowly away.");
            r = 2;
        }
    }
    if (!r && a == VB_TEND) {
        r = 1;
    }
    hub_back (b);
    return r;
}

/* ---- First aid */

static const char* const body[] = {             /* (Words for the character's own body: no thing to look for) */
    "", "me", "myself", "self", "wound", "wounds", "injuries", "injury", "cut", "bite", "arm", "arms", "leg", "legs",
    "hand", "foot", "head", "skull", "face", "neck", "shoulder", "elbow", "forearm", "chest", "belly", "stomach",
    "body", "groin", "hip", "thigh", "knee", "calf", "bleeding"
};

static struct {
    unsigned char   other, b, c, x, tries;
    unsigned        t;
} plan;                                         /* (RS_TREAT's plan, for its end) */

/* The patient said: 0 the character, else an actor's entry plus 1; 0xFF none (told why) */
static unsigned char patient (void)
{
    const char* w = cmd.obj.word;
    const struct actor* p = HUB->actor;
    unsigned char k;
    int dx, dy;

    for (k = 0; k < sizeof body / sizeof body[0]; ++k) {
        if (!strcmp (w, body[k]) || cmd.obj.sel == SL_ME) {
            return 0;
        }
    }
    for (k = 0; k < NACTORS; ++k, ++p) {
        if (p->state && p->name[0] && strstr (p->name, w)) {
            dx = (int) p->x - (int) HUB->pc_x;
            dy = (int) p->y - (int) HUB->pc_y;
            if (p->sn != HUB->pc_stratum || dx < -1 || dx > 1 || dy < -1 || dy > 1) {
                sayf ("\nYou'd have to be beside the %s.", w);
            } else if (p->attitude < AT_FRIENDLY && p->doing != DO_DOWN) {
                sayf ("\nThe %s won't let you near its wounds.", w);
            } else {
                return k + 1;
            }
            return 0xFF;
        }
    }
    sayf ("\nYou see no %s here.", w);
    return 0xFF;
}

/* First aid (VB_TEND, overlay 2's more's): begun, the referee's plan, its first words, a timed work (its command kept,
** main.c's reach 3); its time up (reach 4), done, the referee's words, a bandage used up; a bleeder still bleeding, a
** minute more */
void s_tend (void)
{
    unsigned b = hub_here ();
    struct req* r;
    unsigned char k, res;
    char* p;

    hub_map (0);
    r = &HUB->slot[SLOT_UI];
    if (reach != 4) {
        if ((k = patient ()) == 0xFF) {
            goto out;
        }
        r->other = k;
        strcpy (r->text, k ? "" : strstr (cl, "left") ? "left " : strstr (cl, "right") ? "right " : "");
        strncat (r->text, k ? "" : cmd.obj.word, 9);
        r = ask_b (RS_TREAT, 0, 0);
        plan.other = k;
        plan.tries = 0;
        plan.b = r->b;
        plan.c = r->c;
        plan.x = r->x[0];
        plan.t = r->w1;
        res = r->result;
        strcpy (lb, r->text);
        r->state = REQ_FREE;
        msg (lb);
        if (!res) {
            hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
            reach = 3;
            intend2 (I_WAIT, plan.t, LT_TEND << 8 | 1);
        }
        goto out;
    }
    r->other = plan.other;
    r->c = plan.c;
    r->x[0] = plan.x;
    r = ask_b (RS_TREAT, 1, plan.b);
    res = r->result;
    strcpy (lb, r->text);
    r->state = REQ_FREE;
    if (res != 2) {
        if ((p = strchr (lb, '\n')) != 0) {
            *p++ = 0;
        }
        if (res != 1 || !plan.tries++) {
            msg (lb);                           /* (A bleeder bleeding still told once, the tries after it quietly) */
        }
        if (p && *p) {
            things (IO_USE, p);                 /* (A bandage the dressing took) */
        }
        if (res == 1) {
            reach = 3;                          /* (A bleeder bleeding still: a minute more, the book's) */
            intend2 (I_WAIT, 600, LT_TEND << 8 | 1);
        }
    }
out:
    hub_back (b);
}

/* What time did to its body (E_TASK's TE_BODY: task.c's), the referee's words (RS_DESCRIBE b 4: told once); held
** while it sleeps, told as it wakes (news_due, main.c's E_READY); 8 if it's dead of it */
unsigned char s_body_words (void)
{
    unsigned b = hub_here ();
    struct req* r;
    unsigned char d;

    hub_map (0);
    d = (HUB->pcsum.cond & C_DEAD) != 0;
    if (!d && HUB->pc_task.kind == LT_SLEEP) {
        news_due = 1;
    } else {
        news_due = 0;
        r = ask_b (RS_DESCRIBE, 0, 4);
        strcpy (lb, r->text[0] == ' ' ? r->text + 1 : r->text);
        r->state = REQ_FREE;
        if (lb[0]) {
            msg (lb);
        }
        if (d) {
            msg ("You are dead.");
        }
    }
    hub_back (b);
    return d ? 8 : 0;
}
