/*
** wrope2.c - (dq0w) rope and grapples (wrope.c's the rest, HOUSE.md's), swapped part 98's: a rope in place untied, shaken
** loose, its grapple giving, and looked at.
*/

#pragma local-strings (on)
#pragma code-name ("SW98CODE")
#pragma rodata-name ("SW98RODATA")

#include "wrope.h"

/* Record i's (a rope in place's) other end's record: its number, or 0 (a sky shaft's has none) */
static unsigned __fastcall__ rp_other (unsigned i)
{
    const struct link* l;
    unsigned x, y;
    unsigned char sn, how, h, n, want;

    ip = item_at (i);
    x = ip->x;
    y = ip->y;
    sn = ip->sn;
    how = ip->fuel & RK_HOW;
    h = ip->fuel >> 4 & 7;
    want = ip->kind == KI_TIED ? O_SHAFT_DOWN : O_SHAFT_UP;
    if (how == RK_SKY) {
        return 0;
    }
    if (how == RK_SHAFT) {
        for (n = STRATUM (sn).nlinks, l = STRATUM (sn).link; n; --n, ++l) {
            if (l->x == x && l->y == y && l->kind == want) {
                sn = l->to;
                RP_NEED (sn, x, y);
                break;
            }
        }
    } else {
        x += head_x[h];
        y += head_y[h];
    }
    return rp_on (sn, x, y);
}

/* Rope n come away, coiled at the character's feet: the rope it was, what's tied to it with it */
static void __fastcall__ rp_coil (unsigned n)
{
    struct item* p;
    unsigned x, y;
    unsigned char sn;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    --HUB->ropes;
    p = item_at (n);
    p->kind = KI_ROPE;
    p->where = IW_SQUARE;
    p->sn = sn;
    p->x = x;
    p->y = y;
    p->owner = p->hide = 0;
    p->fuel = 0;
}

/* The rope in place whose foot's record is i come away from above, its grapple shaken loose or giving under a climber's
** weight: its top's found, both gone but the rope, coiled at the feet; its noise */
static void __fastcall__ rp_loose (unsigned i)
{
    unsigned t = rp_other (i);

    if (t) {
        item_at (i)->kind = 0;
    } else {
        t = i;                                  /* (The rope itself) */
    }
    rp_coil (t);
    rp_noise (5);
    den_saw = 1;                                /* (The world to look about again: what lies there) */
}

/* Untied: "untie the rope".  At its top, a round's, coiled at its feet (what it was tied to as it is: a spike stays driven);
** at its foot, a grapple shaken loose, a test a round; a knot above, out of reach; carried, its grapple off, at its feet */
static unsigned char rp_untie (void)
{
    struct req* r;
    unsigned char sn, k;
    unsigned x, y, i = 0, n;
    signed char g;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    for (k = 0; k < 9 && !i; ++k) {
        i = rp_on (sn, k ? x + head_x[k - 1] : x, k ? y + head_y[k - 1] : y);
    }
    if (!i) {
        if ((n = rp_carried (KI_ROPE)) != 0 && (i = rp_in (n, KI_GRAPPLE)) != 0) {
            ip = item_at (i);
            ip->where = IW_SQUARE;
            ip->sn = sn;
            ip->x = x;
            ip->y = y;
            rp_new ();
            put ("\nYou untie the grapple from the rope: it lies at your feet.");
            return rp_done (50);
        }
        return rp_no ("\nYou see no rope tied here.");
    }
    k = ip->kind;
    if (k == KI_TIED) {
        if ((n = rp_other (i)) != 0) {
            item_at (n)->kind = 0;
        }
        rp_coil (i);
        rp_new ();
        put ("\nYou untie the rope, draw it up and coil it at your feet.");
        return rp_done (100);
    }
    if (item_at (i)->hide != KI_GRAPPLE) {
        return rp_no ("\nThe knot's far above you, out of reach.");
    }
    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];                 /* (Shaken loose: the Throwing skill's knack) */
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = P_THROW;
    r->b = 0;
    r->steps = 0;
    r = hub_ask (SLOT_WORLD);
    g = r->grade;
    r->state = REQ_FREE;
    rp_new ();
    if (G_OK (g)) {
        put ("\nYou shake the rope, and the grapple comes loose and clatters down.");
        rp_loose (i);
    } else {
        put ("\nYou shake the rope, but the grapple holds fast.");
    }
    hub_map (0);
    return rp_done (HUB->round);
}

/* A rope in place, thing n, looked at: what it's tied to, how far it goes (after what's said of it) */
static void __fastcall__ rp_look (unsigned n)
{
    char b[6];
    unsigned char how, end, k, hold;

    if (!n || ((ip = item_at (n))->kind != KI_TIED && ip->kind != KI_HUNG)) {
        hub_map (0);
        return;
    }
    how = ip->fuel & RK_HOW;
    end = ip->owner;
    hold = ip->fuel >> 8;
    k = ip->hide;
    put (" It's tied to the ");
    rp_name (k);
    put (how == RK_SKY ? ", in the mouth of the shaft above: " : how == RK_SHAFT ? ", in the shaft: " :
        how == RK_PIT ? ", over the pit's edge: " : ", over the edge: ");
    put (utoa (hold, b, 10));
    put (" feet of the climb");
    if (end) {
        put (", its end ");
        put (utoa (end, b, 10));
        put (" feet above the foot");
    }
    put (".");
    hub_map (0);
}

/* (Its stub's: 0 a request of the things'; RA_LOOK | n a rope in place looked at; RA_LOOSE | n its foot's record, its grapple
** giving) */
unsigned char __fastcall__ s_rope2 (unsigned w)
{
    if (w & RA_LOOSE) {
        rp_loose (w & ~RA_LOOSE);
        return 0;
    }
    if (w) {
        rp_look (w & ~RA_LOOK);
        return 0;
    }
    return rp_untie ();
}
