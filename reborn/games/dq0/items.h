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
    MT_SILVER, MT_OTHER, MT_WAX, MT_TALLOW, MT_GOLD, NMATS };   /* (Beeswax and tallow: a candle's, slice 4.1's; gold, the
                                                **   valuables', 2026-10-27) */
#define MAT_TOUGH       { 1, 3, 6, 8, 5, 3, 1, 2, 1, 8, 1, 4, 1, 1, 1, 3 }
enum { DM_BLUNT, DM_EDGE, DM_POINT, DM_FIRE, DM_COLD, DM_ACID, NDMG };  /* Kinds of harm (the referee's, by aspect) */
struct material {
    char            name[8];
    unsigned char   resist[NDMG];
    unsigned char   tough;
};

/* A kind of thing (the catalogue's: a stand-in for the theme's data), in rs.h's units */
#define NKINDS          144                     /* (In a bank of their own, the hub's KIND_BANK: 144 x 37
                                                **   bytes; a kind's number kept in seven bits in places, a thing struck
                                                **   with's, a light's: rs.h's SK_THING, E_TASK's; so the kinds from 128
                                                **   on, past the theme's (theme.h's KI_THEME, 120 to 127), are none of
                                                **   those: belongings and valuables, never lit, never a striker, KI_HIGH
                                                **   on: 2026-10-27, the denizens' belongings) */
struct kind {
    char            name[14];                   /* "torch", "backpack" ... */
    unsigned char   flags;                      /* K_* */
    unsigned char   weight;                     /* Its weight: quarter pounds (255, near 64 pounds, at most) */
    unsigned char   bulk;                       /* Its size: pints */
    unsigned char   cap_bulk;                   /* A container's (a surface's): the size it holds, pints, */
    unsigned        cap_weight;                 /*   and the weight it bears, quarter pounds */
    unsigned char   slots;                      /* A belt's: things that hang from it; a carrier's (a container
                                                **   that isn't a belt) and a weapon's or a missile's, its family
                                                **   (FM_*: a scabbard keeps a sword, a quiver arrows, and nothing
                                                **   else; 0, anything: a pouch, a sack) */
    unsigned char   light;                      /* A light's reach, lit: squares */
    unsigned char   seen;                       /* How far off it's made out, past a speck: squares (1 a key, 4 a
                                                **   sword, 7 a chest; 15 at most) */
    char            tool[8];                    /* Its key for the referee's tests ("pick", "picks"), or "" */
    unsigned char   mat;                        /* What it's mostly made of, as a rule (MT_*: one may differ) */
    unsigned char   craft;                      /* The craft that knows it (rs.h's AP_*: its appraiser's) */
    unsigned        value;                      /* Its worth, a sound one, new: in farthings (four to the penny,
                                                **   HarnWorld 23: 2026-10-28) */
    unsigned char   keeps;                      /* A container's: the forms it keeps in (KF_*: a basket not sand nor
                                                **   water, a sack not water); KF_ON: a surface, things lie on it;
                                                **   what closes it (KF_LID, KF_TIE, KF_STOPPER; none, it's open) */
    unsigned char   form;                       /* Its own form (KF_*; 0: KF_SOLID, a thing whole); KF_COUNT, a lot
                                                **   counted (its qty how many: rations, pennies, arrows) */
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
#define KF_LID          0x20                    /* (A kind's keeps: what closes it, the user's, 2026-10-21: a lid, a
                                                **   chest's, a pot's, a backpack's flap; */
#define KF_TIE          0x40                    /*   a tie, a sack's, a pouch's; */
#define KF_STOPPER      0x60                    /*   a stopper, a waterskin's, a flask's; none, a basket, a quiver) */
#define KF_CLOSES       0x60
#define KF_FORMS        0x0F                    /* (A kind's form's own: KF_SOLID ... alone) */
#define KF_COUNT        0x40                    /* (A kind's form: a lot of them counted, its qty how many, split and
                                                **   joined again as they're moved: rations, pennies, arrows) */
#define KF_LONG         0x80                    /* (A kind's form: long and sturdy, a haft, a pole, a staff: it serves
                                                **   as a crutch, held: slice 4.1b's) */
#define KF_BACK         0x20                    /* (A kind's form: worn on the back, a backpack: taken off to reach
                                                **   into, and put on again after, HOUSE.md's reach, 2026-10-26; any
                                                **   other worn thing is open to the hand: a quiver, a bandolier) */
#define KF_TENS         0x10                    /* (A kind's form: a lot whose weight and bulk are by the ten, the
                                                **   missiles', a tenth of a pound each: arrows, bolts, sling stones,
                                                **   the outfits', 2026-10-27; with KF_SMALL, coins, its weight by the
                                                **   64, in drams: a silver penny a dram, a gold crown an ounce,
                                                **   Treasure 2's, 2026-10-28) */
#define LOT_WT(f, w)    ((f) & KF_SMALL ? ((w) + 32) >> 6 : ((w) + 9) / 10)    /* (A lot's weight by the ten, w its
                                                **   kind's times its count, form f: quarter pounds) */
enum { FM_NONE, FM_SWORD, FM_KNIFE, FM_ARROW, FM_BOLT, FM_SHOT, FM_THROWN };    /* A carrier's family and its things'
                                                **   (a kind's slots, not a belt's: a scabbard's swords, a sheath's
                                                **   knives and daggers, a quiver's arrows, a bolt case's bolts, a
                                                **   pouch of sling stones, a bandolier's throwing knives) */

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
    KI_OIL, KI_PITCH,                           /* A light's fuel (slice 4.1's): a flask of oil (a lantern's, a
                                                **   torch's), a pot of pitch (a torch's) */
    KI_SCONCE,                                  /* A room's sconce, on its wall (slice 4.1's: the maker's, fixed): a
                                                **   torch in it, lit and tended where the room's kept, else cold */
    KI_AXE,                                     /* A hand axe (a kobold lord's, let fall: slice 4.1b's) */
    KI_PAPER, KI_PARCHMENT, KI_CHARCOAL, KI_PENCIL, KI_QUILL, KI_INK, KI_MAP,  /* The things to draw a map with, and
                                                **   the map (slice 4.2's: by their keys, things.c's chart_now) */
    KI_CHALK,                                   /* (Another point that draws, used up soon: HOUSE.md's things used up,
                                                **   2026-10-26) */
    KI_SCABBARD, KI_SHEATH, KI_BANDOLIER, KI_BOLT_CASE, KI_STONES, KI_BOLTS, KI_WATER,     /* The outfits' (2026-10-27):
                                                **   the carriers and missiles, */
    KI_KNIFE, KI_TABURI, KI_SHORTSWORD, KI_FALCHION, KI_LONGKNIFE, KI_BASTARD, KI_CLUB, KI_STAFF, KI_HATCHET,
    KI_SHORKANA, KI_BATTLEAXE, KI_POLEAXE, KI_SLING, KI_SHORTBOW, KI_LONGBOW, KI_HARTBOW, KI_CROSSBOW, KI_SICKLE,
    KI_NET, KI_WHIP, KI_WARHAMMER,              /*   the weapons, */
    KI_BUCKLER, KI_KSHIELD, KI_KITE, KI_TOWER,  /*   the shields, */
    KI_CLOTHES, KI_ROBE, KI_LVEST, KI_KHELM, KI_MCOWL, KI_MBYRNIE, KI_SBYRNIE, KI_GHELM, KI_MLEGS, KI_SURCOAT,
    KI_JERKIN, KI_SHROUD, KI_MASK,              /*   armour and clothes, */
    KI_CROWBAR, KI_BANDAGES, KI_LETTER, KI_RING, KI_GRAPPLE, KI_HAMMER, KI_HERBS, KI_VIAL,   /*   tools and
                                                **   belongings */
    KI_OWN_END,                                 /* (The theme's eight next: theme.h's KI_THEME, 120) */
    KI_HIGH = 128,                              /* (The kinds past the theme's, from 128: never lit nor struck with,
                                                **   NKINDS's) */
    KI_FURS = KI_HIGH, KI_BLANKET, KI_CASK,     /* The denizens' belongings (2026-10-27: a bed's, a store's) */
    KI_CROWNS, KI_BROOCH, KI_NECKLACE, KI_PENDANT, KI_GOBLET, KI_GEM, KI_TALISMAN, KI_DICE,  /*   and valuables (a
                                                **   jewel's or a plate's worth its kind's for silver: HOUSE.md's worth
                                                **   by material) */
    KI_FARTHINGS,                               /* (A silver penny cut in four, HarnWorld 23's: the user's, 2026-10-28,
                                                **   "Please keep farthings as they might be useful for denizens") */
    KI_MANKAR, KI_MANG,                         /* The Gargun's weapons (Combat 3's (G): Phase 4's peoples, 2026-10-29;
                                                **   past the theme's, the own kinds' room full: struck with as the
                                                **   best held, never named as the striker) */
    KI_COUNT };

#define KIND_WORDS      ((const char*) (CATALOGUE + NKINDS))     /* (In the kinds' bank after them: the words of
                                                **   their names, each once, a space after each, a 0 after all: the
                                                **   build's, dq0w/make.js's, for the command parser) */

/* An item */
#define NITEMS          (ITEM_BANKS * (0x2000 / sizeof (struct item)))
struct item {
    unsigned char   kind;                       /* 0: a free record; else the catalogue's (1 on) */
    unsigned char   flags;                      /* I_* */
    unsigned char   where;                      /* IW_* */
    unsigned char   sn;                         /* (On a square: its stratum; held, the hand it's in: IH_*) */
    unsigned        x, y;                       /* On a square: it; an actor's: x the actor (0 the character); in
                                                **   an item: x that item's number */
    unsigned char   qty;
    unsigned char   owner;                      /* Whose it is (0: nobody's; a faction's, an actor's) */
    unsigned char   wear;                       /* Its wear, 0 new ... 255 (a blade dug with, dulled) */
    unsigned char   hide;                       /* Concealed on purpose (I_HIDDEN): how hard to find, the steps worse
                                                **   a search for it goes (rs.h's: the method's and the concealer's
                                                **   skill's; 0 a floor's dust and corners only); a sconce's (never
                                                **   hidden): its room's light as the world last made it (SC_LIT; and
                                                **   SC_NOW, a pass's mark: a light in it lit), DUNGEON.md's light kept */
    /* Its own (no two alike: the world's, as it first comes to it, a look's; id 0 till then) */
    unsigned        id;                         /* Its number, never another's (record 0's x the last given) */
    unsigned char   quality;                    /* How well it's made: 0 crude ... 9 a master's */
    unsigned char   mat;                        /* What it's made of (MT_*: its resistances) */
    unsigned        dur, dur_max;               /* Its durability: the harm it can take before it breaks, what's
                                                **   left (its most by its quality, its material, its weight) */
    unsigned char   size;                       /* Its measure, where it matters (a portal's part's: its portal's
                                                **   opening, world.h's: feet, its width the low four bits; or 0) */
    unsigned char   thick;                      /*   and its thickness, in eighths of an inch (or 0) */
    unsigned        fuel;                       /* A light's burning left, minutes (0: burnt out; or not its own yet,
                                                **   its life decided as it's made its own), the world's as it burns
                                                **   (DUNGEON.md: light; HOUSE.md's burning); a fuel's (a flask of
                                                **   oil, a pot of pitch), what's in it, in the same minutes */
    unsigned char   shut;                       /* A thing that closes (its kind's KF_CLOSES): closed, locked (IS_*:
                                                **   decided as it's made its own, found closed, a chest locked
                                                **   sometimes; the kit's open, but what's stoppered) */
};
#define OW_HOARD        0x80                    /* (An owner: a faction's hoard, | the faction, kinds' stand-in's: a
                                                **   robber's takings put in it; else a lair's things the faction's
                                                **   number; a denizen's own, IW_DEN, its did's low byte) */
#define SC_LIT          0x01                    /* (A sconce's hide: its room lit by it, as the world last made it; */
#define SC_NOW          0x02                    /*   a light in it lit now, the world's pass's mark) */
#define IS_CLOSED       0x01                    /* Closed (its lid down, its tie tied, its stopper in): nothing put in
                                                **   or taken out, nothing in it seen */
#define IS_LOCKED       0x02                    /* Locked (picked, or forced: P_PICKLOCK, P_FORCE) */
#define IS_LOCK         0x04                    /* It has a lock (a chest's: its lid's) */
#define IS_NOCKED       0x08                    /* (A launcher's: loaded, its missile set to it, an arrow nocked, a
                                                **   crossbow spanned and its bolt in it: slice 4.7's) */
#define IS_BURST        0x08                    /* Its lock forced, burst: it won't lock again */
#define I_LIT           0x01                    /* A light, lit */
#define I_HELD          0x02                    /* In a hand (its sn which: IH_*) */
#define I_WORN          0x04                    /* Worn (a belt, a pack, armour) */
#define I_HIDDEN        0x08                    /* Hidden (found by searching) */
#define I_MAGIC         0x10                    /*   by magic (found only by magical means) */
#define I_KNOWN         0x20                    /* Known to the character (seen, or found) */
#define I_NAMED         0x40                    /* Named by the character (the UI's names: hub.h's struct tname, by
                                                **   its id; marked in what's carried, worded: "#id a sword") */
#define I_TENDED        0x80                    /* A light its keepers tend (a kept room's sconce's torch: the keepers'
                                                **   to set, slice 4.1's): it never burns down while it's tended;
                                                **   moved by the item store (taken up, put down, handled) it's tended
                                                **   no longer, and burns as any light does */
#define IH_LEFT         0x01                    /* (A held thing's hand, its sn: the character's left, slice 4.1b's; */
#define IH_RIGHT        0x02                    /*   its right; */
#define IH_BOTH         0x03                    /*   both, a two-handed thing or one held so; rs.h's DH_* the same) */
#define IH_OTHER        0x04                    /* (IO_HOLD's to: its other hand, not the one it's in, or the off hand) */
enum { IW_SQUARE, IW_ACTOR, IW_ITEM,
    IW_PORTAL,                                  /* A portal's part (a door's bar, a window's pane or bars: the maker's,
                                                **   as it stocks a chunk), in it: sn, x, y its square (a bar's, the
                                                **   square on its bar's side), seen and judged as a thing lying there
                                                **   is, not taken up (IR_FIXED) but lifted out (the maker's) */
    IW_DEN };                                   /* A denizen's own (the outfits' gear, 2026-10-27: worn, held, hung
                                                **   from its belt; what's in them IW_ITEM's), made as its lair's
                                                **   placed (I_KIT's): sn, x, y its lair's home square (filed with
                                                **   that chunk, as a thing lying there is, but not seen there; its
                                                **   chunk kept in the store while it's an actor), owner and hide its
                                                **   number, the denizens' id (its low byte, its high: the hub's
                                                **   actor's did); held (I_HELD), its hand by its kind (two-handed
                                                **   both, a shield or a light the left, else the right); it dead, on
                                                **   its body (IW_ITEM), what it held beside it */

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
    unsigned char   sel, osel;                  /* Which of several of that name, the thing's and the other's (the
                                                **   command's: struct cmd's SL_*, its n times 16; SL_THE none said,
                                                **   the one most to hand; the world's to choose, things.c's) */
    unsigned char   count;                      /* How many (the command's: 0 one, unsaid; CT_ALL, CT_HALF): of
                                                **   several alike, or of a lot (its qty) */
};
#define CT_ALL          0xFF
#define CT_HALF         0xFE
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
    IO_HOLD,                                    /* Taken into a hand (two at most; a two-handed thing both): to the
                                                **   hand wanted (IH_*; 0, the one that suits it: a weapon or a tool
                                                **   the main hand, a shield or a light the other, the free one; one
                                                **   held already moved to it) */
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
#define IO_FILL         (IO_BAR + 6)            /* A light renewed (name: a lantern filled, a torch dipped) from a fuel
                                                **   (into: a flask of oil, a pot of pitch; "": the first carried that
                                                **   it takes), as much as it has room for: the things' words of it in
                                                **   things_text, the time it took back in x (tenths); IR_NO refused,
                                                **   the words why.  A timed work (HOUSE.md's long tasks), as IO_SHUT's
                                                **   picking and forcing are: begun (no words, x its time, qty its long
                                                **   task's kind, LT_FILL, item its thing's kind: its first words the
                                                **   UI's), and done (to | IO_DONE: as its time's up, the UI's again) */
#define IO_SHUT         (IO_BAR + 7)            /* A thing worked that closes (name, carried or within reach: a chest,
                                                **   a sack, a flask), as to says (PA_OPEN, PA_CLOSE, PA_UNLOCK: its
                                                **   lock picked, the referee's P_PICKLOCK; PA_FORCE: forced, P_FORCE;
                                                **   PA_LOCK): its words in things_text, its time in x (tenths); IR_NONE
                                                **   no such thing (the UI's door work then), IR_NO refused (the words
                                                **   why).  Its lock picked, it forced, a timed work (HOUSE.md's long
                                                **   tasks: the user's, 2026-10-22): begun (the means asked, no words,
                                                **   x its time, qty its long task's kind, LT_PICK, LT_FORCE, item its
                                                **   thing's kind: the UI's to word and wait as a long task), then its
                                                **   test made and its outcome told as it's done (to | IO_DONE: the
                                                **   UI's again as its time's up, not stopped) */
#define IO_DONE         0x80                    /* (IO_HIDE's too: a thing hidden a minute's work, begun, x its time,
                                                **   qty LT_HIDE, item its kind; done, its hiding in qty) */
#define IO_INSIDE       (IO_BAR + 8)            /* What's in a thing, or on it (name: carried, or seen within reach; a
                                                **   well: its shaft), in words into things_text (shut: "The chest is
                                                **   closed."); to 1: as it's looked at ("look at": the thing, then
                                                **   what's in it) */
#define IO_LOOK         (IO_BAR + 9)            /* A thing (carried, or seen: the one most to hand, of several) looked at
                                                **   as anyone sees it, no test (to: LM_*), in words into things_text:
                                                **   IR_NONE no such thing; IR_NOT_HERE (its weight felt, it gone over for
                                                **   traps: out of reach) where it lies in x, y (2026-10-22) */
#define IO_SEEK         (IO_BAR + 10)           /* A search's thing (2026-10-22: name, to its form, SF_*): for one
                                                **   (SF_FOR: the time to find it back in x, tenths, 0 a whole search's;
                                                **   the world's tests made now, told as it ends), on or in one (SF_ON:
                                                **   carried, or within reach: IR_NONE, none; IR_NOT_HERE, seen out of
                                                **   reach, where it lies in x, y), the name kept for IO_SEARCH's */
#define IO_HANDS        (IO_BAR + 11)           /* The kinds of what the character holds (a blow with nothing named:
                                                **   what's in its hands strikes, the user's, 2026-10-22): x the
                                                **   first's, y the second's (0 none) */
#define IO_SWAP         (IO_BAR + 12)           /* What's in the character's hands changed over, left to right and right
                                                **   to left (slice 4.1b's: "swap hands"), worded into things_text, its
                                                **   time in x (tenths); IR_NO refused, the words why */
#define IO_GEAR         (IO_BAR + 13)           /* A denizen's things as the character makes them out (the user's,
                                                **   2026-10-27: a look general, examining more, assessing their
                                                **   quality; the eyes and the distance setting how much): name the
                                                **   actor named (its name, or a word of it: the nearest the character
                                                **   sees), to its level (1 examined: what it holds, wears and has at
                                                **   its belt; 2 assessed: each one's make and condition too) | the
                                                **   eyes' sight times 16 (the senses' eye, 0-9); its words in
                                                **   things_text, IR_NONE none (2026-10-27) */
#define IO_USE          (IO_BAR + 14)           /* A thing the character carries used up by its key (name: the
                                                **   catalogue's tool key, "bandage": a dressing's, the UI's first aid,
                                                **   slice 4.3's): a lot one less, its last gone, told; IR_NONE none */
enum { LM_AT, LM_CLOSE, LM_COND, LM_WEIGHT, LM_LOCK, LM_TRAPS };   /* (IO_LOOK's: a look at it; a closer look; its
                                                **   condition; its weight; its lock; gone over for traps) */
#define IO_PUZZLE       (IO_BAR + 4)           /* A command of the theme's puzzle's (theme.h), judged by its rules
                                                **   (the world's): to its verb (TR_*), name the thing (into the other:
                                                **   combine name with into, put name on into; read name; use name on
                                                **   the way out); each found to hand (IO_FIND's), the rule for their
                                                **   kinds looked for, and done (the things made one, IO_COMBINE's;
                                                **   put, IO_PUT's; the way out opened, the hub's way_open): result
                                                **   PZ_*, x the rule done; the things' words of it in things_text
                                                **   (a thing not found, put: "" none) */
enum { PZ_DONE, PZ_NONE, PZ_NO, PZ_FAR };       /* (IO_PUZZLE's: a rule done; none for those kinds; not done, the
                                                **   things' words why; the way out out of reach) */
enum { IR_OK, IR_NONE, IR_FULL, IR_HEAVY, IR_NOT_HERE, IR_NO, IR_FIXED, IR_FUMBLED };    /* (IR_NO: refused, the
                                                **   things' words why; IR_FIXED: a portal's part, in it; IR_FUMBLED:
                                                **   IO_GET's, I_TAKE's, engaged, a grope failed: the round wasted,
                                                **   the things' words) */

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
#define ST_IN           0xFD                    /* (In the thing listed before it, on it: a sconce's torch, cold; */
#define ST_LIT          0xFC                    /*   lit and tended, a kept room's: DUNGEON.md, light kept) */
struct stock {
    unsigned        seq;                        /* The chunks the maker's stocked, counted, as it stocked this one */
    unsigned char   n;
    struct sthing   e[NSTOCK];
};

#endif
