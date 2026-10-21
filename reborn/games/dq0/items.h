/*
** items.h - Dingus Quest Zero's things (DUNGEON.md: items, the dungeon's layer of things): a catalogue of kinds (what
** a thing is: its name, its weight and bulk, a container's room, a belt's slots, a light's reach, a tool's key for
** the referee), and the items themselves, each a record of what it is and where: on a square of the dungeon, an
** actor's (carried, worn, held), or inside another item.  Both in the hub's banks (after the dungeon's), the world's
** alone (hub.h's who writes what: games/dq0w/things.c, its item store): the maker lists a chunk's ordinary things as
** it first makes it (struct stock), the world puts them down, and a denizen's remains as it dies; the others
** read them (the senses, what the character perceives; the UI, a thing's record seen) and ask the world (ireq).
*/

#ifndef _ITEMS_H
#define _ITEMS_H

/* What a thing's made of: its resistance to each kind of harm (0 none ... 9 all but proof: the referee's impact, not a
** difficulty), and its toughness (the durability a quarter pound of it gives, at middling quality) */
enum { MT_NONE, MT_WOOD, MT_IRON, MT_STEEL, MT_BRONZE, MT_LEATHER, MT_CLOTH, MT_BONE, MT_CLAY, MT_STONE, MT_GLASS,
    MT_SILVER, MT_OTHER, NMATS };
#define MAT_TOUGH       { 1, 3, 6, 8, 5, 3, 1, 2, 1, 8, 1, 4, 1 }
enum { DM_BLUNT, DM_EDGE, DM_POINT, DM_FIRE, DM_COLD, DM_ACID, NDMG };  /* Kinds of harm (the referee's, by aspect) */
struct material {
    char            name[8];
    unsigned char   resist[NDMG];
    unsigned char   tough;
};

/* A kind of thing (the catalogue's: a stand-in for the theme's data), in rs.h's units */
#define NKINDS          64
struct kind {
    char            name[14];                   /* "torch", "backpack" ... */
    unsigned char   flags;                      /* K_* */
    unsigned char   weight;                     /* Its weight: quarter pounds (255, near 64 pounds, at most) */
    unsigned char   bulk;                       /* Its size: pints */
    unsigned char   cap_bulk;                   /* A container's (a surface's): the size it holds, pints, */
    unsigned        cap_weight;                 /*   and the weight it bears, quarter pounds */
    unsigned char   slots;                      /* A belt's: things that hang from it */
    unsigned char   light;                      /* A light's reach, lit: squares */
    unsigned char   seen;                       /* How far off it's made out, past a speck: squares (1 a key, 4 a
                                                **   sword, 7 a chest; 15 at most) */
    char            tool[8];                    /* Its key for the referee's tests ("pick", "picks"), or "" */
    unsigned char   mat;                        /* What it's mostly made of, as a rule (MT_*: one may differ) */
    unsigned char   craft;                      /* The craft that knows it (rs.h's AP_*: its appraiser's) */
    unsigned        value;                      /* Its worth, a sound one, new: in pennies */
    unsigned char   keeps;                      /* A container's: the forms it keeps in (KF_*: a basket not sand nor
                                                **   water, a sack not water); KF_ON: a surface, things lie on it */
    unsigned char   form;                       /* Its own form (KF_*; 0: KF_SOLID, a thing whole) */
};
#define K_CONTAINER     0x01                    /* Holds things inside it */
#define K_BELT          0x02                    /* Things hang from it, each in a slot */
#define K_LIGHT         0x04                    /* Can be lit */
#define K_WEAR          0x08                    /* Worn */
#define K_HAND          0x10                    /* Held in a hand (a tool, a weapon, a light) */
#define K_HANG          0x20                    /* Hangs from a belt (a pouch, a sheath, a quiver) */
#define K_PLURAL        0x40                    /* Its name's a plural ("bones", "rations") */
#define K_TWO           0x80                    /* Held in both hands (a pick, a shovel) */
#define KF_SOLID        0x01                    /* Forms: a thing whole; */
#define KF_SMALL        0x02                    /*   small things, grains (coins, sand: through a basket's weave); */
#define KF_LIQUID       0x04                    /*   a liquid; */
#define KF_GAS          0x08                    /*   a gas */
#define KF_ON           0x10                    /* (A kind's keeps: a surface, a table's, a chair's) */
#define KF_FIXED        0x80                    /* (And fixed where it stands, never taken up: a theme's altar) */

/* The catalogue's kinds, by number (things.c's order: the maker puts them down by these) */
enum { KI_NONE, KI_TORCH, KI_BACKPACK, KI_BELT, KI_POUCH, KI_DAGGER, KI_WATERSKIN, KI_RATIONS, KI_TINDERBOX, KI_PICK,
    KI_SHOVEL, KI_LOCKPICKS, KI_CHISEL, KI_ROPE, KI_BONES, KI_PENNIES, KI_KEY, KI_CANDLE, KI_LANTERN, KI_SACK, KI_CHEST,
    KI_SKULL, KI_BROKEN_SWORD, KI_CLOAK, KI_POTSHERDS, KI_SWORD, KI_BASKET, KI_SAND, KI_TABLE, KI_CHAIR, KI_QUIVER,
    KI_ARROWS, KI_BAR, KI_PANE, KI_BARS, KI_SHARDS,  /* (A door's bar, a window's pane and its bars: a portal's parts,
                                                **   IW_PORTAL; a pane broken, its shards) */
    KI_BODY,                                    /* (A denizen dead: the denizens' task's) */
    KI_LEATHER_CAP, KI_LEATHER_TUNIC, KI_GAMBESON, KI_RING_BYRNIE, KI_MAIL_HAUBERK, KI_GAUNTLETS, KI_BOOTS,  /* Armour (its
                                                **   key the referee's piece) */
    KI_HELM, KI_SANDALS, KI_SHOES, KI_HOBNAIL, /* (A helm; footwear: its soles the referee's, against a kick) */
    KI_MACE, KI_SPEAR, KI_SHIELD,               /* Weapons (the referee's by their keys) */
    KI_POT, KI_STRAW, KI_STUBS, KI_WELL,        /* What a room's use left (dq0m/uses.c): a kitchen's pot, a barracks'
                                                **   straw, a chapel's candle stubs; a well, fixed */
    KI_COUNT };

/* An item */
#define NITEMS          (ITEM_BANKS * (0x2000 / sizeof (struct item)))
struct item {
    unsigned char   kind;                       /* 0: a free record; else the catalogue's (1 on) */
    unsigned char   flags;                      /* I_* */
    unsigned char   where;                      /* IW_* */
    unsigned char   sn;                         /* (On a square: its stratum) */
    unsigned        x, y;                       /* On a square: it; an actor's: x the actor (0 the character); in
                                                **   an item: x that item's number */
    unsigned char   qty;
    unsigned char   owner;                      /* Whose it is (0: nobody's; a faction's, an actor's) */
    unsigned char   wear;                       /* Its wear, 0 new ... 255 (a blade dug with, dulled) */
    unsigned char   hide;                       /* Concealed on purpose (I_HIDDEN): how hard to find, the steps worse
                                                **   a search for it goes (rs.h's: the method's and the concealer's
                                                **   skill's; 0 a floor's dust and corners only) */
    /* Its own (no two alike: the world's, as it first comes to it, a look's; id 0 till then) */
    unsigned        id;                         /* Its number, never another's (record 0's x the last given) */
    unsigned char   quality;                    /* How well it's made: 0 crude ... 9 a master's */
    unsigned char   mat;                        /* What it's made of (MT_*: its resistances) */
    unsigned        dur, dur_max;               /* Its durability: the harm it can take before it breaks, what's
                                                **   left (its most by its quality, its material, its weight) */
    unsigned char   size;                       /* Its measure, where it matters (a portal's part's: its portal's
                                                **   opening, world.h's: feet, its width the low four bits; or 0) */
    unsigned char   thick;                      /*   and its thickness, in eighths of an inch (or 0) */
};
#define I_LIT           0x01                    /* A light, lit */
#define I_HELD          0x02                    /* In a hand */
#define I_WORN          0x04                    /* Worn (a belt, a pack, armour) */
#define I_HIDDEN        0x08                    /* Hidden (found by searching) */
#define I_MAGIC         0x10                    /*   by magic (found only by magical means) */
#define I_KNOWN         0x20                    /* Known to the character (seen, or found) */
#define I_NAMED         0x40                    /* Named by the character (the UI's names: hub.h's struct tname, by
                                                **   its id; marked in what's carried, worded: "#id a sword") */
enum { IW_SQUARE, IW_ACTOR, IW_ITEM,
    IW_PORTAL };                                /* A portal's part (a door's bar, a window's pane or bars: the maker's,
                                                **   as it stocks a chunk), in it: sn, x, y its square (a bar's, the
                                                **   square on its bar's side), seen and judged as a thing lying there
                                                **   is, not taken up (IR_FIXED) but lifted out (the maker's) */

#define ITEM_BANKS      2

/* A request to the things (the world's item store: one at a time, the hub's): what's done, to which item, by whom, and
** what came of it */
struct ireq {
    RQH;                                        /* (hub.h's protocol; kind: IO_*) */
    unsigned        item, to;                   /* The item; where it goes (a container's number) */
    unsigned char   result;                     /* IR_* */
    char            name[16];                   /* A thing's name (IO_GET's, IO_DROP's; IO_KIT's things more are in
                                                **   things_text, commas between) */
    unsigned        x, y;                       /* IR_NOT_HERE's: where it is (and item the thing: I_TAKE's) */
    unsigned char   craft, quality, mat, plural, qty;   /* IO_DESCRIBE's: what it is (its kind's name in name) */
    unsigned        dur, dur_max, value;
    unsigned char   size, thick;                /*   (and its size, its thickness: the item's) */
    char            into[16];                   /* The other thing's name (IO_PUT's, IO_TAKE's: in it, on it, from it) */
};
enum { IO_KIT, IO_GET, IO_DROP, IO_LIST, IO_SAVE, IO_RESTORE,
    IO_TOOLS,                                   /* The tools' keys of all it carries, into things_text (for the
                                                **   referee: a skill used, the right tool to hand takes itself) */
    IO_WEAR,                                    /* The tool with the key (name) worn by to: its name back in name */
    IO_SEARCH,                                  /* The things concealed about it searched for (the referee's tests,
                                                **   each at its difficulty), as far as to reaches (the search's,
                                                **   E_PORTAL's a): the result the number found, worded in
                                                **   things_text */
    IO_DESCRIBE,                                /* A thing (carried, or seen) looked at closely: what it truly is,
                                                **   in the request (for the appraiser's judgement: the UI's) */
    IO_HIDE,                                    /* A thing carried hidden where it stands (P_CONCEAL, the referee's
                                                **   test): its difficulty back in qty */
    IO_FIND,                                    /* The thing named that's to hand (carried, or within reach: the
                                                **   theme's puzzle's): its kind back in x, and 256 if carried */
    IO_COMBINE,                                 /* The things named (name, into) to hand made one: name's becomes kind
                                                **   to (a theme's: its rule's), into's gone */
    /* A thing handled (name; into the other): each worded into things_text, the time it took back in x (tenths) */
    IO_HOLD,                                    /* Taken into a hand (two at most; a two-handed thing both) */
    IO_STOW,                                    /* Put in the pack (its room, its weight, what it keeps) */
    IO_PUT,                                     /* Put in into (to 0) or on it (to 1: a surface's; a belt's: hung) */
    IO_TAKE,                                    /* Taken from into: into the pack, else a hand */
    IO_DON, IO_DOFF,                            /* Worn; taken off (into a hand, else the pack) */
    IO_LIGHT, IO_DOUSE,                         /* Lit (a tinderbox carried, or a flame); put out ("": the first) */
    IO_BAR };                                   /* A door's bar to hand (carried, or within reach, not in brackets):
                                                **   the widest's width back in x (to 0: result IR_NO, x 0 none); or
                                                **   it set in a door's brackets (to 1: the maker's barred it), a
                                                **   portal's part again, IW_PORTAL, on the request's x, y (the
                                                **   character's square, the bar's side), its item the stratum */
#define IO_NAME         (IO_BAR + 1)            /* A thing (carried, or seen) marked named (to 1: I_NAMED), or not
                                                **   (to 0) */
#define IO_THEME        (IO_BAR + 2)            /* The theme's kinds (the theme bank's, the UI's) into the catalogue */
#define IO_BEGIN        (IO_BAR + 3)            /* The game begun (the UI's last as it starts: the world's on to its
                                                **   dungeon, the things asked so far, the kit) */
#define IO_THROW        (IO_BAR + 5)            /* The thing named carried ("": the first in its hands) thrown at a
                                                **   mark (x, y: a square it sees), how (to: struct cmd's manner): how
                                                **   it went back in qty (TH_*), where it lies in x, y (the world's
                                                **   throw: the catalogue's throws, its reach; the referee's test) */
enum { TH_HIT, TH_MISS, TH_SHORT };             /* (IO_THROW's: it hit the mark, missed it, fell short of it) */
#define IO_PUZZLE       (IO_BAR + 4)            /* A command of the theme's puzzle's (theme.h), judged by its rules
                                                **   (the world's): to its verb (TR_*), name the thing (into the other:
                                                **   combine name with into, put name on into; read name; use name on
                                                **   the way out); each found to hand (IO_FIND's), the rule for their
                                                **   kinds looked for, and done (the things made one, IO_COMBINE's;
                                                **   put, IO_PUT's; the way out opened, the hub's way_open): result
                                                **   PZ_*, x the rule done; the things' words of it in things_text
                                                **   (a thing not found, put: "" none) */
enum { PZ_DONE, PZ_NONE, PZ_NO, PZ_FAR };       /* (IO_PUZZLE's: a rule done; none for those kinds; not done, the
                                                **   things' words why; the way out out of reach) */
enum { IR_OK, IR_NONE, IR_FULL, IR_HEAVY, IR_NOT_HERE, IR_NO, IR_FIXED };    /* (IR_NO: refused, the things' words
                                                **   why; IR_FIXED: a portal's part, in it) */

/* What the character carries, for the UI to word (IO_LIST's): each thing, how deep it's in others, its state */
#define NCARRY          16
struct carried {
    unsigned char   n;
    struct {
        char        name[14];
        unsigned char qty, depth, flags;        /* depth: 0 on it, 1 in (or on) the first, 2 ... */
        unsigned    item;                       /* Its number */
    } e[NCARRY];
    unsigned        weight;                     /* In quarter pounds, all told */
};

/* A chunk's things, as the maker stocks it the first time it's made (dq0m/stock.c: from the seed), each one's kind, its
** square, its make; the world puts them down (the world bank's stock, a slot's: struct wrec's, as the slot's CS_FRESH,
** world.h's) in the order the maker stocked the chunks */
#define NSTOCK          38
struct sthing {
    unsigned char   kind;
    signed char     x, y;                       /* Its square, from the chunk's first (a portal's part's may be in the
                                                **   next chunk: a bar's by its door) */
    unsigned char   qty;
    unsigned char   hide;                       /* Concealed: how hard to find (struct item's, steps); ST_SHOWN not;
                                                **   ST_PORTAL a portal's part in it (else a part lies by it) */
    unsigned char   make;                       /* A portal's part's: its material (MT_*) times 16, and its quality */
    unsigned char   size;                       /*   and its size (its portal's opening, a bar's its width) */
};
#define ST_SHOWN        0xFF
#define ST_PORTAL       0xFE
struct stock {
    unsigned        seq;                        /* The chunks the maker's stocked, counted, as it stocked this one */
    unsigned char   n;
    struct sthing   e[NSTOCK];
};

#endif
