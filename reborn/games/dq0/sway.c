/*
** sway.c - (overlay 15; overlay 6's till 2026-10-29, its room) the character's ways with the denizens, in words: what the denizens' task answered (the hub's
** sway, copied into the main RAM: main.c's deal) to words that would sway one (talk, persuade, deceive, threaten,
** calm, befriend: the referee's grade, its attitude before and after), a request to an ally, a name given, a look
** at how one seems, a blow, a gift (the commands listed: help.c's)
*/

#pragma code-name ("OV15CODE")
#pragma rodata-name ("OV15RODATA")

#include <string.h>
#include "dq0.h"
#include "hub.h"

/* How it seems (hub.h's AT_*) */
static const char* const seems[AT_ALLIED + 1] = {
    "hostile", "unfriendly", "wary of you", "indifferent to you", "friendly", "your ally"
};

/* What the character does, each way of swaying one (SW_TALK ...: "You talk with" the giant rat ...) */
static const char* const tried[SW_BEFRIEND + 1] = {
    "", "You talk with ", "You try to win over ", "You spin a tale to ", "You threaten ", "You speak softly to ",
    "You coax "
};

/* An ally's request done: one ally's, then several's (OR_FOLLOW ...) */
static const char* const done1[3] = { " follows you.", " waits here.", " goes its own way." };
static const char* const done2[3] = { " follow you.", " wait here.", " go their own way." };

static const struct sway* S;

/* The one it's about, into lb: "Grub" (a name of its own), or "the giant rat" ("The" to start a sentence, c) */
static void __fastcall__ who (unsigned char c)
{
    if (S->name[0] < 'A' || S->name[0] > 'Z') {
        strcat (lb, c ? "The " : "the ");
    }
    strcat (lb, S->name);
}

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

/* "It seems wary of you now." */
static void __fastcall__ seeming (const char* before, const char* after)
{
    add (before);
    add (seems[S->now]);
    add (after);
}

void __fastcall__ o_sway_words (const struct sway* s)
{
    unsigned char h = s->kind;

    S = s;
    lb[0] = 0;
    switch (s->result) {
    case SR_NONE:
        if (h == SW_ORDER && !s->word[0]) {
            add ("You have no ally here to hear you.");
        } else {
            add ("You see no ");
            add (s->word[0] ? s->word : "one");
            add (" here.");
        }
        return;
    case SR_FAR:
        who (1);
        add (s->beast || h == SW_GIVE ? " is too far off: you'd need to be within reach." : " is too far off to hear you.");
        return;
    case SR_TONGUE:
        who (1);
        add (" doesn't speak any tongue you know.");
        return;
    case SR_BEAST:
        who (1);
        add (" is an animal: words won't reach it (calm it, or befriend it).");
        return;
    case SR_MINDLESS:
        who (1);
        add (" pays no heed: there's no mind in it to sway.");
        return;
    case SR_DOWN:
        who (1);
        add (" lies senseless: it can't hear you.");
        return;
    case SR_AS_FAR:
        who (1);
        seeming (" seems ", " already: that won't move it further.");
        return;
    case SR_NOT_ALLY:
        who (1);
        add (h == SW_NAME ? " isn't friendly enough to take a name from you." :
             " isn't your ally.");
        return;
    case SR_CANT:
        if (h == SW_GIVE) {
            who (1);
            add (" has no use for it.");        /* (A beast offered what isn't food: slice 4.8's) */
        } else if (h == SW_NAME) {
            add (s->word2[0] ? "You've given names enough." : "Name it what?");
        } else if (s->order) {
            who (1);
            add (" doesn't know how to do that.");
        } else {
            add ("Ask it to do what? (follow me, wait, open the door, go home)");
        }
        return;
    }
    switch (h) {
    case SW_LOOK:
        who (1);
        seeming (" seems ", ".");
        return;
    case SW_NAME:
        who (1);
        add (" is ");
        add (s->word2);
        add (" now.");
        return;
    case SW_ORDER:
        if (s->order >= OR_FETCH) {             /* (An errand: slice 4.8's, one ally's; at a door, 2026-11-02) */
            who (1);
            add (s->order > OR_FETCH ? " goes to the door" : " goes to fetch the ");
            add (s->word2);
            add (".");
        } else if (s->name[0]) {
            who (1);
            add (done1[(s->order - OR_FOLLOW) % 3]);
        } else {
            add ("Your allies");
            add (done2[(s->order - OR_FOLLOW) % 3]);
        }
        return;
    }
    if (h == SW_GIVE) {                         /* (A gift: slice 4.8's) */
        add ("You offer ");
        who (0);
        add (" the ");
        add (s->word2);
        add (". ");
        if (s->grade == G_CF) {
            seeming ("It won't take it: it's offended, and seems ", " now.");
            return;
        }
    } else {
        add (tried[h]);                         /* (Words: the referee's test, and what came of it) */
        who (0);
        add (". ");
    }
    if (s->now > s->was) {
        if (s->now == AT_ALLIED) {
            add ("It's won over: it will follow you now.");
        } else {
            seeming (s->grade == G_CS ? "It warms to you: it seems " : "It's swayed: it seems ", " now.");
        }
    } else if (s->now == s->was) {
        seeming ("It isn't swayed: it seems ", " still.");
    } else {
        seeming ("It goes badly: it seems ", " now.");
    }
}

/* The thing named may be a denizen, by what's in the hub (the first three letters of the word in an actor's name, or one
** the player has named: the denizens' own meant is the judge), so that they're asked: they're the background, and an
** answer waiting on one of their minds' turns took a second and more (dq0answer's, 2026-11-03: "examine the torch") */
#pragma code-name (push, "OV16CODE")            /* (Overlay 16's since 2026-11-04, overlay 17's room) */
unsigned char o_may_den (void)
{
    char nm[sizeof HUB->actor[0].name + 1];     /* (On the C stack: hub_mem's, with the hub in the window) */
    static unsigned char k, c;

    if (!cmd.obj.word[0]) {
        return 0;
    }
    if (names_n) {
        return 1;
    }
    c = cmd.obj.word[3];
    cmd.obj.word[3] = 0;                        /* (Its first three letters, a moment) */
    nm[sizeof nm - 1] = 0;
    for (k = 0; k < NACTORS; ++k) {
        hub_mem (0, nm, HUB->actor[k].name, sizeof nm - 1);
        if (nm[0] && strstr (nm, cmd.obj.word)) {
            break;
        }
    }
    cmd.obj.word[3] = c;
    return k < NACTORS;
}
#pragma code-name (pop)

/* A walk over told as it begins, at once (the user's D17, a command answers at once: 2026-11-02), by the thing's name, or
** the command's thing (0); its arrival not "You go over to it." (placing, or a command's walk's reach, E_ARRIVE's).  (Here
** since 2026-11-03, a walk's first words no part's swap: an overlay's call takes nothing; overlay 17's, 15's room) */
#pragma code-name (push, "OV17CODE")
#pragma rodata-name (push, "OV17RODATA")
#pragma local-strings (push, on)
#pragma data-name (push, "OV17RODATA")
#pragma bss-name (push, "OV17RODATA")
#pragma static-locals (push, on)                /* (Its locals static, in its bank: the code the less) */
static const char* __fastcall__ port_name (unsigned char k)
{
    return k == OB_DOOR ? "door" : k == OB_WINDOW ? "window" : "portcullis";
}

/* A command's first word that isn't a thing's by its name (walk_says' name, under W_MAX): n a portal's work about to be
** asked of the world, its PA_* + 1, tx, ty the one meant (words that hold whatever comes of it; none if a turn to it comes
** first, its first words those); or W_DIG (a dig, the floor or the wall looked at), W_SWAY (a denizen swayed, a gift, a
** trade: the thing said), W_PEER, W_LISTEN.  Once a line (worded) */
static void __fastcall__ first_word (unsigned char n)
{
    const struct sight* s = sight_get ();
    const char* v = "reach for";
    const char* name = cmd.obj.word;
    const char* a = s->dark ? "feel" : "look at";
    unsigned char k = cmd.obj.kind;

    if (worded) {
        return;
    }
    if (n >= W_PEER) {
        msg (n == W_PEER ? "You peer ahead." : "You listen.");
        t_flush ();
        worded = 3;
        return;
    }
    if (n == W_SWAY) {
        v = s->dark ? "feel for" : "look for";
        if (!*name) {
            return;
        }
    } else if (n == W_DIG) {
        name = cmd.dir == GO_DOWN ? "floor" : "wall";
        v = a;
    } else {
        --n;
        if (!tx || (cmd.obj.lies <= H_NW && cmd.obj.lies != pc_face) || (k != OB_DOOR && k != OB_WINDOW && k != OB_PORTC)) {
            return;
        }
        name = port_name (k);
        v = tx - s->x + 1 > 2 || ty - s->y + 1 > 2 ? "head for" : n == PA_OPEN ? "try" : n == PA_CLOSE ? v : a;
    }
    sayf ("\nYou %s the %s.", v, name);
    t_flush ();
    worded = 1;
}
#pragma static-locals (pop)
#pragma data-name (pop)
#pragma bss-name (pop)

void __fastcall__ o_walk_says (const char* name)
{
    unsigned char k = cmd.obj.kind;

    if (name && (unsigned) name < W_MAX) {
        first_word ((unsigned char) name);
        return;
    }
    if (said_ahead) {
        said_ahead = 0;                         /* (Told already, walk_ahead's) */
        placing = 1;
        return;
    }
    if (!name) {
        name = cmd.obj.word;
    }
    if (*name && !k) {
        sayf ("\nYou go over to the %s.", name);
    } else if (k == OB_DOOR || k == OB_WINDOW || k == OB_PORTC) {
        sayf ("\nYou head for the %s.", port_name (k));
    } else {
        msg ("You go over to it.");
    }
    t_flush ();                                 /* (Out before the world's walk starts: the user's D17) */
    placing = 1;
}
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A command's first words at once, before the world's asked, whose answer's the time (its parts' copies, its chooser through
** the store; the user's D17, and 2026-11-04: "Word as they begin."): words that hold whatever the world then says, and say
** what will happen (the user's playtest, 2026-11-06: "You reach for the torch." of one in hand, in the pack, or nowhere).
** By what the character carries (the UI bank's ix, the world's: the one its chooser would take, things.c's pick, the
** state said) and what it sees lying about: in reach, "You reach for the sack."; in the worn pack, "You reach for your
** backpack."; in a sack, "You reach into the sack."; let go of, "You put down the torch." (all of it said: the world's
** "Put down." not again); a thing in hand, nothing to reach for, none (its answer's own); out of reach only, its walk
** over, the same words as ever (said_ahead, o_walk_says); nowhere, "You look for the sack." ("You feel for ..." in the
** dark).  Named by what tells it apart from another of its kind at hand: "You put down the spent torch.".  The rest
** (first_word, overlay 17's).  Overlay 20's since 2026-11-06, overlay 15's room */
#pragma code-name (push, "OV20CODE")
#pragma rodata-name (push, "OV20RODATA")
#pragma data-name (push, "OV20RODATA")
#pragma bss-name (push, "OV20RODATA")
#pragma local-strings (push, on)
#pragma static-locals (push, off)               /* (Its locals on the C stack: what's copied from the hub, with the hub in the
                                                **   window, can't be in this bank) */
static struct cix* ix;                          /* (What's carried, copied: the world's; o_walk_ahead's, on the C stack) */
static unsigned char nix, rk[NIX];
static char* kn;                                /* (A kind's name: the same) */

/* A name said a word of the kind's, or its start, or that word made plural, or the end of a name of one word (the world's
** named4, things.c's: the chooser's) */
static unsigned char __fastcall__ word_in (const char* k, const char* w, unsigned char n)
{
    for (;;) {
        if (!strncmp (k, w, n)) {
            return 1;
        }
        if (!(k = strchr (k, ' '))) {
            return 0;
        }
        ++k;
    }
}

static unsigned char __fastcall__ named_ (const char* k, const char* w)
{
    unsigned char n = strlen (w), m;

    return word_in (k, w, n) || (n > 2 && w[n - 1] == 's' && (word_in (k, w, n - 1) ||
        (w[n - 2] == 'e' && word_in (k, w, n - 2)))) || (n > 3 && (m = strlen (k)) > n && !strcmp (k + m - n, w));
}

/* Entry e's kind's name (in kn); whether it's a light */
static const char* __fastcall__ kname (unsigned char e)
{
    hub_mem (KIND_BANK, kn, CATALOGUE[ix[e].kind].name, sizeof CATALOGUE[0].name);
    return kn;
}

static unsigned char __fastcall__ alight (unsigned char e)
{
    unsigned char f;

    hub_mem (KIND_BANK, &f, &CATALOGUE[ix[e].kind].flags, 1);
    return f & K_LIGHT;
}

/* Entry e's state's word (AW_*: the world's aw_of) */
static unsigned char __fastcall__ state (unsigned char e)
{
    unsigned char f = ix[e].flags;

    if (alight (e)) {
        return f & CX_LIT ? AW_LIT : f & CX_SPENT ? AW_SPENT : AW_UNLIT;
    }
    return (f & (CX_SPENT | CX_EMPTY)) == (CX_SPENT | CX_EMPTY) ? AW_EMPTY : AW_NONE;
}

/* The outermost entry e's in (e, in none) */
static unsigned char __fastcall__ top (unsigned char e)
{
    while (ix[e].up && ix[e].up <= nix) {
        e = ix[e].up - 1;
    }
    return e;
}

/* The entry the world's chooser would take, of those carried named w, in the state sel says (SL_*, its n times 16), for
** op (a light lit: not lit, not spent unless a state's said; put out: lit), in entry in less 1 (0: anywhere): held first,
** then on the body, then the shallower in what it carries, the store's order else; the nth, the other.  0xFF none */
static unsigned char __fastcall__ ix_pick (const char* w, unsigned char sel, unsigned char op, unsigned char in)
{
    unsigned char e, j, f, r, s = sel & 15, n = s == SL_NTH ? sel >> 4 : s == SL_OTHER ? 2 : 1;

    if (!*w) {
        return 0xFF;
    }
    for (e = 0; e < nix; ++e) {
        f = ix[e].flags;
        rk[e] = 0xFF;
        if (!named_ (kname (e), w) || (s == SL_LIT ? !(f & CX_LIT) : sel == SL_NEW ? f & (CX_HELD | CX_LIT) : s == SL_UNLIT ?
            (f & CX_LIT) || !alight (e) : s == SL_OPEN ? f & CX_SHUT : s == SL_SHUT ? !(f & CX_SHUT) : sel == SL_SPENT ?
            !(f & CX_SPENT) : sel == SL_FULL ? f & CX_EMPTY : s == SL_EMPTY ? !(f & CX_EMPTY) :
            op == IO_LIGHT ? (f & CX_LIT) || (!s && (f & CX_SPENT)) : op == IO_DOUSE && !(f & CX_LIT))) {
            continue;
        }
        for (r = 1, j = e; ix[j].up && ix[j].up <= nix && ix[j].up != in; ++r) {
            j = ix[j].up - 1;
        }
        if (in && ix[j].up != in) {
            continue;                           /* (Not in the one said) */
        }
        rk[e] = r == 1 && (f & CX_HELD) ? 0 : r;
    }
    for (r = 0; r < 12; ++r) {
        for (e = 0; e < nix; ++e) {
            if (rk[e] == r && !--n) {
                return e;
            }
        }
    }
    return 0xFF;
}

/* Entry e's word that tells it apart from another of its kind at hand (carried; lying within reach, lit or not as it's
** seen), else "" */
static const char* __fastcall__ apart_of (unsigned char e, unsigned char lit, unsigned char unlit)
{
    static const char* const aw[AW_EMPTY + 1] = { "", "lit ", "unlit ", "spent ", "empty " };   /* (Its words in this bank: a
                                                **   function's, local-strings') */
    unsigned char a = state (e), j, b = lit | unlit;

    for (j = 0; j < nix; ++j) {
        if (j != e && ix[j].kind == ix[e].kind) {
            if (state (j) == a) {
                return "";
            }
            b = 1;
        }
    }
    return b && !(lit && a == AW_LIT) && !(unlit && a != AW_LIT) ? aw[a] : "";
}

/* A place said where a thing's other would be ("take the torch from the floor", "put it on the ground": the user's
** playtest, 2026-11-06, "You see no floor here"): where things lie about, not a thing: taken up, put down, of those lying
** there (the world's chooser leaves out those carried) */
void o_place_said (void)
{
    static const char* const places[] = { "floor", "ground", "bottom", "here" };
    unsigned char k;

    for (k = 0; k < sizeof places / sizeof places[0]; ++k) {
        if (!strcmp (cmd.other.word, places[k])) {
            cmd.other.word[0] = cmd.other.sel = cmd.other.n = 0;
            cmd.prep = PR_NONE;
            if (cmd.action == VB_PUT_OBJECT) {
                cmd.action = VB_DROP_OBJECT;
            }
            return;
        }
    }
}

/* The state said, as a word ("the spent torch") */
static const char* __fastcall__ sel_word (unsigned char sel)
{
    return sel == SL_LIT ? "lit " : sel == SL_UNLIT ? "unlit " : sel == SL_SPENT ? "spent " : sel == SL_EMPTY ? "empty " :
        sel == SL_FULL ? "full " : sel == SL_NEW ? "new " : "";
}

void __fastcall__ o_walk_ahead (const char* name)
{
    struct sight* s = sight_get ();
    struct sitem* it;
    const char* v = "reach for";
    const char* w = "";
    unsigned char k, away = 0, op = walk_op & ~IO_STEPS, lit = 0, unlit = 0, e, y, sel, in = 0;
    struct cix b[NIX];
    char nb[sizeof CATALOGUE[0].name];
    static const char ops[] = { IO_GET, IO_DROP, IO_FILL, IO_HIDE, IO_SHUT, IO_THROW, IO_DESCRIBE, IO_INSIDE, 0 };

    if (worded) {                               /* (Told once a line: not again as its command's asked again after a step) */
        return;
    }
    if (*name == '#' || !*name || (!strchr (ops, op) && (op < IO_HOLD || op > IO_DOUSE))) {
        return;
    }
    for (k = s->nitems, it = s->item; k; --k, ++it) {
        if (strstr (it->name, name)) {
            if (it->x - s->x + 1 > 2 || it->y - s->y + 1 > 2) {
                away |= 1;
            } else {
                away |= 2;
                if (it->hid & 4) {              /* (Lit, as it's seen) */
                    lit = 1;
                } else {
                    unlit = 1;
                }
            }
        }
    }
    sel = cmd.obj.sel | cmd.obj.n << 4;
    if (op == IO_GET) {                         /* (Taken up: of what it doesn't carry, as ever) */
        if (!away) {
            if (s->dark) {
                return;
            }
            v = "look for";
        } else if (away == 1) {                 /* (Out of reach only: walked to) */
            if (!(cmd.obj.sel | cmd.obj.n)) {
                walk_says (name);
                placing = 0;
                said_ahead = 1;
            }
            return;
        }
        goto say;
    }
    ix = b;
    kn = nb;
    hub_mem (UI_BANK, &k, &UIREC->ixn, 1);
    nix = k > NIX ? NIX : k;
    hub_mem (UI_BANK, b, UIREC->ix, nix * sizeof (struct cix));
    if (op == IO_TAKE) {                        /* (From the other: in it, if it's carried) */
        if ((y = ix_pick (cmd.other.word, cmd.other.sel | cmd.other.n << 4, 0, 0)) == 0xFF) {
            for (k = s->nitems, it = s->item; k && (!strstr (it->name, cmd.other.word) || it->x - s->x + 1 > 2 ||
                it->y - s->y + 1 > 2); --k, ++it) {
            }
            sayf (k ? "\nYou reach into the %s." : s->dark ? "\nYou feel for the %s." : "\nYou look for the %s.", cmd.other.word);
            goto told;                          /* (Not carried: lying in reach, reached into; else looked for) */
        }
        in = y + 1;
    }
    e = ix_pick (name, sel, op, in);
    if (op == IO_DESCRIBE || op == IO_INSIDE) { /* (Looked at, asked of it: its quality, its weight ...; way.c's.  In the
                                                **   dark, the answer's own) */
        if (s->dark) {
            return;
        }
        v = e != 0xFF || away ? "look at" : "look for";
        goto say;
    }
    if (e == 0xFF) {
        if (op != IO_DROP && op != IO_HIDE && op != IO_THROW && op != IO_TAKE && away) {
            if (away == 1) {
                return;                         /* (Out of reach only: walked to, the world's IR_NOT_HERE) */
            }
            goto say;                           /* (Lying within reach) */
        }
        v = s->dark ? "feel for" : "look for";  /* (None it carries, nor sees in reach: looked for, then the world's why) */
        w = sel_word (sel);
        goto say;
    }
    name = kname (e);
    w = apart_of (e, lit, unlit);
    y = 0xFF;
    if (op == IO_STOW || op == IO_PUT) {        /* (Put in the other, the pack: in it already, the world's words, nothing
                                                **   reached for) */
        y = op == IO_STOW ? 0xFF : ix_pick (cmd.other.word, cmd.other.sel | cmd.other.n << 4, 0, 0);
        for (k = 0; op == IO_STOW && k < nix; ++k) {
            if (ix[k].flags & CX_BACK) {
                y = k;
            }
        }
        if (y != 0xFF && ix[e].up == y + 1) {
            return;
        }
    }
    if ((k = top (e)) != e) {                   /* (In something: the pack worn first, or what it's in) */
        sayf (ix[k].flags & CX_BACK ? "\nYou reach for your %s." : "\nYou reach into the %s.", kname (k));
        goto told;
    }
    if (op == IO_DROP) {
        if (cmd.n || cmd.obj.sel == SL_ALL || (cmd.flags & CF_HALF)) {
            return;                             /* (By the count: the world's words) */
        }
        sayf ("\nYou put down the %s%s.", w, name);
        t_flush ();
        worded = 4;                             /* (All of it said: not "Put down." again) */
        return;
    }
    if (op == IO_STOW || op == IO_PUT) {        /* (Put in the other: the pack worn first, or the other in reach) */
        if (y != 0xFF && (ix[k = top (y)].flags & CX_BACK)) {
            sayf ("\nYou reach for your %s.", kname (k));
            goto told;
        }
        if (y != 0xFF && !(ix[y].flags & CX_HELD)) {
            sayf ("\nYou reach for the %s.", kname (y));
            goto told;
        }
    }
    if (ix[e].flags & CX_HELD) {
        if (op != IO_LIGHT && op != IO_DOUSE) {
            return;                             /* (In hand: nothing to reach for, its answer's own) */
        }
        v = "raise";                            /* (A light in hand lit, put out: "You raise the torch.", whether it catches or
                                                **   not; its answer two seconds off, a look in the new light) */
    }
say:
    sayf ("\nYou %s the %s%s.", v, w, name);
told:
    t_flush ();
    worded = 1;
}
#pragma static-locals (pop)
#pragma local-strings (pop)
#pragma bss-name (pop)
#pragma data-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)
