/*
** kinds.c - (dq0w) the catalogue (items.h's struct kind: a stand-in for the theme's data, till themes are data): not
** in the task's RAM but its own file's, dq0w.cat (dq0w.cfg's KIN and CAT), which the world reads as it starts into the
** hub's banks: the kinds into their own (KIND_BANK, 2026-10-27: the outfits' room), where they're at $8000 as they're
** linked, the words of their names after them (dq0w/make.js's); a kind's record copied out as it's wanted (things.c;
** the senses', the UI's, the referee's too).  Its order is items.h's KI_* (the maker lists things by those).  Then,
** in the catalogue's bank (CAT_BANK, the world's), how the lights burn (burns: slice 4.1's), what's thrown how far,
** and the things' words (things.h's SA_*), each copied out as it's wanted.
*/

#pragma rodata-name ("CATRODATA")

#include "../dq0/hub.h"
#include "things.h"

#define SS      (KF_SOLID | KF_SMALL)           /* (What most containers keep: things whole, and small ones) */

/* The catalogue, in rs.h's units: name, flags, weight (quarter pounds), bulk (pints), its room (pints; the weight it
** bears, quarter pounds), slots, light (squares), how far off it's made out (squares past a speck), tool's key,
** material, the craft that knows it, its worth (pennies); what it keeps (KF_*: a surface KF_ON) and what closes it (a
** lid, a tie, a stopper: KF_LID, KF_TIE, KF_STOPPER), its own form (0 a thing whole; KF_COUNT a lot, counted: its
** weight one's) */
typedef char kinds_fit[KI_OWN_END <= KI_THEME && KI_COUNT <= NKINDS ? 1 : -1];   /* (The theme's after the catalogue's
                                                **   own, the high kinds after the theme's) */
#pragma rodata-name (push, "KINDATA")           /* (The kinds in a bank of their own: the hub's KIND_BANK) */
const struct kind kinds[NKINDS] = {          /* (Its last NTKINDS the theme's, theme.h: the UI's, as it starts) */
    { "", 0 },
    { "torch", K_LIGHT | K_HAND, 4, 2, 0, 0, 0, 4, 3, "torch", MT_WOOD, AP_WOOD, 1 },
    { "backpack", K_CONTAINER | K_WEAR, 8, 20, 32, 160, 0, 0, 5, "", MT_LEATHER, AP_HIDE, 24, SS | KF_LID, KF_BACK },
    { "belt", K_BELT | K_WEAR, 2, 1, 0, 0, 6, 0, 2, "", MT_LEATHER, AP_HIDE, 10 },     /* (Six slots: a pouch, a sheath,
                                                **   a scabbard, a waterskin, a quiver, a mace, the outfits') */
    { "pouch", K_CONTAINER | K_HANG, 1, 1, 3, 16, 0, 0, 1, "", MT_LEATHER, AP_HIDE, 9, SS | KF_TIE },
    { "dagger", K_HAND | K_HANG, 4, 1, 0, 0, FM_KNIFE, 0, 2, "dagger", MT_IRON, AP_WEAPON, 24 },
    { "waterskin", K_CONTAINER | K_HANG, 4, 4, 4, 20, 0, 0, 2, "", MT_LEATHER, AP_HIDE, 8, KF_LIQUID | KF_STOPPER },
    { "rations", K_PLURAL, 4, 2, 0, 0, 0, 0, 2, "", MT_OTHER, AP_FOOD, 2, 0, KF_COUNT },   /* (A day's: a pound) */
    { "tinderbox", 0, 1, 1, 0, 0, 0, 0, 1, "tinder", MT_IRON, AP_METAL, 6 },
    { "pick", K_HAND | K_TWO, 24, 6, 0, 0, 0, 0, 4, "pick", MT_IRON, AP_METAL, 8 },
    { "shovel", K_HAND | K_TWO, 20, 8, 0, 0, 0, 0, 4, "shovel", MT_IRON, AP_METAL, 36, 0, KF_LONG },
    { "lockpicks", K_PLURAL, 1, 1, 0, 0, 0, 0, 1, "picks", MT_IRON, AP_METAL, 40 },
    { "chisel", K_HAND, 2, 1, 0, 0, 0, 0, 1, "chisel", MT_STEEL, AP_METAL, 12 },
    { "rope", 0, 40, 8, 0, 0, 0, 0, 4, "rope", MT_CLOTH, AP_CLOTH, 12 },
    { "bones", K_PLURAL, 8, 4, 20, 200, 0, 0, 4, "", MT_BONE, AP_BONE, 0, KF_ON },   /* (What a skeleton had on it
                                                **   on them: the outfits' gear) */
    { "pennies", K_PLURAL, 0, 0, 0, 0, 0, 0, 1, "", MT_BRONZE, AP_METAL, 1, 0, KF_SMALL | KF_COUNT },
    { "iron key", 0, 1, 0, 0, 0, 0, 0, 1, "key", MT_IRON, AP_METAL, 2 },
    { "candle", K_LIGHT | K_HAND, 1, 1, 0, 0, 0, 2, 1, "", MT_TALLOW, AP_FOOD, 1 },   /* (Tallow, a fine one beeswax) */
    { "lantern", K_LIGHT | K_HAND | K_HANG, 8, 4, 0, 0, 0, 6, 3, "", MT_IRON, AP_METAL, 12 },
    { "sack", K_CONTAINER, 2, 2, 24, 120, 0, 0, 3, "", MT_CLOTH, AP_CLOTH, 1, SS | KF_TIE },
    { "chest", K_CONTAINER, 120, 80, 60, 400, 0, 0, 7, "", MT_WOOD, AP_WOOD, 80, SS | KF_LID },
    { "skull", 0, 4, 2, 0, 0, 0, 0, 3, "", MT_BONE, AP_BONE, 0 },
    { "broken sword", 0, 12, 2, 0, 0, 0, 0, 4, "blade", MT_IRON, AP_WEAPON, 10 },
    { "cloak", K_WEAR, 8, 6, 0, 0, 0, 0, 4, "", MT_CLOTH, AP_CLOTH, 24 },
    { "potsherds", K_PLURAL, 2, 1, 0, 0, 0, 0, 2, "", MT_CLAY, AP_POTTERY, 0 },
    { "sword", K_HAND | K_HANG, 12, 3, 0, 0, FM_SWORD, 0, 4, "blade", MT_IRON, AP_WEAPON, 150 },  /* (The book's
                                                **   broadsword) */
    { "basket", K_CONTAINER, 4, 8, 16, 80, 0, 0, 3, "", MT_WOOD, AP_WOOD, 2, KF_SOLID },
    { "heap of sand", 0, 8, 1, 0, 0, 0, 0, 2, "", MT_STONE, AP_POTTERY, 0, 0, KF_SMALL },
    { "table", 0, 160, 60, 30, 400, 0, 0, 7, "", MT_WOOD, AP_WOOD, 20, KF_ON },
    { "chair", 0, 40, 20, 8, 120, 0, 0, 5, "", MT_WOOD, AP_WOOD, 8, KF_ON },
    { "quiver", K_CONTAINER | K_HANG, 2, 2, 4, 24, FM_ARROW, 0, 2, "", MT_LEATHER, AP_HIDE, 8, KF_SOLID },
    { "arrows", K_PLURAL, 4, 1, 0, 0, FM_ARROW, 0, 2, "", MT_WOOD, AP_WEAPON, 1, 0, KF_COUNT | KF_TENS },  /* (A tenth
                                                **   of a pound each: Combat 3) */
    { "bar", 0, 40, 12, 0, 0, 0, 0, 5, "", MT_WOOD, AP_WOOD, 4, 0, KF_LONG },  /* (A portal's parts: a door's bar, */
    { "pane of glass", 0, 4, 6, 0, 0, 0, 0, 3, "", MT_GLASS, AP_GLASS, 30 },     /*   a window's glass, */
    { "iron bars", K_PLURAL, 60, 20, 0, 0, 0, 0, 5, "", MT_IRON, AP_METAL, 10 },  /*   its bars; */
    { "glass shards", K_PLURAL, 2, 1, 0, 0, 0, 0, 2, "", MT_GLASS, AP_GLASS, 0 },  /*   a pane broken) */
    { "body", 0, 240, 60, 60, 400, 0, 0, 6, "", MT_OTHER, AP_BONE, 0, KF_ON },  /* (A denizen dead: what it wore
                                                **   and carried on it, the outfits' gear) */
    { "leather cap", K_WEAR, 3, 1, 0, 0, 0, 0, 2, "lcap", MT_LEATHER, AP_ARMOUR, 16 },    /* (Armour: its key the */
    { "leather tunic", K_WEAR, 35, 8, 0, 0, 0, 0, 4, "ltunic", MT_LEATHER, AP_ARMOUR, 176 },  /*   referee's piece) */
    { "gambeson", K_WEAR, 79, 16, 0, 0, 0, 0, 4, "gambson", MT_CLOTH, AP_ARMOUR, 264 },
    { "ring byrnie", K_WEAR, 70, 8, 0, 0, 0, 0, 4, "ringbyr", MT_IRON, AP_ARMOUR, 308 },
    { "mail hauberk", K_WEAR, 132, 10, 0, 0, 0, 0, 4, "mailhbk", MT_IRON, AP_ARMOUR, 990 },
    { "gauntlets", K_WEAR | K_PLURAL, 3, 1, 0, 0, 0, 0, 2, "gauntlt", MT_LEATHER, AP_ARMOUR, 16 },
    { "knee boots", K_WEAR | K_PLURAL, 15, 4, 0, 0, 0, 0, 3, "boots", MT_LEATHER, AP_ARMOUR, 76 },
    { "iron helm", K_WEAR, 13, 4, 0, 0, 0, 0, 3, "helm", MT_IRON, AP_ARMOUR, 100 },
    { "sandals", K_WEAR | K_PLURAL, 3, 1, 0, 0, 0, 0, 2, "sandals", MT_LEATHER, AP_ARMOUR, 8 },
    { "shoes", K_WEAR | K_PLURAL, 5, 2, 0, 0, 0, 0, 2, "shoes", MT_LEATHER, AP_ARMOUR, 24 },
    { "hobnail boots", K_WEAR | K_PLURAL, 20, 4, 0, 0, 0, 0, 3, "hobnail", MT_LEATHER, AP_ARMOUR, 120 },
    { "mace", K_HAND | K_HANG, 16, 3, 0, 0, 0, 0, 3, "mace", MT_IRON, AP_WEAPON, 84 },     /* (Weapons) */
    { "spear", K_HAND, 20, 6, 0, 0, 0, 0, 5, "spear", MT_WOOD, AP_WEAPON, 60, 0, KF_LONG },  /* (Long and sturdy:
                                                **   a crutch, the spear, the shovel, a door's bar) */
    { "round shield", K_HAND | K_WEAR, 24, 12, 0, 0, 0, 0, 4, "shield", MT_WOOD, AP_WEAPON, 42 },  /* (Worn: slung on
                                                **   the back, taken in hand to fight) */
    { "iron pot", K_CONTAINER, 32, 8, 6, 48, 0, 0, 3, "", MT_IRON, AP_METAL, 10, SS | KF_LIQUID | KF_LID }, /* (A room's use's
                                                **   leavings: the stock's, dq0m/uses.c) */
    { "heap of straw", 0, 4, 8, 0, 0, 0, 0, 3, "", MT_OTHER, AP_WOOD, 0 },
    { "candle stubs", K_LIGHT | K_HAND | K_PLURAL, 1, 1, 0, 0, 0, 1, 1, "", MT_TALLOW, AP_FOOD, 0 },  /* (A light
                                                **   of a few minutes: slice 4.1's) */
    { "well", 0, 255, 255, 0, 0, 0, 0, 8, "", MT_STONE, AP_STONE, 0, KF_FIXED },   /* (Its stone curb: never taken
                                                **   up; its water the water's work's, to come) */
    { "flask of oil", K_HAND, 4, 1, 0, 0, 0, 0, 1, "", MT_CLAY, AP_POTTERY, 3, KF_STOPPER },   /* (A light's fuel: a lantern's
                                                **   fill, a torch's; slice 4.1's, the stopped branch's) */
    { "pot of pitch", K_HAND, 6, 2, 0, 0, 0, 0, 2, "", MT_CLAY, AP_POTTERY, 2, KF_LID },   /* (A torch's) */
    { "sconce", 0, 6, 2, 3, 8, 0, 0, 12, "", MT_IRON, AP_METAL, 4, KF_ON | KF_FIXED },  /* (On a room's wall, fixed: a
                                                **   torch in it, the maker's: DUNGEON.md, light kept) */
    { "axe", K_HAND | K_HANG, 12, 3, 0, 0, 0, 0, 3, "axe", MT_IRON, AP_WEAPON, 72 },   /* (The book's handaxe: a
                                                **   kobold lord's, let fall; slice 4.1b's) */
    { "paper", K_HAND | K_PLURAL, 0, 1, 0, 0, 0, 0, 2, "sheet", MT_CLOTH, AP_CLOTH, 2 },     /* (The things to draw a
                                                **   map with, by their keys: a sheet to draw on, */
    { "parchment", K_HAND | K_PLURAL, 1, 1, 0, 0, 0, 0, 2, "sheet", MT_LEATHER, AP_HIDE, 4 },
    { "charcoal", K_HAND | K_PLURAL, 0, 1, 0, 0, 0, 0, 1, "draw", MT_WOOD, AP_WOOD, 1 },    /*   a point that draws, */
    { "pencil", K_HAND, 0, 1, 0, 0, 0, 0, 1, "draw", MT_WOOD, AP_WOOD, 3 },
    { "quill", K_HAND, 0, 1, 0, 0, 0, 0, 1, "quill", MT_BONE, AP_BONE, 1 },     /*   or a quill and its ink; */
    { "horn of ink", K_HAND, 1, 1, 0, 0, 0, 0, 1, "ink", MT_BONE, AP_BONE, 6 },
    { "map", K_HAND, 0, 1, 0, 0, 0, 0, 2, "map", MT_CLOTH, AP_CLOTH, 2 },          /*   a sheet drawn on, the map:
                                                **   DUNGEON.md, the map kept, slice 4.2's) */
    { "chalk", K_HAND | K_PLURAL, 0, 1, 0, 0, 0, 0, 1, "draw", MT_STONE, AP_STONE, 1 },   /* (A point that draws, soon
                                                **   used up: HOUSE.md's things used up, 2026-10-26) */
    /* (2026-10-27, the outfits: the carriers, each its family's alone, and the missiles, by the ten; then the weapons
    ** (the referee's by their keys: Combat 3), the shields, the armour and clothes (Combat 4), the tools; prices the
    ** book's, else the house's: outfits' kinds.md) */
    { "scabbard", K_CONTAINER | K_HANG, 4, 3, 4, 24, FM_SWORD, 0, 2, "", MT_LEATHER, AP_HIDE, 48, KF_SOLID },
    { "sheath", K_CONTAINER | K_HANG, 1, 1, 1, 8, FM_KNIFE, 0, 1, "", MT_LEATHER, AP_HIDE, 6, KF_SOLID },
    { "bandolier", K_CONTAINER | K_WEAR, 2, 1, 5, 20, FM_THROWN, 0, 2, "", MT_LEATHER, AP_HIDE, 12, KF_SOLID },
    { "bolt case", K_CONTAINER | K_HANG, 2, 2, 4, 24, FM_BOLT, 0, 2, "", MT_LEATHER, AP_HIDE, 8, KF_SOLID },
    { "sling stones", K_PLURAL, 4, 1, 0, 0, FM_SHOT, 0, 1, "", MT_STONE, AP_STONE, 0, 0, KF_COUNT | KF_TENS },
    { "bolts", K_PLURAL, 4, 1, 0, 0, FM_BOLT, 0, 2, "", MT_WOOD, AP_WEAPON, 1, 0, KF_COUNT | KF_TENS },
    { "water", K_PLURAL, 5, 1, 0, 0, 0, 0, 1, "", MT_OTHER, AP_FOOD, 0, 0, KF_LIQUID | KF_COUNT },   /* (Pints) */
    { "knife", K_HAND | K_HANG, 4, 1, 0, 0, FM_KNIFE, 0, 1, "knife", MT_IRON, AP_WEAPON, 6 },
    { "taburi", K_HAND | K_HANG | K_PLURAL, 4, 1, 0, 0, FM_THROWN, 0, 2, "taburi", MT_IRON, AP_WEAPON, 20, 0, KF_COUNT },
    { "shortsword", K_HAND | K_HANG, 8, 2, 0, 0, FM_SWORD, 0, 3, "shortsw", MT_IRON, AP_WEAPON, 90 },
    { "falchion", K_HAND | K_HANG, 16, 3, 0, 0, FM_SWORD, 0, 4, "falchin", MT_IRON, AP_WEAPON, 120 },
    { "longknife", K_HAND | K_HANG, 4, 2, 0, 0, FM_SWORD, 0, 3, "longknf", MT_IRON, AP_WEAPON, 96 },
    { "bastard sword", K_HAND | K_HANG, 20, 4, 0, 0, FM_SWORD, 0, 4, "bastard", MT_IRON, AP_WEAPON, 180 },
    { "club", K_HAND | K_HANG, 12, 3, 0, 0, 0, 0, 3, "club", MT_WOOD, AP_WEAPON, 12 },
    { "staff", K_HAND, 16, 8, 0, 0, 0, 0, 5, "staff", MT_WOOD, AP_WEAPON, 36, 0, KF_LONG },
    { "hatchet", K_HAND | K_HANG, 8, 2, 0, 0, 0, 0, 3, "hatchet", MT_IRON, AP_WEAPON, 12 },
    { "shorkana", K_HAND | K_HANG, 8, 2, 0, 0, 0, 0, 3, "shorkan", MT_IRON, AP_WEAPON, 48 },
    { "battleaxe", K_HAND, 24, 6, 0, 0, 0, 0, 4, "battlax", MT_IRON, AP_WEAPON, 100, 0, KF_LONG },
    { "poleaxe", K_HAND | K_TWO, 32, 12, 0, 0, 0, 0, 6, "poleaxe", MT_IRON, AP_WEAPON, 96, 0, KF_LONG },
    { "sling", K_HAND | K_HANG, 1, 1, 0, 0, 0, 0, 1, "sling", MT_LEATHER, AP_HIDE, 6 },
    { "shortbow", K_HAND | K_TWO | K_WEAR, 8, 6, 0, 0, 0, 0, 4, "shortbw", MT_WOOD, AP_WEAPON, 24 },  /* (Worn
                                                **   slung, both hands to shoot) */
    { "longbow", K_HAND | K_TWO | K_WEAR, 12, 10, 0, 0, 0, 0, 5, "longbow", MT_WOOD, AP_WEAPON, 36 },
    { "hartbow", K_HAND | K_TWO | K_WEAR, 8, 6, 0, 0, 0, 0, 4, "hartbow", MT_WOOD, AP_WEAPON, 96 },
    { "crossbow", K_HAND | K_TWO | K_WEAR, 20, 8, 0, 0, 0, 0, 4, "crossbw", MT_WOOD, AP_WEAPON, 60 },
    { "sickle", K_HAND | K_HANG, 4, 2, 0, 0, 0, 0, 2, "sickle", MT_IRON, AP_WEAPON, 10 },
    { "net", K_HAND, 16, 6, 0, 0, 0, 0, 3, "net", MT_CLOTH, AP_CLOTH, 48 },
    { "whip", K_HAND | K_HANG, 8, 2, 0, 0, 0, 0, 3, "whip", MT_LEATHER, AP_HIDE, 12 },
    { "warhammer", K_HAND | K_HANG, 20, 3, 0, 0, 0, 0, 3, "warhamm", MT_IRON, AP_WEAPON, 90 },
    { "buckler", K_HAND | K_HANG | K_WEAR, 12, 4, 0, 0, 0, 0, 3, "buckler", MT_WOOD, AP_WEAPON, 24 },  /* (Shields:
                                                **   worn slung, taken in hand to fight) */
    { "knight shield", K_HAND | K_WEAR, 20, 10, 0, 0, 0, 0, 4, "kshield", MT_WOOD, AP_WEAPON, 60 },
    { "kite shield", K_HAND | K_WEAR, 28, 14, 0, 0, 0, 0, 5, "kite", MT_WOOD, AP_WEAPON, 72 },
    { "tower shield", K_HAND | K_WEAR, 32, 18, 0, 0, 0, 0, 5, "tower", MT_WOOD, AP_WEAPON, 96 },
    { "clothes", K_WEAR | K_PLURAL, 35, 8, 0, 0, 0, 0, 3, "clothes", MT_CLOTH, AP_CLOTH, 176 },  /* (A tunic and
                                                **   leggings) */
    { "robe", K_WEAR, 32, 8, 0, 0, 0, 0, 4, "robe", MT_CLOTH, AP_CLOTH, 158 },
    { "leather vest", K_WEAR, 22, 6, 0, 0, 0, 0, 3, "lvest", MT_LEATHER, AP_ARMOUR, 112 },
    { "kurbul helm", K_WEAR, 4, 2, 0, 0, 0, 0, 2, "khelm", MT_LEATHER, AP_ARMOUR, 20 },
    { "mail cowl", K_WEAR, 16, 2, 0, 0, 0, 0, 2, "mcowl", MT_IRON, AP_ARMOUR, 120 },
    { "mail byrnie", K_WEAR, 88, 8, 0, 0, 0, 0, 4, "mbyrnie", MT_IRON, AP_ARMOUR, 660 },
    { "scale byrnie", K_WEAR, 123, 10, 0, 0, 0, 0, 4, "sbyrnie", MT_IRON, AP_ARMOUR, 440 },
    { "great helm", K_WEAR, 35, 6, 0, 0, 0, 0, 3, "ghelm", MT_STEEL, AP_ARMOUR, 275 },
    { "mail leggings", K_WEAR | K_PLURAL, 88, 8, 0, 0, 0, 0, 3, "mlegs", MT_IRON, AP_ARMOUR, 660 },
    { "surcoat", K_WEAR, 21, 6, 0, 0, 0, 0, 4, "surcoat", MT_CLOTH, AP_CLOTH, 106 },
    { "hide jerkin", K_WEAR, 40, 10, 0, 0, 0, 0, 3, "hide", MT_LEATHER, AP_HIDE, 60 },     /* (Untanned hide and fur:
                                                **   quilt's protection, the house's reading of Combat 4) */
    { "shroud", K_WEAR, 6, 3, 0, 0, 0, 0, 3, "shroud", MT_CLOTH, AP_CLOTH, 4 },            /* (The dead's linen) */
    { "mask", K_WEAR, 4, 2, 0, 0, 0, 0, 2, "", MT_WOOD, AP_WOOD, 6 },
    { "crowbar", K_HAND, 20, 3, 0, 0, 0, 0, 3, "crowbar", MT_IRON, AP_METAL, 9 },
    { "bandages", K_PLURAL, 1, 1, 0, 0, 0, 0, 1, "bandage", MT_CLOTH, AP_CLOTH, 1, 0, KF_COUNT },
    { "letter", K_HAND, 0, 0, 0, 0, 0, 0, 2, "", MT_CLOTH, AP_BOOK, 0 },   /* (Credentials: guild papers, a letter of
                                                **   introduction, Character 17) */
    { "ring", K_WEAR, 0, 0, 0, 0, 0, 0, 1, "", MT_SILVER, AP_JEWEL, 12 },
    { "grapple", K_HAND, 12, 2, 0, 0, 0, 0, 3, "grapple", MT_IRON, AP_METAL, 10 },
    { "hammer", K_HAND | K_HANG, 8, 1, 0, 0, 0, 0, 2, "hammer", MT_IRON, AP_METAL, 6 },
    { "herbs", K_PLURAL, 1, 1, 0, 0, 0, 0, 1, "herbs", MT_OTHER, AP_FOOD, 6, 0, KF_COUNT },   /* (Ounces) */
    { "vial", K_CONTAINER | K_HAND, 2, 1, 1, 5, 0, 0, 1, "", MT_GLASS, AP_GLASS, 6, KF_LIQUID | KF_STOPPER },
    { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 },    /* (The theme's eight, the
                                                **   UI's as it starts: KI_THEME's) */
    /* (KI_HIGH on, 2026-10-27: the denizens' belongings and valuables, never lit nor struck with) */
    { "furs", K_PLURAL, 40, 20, 0, 0, 0, 0, 4, "", MT_LEATHER, AP_HIDE, 30 },    /* (A lord's bed: a heap of them) */
    { "blanket", 0, 12, 6, 0, 0, 0, 0, 4, "", MT_CLOTH, AP_CLOTH, 4 },
    { "cask", K_CONTAINER, 40, 40, 32, 200, 0, 0, 5, "", MT_WOOD, AP_WOOD, 12, KF_LIQUID | KF_STOPPER },   /* (Four
                                                **   gallons: a band's water) */
    { "crowns", K_PLURAL, 0, 0, 0, 0, 0, 0, 1, "", MT_GOLD, AP_METAL, 100, 0, KF_SMALL | KF_COUNT },   /* (The
                                                **   Khuzdul's gold coin: a hundred pennies, the house's) */
    { "brooch", K_WEAR, 0, 0, 0, 0, 0, 0, 1, "", MT_SILVER, AP_JEWEL, 30 },   /* (A jewel's or a plate's worth
                                                **   its kind's in silver: by its material, HOUSE.md's) */
    { "necklace", K_WEAR, 0, 0, 0, 0, 0, 0, 1, "", MT_SILVER, AP_JEWEL, 60 },
    { "pendant", K_WEAR, 0, 0, 0, 0, 0, 0, 1, "", MT_SILVER, AP_JEWEL, 24 },
    { "goblet", K_HAND, 2, 1, 0, 0, 0, 0, 2, "", MT_SILVER, AP_JEWEL, 80 },
    { "gem", 0, 0, 0, 0, 0, 0, 0, 1, "", MT_STONE, AP_JEWEL, 120 },
    { "talisman", K_WEAR, 0, 0, 0, 0, 0, 0, 1, "", MT_BONE, AP_BONE, 1 },    /* (A tribal charm: a tooth on a
                                                **   thong, a bright pebble) */
    { "dice", K_PLURAL, 0, 0, 0, 0, 0, 0, 1, "", MT_BONE, AP_BONE, 2 },
};
#pragma rodata-name (pop)

/* How each light burns (DUNGEON.md: light; HOUSE.md's burning, slice 4.1's): its kind; how long a whole one burns
** at middling make, in tens of minutes (a torch an hour, the house rule's; a tallow candle three-quarters of an inch
** thick four hours, its make, its thickness and its wax more or less; a lantern's fill of oil, a pint, six hours; a
** handful of candle stubs half an hour); and the fuels that renew it (FU_*: none, it burns away).  Then each fuel's:
** its kind, how much a whole one holds, in the same tens of minutes of a light's burning (a flask of oil a lantern's
** fill), and what it is.  A light that isn't listed (a glow, the theme's) never burns down.  And the things used up as
** they're drawn with (FU_DRAW: HOUSE.md's things used up, the user's, 2026-10-23, "A pot of ink might last a really long
** time, as might a pencil, but a chunk of charcoal or chalk would be used up faster"), in the same tens of minutes, of
** drawing: a stick of charcoal four hours, chalk three, a pencil twenty, a horn of ink ten, a quill thirty */
const struct burn burns[NBURNS] = {
    { KI_TORCH, 6, FU_OIL | FU_PITCH }, { KI_CANDLE, 24, 0 }, { KI_LANTERN, 36, FU_OIL }, { KI_STUBS, 3, 0 },
    { KI_OIL, 36, FU_OIL }, { KI_PITCH, 24, FU_PITCH },
    { KI_CHARCOAL, 24, FU_DRAW }, { KI_CHALK, 18, FU_DRAW }, { KI_PENCIL, 120, FU_DRAW }, { KI_INK, 60, FU_DRAW },
    { KI_QUILL, 180, FU_DRAW }
};

/* What breaks as it's used (slice 4.7's missiles to use, the user's, 2026-10-23: "arrows can break"), its kind and its
** chance in a hundred, shot or thrown; a lot's one used up as a drawing thing is (dq0w.c's s_use_up, part 7's) */
const struct snap snaps[NSNAPS] = {
    { KI_ARROWS, 25 }
};

/* What a kind of thing affords (the plan's affordances, in the kinds' data): thrown, its reach (the short band of
** HarnMaster's missile ranges, in squares: 0, not a thing to throw; things.c's s_throw_at), by KI_* (the theme's own
** kinds none yet) */
const unsigned char throws[KI_COUNT] = {
    0, 2, 0, 0, 2, 2, 1, 2, 2, 1,               /* (none), torch, backpack, belt, pouch, dagger, waterskin, rations,
                                                **   tinderbox, pick */
    1, 2, 2, 1, 2, 2, 2, 2, 1, 0,               /* shovel, lockpicks, chisel, rope, bones, pennies, key, candle, lantern,
                                                **   sack */
    0, 2, 1, 0, 2, 1, 1, 0, 0, 0,               /* chest, skull, broken sword, cloak, potsherds, sword, basket, sand,
                                                **   table, chair */
    1, 1, 1, 1, 0, 2, 0,                        /* quiver, arrows, bar, pane, bars, shards, body */
    1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,            /* leather cap ... hobnail boots: armour (a helm, boots, a cap) */
    1, 4, 1,                                    /* mace, spear (a javelin's), shield */
    1, 0, 2, 0,                                 /* pot, straw, stubs, well */
    2, 1,                                       /* flask of oil, pot of pitch */
    0, 2,                                       /* sconce, axe */
    0, 0, 2, 2, 1, 2, 0,                        /* paper, parchment, charcoal, pencil, quill, horn of ink, map */
    2,                                          /* chalk */
    1, 1, 1, 1, 2, 1, 0,                        /* scabbard, sheath, bandolier, bolt case, sling stones, bolts, water */
    2, 4, 1, 1, 1, 1, 2, 1, 2, 3,               /* knife, taburi (thrown: Combat 16), shortsword, falchion, longknife,
                                                **   bastard sword, club, staff, hatchet, shorkana (thrown) */
    1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1,            /* battleaxe, poleaxe, sling, shortbow, longbow, hartbow, crossbow,
                                                **   sickle, net, whip, warhammer */
    1, 0, 0, 0,                                 /* buckler, knight shield, kite shield, tower shield */
    1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1,      /* clothes ... mask: armour and clothes */
    1, 2, 1, 2, 2, 2, 2, 2,                     /* crowbar, bandages, letter, ring, grapple, hammer, herbs, vial */
    0, 0, 0, 0, 0, 0, 0, 0,                     /* (the theme's eight) */
    1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2             /* furs, blanket, cask, crowns, brooch, necklace, pendant, goblet, gem,
                                                **   talisman, dice */
};
typedef char throws_fit[sizeof throws == KI_COUNT ? 1 : -1];

/* The things' words (things.h's SA_*): % the thing's name, # the other's (the pack's, the belt's, a container's) */
const char* const said[SA_COUNT] = {
    "You take the % in your hand.",
    "You take the % in both hands.",
    "You hold the % already.",
    "Your hands are full: stow something, or put it down.",
    "You stow the % in your #.",
    "You have no pack to stow it in.",
    "The % is there already.",
    "You put the % in the #.",
    "You put the % on the #.",
    "You hang the % from your #.",
    "The % won't hang from a belt.",
    "Your # has no room left to hang the % from.",
    "You have no belt to hang it from.",
    "The # won't hold anything.",
    "The # won't keep the %: it would run out.",
    "The % won't go in the #.",
    "The % won't go inside itself.",
    "Put the % out first.",
    "There's no room there for the %.",
    "The # won't bear the % as well.",
    "You take the % from the #.",
    "You find no % in the #.",
    "You put on the %.",
    "You wear the % already.",
    "You wear a % already.",
    "You can't wear the %.",
    "You take off the %.",
    "You aren't wearing any %.",
    "You light the %.",
    "The % is lit already.",
    "You have nothing to light it with: a tinderbox, or a flame.",
    "The % won't light.",
    "You put out the %.",
    "The % isn't lit.",
    "You have nothing to light.",
    "You have nothing lit.",
    "You have no % to hand.",
    "You see no # here.",
    "The % is out of reach: go over to it first.",
    "Put the % in what, or on what?",
    "Which thing? Name it (hold the dagger, put on the cloak ...).",
    " It's in your hand.",
    " It's in your #.",
    "The % is fixed where it stands.",
    "The % is burnt out.",
    "The % is out of oil.",
    "The # is closed.",                        /* (A lid, a tie, a stopper: opened first, the user's) */
};
