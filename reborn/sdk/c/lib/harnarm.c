/*
** harnarm.c - harn.h's armour pieces (Combat 4): each one's material, the locations it covers, its weight and price.
*/

#include <harn.h>

#define L(x)    (1u << HARN_ ## x)
#define TX_AB   (L (THORAX) | L (ABDOMEN))
#define TUNIC   (L (UPPERARM) | L (SHOULDER) | TX_AB | L (HIP) | L (GROIN))
#define VEST    (L (SHOULDER) | TX_AB)
#define SURCOAT (L (SHOULDER) | TX_AB | L (HIP) | L (GROIN) | L (THIGH))
#define LEGS    (L (HIP) | L (GROIN) | L (THIGH) | L (KNEE) | L (CALF) | L (FOOT))
#define HAUBERK (L (FOREARM) | L (ELBOW) | TUNIC | L (THIGH))

const harn_armour harn_armours[] = {
    { "cloth cap",          HARN_CLOTH,   HARN_BOTH,  L (SKULL), 4, 8 },
    { "cloth hood",         HARN_CLOTH,   HARN_BOTH,  L (SKULL) | L (NECK), 8, 16 },
    { "cloth vest",         HARN_CLOTH,   HARN_BOTH,  VEST, 28, 56 },
    { "cloth tunic",        HARN_CLOTH,   HARN_BOTH,  TUNIC, 44, 88 },
    { "cloth surcoat",      HARN_CLOTH,   HARN_BOTH,  SURCOAT, 53, 106 },
    { "cloth robe",         HARN_CLOTH,   HARN_BOTH,  TUNIC | L (FOREARM) | L (ELBOW) | L (THIGH) | L (KNEE) | L (CALF), 79, 158 },
    { "cloth leggings",     HARN_CLOTH,   HARN_BOTH,  LEGS, 44, 88 },
    { "quilt cap",          HARN_QUILT,   HARN_BOTH,  L (SKULL), 12, 16 },
    { "quilt cowl",         HARN_QUILT,   HARN_BOTH,  L (SKULL) | L (NECK), 24, 32 },
    { "quilt tunic",        HARN_QUILT,   HARN_BOTH,  TUNIC, 132, 176 },
    { "gambeson",           HARN_QUILT,   HARN_BOTH,  HAUBERK, 198, 264 },
    { "quilt leggings",     HARN_QUILT,   HARN_BOTH,  LEGS, 132, 176 },
    { "leather cap",        HARN_LEATHER, HARN_BOTH,  L (SKULL), 8, 16 },
    { "leather cowl",       HARN_LEATHER, HARN_BOTH,  L (SKULL) | L (NECK), 16, 32 },
    { "leather vest",       HARN_LEATHER, HARN_BOTH,  VEST, 56, 112 },
    { "leather tunic",      HARN_LEATHER, HARN_BOTH,  TUNIC, 88, 176 },
    { "leather surcoat",    HARN_LEATHER, HARN_BOTH,  SURCOAT, 106, 212 },
    { "leather leggings",   HARN_LEATHER, HARN_BOTH,  LEGS, 88, 176 },
    { "shoes",              HARN_LEATHER, HARN_BOTH,  L (FOOT), 12, 24 },
    { "calf boots",         HARN_LEATHER, HARN_BOTH,  L (CALF) | L (FOOT), 32, 64 },
    { "knee boots",         HARN_LEATHER, HARN_BOTH,  L (KNEE) | L (CALF) | L (FOOT), 38, 76 },
    { "leather gauntlets",  HARN_LEATHER, HARN_BOTH,  L (HAND), 8, 16 },
    { "kurbul halfhelm",    HARN_KURBUL,  HARN_BOTH,  L (SKULL), 10, 20 },
    { "kurbul breastplate", HARN_KURBUL,  HARN_FRONT, TX_AB, 30, 60 },
    { "kurbul backplate",   HARN_KURBUL,  HARN_BACK,  TX_AB, 30, 60 },
    { "kurbul ailettes",    HARN_KURBUL,  HARN_BOTH,  L (SHOULDER), 10, 20 },
    { "kurbul rerebraces",  HARN_KURBUL,  HARN_BOTH,  L (UPPERARM), 15, 30 },
    { "kurbul coudes",      HARN_KURBUL,  HARN_BOTH,  L (ELBOW), 5, 10 },
    { "kurbul vambraces",   HARN_KURBUL,  HARN_BOTH,  L (FOREARM), 13, 25 },
    { "kurbul kneecops",    HARN_KURBUL,  HARN_BOTH,  L (KNEE), 8, 15 },
    { "kurbul greaves",     HARN_KURBUL,  HARN_BOTH,  L (CALF), 25, 50 },
    { "ring halfhelm",      HARN_RING,    HARN_BOTH,  L (SKULL), 16, 28 },
    { "ring vest",          HARN_RING,    HARN_BOTH,  VEST, 112, 196 },
    { "ring byrnie",        HARN_RING,    HARN_BOTH,  TUNIC, 176, 308 },
    { "ring hauberk",       HARN_RING,    HARN_BOTH,  HAUBERK, 264, 462 },
    { "ring leggings",      HARN_RING,    HARN_BOTH,  LEGS, 176, 308 },
    { "ring gauntlets",     HARN_RING,    HARN_BOTH,  L (HAND), 16, 28 },
    { "mail cowl",          HARN_MAIL,    HARN_BOTH,  L (SKULL) | L (NECK), 40, 120 },
    { "mail byrnie",        HARN_MAIL,    HARN_BOTH,  TUNIC, 220, 660 },
    { "mail hauberk",       HARN_MAIL,    HARN_BOTH,  HAUBERK, 330, 990 },
    { "mail leggings",      HARN_MAIL,    HARN_BOTH,  LEGS, 220, 660 },
    { "mail mittens",       HARN_MAIL,    HARN_BOTH,  L (HAND), 20, 60 },
    { "scale vest",         HARN_SCALE,   HARN_BOTH,  VEST, 196, 280 },
    { "scale byrnie",       HARN_SCALE,   HARN_BOTH,  TUNIC, 308, 440 },
    { "scale hauberk",      HARN_SCALE,   HARN_BOTH,  HAUBERK, 462, 660 },
    { "plate halfhelm",     HARN_PLATE,   HARN_BOTH,  L (SKULL), 32, 100 },
    { "plate 3/4 helm",     HARN_PLATE,   HARN_BOTH,  L (SKULL) | L (FACE), 56, 175 },
    { "great helm",         HARN_PLATE,   HARN_BOTH,  L (SKULL) | L (FACE) | L (NECK), 88, 275 },
    { "plate breastplate",  HARN_PLATE,   HARN_FRONT, TX_AB, 96, 300 },
    { "plate backplate",    HARN_PLATE,   HARN_BACK,  TX_AB, 96, 300 },
    { "plate ailettes",     HARN_PLATE,   HARN_BOTH,  L (SHOULDER), 32, 100 },
    { "plate rerebraces",   HARN_PLATE,   HARN_BOTH,  L (UPPERARM), 48, 150 },
    { "plate coudes",       HARN_PLATE,   HARN_BOTH,  L (ELBOW), 16, 50 },
    { "plate vambraces",    HARN_PLATE,   HARN_BOTH,  L (FOREARM), 40, 125 },
    { "plate kneecops",     HARN_PLATE,   HARN_BOTH,  L (KNEE), 24, 75 },
    { "plate greaves",      HARN_PLATE,   HARN_BOTH,  L (CALF), 80, 250 },
};
const unsigned char harn_narmours = sizeof harn_armours / sizeof harn_armours[0];
