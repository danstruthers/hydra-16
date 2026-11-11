/*
** hthings.c - the things' verbs (COMMANDS-CRITIQUE.md's §5.2, step L, stage 3: the command handlers by meaning,
** cmds.h): a thing taken up, put down, handled (held, stowed, put in or on another, taken from it, worn, taken off,
** lit, put out), hidden, eaten or drunk (part 2's, the commands' own: things_do); and their timed works, a thing
** filled, tied, looked into, one that closes worked (a lid, a tie, a lock: the door verbs' first) and a hide begun
** (overlay 1's: things_work, words.c's in overlay 16 till stage 3, its room).  Each verb's work as it was in main.c's
** s_command and words.c's more; every one the one path for things (main.c's things with IO_GO: §5.2's step H).
*/

#include <string.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "cmds.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's in it, an overlay's in it) */

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* After a thing's handled (taken up, put down ...): its light changed (lit, put out, a torch put down), the world's
** look about in the new light; else the time it took (t tenths), quietly */
void __fastcall__ s_after (unsigned char was, unsigned t, unsigned char op)
{
    t_flush ();                                 /* (Its words out before the world's asked again: the user's D17) */
    to_hand ();
    if (op == IO_LIGHT || op == IO_DOUSE || HUB->pc_light != was) {
        intend_a = op == IO_LIGHT || HUB->pc_light > was;
        intend2 (I_LIGHT, op == IO_LIGHT ? 0 : t ? t : 1, 0);   /* (Its own time, the handling's; lit, the lighting's) */
    } else {
        intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
    }
}

/* A thing handled (held, stowed, put in or on another, taken from it, worn, taken off, lit, put out), by its name
** (and the other's: y, on 1 for on it): the things' words of it said */
void __fastcall__ s_handle (unsigned char op, const char* x, const char* y, unsigned char on)
{
    unsigned char was = HUB->pc_light;

    strcpy (HUB->ireq.into, y);
    HUB->ireq.to = on | shand;                  /* (A hold's: the hand said, slice 4.1b's) */
    if (things (op | IO_STEPS | IO_GO, x) == IR_GONE) {
        return;                                 /* (Out of reach: walked over to it first, then again; stowed: where,
                                                **   asked) */
    }
    msg (tt ());
    if (HUB->ireq.result == IR_OK) {
        after (was, HUB->ireq.x, op);
    }
}

/* A thing put in another, or let fall in it ("drop the dagger into the well", "down the shaft", 2026-11-06; "throw
** the dagger into the well") */
void s_put_in (void)
{
    handle (IO_PUT, cmd.obj.word, cmd.other.word[0] ? cmd.other.word : "shaft", 0);
}

/* H_TAKE: a thing taken up, or taken from another */
static void do_take (void)
{
    if (cmd.prep == PR_FROM) {
        handle (IO_TAKE, cmd.obj.word, cmd.other.word, 0);
    } else if (!cmd.obj.word[0]) {
        unknown (U_WHAT);
    } else {
        take (cmd.obj.word);
    }
}

/* H_DROP: a thing put down */
static void do_drop (void)
{
    unsigned char d, pa;

    if (cmd.other.word[0] || cmd.prep == PR_IN) {
        put_in ();
        return;
    }
    if (!cmd.obj.word[0]) {
        unknown (U_WHAT);
    } else if (d = HUB->pc_light, (pa = things (IO_DROP | IO_STEPS | IO_GO, cmd.obj.word)) == IR_OK) {
        if (*tt ()) {
            msg (lb);                           /* (By the count: "You put down two sacks.", the world's) */
        } else if (worded != 4) {               /* ("You put down the torch." said already, its first words: sway.c's) */
            tell (T_PUT_DOWN);
        }
        after (d, 1, IO_DROP);                  /* (No time: let fall, HOUSE.md's reach; the world's reach owed
                                                **   for one got out first) */
    } else if (pa == IR_NO) {
        msg (tt ());                            /* (Why not: in a thing closed) */
    } else if (pa != IR_GONE) {                 /* (The one path for things: several, which asked) */
        say_no (A_HAVE_NO, cmd.obj.word);
    }
}

void __fastcall__ things_use (unsigned char h);    /* (Part 29's, below: use, push, pull, touch, taste) */

/* The things' verbs, by their handler's number (H_TAKE ... H_TASTE) */
void __fastcall__ s_things_do (unsigned h)
{
    if ((unsigned char) h >= H_USE && (unsigned char) h <= H_TASTE) {
        things_use ((unsigned char) h);         /* (The verbs plan's B4: part 29's, part 2 full) */
        return;
    }
    switch ((unsigned char) h) {
    case H_TAKE:
        do_take ();
        break;
    case H_DROP:
        do_drop ();
        break;
    case H_HIDE:                                /* (P_CONCEAL: a thing; else oneself: HOUSE.md's; main.c's, part 18) */
        hide (cmd.obj.word);
        break;
    case H_EAT:                                 /* (Eating, drinking: part 16's, slice 4.9's) */
        feed (cmd.action == VB_EAT_OBJECT);
        break;
    default:                                    /* (H_HANDLE: a thing handled, the things' request the verb's, IO_*;
                                                **   "wear"'s IO_DON, worn only: DN_WORN, 2026-11-09) */
        if (cmd.action == VB_HOLD_OBJECT && grip_go (GM_HOLD | GT_TRY)) {
            break;                              /* ("Hold the goblin": a denizen beside, held, a grapple's) */
        }
        handle (cmd.aux == IO_DON_WORN ? IO_DON : cmd.aux == IO_RIP && strstr (cl, "wash") ? IO_WASH : cmd.aux,
            cmd.obj.word,                       /* ("Wash" a cloth washed: rip's verb, its request the word's) */ cmd.other.word[0] || cmd.prep != PR_IN ?
            cmd.other.word : "shaft", cmd.aux == IO_DON_WORN ? DN_WORN : cmd.prep == PR_ON);
    }                                           /* ("Put the dagger down the shaft", the hole: its kind said, no word) */
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The things' verbs the plan's batch B4 built (2026-11-10, DECISIONS.md's "The things' verbs: use, push, pull, touch,
** taste"): part 29's, part 2 full.  Its strings are its own: each said (msg, sayf, say_no copy it) before any other part's
** called, none kept across a call */
#pragma code-name (push, "SW29CODE")
#pragma rodata-name (push, "SW29RODATA")

/* The command carried out as verb v's, aux its request (0: the row's), the same things: its handler the dispatch table's
** (main.c's s_command's), its command kept as that verb's (a timed work's end, a walk over first: that verb again) */
static void __fastcall__ as_verb (unsigned char v, unsigned char aux)
{
    unsigned h;

    cmd.action = v;
    cmd.aux = aux;
    h = verb_row (v);
    if ((unsigned char) h >= H_SELF) {
        self_do (h);
    } else if ((unsigned char) h >= H_PLACES) {
        places_do (h);
    } else if ((unsigned char) h >= H_WORK) {
        things_work ((unsigned char) h);
    } else {
        things_do (h);
    }
}

/* H_USE ("use", "activate"): the theme's puzzle's first (theme.c's: its rules for the thing, or for the thing on another);
** none, the thing's kind says what using it is: a bandage tends (a wound, or the place said: "use the bandage on my
** arm"), food is eaten, a vial, a waterskin, an antidote, a remedy drunk, a light lit, a tinderbox lights the thing it's
** used on, a key or the lockpicks unlock what they're used on (none said, the door ahead), a rope or a grapple is tied
** (to what it's used on), a whetstone sharpens the blade it's used on (none said, the one in hand), a thing held in a
** hand (a weapon, a tool) is taken in hand; a thing used on a cloth one cuts it up for bandages (rip's) */
static void do_use (void)
{
    char key[8];
    unsigned char k, f;
    char* w = cmd.obj.word;

    if (!*w) {
        unknown (U_WHAT);
        return;
    }
    if (othings (IO_FIND, w, "", 0) != IR_OK) {
        say_no (A_HAVE_NO, w);
        return;
    }
    k = (unsigned char) othings_x;
    if (k != KI_BANDAGES && cmd.other.word[0] && othings (IO_FIND, cmd.other.word, "", 0) == IR_OK) {
        hub_mem (KIND_BANK, &f, &CATALOGUE[(unsigned char) othings_x].mat, 1);
        if (MT_TEXTILE (f)) {                   /* (A thing used on a cloth one, "use the knife on the cloak": a rip,
                                                **   the cloth cut up with it, the textiles' IO_RIP: wcloth.c's, which
                                                **   says what won't cut, by hand if it's no blade) */
            cmd.action = VB_RIP_OBJECT;
            cmd.aux = IO_RIP;
            handle (IO_RIP, cmd.other.word, w, 0);
            return;
        }
    }
    hub_mem (KIND_BANK, key, CATALOGUE[k].tool, sizeof key);
    hub_mem (KIND_BANK, &f, &CATALOGUE[k].flags, 1);
    if (k == KI_BANDAGES) {                     /* (Tend: its patient or its place the thing it's used on, "" the
                                                **   character's worst; the bandages' own batch to say more) */
        strcpy (w, cmd.other.word);
        cmd.other.word[0] = 0;
        as_verb (VB_TEND, 0);
    } else if (!strcmp (key, "food")) {
        as_verb (VB_EAT_OBJECT, 0);
    } else if (k == KI_WATERSKIN || k == KI_VIAL || k == KI_ANTIDOTE || k == KI_REMEDY || !strcmp (key, "water")) {
        if (k >= KI_ANTIDOTE) {
            strcpy (w, k == KI_ANTIDOTE ? "antidote" : "remedy");   /* (feed.c's remedies, by their words) */
        }
        as_verb (VB_DRINK, 0);
    } else if (f & K_LIGHT) {
        cmd.other.word[0] = 0;
        as_verb (VB_LIGHT_OBJECT, IO_LIGHT);
    } else if (k == KI_TINDERBOX || k == KI_KEY || k == KI_LOCKPICKS || k == KI_WHETSTONE) {
        if (cmd.other.word[0] || cmd.other.kind) {
            memcpy (&cmd.obj, &cmd.other, sizeof cmd.obj);   /* (The thing it's used on: lit, unlocked, sharpened) */
        } else if (k == KI_KEY || k == KI_LOCKPICKS) {
            strcpy (w, "door");                 /* (None said: the door there, or which, asked) */
            cmd.obj.kind = OB_DOOR;
            cmd.obj.lies = 0xFF;
        } else if (k == KI_WHETSTONE) {         /* (None said: the blade in hand, not the light) */
            things (IO_HANDS, nil);
            k = HUB->ireq.x;
            f = HUB->ireq.y;
            hub_mem (KIND_BANK, key, &CATALOGUE[k].flags, 1);
            if (*key & K_LIGHT) {
                k = f;
            }
            if (!k) {
                msg ("Sharpen what? (use the whetstone on the blade)");
                return;
            }
            hub_mem (KIND_BANK, w, CATALOGUE[k].name, sizeof CATALOGUE[0].name);
            k = KI_WHETSTONE;
        } else {
            *w = 0;                             /* (The light to hand) */
        }
        cmd.other.word[0] = 0;
        cmd.other.kind = 0;
        if (k == KI_WHETSTONE) {
            strcpy (cmd.other.word, "whetstone");
            as_verb (VB_FILL_OBJECT, IO_SHARP);
        } else {
            as_verb (k == KI_TINDERBOX ? VB_LIGHT_OBJECT : VB_UNLOCK_OBJECT, k == KI_TINDERBOX ? IO_LIGHT : 0);
        }
    } else if (k == KI_ROPE || k == KI_GRAPPLE) {
        as_verb (VB_TIE_OBJECT, 0);             /* (To the thing it's used on, or what's nearest that holds) */
    } else if (f & K_HAND) {
        as_verb (VB_EQUIP_OBJECT, IO_HOLD);     /* (A weapon, a tool: taken in hand, wield's) */
    } else if (cmd.other.word[0]) {
        sayf ("\nNothing comes of using the %s on the %s.", w, cmd.other.word);
    } else {
        sayf ("\nYou can't see how to use the %s.", w);
    }
}

/* H_PUSH, H_PULL: a door pushed open, pulled shut (a window's shutters alike); a portcullis pushed up, pulled down; a
** door's bar pushed home in its brackets, pulled out of them (PA_LIFT); a rope pulled, untied (from below, its grapple
** shaken loose: wrope2.c's); a heavy thing shoved a square, dragged a square and the character back a square (the
** world's IO_DRAG, wdrag.c's).  There are no levers */
static void __fastcall__ do_shove (unsigned char pull)
{
    char* w = cmd.obj.word;
    unsigned char k = cmd.obj.kind, r;
    unsigned t;

    if (strstr (w, "bar")) {
        portal (pull ? PA_LIFT : PA_BAR);       /* (A door's bar: its door the kind, the resolver's) */
    } else if (k == OB_DOOR || k == OB_WINDOW || k == OB_PORTC) {
        portal (k == OB_PORTC ? (pull ? PA_LOWER : PA_RAISE) : pull ? PA_CLOSE : PA_OPEN);
    } else if (!*w) {
        unknown (U_WHAT);
    } else if (strstr (w, "rope")) {
        if (pull) {
            as_verb (VB_UNTIE_OBJECT, 0);
        } else {
            unknown (U_CANT);
        }
    } else if (HUB->ireq.to = pull, (r = things (IO_DRAG | IO_GO, w)) == IR_NONE) {
        say_no (A_SEE_NO, w);
    } else if (r != IR_GONE) {                  /* (Out of reach: walked over to it first, then again) */
        k = HUB->ireq.y;
        t = HUB->ireq.x;
        msg (tt ());
        if (r == IR_OK) {
            if (pull && k != 0xFF) {
                intend2 (I_STEP, I_STEP_W1 (k, 1), 1);  /* (Back a square, the way it came, its facing kept) */
            } else {
                intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
            }
        }
    }
}

/* H_TOUCH ("touch", "feel"): in the dark, nothing said or a wall, the floor: the felt words, a feel about (H_FEEL's);
** else the thing examined, by touch in the dark (its make, its state: a close look's, as the hands find them) */
static void do_touch (void)
{
    unsigned char k = cmd.obj.kind;

    if ((!cmd.obj.word[0] || k == OB_WALL || k == OB_ROOM) && sight_get ()->dark) {
        as_verb (VB_OBSERVE_LOCALE, 0);
    } else if (!cmd.obj.word[0] && !k) {
        unknown (U_WHAT);
    } else {
        as_verb (VB_EXAMINE_OBJECT, 0);
    }
}

/* H_TASTE: a sip, not a dose (nothing used up, no remedy taken: DECISIONS.md's): an antidote or a remedy named by a lore
** test (the referee's P_LORE), water fresh or still (its wear: from still water), food still good (none goes off in this
** game); the water it stands in, still or running.  Still water's sip the flux's risk, a sip's (the user's, 2026-11-11):
** the referee told it's drunk foul, no pints (RS_TIME's drink 0x80: a tenth of a drink's chance, afflict.c's).  Three
** seconds */
static void do_taste (void)
{
    struct req* r;
    char key[8];
    unsigned char k, c, f;
    const char* s;

    if (!cmd.obj.word[0]) {
        unknown (U_WHAT);
        return;
    }
    HUB->ireq.to = DG_TASTE;
    if (HUB->pc_wet && !strcmp (cmd.obj.word, "water")) {
        k = c = KI_WATER;                       /* (The water it stands in: still, the flux's, feed.c's) */
        f = (unsigned char) (HUB->pc_wet - 1) < 0x7F;
    } else if (things (IO_DRAG, cmd.obj.word) == IR_OK) {
        k = HUB->ireq.x;
        c = HUB->ireq.y;
        f = HUB->ireq.qty;
    } else {
        say_no (A_HAVE_NO, cmd.obj.word);
        return;
    }
    hub_mem (KIND_BANK, key, &CATALOGUE[k].keeps, 1);
    if (!(*key & KF_LIQUID)) {
        c = 0;                                  /* (What's in a pack, a sack: not tasted, the vessel's own alone) */
    }
    f = c ? f : *key & KF_LIQUID ? 2 : f;       /* (An empty vessel: 2) */
    hub_mem (KIND_BANK, key, CATALOGUE[c ? c : k].tool, sizeof key);
    if (c == KI_ANTIDOTE || k == KI_ANTIDOTE || k == KI_REMEDY) {
        r = ask_b (RS_TEST, P_LORE, 0);         /* (Lore: what's in it known by its taste) */
        f = G_OK (RS_TEST_GRADE (*r));
        r->state = REQ_FREE;
        s = !f ? "Bitter, and strange: you can't tell what it's for." : k == KI_REMEDY ?
            "Sharp and herby: a physician's draught, a remedy against disease." :
            "Bitter with rue and wormwood: a theriac, an antidote against poisons and venoms.";
    } else if (c == KI_WATER || k == KI_WATER) {
        s = f ? "Flat and stale: still water, not fresh. It may carry the flux." : "Cold and clean: fresh water.";
        if (f) {
            hub_map (0);                        /* (A sip of it: feed.c's drink, foul, no pints) */
            r = &HUB->slot[SLOT_UI];
            RS_TIME_MINUTES (*r) = RS_TIME_BLEEDERS (*r) = 0;
            RS_TIME_FIGHTING (*r) = RS_TIME_DROWNED (*r) = 0;
            RS_TIME_MEAL (*r) = 0;
            RS_TIME_DRINK (*r) = 0x80;
            ask_b (RS_TIME, 0, 0)->state = REQ_FREE;
        }
    } else if (!strcmp (key, "food")) {
        s = "It tastes as it should: it's still good.";
    } else if (!c && f == 2) {
        s = "There's nothing in it to taste.";
    } else {
        s = "It tastes of nothing you'd care to eat.";
    }
    sayf ("\nYou taste the %s. %s", cmd.obj.word, s);   /* (Its answer names its thing: the sweeps') */
    intend2 (I_WAIT, 30, I_WAIT_W2 (0, 0, 1));
}

/* The verbs plan's B4's, by their handler's number (H_USE ... H_TASTE) */
void __fastcall__ s_things_use (unsigned char h)
{
    if (h == H_USE) {
        do_use ();
    } else if (h <= H_PULL) {
        do_shove (h == H_PULL);
    } else if (h == H_TOUCH) {
        do_touch ();
    } else {
        do_taste ();
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Their timed works: overlay 1's, through its stub (overlay 16's till stage 3: its room).  An overlay's: the hub
** asked through othings, read by hub_mem (it isn't in the window while an overlay is); the names it gives in the main
** RAM (the window's the hub's as othings asks: nil, "") */
#pragma code-name (push, "OV1CODE")
#pragma rodata-name (push, "OV1RODATA")

/* A thing worked (the things' IO_FILL, IO_SHUT, IO_HIDE: its words said), its time (the request's x): quietly; a
** timed work's (HOUSE.md's long tasks: the reply's qty its long task's kind, item its thing's kind), its first words (a
** long task's begun, task.c's) and a long task of the UI's (I_WAIT's kind), its command kept (the UI bank's) and
** carried out again as its time's up (main.c's reach 3: its end, 4, the things' IO_DONE, the test made then); done,
** nothing more */
static void __fastcall__ worked (unsigned char action)
{
    unsigned char k, n;
    unsigned t;

    if (action == IR_GONE) {
        return;                                 /* (Walked over to first, or which asked: the one path for things) */
    }
    hub_mem (0, &k, &HUB->ireq.qty, 1);
    hub_mem (0, &n, &HUB->ireq.item, 1);
    hub_mem (0, &t, &HUB->ireq.x, 2);
    if (action != IR_OK || !k || reach == 4) {
        msg (lb);
        if (action == IR_OK && reach != 4) {
            intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
        }
        if (action == IR_OK && reach == 4) {    /* (A timed work's end, the reach's steps to come: the UI waits for them;
                                                **   the torch to light again by its own steps: HOUSE.md's) */
            hub_mem (0, &k, &HUB->ireq.y, 1);
            if (k >= 0xFD) {
                if (k == 0xFE) {
                    cmd.action = VB_LIGHT_OBJECT;
                    cmd.aux = IO_LIGHT;
                    cmd.other.word[0] = 0;
                    cmd.obj.sel = 0;
                    hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
                    reach = 5;
                }
                intend2 (I_WAIT, 1, I_WAIT_W2 (0, 0, 1));
            }
        }
        return;
    }
    what_hi = n & 0x80;
    task_words (TE_BEGUN | (unsigned) k << 8, !(k & LT_STEP) ? n | 0x80 : n);    /* (The steps' kinds, 8 bits: a mang sharpened) */
    hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
    reach = 3;
    intend2 (I_WAIT, t, I_WAIT_W2 (k & ~SM_ALL, n, 1));   /* (Its long task the verb: "the old torch" its first words' alone) */
}

/* The things' timed works, by their handler's number h (H_FILL ... H_HIDDEN): 1 carried out; 0, H_SHUT's: none that
** closes of that name, a door's work then (hplaces.c's H_DOOR).  The verb the command's */
unsigned char __fastcall__ o_things_work (unsigned char h)
{
    unsigned char d = reach == 4 ? IO_DONE : 0;    /* (A timed work's end: main.c's reach) */
    unsigned char action = cmd.action;

    switch (h) {
    case H_FILL:                                /* (A light renewed, slice 4.1's: "fill the lantern", "dip the torch in
                                                **   the oil", the things' IO_FILL; its time, quietly: a timed work) */
        if (cmd.aux == IO_SHARP || strstr (cl, "sharpen") || strstr (cl, "hone") || strstr (cl, "whet")) {   /* (IO_SHARP:
                                                **   "use the whetstone", part 29's) */
            d |= IO_SHARP;                      /* (A blade's words: a light has no edge, the things') */
        }
        worked (othings (IO_FILL | IO_STEPS | IO_GO, cmd.obj.word, cmd.other.word, d));
        break;
    case H_LOOK_IN:                             /* (Looked into a thing, onto it: "look in the sack", "look on the */
                                                /*   table", "look into the well", the user's, 2026-10-21; the */
                                                /*   world's IO_INSIDE; a way, "look into the passage", a look */
                                                /*   that way, way.c's) */
        if (cmd.obj.kind) {
            look_way (VB_OBSERVE_THROUGH);
            break;
        }
        action = othings (IO_INSIDE | IO_STEPS | IO_GO, cmd.obj.word, nil, 0);   /* (The one path for things: out of
                                                **   reach, walked over to first, then again) */
        if (action == IR_NONE) {
            say_no (A_SEE_NO, cmd.obj.word);
        } else if (action == IR_OK) {           /* (IR_NO: a step of the reach first, part 19's) */
            msg (lb);
            intend2 (I_WAIT, 30, I_WAIT_W2 (0, 0, 1));   /* (A look's three seconds) */
        }
        break;
    case H_TIE:                                 /* (A rope's: the things', wrope.c's, IO_HIDE's shape; a sack's tie, as it's
                                                **   opened or closed) */
        if (hide (HD_ROPE)) {
            worked (othings (IO_HIDE | IO_STEPS | IO_GO, cmd.obj.word, cmd.other.word,
                d | IO_ROPE | (action == VB_UNTIE_OBJECT ? RP_UNTIE : cmd.aux)));
            break;
        }
        cmd.aux = action == VB_UNTIE_OBJECT ? PA_OPEN : PA_CLOSE;
    case H_SHUT:                                /* (A thing that closes, worked: "open the chest", "untie the sack",
                                                **   "pick the chest's lock", "force the chest", the user's; the world's
                                                **   IO_SHUT, a lid, a tie, a stopper, a lock (lock's PA_LOCK its row's,
                                                **   actions.c's); none of that name, a door's work: H_DOOR's, 0.  Its
                                                **   verbs the table's TK_CLOSER) */
        if (!cmd.obj.word[0] || cmd.obj.kind) {
            return 0;
        }
        action = othings (IO_SHUT | IO_STEPS | IO_GO, cmd.obj.word, nil, cmd.aux | d);
        if (action == IR_NONE) {
            if (cmd.action != VB_LOCK_OBJECT) {
                return 0;
            }
            say_no (A_SEE_NO, cmd.obj.word);
        } else if (action == IR_NO && (cmd.action == VB_TIE_OBJECT || cmd.action == VB_UNTIE_OBJECT) && strstr (lb, "can't")) {
            unknown (U_CANT);                   /* ("Tie the torch": "You can't tie the torch.", its verb the line's, not the
                                                **   lid's "close" the world worked it as: 2026-11-08) */
        } else {
            worked (action);                    /* (Its lock picked, it forced: a timed work) */
        }
        break;
    default:                                    /* (H_HIDDEN: a thing hidden, begun: main.c's hide, the things' IO_HIDE) */
        worked (IR_OK);
    }
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
