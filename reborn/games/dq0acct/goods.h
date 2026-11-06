/*
** goods.h - (dq0acct, dq0town) what's carried, its money and its worth: the account's and the town's alike (HOUSE.md's between
** dungeons, the market, the smith), each program its own copy, in its main RAM.  Included once, where these are: the kinds (CAT)
** and what's carried (K, kn of it, gone) in a bank at the window; the money (cash, farthings), the days gone by (passed: whetting's
** months); say, sayn, fail, in_ and top.
*/

/* A thing wanted (restocked, or bought): its kind, how many, where it goes, the thing it goes in (PL_IN: the list's entry plus 1) */
struct want {
    unsigned char   kind, n, place, in;
};

static unsigned char __fastcall__ is_coin (const struct kitrec* r)
{
    return (CAT[r->o.kind].form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT);
}

/* How many of it (a lot its count, else one) */
static unsigned char __fastcall__ many (const struct kitrec* r)
{
    return CAT[r->o.kind].form & KF_COUNT ? r->o.qty : 1;
}

/* What's used up, and comes back at a price (the plan's: torches, oil, food, bandages; every lot but coins and water) */
static unsigned char __fastcall__ used_up (unsigned char k)
{
    return k != KI_WATER && (k == KI_TORCH || k == KI_CANDLE || k == KI_OIL || k == KI_PITCH || k == KI_CHALK ||
        (CAT[k].form & (KF_COUNT | KF_SMALL)) == KF_COUNT);
}

/* A thing's worth, a sound one, one of it (the market's, things.c's goods: a kind written in silver by its material), farthings */
static unsigned long __fastcall__ worth (const struct kitrec* r)
{
    const struct kind* k = &CAT[r->o.kind];
    unsigned long v = k->value;
    unsigned char m = r->dur_max ? r->mat : r->o.mat;

    if (k->mat == MT_SILVER && m && m != MT_SILVER && m != MT_JEWEL) {
        v = m == MT_GOLD ? v * 20 : m == MT_STONE ? v * 8 : m == MT_GLASS ? v / 10 : v / 60;
    }
    return v;
}

/* Its condition, in tenths of its life (the market's: at least 1) */
static unsigned char __fastcall__ tenths (const struct kitrec* r)
{
    unsigned char t;

    if (!r->dur_max) {
        return 10;
    }
    t = (unsigned char) (((unsigned long) r->dur * 10 + r->dur_max - 1) / r->dur_max);
    return t ? t : 1;
}

/* What the market gives for it all: half its worth, by its condition (HOUSE.md's the market), at least a farthing */
static unsigned long __fastcall__ sell_price (const struct kitrec* r)
{
    unsigned long v = worth (r) * many (r) * tenths (r) / 20;

    return v ? v : 1;
}

/* Things in thing i */
static unsigned char __fastcall__ holds (unsigned char i)
{
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k] && K[k].o.place == PL_IN && K[k].o.in == i + 1) {
            return 1;
        }
    }
    return 0;
}

static unsigned char __fastcall__ in_hand (const struct kitrec* r)
{
    return r->o.place == PL_HAND || r->o.place == PL_OFF || r->o.place == PL_BOTH;
}

static void cash_count (void)
{
    unsigned char k;

    cash = 0;
    for (k = 0; k < kn; ++k) {
        if (!gone[k] && is_coin (&K[k])) {
            cash += CAT[K[k].o.kind].value * (unsigned long) K[k].o.qty;
        }
    }
}

static void __fastcall__ coin_add (unsigned char kind, unsigned char q)
{
    if (kn < NOFIT) {
        memset (&K[kn], 0, sizeof K[0]);
        K[kn].o.kind = kind;
        K[kn].o.qty = q;
        K[kn].o.quality = 4;
        K[kn].o.place = PL_POUCH;
        ++kn;
    }
}

/* cash laid out as the kit's piles: the dearest coins kept as far as they go, then pennies in lots of 250 at most, its odd farthings */
static void cash_lay (void)
{
    unsigned long f = cash, v, n;
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k] && is_coin (&K[k]) && K[k].o.kind != KI_PENNIES && K[k].o.kind != KI_FARTHINGS) {
            v = CAT[K[k].o.kind].value;
            n = v ? f / v : 0;
            if (n > K[k].o.qty) {
                n = K[k].o.qty;
            }
            f -= n * v;
            if (!(K[k].o.qty = (unsigned char) n)) {
                gone[k] = 1;
            }
        }
    }
    for (k = 0; k < kn; ++k) {
        if (!gone[k] && K[k].o.kind == KI_PENNIES) {
            n = f / PENNY;
            n = n > 250 ? 250 : n;
            f -= n * PENNY;
            if (!(K[k].o.qty = (unsigned char) n)) {
                gone[k] = 1;
            }
        }
    }
    while (f >= PENNY && kn < NOFIT) {
        n = f / PENNY;
        n = n > 250 ? 250 : n;
        coin_add (KI_PENNIES, (unsigned char) n);
        f -= n * PENNY;
    }
    for (k = 0; k < kn; ++k) {
        if (!gone[k] && K[k].o.kind == KI_FARTHINGS) {
            if (!(K[k].o.qty = (unsigned char) f)) {
                gone[k] = 1;
            }
            f = 0;
        }
    }
    if (f) {
        coin_add (KI_FARTHINGS, (unsigned char) f);
    }
}

#ifndef GOODS_TOWN                              /* (dq0town's: not the account's words of the money) */
/* The money in words: as it is, or (changed) as the next place's money-changer will give it (the 15%, each pile: outfit.c's) */
static void __fastcall__ coins_say (unsigned char changed)
{
    unsigned p = 0, f = 0, c = 0, q;
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k] && is_coin (&K[k])) {
            q = K[k].o.qty;
            if (changed) {
                q -= (q * MONEY_CUT + 50) / 100;
            }
            if (K[k].o.kind == KI_PENNIES) {
                p += q;
            } else if (K[k].o.kind == KI_FARTHINGS) {
                f += q;
            } else {
                c += q;
            }
        }
    }
    if (c) {
        sayn (c);
        say (c == 1 ? " crown" : " crowns");
        say (p || f ? (p && f ? ", " : " and ") : "");
    }
    if (p) {
        sayn (p);
        say (p == 1 ? " penny" : " pennies");
        say (f ? " and " : "");
    }
    if (f) {
        sayn (f);
        say (f == 1 ? " farthing" : " farthings");
    }
    if (!p && !f && !c) {
        say ("no money");
    }
}

#endif

/* A kind's name, one or n */
static void __fastcall__ kind_say (unsigned char k, unsigned char n)
{
    const char* s = CAT[k].name;
    const char* of = strstr (s, " of ");
    unsigned char l = strlen (s);
    char b[20];

    if (CAT[k].flags & K_PLURAL) {
        if (n > 1 || CAT[k].form & KF_COUNT) {
            sayn (n);
            say (" ");
        }
        say (s);
        return;
    }
    if (n > 1) {
        sayn (n);
        say (" ");
        if (of) {
            l = of - s;
        }
        memcpy (b, s, l);
        b[l] = 0;
        say (b);
        say (b[l - 1] == 'h' || b[l - 1] == 's' ? "es" : "s");
        if (of) {
            say (of);
        }
        return;
    }
    say (strchr ("aeiou", *s) ? "an " : "a ");
    say (s);
}

/* The smith's price for thing i: mending a tenth of its price a tenth of its life, at most half, past that a new one at its whole
** price (the plan's); no mending but of what's its own and whole enough to call a thing */
static unsigned long __fastcall__ mend_price (const struct kitrec* r)
{
    unsigned long v = worth (r), m;

    if (!r->dur_max || r->dur >= r->dur_max || is_coin (r) || used_up (r->o.kind) || r->o.kind == KI_WRECK || r->o.kind == KI_BROKEN_SWORD) {
        return 0;
    }
    m = v * (10 - tenths (r) + 1) / 10;
    return m > v / 2 ? v : m;
}

/* Whetting and oiling: a hundredth of its price a month, at least a farthing, what's worn alone (only what wears today wears) */
static unsigned long __fastcall__ whet_price (const struct kitrec* r)
{
    unsigned long v;

    if (!r->wear || !r->dur_max || is_coin (r)) {
        return 0;
    }
    v = worth (r) * (passed / 30 ? passed / 30 : 1) / 100;
    return v ? v : 1;
}

/* What seeing to it would cost: mending (it's whetted with it), else whetting */
static unsigned long __fastcall__ owed (const struct kitrec* r)
{
    unsigned long v = mend_price (r);

    return v ? v : whet_price (r);
}

#ifndef GOODS_TOWN                              /* (Nor its restocking's) */
/* It still has one of these kinds */
static unsigned char __fastcall__ has_kind (unsigned char a, unsigned char b)
{
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k] && K[k].o.kind >= a && K[k].o.kind <= b) {
            return 1;
        }
    }
    return 0;
}

/* A kind wanted that's any use to it: no arrows, bolts or sling stones with nothing left to shoot them */
static unsigned char __fastcall__ of_use (unsigned char kind)
{
    return kind == KI_ARROWS ? has_kind (KI_SHORTBOW, KI_HARTBOW) : kind == KI_BOLTS ? has_kind (KI_CROSSBOW, KI_CROSSBOW) :
        kind == KI_STONES ? has_kind (KI_SLING, KI_SLING) : 1;
}

#endif

static void __fastcall__ thing_in (unsigned char kind, unsigned char n, const struct want* w)
{
    if (kn >= NOFIT) {
        return;
    }
    memset (&K[kn], 0, sizeof K[0]);
    K[kn].o.kind = kind;
    K[kn].o.qty = n;
    K[kn].o.quality = 4;
    K[kn].o.place = w->place;
    K[kn].o.in = w->in;
    if (w->place == PL_IN && (!w->in || gone[w->in - 1])) {
        K[kn].o.place = PL_PACK;
        K[kn].o.in = 0;
    }
    ++kn;
}

/* kit.dat written (or the town's, town.h's TOWN_KIT), what's gone out of it, each thing's place in the list kept */
static void __fastcall__ kit_write (const char* name)
{
    static unsigned char to[NOFIT];
    unsigned char k, n = 0;
    int fd;

    for (k = 0; k < kn; ++k) {
        to[k] = gone[k] ? 0 : ++n;
    }
    if ((fd = open (in_ (top, name), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        fail ("can't write what's carried");
    }
    for (k = 0; k < kn; ++k) {
        if (!gone[k]) {
            if (K[k].o.place == PL_IN) {
                K[k].o.in = K[k].o.in ? to[K[k].o.in - 1] : 0;
                if (!K[k].o.in) {
                    K[k].o.place = PL_PACK;     /* (Its container sold, or gone) */
                }
            }
            write (fd, &K[k], sizeof K[0]);
        }
    }
    close (fd);
}
