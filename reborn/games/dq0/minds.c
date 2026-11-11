/*
** minds.c - (part 28, its own: overlay 26's bank, 2026-11-10) words said aloud, a knock, and the minds swayed: the verbs plan's
** batches B2 (the noise) and B3 (the minds), 2026-11-10, DECISIONS.md's "Words said, a knock, the minds swayed".  Each
** does a thing in the world (the user's rule):
**   say, yell, whisper   the words said back, then heard by their loudness (the world's I_NOISE: 1 a whisper, 3 a word,
**                        6 a yell; a sleeper woken by what it hears, the rest turned to it, a foe come to look: the
**                        denizens'); a minded one near and facing the character takes them as talk (SW_TALK)
**   knock                on a door, heard beyond it (3); along a wall, a short search by ear for a hollow, a hidden
**                        door (the world's wall search, SF_KNOCK: P_NOTICE_SOUND, half a minute)
**   wake                 oneself asleep, a stop's; awake, "You're awake."; a sleeper, walked over to first, shaken
**                        awake (SW_WAKE), its attitude deciding what follows
**   taunt                a contest of Rhetoric (SW_TAUNT): a success leaves it hostile, coming at the character
**   orate                one Oratory test, a persuasion of each minded one in earshot who speaks the tongue (SW_PERSUADE
**                        | SWO_ALL), heard (4)
**   pretend              dead or asleep: lying prone, an Acting contest with each foe that senses it (SW_DECEIVE |
**                        SWO_ALL), the fooled taking it for helpless while it lies still in its wait (ten minutes, or
**                        till "stop"); any other pretence a deception of the nearest
** Their words here; the denizens' task the minds (the hub's sway: dq0d.c's s_sway, part 16's provoke and crowd).
*/

#include <string.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "cmds.h"

#pragma local-strings (on)
#pragma code-name ("SW28CODE")
#pragma rodata-name ("SW28RODATA")

extern unsigned char finds;                     /* (main.c's: a search's end awaited) */

#define dealt       (*(struct sway*) (lb + sizeof lb - sizeof (struct sway)))  /* (The reply's copy at lb's end, as
                                                **   main.c's deal keeps it: the words at its start) */

/* A noise the character makes (I_NOISE: how loud, what it is), the world's to carry out (two seconds) */
static void __fastcall__ noise (unsigned char loud, unsigned char what)
{
    struct msg m;                               /* (On the C stack: the main RAM's room) */

    m.kind = I_NOISE;
    m.who = 0;
    I_NOISE_LOUD (m) = loud;
    I_NOISE_WHAT (m) = what;
    m.w1 = m.w2 = 0;
    hub_map (0);
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
}

/* The denizens asked (the hub's sway: how, SW_*, | SW_QUIET nothing said; order, SWO_ALL to all in earshot), as
** main.c's deal asks them, the reply in dealt; a refusal worded (the sway's words: none such, too far, no tongue ...;
** SR_CANT the caller's own).  Its result */
static unsigned char __fastcall__ ask_den (unsigned char how, unsigned char order)
{
    struct sway* s = &HUB->sway;

    hub_map (0);
    s->kind = how & ~SW_QUIET;
    s->actor = order ? 0 : den_meant;
    strcpy (s->word, cmd.obj.word);
    s->prep = cmd.prep;
    s->word2[0] = 0;
    s->order = order;
    hub_call (s, T_DENIZENS);
    memcpy (&dealt, s, sizeof dealt);
    s->state = REQ_FREE;
    lb[0] = 0;
    if (!(how & SW_QUIET) && dealt.result != SR_OK && dealt.result != SR_CANT) {
        sway_words (&dealt);
        msg (lb);
    }
    return dealt.result;
}

/* The one the reply's about, into lb after what's there: "Grub", "the giant rat" ("The" to start a sentence, c) */
static void __fastcall__ who (unsigned char c)
{
    if (dealt.name[0] < 'A' || dealt.name[0] > 'Z') {
        strcat (lb, c ? "The " : "the ");
    }
    strcat (lb, dealt.name);
}

/* Its time, as deal's: the character's */
static void took (void)
{
    if (dealt.t) {
        intend2 (I_WAIT, dealt.t, 0);
    }
}

/* ---- Words said aloud (say, yell, whisper; a line in quotes: "Hello", y"Help!" yelled, w"Psst" whispered, one ending
** in ! yelled): from the line itself (cl: the parser keeps no words); a yell with no words a shout */
static void say_aloud (void)
{
    static const char* const said[3] = { "whisper", "say", "yell" };
    const char* w = cl;
    unsigned char loud = 3, c = *w | 0x20, n, k;
    struct actor* a;
    int dx, dy;

    if (*w == '"') {
    } else if ((c == 'y' || c == 'w') && w[1] == '"') {
        loud = c == 'y' ? 6 : 1;                /* (y"Here!", w"Psst") */
        ++w;
    } else {
        if (c == 'y' || (c == 's' && (w[1] | 0x20) != 'a')) {
            loud = 6;                           /* (yell, shout, scream) */
        } else if (c == 'w') {
            loud = 1;
        }
        while (*w && *w != ' ') {
            ++w;
        }
        while (*w == ' ') {
            ++w;
        }
    }
    if (*w == '"') {
        ++w;
    }
    strncpy (lb, w, 120);
    lb[120] = 0;
    n = strlen (lb);
    while (n && (lb[n - 1] == '"' || lb[n - 1] == ' ')) {
        lb[--n] = 0;
    }
    if (n && lb[n - 1] == '!' && loud == 3) {
        loud = 6;                               /* ("Help!": yelled) */
    }
    if (!n) {
        if (loud < 6) {
            msg (loud == 1 ? "Whisper what? (whisper \"over here\")" : "Say what? (say \"hello\")");
            return;
        }
        msg ("You shout.");
    } else {
        sayf ("\nYou %s, \"%s\"", said[loud / 3], lb);
    }
    noise (loud, NZ_VOICE);
    hub_map (0);
    for (k = 0, a = HUB->actor; n && k < NACTORS; ++k, ++a) {   /* (A minded one near and facing it: its words talk) */
        dx = HUB->pc_x - a->x;
        dy = HUB->pc_y - a->y;
        if (a->state && a->name[0] && a->sn == HUB->pc_stratum && a->doing != DO_SLEEP && a->doing != DO_DOWN &&
            dx >= -loud && dx <= loud && dy >= -loud && dy <= loud && ((bearing (dx, dy) - a->face + 1) & 7) < 3) {
            den_meant = (k + 1) | SWA_MEANT;
            cmd.obj.word[0] = 0;
            if (ask_den (SW_TALK | SW_QUIET, 0) == SR_OK) {   /* (Taken as talk: its words; a beast's, a tongue
                                                **   not its own, nothing) */
                sway_words (&dealt);
                msg (lb);
                took ();
            }
            den_meant = 0;
            break;
        }
    }
}

/* ---- A knock: on a door (a portcullis, a window), walked up to and faced first (the one path for features), heard
** beyond it; along a wall (the one ahead unsaid), a short search by ear (the world's wall search, SF_KNOCK: half a
** minute, a hidden door's hollow heard); on a thing to hand, a rap */
static void knock (void)
{
    unsigned char k = cmd.obj.kind, h;
    struct msg m;

    if (k == OB_WALL) {
        h = (cmd.obj.lies <= H_NW ? cmd.obj.lies : HUB->pc_face) & 6;   /* (Its side: the world's, by the four ways) */
        sayf ("\nYou knock along the %s wall, listening for a hollow...", head_name[believed (h)]);
        noise (2, NZ_KNOCK);
        finds = 0xFF;                           /* (Its end awaited: what it finds told, E_PORTAL's, a search's) */
        m.kind = I_PORTAL;
        m.who = 0;
        I_PORTAL_ACT (m) = PA_SEARCH;
        I_PORTAL_WHICH (m) = I_PORTAL_SEARCH_B (SF_WALL, h) | SF_KNOCK;
        I_PORTAL_TENTHS (m) = 300;
        m.w2 = 0;
        I_PORTAL_BAR (m) = I_PORTAL_STRIKE (m) = 0;
        if (hub_send (&m)) {
            busy = 1;
            t_prompt (0);
        }
        return;
    }
    if (k == OB_DOOR || k == OB_PORTC || k == OB_WINDOW) {
        if (feature (k, FW_NONE)) {             /* (Out of reach: walked up to first, then again) */
            msg (k == OB_DOOR ? "You knock on the door." : "You rap on it.");
            noise (3, NZ_KNOCK);
        }
        return;
    }
    if (cmd.obj.word[0] && things (IO_FIND, cmd.obj.word) == IR_OK) {
        sayf ("\nYou rap on the %s.", cmd.obj.word);
        noise (2, NZ_KNOCK);
        return;
    }
    unknown (U_CANT);
}

/* ---- Wake: oneself (asleep: a stop's, at once, task.c's free_cmd; awake, said so); a sleeper, the one meant beside
** the character, else walked over to first (the reach's, the command again as it's there), shaken awake (SW_WAKE), its
** attitude its words */
static void wake (void)
{
    struct actor* a;
    unsigned char k, best = 0xFF, d, bd = 0xFF;
    int dx, dy;

    if (!cmd.obj.word[0]) {
        if (HUB->pc_task.kind == LT_SLEEP) {
            intend2 (I_STOP, 0, 0);             /* (A stop's: up at once) */
        } else {
            msg ("You're awake.");
        }
        return;
    }
    for (k = 0, a = HUB->actor; k < NACTORS; ++k, ++a) {
        if (!a->state || !a->name[0] || a->sn != HUB->pc_stratum || !name_is (a->name, cmd.obj.word)) {
            continue;
        }
        dx = a->x - HUB->pc_x;
        dy = a->y - HUB->pc_y;
        dx = dx < 0 ? -dx : dx;
        dy = dy < 0 ? -dy : dy;
        d = dx > 0xFE || dy > 0xFE ? 0xFE : dx > dy ? dx : dy;
        if (d < bd) {
            bd = d;
            best = k;
        }
    }
    if (best == 0xFF || (bd > 1 && !den_seen (HUB->actor[best].x, HUB->actor[best].y))) {
        say_no (A_SEE_NO, cmd.obj.word);
        return;
    }
    hub_map (0);
    if (bd > 1) {
        HUB->ireq.x = HUB->actor[best].x;       /* (Walked over to it first, then again: the reach's) */
        HUB->ireq.y = HUB->actor[best].y;
        HUB->ireq.item = 0;
        reach_go ();
        return;
    }
    if (ask_den (SW_WAKE, 0) == SR_OK) {
        strcpy (lb, "You shake ");
        who (0);
        strcat (lb, dealt.now == AT_HOSTILE ? " awake. It comes to with a snarl, and turns on you!" :
            dealt.now <= AT_WARY ? " awake. It starts up, and eyes you warily." : " awake. It blinks up at you.");
        msg (lb);
    } else if (dealt.result == SR_CANT) {
        lb[0] = 0;
        who (1);
        strcat (lb, " isn't asleep.");
        msg (lb);
    }
    took ();
}

/* ---- Taunt: one goaded (SW_TAUNT, a contest of Rhetoric): hostile if it works, coming for the character */
static void taunt (void)
{
    if (ask_den (SW_TAUNT, 0) == SR_OK) {
        strcpy (lb, "You taunt ");
        who (0);
        strcat (lb, G_OK (dealt.grade) ? ". It's goaded: it comes for you!" : dealt.grade == G_BADLY ?
            ". It only laughs at you." : ". It ignores your jeers.");
        msg (lb);
    }
    took ();
}

/* ---- Orate: a speech to all in earshot (SW_PERSUADE | SWO_ALL), heard (4): its words by how it went */
static void orate (void)
{
    unsigned char r;

    noise (4, NZ_VOICE);
    cmd.obj.word[0] = 0;
    if ((r = ask_den (SW_PERSUADE | SW_QUIET, SWO_ALL)) == SR_NONE) {
        msg ("There's no one here to hear you.");
    } else if (r != SR_OK) {
        sway_words (&dealt);                    /* (The nearest's tongue not the character's) */
        msg (lb);
    } else {
        sayf ("\nYou hold forth to those about you, in %s. ", dealt.word2);
        say (G_OK (dealt.grade) ? (dealt.ix ? "Your words carry:" : "Your words carry, but sway no one further.") :
            dealt.grade == G_BADLY ? (dealt.ix ? "Your words go badly:" : "Your words go badly.") : "Your words fall flat.");
        if (dealt.ix == 1) {
            sayf (" one of them seems %s for it.", dealt.grade == G_BADLY ? "the colder" : "the friendlier");
        } else if (dealt.ix) {
            sayf (" %u of them seem %s for it.", dealt.ix, dealt.grade == G_BADLY ? "the colder" : "the friendlier");
        }
        took ();
    }
}

/* ---- Pretend: dead or asleep (or senseless) feigned, lying prone, an Acting contest with each foe that senses the
** character (SW_DECEIVE | SWO_ALL), then a wait lying still (ten minutes; "stop" ends it, as a move does); any other
** pretence ("feign ignorance") a tale spun to the nearest (deceive's) */
static void pretend (void)
{
    unsigned char s = strstr (cl, "sleep") != 0;

    if (!s && !strstr (cl, "dead") && !strstr (cl, "death") && !strstr (cl, "uncon") && !strstr (cl, "sense")) {
        cmd.obj.word[0] = 0;
        deal (SW_DECEIVE);                      /* (Another pretence: a deception, the nearest's) */
        return;
    }
    hub_map (0);
    if (HUB->pc_pos != POS_PRONE) {
        intend2 (I_RISE, POS_PRONE, 0);         /* (Down first: the world's) */
    }
    cmd.obj.word[0] = 0;
    ask_den (SW_DECEIVE | SW_QUIET, SWO_ALL);   /* (SR_NONE: no foe sees it; its words its own) */
    sayf ("\nYou lie still, feigning %s. ", s ? "sleep" : "death");
    lb[0] = 0;
    if (!dealt.iy) {
        strcpy (lb, "No foe is watching you.");
    } else if (!dealt.ix) {
        strcpy (lb, dealt.iy > 1 ? "No one is taken in." : "It isn't taken in.");
    } else {
        who (1);
        strcat (lb, dealt.ix < dealt.iy ? " is taken in, but not all of them are." : dealt.ix > 1 ?
            " and the rest are taken in." : " is taken in.");
    }
    say (lb);
    intend2 (I_WAIT, 6000, 0);                  /* (Ten minutes lying still: "stop" ends it) */
}

void __fastcall__ s_minds (unsigned char h)
{
    switch (h) {
    case H_SAY:
        say_aloud ();
        break;
    case H_KNOCK:
        knock ();
        break;
    case H_WAKE:
        wake ();
        break;
    case H_TAUNT:
        taunt ();
        break;
    case H_ORATE:
        orate ();
        break;
    default:
        pretend ();
    }
}
