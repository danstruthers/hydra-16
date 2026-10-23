/*
** t_rules.c - the referee's unit tests (Dingus Quest Zero's, the test dq0rules): dq0harn, the HarnMaster referee, asked
** rs.h's requests with its dice fixed (RS_DICE), each answer held to what the book's rules say it should be (and the
** game's own, games/dq0/HOUSE.md).  It plays the world's part: a hub of its own (hub.h's, HUB_BANKS banks, its header
** made as the UI makes it), the referee started in it (/sd/0/games/lib/dq0/dq0harn, role T_REFEREE), the requests
** posted in the world's slot.  The cases are a script (its first argument: rules.txt, the test's copy in /ram), a
** line each:
**
**   # ...                      a comment (a # starting a word: the rest of the line)
**   base ... end               the lines every character's file begins with (hmchar's format: harnfile.c)
**   char ... end               a character: base's lines, then these (a later line over an earlier one), written to
**                              /ram/t.hm and loaded (RS_LOAD_PC)
**   case WORDS                 the next case's name: its checks one line, "ok - WORDS" or "not ok - WORDS: why"
**   set FIELD=V ...            the hub's, the world's own: gait watch going (pc_going) stance strike shand
**   dice S:F ... [S:F*] [...]  the next request's dice, in turn: each one's sides and the face it's to show (S:F*:
**                              that face for every die after; ...: more dice after them, not held to anything); no
**                              words: no dice at all.  Without it, the dice fall as the seed has them
**   seed N                     the referee's dice seeded anew, as the next request is made
**   ask KIND FIELD=V ...       a request (rs.h's RS_KIND: HARM, TEST ...): actor other steps a b c w1 w2, xN (its
**                              x[N]: RS_WIELD's kinds held, a thing's resistances), text="...";
**                              its reply kept, and the dice it rolled held to the case's (dice)
**   want FIELD=V ...           the reply: grade result a b c w1 w2, eN.kind eN.band eN.loc eN.aspect eN.cond (its
**                              effect N), dice (how many it rolled), sum.FIELD (the hub's summary after it: cond
**                              health step gN (gstep[N]) sight hearing smell load northsense limbs limp mapmind
**                              mapfade),
**                              status="..." (the hub's pcstatus), text="..." (all of it), text~"..." (it has it),
**                              text!"..." (it hasn't), text=@ (the words of the request before it); FIELD&MASK=V;
**                              a value a number
**                              or rs.h's names, | and + between (C_PRONE|C_BLEEDING, GA_JOG+1)
**
** Then "t_rules: N cases, F failed" and "t_rules: PASS" (or FAIL).  The catalogue (LIBDIR/dq0w.cat, the world's) is
** read into the hub's catalogue bank as it starts: a thing named that strikes (set strike=SK_THING+KI_BAR) the
** referee's to read, as in the game.
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../../../games/dq0/hub.h"

#define REFEREE     "/sd/0/games/lib/dq0/dq0harn"
#define LIBDIR      "/sd/0/games/lib/dq0"
#define PCFILE      "/ram/t.hm"
#define NDICE       sizeof ((struct req*) 0)->x

static unsigned char seg, u[HUB_BANKS], x[HUB_BANKS];
static struct hy_regs regs;

static void __fastcall__ map (unsigned char b)
{
    *(volatile unsigned char*) 0xFFF1 = u[b];   /* (U, then the bank: hub.c's hub_map) */
    *(volatile unsigned char*) 0 = x[b];
}

/* ---- The referee asked: q's request posted in the world's slot, its reply waited for (asleep on the world's
** doorbell, as hub_wait does), copied to out with the hub's summary and status after it */

static struct req q, rep, dq;                   /* A request; the last reply; RS_DICE's */
static char prev[REQ_TEXT];                     /* (The reply before the last's words) */
static struct rs_summary sum;
static char status[16];

static void __fastcall__ ask (const struct req* r, struct req* out)
{
    static unsigned char seq;
    struct req* s = &HUB->slot[SLOT_WORLD];

    map (0);
    memcpy ((char*) s + 1, (const char*) r + 1, sizeof *s - 1);     /* (All but its state, */
    s->from = T_WORLD;
    s->seq = ++seq;
    s->state = REQ_POSTED;                      /*   then that) */
    hy_sem_release (HUB->sem[T_REFEREE]);
    while (map (0), s->state != REQ_DONE) {
        hy_sem_wait (HUB->sem[T_WORLD], 255);
    }
    memcpy (out, s, sizeof *s);
    memcpy (&sum, &HUB->pcsum, sizeof sum);
    memcpy (status, HUB->pcstatus, sizeof status);
    s->state = REQ_FREE;
}

/* ---- The script */

static int sfd;
static unsigned char rb[256];
static unsigned rpos, rlen, lineno;
static char line[200];

/* The next line into line (its newline and CR off): its length, or -1 at the end */
static int getl (void)
{
    unsigned char n = 0;
    int k;

    for (;;) {
        if (rpos == rlen) {
            k = read (sfd, rb, sizeof rb);
            if (k <= 0) {
                line[n] = 0;
                return n ? n : -1;
            }
            rlen = k;
            rpos = 0;
        }
        k = rb[rpos++];
        if (k == '\n') {
            break;
        }
        if (k != '\r' && n + 1 < sizeof line) {
            line[n++] = k;
        }
    }
    line[n] = 0;
    ++lineno;
    return n;
}

/* The line's words (a quoted one's spaces its own), up to a # that starts one */
static char* tok[40];
static unsigned char ntok;

static void split (void)
{
    char* p = line;
    unsigned char quoted;

    ntok = 0;
    for (;;) {
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (!*p || *p == '#' || ntok == sizeof tok / sizeof tok[0]) {
            return;
        }
        tok[ntok++] = p;
        for (quoted = 0; *p && (quoted || (*p != ' ' && *p != '\t')); ++p) {
            quoted ^= *p == '"';
        }
        if (*p) {
            *p++ = 0;
        }
    }
}

/* ---- Cases and their checks */

static char cname[100], why[160];
static unsigned char open_case, fails, nasks;  /* (A case open, its checks failed, its requests made) */
static unsigned cases, failed;

static void __fastcall__ fail (const char* s)
{
    if (!fails++) {
        strncpy (why, s, sizeof why - 1);
    } else if (strlen (why) + 5 < sizeof why && !strstr (why, "; ...")) {
        strcat (why, "; ...");
    }
}

/* A fault outside a case (the script's, a character not loaded): a line of its own */
static void __fastcall__ fault (const char* s)
{
    printf ("not ok - rules line %u: %s\n", lineno, s);
    ++failed;
}

static void finish (void)
{
    if (open_case) {
        ++cases;
        if (fails) {
            printf ("not ok - %s: %s\n", cname, why);
            ++failed;
        } else {
            printf ("ok - %s\n", cname);
        }
    }
    open_case = fails = nasks = 0;
    why[0] = 0;
}

/* A check failed, the case's (or, outside one, a line of its own): its words a word in f (bads), or numbers */
static char msg[160];

static void bad (void)
{
    static char m[sizeof msg + 12];

    if (open_case) {
        sprintf (m, "(ask %u) %s", nasks, msg);
        fail (nasks ? m : msg);
    } else {
        fault (msg);
    }
}

static void __fastcall__ bads (const char* f, const char* s)
{
    sprintf (msg, f, s);
    bad ();
}

static void __fastcall__ badn (const char* f, long a, long b)
{
    sprintf (msg, f, a, b);
    bad ();
}

/* ---- Values: numbers and rs.h's names */

struct sym {
    const char*     name;
    int             value;
};
#define S(n)    { #n, n }
static const struct sym syms[] = {
    S (RS_HELLO), S (RS_LOAD_PC), S (RS_TEST), S (RS_ATTACK), S (RS_HARM), S (RS_TIME), S (RS_EXERT), S (RS_EQUIP),
    S (RS_DESCRIBE), S (RS_MORALE), S (RS_DICE), S (RS_NEW_ACTOR), S (RS_CONTEST), S (RS_WIELD),
    S (SK_BEST), S (SK_PUNCH), S (SK_KICK), S (SK_HEAD), S (SK_THING), S (RS_THING),
    S (KI_TORCH), S (KI_DAGGER), S (KI_PICK), S (KI_SHOVEL), S (KI_ROPE), S (KI_CANDLE), S (KI_LANTERN), S (KI_SACK),
    S (KI_CHEST), S (KI_SKULL), S (KI_CLOAK), S (KI_SWORD), S (KI_BASKET), S (KI_CHAIR), S (KI_BAR), S (KI_PANE),
    S (KI_LEATHER_TUNIC), S (KI_HELM), S (KI_MACE), S (KI_SHIELD), S (KI_POT),
    S (G_CF), S (G_MF), S (G_MS), S (G_CS),
    S (P_NOTICE_SIGHT), S (P_NOTICE_SOUND), S (P_NOTICE_SMELL), S (P_SEARCH), S (P_STEALTH), S (P_CLIMB), S (P_JUMP),
    S (P_SWIM), S (P_BALANCE), S (P_FORCE), S (P_LIFT), S (P_PICKLOCK), S (P_DISARM), S (P_ENDURE),
    S (P_RESIST_POISON), S (P_RESIST_MIND), S (P_DODGE_HAZARD), S (P_FIRST_AID), S (P_LORE), S (P_TRACK),
    S (P_PERSUADE), S (P_INTIMIDATE), S (P_ANIMALS), S (P_DIG), S (P_APPRAISE), S (P_CONCEAL), S (P_ASSESS),
    S (P_DECEIVE), S (P_THROW), S (P_INITIATIVE), S (P_GROPE), S (P_STUMBLE), S (P_RISE), S (P_DRAW), S (P_THEME),
    S (HIDE_MECHANICAL), S (HIDE_MAGICAL),
    S (AP_WEAPON), S (AP_ARMOUR), S (AP_METAL), S (AP_WOOD), S (AP_HIDE), S (AP_CLOTH), S (AP_POTTERY), S (AP_JEWEL),
    S (AP_BOOK), S (AP_FOOD), S (AP_BONE), S (AP_STONE), S (AP_GLASS),
    S (MAT_EARTH), S (MAT_STONE), S (MAT_MASONRY), S (MAT_FLAGS), S (MAT_SOIL),
    S (GA_CREEP), S (GA_SLOW), S (GA_WALK), S (GA_JOG), S (GA_RUN), S (GA_SPRINT), S (GA_REST), S (GA_WORK),
    S (C_PRONE), S (C_UNCONSCIOUS), S (C_DEAD), S (C_STUNNED), S (C_BLEEDING), S (C_FATIGUED), S (C_ARM_USELESS),
    S (C_LEG_USELESS), S (C_LAME), S (C_CRAWLS), S (C_STUCK),
    S (LB_LEFT), S (LB_AMBI), S (LB_ARM_L), S (LB_ARM_R), S (LB_LEG_L), S (LB_LEG_R), S (LB_HURT), S (DH_LEFT),
    S (DH_RIGHT),
    S (IH_LEFT), S (IH_RIGHT), S (IH_BOTH),
    S (H_FINE), S (H_HURT), S (H_BADLY), S (H_CRITICAL), S (H_DOWN), S (H_DEAD),
    S (X_NONE), S (X_WOUND), S (X_HEAL), S (X_COND_ON), S (X_COND_OFF), S (X_DEATH), S (X_DROP),
    S (BP_SKULL), S (BP_FACE), S (BP_NECK), S (BP_SHOULDER), S (BP_UPPER_ARM), S (BP_ELBOW), S (BP_FOREARM),
    S (BP_HAND), S (BP_CHEST), S (BP_BELLY), S (BP_GROIN), S (BP_HIP), S (BP_THIGH), S (BP_KNEE), S (BP_CALF),
    S (BP_FOOT), S (BP_RIGHT),
    S (HC_FALL), S (HC_STRIKE), S (HC_FIRE), S (HC_STRUCK),
    S (AS_BLUNT), S (AS_EDGE), S (AS_POINT), S (AS_FIRE),
    S (Z_MID), S (Z_HIGH), S (Z_LOW), S (AS_BEST),
    S (D_AUTO), S (D_BLOCK), S (D_DODGE), S (D_COUNTER), S (D_NONE),
    S (O_MISS), S (O_DODGED), S (O_BLOCKED), S (O_HIT), S (O_ADVANTAGE), S (O_COUNTER_HIT), S (O_FUMBLE),
    S (O_STUMBLE), S (O_RISE), S (O_REARM), S (RS_PRONE_A), S (RS_PRONE_D), S (RS_REARM),
    S (GR_FLOOR), S (GR_DEEP_WATER), S (GR_SHALLOW_WATER), S (GR_SOFT), S (GR_HARD), S (GR_ROCKY),
    S (HF_CLEAR), S (HF_HOLDS),
    S (WA_TRAPS), S (WA_LISTEN), S (WA_MAP), S (GOING_DARK),
};
#define NSYMS   (sizeof syms / sizeof syms[0])

static long __fastcall__ value (const char* s)
{
    static char w[24];
    long v = 0, t;
    char op = '+';
    unsigned char n, k;

    for (;;) {
        for (n = 0; s[n] && s[n] != '|' && (s[n] != '+' || !n) && n < sizeof w - 1; ++n) {
            w[n] = s[n];
        }
        w[n] = 0;
        if ((w[0] >= '0' && w[0] <= '9') || w[0] == '-') {
            t = strtol (w, 0, 0);
        } else {
            for (k = 0; k < NSYMS && strcmp (syms[k].name, w); ++k) {
            }
            if (k == NSYMS) {
                bads ("no such name: %s", w);
                t = 0;
            } else {
                t = syms[k].value;
            }
        }
        v = op == '|' ? v | t : v + t;
        s += n;
        if (!*s) {
            return v;
        }
        op = *s++;
    }
}

/* A word's quoted part, its quotes off ("a b" -> a b), in place */
static char* __fastcall__ unquote (char* s)
{
    unsigned char n;

    if (*s == '"') {
        ++s;
    }
    n = strlen (s);
    if (n && s[n - 1] == '"') {
        s[n - 1] = 0;
    }
    return s;
}

/* ---- The commands */

static char base[1400], more[700];
static unsigned char sides[NDICE], faces[NDICE], nd, dmore, drep, ddice, seeded;
static unsigned dseed, rolled;

/* Lines up to "end" added to b (n its room) */
static void __fastcall__ block (char* b, unsigned n)
{
    unsigned l = 0;

    b[0] = 0;
    while (getl () >= 0 && strcmp (line, "end")) {
        if (l + strlen (line) + 2 < n) {
            strcpy (b + l, line);
            l += strlen (line);
            b[l++] = '\n';
            b[l] = 0;
        } else {
            fault ("a block too big");
        }
    }
}

static void character (void)
{
    int fd;

    block (more, sizeof more);
    if ((fd = open (PCFILE, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        fault ("no " PCFILE);
        return;
    }
    write (fd, base, strlen (base));
    write (fd, more, strlen (more));
    close (fd);
    memset (&q, 0, sizeof q);
    q.kind = RS_LOAD_PC;
    strcpy (q.text, PCFILE);
    ask (&q, &rep);
    if (rep.result) {
        fault ("the character not loaded");
    }
}

static void set (void)
{
    unsigned char k;
    char* v;
    long n;

    for (k = 1; k < ntok; ++k) {
        if (!(v = strchr (tok[k], '='))) {
            bads ("set: %s?", tok[k]);
            continue;
        }
        *v++ = 0;
        n = value (v);
        map (0);
        if (!strcmp (tok[k], "gait")) {
            HUB->gait = n;
        } else if (!strcmp (tok[k], "watch")) {
            HUB->watch = n;
        } else if (!strcmp (tok[k], "going")) {
            HUB->pc_going = n;
        } else if (!strcmp (tok[k], "stance")) {
            HUB->pc_stance = n;
        } else if (!strcmp (tok[k], "strike")) {
            HUB->pc_strike = n;
        } else if (!strcmp (tok[k], "shand")) {
            HUB->pc_shand = n;
        } else if (!strcmp (tok[k], "now")) {
            HUB->now = n;
        } else {
            bads ("set: no %s", tok[k]);
        }
    }
}

static void dice (void)
{
    unsigned char k;
    char* c;

    nd = dmore = drep = 0;
    ddice = 1;
    for (k = 1; k < ntok; ++k) {
        if (!strcmp (tok[k], "...")) {
            dmore = 1;
        } else if (!(c = strchr (tok[k], ':')) || nd == NDICE) {
            bads ("dice: %s?", tok[k]);
        } else if (c[strlen (c) - 1] == '*') {
            sides[nd] = atoi (tok[k]);
            drep = atoi (c + 1);
            dmore = 1;
        } else {
            sides[nd] = atoi (tok[k]);
            faces[nd++] = atoi (c + 1);
        }
    }
}

/* A request, its dice fixed first (RS_DICE: the faces, the seed), and what it rolled asked after (RS_DICE again) */
static void request (void)
{
    static char kind[24];
    unsigned char k;
    char* v;
    long n;

    ++nasks;
    memset (&q, 0, sizeof q);
    strcpy (kind, "RS_");
    strncat (kind, tok[1], sizeof kind - 4);
    q.kind = value (kind);
    for (k = 2; k < ntok; ++k) {
        if (!(v = strchr (tok[k], '='))) {
            bads ("ask: %s?", tok[k]);
            continue;
        }
        *v++ = 0;
        if (!strcmp (tok[k], "text")) {
            strncpy (q.text, unquote (v), sizeof q.text - 1);
            continue;
        }
        n = value (v);
        if (!strcmp (tok[k], "actor")) {
            q.actor = n;
        } else if (!strcmp (tok[k], "other")) {
            q.other = n;
        } else if (!strcmp (tok[k], "steps")) {
            q.steps = n;
        } else if (!strcmp (tok[k], "a")) {
            q.a = n;
        } else if (!strcmp (tok[k], "b")) {
            q.b = n;
        } else if (!strcmp (tok[k], "c")) {
            q.c = n;
        } else if (!strcmp (tok[k], "w1")) {
            q.w1 = n;
        } else if (!strcmp (tok[k], "w2")) {
            q.w2 = n;
        } else if (tok[k][0] == 'x' && tok[k][1] >= '0' && tok[k][1] <= '9' && !tok[k][2]) {
            q.x[tok[k][1] - '0'] = n;
        } else {
            bads ("ask: no %s", tok[k]);
        }
    }
    memset (&dq, 0, sizeof dq);                 /* (Its dice: the faces, and the seed) */
    dq.kind = RS_DICE;
    if (ddice) {
        memcpy (dq.x, faces, nd);
        dq.b = drep;
    }
    dq.a = seeded;
    dq.w1 = dseed;
    ask (&dq, &dq);
    memcpy (prev, rep.text, sizeof prev);
    ask (&q, &rep);
    memset (&dq, 0, sizeof dq);                 /* (What it rolled) */
    dq.kind = RS_DICE;
    ask (&dq, &dq);
    rolled = dq.w1;
    if (ddice) {
        if (!dmore && rolled != nd) {
            badn ("it rolled %ld dice, not %ld", rolled, nd);
        }
        for (k = 0; k <= nd && k < rolled && k < NDICE; ++k) {    /* (After the faces, a repeated one's) */
            if ((k < nd || drep) && dq.x[k] != sides[k]) {
                sprintf (msg, "its die %u a d%u, not a d%u", k + 1, dq.x[k], sides[k]);
                bad ();
                break;
            }
        }
        if (dq.c) {
            badn ("%ld of its faces not rolled", dq.c, 0);
        }
    }
    ddice = seeded = 0;
}

/* A field of the reply, by its name: its value, ok 0 if there's no such field */
static long __fastcall__ field (const char* f, unsigned char* ok)
{
    static const char* const sf[] = { "cond", "health", "step", "attacks", "sight", "hearing", "smell",
        "darkvision", "nightvision", "morale", "load", "power", "northsense", "limbs", "limp", "mapmind", "mapfade" };
#define SO(m)   offsetof (struct rs_summary, m)
    static const unsigned char so[] = { 0, SO (health), SO (step), SO (attacks), SO (sight), SO (hearing), SO (smell),
        SO (darkvision), SO (nightvision), SO (morale), SO (load), SO (power), SO (northsense), SO (limbs), SO (limp),
        SO (mapmind), SO (mapfade) };
    const struct rs_effect* e;
    unsigned char k;

    *ok = 1;
    if (!strcmp (f, "grade")) {
        return rep.grade;
    } else if (!strcmp (f, "result")) {
        return rep.result;
    } else if (!strcmp (f, "a")) {
        return rep.a;
    } else if (!strcmp (f, "b")) {
        return rep.b;
    } else if (!strcmp (f, "c")) {
        return rep.c;
    } else if (!strcmp (f, "w1")) {
        return rep.w1;
    } else if (!strcmp (f, "w2")) {
        return rep.w2;
    } else if (!strcmp (f, "steps")) {
        return rep.steps;
    } else if (!strcmp (f, "dice")) {
        return rolled;
    } else if (f[0] == 'e' && f[1] >= '0' && f[1] < '0' + RS_EFFECTS && f[2] == '.') {
        e = (const struct rs_effect*) rep.x + (f[1] - '0');
        f += 3;
        if (!strcmp (f, "target")) {
            return e->target;
        } else if (!strcmp (f, "kind")) {
            return e->kind;
        } else if (!strcmp (f, "band")) {
            return e->band;
        } else if (!strcmp (f, "cond")) {
            return e->cond;
        } else if (!strcmp (f, "loc")) {
            return e->x[0];
        } else if (!strcmp (f, "aspect")) {
            return e->x[1];
        }
    } else if (!strncmp (f, "sum.", 4)) {
        f += 4;
        if (f[0] == 'g' && f[1] >= '0' && f[1] < '0' + NGAITS && !f[2]) {
            return sum.gstep[f[1] - '0'];
        }
        for (k = 0; k < sizeof sf / sizeof sf[0] && strcmp (f, sf[k]); ++k) {
        }
        if (k < sizeof sf / sizeof sf[0]) {
            return k ? ((const unsigned char*) &sum)[so[k]] : sum.cond;    /* (Its bytes but cond, its first) */
        }
    }
    *ok = 0;
    return 0;
}

static void want (void)
{
    static char m[160];
    unsigned char k, ok;
    char* v;
    char* mask;
    char op;
    long got, w;

    for (k = 1; k < ntok; ++k) {
        v = strpbrk (tok[k], "=~!");
        if (!v) {
            bads ("want: %s?", tok[k]);
            continue;
        }
        op = *v;
        *v++ = 0;
        if (!strcmp (tok[k], "text") || !strcmp (tok[k], "status")) {
            v = unquote (v);
            if (!strcmp (v, "@")) {
                v = prev;
            }
            mask = tok[k][0] == 't' ? rep.text : status;
            if (op == '=' ? strcmp (mask, v) != 0 : op == '~' ? !strstr (mask, v) : strstr (mask, v) != 0) {
                strcpy (m, tok[k]);
                strcat (m, op == '!' ? " has \"" : " hasn't \"");
                strncat (m, v, 40);
                strcat (m, "\": \"");
                strncat (m, mask, 60);
                strcat (m, "\"");
                bads ("%s", m);
            }
            continue;
        }
        if ((mask = strchr (tok[k], '&')) != 0) {
            *mask++ = 0;
        }
        got = field (tok[k], &ok);
        if (!ok) {
            bads ("want: no %s", tok[k]);
            continue;
        }
        if (mask) {
            got &= value (mask);
        }
        w = value (v);
        if (got != w) {
            strcpy (m, tok[k]);
            strcat (m, " %ld, not %ld");
            badn (m, got, w);
        }
    }
}

int main (int argc, char* argv[])
{
    static char args[40];
    unsigned char t, n;
    unsigned off;
    int s;

    if (argc < 2 || (sfd = open (argv[1], O_RDONLY)) < 0) {
        puts ("t_rules: no script");
        return 1;
    }
    if ((s = hy_seg_create (HUB_BANKS)) < 0) {
        puts ("t_rules: no shared segment");
        return 1;
    }
    seg = s;
    for (t = 0; t < HUB_BANKS; ++t) {           /* (Each bank's U and $00: hub.c's find_banks) */
        regs.a = seg;
        regs.x = t;
        if (hy_call (HY_SEG_MAP, &regs)) {
            puts ("t_rules: no bank");
            return 1;
        }
        u[t] = regs.a;
        x[t] = regs.x;
    }
    map (SIGHT_BANK);                           /* (The senses' record: none) */
    memset (HUB_WINDOW, 0, 0x2000);
    if ((s = open (LIBDIR "/dq0w.cat", O_RDONLY)) >= 0) {   /* (The catalogue, the world's: wfile.c's) */
        for (off = 0; off < 0x2000 && (n = read (s, line, sizeof line)) > 0; off += n) {
            map (CAT_BANK);
            memcpy (HUB_WINDOW + off, line, n);
        }
        close (s);
    }
    map (0);                                    /* The header as hub_create makes it (its magic last: the writers'
                                                **   check finds the hub by it), this task the world */
    memset (HUB, 0, sizeof (struct hub));
    HUB->version = HUB_VERSION;
    HUB->seg = seg;
    for (t = 0; t < NTASKS; ++t) {
        if ((s = hy_sem_new (0, 0)) < 0) {
            puts ("t_rules: no semaphore");
            return 1;
        }
        HUB->sem[t] = s;
    }
    HUB->seed = 1;
    HUB->magic = HUB_MAGIC;
    HUB->task[T_WORLD] = hy_task ();
    utoa (seg, args, 10);                       /* dq0harn SEG ROLE DIR (hub.c's hub_spawn) */
    n = strlen (args) + 1;
    utoa (T_REFEREE, args + n, 10);
    n += strlen (args + n) + 1;
    strcpy (args + n, LIBDIR);
    n += strlen (LIBDIR) + 1;
    args[n] = 0;
    regs.a = 0;
    regs.r[0] = (unsigned) REFEREE;
    regs.r[1] = (unsigned) args;
    if (hy_call (HY_SPAWN, &regs)) {
        puts ("t_rules: no referee (" REFEREE ")");
        return 1;
    }
    t = regs.a;
    map (0);
    while (getl () >= 0) {
        if (!strncmp (line, "case ", 5)) {
            finish ();
            strncpy (cname, line + 5, sizeof cname - 1);
            open_case = 1;
            continue;
        }
        split ();
        if (!ntok) {
            continue;
        }
        if (!strcmp (tok[0], "base")) {
            block (base, sizeof base);
        } else if (!strcmp (tok[0], "char")) {
            character ();
        } else if (!strcmp (tok[0], "set")) {
            set ();
        } else if (!strcmp (tok[0], "dice")) {
            dice ();
        } else if (!strcmp (tok[0], "seed") && ntok == 2) {
            seeded = 1;
            dseed = value (tok[1]);
        } else if (!strcmp (tok[0], "ask") && ntok >= 2) {
            request ();
        } else if (!strcmp (tok[0], "want")) {
            want ();
        } else {
            bads ("what's %s?", tok[0]);
        }
    }
    finish ();
    close (sfd);
    map (0);                                    /* (Its end: the world's, in the game) */
    HUB->quit = 1;
    hy_sem_release (HUB->sem[T_REFEREE]);
    hy_wait (t, 0);
    printf ("t_rules: %u cases, %u failed\n", cases, failed);
    puts (failed || !cases ? "t_rules: FAIL" : "t_rules: PASS");
    return failed != 0;
}
