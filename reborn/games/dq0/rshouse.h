/*
** rshouse.h - Dingus Quest Zero's house rules that every referee applies alike (HOUSE.md's: the game's own, not a
** ruleset's), so a second ruleset gives the same game where the rules aren't its own: the tools that dig, by the
** material (RS_HOUSE_DIG: the table; RS_HOUSE_DIG_TOOL: the one to hand that suits best, the function named so); what
** the character's going and watchfulness make of a notice (RS_HOUSE_NOTICE); and the words of its pace, its
** watchfulness and its light in its state (RS_HOUSE_STATE: RS_DESCRIBE b 3's); what a thing does swung (RS_HOUSE_SWING:
** the function named so, the world's too); the bearings' own dice (RS_HOUSE_BEARING: P_BEARING's, apart from the
** rest).  A referee defines the parts it wants before it includes this, where it
** wants their code (its main RAM, an overlay, a swapped part): each part once in a file, static (but the dig tool's;
** the swing's in each part that wants it, named for it), its words through the referee's own put and putn, the tools
** to hand its own (tools[NTOOLS][8], RS_EQUIP's keys).
*/

/* ---- The tools that dig (HOUSE.md: digging; rs.h's P_DIG): each one's key (the things' catalogue's), its half-feet a
** spell of work, and the wear a spell does it, by what it's into (rs.h's MAT_*: hard-packed earth, stone, masonry, a
** floor's flags, soft soil); bare hands the first.  The user's, 2026-10-20: earth is dug at the tool's rate, no test
** (bare hands half a foot of soft soil, nothing of hard-packed earth; every tool half as far in hard-packed earth
** as in soil); anything harder only with a pick, a Mining test each spell: its rate at a marginal success, twice at
** a critical, half at a marginal failure, none at a critical failure */
#if defined (RS_HOUSE_DIG) && !defined (RS_HOUSE_DIG_DONE)
#define RS_HOUSE_DIG_DONE
static const struct { char key[8]; unsigned char rate[5], wear[5]; } digger[] = {   /* Earth, stone, masonry, flags,
                                                                                    **   soil */
    { "hands", { 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0 } }, { "shovel", { 3, 0, 0, 0, 6 }, { 0, 0, 0, 0, 0 } },
    { "spade", { 3, 0, 0, 0, 6 }, { 0, 0, 0, 0, 0 } }, { "pick", { 2, 4, 2, 6, 4 }, { 0, 1, 1, 1, 0 } },
    { "mattock", { 3, 0, 0, 0, 6 }, { 0, 0, 0, 0, 0 } },
    { "blade", { 1, 0, 0, 0, 2 }, { 40, 0, 0, 0, 40 } }, { "dagger", { 1, 0, 0, 0, 2 }, { 40, 0, 0, 0, 40 } },
    { "knife", { 1, 0, 0, 0, 2 }, { 40, 0, 0, 0, 40 } },
};
#define NDIGGERS        (sizeof digger / sizeof digger[0])
#define DIG_TESTED(m)   ((m) != MAT_EARTH && (m) != MAT_SOIL)   /* (Earth dug at the tool's rate; the rest a Mining test) */
#endif

/* The tool to hand that suits the material best (bare hands, digger's 0, if none does better; of two as good, the
** one it wears less): its digger's number */
#if defined (RS_HOUSE_DIG_TOOL) && !defined (RS_HOUSE_DIG_TOOL_DONE)
#define RS_HOUSE_DIG_TOOL_DONE
unsigned char __fastcall__ RS_HOUSE_DIG_TOOL (unsigned char m)
{
    unsigned char k, t, best = 0, r, br = digger[0].rate[m];

    for (k = 0; k < NTOOLS; ++k) {
        for (t = 1; tools[k][0] && t < NDIGGERS; ++t) {
            if (strcmp (tools[k], digger[t].key)) {
                continue;
            }
            r = digger[t].rate[m];
            if (r > br || (r && r == br && digger[t].wear[m] < digger[best].wear[m])) {
                best = t;
                br = r;
            }
        }
    }
    return best;
}
#endif

/* ---- What a thing does swung (HOUSE.md's fights and portals, the user's, 2026-10-22: "I think it should try and use
** whatever you request, but if it is something soft ... it should reply with something like 'You attempt to batter the
** door with the noodle, but it seems ineffectual'"): a thing named to strike with (or the one in its hands) that isn't
** a weapon of the ruleset's, by its kind's data (items.h's struct kind, the catalogue's): soft (IM_SOFT) if it's made of
** something that gives (cloth, leather, wax, tallow, food and the like) or only breaks (glass, pottery), if it's a heap
** or a lot (sand, pennies, arrows), anything worn (armour, clothes: not made to swing), under half a pound, or lighter
** than its bulk (hollow, loose: a basket, a pack, straw): one attempt, told, no harm; else light (IM_LIGHT, under two and
** a half pounds: the book's stick, HarnMaster's referee) or hefty (IM_HEFTY: its club).  The ruleset's own weapons by
** their keys first, the referee's */
#if defined (RS_HOUSE_SWING)
#if !defined (RS_HOUSE_SWING_DONE)
#define RS_HOUSE_SWING_DONE
enum { IM_SOFT, IM_LIGHT, IM_HEFTY };
#define IM_HARD         ((1u << MT_WOOD) | (1u << MT_IRON) | (1u << MT_STEEL) | (1u << MT_BRONZE) | (1u << MT_BONE) | \
                         (1u << MT_STONE) | (1u << MT_SILVER))
#endif

static unsigned char __fastcall__ RS_HOUSE_SWING (const struct kind* k)
{
    if (!(IM_HARD & 1u << k->mat) || (k->form & (KF_SMALL | KF_LIQUID | KF_GAS | KF_COUNT)) ||
        (k->flags & (K_WEAR | K_HAND)) == K_WEAR ||    /* (Worn and not held: a shield or a bow slung isn't worn
                                                **   to swing, it's held, 2026-10-27) */
        k->weight < 2 || k->weight < k->bulk) {
        return IM_SOFT;
    }
    return k->weight < 10 ? IM_LIGHT : IM_HEFTY;
}
#endif

/* ---- What the character notices, by its going and its watchfulness (HOUSE.md: noticing; DUNGEON.md: moving): its
** pace as it moves (a creep or a slow walk a step better, but not by feel in the dark; a jog two worse, a run four,
** a sprint six); listening, a sound two better.  A hazard underfoot (P_NOTICE_SIGHT's b 1, its steps the hiding's)
** by a search's chance while it searches as it goes, else a passing glance's: twice its hiding's steps, and two
** more.  The steps more for request r (go: the hub's pc_going; watch: its watch, WA_*); the ruleset's own (the
** character's state ...) the referee's to add */
#if defined (RS_HOUSE_NOTICE) && !defined (RS_HOUSE_NOTICE_DONE)
#define RS_HOUSE_NOTICE_DONE
static const signed char by_pace[NGAITS] = { 1, 1, 0, -2, -4, -6 };

static signed char __fastcall__ house_notice (const struct req* r, unsigned char go, unsigned char watch)
{
    unsigned char g = go & ~GOING_DARK;
    signed char s = 0, p;

    if (r->a == P_NOTICE_SIGHT && r->b == 1 && !(watch & WA_TRAPS)) {
        s = r->steps - 2;
    }
    if (g && g <= NGAITS) {
        p = by_pace[g - 1];
        s += p > 0 && (go & GOING_DARK) ? 0 : p;
    }
    if (r->a == P_NOTICE_SOUND && (watch & WA_LISTEN)) {
        s += 2;
    }
    return s;
}
#endif

/* ---- The character's state in words (the status command's, RS_DESCRIBE b 3): its pace and what it's held to (the
** world's: a slow walk at most while it searches or maps as it goes, or feels its way in the dark; a walk while it
** listens), "Pace: a jog, held to a slow walk while you search as you go", the pace it's held to returned (the
** referee's words after it: its feet a round); its watchfulness, ".\nWatching: ..."; its light, "\nLight: ..." (the
** referee's condition before it) */
#if defined (RS_HOUSE_STATE) && !defined (RS_HOUSE_STATE_DONE)
#define RS_HOUSE_STATE_DONE
static const char* const pace_name[NGAITS] = { "a creep", "a slow walk", "a walk", "a jog", "a run", "a sprint" };

static unsigned char __fastcall__ house_pace (unsigned char g, unsigned char watch, unsigned char dark)
{
    unsigned char h = g;

    if (h > GA_SLOW && ((watch & (WA_TRAPS | WA_MAP)) || dark)) {
        h = GA_SLOW;
    } else if (h > GA_WALK && (watch & WA_LISTEN)) {
        h = GA_WALK;
    }
    put ("Pace: ");
    put (pace_name[g]);
    if (h != g) {
        put (", held to ");
        put (pace_name[h]);
        put (watch & WA_TRAPS ? " while you search as you go" : watch & WA_MAP ? " while you map as you go" :
            dark && h == GA_SLOW ? " in the dark" : " while you listen");
    }
    return h;
}

static void __fastcall__ house_watching (unsigned char watch)
{
    put (".\nWatching: ");
    put (!watch ? "nothing in particular" : watch & WA_TRAPS ? "for traps and hidden doors as you go" : "");
    put ((watch & WA_TRAPS) && (watch & (WA_LISTEN | WA_MAP)) ? "; " : "");
    put (watch & WA_LISTEN ? (watch & WA_MAP ? "listening and mapping as you go" : "listening as you go") :
        watch & WA_MAP ? "mapping as you go" : "");
    put (".");
}

/* (light: its reach in squares; lamp: what gives it; dark: its square unlit) */
static void __fastcall__ house_light (unsigned char light, const char* lamp, unsigned char dark)
{
    put ("\nLight: ");
    if (light) {
        put ("your ");
        put (lamp);
        put (" (");
        putn (SQ_FT * light);
        put (" feet).");
    } else {
        put (dark ? "none: it's dark here, and you feel your way." : "none of your own: this place's own light.");
    }
}
#endif

/* ---- The bearings' own dice (rs.h's P_BEARING; HOUSE.md's bearings underground, every ruleset's): the generator's
** state (dice.h's numbers: xorshift.s's xs_seed) swapped for the bearings' own about their test, before it and after
** (bdice), so the game's other tests roll as they did whether its walks bend or not; the bearings' state the referee's
** bseed, begun at the first from the generator's as it is then (its own seed's: the game's), scrambled */
#if defined (RS_HOUSE_BEARING) && !defined (RS_HOUSE_BEARING_DONE)
#define RS_HOUSE_BEARING_DONE
extern unsigned long xs_seed;
extern unsigned long bseed;

static void bdice (void)
{
    unsigned long t = xs_seed;

    xs_seed = bseed ? bseed : t ^ 0x42454152UL;
    bseed = t;
}
#endif

/* ---- Moving quietly's own dice (rs.h's P_STEALTH, the character's and a creature's: HOUSE.md's sound, the air and
** moving quietly, every ruleset's): the same way as the bearings' (qdice about the test, before it and after), so the
** game's other tests roll as they did whether anyone creeps or stalks; its state the referee's qseed */
#if defined (RS_HOUSE_QUIET) && !defined (RS_HOUSE_QUIET_DONE)
#define RS_HOUSE_QUIET_DONE
extern unsigned long xs_seed;
static unsigned long qseed;

static void qdice (void)
{
    unsigned long t = xs_seed;

    xs_seed = qseed ? qseed : t ^ 0x51554945UL;
    qseed = t;
}
#endif
