/*
** rshouse.h - Dingus Quest Zero's house rules that every referee applies alike (HOUSE.md's: the game's own, not a
** ruleset's), so a second ruleset gives the same game where the rules aren't its own: the tools that dig, by the
** material (RS_HOUSE_DIG: the table; RS_HOUSE_DIG_TOOL: the one to hand that suits best, the function named so); what
** the character's going and watchfulness make of a notice (RS_HOUSE_NOTICE); and the words of its pace, its
** watchfulness and its light in its state (RS_HOUSE_STATE: RS_DESCRIBE b 3's).  A referee defines the parts it wants
** before it includes this, where it wants their code (its main RAM, an overlay, a swapped part): each part once in a
** file, static (but the dig tool's), its words through the referee's own put and putn, the tools to hand its own
** (tools[NTOOLS][8], RS_EQUIP's keys).
*/

/* ---- The tools that dig (HOUSE.md: digging; rs.h's P_DIG): each one's key (the things' catalogue's), its feet a
** spell of work at a marginal success, and the wear a spell does it, by what it's into (rs.h's MAT_*: earth, stone,
** masonry, a floor's flags); bare hands the first.  A critical success twice the feet, a marginal failure half, a
** critical failure none */
#if defined (RS_HOUSE_DIG) && !defined (RS_HOUSE_DIG_DONE)
#define RS_HOUSE_DIG_DONE
static const struct { char key[8]; unsigned char rate[4], wear[4]; } digger[] = {    /* Earth, stone, masonry, flags */
    { "hands", { 1, 0, 0, 0 }, { 0, 0, 0, 0 } }, { "shovel", { 3, 0, 0, 1 }, { 0, 0, 0, 0 } },
    { "spade", { 3, 0, 0, 1 }, { 0, 0, 0, 0 } }, { "pick", { 2, 2, 1, 3 }, { 0, 1, 1, 1 } },
    { "mattock", { 3, 2, 1, 3 }, { 0, 1, 1, 1 } }, { "chisel", { 0, 1, 2, 1 }, { 0, 1, 1, 1 } },
    { "blade", { 1, 0, 0, 0 }, { 40, 0, 0, 0 } }, { "knife", { 1, 0, 0, 0 }, { 40, 0, 0, 0 } },
};
#define NDIGGERS        (sizeof digger / sizeof digger[0])
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
        putn (5 * light);
        put (" feet).");
    } else {
        put (dark ? "none: it's dark here, and you feel your way." : "none of your own: this place's own light.");
    }
}
#endif
