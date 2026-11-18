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

/* What the denizens' task answered (the hub's sway, copied into the main RAM: main.c's deal) to words that would sway
** one, told: said.txt's SWAY rows, the UI's own kind (UK_SWAY; sway_words' words before, 2026-11-12: narration as data,
** stage 4).  Here what's the sway's own: the one it's about as it's said ("the giant rat", "Grub"), how its attitude
** moved; one not seen, an animal out of reach, the refusals' */
void __fastcall__ o_sway_said (const struct sway* s)
{
    struct msg c;
    unsigned char h = s->kind, r = s->result, f = 0;

    if (s->actor & 0x7F) {                      /* (The one it's about a person, "they": the hub's folk, 2026-11-17) */
        hub_mem (0, &f, &HUB->folk[((s->actor & 0x7F) - 1) & 7], 1);
    }
    lb[0] = 0;
    if ((r == SR_NONE && s->word[0]) || (r == SR_FAR && (s->beast || h == SW_GIVE))) {
        say_no (r == SR_NONE ? A_SEE_NO | A_LB : A_FAR | A_LB | (f ? A_THEY : 0), r == SR_NONE ? s->word : s->name);
        msg (lb);
        return;
    }
    said_s[0] = lb + 300;                       /* (The main RAM's, before the sway's copy at lb's end) */
    lb[300] = 0;
    if (s->name[0]) {
        strcpy (lb + 300, s->name[0] < 'A' || s->name[0] > 'Z' ? "the " : "");
        strcat (lb + 300, s->name);
    }
    said_s[2] = s->word2;
    said_n = h == SW_GIVE && s->grade == G_BADLY ? 5 : s->now > s->was ? (s->now == AT_ALLIED ? 0 : s->grade == G_WELL ?
        1 : 2) : s->now == s->was ? 3 : 4;
    c.kind = UK_SWAY;
    c.who = 0;
    c.a = s->order;
    c.b = s->now;
    c.w1 = r << 8 | h;
    c.w2 = f;
    outcome_words (&c);
}

/* The thing named may be a denizen, by what's in the hub (the first three letters of the word in an actor's name, or one
** the player has named: the denizens' own meant is the judge), so that they're asked: they're the background, and an
** answer waiting on one of their minds' turns took a second and more (dq0answer's, 2026-11-03: "examine the torch").
** Looser than the one rule (main.c's name_is, 2026-11-07), as it's only a maybe: a denizen it names has those letters */
#pragma code-name (push, "OV14CODE")            /* (Overlay 14's since 2026-11-06, 16's room; 16's since 2026-11-04, 17's) */
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
#pragma static-locals (push, off)
/* A command's first words told (said.txt's FIRST rows, the UI's own kind: its verb's begun row, the words D17 says at
** once, 2026-11-12): form f (FS_*), verb v (FV_*), the word that tells it apart w, its name n (copied into the main RAM,
** for the words' overlay; 0 none) */
static void __fastcall__ first17 (unsigned char f, unsigned char v, const char* w, const char* n)
{
    struct msg c;

    said_s[3] = strcpy (lb + 300, w ? w : "");
    said_s[2] = strcpy (lb + 340, n ? n : "");
    said_n = v;
    c.kind = UK_FIRST;
    c.who = 0;
    c.w1 = f << 8;
    outcome_words (&c);
}
#pragma static-locals (pop)

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
    unsigned char v = FV_REACH;
    const char* name = cmd.obj.word;
    unsigned char a = s->dark ? FV_FEEL : FV_LOOK_AT;
    unsigned char k = cmd.obj.kind;

    if (worded) {
        return;
    }
    if (n >= W_PEER) {
        first17 (n == W_PEER ? FS_PEER : FS_LISTEN, 0, 0, 0);
        t_flush ();
        worded = 3;
        return;
    }
    if (n == W_SWAY) {
        v = s->dark ? FV_FEEL_FOR : FV_LOOK_FOR;
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
        v = tx - s->x + 1 > 2 || ty - s->y + 1 > 2 ? FV_HEAD : k == OB_PORTC ? a : n == PA_OPEN ? FV_TRY : n == PA_CLOSE ?
            v : a;                              /* (A portcullis opened, closed: as it's raised, lowered, 2026-11-14) */
    }
    first17 (FS_THE, v, 0, name);
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
        first17 (FS_THE, FV_OVER, 0, name);
    } else if (k == OB_DOOR || k == OB_WINDOW || k == OB_PORTC) {
        first17 (FS_THE, FV_HEAD, 0, port_name (k));
    } else {
        first17 (FS_IT, 0, 0, 0);
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
static struct cix ix[NIX];                      /* (What's carried, copied: the world's, by way of the C stack) */
static unsigned char nix, rk[NIX];
static char* kn;                                /* (A kind's name as it's copied: the same) */
static struct cixk ck[NIXK];                         /* (The kinds of what it carries, their flags and names: the world's, copied
                                                **   with it, no catalogue read, the user's D17; o_walk_ahead's) */
static unsigned char nck, cm[NIXK];             /* (How many; whether the word said names each, 0xFF not yet asked) */

/* The UI bank's bytes copied into this bank, by way of the C stack (the two share the window), a piece at a time */
static void __fastcall__ from_ui (void* to, const void* from, unsigned n)
{
    char t[128];
    unsigned k;

    while (n) {
        k = n > sizeof t ? sizeof t : n;
        hub_mem (UI_BANK, t, from, k);
        memcpy (to, t, k);
        to = (char*) to + k;
        from = (const char*) from + k;
        n -= k;
    }
}

/* The entry a letter named word w by (letters.c's ldes: its id, the entry whose letter's thing has it, the UI bank's low),
** or 0xFF: the thing itself, not the one most to hand (the user's designators, 2026-11-06) */
static unsigned char __fastcall__ des_e (const char* w)
{
    unsigned low[NLET];
    unsigned char k, j;

    for (k = 0; k < ndes; ++k) {
        if (ldes[k].id && !strcmp (w, ldes[k].word)) {
            hub_mem (UI_BANK, low, UIREC->low, sizeof low);
            for (j = 0; j < NLET && low[j] != ldes[k].id; ++j) {
            }
            for (k = 0; k < nix && ix[k].let != j + 1; ++k) {
            }
            return k < nix ? k : 0xFF;
        }
    }
    return 0xFF;
}

/* The carried thing entry e, the first words' choice for word w, made the one the world's asked of: its id the line's
** (letters.c's ldes, its "#" and id in the request: des_req), as a letter's is; the world doesn't choose again (the
** chooser survey's disagreements, COMMANDS-CRITIQUE.md's §5.1 step C, 2026-11-07).  One with no id yet, the world's */
static void __fastcall__ bind (const char* w, unsigned char e)
{
    unsigned low[NLET];
    unsigned id;

    if (!ix[e].let || ndes >= NDES) {
        return;
    }
    hub_mem (UI_BANK, low, UIREC->low, sizeof low);
    if (!(id = low[ix[e].let - 1]) || (id & LT_REC)) {
        return;
    }
    ldes[ndes].id = id;
    ldes[ndes].low = 1;
    strncpy (ldes[ndes].word, w, sizeof ldes[0].word - 1);
    ldes[ndes].word[sizeof ldes[0].word - 1] = 0;
    ++ndes;
    if (!strcmp (w, cmd.obj.word)) {
        it_id = id;                             /* ("it" this one too: resolve.c's, step B's) */
    }
}

/* Entry e's kind's place among those looked up (its name and flags read from the catalogue once a command); its name;
** whether it's a light */
static const char* __fastcall__ kname (unsigned char e)
{
    return ix[e].kind < nck ? ck[ix[e].kind].name : "thing";
}

static unsigned char __fastcall__ alight (unsigned char e)
{
    return ix[e].kind < nck ? ck[ix[e].kind].flags & K_LIGHT : 0;
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
    for (e = 0; e < nck; ++e) {
        cm[e] = name_is (ck[e].name, w);        /* (The word asked of each kind once: the one rule, the world's, main.c's) */
    }
    for (e = 0; e < nix; ++e) {
        f = ix[e].flags;
        rk[e] = 0xFF;
        if (ix[e].kind >= nck || !cm[ix[e].kind] || (s == SL_LIT ? !(f & CX_LIT) : sel == SL_NEW ? f & (CX_HELD | CX_LIT) : s == SL_UNLIT ?
            (f & CX_LIT) || !alight (e) : s == SL_OPEN ? !(ck[ix[e].kind].flags & CK_CLOSES) || (f & CX_SHUT) : s == SL_SHUT ? !(f & CX_SHUT) :
            sel == SL_SPENT ? !(f & (CX_SPENT | CX_EMPTY)) : sel == SL_FULL ? (f & (CX_FILLS | CX_EMPTY)) != CX_FILLS : s == SL_EMPTY ?
            !(f & CX_EMPTY) : op == IO_LIGHT ? ((f & CX_LIT) && (sel || !(f & CX_HELD))) || (!s && (f & (CX_SPENT | CX_LIT)) ==
            CX_SPENT) : op == IO_DOUSE && !(f & CX_LIT))) {
            continue;
        }
        for (r = 1, j = e; ix[j].up && ix[j].up <= nix && ix[j].up != in; ++r) {
            j = ix[j].up - 1;
        }
        if (in && ix[j].up != in) {
            continue;                           /* (Not in the one said) */
        }
        rk[e] = r == 1 && (f & CX_HELD) ? op == IO_LIGHT && (f & CX_LIT) : r + 1;   /* (A light lit in a hand, none
                                                **   said: after one unlit in a hand, before any other: "lit already",
                                                **   the world's held_lit, 2026-11-07) */
        if (op == IO_DON ? f & CX_WORN : op == IO_DOFF && !(f & CX_WORN)) {
            rk[e] = 9;                          /* (To wear, one not worn first; to take off, one worn: the world's pick_,
                                                **   the second ring, 2026-11-10) */
        }
    }
    for (r = 0; r < 13; ++r) {
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

/* A command's first words told (said.txt's FIRST rows, the UI's own kind: its verb's begun row, the words D17 says at
** once, 2026-11-12): form f (FS_*), verb v (FV_*), the word that tells it apart w, its name n (copied into the main RAM,
** for the words' overlay; 0 none) */
static void __fastcall__ first20 (unsigned char f, unsigned char v, const char* w, const char* n)
{
    struct msg c;

    said_s[3] = strcpy (lb + 300, w ? w : "");
    said_s[2] = strcpy (lb + 340, n ? n : "");
    said_n = v;
    c.kind = UK_FIRST;
    c.who = 0;
    c.w1 = f << 8;
    outcome_words (&c);
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
    unsigned char v = FV_REACH;
    const char* w = "";
    unsigned char k, away = 0, op = walk_op & ~IO_STEPS, lit = 0, unlit = 0, e, y, sel, in = 0, hd, nall = 0, dif = 0;
    const struct sitem* i0 = 0;
    char nb[sizeof CATALOGUE[0].name];
    static const char ops[] = { IO_GET, IO_DROP, IO_FILL, IO_HIDE, IO_SHUT, IO_THROW, IO_DESCRIBE, IO_INSIDE, 0 };

    if (worded) {                               /* (Told once a line: not again as its command's asked again after a step) */
        return;
    }
    if (*name == '#' || !*name || (!strchr (ops, op) && (op < IO_HOLD || op > IO_DOUSE))) {
        return;
    }
    for (k = s->nitems, it = s->item; k; --k, ++it) {
        if (name_is (it->name, name)) {         /* (The one rule, the world's chooser's: main.c's) */
            if (!nall++) {
                i0 = it;                        /* (Those seen, alike or not: the world asks which only of two that
                                                **   differ, its which: §5.1's disagreement 5, 2026-11-07) */
            } else if (it->qty != i0->qty || (it->hid & 4) != (i0->hid & 4) || strcmp (it->name, i0->name)) {
                dif = 1;
            }
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
    sel = op == IO_HOLD && cmd.other.word[0] && !strcmp (name, cmd.other.word) ? cmd.other.sel | cmd.other.n << 4 :
        cmd.obj.sel | cmd.obj.n << 4;          /* (A blow's weapon taken in hand, "with the other sword": its own, §5.1's 9) */
    if (op == IO_GET) {                         /* (Taken up: of what it doesn't carry, as ever) */
        if (dif && !sel && !ndes) {             /* (Several seen that differ: the world's which to ask; alike, the one
                                                **   it takes reached for) */
            first20 (FS_THEM, 0, 0, 0); /* (Several in reach, none chosen, nor by its letter: which, perhaps, the
                                                **   world's to ask; true whatever it answers, at once) */
            goto told;
        }
        if (!away) {
            if (s->dark) {
                return;
            }
            v = FV_LOOK_FOR;
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
    kn = nb;
    hub_mem (UI_BANK, &k, &UIREC->ixn, 1);
    nix = k > NIX ? NIX : k;
    from_ui (ix, UIREC->ix, nix * sizeof (struct cix));
    hub_mem (UI_BANK, &k, &UIREC->ixk, 1);
    nck = k > NIXK ? NIXK : k;
    from_ui (ck, UIREC->ixks, nck * sizeof (struct cixk));
    if (op == IO_TAKE) {                        /* (From the other: in it, if it's carried) */
        if ((y = des_e (cmd.other.word)) == 0xFF && (y = ix_pick (cmd.other.word, cmd.other.sel | cmd.other.n << 4, 0, 0)) ==
            0xFF) {
            for (k = s->nitems, it = s->item; k && (!name_is (it->name, cmd.other.word) || it->x - s->x + 1 > 2 ||
                it->y - s->y + 1 > 2); --k, ++it) {
            }
            first20 (FS_THE, k ? FV_REACH : s->dark ? FV_FEEL_FOR : FV_LOOK_FOR, 0, k ? name : cmd.other.word);
            goto told;                          /* (Not carried: lying in reach, the thing reached for, in it or on it; else
                                                **   it looked for) */
        }
        in = y + 1;
    }
    for (hd = 0; hd < ndes && strcmp (ldes[hd].word, name); ++hd) {
    }                                           /* (A handle's, a letter's: that thing, carried or not, its
                                                **   first words; none other of its name's, 2026-11-07) */
    if ((e = des_e (name)) == 0xFF && hd >= ndes && (e = ix_pick (name, sel, op, in)) != 0xFF && (op == IO_DROP || (op >= IO_HOLD &&
        op <= IO_DOUSE && op != IO_TAKE && op != IO_DOFF)) && ((y = sel & 15) <= SL_ANY || y == SL_NTH || y == SL_OTHER ||
        (y > SL_OTHER && op < IO_LIGHT)) && !(op == IO_LIGHT && (ix[e].flags & CX_LIT)) && (!cmd.other.word[0] ||
        ((op == IO_PUT || op == IO_STOW) && strcmp (cmd.other.word, name)))) {
        bind (name, e);                         /* (Chosen here, the first words', and made final: the world's asked of
                                                **   that one, COMMANDS-CRITIQUE.md's §5.1 step C, 2026-11-07; a state
                                                **   said too, "the spent torch", its flags the chooser's, s_ix_make's,
                                                **   but to light or put out: the world's light ()
                                                **   chooses by the name, not pick_, "light a new
                                                **   torch" its old one let fall by it;
                                                **   another named, "the torch in the sconce", "the sack in the sack",
                                                **   the world's, but a put's of another name) */
    }
    if (op == IO_DESCRIBE || op == IO_INSIDE) { /* (Looked at, asked of it: its quality, its weight ...; way.c's.  In the
                                                **   dark, the answer's own) */
        if (s->dark) {
            return;
        }
        v = e != 0xFF || away ? FV_LOOK_AT : FV_LOOK_FOR;
        goto say;
    }
    if (e == 0xFF) {
        if (op != IO_DROP && op != IO_HIDE && op != IO_THROW && op != IO_TAKE && away) {
            if (away == 1) {                    /* (Out of reach only: walked to, the world's IR_NOT_HERE, told now) */
                walk_says (name);
                placing = 0;
                said_ahead = 1;
                return;
            }
            goto say;                           /* (Lying within reach) */
        }
        v = s->dark ? FV_FEEL_FOR : FV_LOOK_FOR;    /* (None it carries, nor sees in reach: looked for, then the world's why) */
        w = sel_word (sel);
        goto say;
    }
    name = kname (e);
    w = apart_of (e, lit, unlit);
    y = 0xFF;
    if (op == IO_STOW || op == IO_PUT) {        /* (Put in the other, the pack: in it already, the world's words, nothing
                                                **   reached for) */
        y = op == IO_STOW ? 0xFF : (y = des_e (cmd.other.word)) != 0xFF ? y : ix_pick (cmd.other.word, cmd.other.sel |
            cmd.other.n << 4, 0, 0);    /* (Stowed: where, the world's to choose, a sheath, a pouch, the pack, or
                                                **   ask, the designators' step 7: the thing's own first words) */
        if (y != 0xFF && ix[e].up == y + 1) {
            v = s->dark ? FV_FEEL_FOR : FV_LOOK_FOR;    /* (In it already: looked for, and found there) */
            goto say;
        }
    }
    if ((k = top (e)) != e && !(ix[k].flags & CX_WORN && !(ix[k].flags & CX_BACK))) {   /* (In something: the pack worn
                                                **   first, or what it's in; on the belt or in what's on it, worn,
                                                **   at hand: the thing's own) */
        first20 (ix[k].flags & CX_BACK ? FS_YOUR : FS_INTO, 0, 0, kname (k));
        goto told;
    }
    if (op == IO_DROP) {
        if (cmd.n || cmd.obj.sel == SL_ALL || (cmd.flags & CF_HALF)) {
            return;                             /* (By the count: the world's words) */
        }
        first20 (FS_THE, FV_PUT_DOWN, w, name);
        t_flush ();
        worded = 4;                             /* (All of it said: not "Put down." again) */
        return;
    }
    if (op == IO_STOW || op == IO_PUT) {        /* (Put in the other: the pack worn first, or the other in reach) */
        if (y != 0xFF && (ix[k = top (y)].flags & CX_BACK)) {
            first20 (FS_YOUR, 0, 0, kname (k));
            goto told;
        }
        if (y != 0xFF && !(ix[y].flags & CX_HELD)) {
            first20 (FS_THE, FV_REACH, 0, kname (y));
            goto told;
        }
    }
    if (ix[e].flags & CX_HELD) {                /* (In hand, nothing to reach for: the act's own start, true whatever comes
                                                **   of it, the answer some seconds off: a light raised to be lit or put
                                                **   out, the rest lifted; 2026-11-06) */
        v = op == IO_LIGHT || op == IO_DOUSE ? FV_RAISE : FV_LIFT;
    }
say:
    first20 (FS_THE, v, w, name);
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

#pragma code-name (push, "OV14CODE")
#pragma rodata-name (push, "OV14RODATA")
#pragma local-strings (push, on)
/* A place said where a thing's other would be ("take the torch from the floor", "put it on the ground": the user's
** playtest, 2026-11-06, "You see no floor here"): where things lie about, not a thing: taken up, put down, of those lying
** there (the world's chooser leaves out those carried).  Overlay 14's since 2026-11-06 (overlay 20's room) */
void worn_said (void);                          /* (worn.c's: a place on the body, "on my left hand", 2026-11-10) */
void o_place_said (void)
{
    static const char* const places[] = { "floor", "ground", "bottom", "here" };
    const struct sight* s;
    unsigned char k;

    worn_said ();
    if (cmd.other.kind == OB_STAIRS && cmd.prep == PR_ON && (cmd.action == VB_PUT_OBJECT || cmd.action == VB_DROP_OBJECT)) {
        cmd.action = VB_DROP_OBJECT;            /* (On the stairs: placed there, put down on its own square, the user's,
                                                **   2026-11-17; its stairway elsewhere, out of reach: told, the command
                                                **   left be, CF_ASKED's, resolve's) */
        s = sight_get ();
        for (k = 0; k < s->nopen; ++k) {
            if (s->open[k].rel == 0xFF && (s->open[k].kind == O_STAIRS_UP || s->open[k].kind == O_STAIRS_DOWN ||
                s->open[k].kind == O_WAY_OUT)) {
                cmd.other.kind = 0;
                cmd.prep = PR_NONE;
                return;
            }
        }
        strcpy (lb, "The stairway");        /* (In the main RAM: the refusals' overlay's to read) */
        say_no (A_FAR, lb);
        cmd.flags |= CF_ASKED;
        return;
    }
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
#pragma local-strings (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)
