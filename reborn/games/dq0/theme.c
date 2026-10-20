/*
** theme.c - (overlay 5) Dingus Quest Zero's theme (theme.h), the UI's: its words (its name and story, its strata's
** and rooms' names, the way out shut and open, the endings) and its puzzle's words, from the theme's copy in this
** overlay's own bank (TCOPY: main.c reads the theme into it as the game starts, for the hub isn't in the window while
** an overlay is).  The puzzle: a command for a piece (read the tablet, combine the sunstone with the moonstone, put
** the crown on the throne, use the seal on the way out) is the world's to judge by the theme's rules (items.h's
** IO_PUZZLE: the things found to hand, the rule for their kinds, done: the things made one, put, the way out opened,
** the hub's way_open), its words told here and its effect given back for main.c to word (the place ruled: the
** ending).  What's been done is the world's to keep (with the game, its world.dat); the ending the quest's (main.c's
** Q, kept in the save).
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

/* The theme read in: libdir/dq0t.dat's t'th (counted round: the file's first says how many), or (t 0xFF) the game's
** own, its record theme.dat (themegen's) in the save's folder (the hub's savedir; /ram/dq0 without one), into this
** overlay's bank (TCOPY) and the hub's theme bank (THEME), through the main RAM (lb); its kinds of things the world's
** to put in the hub's catalogue, after the catalogue's own (the UI's IO_THEME, after) */
void __fastcall__ o_theme_load (unsigned char t)
{
    char* path = lb + 300;
    unsigned off;
    unsigned char k, own = t == 0xFF;
    int fd, n;

    if (own) {
        hub_mem (0, path, (const void*) HUB->savedir, sizeof HUB->savedir);
        if (!*path) {
            strcpy (path, "/ram/dq0");
        }
        strcat (path, "/theme.dat");
    } else {
        strcpy (path, libdir);
        strcat (path, "/dq0t.dat");
    }
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
    Q.theme = own ? 0xFF : t;                   /* (Its kinds the world's to put in the catalogue: IO_THEME) */
}

/* A theme's name for something (TN_*): its own, a stratum's by its number, into tname; a room's by its kind, a lit
** room's light, added to lb (words.c's) */
void __fastcall__ o_theme_named (unsigned char what, unsigned char n)
{
    if (what >= TN_ROOM) {
        strcat (lb, what != TN_ROOM ? T->light : n < 8 ? T->room[n] : T->uses[(n - 8) % TUSES]);
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
    unsigned char o;

    hub_mem (0, &o, (const void*) &HUB->way_open, 1);   /* (The world's: the puzzle's rules) */
    words_at (o ? T->opened : T->lock);
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

/* Rule r done (the world's judgement, IO_PUZZLE's, and its keeping: the way out opened, the rules done): its words
** told, and its effect (Q_*) */
static unsigned char __fastcall__ done (unsigned char r)
{
    const struct trule* p = &T->rule[r];

    words_at (p->words);
    if (p->effect == TE_OPEN) {
        return Q_OPEN;
    }
    return p->effect == TE_RULE ? Q_RULED : Q_DONE;
}

/* A command the puzzle's (read X; combine X with Y; put X on Y, use X on Y; use X on the way out, open or unlock it
** with X, examine it): Q_NONE (not the puzzle's: main.c carries it out as ever), Q_TOLD (words only), or what came
** of a rule done (Q_DONE, Q_OPEN, Q_RULED).  Its words here; whether it's done the world's, by the theme's rules
** (the things' request, IO_PUZZLE: the things found to hand, the rule for their kinds, the things made one or put,
** the way out opened) */
unsigned char __fastcall__ o_puzzle (struct pcmd* c)
{
    unsigned char v = c->verb, t, r;

    if (v == V_READ) {
        if (!c->word[0]) {
            msg ("Read what?");
            return Q_TOLD;
        }
        t = TR_READ;
    } else if (v == V_COMBINE) {
        if (!c->word[0] || !c->word2[0]) {
            msg ("Combine what with what? (combine the one with the other)");
            return Q_TOLD;
        }
        t = TR_COMBINE;
    } else if (c->obj == OB_WAY_OUT && (v == V_USE || v == V_UNLOCK || v == V_OPEN || v == V_EXAMINE)) {
        if (v == V_EXAMINE || !c->word[0]) {
            o_way_words ();
            return Q_TOLD;
        }
        t = TR_OUT;
    } else if ((v == V_PUT || v == V_USE) && c->word[0] && c->word2[0]) {
        t = TR_PUT;
    } else {
        if (v == V_USE) {
            msg ("Use what, and on what? (use the key on the way out ...)");
            return Q_TOLD;
        }
        return Q_NONE;
    }
    r = othings (IO_PUZZLE, c->word, c->word2, t);
    if (lb[0]) {
        msg (lb);                               /* (The things' words: one not to hand, one put) */
    }
    if (r == PZ_DONE) {
        return done (othings_x);
    }
    if (r == PZ_FAR) {
        msg ("The way out is out of reach: go over to it first.");
    } else if (r == PZ_NONE) {
        if (t == TR_PUT && v == V_PUT) {
            return Q_NONE;                      /* (A thing put as any other) */
        }
        if (t == TR_COMBINE) {
            msg ("Those don't go together.");
        } else {
            sayf (t == TR_READ ? "\nThere's nothing written on the %s." : t == TR_OUT ?
                "\nNothing comes of trying the %s on it." : "\nNothing comes of using the %s on the %s.", c->word,
                c->word2);
        }
    }
    return Q_TOLD;
}
