/*
** dq0null.c - Dingus Quest Zero's null referee: a ruleset of flat odds, a task of the game's (dq0null SEG ROLE DIR,
** started by the UI as dq0 -r null asks), answering every one of rs.h's requests in the hub's slots as dq0harn does,
** with no skills, no attributes and no book: a test's chance its steps' alone (60 and 5 a step, a fifth of either
** band critical), a blow's the same, harm a flat few points, no fatigue, no shock, no bleeding.  It's there to show
** the game asks a ruleset nothing but rs.h's requests (RULESETS.md): the tests that are meaning checks (tests.js's
** meaning: true) run with it too (RULES=null node sim/test.js).  The house's rules it applies as every referee does
** (../dq0/rshouse.h: the tools that dig, a notice's steps by the pace, the state's words).
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

#define BASE            60                      /* A test's chance with no steps, percent */
#define TOUGH           10                      /* The wounds the character can take (dead at them) */
#define BTOUGH          6                       /*   and a creature */
#define NSHEETS         256                     /* (The creatures' records: the denizens' task's numbers) */
#define NTOOLS          20

static char name[16] = "Dingus", calling[20] = "Wanderer";
static char tongues[32] = " common";            /* The tongues it speaks, a space before each (dq0harn's way) */
static unsigned char gait = GA_WALK, watch;     /* (Its file's pace and watchfulness: the world's to set) */
static unsigned char wounds;                    /* The character's, and each creature's (by its record) */
static unsigned char sheet[NSHEETS];
static unsigned char aden[NACTORS];             /* (Each actor's record) */
static unsigned char restored;
static char hurt[80];                           /* The last harm's words */
char tools[NTOOLS][8];                          /* What's to hand (RS_EQUIP's keys), */
static unsigned char held[NTOOLS];              /*   1 held */
static const unsigned char step[NGAITS] = { 45, 30, 15, 8, 4, 3 };    /* Tenths a square at each pace */

/* ---- Words into a reply */

static char* tp;
static char* tend;

static void __fastcall__ put (const char* s)
{
    while (*s && tp < tend) {
        *tp++ = *s++;
    }
    *tp = 0;
}

static void __fastcall__ putn (int v)
{
    char b[8];

    put (itoa (v, b, 10));
}

#define RS_HOUSE_DIG
#define RS_HOUSE_DIG_TOOL   dig_tool
#define RS_HOUSE_NOTICE
#define RS_HOUSE_STATE
#include "../dq0/rshouse.h"

/* ---- The flat odds: a d100 at most BASE and 5 a step (5 to 95) a success, critical in the fifth of its band nearest
** the end (the low rolls of a success, the high of a failure) */

static signed char __fastcall__ flat (signed char steps)
{
    int c = BASE + STEP_PCT * steps;
    unsigned char r = 1 + random (100);

    c = c < 5 ? 5 : c > 95 ? 95 : c;
    if (r <= c) {
        return r <= c / 5 ? G_CS : G_MS;
    }
    return r > 100 - (100 - c) / 5 ? G_CF : G_MF;
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

    strcpy (HUB->pcstatus, wounds >= TOUGH ? "DEAD" : wounds ? "HURT" : "");
    memset (s, 0, sizeof *s);
    s->cond = wounds >= TOUGH ? C_DEAD : 0;
    s->health = health (wounds, TOUGH);
    s->step = step[GA_WALK];
    memcpy (s->gstep, step, sizeof step);
    s->attacks = 1;
    s->sight = s->smell = 5;
    s->hearing = HUB->watch & WA_LISTEN ? 7 : 5;    /* (Listening as it goes: two better, the house's) */
    s->nightvision = 1;
    s->morale = 10;
    s->power = 10;
}

/* ---- What's to hand: the keys, each after its mark (* held, + worn, a space neither) and a digit after it */

static void __fastcall__ equip_all (const char* keys)
{
    unsigned char k = 0, n, f;

    memset (tools, 0, sizeof tools);
    while (*keys && k < NTOOLS) {
        f = *keys == '*';
        if (*keys < 'a') {
            ++keys;
        }
        for (n = 0; keys[n] >= 'a'; ++n) {
        }
        if (n && n < sizeof tools[0]) {
            memcpy (tools[k], keys, n);
            held[k++] = f;
        }
        keys += n;
        if (*keys >= '0' && *keys <= '9') {
            ++keys;
        }
    }
}

static unsigned char __fastcall__ has (const char* key)
{
    unsigned char k;

    for (k = 0; k < NTOOLS && strcmp (tools[k], key); ++k) {
    }
    return k < NTOOLS;
}

/* What the character strikes with (the hub's pc_strike): its fists, a foot, the head (bare 1), or the first thing it
** holds (the words the UI knows: RULESETS.md) */
static unsigned char bare;

static const char* weapon (void)
{
    unsigned char k, s = HUB->pc_strike;

    bare = 1;
    if (s == SK_KICK) {
        return "foot";
    }
    if (s == SK_HEAD) {
        return "head";
    }
    for (k = 0; s == SK_BEST && k < NTOOLS; ++k) {
        if (tools[k][0] && held[k]) {
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
        if (p[-1] == ' ' && (!p[n] || p[n] == ' ')) {
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

    r->result = 0;
    if (r->text[0]) {
        if ((fd = open (r->text, O_RDONLY)) < 0) {
            r->result = 1;
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
            } else if (!strncmp (l, "language ", 9) && strlen (tongues) + strlen (l + 8) < sizeof tongues) {
                strcat (tongues, l + 8);
            }
        }
        close (fd);
    }
    tp = r->text;
    put (name);
    put ("\n");
    put (calling);
    r->a = gait;
    r->b = watch;
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

    r->result = 1;
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
    for (p = tongues + 7; *p; p = e) {          /* (Its tongues but common) */
        e = strchr (p + 1, ' ');
        e = e ? e : p + strlen (p);
        strcpy (line, "language");
        strncat (line, p, e - p);
        put_line (line, "");
    }
    close (fd);
    r->result = 0;
}

/* The creatures' sheets read back (back) or written, in the save folder's beasts.dat */
static void __fastcall__ sheets (unsigned char back)
{
    static char path[56];

    if (!HUB->savedir[0]) {
        return;
    }
    strcpy (path, HUB->savedir);
    strcat (path, "/beasts.dat");
    if ((fd = back ? open (path, O_RDONLY) : open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        if (back) {
            read (fd, sheet, sizeof sheet);
        } else {
            write (fd, sheet, sizeof sheet);
        }
        close (fd);
    }
}

static void sheets_in (void)
{
    if (HUB->resume && !restored) {
        restored = 1;
        sheets (1);
    }
}

/* ---- Harm (RS_HARM): a point each 10 feet of a fall, two a trap's strike or a burn, a point a blow gone wrong */

static void __fastcall__ wound_words (unsigned char n)
{
    put (n >= 4 ? "a grave wound" : n >= 2 ? "a wound" : "a light wound");
}

static void __fastcall__ harm (struct req* r)
{
    struct rs_effect* e = (struct rs_effect*) r->x;
    unsigned char n;

    memset (r->x, 0, sizeof r->x);
    tp = hurt;
    tend = hurt + sizeof hurt - 1;
    *tp = 0;
    r->b = 0;
    r->w2 = 0;
    if (r->other || wounds >= TOUGH) {
        r->result = 1;
        return;
    }
    r->result = 0;
    n = r->a == HC_FALL ? r->w1 / 10 : r->a == HC_STRUCK ? 1 : 2;
    if (!n) {
        return;
    }
    wounds += n;
    e->target = 0;
    e->kind = wounds >= TOUGH ? X_DEATH : X_WOUND;
    e->band = r->b = n >= 4 ? 3 : n >= 2 ? 2 : 1;
    e->phrase = 1;
    e->cond = wounds >= TOUGH ? C_DEAD : 0;
    e->x[0] = BP_CHEST;
    e->x[1] = AS_BLUNT;
    wound_words (n);
}

/* ---- Fights (RS_ATTACK): a blow a flat test (a prone defender four steps the easier, each other foe two), a hit
** two wounds (a critical one four); a creature wounded that stands holds its ground on a flat test (a shy one four
** steps the worse) or flees.  At a thing: a flat 6 of impact, less its least resistance to a blow */

static void __fastcall__ attack (struct req* r)
{
    struct rs_effect* e = (struct rs_effect*) r->x;
    unsigned char a = r->actor, d = r->other, f = r->w2, n, k, t, w;
    unsigned char* dw;
    signed char g, st = r->steps - 2 * r->c;

    for (k = 1, t = r->x[0]; k < 3; ++k) {      /* (At a thing: its least resistance to a blow) */
        t = r->x[k] < t ? r->x[k] : t;
    }
    memset (r->x, 0, sizeof r->x);
    r->a = r->c = r->grade = 0;
    r->b = D_AUTO;
    r->w2 = 0;
    if (d == RS_THING) {
        r->w1 = t < 6 ? 6 - t : 0;
        put (weapon ());
        r->result = bare ? 2 : 0;               /* (A part of it struck the thing) */
        return;
    }
    if ((a && d) || (a ? sheet[aden[(a - 1) & 7]] >= BTOUGH : wounds >= TOUGH)) {
        r->result = 0xFF;                       /* (Creatures fighting each other: none; the attacker dead) */
        return;
    }
    dw = d ? &sheet[aden[(d - 1) & 7]] : &wounds;
    t = d ? BTOUGH : TOUGH;
    put (a ? "" : weapon ());                   /* (Its words: the attacker's weapon, the defender's, the strike's) */
    put ("\n");
    put (d ? "" : weapon ());
    put ("\n");
    if (f & RS_PRONE_A) {
        r->result = O_RISE;                     /* (Fallen: it gets up, its turn spent) */
    } else if (!G_OK (g = flat (st + (f & RS_PRONE_D ? 4 : 0)))) {
        r->result = O_MISS;
    } else {
        r->result = O_HIT;
        n = g == G_CS ? 4 : 2;
        w = *dw + n;
        *dw = w > t ? t : w;
        e->target = d;
        e->kind = *dw >= t ? X_DEATH : X_WOUND;
        e->band = n >= 4 ? 3 : 2;
        e->phrase = 3;
        e->cond = *dw >= t ? C_DEAD : 0;
        e->x[0] = BP_CHEST;
        e->x[1] = AS_BLUNT;
        wound_words (n);
        if (*dw >= t) {
            r->grade = FF_D_DEAD;
        } else if (d && !G_OK (flat (f & 1 ? -4 : 0))) {
            r->grade = FF_FLEES;
        }
    }
    put ("\n");
    r->w1 = health (a ? sheet[aden[(a - 1) & 7]] : wounds, a ? BTOUGH : TOUGH) | health (*dw, t) << 8;
}

/* ---- The character's state in words (RS_DESCRIBE b 3: the house's, and its wounds) */

static void __fastcall__ state (unsigned char a)
{
    unsigned char h, dark;

    hub_map (SIGHT_BANK);
    dark = SIGHT->dark;
    hub_map (0);
    h = house_pace (HUB->gait, HUB->watch, dark);
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

static unsigned tlim = REQ_TEXT;

static void __fastcall__ answer (struct req* r)
{
    unsigned char k, m, t;
    signed char g;

    tp = r->text;
    tend = r->text + tlim - 1;
    r->result = 0;
    switch (r->kind) {
    case RS_HELLO:
        r->w1 = 100;                            /* (A round: 10 seconds, the game's clock's) */
        put ("Null\n");                         /* (No character generator: its own character, if no file's given) */
        break;
    case RS_LOAD_PC:
        load_pc (r);
        status ();
        break;
    case RS_NEW_ACTOR:
        sheets_in ();
        if ((k = r->actor - 1) >= NACTORS) {
            r->result = 1;
            break;
        }
        aden[k] = r->w1;
        memset (r->x, 0, sizeof (struct rs_summary));
        ((struct rs_summary*) r->x)->cond = sheet[r->w1] >= BTOUGH ? C_DEAD : 0;
        ((struct rs_summary*) r->x)->health = health (sheet[r->w1], BTOUGH);
        ((struct rs_summary*) r->x)->attacks = 1;
        ((struct rs_summary*) r->x)->power = 10;
        break;
    case RS_TEST:
        if (r->a == P_DIG) {                    /* (The tool to hand that suits it best: the house's) */
            m = r->b > MAT_SOIL ? MAT_STONE : r->b;
            t = dig_tool (m);
            k = digger[t].rate[m];
            strcpy (r->text, digger[t].key);
            r->w2 = digger[t].wear[m];
            if (!k) {
                r->result = 1;
                r->w1 = r->w2 = 0;
                break;
            }
            g = r->grade = DIG_TESTED (m) ? flat (r->steps) : G_MS;    /* (Earth: the tool's rate, untested) */
            r->w1 = g == G_CS ? 2 * k : g == G_MS ? k : g == G_MF ? k / 2 : 0;
            break;
        }
        if (((r->a == P_SEARCH || r->a == P_DISARM) && r->b == HIDE_MAGICAL) ||
            (r->a == P_PICKLOCK && !has ("picks"))) {
            r->result = 1;                      /* (No magic; no lockpicks) */
            break;
        }
        if ((r->a == P_PERSUADE || r->a == P_INTIMIDATE || r->a == P_DECEIVE) && r->text[0] && !speaks (r->text)) {
            r->result = 2;
            break;
        }
        if (r->a <= P_NOTICE_SMELL) {           /* (By its going and watchfulness: the house's) */
            r->steps += house_notice (r, HUB->pc_going, HUB->watch);
        }
        r->grade = flat (r->steps);
        *tp = 0;
        break;
    case RS_ATTACK:
        attack (r);
        status ();
        break;
    case RS_MORALE:
        g = r->grade = flat (r->steps);
        r->result = g == G_CF ? M_BROKEN : g == G_MF ? M_CAUTIOUS : M_STEADY;
        break;
    case RS_TIME:                               /* (No bleeding: none dies of it) */
        r->w1 = 0;
        r->b = health (wounds, TOUGH);
        break;
    case RS_EXERT:                              /* (No fatigue) */
        status ();
        break;
    case RS_EQUIP:
        if (r->a == 2) {
            equip_all (r->text);
        } else if (r->a) {
            for (k = 0; k < NTOOLS && tools[k][0]; ++k) {
            }
            if (k < NTOOLS) {
                strncpy (tools[k], r->text, sizeof tools[k] - 1);
                held[k] = 0;
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
        save_pc (r);
        sheets_in ();
        sheets (0);
        break;
    case RS_DESCRIBE:
        if (r->b == 1) {                        /* (The map's panel: a line) */
            put ("Wounds ");
            putn (wounds);
            put ("/");
            putn (TOUGH);
        } else if (r->b == 2) {
            put (hurt);
        } else if (r->b == 3) {
            status ();
            state (r->a);
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
    default:
        r->result = 0xFF;                       /* (Not yet) */
        *tp = 0;
        break;
    }
}

int main (int argc, char* argv[])
{
    unsigned char s;

    if (argc < 3 || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    rseed (HUB->seed ^ 0x5245);                 /* (Its own dice: the game's seed, and its role) */
    for (;;) {
        hy_sem_acquire (HUB->sem[T_REFEREE]);
        hub_map (0);
        if (HUB->quit) {
            break;
        }
        for (s = 0; s < NSLOTS; ++s) {
            if (hub_posted (&HUB->slot[s])) {
                answer (&HUB->slot[s]);
                hub_reply (&HUB->slot[s]);
            }
        }
        if (hub_posted (DSLOT)) {               /* (The denizens' task's, its own: DTEXT of its text) */
            tlim = DTEXT;
            answer (DSLOT);
            tlim = REQ_TEXT;
            hub_reply (DSLOT);
        }
    }
    return 0;
}
