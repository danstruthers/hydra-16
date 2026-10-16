/*
** kinds.c - (dq0p) the catalogue (items.h's struct kind: a stand-in for the theme's data, till themes are data): not
** in the task's RAM but its overlay's, dq0p.ovl, which the things read as they start into the hub's catalogue bank
** (CAT_BANK), where the kinds are at $8000 as they're linked; a kind's record copied out as it's wanted (things.c).
** Its order is items.h's KI_* (the maker puts things down by those).  After it, the things' words (things.h's SA_*),
** in the bank too, each copied out as it's said.
*/

#pragma rodata-name ("OV1RODATA")

#include "../dq0/hub.h"
#include "things.h"

#define SS      (KF_SOLID | KF_SMALL)           /* (What most containers keep: things whole, and small ones) */

/* The catalogue: name, flags, weight (quarter pounds), bulk (pints), its room (pints, pounds), slots, light, size,
** tool's key, material, the craft that knows it, its worth (pennies); what it keeps (KF_*: a surface KF_ON), its own
** form (0 a thing whole) */
const struct kind kinds[KI_COUNT] = {
    { "", 0 },
    { "torch", K_LIGHT | K_HAND, 4, 2, 0, 0, 0, 4, 3, "torch", MT_WOOD, AP_WOOD, 1 },
    { "backpack", K_CONTAINER | K_WEAR, 8, 20, 32, 40, 0, 0, 5, "", MT_LEATHER, AP_HIDE, 24, SS },
    { "belt", K_BELT | K_WEAR, 2, 1, 0, 0, 4, 0, 2, "", MT_LEATHER, AP_HIDE, 6 },
    { "pouch", K_CONTAINER | K_HANG, 1, 1, 2, 4, 0, 0, 1, "", MT_LEATHER, AP_HIDE, 3, SS },
    { "dagger", K_HAND | K_HANG, 4, 1, 0, 0, 0, 0, 2, "knife", MT_IRON, AP_WEAPON, 60 },
    { "waterskin", K_CONTAINER | K_HANG, 4, 4, 4, 5, 0, 0, 2, "", MT_LEATHER, AP_HIDE, 8, KF_LIQUID },
    { "rations", K_PLURAL, 4, 2, 0, 0, 0, 0, 2, "", MT_OTHER, AP_FOOD, 4 },
    { "tinderbox", 0, 1, 1, 0, 0, 0, 0, 1, "tinder", MT_IRON, AP_METAL, 6 },
    { "pick", K_HAND | K_TWO, 24, 6, 0, 0, 0, 0, 4, "pick", MT_IRON, AP_METAL, 48 },
    { "shovel", K_HAND | K_TWO, 20, 8, 0, 0, 0, 0, 4, "shovel", MT_IRON, AP_METAL, 36 },
    { "lockpicks", 0, 1, 1, 0, 0, 0, 0, 1, "picks", MT_IRON, AP_METAL, 40 },
    { "chisel", K_HAND, 2, 1, 0, 0, 0, 0, 1, "chisel", MT_STEEL, AP_METAL, 12 },
    { "rope", 0, 40, 8, 0, 0, 0, 0, 4, "rope", MT_CLOTH, AP_CLOTH, 12 },
    { "bones", K_PLURAL, 8, 4, 0, 0, 0, 0, 4, "", MT_BONE, AP_BONE, 0 },
    { "pennies", K_PLURAL, 0, 0, 0, 0, 0, 0, 1, "", MT_BRONZE, AP_METAL, 1, 0, KF_SMALL },
    { "iron key", 0, 1, 0, 0, 0, 0, 0, 1, "key", MT_IRON, AP_METAL, 4 },
    { "candle", K_LIGHT | K_HAND, 1, 1, 0, 0, 0, 2, 1, "", MT_OTHER, AP_FOOD, 1 },
    { "lantern", K_LIGHT | K_HAND | K_HANG, 8, 4, 0, 0, 0, 6, 3, "", MT_IRON, AP_METAL, 30 },
    { "sack", K_CONTAINER, 2, 2, 24, 30, 0, 0, 3, "", MT_CLOTH, AP_CLOTH, 3, SS },
    { "chest", K_CONTAINER, 120, 80, 60, 100, 0, 0, 7, "", MT_WOOD, AP_WOOD, 80, SS },
    { "skull", 0, 4, 2, 0, 0, 0, 0, 3, "", MT_BONE, AP_BONE, 0 },
    { "broken sword", 0, 12, 2, 0, 0, 0, 0, 4, "blade", MT_IRON, AP_WEAPON, 10 },
    { "cloak", K_WEAR, 8, 6, 0, 0, 0, 0, 4, "", MT_CLOTH, AP_CLOTH, 24 },
    { "potsherds", K_PLURAL, 2, 1, 0, 0, 0, 0, 2, "", MT_CLAY, AP_POTTERY, 0 },
    { "sword", K_HAND | K_HANG, 12, 3, 0, 0, 0, 0, 4, "blade", MT_IRON, AP_WEAPON, 240 },
    { "basket", K_CONTAINER, 4, 8, 16, 20, 0, 0, 3, "", MT_WOOD, AP_WOOD, 2, KF_SOLID },
    { "heap of sand", 0, 8, 1, 0, 0, 0, 0, 2, "", MT_STONE, AP_POTTERY, 0, 0, KF_SMALL },
    { "table", 0, 160, 60, 30, 100, 0, 0, 7, "", MT_WOOD, AP_WOOD, 20, KF_ON },
    { "chair", 0, 40, 20, 8, 30, 0, 0, 5, "", MT_WOOD, AP_WOOD, 8, KF_ON },
    { "quiver", K_CONTAINER | K_HANG, 2, 2, 4, 6, 0, 0, 2, "", MT_LEATHER, AP_HIDE, 4, KF_SOLID },
    { "arrows", K_PLURAL, 2, 2, 0, 0, 0, 0, 2, "", MT_WOOD, AP_WEAPON, 1 },
    { "bar", 0, 40, 12, 0, 0, 0, 0, 5, "", MT_WOOD, AP_WOOD, 4 },        /* (A portal's parts: a door's bar, */
    { "pane of glass", 0, 4, 6, 0, 0, 0, 0, 3, "", MT_GLASS, AP_GLASS, 30 },     /*   a window's glass, */
    { "iron bars", K_PLURAL, 60, 20, 0, 0, 0, 0, 5, "", MT_IRON, AP_METAL, 10 },  /*   its bars; */
    { "glass shards", K_PLURAL, 2, 1, 0, 0, 0, 0, 2, "", MT_GLASS, AP_GLASS, 0 },  /*   a pane broken) */
    { "body", 0, 240, 60, 0, 0, 0, 0, 6, "", MT_OTHER, AP_BONE, 0 },  /* (A denizen dead) */
    { "leather cap", K_WEAR, 3, 1, 0, 0, 0, 0, 2, "lcap", MT_LEATHER, AP_ARMOUR, 16 },    /* (Armour: its key the */
    { "leather tunic", K_WEAR, 35, 8, 0, 0, 0, 0, 4, "ltunic", MT_LEATHER, AP_ARMOUR, 176 },  /*   referee's piece) */
    { "gambeson", K_WEAR, 79, 16, 0, 0, 0, 0, 4, "gambson", MT_CLOTH, AP_ARMOUR, 264 },
    { "ring byrnie", K_WEAR, 70, 8, 0, 0, 0, 0, 4, "ringbyr", MT_IRON, AP_ARMOUR, 308 },
    { "mail hauberk", K_WEAR, 132, 10, 0, 0, 0, 0, 4, "mailhbk", MT_IRON, AP_ARMOUR, 990 },
    { "gauntlets", K_WEAR | K_PLURAL, 3, 1, 0, 0, 0, 0, 2, "gauntlt", MT_LEATHER, AP_ARMOUR, 16 },
    { "mace", K_HAND | K_HANG, 16, 3, 0, 0, 0, 0, 3, "mace", MT_IRON, AP_WEAPON, 84 },     /* (Weapons) */
    { "spear", K_HAND, 20, 6, 0, 0, 0, 0, 5, "spear", MT_WOOD, AP_WEAPON, 60 },
    { "round shield", K_HAND, 24, 12, 0, 0, 0, 0, 4, "shield", MT_WOOD, AP_WEAPON, 42 },
};

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
};
