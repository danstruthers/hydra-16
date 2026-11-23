/*
** dq0null.c - Dingus Quest Zero's null referee: a ruleset of flat odds, a task of the game's (dq0null SEG ROLE DIR,
** started by the UI as dq0 -r null asks), answering every one of rs.h's requests in the hub's slots as dq0harn does,
** with no skills, no attributes and no book: a test's chance its steps' alone (60 and 5 a step, a fifth of either
** band critical), a blow's the same, harm a flat few points, no fatigue, no shock, no bleeding.  It's there to show
** the game asks a ruleset nothing but rs.h's requests (RULESETS.md): the tests that are meaning checks (tests.js's
** meaning: true) run with it too (RULES=null node sim/test.js).  The house's rules it applies as every referee does
** (../dq0/rshouse.h: the tools that dig, a notice's steps by the pace, the state's words, the bearings' own dice).
**
** Its character: any file of name, occupation, pace and language lines (hmchar's will do: its other lines passed
** over), or none (RS_LOAD_PC's path empty: it makes its own; RS_HELLO names no character generator); written back
** the same, with its wounds.  The creatures' sheets are a byte each (their wounds), kept in the save folder's
** beasts.dat as dq0harn keeps its own: a save is its ruleset's.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "../dq0/referee.h"

#define BASE            60                      /* A test's chance with no steps, percent */
#define TOUGH           10                      /* The wounds the character can take (dead at them) */
#define BTOUGH          6                       /*   and a creature */
#define NSHEETS         256                     /* (The creatures' records: the denizens' task's numbers) */

static char name[16] = "Dingus", calling[20] = "Wanderer";
static char tongues[120] = " common";           /* The tongues it speaks, a space before each, and the scripts it
                                                **   knows, a * before each (dq0harn's way: kept, not used; as many as its
                                                **   store's, ../dq0harn/dq0harn.h's TONGUES_N: more refused, RS_LOAD_PC 2) */
static unsigned char gait = GA_WALK, watch;     /* (Its file's pace and watchfulness: the world's to set) */
static unsigned char ntongue;                   /* (Its tongues and scripts past its own, read) */
static unsigned char wounds;                    /* The character's, and each creature's (by its record) */
static unsigned char tended;                    /* (The character's wounds tended: slice 4.3's simple way) */
unsigned long pc_out;                           /* (The character senseless till then, a choke's minute: the minds' to
                                                **   know it helpless, RS_ROUSE's to answer; 0 none.  dq0harn's name,
                                                **   the tests' dq0PcShock's) */
static unsigned rest_m;                         /* (Its rest toward the next wound healed: minutes, RS_TIME's) */
static char news[72];                           /* (What time did to it: RS_DESCRIBE b 4's) */
static unsigned char sheet[NSHEETS];
static unsigned char sround[NSHEETS];           /* (Each creature's rounds fought, toward its tiring: the null's simple
                                                **   way, 2026-10-31: a level each 30, a step the worse in its tests; */
static unsigned slast[NSHEETS];                 /*   the clock's minute of its last, 30 rounds rested off each ten
                                                **   minutes since) */
#define TIRE_N          30
static unsigned char aden[NACTORS];             /* (Each actor's record) */
static char hurt[80];                           /* The last harm's words */
static const unsigned char step[NGAITS] = { 45, 30, 15, 8, 4, 3 };    /* Tenths a square at each pace */

#include "../dq0/rshouse.h"                   /* (The house's rules, every referee's: house.c's, ../dq0/rf/) */

/* Words into the news (rshouse.h's want_say and air_say: the house's hunger and thirst, and air) */
void __fastcall__ want_say (const char* s)
{
    if (strlen (news) + strlen (s) < sizeof news) {
        strcat (news, s);
    }
}
void __fastcall__ air_say (const char* s)
{
    want_say (s);
}

/* ---- Its afflictions, the simple way (2026-10-31: HOUSE.md's poison and disease, the null ruleset's): a poison taken
** sickens it a while (a dart's or a smear's half an hour a point of its dose, spoiled food's two hours), a disease
** caught on a flat test (the flux) shows a day on and lasts three; neither kills; told as they come and go */
static unsigned aff_m, sick_m;                  /* (A poison's minutes left; a disease's, its last three days the fever's) */

static signed char __fastcall__ flat (signed char steps);

static void __fastcall__ aff_null (struct req* r)
{
    unsigned m = RS_TIME_MINUTES (*r);
    unsigned char k = RS_TIME_AFF (*r);

    if (k == AK_CURE || k == AK_REMEDY) {       /* (A remedy taken, 2026-11-02: the poison's sickness, or the
                                                **   disease's, gone) */
        if (k == AK_CURE ? aff_m : sick_m) {
            want_say (" The remedy works: the sickness passes.");
        } else {
            want_say (" It does nothing for you.");
        }
        if (k == AK_CURE) {
            aff_m = 0;
        } else {
            sick_m = 0;
        }
    } else if (k == AK_FLUX) {
        if (!sick_m && !G_OK (flat (RS_TIME_DOSE (*r) > 1 ? -4 : 0))) {
            sick_m = 5760;                      /* (Four days: a day hidden, three of fever) */
        }
    } else if (k && k < NAK) {
        if (!aff_m) {
            want_say (k == AK_FOOD ? " Your belly cramps: the food was bad." : " You feel sick: there was poison in it.");
        }
        aff_m = k == AK_FOOD ? 120 : 30 * (RS_TIME_DOSE (*r) ? RS_TIME_DOSE (*r) : 1);
    } else if (aff_m) {
        if (m >= aff_m) {
            aff_m = 0;
            want_say (" The sickness passes.");
        } else {
            aff_m -= m;
        }
    }
    if (sick_m && m) {
        k = sick_m > 4320;
        sick_m = m >= sick_m ? 0 : sick_m - m;
        if (k && sick_m <= 4320) {
            want_say (" Your belly gripes and cramps: you have the flux.");
        } else if (!sick_m) {
            want_say (" The sickness leaves you.");
        }
    }
}

/* ---- The flat odds: a d100 at most BASE and 5 a step (5 to 95) a success, critical in the fifth of its band nearest
** the end (the low rolls of a success, the high of a failure) */

static signed char __fastcall__ flat_r (signed char steps, unsigned char r)
{
    int c = BASE + STEP_PCT * steps;

    c = c < 5 ? 5 : c > 95 ? 95 : c;
    if (r <= c) {
        return r <= c / 5 ? G_WELL : G_PASS;
    }
    return r > 100 - (100 - c) / 5 ? G_BADLY : G_FAIL;
}

static signed char __fastcall__ flat (signed char steps)
{
    return flat_r (steps, 1 + random (100));
}

/* Health in words any ruleset can say, by the wounds against the toughness */
static unsigned char __fastcall__ health (unsigned char w, unsigned char t)
{
    return w >= t ? H_DEAD : !w ? H_FINE : 3 * w < t ? H_HURT : 3 * w < 2 * t ? H_BADLY : H_CRITICAL;
}

/* The character's state in the hub's header: the status line's words and its summary */
static void status (void)
{
    struct rs_summary* s = &HUB->pcsum;

    strcpy (HUB->pcstatus, wounds >= TOUGH ? "DEAD" : pc_out > HUB->now ? "OUT" : wounds ? "HURT" : "");
    memset (s, 0, sizeof *s);
    s->cond = wounds >= TOUGH ? C_DEAD : pc_out > HUB->now ? C_UNCONSCIOUS : 0;   /* (Senseless till it comes round: the
                                                **   minds' to know it helpless, as dq0harn's) */
    s->health = health (wounds, TOUGH);
    s->step = step[GA_WALK];
    memcpy (s->gstep, step, sizeof step);
    s->attacks = 1;
    s->sight = s->smell = 5;
    s->hearing = HUB->watch & WA_LISTEN ? 7 : 5;    /* (Listening as it goes: two better, the house's) */
    s->nightvision = 1;
    s->morale = 10;
    s->power = 10;
    s->mapmind = 50;                            /* (Its map in mind, as dq0harn's of a middling mind: half of what it
                                                **   sees kept, mapping as it goes, a tenth lost every forty minutes) */
    s->mapfade = 40;
}

/* What the character strikes with (the hub's pc_strike): its fists, a foot, the head (bare 1), the thing named
** (SK_OF: its kind's name, the catalogue's), or the first thing it holds (the words the UI knows: RULESETS.md) */
static unsigned char bare;
static char wname[sizeof ((struct kind*) 0)->name];

static const char* weapon (void)
{
    unsigned char k, s = HUB->pc_strike;

    if (SK_IS_KIND (s)) {
        bare = 0;
        hub_map (KIND_BANK);
        memcpy (wname, CATALOGUE[SK_KIND (s)].name, sizeof wname);
        hub_map (0);
        return wname;
    }
    bare = 1;
    if (s == SK_KICK) {
        return "foot";
    }
    if (s == SK_HEAD) {
        return "head";
    }
    for (k = 0; s == SK_BEST && k < NTOOLS; ++k) {
        if (tools[k][0] && (tflag[k] & T_HELD)) {
            bare = 0;
            return tools[k];
        }
    }
    return "fists";
}

/* Whether it speaks tongue t */
static unsigned char __fastcall__ speaks (const char* t)
{
    char* p = tongues;
    unsigned char n = strlen (t);

    while ((p = strstr (p, t)) != 0) {
        if (p[-1] == ' ' && (!p[n] || p[n] == ' ' || p[n] == '=')) {
            return 1;
        }
        ++p;
    }
    return 0;
}

/* ---- The character's file: its lines read (name, occupation, pace, language, wounds), the rest passed over */

static int fd;
static char buf[128], line[64];
static unsigned char bn, bi;

static char* get_line (void)
{
    unsigned char n = 0;
    int k;
    char c;

    for (;;) {
        if (bi == bn) {
            bi = bn = 0;
            if ((k = read (fd, buf, sizeof buf)) <= 0) {
                if (!n) {
                    return 0;
                }
                break;
            }
            bn = k;
        }
        c = buf[bi++];
        if (c == '\n') {
            break;
        }
        if (c != '\r' && n < sizeof line - 1) {
            line[n++] = c;
        }
    }
    line[n] = 0;
    return line;
}

static void __fastcall__ load_pc (struct req* r)
{
    char* l;

    RS_LOAD_PC_BAD (*r) = 0;
    if (r->text[0]) {
        if ((fd = open (r->text, O_RDONLY)) < 0) {
            RS_LOAD_PC_BAD (*r) = 1;
            return;
        }
        bn = bi = 0;
        while ((l = get_line ()) != 0) {
            if (!strncmp (l, "name ", 5)) {
                strncpy (name, l + 5, sizeof name - 1);
            } else if (!strncmp (l, "occupation ", 11)) {
                strncpy (calling, l + 11, sizeof calling - 1);
            } else if (!strncmp (l, "pace ", 5) && strlen (l) >= 8) {
                gait = l[5] - '0' < NGAITS ? l[5] - '0' : GA_WALK;
                watch = l[7] - '0';
            } else if (!strncmp (l, "wounds ", 7)) {
                wounds = atoi (l + 7);
            } else if (!strncmp (l, "language ", 9) || !strncmp (l, "script ", 7)) {
                l = *l == 's' ? (l[5] = ' ', l[6] = '*', l + 5) : l + 8;
                if (++ntongue <= 8 && strlen (tongues) + strlen (l) < sizeof tongues) {   /* (Eight past its own at most:
                                                **   dq0harn.h's TONGUE_LINES) */
                    strcat (tongues, l);
                } else {
                    RS_LOAD_PC_BAD (*r) = 2;              /* (More than it can hold: refused, loudly, the UI's, never cut short) */
                }
            }
        }
        close (fd);
    }
    tp = r->text;
    put (name);
    put ("\n");
    put (calling);
    RS_LOAD_PC_GAIT (*r) = gait;
    RS_LOAD_PC_WATCH (*r) = watch;
}

static void __fastcall__ put_line (const char* a, const char* b)
{
    write (fd, a, strlen (a));
    write (fd, b, strlen (b));
    write (fd, "\n", 1);
}

/* The character written (r's text a path), and the creatures' sheets (the save folder's beasts.dat) */
static void __fastcall__ save_pc (struct req* r)
{
    static char b[16];
    char* p;
    char* e;
    unsigned char k;

    RS_SAVE_FAILED (*r) = 1;
    if ((fd = open (r->text, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    put_line ("null 1", "");
    put_line ("name ", name);
    put_line ("occupation ", calling);
    strcpy (b, "0 0 0");                        /* (The pace and watchfulness, the hub's: dq0harn's line) */
    b[0] += HUB->gait;
    b[2] += HUB->watch;
    put_line ("pace ", b);
    put_line ("wounds ", itoa (wounds, b, 10));
    for (p = tongues + 7; *p; p = e) {          /* (Its tongues but common; its scripts) */
        e = strchr (p + 1, ' ');
        e = e ? e : p + strlen (p);
        k = p[1] == '*' ? 2 : 0;
        strcpy (line, k ? "script " : "language");
        strncat (line, p + k, e - p - k);
        put_line (line, "");
    }
    close (fd);
    RS_SAVE_FAILED (*r) = 0;
}

/* The creatures' sheets read back (back) or written, in the save folder's beasts.dat (the skeleton's rf_sheets: the
** file opened by its sheets_open) */
void __fastcall__ rf_sheets (unsigned char back)
{
    if ((fd = sheets_open (back)) >= 0) {
        if (back) {
            read (fd, sheet, sizeof sheet);
            read (fd, sround, sizeof sround);   /* (A save before them: none, rested) */
            read (fd, slast, sizeof slast);
        } else {
            write (fd, sheet, sizeof sheet);
            write (fd, sround, sizeof sround);
            write (fd, slast, sizeof slast);
        }
        close (fd);
    }
}

/* ---- Harm (RS_HARM): a point each 10 feet of a fall, two a trap's strike or a burn, a point a blow gone wrong */

static void __fastcall__ wound_words (unsigned char n)
{
    put (n >= 4 ? "a grave wound" : n >= 2 ? "a wound" : "a light wound");
}

static void __fastcall__ harm (struct req* r)
{
    struct rs_effect* e = (struct rs_effect*) RS_HARM_EFFECTS (*r);
    unsigned char n, t = r->actor ? BTOUGH : TOUGH;
    unsigned char* dw = r->actor ? &sheet[aden[(r->actor - 1) & 7]] : &wounds;   /* (The character's, or a creature's:
                                                **   its harm by the same rule, the one path's step, 2026-11-23) */

    memset (RS_HARM_EFFECTS (*r), 0, sizeof RS_HARM_EFFECTS (*r));
    tp = hurt;
    tend = hurt + sizeof hurt - 1;
    *tp = 0;
    RS_HARM_BAND (*r) = 0;
    RS_HARM_OUT (*r) = 0;
    if (r->other || *dw >= t || (r->actor && RS_HARM_CAUSE (*r) == HC_STRUCK)) {
        r->result = 1;
        return;
    }
    r->result = 0;
    n = RS_HARM_CAUSE (*r) == HC_FALL ? RS_HARM_FEET (*r) / 10 : RS_HARM_CAUSE (*r) == HC_STRUCK ? 1 : 2;
    if (!n) {
        return;
    }
    *dw += n;
    e->target = r->actor;
    e->kind = *dw >= t ? X_DEATH : X_WOUND;
    e->band = RS_HARM_BAND (*r) = n >= 4 ? 3 : n >= 2 ? 2 : 1;
    e->phrase = 1;
    e->cond = *dw >= t ? C_DEAD : 0;
    e->x[0] = 0;                                /* (Where: none said, the null's) */
    e->x[1] = AS_BLUNT;
    wound_words (n);
}

/* ---- Shields (2026-10-29): the shields' keys (the catalogue's tools: a round shield, a buckler, a knight shield, a
** kite and a tower shield) and their words; each actor's held (RS_EQUIP 2's marks, the world's for an actor: its row
** plus 1, 0 none); the character's by its things held; each one's shield free to block again then (a round on: the
** character's first, then each actor's) */
static const char shk[5][8] = { "shield", "buckler", "kshield", "kite", "tower" };
static const char* const shw[5] = { "shield", "buckler", "knight shield", "kite shield", "tower shield" };
static unsigned char ashd[NACTORS];
static unsigned long sfree[NACTORS + 1];

/* The shield key n letters at k is (its row plus 1), or 0 none */
static unsigned char __fastcall__ shield_key (const char* k, unsigned char n)
{
    unsigned char j;

    for (j = 0; j < 5; ++j) {
        if (strlen (shk[j]) == n && !memcmp (shk[j], k, n)) {
            return j + 1;
        }
    }
    return 0;
}

/* Actor k's things told (RS_EQUIP 2's marks: < the left hand, > the right, * both, + worn, each key's quality after
** it): the shield it holds */
static void __fastcall__ den_shield (unsigned char k, const char* t)
{
    unsigned char m, n;

    ashd[k] = 0;
    while (*t) {
        m = *t++;
        for (n = 0; t[n] >= 'a'; ++n) {
        }
        if (m != '+' && !ashd[k]) {
            ashd[k] = shield_key (t, n);
        }
        t += n;
        if (*t >= '0' && *t <= '9') {
            ++t;
        }
    }
}

/* The character's shield held (its row plus 1), or 0 */
static unsigned char pc_shield (void)
{
    unsigned char k, j;

    for (k = 0; k < NTOOLS; ++k) {
        if (tools[k][0] && (tflag[k] & T_HELD) && (j = shield_key (tools[k], strlen (tools[k]))) != 0) {
            return j;
        }
    }
    return 0;
}

/* ---- Fights (RS_ATTACK): a blow a flat test (a prone defender four steps the easier, each other foe two), a hit
** two wounds (a critical one four); a creature wounded that stands holds its ground on a flat test (a shy one four
** steps the worse) or flees.  A shield held two steps the harder for a blow at its holder, once a round, the null's
** simple way (2026-10-29): a blow that would have hit but for it blocked, told so (AF_SHIELD, its words the shield's).  At a thing: a flat 6 of impact, less its least resistance to a blow.
** A missile's (RS_MISSILE, slice 4.7's, the null's simple way): the same flat test, two steps the worse a range's band
** past the short, four behind cover, two the easier unaware of it; a stray's a hit, no test; its words the kind's name */

/* A creature's tiring (the null's simple way, 2026-10-31: every actor's): sheet n's rounds rested off by the clock
** (30 each ten minutes since its last), and a round more (w 1: a round of a fight); its fatigue levels after */
static unsigned char __fastcall__ tired (unsigned char n, unsigned char w)
{
    unsigned m = (unsigned) (HUB->now / 600), d = m - slast[n];

    if (d >= 10) {
        d /= 10;
        sround[n] = d >= TIRE_N || d * TIRE_N >= sround[n] ? 0 : sround[n] - d * TIRE_N;
    }
    if (w) {
        if (sround[n] < 255) {
            ++sround[n];
        }
        slast[n] = m;
    }
    return sround[n] / TIRE_N;
}

/* A grapple (RS_GRAPPLE: HOUSE.md's grappling, 2026-11-09), the null's simple way: a flat test at the steps (the held
** one's try, pinned, four steps the worse); a throw and a pin lay it down (no hurt), the third round of a choke leaves it
** senseless a minute */
static void __fastcall__ grapple (struct req* r)
{
    struct rs_effect* e = (struct rs_effect*) RS_ATTACK_EFFECTS (*r);
    unsigned char m = RS_ATTACK_MOVE (*r), h = RS_ATTACK_HOLD (*r), o;
    signed char g = flat (RS_ATTACK_STEPS (*r) - (m == GM_BREAK && (h & GH_PIN) ? 4 : 0));

    o = m == GM_BREAK ? (G_OK (g) ? O_FREE : O_STRUGGLES) : !G_OK (g) ? (m == GM_HOLD ? O_SLIP : O_RESISTS) :
        m == GM_HOLD ? O_HOLD : m == GM_THROW ? O_THROWN : m == GM_PIN ? O_PINNED : O_CHOKED;
    RS_ATTACK_OUTCOME (*r) = o;
    e->target = r->other;
    if (o == O_THROWN || o == O_PINNED) {
        e->kind = X_COND_ON;
        e->cond = C_PRONE;
    } else if (o == O_CHOKED && (h & ~GH_PIN) >= 2) {
        e->kind = X_COND_ON;
        e->cond = C_UNCONSCIOUS;
        RS_ATTACK_LEFT (*r) = FF_D_DOWN;
        RS_ATTACK_OUT (*r) = 600;
        if (!r->other) {
            pc_out = HUB->now + 600;             /* (The character: senseless the minute, its summary's) */
        }
    }
    put ("\n\n\n");
}

static void __fastcall__ attack (struct req* r)
{
    struct rs_effect* e = (struct rs_effect*) RS_ATTACK_EFFECTS (*r);
    unsigned char a = r->actor, d = r->other, f = RS_ATTACK_FLAGS (*r), n, k, t, w, sh = 0, rr = 0;
    unsigned char* dw;
    unsigned mw = RS_ATTACK_FLAGS (*r);
    unsigned char mk = RS_ATTACK_MISSILE (*r);
    signed char g = 0, st = mw & RS_MISSILE ? RS_ATTACK_STEPS (*r) - 2 * (RS_ATTACK_BAND (*r) & 3) - (mw & MS_COVER ? 4 : 0) +
        (mw & MS_UNAWARE ? 2 : 0) : RS_ATTACK_STEPS (*r) - 2 * RS_ATTACK_OUTNUMBERED (*r) + (f & RS_ASLEEP ? 2 : 0);   /* (A sleeper struck at: two
                                                **   steps easier, as one unaware of a shot, 2026-11-02) */
    signed char ht = RS_HEART (r);              /* (A creature's heart: its courage's steps, the denizens' field and
                                                **   bonds, HOUSE.md's morale) */

    for (k = 1, t = RS_ATTACK_RESIST (*r)[0]; k < 3; ++k) {      /* (At a thing: its least resistance to a blow) */
        t = RS_ATTACK_RESIST (*r)[k] < t ? RS_ATTACK_RESIST (*r)[k] : t;
    }
    memset (RS_ATTACK_EFFECTS (*r), 0, sizeof RS_ATTACK_EFFECTS (*r));
    RS_ATTACK_WHOSE (*r) = RS_ATTACK_AF (*r) = RS_ATTACK_LEFT (*r) = 0;
    RS_ATTACK_DEFENCE (*r) = D_AUTO;
    RS_ATTACK_OUT (*r) = 0;
    if (d == RS_THING) {
        RS_ATTACK_HARM (*r) = t < 6 ? 6 - t : 0;
        put (weapon ());
        RS_ATTACK_STRUCK (*r) = bare ? 2 : 0;               /* (A part of it struck the thing) */
        return;
    }
    if ((a && d && !(mw & MS_STRAY)) || (a ? sheet[aden[(a - 1) & 7]] >= BTOUGH : wounds >= TOUGH)) {
        RS_ATTACK_OUTCOME (*r) = 0xFF;                       /* (Creatures fighting each other: none, but a missile's stray at one
                                                **   beside its mark, 2026-10-30; the attacker dead) */
        return;
    }
    if (pc_out > HUB->now && (!a || (!d && !(f & RS_KILLER)))) {
        RS_ATTACK_OUTCOME (*r) = 0xFF;                       /* (The character senseless: it can't strike, and a blow at it is
                                                **   held, the minds' to say, but a killer's, as dq0harn's) */
        return;
    }
    dw = d ? &sheet[aden[(d - 1) & 7]] : &wounds;
    t = d ? BTOUGH : TOUGH;
    if (mw & RS_GRAPPLE) {
        grapple (r);
        goto healths;
    }
    if (!(mw & MS_STRAY)) {
        st += a ? -tired (aden[(a - 1) & 7], 0) : tired (aden[(d - 1) & 7], 0);    /* (A creature tired, the worse at
                                                **   it: a step a level, attacking or defending) */
    }
    if (!(f & RS_PRONE_A)) {                    /* (The blow's die; its holder's shield, if it's free this round) */
        k = d ? ((d - 1) & 7) + 1 : 0;
        if ((sh = d ? ashd[k - 1] : pc_shield ()) != 0 && HUB->now >= sfree[k]) {
            sfree[k] = HUB->now + 100;
        } else {
            sh = 0;
        }
        st += f & RS_PRONE_D ? 4 : 0;
        g = flat_r (st - (sh ? 2 : 0), rr = 1 + random (100));
        sh = !G_OK (g) && sh && G_OK (flat_r (st, rr)) ? sh : 0;  /* (Blocked: it would have hit but for it) */
        if ((mw & (RS_MISSILE | MS_STRAY)) == (RS_MISSILE | MS_STRAY)) {
            g = G_PASS;                           /* (A missile's stray: a hit) */
            sh = 0;
        }
    }
    if (a && (mw & RS_MISSILE)) {
        hub_map (KIND_BANK);                    /* (A creature's missile: its kind's name) */
        memcpy (wname, CATALOGUE[mk < NKINDS ? mk : 0].name, sizeof wname);
        hub_map (0);
    }
    put (a ? (mw & RS_MISSILE ? wname : "") : weapon ());   /* (Its words: the attacker's weapon, the defender's, the
                                                **   strike's) */
    put ("\n");
    put (sh ? shw[sh - 1] : d ? "" : weapon ());
    put ("\n");
    if (f & RS_PRONE_A) {
        RS_ATTACK_OUTCOME (*r) = O_RISE;                     /* (Fallen: it gets up, its turn spent) */
    } else if (sh) {
        RS_ATTACK_OUTCOME (*r) = O_BLOCKED;
        RS_ATTACK_AF (*r) = AF_SHIELD;
    } else if (!G_OK (g)) {
        RS_ATTACK_OUTCOME (*r) = O_MISS;
    } else {
        RS_ATTACK_OUTCOME (*r) = O_HIT;
        n = g == G_WELL ? 4 : 2;
        w = *dw + n;
        *dw = w > t ? t : w;
        e->target = d;
        e->kind = *dw >= t ? X_DEATH : X_WOUND;
        e->band = n >= 4 ? 3 : 2;
        e->phrase = 3;
        e->cond = *dw >= t ? C_DEAD : 0;
        e->x[0] = 0;
        e->x[1] = AS_BLUNT;
        wound_words (n);
        if (*dw >= t) {
            RS_ATTACK_LEFT (*r) = FF_D_DEAD;
        } else if (d && !G_OK (flat ((f & 1 ? -4 : 0) + ht))) {
            RS_ATTACK_LEFT (*r) = FF_FLEES;
        }
    }
    if (!(mw & MS_STRAY) && RS_ATTACK_OUTCOME (*r) != O_RISE && !(RS_ATTACK_LEFT (*r) & (FF_D_DEAD | FF_FLEES))) {
        k = aden[((a ? a : d) - 1) & 7];        /* (The creature's round fought; a level more, its courage, the worse
                                                **   by it: failed, it breaks off) */
        n = tired (k, 0);
        if (tired (k, 1) > n && !G_OK (flat (ht - n - 1))) {
            RS_ATTACK_LEFT (*r) |= FF_FLEES;
        }
    }
    put ("\n");
healths:
    RS_ATTACK_HEALTH (*r) = health (a ? sheet[aden[(a - 1) & 7]] : wounds, a ? BTOUGH : TOUGH) | health (*dw, t) << 8;
}

/* ---- The character's state in words (RS_DESCRIBE b 3: the house's, and its wounds) */

static void __fastcall__ state (unsigned char a)
{
    unsigned char h, dark;

    hub_map (SIGHT_BANK);
    dark = SIGHT->dark;
    hub_map (0);
    h = house_pace (HUB->gait, HUB->watch, dark && !(HUB->pc_going & GOING_BOLD));   /* (Said outright in the dark:
                                                **   held to nothing there) */
    put (" (");
    putn (SQ_FT * HUB->round / step[h]);
    put (" feet a round)");
    house_watching (HUB->watch);
    if (a) {
        return;
    }
    put ("\nCondition: ");
    if (wounds >= TOUGH) {
        put ("dead");
    } else if (wounds) {
        put ("hurt (");
        putn (wounds);
        put (" wounds)");
    } else {
        put ("unhurt");
    }
    put (".");
    house_light (HUB->pc_light, HUB->pc_lamp, dark);
}

/* ---- A request answered */

/* (The skeleton's rf_answer, ../dq0/referee.h's: tp and tend at its text, its result 0; the creatures' sheets read back
** before a creature's asked for, written after the character, the skeleton's) */
void __fastcall__ rf_answer (struct req* r)
{
    unsigned char k, m, t;
    unsigned long xt;
    signed char g;

    switch (r->kind) {
    case RS_HELLO:
        RS_HELLO_ROUND (*r) = 100;                            /* (A round: 10 seconds, the game's clock's) */
        put ("Null\n\nstunned\n\n");            /* (No character generator: its own character, if no file's given;
                                                **   C_STUNNED's words, never set; no stance of its own) */
        break;
    case RS_LOAD_PC:
        load_pc (r);
        status ();
        break;
    case RS_NEW_ACTOR:
        if ((k = r->actor - 1) >= NACTORS) {
            RS_NEW_ACTOR_BAD (*r) = 1;
            break;
        }
        aden[k] = RS_NEW_ACTOR_RECORD (*r);
        memset (RS_NEW_ACTOR_SUMMARY (*r), 0, sizeof (struct rs_summary));
        ((struct rs_summary*) RS_NEW_ACTOR_SUMMARY (*r))->cond = (sheet[RS_NEW_ACTOR_RECORD (*r)] >= BTOUGH ? C_DEAD : 0) | (tired (RS_NEW_ACTOR_RECORD (*r), 0) ? C_FATIGUED : 0);
        ((struct rs_summary*) RS_NEW_ACTOR_SUMMARY (*r))->health = health (sheet[RS_NEW_ACTOR_RECORD (*r)], BTOUGH);
        ((struct rs_summary*) RS_NEW_ACTOR_SUMMARY (*r))->attacks = 1;
        ((struct rs_summary*) RS_NEW_ACTOR_SUMMARY (*r))->power = 10;
        break;
    case RS_TEST:
        if (RS_TEST_PURPOSE (*r) == P_DIG) {    /* (The tool to hand that suits it best, every actor's: the house's
                                                **   (rshouse.h's house_dig, the one-path plan's digging, 2026-11-23);
                                                **   a creature's grade by its own dice as moving quietly's) */
            m = RS_TEST_MAT (*r) > MAT_SOIL ? MAT_STONE : RS_TEST_MAT (*r);
            if (!(k = house_dig (r)) || RS_TEST_STEPS (*r) == RS_DIG_QUERY) {
                break;
            }
            if (!DIG_TESTED (m)) {
                g = G_PASS;                     /* (Earth: the tool's rate, untested) */
            } else if (r->actor && r->other != RS_ASKER) {
                QUIET_IN
                g = flat (RS_TEST_STEPS (*r));
                QUIET_OUT
            } else {
                g = flat (RS_TEST_STEPS (*r));
            }
            RS_TEST_GRADE (*r) = g;
            RS_TEST_RATE (*r) = g == G_WELL ? 2 * k : g == G_PASS ? k : g == G_FAIL ? k / 2 : 0;
            break;
        }
        if (RS_TEST_PURPOSE (*r) == P_RISE) {
            RS_TEST_GRADE (*r) = G_PASS;                    /* (Getting up: always, the null ruleset's, slice 4.1b's) */
            break;
        }
        if (RS_TEST_PURPOSE (*r) == P_SWIM) {                   /* (A minute's swimming, the flat odds: its squares, a middling
                                                **   swimmer's; it sinks at a critical failure; its breath two
                                                **   minutes: slice 4.9's) */
            g = RS_TEST_GRADE (*r) = flat (RS_TEST_STEPS (*r));
            RS_TEST_SQUARES (*r) = g == G_WELL ? 27 : g == G_PASS ? 21 : g == G_FAIL ? 6 : 0;
            RS_TEST_BREATH (*r) = 120;
            break;
        }
        if (RS_TEST_PURPOSE (*r) == P_BEARING) {                /* (Its bearings: the flat odds, by their own dice, and a failure's
                                                **   d8 by them too: rshouse.h's, slice 4.2's) */
            bdice ();
            g = RS_TEST_GRADE (*r) = flat (RS_TEST_STEPS (*r));
            RS_TEST_DRIFT (*r) = g < 0 ? random (8) : 0;
            bdice ();
            break;
        }
        if (RS_TEST_PURPOSE (*r) == P_BEARING && RS_TEST_STEPS (*r) >= 64) {  /* (The time kept, rs.h's TIME_STEPS: the flat odds, by the game's
                                                **   dice mixed and put back, a failure's d8 by them too: 2026-10-31) */
            xt = xs_seed;
            xs_seed = (xt * 1103515245UL + 12345UL) ^ 0x54494D45UL;
            g = RS_TEST_GRADE (*r) = flat (RS_TEST_STEPS (*r) - TIME_STEPS);
            RS_TEST_DRIFT (*r) = g < 0 ? random (8) : 0;
            xs_seed = xt;
            break;
        }
        if (((RS_TEST_PURPOSE (*r) == P_SEARCH || RS_TEST_PURPOSE (*r) == P_DISARM) && RS_TEST_HOW (*r) == HIDE_MAGICAL) ||
            (RS_TEST_PURPOSE (*r) == P_PICKLOCK && !r->actor && !has ("picks"))) {
            RS_TEST_CANT (*r) = 1;                      /* (No magic; no lockpicks: the character's, a robber's kind its own, as the
                                                **   HarnMaster referee's, 2026-11-02) */
            break;
        }
        if (RS_TEST_PURPOSE (*r) == P_READ && r->text[0]) {     /* (A writing read: its tongue and its script known, the flat odds: rs.h's,
                                                **   step 9's) */
            *strchr (r->text, ' ') = 0;
            RS_TEST_CANT (*r) = !strchr (tongues, '*') ? 1 : !speaks (r->text) ? 2 : !speaks (r->text + strlen (r->text) + 1) ? 3 : 0;
            RS_TEST_GRADE (*r) = flat (RS_TEST_STEPS (*r));
            *tp = 0;
            break;
        }
        if ((RS_TEST_PURPOSE (*r) == P_PERSUADE || RS_TEST_PURPOSE (*r) == P_INTIMIDATE || RS_TEST_PURPOSE (*r) == P_DECEIVE) && r->text[0] && !speaks (r->text)) {
            RS_TEST_CANT (*r) = 2;
            break;
        }
        if (RS_TEST_PURPOSE (*r) <= P_NOTICE_SMELL) {           /* (By its going and watchfulness: the house's) */
            RS_TEST_STEPS (*r) += house_notice (r, HUB->pc_going, HUB->watch) - (!r->actor && HUB->pc_task.kind == LT_SLEEP ? BASE / 10 : 0);
        }                                       /* (Asleep, at half, 2026-11-06: HarnMaster's, Skills 11) */
        if ((RS_TEST_PURPOSE (*r) == P_STEALTH || RS_TEST_PURPOSE (*r) == P_DIG) && r->other != RS_ASKER) {
            QUIET_IN                            /* (Moving quietly, by its own dice: the house's) */
            RS_TEST_GRADE (*r) = flat (RS_TEST_STEPS (*r));
            QUIET_OUT
            *tp = 0;
            break;
        }
        if (RS_TEST_PURPOSE (*r) == P_APPRAISE && RS_TEST_CRAFT (*r) < NAP) {  /* (A thing judged: the first answer kept, HarnMaster referee's) */
            RS_TEST_MASTERY (*r) = 0;
            JUDGE_IN (RS_TEST_ITEM (*r), 0)
            RS_TEST_GRADE (*r) = flat (RS_TEST_STEPS (*r));
            JUDGE_OUT
            *tp = 0;
            break;
        }
        RS_TEST_GRADE (*r) = flat (RS_TEST_STEPS (*r));
        *tp = 0;
        break;
    case RS_ATTACK:
        attack (r);
        status ();
        break;
    case RS_OUTFIT:                             /* (The universal kit only, at an average make, with its 49 pennies:
                                                **   some money left; no occupation, no skills, no tags) */
        RS_OUTFIT_MONEY (*r) = 49 * 4;                         /* (Farthings, 2026-10-28) */
        RS_OUTFIT_SEED (*r) = (unsigned) HUB->seed | 1;
        RS_OUTFIT_LEVEL (*r) = 1;
        RS_OUTFIT_TAGS (*r)[0] = RS_OUTFIT_TAGS (*r)[1] = RS_OUTFIT_HEFT (*r) = 0;       /* (x[2]: its weapons average, D13's) */
        *tp = 0;
        break;
    case RS_WIELD:                              /* (What it would strike with: the first it holds; its hands empty,
                                                **   none better, asked of, than its bare hands) */
        RS_WIELD_WHICH (*r) = RS_WIELD_HELD (*r)[0] ? 1 : RS_WIELD_HELD (*r)[1] ? 2 : 0;
        *tp = 0;
        break;
    case RS_MORALE:
        g = RS_MORALE_GRADE (*r) = flat (RS_MORALE_STEPS (*r));
        RS_MORALE_STATE (*r) = g == G_BADLY ? M_BROKEN : g == G_FAIL ? M_CAUTIOUS : M_STEADY;
        break;
    case RS_CONTEST:                            /* (Two flat tests, the actor's first: who did better) */
        g = RS_CONTEST_GRADE (*r) = flat (RS_CONTEST_STEPS (*r));
        RS_CONTEST_OGRADE (*r) = flat ((signed char) RS_CONTEST_OSTEPS (*r));
        RS_CONTEST_BETTER (*r) = g > (signed char) RS_CONTEST_OGRADE (*r) ? 1 : g < (signed char) RS_CONTEST_OGRADE (*r) ? 2 : 0;
        *tp = 0;
        break;
    case RS_TIME:                               /* (Its minutes for hunger and thirst, rf/want.c's, before its
                                                **   reply's w1; no bleeding: none dies of it; a wound healed by its
                                                **   rest, a tended one's eight hours, an untended one's a day: slice
                                                **   4.3's simple way) */
        r->x[16] = (unsigned char) RS_TIME_MINUTES (*r);
        r->x[17] = RS_TIME_MINUTES (*r) >> 8;
        if (r->actor != 0xFF) {                 /* (The UI's: a drink of still water, the flux's; no air, nothing else,
                                                **   2026-10-31) */
            RS_TIME_AFF (*r) = RS_TIME_FOUL (*r) ? ((RS_TIME_DRINK (*r) & 0x3F) || random (10) == 0 ? AK_FLUX : 0) :
                RS_TIME_TAKEN (*r) ? (RS_TIME_DRINK (*r) & 1 ? AK_CURE : AK_REMEDY) : 0;  /* (A sip of it, no pints, the
                                                **   taste's: a tenth of a drink's chance, 2026-11-11) */
            RS_TIME_DOSE (*r) = 1;
            RS_TIME_AIR (*r) = RS_TIME_FLAME (*r) = 0;
        }
        RS_TIME_DRINK (*r) &= RS_TIME_TAKEN (*r) ? 0 : 0x7F;
        if (!RS_TIME_ABOUT (*r) && wounds) {
            rest_m += RS_TIME_MINUTES (*r);                    /* (At rest: toward a wound healed) */
        }
        RS_TIME_DEAD (*r) = 0;
        RS_TIME_BOUND (*r) = 0;
        RS_TIME_OUT (*r) = 0;
        RS_TIME_NEWS (*r) = 0;
        if (wounds && rest_m >= (tended ? 420 : 1260)) {
            --wounds;
            tended -= tended != 0;
            rest_m = 0;
            strcpy (news, "A wound has healed.");
            RS_TIME_NEWS (*r) = 2;
            status ();
        }
        if (RS_TIME_DROWNED (*r) == 3) {                     /* (Drowned: the world's, slice 4.9's) */
            strcpy (news, "You drown.");
            RS_TIME_DIED (*r) = 1;
            RS_TIME_NEWS (*r) = 2;
        }
        RS_TIME_HEALTH (*r) = health (wounds, TOUGH);
        RS_TIME_NEXT (*r) = HUB->now + 36000UL;
        memcpy (RS_TIME_WANT (*r), &HUB->pc_hunger, 6);    /* (Hunger and thirst, rf/want.c's: no fatigue, the null
                                                **   ruleset's, but dead of it) */
        if (want_of (r, HUB->now) & WANT_DIES) {
            RS_TIME_DIED (*r) = 1;
        }
        memcpy (&HUB->pc_hunger, RS_TIME_WANT (*r), 6);     /* (Asked an hour on, nothing bleeding, unhurt) */
        RS_TIME_MINUTES (*r) = r->x[16] | r->x[17] << 8;
        g = strlen (news);
        aff_null (r);                           /* (Its afflictions, the simple way; the air it breathes, every
                                                **   ruleset's: rf/air.c's, dead of it) */
        if (RS_TIME_AIR (*r) || RS_TIME_FLAME (*r) || air_lv || air_co) {
            k = air_of (RS_TIME_AIR (*r), RS_TIME_FLAME (*r), RS_TIME_MINUTES (*r));
            if (k & 1) {
                wounds = TOUGH;
                RS_TIME_DIED (*r) = 1;
                status ();
            }
            RS_TIME_NEWS (*r) |= k & 4;
        }
        RS_TIME_NEWS (*r) |= strlen (news) != (unsigned char) g ? 2 : 0;
        RS_TIME_DEAD (*r) = 0;
        *tp = 0;
        break;
    case RS_TREAT:                              /* (First aid, the simple way: five minutes a wound, the character's
                                                **   own, each tended once; its heal the sooner) */
        if (r->other || wounds <= tended) {
            RS_TREAT_CANT (*r) = 1;
            put (!r->other ? "You have no wounds that want tending." : HUB->folk[(r->other - 1) & 7] ?
                "They have no wounds you can tend." : "It has no wounds you can tend.");    /* (A person "they": the hub's
                                                **   folk, 2026-11-17) */
        } else if (!RS_TREAT_DONE (*r)) {
            RS_TREAT_TENTHS (*r) = 3000;
            put ("You clean and bind a wound.");
        } else {
            ++tended;
            put (G_OK (flat (0)) ? "It's tended." : "It's badly done.");
        }
        break;
    case RS_EXERT:                              /* (No fatigue: never a level more) */
        RS_EXERT_TIRED (*r) = 0;
        status ();
        break;
    case RS_EQUIP:
        if (r->actor) {                         /* (An actor's things: not this ruleset's, its creatures' blows
                                                **   its own; but the shield it holds, 2026-10-29) */
            if (RS_EQUIP_HOW (*r) == 2) {
                den_shield ((r->actor - 1) & 7, r->text);
            }
            break;
        }
        ++tgen;                                 /* (What's to hand changed: referee.h's) */
        if (RS_EQUIP_HOW (*r) == 2) {
            equip_all (r->text);
        } else if (RS_EQUIP_HOW (*r)) {
            for (k = 0; k < NTOOLS && tools[k][0]; ++k) {
            }
            if (k < NTOOLS) {
                strncpy (tools[k], r->text, sizeof tools[k] - 1);
                tflag[k] = 0;
            }
        } else {
            for (k = 0; k < NTOOLS; ++k) {
                if (!strcmp (tools[k], r->text)) {
                    tools[k][0] = 0;
                    break;
                }
            }
        }
        *tp = 0;
        status ();
        break;
    case RS_HARM:
        harm (r);
        strcpy (r->text, hurt);
        status ();
        break;
    case RS_SAVE:
        save_pc (r);                            /* (The creatures' sheets after it: the skeleton's) */
        break;
    case RS_DESCRIBE:
        if (RS_DESCRIBE_WHAT (*r) == 1) {                        /* (The map's panel: a line) */
            put ("Wounds ");
            putn (wounds);
            put ("/");
            putn (TOUGH);
        } else if (RS_DESCRIBE_WHAT (*r) == 2) {
            put (hurt);
        } else if (RS_DESCRIBE_WHAT (*r) == 4) {
            put (news);                         /* (What time did to it, told once) */
            news[0] = 0;
        } else if (RS_DESCRIBE_WHAT (*r) == 3) {
            status ();
            state (RS_DESCRIBE_BRIEF (*r));
        } else if (RS_DESCRIBE_WHAT (*r) == RD_DEITY) {
            *tp = 0;                            /* (Its deity: none, the null's) */
        } else {
            put (name);
            put (", ");
            put (calling);
            put (" (the null ruleset: flat odds, no skills).\n");
            if (wounds) {
                put ("Wounds: ");
                putn (wounds);
                put (" of ");
                putn (TOUGH);
                put (".");
            } else {
                put ("No wounds.");
            }
        }
        break;
    case RS_ROUSE:                              /* (Its time out come: no shock rolls, the null's; the character's
                                                **   minute out, if it isn't up, the tenths still to go) */
        r->w2 = 0;
        if (!r->actor && pc_out) {
            if (pc_out > HUB->now) {
                r->w2 = (unsigned) (pc_out - HUB->now);
            } else {
                pc_out = 0;
            }
            status ();
        }
        break;
    default:
        r->result = 0xFF;                       /* (Not yet) */
        *tp = 0;
        break;
    }
}

/* Its start (the skeleton's rf_start: its arguments SEG ROLE), its beginning (nothing of its own), and its arguments'
** numbers (atoi's: no game's library here) */
unsigned char __fastcall__ rf_start (int argc, char* argv[])
{
    (void) argv;
    return argc < 3;
}

void rf_begin (void)
{
}

void rf_after (void)                            /* (Its summary kept whole in rf_answer: rf_later never set) */
{
}

unsigned __fastcall__ num (const char* s)
{
    return atoi (s);
}
