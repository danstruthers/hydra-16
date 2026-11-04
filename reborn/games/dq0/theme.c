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
        hy_exits ("dq0: its themes (lib/dq0/dq0t.dat) not found");
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
    if (savedir) {                              /* (What's next: D7's, D8's, slice 4.4's) */
        msg (how != END_DIED ? "Your character carries on: start the game again with this save's folder, and go on into a new dungeon." :
            hard ? "Hard mode: there's no save to go back to." :
            "Your last save is kept: start the game again with its folder to go back to it.");
    }
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
#pragma data-name (push, "OV5RODATA")
#pragma bss-name (push, "OV5RODATA")
#pragma static-locals (push, on)                /* (Its locals static, in its bank: the code the less) */
unsigned char __fastcall__ o_puzzle (struct cmd* c)
{
    unsigned char v = c->action, t, r;

    if (v == VB_READ_OBJECT) {
        if (!c->obj.word[0]) {
            msg (c->obj.kind ? "There's nothing written on it." : "Read what?");
            return Q_TOLD;
        }
        hub_mem (SIGHT_BANK, lb, &SIGHT->dark, 1);   /* (Not t: its statics in this bank, in the window) */
        if (lb[0]) {
            msg ("You can't see to read.");     /* (In the dark, or blind: none of it, of anything) */
            return Q_TOLD;
        }
        t = TR_READ;
    } else if (v == VB_COMBINE_OBJECT) {
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
    if (t == TR_READ && c->obj.word[0] && othings (IO_LOOK, c->obj.word, "", LM_COND) == IR_NOT_HERE) {
        reach_go ();                            /* (Out of reach: walked over to first, then again, as any inspecting) */
        return Q_TOLD;
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
            sayf (t == TR_READ ? "\nThere's nothing written on the %s." : t == TR_OUT ?
                "\nNothing comes of trying the %s on it." : "\nNothing comes of using the %s on the %s.", c->obj.word,
                c->other.word);
        }
    }
    return Q_TOLD;
}
#pragma static-locals (pop)
#pragma data-name (pop)
#pragma bss-name (pop)
