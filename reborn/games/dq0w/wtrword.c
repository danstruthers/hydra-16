/*
** wtrword.c - (dq0w) the traces read, in words (wtrace.c's: the reading, I_TRACE's): the world's, as the things'
** are, the UI's to say (the UI bank's trtext).  "The floor here is flagged, grey with dust. It's been trodden: there
** are the marks of feet."  "Several small clawed feet have crossed the dust here, recently, leading ahead to your
** left: giant rats, by the look of them."  "A rank animal smell hangs here, fresh: something large passed not long
** ago."  "A musky smell hangs heavy here: giant rats, by the smell of it." (a scent strong, its passings and stays
** many: a lair's).  Each way by the character's facing, never the compass (HOUSE.md's).  Swapped part 31's
** (../dq0/swap.inc, bank.s's stub tr_words).
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "trace.h"
#include "tstore.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)

#pragma code-name (push, "SW31CODE")
#pragma rodata-name (push, "SW31RODATA")
#pragma bss-name (push, "SW31RODATA")

static struct tread R;
static unsigned n;                              /* (The words so far: their length) */
/* Words added (the UI bank's trtext, as far as it holds) */
static void __fastcall__ add (const char* s)
{
    static unsigned k;

    if (n + (k = strlen (s)) < TR_TEXT) {
        hub_map (UI_BANK);
        strcpy (UIREC->trtext + n, s);
        hub_map (0);
        n += k;
    }
}

/* A trail's maker, as it's told: "giant rats" ("your own"; by the grade, plainly, or by the look of them) */
static void __fastcall__ maker (const struct trail* t, unsigned char smell)
{
    if (!t->name[0]) {
        add (": your own.");
        return;
    }
    add (": ");
    add (t->name);
    add (t->n == TP_ONE ? "" : "s");
    add (R.grade == G_CS ? ", plainly." : smell ? ", by the smell of it." : t->n == TP_ONE ? ", by the look of it." :
        ", by the look of them.");
}

/* What the character wears on its feet (its prints' words: shod's) */
static unsigned char feet (void)
{
    static unsigned i;
    static unsigned char f, k;

    for (i = f = 0; (i = items (i, IN_ACTOR | IN_X, 0)) != 0; ) {
        k = ip->kind;
        if (ip->flags & I_WORN) {
            f = k == KI_BOOTS ? 1 : k == KI_SANDALS ? 2 : k == KI_SHOES ? 3 : k == KI_HOBNAIL ? 4 : f;
        }
    }
    hub_map (0);
    return f;
}

/* The words of the traces read (the UI bank's tread: each trail's passings and age made TP_*, TA_*; a critical
** failure's misread; a scent by its strength, heavy at TR_HEAVY, else by its age), into its trtext; the UI told
** (E_VIEW V_TRACED): w back (the tenths the reading took) */
unsigned __fastcall__ s_tr_words (unsigned w)
{
    static const char* const ground[5] = { "bare earth", "rough stone", "paved", "flagged", "soft earth" };    /* (MAT_*) */
    static const char* const dusty[8] = { "", ", under a skin of dust", ", under a skin of dust", ", grey with dust",
        ", grey with dust", ", deep in dust", ", deep in dust", ", swept clean" };
    static const char* const many[4] = { "One set of ", "A few ", "Several ", "Many " };
    static const char* const when[5] = { "only just", "recently", "some hours ago", "a day or so ago", "long ago" };
    static const char* const since[5] = { "just now", "not long ago", "some hours ago", "a day or so ago", "long ago" };
    static const char* const rel_w[8] = { "ahead", "ahead to your right", "to your right", "behind you to your right",
        "behind you", "behind you to your left", "to your left", "ahead to your left" };
    static const char* const shod[5] = { "bare feet", "booted feet", "sandalled feet", "shod feet", "hobnailed boots" };
    static const unsigned char n_at[3] = { 2, 4, 12 };     /* (A trail's passings: TP_*'s bounds) */
    static const unsigned ago_at[4] = { 2, 15, 150, 720 };  /* (TA_*'s, in TQs: under 8 minutes, an hour, ten hours, two
                                                    **   days) */
    static struct trail* t;
    static unsigned char i, d, soil, face, e;
    static int fd;
    static struct msg ev;

    hub_map (UI_BANK);
    memcpy (&R, &UIREC->tread, sizeof R);
    UIREC->trtext[0] = 0;
    hub_map (0);
    n = 0;
    face = HUB->pc_face;
    if (trfd < 0) {
        hy_bank (ovbank[OVBANKS]);              /* (The kinds' words: the lib folder's dq0tr.dat, opened the first
                                                **   time and kept open: an open on the card some 700,000 cycles) */
        strcpy (path, TR_DIR);
        hub_map (0);
        strcat (path, "/dq0tr.dat");
        trfd = open (path, O_RDONLY);
    }
    fd = trfd;
    for (i = 0, t = R.tr; i < R.ntr; ++i, ++t) {
        if (fd >= 0 && lseek (fd, (unsigned) (t->kind * TREC), SEEK_SET) >= 0 && read (fd, buf, TREC) == TREC) {
            memcpy (t->name, buf, sizeof t->name);
            memcpy (t->what, (char*) buf + (R.how == TRC_SMELL ? 40 : 16), sizeof t->what);
        }
        if (!t->kind) {
            t->name[0] = 0;                     /* (The character's own: its footwear's marks) */
            strcpy (t->what, shod[feet ()]);
        }
        for (d = TP_ONE; d < TP_MANY && t->n >= n_at[d]; ++d) {
        }
        t->n = d;
        for (t->ago = TA_NOW; t->ago < TA_LONG && t->age >= ago_at[t->ago]; ++t->ago) {
        }
        if (R.how == TRC_TRACK && R.grade == G_CF) {
            t->head ^= t->head <= H_NW ? 4 : 0;     /* (Misread: the way it went wrong, how long ago) */
            t->ago = (t->ago + 2) % 5;
        }
    }
    soil = R.floor == MAT_EARTH || R.floor == MAT_SOIL;
    d = soil && R.dust != TD_SWEPT ? 0 : R.dust & 7;
    if (R.result == TRR_DARK) {
        add ("It's too dark to make out the floor.");
    } else if (R.result == TRR_NOSE) {
        add ("You can smell nothing at all.");
    } else if (R.how == TRC_FLOOR) {            /* (Looked at plainly: the floor, its dust, trodden or not) */
        add ("The floor here is ");
        add (ground[R.floor % 5]);
        add (dusty[d]);
        add (R.trod ? ". It's been trodden: there are the marks of feet." : ". Nothing seems to have disturbed it.");
    } else if (R.how == TRC_TRACK) {            /* (The Tracking test's: how many, how long ago, which way, whose) */
        if (!R.ntr) {
            add (G_OK (R.grade) ? "You find no tracks here." : "You can make nothing of the floor here.");
        } else if (R.grade == G_MF) {
            add ("The floor's been trodden here, but you can make nothing of the marks.");
        }
        for (i = 0, t = R.tr; R.grade != G_MF && i < R.ntr; ++i, ++t) {
            add (i ? " " : "");
            add (many[t->n & 3]);
            add (t->what);
            add (t->n == TP_ONE ? " has crossed the " : " have crossed the ");
            add (soil ? "earth here, " : d && d < TD_SWEPT ? "dust here, " : "floor here, ");
            add (when[t->ago % 5]);
            if (t->head <= H_NW) {
                add (", leading ");
                add (rel_w[(t->head - face) & 7]);
            }
            maker (t, 0);
        }
    } else {                                    /* (A smell: the smoke on the air; the freshest scents, and whose, if
                                                **   it was noticed well) */
        if (R.smoke) {
            add (R.smoke > 1 ? "There's a smell of smoke on the air." : "A faint smell of smoke hangs on the air.");
        } else if (!R.ntr) {
            add ("You smell nothing out of the ordinary.");
        }
        for (i = 0, t = R.tr; i < R.ntr && i < 2; ++i, ++t) {
            add (i || R.smoke ? " " : "");
            e = t->what[0];
            t->what[0] -= e >= 'a' && e <= 'z' ? 'a' - 'A' : 0;
            add (t->what);
            t->what[0] = e;
            add (t->strength >= TR_HEAVY ? " hangs heavy here" : t->ago <= TA_RECENT ? " hangs here, fresh" :
                t->ago == TA_HOURS ? " hangs here, faint" : " lingers here, stale");
            if (G_OK (R.grade)) {
                maker (t, 1);
            } else {
                add (t->big ? ": something large passed " : ": something passed ");
                add (since[t->ago % 5]);
                add (".");
            }
        }
    }
    hub_map (UI_BANK);
    memcpy (&UIREC->tread, &R, sizeof R);       /* (Its trails as worded) */
    hub_map (0);
    ev.kind = E_VIEW;
    ev.w1 = V_TRACED;
    ev.t = HUB->now;
    hub_event (&ev);
    return w;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)
