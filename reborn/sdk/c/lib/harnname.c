/*
** harnname.c - harn.h's names: the attributes', species', sexes', months', signs' and sunsigns', cultures',
** classes', family's, frames', comeliness's, colours', medical traits', disorders', sexualities' and deities'.
*/

#include <harn.h>

static const char* const attrs[HARN_NATTRS] = {
    "Species", "Sex", "Birthday", "Sunsign", "Height", "Frame", "Optimum Weight", "Weight", "Comeliness",
    "Strength", "Stamina", "Dexterity", "Agility", "Eyesight", "Hearing", "Smell", "Voice", "Intelligence", "Aura",
    "Will", "Morality", "Deity", "Piety", "Endurance"
};

const char* __fastcall__ harn_attr_name (unsigned char a)
{
    return a < HARN_NATTRS ? attrs[a] : "";
}

/* A name from a table of n, "" past its end */
static const char* __fastcall__ pick (const char* const* t, unsigned char n, unsigned char i)
{
    return i < n ? t[i] : "";
}
#define NAME(f, t)  const char* __fastcall__ f (unsigned char i) { return pick (t, sizeof t / sizeof t[0], i); }

static const char* const species[] = { "Human", "Sindarin", "Khuzdul", "Other" };
static const char* const sexes[] = { "Female", "Male" };
static const char* const months[] = {
    "Nuzyael", "Peonu", "Kelen", "Nolus", "Larane", "Agrazhar", "Azura", "Halane", "Savor", "Ilvin", "Navek", "Morgat"
};
static const char* const signs[] = {
    "Ulandus", "Aralius", "Feniri", "Ahnu", "Angberelius", "Nadai", "Hirin", "Tarael", "Tai", "Skorus", "Masara",
    "Lado"
};
static const char* const cultures[] = { "Tribal", "Viking", "Feudal", "Imperial", "Khuzan", "Sindarin" };
static const char* const classes[] = { "Slave", "Serf", "Unguilded", "Guilded", "Noble" };
static const char* const parents[] = { "Offspring", "Fostered", "Adopted", "Bastard", "Orphan" };
static const char* const estrangements[] = { "Outcast", "Unpopular", "Average", "Popular", "Favorite" };
static const char* const clanheads[] = { "Distant Relation", "Aunt or Uncle", "Cousin", "Father or Mother" };
static const char* const frames[] = { "Scant", "Light", "Medium", "Heavy", "Massive" };
static const char* const comeliness[] = { "Ugly", "Plain", "Average", "Attractive", "Handsome" };
static const char* const complexions[] = { "Fair", "Medium", "Dark" };
static const char* const hairs[] = { "Brown", "Black", "Red", "Silver", "Blond" };
static const char* const eyes[] = { "Hazel", "Gray", "Violet", "Green", "Blue" };
static const char* const medical[] = {
    "None", "Addiction", "Albinism", "Allergy", "Ambidextrous", "Arm", "Birthmark", "Dwarfism", "Epilepsy", "Ear",
    "Eye", "Gigantism", "Hemophilia", "Hirsutism", "Leg", "Left-Handed", "Leprosy", "Lycanthropy", "Monochromasia",
    "Obesity", "Parasites", "Poxmarks", "Scars", "Sterile"
};
static const char* const psyche[] = {
    "None", "Acrophobia", "Aichmophobia", "Algophobia", "Astraphobia", "Ballistophobia", "Bulimia",
    "Claustrophobia", "Cynophobia", "Dyslexia", "Entomophobia", "Gamblamania", "Gephyrophobia", "Heliophobia",
    "Hemophobia", "Hydrophobia", "Hypochondria", "Ivashuphobia", "Kleptomania", "Megalomania", "Mysophobia",
    "Multiple Personality", "Musophobia", "Necrophobia", "Nyctophobia", "Ophidiophobia", "Ornithophobia",
    "Paranoia", "Psychopathy", "Pvaraphobia", "Pyromania", "Pyrophobia", "Sadism", "Sex Mania", "Theophobia",
    "Toxicophobia", "Violent Temper", "Xenophobia", "Yaelaphobia"
};
static const char* const sexualities[] = { "Heterosexual", "Bi-Sexual", "Homosexual" };
static const char* const deities[] = {
    "Agrik", "Halea", "Ilvir", "Larani", "Morgath", "Naveh", "Peoni", "Sarajin", "Save-K'nor", "Siem", "None"
};

NAME (harn_species_name, species)
NAME (harn_sex_name, sexes)
NAME (harn_sign_name, signs)
NAME (harn_culture_name, cultures)
NAME (harn_class_name, classes)
NAME (harn_parent_name, parents)
NAME (harn_estrangement_name, estrangements)
NAME (harn_clanhead_name, clanheads)
NAME (harn_frame_name, frames)
NAME (harn_complexion_name, complexions)
NAME (harn_hair_name, hairs)
NAME (harn_eye_name, eyes)
NAME (harn_medical_name, medical)
NAME (harn_psyche_name, psyche)
NAME (harn_sexuality_name, sexualities)
NAME (harn_deity_name, deities)

const char* __fastcall__ harn_month_name (unsigned char m)
{
    return m >= 1 && m <= 12 ? months[m - 1] : "";
}

const char* __fastcall__ harn_comeliness_name (unsigned char cml)
{
    return comeliness[cml <= 5 ? 0 : cml <= 8 ? 1 : cml <= 12 ? 2 : cml <= 15 ? 3 : 4];
}

const char* __fastcall__ harn_sunsign_name (unsigned char ss)
{
    static char b[32];
    unsigned char s, i = 0;
    const char* p;

    if (ss >= HARN_NSUNSIGNS) {
        return "";
    }
    s = harn_sunsign_signs (ss);
    for (p = signs[s >> 4]; *p; ) {
        b[i++] = *p++;
    }
    if ((s >> 4) != (s & 15)) {
        b[i++] = '-';
        for (p = signs[s & 15]; *p; ) {
            b[i++] = *p++;
        }
        for (p = " Cusp"; *p; ) {
            b[i++] = *p++;
        }
    }
    b[i] = 0;
    return b;
}
