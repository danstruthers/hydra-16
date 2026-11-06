/*
** theme.c - (overlay 5) Dingus Quest Zero's theme (theme.h), the UI's: its words (its name and story, its strata's
** and rooms' names, the way out shut and open, the endings) and its puzzle's words, from the theme's copy in this
** overlay's own bank (TCOPY: main.c reads the theme into it as the game starts, for the hub isn't in the window while
** an overlay is).  The puzzle: a command for a piece (read the tablet, combine the sunstone with the moonstone, put
** the crown on the throne, use the seal on the way out) is the world's to judge by the theme's rules (items.h's
** IO_PUZZLE: the things found to hand, the rule for their kinds, done: the things made one, put, the way out opened,
** the hub's way_open), its words told here, the dungeon solved by it noted (Q.solved: the ending it will have as the
** character gets out).  What's been done is the world's to keep (with the game, its world.dat); the ending the quest's (main.c's
** Q, kept in the save).
*/

#pragma code-name ("OV5CODE")
#pragma rodata-name ("OV5RODATA")
#pragma local-strings (on)                      /* (Each function's words in its own overlay: its last, 15's, below) */

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
    int fd;

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
        hy_exits ("dq0: no lib/dq0/dq0t.dat");
    }
    for (k = 0; ; ++k) {                        /* (Read straight into the overlay's copy, one read a theme: the
                                                **   start's time, slice 4.4's) */
        read (fd, T, THEME_SIZE);
        if (!k) {
            t %= T->count;
        }
        if (k == t) {
            break;
        }
    }
    close (fd);
    for (off = 0; off < THEME_SIZE; off += 256) {
        memcpy (lb, (char*) T + off, 256);
        hub_mem (THEME_BANK, (char*) THEME + off, lb, 256);
    }
    Q.theme = own ? 0xFF : t;                   /* (Its kinds the world's to put in the catalogue: IO_THEME) */
}

/* Its words at offset w (0: none), said on a line of their own */
static void __fastcall__ words_at (unsigned w)
{
    if (w) {
        msg ((const char*) T + w);
    }
}

/* A theme's name for something (TN_*): its own, a stratum's by its number, into tname; a room's by its kind, a lit
** room's light, added to lb (words.c's) */
static void __fastcall__ gone (unsigned char w);

void __fastcall__ o_theme_named (unsigned char what, unsigned char n)
{
    if (what >= TN_END) {
        if (what == TN_END) {
            words_at (T->end[n & 3]);
        } else {
            gone (n);
        }
        return;
    }
    if (what >= TN_ROOM) {
        strcat (lb, what != TN_ROOM ? T->light : n < 8 ? T->room[n] : T->uses[(n - 8) % TUSES]);
    } else {
        strcpy (tname, what == TN_THEME ? T->name : T->stratum[n & 7]);
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

/* The way out: shut (the lock's words), or open; another way out, below the top, the same words whatever the theme's are */
void o_way_words (void)
{
    unsigned char o, sn;

    hub_mem (0, &o, (const void*) &HUB->way_open, 1);   /* (The world's: the puzzle's rules) */
    hub_mem (0, &sn, (const void*) &HUB->pc_stratum, 1);
    if (sn) {
        msg ("A stair climbs to the open air: go up to leave.");
    } else {
        words_at (o ? T->opened : T->lock);
    }
}

/* Rule r done (the world's judgement, IO_PUZZLE's, and its keeping: the way out opened, the rules done): its words
** told, and its effect (Q_*) */
static unsigned char __fastcall__ done (unsigned char r)
{
    const struct trule* p = &T->rule[r];

    words_at (p->words);
    if (p->effect) {                            /* (Solved: told, and it's still to walk out, Q2's) */
        if (!Q.solved) {
            msg ("That's what you came to do.  Now to get out alive.");
            Q.solved = END_ESCAPED;
        }
        if (p->effect == TE_RULE) {
            Q.solved = END_RULED;
        }
    }
    return Q_DONE;
}

/* E_LEFT's (w, the world's, less EL_WAY): a take rule done (EL_RULE on), told; or the character out of the dungeon, by a shaft to the
** sky or a way out, the ending it's earned.  Solved, that's its ending; a through goal's far exit, the going out's
** the solving; else left, unsolved, a stair walked up only as it's meant (told the first time, the same again goes: the
** theme's copy's count, read only as it's loaded, 0 for told) */
static void __fastcall__ gone (unsigned char w)
{
    unsigned char how = Q.solved;

    if (w >= EL_RULE - EL_WAY) {
        done (w - (EL_RULE - EL_WAY));
        return;
    }
    if (!how) {
        how = END_LEFT;
        if (!w) {
            if (T->goal == G_THROUGH) {         /* (Its stratum 0's never opens: the far exit, this) */
                how = END_ESCAPED;
            } else if (T->count) {
                T->count = 0;
                msg ("That's the way out, and what you came for undone.  Go up again to leave it so.");
                return;
            }
        }
    }
    ending (how);
}

/* A command the puzzle's (read X; combine X with Y; put X on Y, use X on Y; use X on the way out, open or unlock it
** with X, examine it): Q_NONE (not the puzzle's: main.c carries it out as ever), Q_TOLD (words only), or a rule
** done (Q_DONE).  Its words here; whether it's done the world's, by the theme's rules
** (the things' request, IO_PUZZLE: the things found to hand, the rule for their kinds, the things made one or put,
** the way out opened) */
#pragma data-name (push, "OV5RODATA")
#pragma bss-name (push, "OV5RODATA")
#pragma static-locals (push, on)                /* (Its locals static, in its bank: the code the less) */
unsigned char __fastcall__ o_puzzle (struct cmd* c)
{
    unsigned char v = c->action, t, r;

    if (v == VB_READ_OBJECT) {                  /* (A writing read, or copied: read.c's, overlay 20, the plan's step 9) */
        return read_cmd (c);
    }
    if (v == VB_COMBINE_OBJECT) {
        if (!c->obj.word[0] || !c->other.word[0]) {
            msg ("Combine what with what? (combine the one with the other)");
            return Q_TOLD;
        }
        t = TR_COMBINE;
    } else if (c->obj.kind == OB_WAY_OUT && (v == VB_USE_OBJECT || v == VB_UNLOCK_OBJECT || v == VB_OPEN_OBJECT ||
        v == VB_EXAMINE_OBJECT)) {
        if (v == VB_EXAMINE_OBJECT || !c->obj.word[0]) {
            o_way_words ();
            return Q_TOLD;
        }
        t = TR_OUT;
    } else if ((v == VB_PUT_OBJECT || v == VB_HANG_OBJECT || v == VB_USE_OBJECT) && c->obj.word[0] && c->other.word[0]) {
        t = TR_PUT;
    } else {
        if (v == VB_USE_OBJECT) {
            msg ("Use what, and on what? (use the key on the way out ...)");
            return Q_TOLD;
        }
        return Q_NONE;
    }
    if (t != TR_OUT) {
        walk_op = IO_PUT;                       /* (Its first words before this asking: walk_ahead's, sway.c) */
        walk_ahead (c->obj.word);
    }
    r = othings (IO_PUZZLE, c->obj.word, c->other.word, t);
    if (lb[0]) {
        msg (lb);                               /* (The things' words: one not to hand, one put) */
    }
    if (r == PZ_DONE) {
        return done (othings_x);
    }
    if (r == PZ_FAR) {
        reach_go ();                            /* (Walked over to it first, then again) */
    } else if (r == PZ_NONE) {
        if (t == TR_PUT && v != VB_USE_OBJECT) {
            return Q_NONE;                      /* (A thing put as any other) */
        }
        if (t == TR_COMBINE) {
            msg ("Those don't go together.");
        } else {
            sayf (t == TR_OUT ?
                "\nNothing comes of trying the %s on it." : "\nNothing comes of using the %s on the %s.", c->obj.word,
                c->other.word);
        }
    }
    return Q_TOLD;
}
#pragma static-locals (pop)
#pragma data-name (pop)
#pragma bss-name (pop)

/* ---- The ending: overlay 15's (5's room), the theme's words asked of overlay 5 */

#pragma code-name ("OV15CODE")
#pragma rodata-name ("OV15RODATA")

/* An ending's words (END_*), and the game's end */
void __fastcall__ o_ending (unsigned char how)
{
    blank ();
    if (how >= EL_WAY) {
        theme_named (TN_GONE, how - EL_WAY);
        return;
    }
    if (how == END_DIED && Q.solved) {          /* (Q2: dying on the way out still solved) */
        msg ("You did what you came to do, and you won't be climbing out to tell of it.");
    } else {
        theme_named (TN_END, how);
    }
    blank ();
    if (how != END_DIED) {
        clues_kept ();                          /* (Its writings' clues kept for dq0life: read.c's, the plan's step 10) */
    }
    if (savedir && how != END_DIED) {           /* (What's next: D8's, slice 4.4's; after a death, dq0end's, which reads the folder for it) */
        msg ("Your character carries on: start the game again with this save's folder, and go on into a new dungeon.");
    }
    msg ("The game is over.");
    blank ();
    if (how < END_DIED) {
        Q.solved = how;                         /* (A far exit gone out by, solved as it goes) */
    }
    Q.done = how;
    quit = 1;
}
