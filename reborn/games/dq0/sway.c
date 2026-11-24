/*
** sway.c - (overlay 15; overlay 6's till 2026-10-29, its room) the character's ways with the denizens, in words: what the denizens' task answered (the hub's
** sway, copied into the main RAM: main.c's deal) to words that would sway one (talk, persuade, deceive, threaten,
** calm, befriend: the referee's grade, its attitude before and after), a request to an ally, a name given, a look
** at how one seems, a blow, a gift (the commands listed: help.c's)
*/

#pragma code-name ("OV15CODE")
#pragma rodata-name ("OV15RODATA")

#include <stdlib.h>
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
    if (k < NACTORS) {
        walk_says ((const char*) (name_is (nm, cmd.obj.word) ? W_SEEN + k : W_SWAY));   /* ("You look at the giant
                                                **   rat.": the look at it's first words at once, the minds' answer to
                                                **   come, 2026-11-17, by its name as it's seen, the user's, 2026-11-18,
                                                **   where the word truly names it (not "rations" by its "rat": the word
                                                **   typed then); not of a name given that may be no denizen's,
                                                **   "examine the moon") */
        return 1;
    }
    return names_n != 0;
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

/* A walk a way (hmove.c's do_go: cmd.dir its true heading): its set-off words at once, the world's own (its V_SETOFF's,
** said.txt's SELF.setoff, "You set off west.", by the north the character believes in; not again as the world's come,
** worded 2), where the senses' record s shows the way open: the room's floor that way, a doorway or an open door there,
** a passage's way on, level enough for a step (dq0w's STEP_MAX, STAIR_MAX on steps); not in the dark (the walk's own
** words, felt), nor with a light left burning at its feet (taken up first), nor toward a part of the room higher or
** lower that way (a ledge: "The floor rises 10 feet at a ledge ahead ...", perhaps the first square), nor into a
** denizen seen there ("A giant rat is in the way.").  Else the world's alone, as ever (a way refused, "You can't go that
** way.") */
#pragma static-locals (push, off)               /* (Its words' event on the C stack: the words' overlay reads it) */
static void __fastcall__ setoff17 (const struct sight* s)
{
    struct msg c;
    unsigned char h = cmd.dir, k = 0, i, r = (h - pc_face) & 7;
    unsigned x = s->x + head_x[h & 7], y = s->y + head_y[h & 7];
    signed char z;

    if (h > H_NW || s->dark) {
        return;
    }
    if (s->where == W_ROOM) {
        k = x - s->rx < s->rw && y - s->ry < s->rh;
        for (i = 0; i < s->nzones && k; ++i) {
            z = s->zone[i].dz;                  /* (By steps or a slope too: the straight way may meet its ledge; its
                                                **   middle's way, an eighth either side of it) */
            k = ((s->zone[i].rel - r + 1) & 7) > 2 || (z <= 2 && z >= -2);
        }
    } else {
        for (i = 0; i < s->nways && !k; ++i) {
            z = s->way[i].dz;
            k = (s->way[i].rel & 7) == r && z <= 4 && z >= -4 && (s->way[i].steps == 5 || (z <= 2 && z >= -2));
        }                                       /* (5: dq0w/world.h's T_STEPS, words.c's T_STEPS_) */
    }
    for (i = 0; i < s->ndoors && !k; ++i) {
        k = s->door[i].x == x && s->door[i].y == y && (s->door[i].state == DOORWAY || (s->door[i].state &
            (PS_OPEN | PS_WRECKED)));
    }
    for (i = 0; i < s->ndens && k; ++i) {
        k = s->den[i].x != x || s->den[i].y != y;
    }
    for (i = 0; i < s->nitems && k; ++i) {      /* (A light left burning at its feet: taken up as the walk begins, the
                                                **   hands' rule, its words first: the world's alone) */
        k = s->item[i].x != s->x || s->item[i].y != s->y || !(s->item[i].hid & 4);
    }
    if (!k) {
        return;
    }
    hub_mem (0, &k, &HUB->pc_skew, 1);          /* (By the north it believes in: the hub's, the senses' in the window) */
    said_s[2] = head_name[(h + k) & 7];
    c.kind = UK_SELF;
    c.who = 0;
    c.b = 0;
    c.w1 = MY_SETOFF << 8;
    outcome_words (&c);
    t_flush ();
    worded = 2;
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
    if (n == W_SETOFF) {                        /* (A walk a way, its heading the command's: its set-off words at once,
                                                **   the world's own, where the senses show the way open; 2026-11-17) */
        setoff17 (s);
        return;
    }
    if (n >= W_PEER) {
        first17 (n == W_PEER ? FS_PEER : FS_LISTEN, 0, 0, 0);
        t_flush ();
        worded = 3;
        return;
    }
    if (n == W_SWAY) {
        v = s->dark ? FV_FEEL_FOR : cmd.action == VB_OBSERVE_OBJECT || cmd.action == VB_EXAMINE_OBJECT ? FV_LOOK_AT :
            FV_LOOK_FOR;                        /* (A denizen looked at: "You look at the giant rat.", 2026-11-17) */
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

/* An attack's first words at once, its target beside it (the user's, 2026-11-17: "You close with the giant rat."), or a
** shot's (f FS_AIM: "You take aim at the giant rat."): actor d - 1 by its name (the one the command found by it), or
** "something" in the dark, as its blows are; said.txt's FIRST.close and FIRST.aim, their words the pieces @closes and
** @aims, a denizen's at the character the same (DEN.closes, "The bandit closes with you.") */
#pragma static-locals (push, off)               /* (On the C stack: its name copied from the hub, with the hub in the
                                                **   window) */
static void __fastcall__ close_word (unsigned char d, unsigned char f, unsigned char v)
{
    char nm[sizeof HUB->actor[0].name + 5];

    nm[4] = nm[sizeof nm - 1] = 0;
    hub_mem (0, nm + 4, HUB->actor[(d - 1) & 7].name, sizeof HUB->actor[0].name);
    if (sight_get ()->dark) {
        strcpy (nm, "something");
    } else if (nm[4] < 'A' || nm[4] > 'Z') {
        memcpy (nm, "the ", 4);                 /* (A name given its own, "Grub": as it is) */
    } else {
        memmove (nm, nm + 4, sizeof nm - 4);
    }
    said_s[0] = lb + 340;                       /* (The one closed with, first17's copy: the row's {obj}) */
    first17 (f, v, 0, nm);
    said_s[0] = 0;
    t_flush ();
}
#pragma static-locals (pop)

void __fastcall__ o_walk_says (const char* name)
{
    unsigned char k = cmd.obj.kind;

    if (cmd.aux == IO_PLACE && !name) {         /* (A thing put in a place, walked there first: the place's words told
                                                **   already, o_walk_ahead's, P5's, 2026-11-18) */
        placing = 1;
        return;
    }
    if (name && (unsigned) name < W_MAX) {
        if ((unsigned) name < W_CLOSE) {
            first_word ((unsigned char) name);
            return;
        }
        if (worded) {
            return;                             /* (An attack's said already: its target looked for again as it's
                                                **   carried out, the wield's first, then the blow's) */
        }
        k = (unsigned char) name - W_CLOSE;
        if (k >= W_SEEN - W_CLOSE) {            /* (A denizen looked at, asked of: by its seen name, FIRST.seen; in the
                                                **   dark, felt for by the word typed, first_word's) */
            if (sight_get ()->dark) {
                first_word (W_SWAY);
                return;
            }
            worded = 1;
            close_word ((k & 7) + 1, FS_SEEN, cmd.action == VB_OBSERVE_OBJECT || cmd.action == VB_EXAMINE_OBJECT ?
                FV_LOOK_AT : FV_LOOK_FOR);
            return;
        }
        worded = 1;
        close_word (k & 0x0F, k & FT_CLOSE ? FS_OVER : k & 0x10 ? FS_AIM : FS_CLOSE, 0);  /* (Beside it: closed with; a
                                                **   shot's: aimed at; out of reach: walked over to first, by its name
                                                **   as it's seen, "the small orc", not the word typed) */
        if (k & FT_CLOSE) {
            placing = 1;
        }
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
static unsigned char exact;                     /* (ix_pick's: a kind of the very name only, an empty's) */

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
        cm[e] = exact ? !strcmp (ck[e].name, w) : name_is (ck[e].name, w);   /* (The word asked of each kind once: the one
                                                **   rule, the world's, main.c's) */
        if (op == IO_EMPTY && cm[e] && !strncmp (ck[e].name, w, j = strlen (w)) && ck[e].name[j] >= 'a') {
            cm[e] = 0;                          /* (Emptied, poured: "water" not the waterskin, a word's start; the
                                                **   world's the liquid's, wpour.c's liquid_named, H11, 2026-11-19) */
        }
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
    if (n == 1) {                               /* (The first: the least rank under 13, the first of it, in one pass, not
                                                **   thirteen: room8, 2026-11-24, D17's) */
        for (j = 0xFF, r = 13, e = 0; e < nix; ++e) {
            if (rk[e] < r) {
                r = rk[e];
                j = e;
            }
        }
        return j;
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

/* A count's first words at once, before the world splits a lot (the user's, 2026-11-17: "You count out a farthing."):
** carried entry e counted out by the command's count, half or all (said.txt's FIRST.count: n1 the count, ten and more
** 11, w4 its figures; n2 1 the kind's name a plural, "pennies" one "a penny", | 2 half, | 4 all), the world's words
** after it, what's put down */
static void __fastcall__ count20 (unsigned char e)
{
    struct msg c;
    unsigned n = cmd.n, t = 0;
    unsigned char q[NIX], k;

    from_ui (q, UIREC->ixq, nix);               /* (How many each is: the world's, ix_make's) */
    for (k = 0; k < nix; ++k) {
        if (ix[k].kind == ix[e].kind) {
            t += q[k];
        }
    }
    k = FS_COUNT;
    if (t < n) {                                /* (Fewer carried than counted, its own view: not counted out, the
                                                **   user's, 2026-11-18; "You have only one.", the world's "You drop it."
                                                **   after it) */
        n = t;
        k = FS_ONLY;
    }
    said_s[3] = utoa (n, lb + 300, 10);
    said_s[2] = strcpy (lb + 340, kname (e));
    c.kind = UK_FIRST;
    c.who = 0;
    c.b = (ix[e].kind < nck && (ck[ix[e].kind].flags & K_PLURAL) ? 1 : 0) | (cmd.flags & CF_HALF ? 2 : cmd.obj.sel == SL_ALL ?
        4 : 0);
    c.w1 = k << 8 | (n > 11 ? 11 : n);
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
    static const char ops[] = { IO_GET, IO_DROP, IO_FILL, IO_HIDE, IO_SHUT, IO_THROW, IO_DESCRIBE, IO_INSIDE, IO_EMPTY,
        IO_PLACE, 0 };

    if (worded) {                               /* (Told once a line: not again as its command's asked again after a step) */
        return;
    }
    if (*name == '#' || !*name || (!strchr (ops, op) && (op < IO_HOLD || op > IO_DOUSE))) {
        return;
    }
    if (op == IO_PLACE && ((unsigned char) (cmd.other.x - s->x + 1) > 2 || (unsigned char) (cmd.other.y - s->y + 1) > 2)) {
        msg (cmd.other.n == PW_LEDGE ? "You go over to the ledge." : cmd.other.n == PW_PASSAGE ?
            "You go over to the passage." : "You set off across the room.");
        goto told;                              /* (Put in a place out of reach: the place's words, not the thing's, its
                                                **   walk told now, P5's; in reach, and a thing emptied, P4's, the
                                                **   thing's own, 2026-11-18) */
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
    if (op == IO_EMPTY && e != 0xFF && cmd.other.word[0] && ix_pick (cmd.other.word, cmd.other.sel | cmd.other.n << 4, 0, 0) ==
        e) {
        exact = 1;                              /* ("Pour the oil into the flask of oil": the oil, not the flask, the
                                                **   world's too: wpour.c's liquid_named, H14, 2026-11-19) */
        e = ix_pick (name, sel, op, in);
        exact = 0;
    }
    if (op == IO_DESCRIBE || op == IO_INSIDE) { /* (Looked at, asked of it: its quality, its weight ...; way.c's.  In the
                                                **   dark, the answer's own) */
        if (s->dark) {
            return;
        }
        v = e != 0xFF || away ? FV_LOOK_AT : FV_LOOK_FOR;
        if (v == FV_LOOK_FOR && cmd.action == VB_OBSERVE_OBJECT) {
            return;                             /* ("Look at the drawer", a part of a thing, "the moon": the answer's
                                                **   own words, not looked for, way.c's, 2026-11-17) */
        }
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
            count20 (e);                        /* (By the count: counted out, at once, the world's words after it as
                                                **   ever, what's put down) */
            goto told;
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
/* A place in the room the character's in (HOUSE.md's going places), by its facing alone: the centre (the middle); a
** side, the far one (the other side, the far end, the wall ahead) unless it says which (near, left, right, the way it
** lies); a corner ("the far left corner"; unsaid, the nearest).  Thing o's (cmd.obj's, a move's: place_go's; cmd.other's,
** a put's).  OUT: 2, its square in tx, ty (the nearest the way reaches, a cave's rock about it, the world's walk's); 1, not
** in a room (told); 3, in the dark (a move straight on, place_go's); 0, not such a place.  (Part 7's place_go's till
** 2026-11-18: P5's put there too) */
static unsigned char __fastcall__ place_sq (const struct cthing* o)
{
    signed char a, b, fa, ra;
    unsigned char k, l, q, fy;
    unsigned x0, x1, y0, y1, px, py, f0, f1, r0, r1, fc, rc;
    const struct sight* s;
    const char* w = cl;                         /* (Its words: the line's, "the near side" the parser's how) */

    k = o->kind;
    if (k == OB_ROOM) {                         /* ("The centre of the room": the place, not the room) */
        k = strstr (w, "corner") ? OB_CORNER : strstr (w, "cent") || strstr (w, "middle") ? OB_CENTRE : OB_SIDE;   /* (Its
                                                **   far side: "the far end of the room", "cross the room") */
    }
    if (k != OB_CENTRE && k != OB_SIDE && k != OB_CORNER && k != OB_WALL) {
        return 0;
    }
    s = sight_get ();
    if (s->where != W_ROOM || s->dark) {
        if (k == OB_CORNER || k == OB_WALL) {
            return 0;                           /* (A passage's corner, a wall: as ever) */
        }
        if (s->dark) {
            return 3;
        }
        msg ("You aren't in a room.");
        return 1;
    }
    a = b = 2;
    if (k == OB_CENTRE) {
        a = b = 0;
    }
    a = strstr (w, "far") || strstr (w, "other") || strstr (w, "opposite") ? 1 : strstr (w, "near") ? -1 : a;
    b = strstr (w, "left") ? -1 : strstr (w, "right") ? 1 : b;
    l = o->lies != 0xFF ? o->lies : cmd.dir <= H_NW && a == 2 && b == 2 ? cmd.dir : 0xFF;
    if (l != 0xFF) {
        l = l & CQ_REL ? l & 7 : (l - pc_face) & 7;    /* (As it faces) */
        a = l == 7 || l <= 1 ? 1 : l >= 3 && l <= 5 ? -1 : a;
        b = l >= 1 && l <= 3 ? 1 : l >= 5 ? -1 : b;
    }
    if (k != OB_CORNER && a == 2 && b == 2) {
        a = 1;                                  /* (A side, a wall: the far one) */
    }
    x0 = s->rx;
    x1 = s->rx + s->rw - 1;
    y0 = s->ry;
    y1 = s->ry + s->rh - 1;
    px = s->x;
    py = s->y;
    q = ((pc_face + 1) & 7) >> 1;               /* (The nearest of the four it faces: its ahead and its right) */
    fy = !(q & 1);                              /* (Ahead along y: facing north or south) */
    fa = q == 1 || q == 2 ? 1 : -1;             /* (Ahead the way x or y grows) */
    ra = q <= 1 ? 1 : -1;                       /* (Its right the way the other grows) */
    f0 = fy ? y0 : x0;
    f1 = fy ? y1 : x1;
    r0 = fy ? x0 : y0;
    r1 = fy ? x1 : y1;
    fc = fy ? py : px;
    rc = fy ? px : py;
    if (k == OB_CORNER) {                       /* (Unsaid: the nearer end each way) */
        if (a == 2) {
            a = (fa > 0 ? f1 - fc <= fc - f0 : fc - f0 <= f1 - fc) ? 1 : -1;
        }
        if (b == 2) {
            b = (ra > 0 ? r1 - rc <= rc - r0 : rc - r0 <= r1 - rc) ? 1 : -1;
        }
    }
    fc = a == 2 ? fc : !a ? f0 + (f1 - f0) / 2 : a * fa > 0 ? f1 : f0;   /* (Not (f0 + f1) / 2: the frame's squares near */
    rc = b == 2 ? rc : !b ? r0 + (r1 - r0) / 2 : b * ra > 0 ? r1 : r0;   /*   $8000, a sum past 16 bits) */
    tx = fy ? rc : fc;
    ty = fy ? fc : rc;
    return 2;
}

/* A thing put on a ledge, in a corner, the centre, a side or a passage (P5, the user's, 2026-11-18: walked there and
** put down on that square): its square found (a corner, the centre, a side: place_sq's; a ledge, a passage: pick's,
** which asked of several), the command made a put by the things' IO_PLACE (the put's handle: hthings.c's s_handle sends
** cmd.other's square and its words' number, PW_*, the world's), the place no other thing (no refusal of it, no thing
** looked for); not found, told, the command left be (CF_ASKED's, resolve's).  1 if it was such a put */
static unsigned char place_put (void)
{
    unsigned char k = cmd.other.kind, r, n;
    const char* w = cl;

    if ((cmd.action != VB_PUT_OBJECT && cmd.action != VB_DROP_OBJECT) || cmd.aux == IO_PLACE ||
        (k != OB_LEDGE && k != OB_PASSAGE && k != OB_CORNER && k != OB_CENTRE && k != OB_SIDE &&
        (k != OB_ROOM || !(strstr (w, "corner") || strstr (w, "cent") || strstr (w, "middle") || strstr (w, "side") ||
        strstr (w, "end"))))) {
        return 0;
    }
    if (k == OB_LEDGE || k == OB_PASSAGE) {
        r = pick (k | PICK_TELL, cmd.other.lies) == 1;
        n = k == OB_LEDGE ? PW_LEDGE : PW_PASSAGE;
    } else {
        if ((r = place_sq (&cmd.other)) == 3) {
            msg ("You can't see where, in the dark.");
        }
        r = r == 2;
        n = strstr (w, "corner") ? PW_CORNER : strstr (w, "cent") || strstr (w, "middle") ? PW_MIDDLE :
            strstr (w, "left") ? PW_LEFT : strstr (w, "right") ? PW_RIGHT : strstr (w, "near") ? PW_NEAR : PW_FAR;
    }
    if (!r) {
        cmd.flags |= CF_ASKED;                  /* (None there, which asked, in the dark, not in a room: told) */
        return 1;
    }
    cmd.action = VB_PUT_OBJECT;
    cmd.aux = IO_PLACE;
    cmd.prep = PR_NONE;
    cmd.other.kind = cmd.other.sel = 0;
    cmd.other.word[0] = 0;
    cmd.other.lies = 0xFF;
    cmd.other.x = tx;
    cmd.other.y = ty;
    cmd.other.n = n;
    return 1;
}

void worn_said (void);                          /* (worn.c's: a place on the body, "on my left hand", 2026-11-10) */
void o_place_said (void)
{
    static const char* const places[] = { "floor", "ground", "bottom", "here" };
    const struct sight* s;
    unsigned char k;

    if (tx == PQ_NONE) {                        /* (place_go's, a move's: the place cmd.obj names, its square) */
        if ((k = place_sq (&cmd.obj)) != 2) {
            tx = k == 1 ? PQ_TOLD : k == 3 ? PQ_DARK : PQ_NONE;
        }
        return;
    }
    worn_said ();
    if (place_put ()) {
        return;                                 /* (A thing put in a place in the room, on a ledge, in a passage: P5's) */
    }
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
