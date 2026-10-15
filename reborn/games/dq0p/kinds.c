/*
** kinds.c - (dq0p) the catalogue (items.h's struct kind: a stand-in for the theme's data, till themes are data): not
** in the task's RAM but its overlay's, dq0p.ovl, which the things read as they start into the hub's catalogue bank
** (CAT_BANK), where the kinds are at $8000 as they're linked; a kind's record copied out as it's wanted (things.c).
** Its order is items.h's KI_* (the maker puts things down by those).
*/

#pragma rodata-name ("OV1RODATA")

#include "../dq0/hub.h"

/* The catalogue: name, flags, weight (quarter pounds), bulk (pints), its room (pints, pounds), slots, light, size,
** tool's key, material, the craft that knows it, its worth (pennies) */
const struct kind kinds[KI_COUNT] = {
    { "", 0 },
    { "torch", K_LIGHT | K_HAND, 4, 2, 0, 0, 0, 4, 3, "", MT_WOOD, AP_WOOD, 1 },
    { "backpack", K_CONTAINER | K_WEAR, 8, 20, 32, 40, 0, 0, 5, "", MT_LEATHER, AP_HIDE, 24 },
    { "belt", K_BELT | K_WEAR, 2, 1, 0, 0, 4, 0, 2, "", MT_LEATHER, AP_HIDE, 6 },
    { "pouch", K_CONTAINER | K_HANG, 1, 1, 2, 4, 0, 0, 1, "", MT_LEATHER, AP_HIDE, 3 },
    { "dagger", K_HAND | K_HANG, 4, 1, 0, 0, 0, 0, 2, "knife", MT_IRON, AP_WEAPON, 60 },
    { "waterskin", K_HANG, 4, 4, 0, 0, 0, 0, 2, "", MT_LEATHER, AP_HIDE, 8 },
    { "rations", K_PLURAL, 4, 2, 0, 0, 0, 0, 2, "", MT_OTHER, AP_FOOD, 4 },
    { "tinderbox", 0, 1, 1, 0, 0, 0, 0, 1, "tinder", MT_IRON, AP_METAL, 6 },
    { "pick", K_HAND, 24, 6, 0, 0, 0, 0, 4, "pick", MT_IRON, AP_METAL, 48 },
    { "shovel", K_HAND, 20, 8, 0, 0, 0, 0, 4, "shovel", MT_IRON, AP_METAL, 36 },
    { "lockpicks", 0, 1, 1, 0, 0, 0, 0, 1, "picks", MT_IRON, AP_METAL, 40 },
    { "chisel", K_HAND, 2, 1, 0, 0, 0, 0, 1, "chisel", MT_STEEL, AP_METAL, 12 },
    { "rope", 0, 40, 8, 0, 0, 0, 0, 4, "rope", MT_CLOTH, AP_CLOTH, 12 },
    { "bones", K_PLURAL, 8, 4, 0, 0, 0, 0, 4, "", MT_BONE, AP_BONE, 0 },
    { "pennies", K_PLURAL, 0, 0, 0, 0, 0, 0, 1, "", MT_BRONZE, AP_METAL, 1 },
    { "iron key", 0, 1, 0, 0, 0, 0, 0, 1, "key", MT_IRON, AP_METAL, 4 },
    { "candle", K_LIGHT | K_HAND, 1, 1, 0, 0, 0, 2, 1, "", MT_OTHER, AP_FOOD, 1 },
    { "lantern", K_LIGHT | K_HAND, 8, 4, 0, 0, 0, 6, 3, "", MT_IRON, AP_METAL, 30 },
    { "sack", K_CONTAINER, 2, 2, 24, 30, 0, 0, 3, "", MT_CLOTH, AP_CLOTH, 3 },
    { "chest", K_CONTAINER, 120, 80, 60, 100, 0, 0, 7, "", MT_WOOD, AP_WOOD, 80 },
    { "skull", 0, 4, 2, 0, 0, 0, 0, 3, "", MT_BONE, AP_BONE, 0 },
    { "broken sword", 0, 12, 2, 0, 0, 0, 0, 4, "blade", MT_IRON, AP_WEAPON, 10 },
    { "cloak", K_WEAR, 8, 6, 0, 0, 0, 0, 4, "", MT_CLOTH, AP_CLOTH, 24 },
    { "potsherds", K_PLURAL, 2, 1, 0, 0, 0, 0, 2, "", MT_CLAY, AP_POTTERY, 0 },
    { "sword", K_HAND | K_HANG, 12, 3, 0, 0, 0, 0, 4, "blade", MT_IRON, AP_WEAPON, 240 },
};
