/*
** theme.c - (overlay 5) Dingus Quest Zero's theme (theme.h), the UI's: its words (its name and story, its strata's
** and rooms' names, the way out shut and open, the endings) and its puzzle, from the theme's copy in this overlay's
** own bank (TCOPY: main.c reads the theme into it as the game starts, for the hub isn't in the window while an
** overlay is).  The puzzle: a command for a piece (read the tablet, combine the sunstone with the moonstone, put the
** crown on the throne, use the seal on the way out), matched against the theme's rules by the things' kinds (the
** things task's: what's to hand, IO_FIND), done (the things made one, IO_COMBINE; put, IO_PUT), its words told and
** its effect given back for main.c to carry out (the way out opened, the place ruled).  What's been done is the
** quest's (main.c's Q, kept in the save).
*/

#pragma code-name ("OV5CODE")
#pragma rodata-name ("OV5RODATA")

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "dq0.h"
#include "hub.h"

#define T               TCOPY

/* The theme read in: libdir/dq0t.dat's t'th (counted round: the file's first says how many) into this overlay's bank
** (TCOPY) and the hub's theme bank (THEME), through the main RAM (lb); then its kinds of things into the hub's
** catalogue, after the catalogue's own (the things task has read them in) */
void __fastcall__ o_theme_load (unsigned char t)
{
    char* path = lb + 300;
    unsigned off;
    unsigned char k;
    int fd, n;

    strcpy (path, libdir);
    strcat (path, "/dq0t.dat");
    if ((fd = open (path, O_RDONLY)) < 0) {
        t_end ();
        hy_exits ("dq0: its themes (lib/dq0/dq0t.dat) not found");
    }
    for (k = 0; ; ++k) {
        for (off = 0; off < THEME_SIZE && (n = read (fd, lb, 256)) > 0; off += n) {
            memcpy ((char*) T + off, lb, n);
            hub_mem (THEME_BANK, (char*) THEME + off, lb, n);
        }
        if (!k) {
            t %= T->count;
        }
        if (k == t) {
            break;
        }
    }
    close (fd);
    Q.theme = t;
    memcpy (lb, T->kind, sizeof T->kind);
    hub_mem (CAT_BANK, CATALOGUE + KI_THEME, lb, sizeof T->kind);
}

/* A theme's name for something (TN_*): its own, a stratum's by its number, into tname; a room's by its kind, a lit
** room's light, added to lb (words.c's) */
void __fastcall__ o_theme_named (unsigned char what, unsigned char n)
{
    if (what >= TN_ROOM) {
        strcat (lb, what == TN_ROOM ? T->room[n & 7] : T->light);
    } else {
        strcpy (tname, what == TN_THEME ? T->name : T->stratum[n & 7]);
    }
}

/* Its words at offset w (0: none), said on a line of their own */
static void __fastcall__ words_at (unsigned w)
{
    if (w) {
        msg ((const char*) T + w);
    }
}

/* Its name, as the game starts (and its story, a new game's) */
void __fastcall__ o_story (unsigned char resumed)
{
    strcpy (tname, T->name);
    tname[0] &= ~0x20;                          /* ("The Crypt ...") */
    blank ();
    msg (tname);
    say (".");
    if (!resumed) {
        words_at (T->story);
    }
}

/* The way out: shut (the lock's words), or open */
void o_way_words (void)
{
    words_at (Q.open ? T->opened : T->lock);
}

/* An ending's words (END_*), and the game's end */
void __fastcall__ o_ending (unsigned char how)
{
    blank ();
    words_at (T->end[how & 3]);
    blank ();
    msg ("The game is over.");
    blank ();
}

/* The thing named that's to hand (carried, or within reach): its kind, the theme's (0 ...), or 0xFE (a thing of
** the catalogue's); 0xFF if there's none (the things' words why said) */
static unsigned char __fastcall__ kind_of (const char* name)
{
    unsigned char k;

    if (othings (IO_FIND, name, name, 0) != IR_OK) {
        msg (lb);
        return 0xFF;
    }
    k = othings_x;
    return k >= KI_THEME ? k - KI_THEME : 0xFE;
}

/* The rule (verb, a, b) in the theme's: its number, or 0xFF */
static unsigned char __fastcall__ rule (unsigned char verb, unsigned char a, unsigned char b)
{
    unsigned char r;
    const struct trule* p = T->rule;

    for (r = 0; r < T->nrules; ++r, ++p) {
        if (p->verb == verb && p->a == a && (b == 0xFF || p->b == b)) {
            return r;
        }
    }
    return 0xFF;
}

/* The way out within reach: the character on the top stratum, a square or two from it (its first link's) */
static unsigned char near_out (void)
{
    static struct { unsigned char sn; unsigned x, y; } pc;
    static struct { unsigned x, y; } w;

    hub_mem (0, &pc, (const void*) &HUB->pc_stratum, sizeof pc);
    hub_mem (0, &w, &HUB->strata[0].link[0].x, sizeof w);
    return !pc.sn && w.x - pc.x + 2 <= 4 && w.y - pc.y + 2 <= 4;
}

/* Rule r done: its words told, kept as done, and its effect (Q_*) */
static unsigned char __fastcall__ done (unsigned char r)
{
    const struct trule* p = &T->rule[r];

    words_at (p->words);
    Q.flags |= 1 << r;
    if (p->effect == TE_OPEN) {
        Q.open = 1;
        return Q_OPEN;
    }
    return p->effect == TE_RULE ? Q_RULED : Q_DONE;
}

/* A command the puzzle's (read X; combine X with Y; put X on Y, use X on Y; use X on the way out, open or unlock it
** with X, examine it): Q_NONE (not the puzzle's: main.c carries it out as ever), Q_TOLD (words only), or what came
** of a rule done (Q_DONE, Q_OPEN, Q_RULED) */
unsigned char __fastcall__ o_puzzle (struct pcmd* c)
{
    unsigned char v = c->verb, a, b, r;
    char* x = tname;                            /* (The name of the one that becomes the whole: in the main RAM) */

    if (v == V_READ) {
        if (!c->word[0]) {
            msg ("Read what?");
        } else if ((a = kind_of (c->word)) != 0xFF) {
            if ((r = rule (TR_READ, a, 0xFF)) != 0xFF) {
                return done (r);
            }
            sayf ("\nThere's nothing written on the %s.", c->word);
        }
        return Q_TOLD;
    }
    if (v == V_COMBINE) {
        if (!c->word[0] || !c->word2[0]) {
            msg ("Combine what with what? (combine the one with the other)");
        } else if ((a = kind_of (c->word)) != 0xFF && (b = kind_of (c->word2)) != 0xFF) {
            r = rule (TR_COMBINE, a, b);
            strcpy (x, c->word);                /* (The one that becomes the whole: the rule's first) */
            if (r == 0xFF && (r = rule (TR_COMBINE, b, a)) != 0xFF) {
                strcpy (x, c->word2);
                strcpy (c->word2, c->word);
            }
            if (r == 0xFF) {
                msg ("Those don't go together.");
            } else if (othings (IO_COMBINE, x, c->word2, KI_THEME + T->rule[r].c) == IR_OK) {
                return done (r);
            } else {
                msg (lb);
            }
        }
        return Q_TOLD;
    }
    if (c->obj == OB_WAY_OUT && (v == V_USE || v == V_UNLOCK || v == V_OPEN || v == V_EXAMINE)) {
        if (v == V_EXAMINE || !c->word[0]) {
            o_way_words ();
        } else if (!near_out ()) {
            msg ("The way out is out of reach: go over to it first.");
        } else if ((a = kind_of (c->word)) != 0xFF) {
            if ((r = rule (TR_OUT, a, 0xFF)) != 0xFF) {
                return done (r);
            }
            sayf ("\nNothing comes of trying the %s on it.", c->word);
        }
        return Q_TOLD;
    }
    if ((v == V_PUT || v == V_USE) && c->word[0] && c->word2[0]) {
        if ((a = kind_of (c->word)) == 0xFF || (b = kind_of (c->word2)) == 0xFF) {
            return Q_TOLD;
        }
        if ((r = rule (TR_PUT, a, b)) == 0xFF) {
            if (v == V_PUT) {
                return Q_NONE;                  /* (A thing put as any other) */
            }
            sayf ("\nNothing comes of using the %s on the %s.", c->word, c->word2);
            return Q_TOLD;
        }
        a = othings (IO_PUT, c->word, c->word2, (T->kind[b].keeps & KF_ON) != 0);
        msg (lb);                               /* (The things' words of it) */
        return a == IR_OK ? done (r) : Q_TOLD;
    }
    if (v == V_USE) {
        msg ("Use what, and on what? (use the key on the way out ...)");
        return Q_TOLD;
    }
    return Q_NONE;
}

/* A thing named, to hand, that's fixed where it stands (a theme's altar: never taken up): 1, told so; else 0 */
unsigned char __fastcall__ o_fixed (const char* name)
{
    unsigned char k;

    if (othings (IO_FIND, name, name, 0) != IR_OK || (k = othings_x) < KI_THEME ||
        !(T->kind[k - KI_THEME].keeps & KF_FIXED)) {
        return 0;
    }
    sayf ("\nThe %s is fixed where it stands.", T->kind[k - KI_THEME].name);
    return 1;
}
