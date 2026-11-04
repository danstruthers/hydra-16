/*
** errand.c - (dq0) an ally's errands and the character's gifts, the UI's part (slice 4.8, 2026-10-30: DENIZENS.md's
** fetching and influence): before the denizens are asked (deal's: the hub's sway), the thing found, the world's
** (IO_FETCH): for "tell Grub to fetch the key", one seen lying about (its item and where it lies, the sway's item,
** ix, iy); for "give the bandit 5 pennies" ("give the pennies to the bandit", "offer the orc my dagger"), one the
** character carries (its item, its worth as a local takes it times the count, whether it's food), the one meant the
** command's thing then, the gift its other; for "follow me at 3 squares", the distance (ix).  Swapped part 20's, the
** market part 15's.
*/

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "actions.h"

#pragma code-name ("SW20CODE")
#pragma rodata-name ("SW20RODATA")
#pragma local-strings (on)

/* ---- The market and entertaining (2026-11-02: DENIZENS.md's trading, HOUSE.md's the market, for review): the goods
** found first, the world's (the character's: IO_FETCH's, carried; one's it trades with: IO_GEAR's goods, the nearest
** named, or any, that has them), each side's pennies, then the denizens asked (the sway: they decide, and the one
** traded with takes and hands over by the character's own way with things, the world's), its outcome worded here, the
** world telling the things changing hands (E_TASK's TE_FETCHED).  Its data the part's own (zeros as it's copied in) */
unsigned char trade (void);                     /* (Part 15's stub) */

/* A word of the line's: in lower case into w (15 at most); the line on past it */
static const char* __fastcall__ word_at (const char* p, char* w)
{
    unsigned char n = 0;

    while (*p == ' ') {
        ++p;
    }
    for (; *p && *p != ' '; ++p) {
        if (n < 15) {
            w[n++] = *p >= 'A' && *p <= 'Z' ? *p + 'a' - 'A' : *p;
        }
    }
    w[n] = 0;
    return p;
}

#pragma code-name (push, "SW15CODE")            /* (Part 15's, the market's; the rest part 20's: its room, 2026-11-03) */
#pragma rodata-name (push, "SW15RODATA")
#pragma bss-name (push, "SW15RODATA")
static char gd[16], wh[16], nm[16], nl[16];

/* (A word of the line's: word_at's, in the part) */
static const char* __fastcall__ word_at2 (const char* p, char* w)
{
    unsigned char n = 0;

    while (*p == ' ') {
        ++p;
    }
    for (; *p && *p != ' '; ++p) {
        if (n < 15) {
            w[n++] = *p >= 'A' && *p <= 'Z' ? *p + 'a' - 'A' : *p;
        }
    }
    w[n] = 0;
    return p;
}

#define S       (HUB->sway)
unsigned char s_trade (void)
{
    static const char played[4][10] = { "entertain", "sing for", "play for", "dance for" };
    struct ireq* q = &HUB->ireq;
    unsigned char h = cmd.aux, sell, r, life, o;
    unsigned it, w = 0xFFFF, qty, n, item, its = 0, nits = 0, c = 0, ni = 0, fc = 0, fi = 0, fn = 0, fm = 0;
    unsigned long v;
    char t[16], pr[16], ps[6];

    strcpy (gd, cmd.obj.word);
    strcpy (wh, cmd.other.word);
    item = 0;
    o = sell = 0;
    it = w = 0;
    if (h == SW_ENTERTAIN) {                    /* (The one entertained the thing said; the skill the verb said) */
        word_at2 (cl, t);
        o = !strcmp (t, "sing") ? PF_SING : !strcmp (t, "play") ? PF_PLAY : !strcmp (t, "dance") ? PF_DANCE : PF_ANY;
        strcpy (wh, gd);
        r = 0;
        goto ask;
    }
    if (!gd[0]) {
        msg ("Which thing?");
        return 0;
    }
    q->sel = 0;
    q->to = 1;                                  /* (The character's: sold, or haggled over as it's sold) */
    sell = h != SW_BUY && h != SW_PRICE && things (IO_FETCH, gd) == IR_OK;
    if (!sell && (h == SW_SELL || h == SW_OFFER)) {
        sayf ("\nYou have no %s.", gd);
        return 0;
    }
    if (sell) {
        item = q->item;
        v = q->value;
        qty = q->qty;
        w = q->dur;
        n = q->dur_max;
    }
    q->to = 0x80;                               /* (Its goods, or its pennies: IO_GEAR's GR_GOODS) */
    q->x = 0;
    strcpy (q->into, sell ? "pennies" : gd);
    r = things (IO_GEAR, wh);
    if (r == IR_NONE && wh[0]) {                /* (A name given is capitalised: "from Grub") */
        wh[0] &= 0xDF;
        r = things (IO_GEAR, wh);
        wh[0] |= 0x20;
    }
    if (r == IR_NONE) {
        sayf (wh[0] ? "\nYou see no %s here." : "\nThere's no one here to trade with.", wh);
        return 0;
    }
    if (r == IR_OK) {
        if (sell) {
            its = q->item;
            nits = q->qty;
        } else {
            item = q->item;
            v = q->value;
            qty = q->qty;
            w = q->dur;
            n = q->dur_max;
        }
    }
    r = q->x;                                   /* (The one found: plus 1) */
    if (item) {                                 /* (Its worth: a unit's, times the count, by its condition) */
        qty = qty ? qty : 1;
        it = cmd.n && cmd.n < qty ? cmd.n : qty;
        life = n ? (unsigned char) (((unsigned long) w * 10 + n - 1) / n) : 10;
        v = v * it * (life ? life : 1) / 10;
        w = v > 0xFFFF ? 0xFFFF : (unsigned) v;
        it = it < qty ? it << 8 : 0;
        o = sell;
    }
    q->to = 1;                                  /* (The character's pennies) */
    q->sel = 0;
    if ((!sell || h == SW_SELL || h == SW_HAGGLE) && things (IO_FETCH, "pennies") == IR_OK) {
        c = q->item;
        ni = q->qty;
    }
    if (w < 16 && !sell) {                      /* (A price that may come under a penny, the character's farthings) */
        q->to = 1;
        q->sel = 0;
        if (things (IO_FETCH, "farthings") == IR_OK) {
            fc = q->item;
            fn = q->qty ? q->qty : 1;
        }
    }
    if (w < 16 && sell) {                       /* (Its farthings) */
        q->to = 0x80;
        q->x = 0;
        strcpy (q->into, "farthings");
        o = things (IO_GEAR, wh);
        if (o == IR_NONE && wh[0]) {            /* (A name given is capitalised) */
            wh[0] &= 0xDF;
            o = things (IO_GEAR, wh);
            wh[0] |= 0x20;
        }
        if (o == IR_OK) {
            fi = q->item;
            fm = q->qty ? q->qty : 1;
        }
        o = sell;
    }
ask:
    hub_map (0);
    memset (&S, 0, sizeof S);
    S.kind = h;
    S.order = o;
    S.actor = r;
    strcpy (S.word, wh);
    S.item = item;
    S.ix = w;
    S.iy = it | sell;
    SW_COIN (&S) = c;
    SW_NCOIN (&S) = ni;
    SW_ITS (&S) = its;
    SW_NITS (&S) = nits;
    SW_FCOIN (&S) = fc;
    SW_NFCOIN (&S) = fn;
    SW_FITS (&S) = fi;
    SW_NFITS (&S) = fm;
    hub_call (&S, T_DENIZENS);
    S.state = REQ_FREE;
    strcpy (nl, S.name[0] >= 'A' && S.name[0] <= 'Z' ? "" : "the ");    /* (Its name: "the dwarf", "Grub") */
    strcat (nl, S.name);
    strcpy (nm, nl);
    nm[0] &= 0xDF;                              /* ("The dwarf", "Grub" to start a sentence) */
    n = S.ix & ~SW_FARTHINGS;
    strcpy (pr, S.ix & SW_FARTHINGS ? (n == 1 ? "1 farthing" : n == 2 ? "2 farthings" : "3 farthings") : utoa (n, ps, 10));
    if (!(S.ix & SW_FARTHINGS)) {
        strcat (pr, n == 1 ? " penny" : " pennies");
    }
    t[0] = 0;
    switch (S.result) {                         /* (Its words) */
    case SR_NONE:
        msg ("You see no one here to entertain.");
        break;
    case SR_OK:
        if (h == SW_ENTERTAIN) {
            sayf ("\nYou %s %s. %s", played[S.order & 3], nl, S.now > S.was ? (S.grade == G_CS ?
                "It's delighted: it seems friendlier now." : "It's pleased: it seems friendlier now.") : S.now < S.was ?
                "It's bored, and seems the less friendly." : "It isn't moved.");
            break;
        }
        sayf ("\n%s%s%s for %s %s.", nm, h == SW_PRICE ? " asks " : h == SW_OFFER ? " would give you " :
            h != SW_HAGGLE ? " agrees: " : S.grade == G_CS ? " gives way, and agrees: " : S.grade == G_MS ?
            " gives a little, and agrees: " : " won't budge, but agrees: ", pr,
            h == SW_PRICE || !(S.iy & 1) ? "the" : "your", gd);
        break;
    case SR_FAR:
        sayf ("\n%s is too far off: you'd need to be within reach.", nm);
        break;
    case SR_TONGUE:
        sayf ("\n%s doesn't speak any tongue you know: you can't haggle.", nm);
        break;
    case SR_BEAST:
    case SR_MINDLESS:
        sayf (h == SW_ENTERTAIN ? "\n%s pays no heed." : "\n%s has no notion of trade.", nm);
        break;
    case SR_DOWN:
        sayf ("\n%s lies senseless.", nm);
        break;
    case SR_AS_FAR:
        sayf ("\n%s is friendly already: that won't move it further.", nm);
        break;
    case SR_WONT:
        sayf ("\n%s won't trade with you.", nm);
        break;
    case SR_NO_GOODS:
        sayf ("\n%s has no %s to sell.", nm, gd);
        break;
    case SR_POOR:
        sayf ("\nYou haven't the money: %s asks %s.", nl, pr);
        break;
    case SR_BROKE:
        sayf ("\n%s hasn't the money: it would give %s.", nm, pr);
        break;
    case SR_SPURNED:
        sayf ("\nYou haggle too hard: %s takes offence, and won't deal with you now.", nl);
        break;
    default:
        msg ("It won't trade.");
        break;
    }
    if (S.t) {
        intend2 (I_WAIT, S.t, 0);
    }
    return 0;
}
#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

/* "give the bandit 5 pennies", "offer the orc my dagger" (2026-11-02): no "to", the one given it the line's first word
** after the verb that isn't the thing's, a number or a little word */
static void given_to (void)
{
    const char* p = cl;
    char w[16];

    p = word_at (p, w);
    while (*p) {
        p = word_at (p, w);
        if (w[0] && (w[0] < '0' || w[0] > '9') && strcmp (w, cmd.obj.word) && strcmp (w, "the") && strcmp (w, "a") &&
            strcmp (w, "an") && strcmp (w, "my") && strcmp (w, "some") && strcmp (w, "your")) {
            strcpy (cmd.other.word, w);
            return;
        }
    }
}

/* 1 to ask the denizens (the sway's item, ix, iy filled in), or 0 (told why not) */
unsigned char s_errand (void)
{
    struct ireq* q = &HUB->ireq;
    unsigned char r;
    unsigned n, w, v;
    char t[16];

    walk_says ((const char*) W_SWAY);           /* (Its first words at once, the denizens the background: sway.c's) */
    if (cmd.action == VB_TRADE) {
        return trade ();                        /* (The market, entertaining: part 15's, 2026-11-02) */
    }
    hub_map (0);
    HUB->sway.item = HUB->sway.ix = HUB->sway.iy = 0;
    HUB->sway.actor = 0;
    q->sel = 0;
    if (cmd.action == VB_GIVE_OBJECT) {         /* (A gift: the thing the character carries, then the one meant) */
        if (!cmd.other.word[0]) {
            given_to ();
        }
        hub_map (0);
        if (!cmd.other.word[0]) {
            msg ("Give it to whom?");
            return 0;
        }
        q->to = 1;
        if (things (IO_FETCH, cmd.obj.word) != IR_OK) {
            sayf ("\nYou have no %s.", cmd.obj.word[0] ? cmd.obj.word : "such thing");
            return 0;
        }
        hub_map (0);
        n = q->qty ? q->qty : 1;
        if (cmd.n && cmd.n < n) {
            n = cmd.n;                          /* (A count of a lot: split off, the world's) */
            HUB->sway.iy = n << 8;
        }
        v = q->value;
        w = v && n > 0xFFFFU / v ? 0xFFFFU : v * n;
        HUB->sway.item = q->item;
        HUB->sway.ix = w;
        HUB->sway.iy |= q->plural >> 1;         /* (Food: IO_DESCRIBE's reply, by its key) */
        strcpy (t, cmd.obj.word);
        strcpy (cmd.obj.word, cmd.other.word);
        strcpy (cmd.other.word, t);
        cmd.obj.kind = 0;
        return 1;
    }
    if (cmd.order == OR_FETCH) {                /* (A thing to fetch: one seen lying about) */
        if (!cmd.other.word[0]) {
            msg ("Fetch what?");
            return 0;
        }
        q->to = 0;
        if ((r = things (IO_FETCH, cmd.other.word)) != IR_OK) {
            sayf (r == IR_NONE ? "\nYou see no %s here." : "\nThe %s can't be fetched from where it is.",
                  cmd.other.word);
            return 0;
        }
        hub_map (0);
        HUB->sway.item = q->item;
        HUB->sway.ix = q->x;
        HUB->sway.iy = q->y;
    } else if (cmd.order == OR_FOLLOW) {
        HUB->sway.ix = cmd.n;                   /* (Its distance: "follow me at 3 squares") */
    } else if (cmd.order >= OR_OPEN) {          /* (A door's errand, 2026-11-02: the door found, as the character's own
                                                **   command finds it: its square) */
        if (cmd.obj.kind != OB_DOOR) {
            msg ("Ask it to open which door?");
            return 0;
        }
        if (pick (OB_DOOR | PICK_TELL, cmd.obj.lies) != 1) {
            return 0;
        }
        hub_map (0);
        HUB->sway.ix = tx;
        HUB->sway.iy = ty;
    }
    return 1;
}
