/*
** pace.c - (swapped part 4) the character's pace, its watchfulness, its state and its small moves, from the line typed
** (DUNGEON.md: moving; the user's, 2026-10-16), before the parser's command: a pace ("walk", "walk slowly", "creep",
** "sneak", "jog", "run", "sprint"; "set pace to jog"; "run north" sets it and goes, the move the command's; and the
** way its legs go, "crawl", "hop", "walk": slice 4.1b's), a
** watchfulness ("search for traps as I go", "watch for traps", "move carefully", "listen as I go", "map as I go";
** "stop searching", "stop listening", "stop mapping", "stop running"), "status", the map's colours ("colour off",
** "colour on": dq0.h's plain), and a small move ("move left one
** space", "step forward", "go north 20 feet", "back up two squares", "sidestep right": a square is 5 feet, a yard 3).
** The pace and the watchfulness are the world's to set (the hub's gait and watch: its request, WQ_SET), the
** referee's to word (RS_DESCRIBE b 3), the small move the world's (I_STEP).  Its own words matched here; the direction
** the parser's (a relative one made the compass's).
*/

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "rs.h"
#include "level.h"

#pragma local-strings (on)
#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

#define NW      12
static char b[INMAX + 1];
static char* w[NW];
static unsigned char nw;

/* The line's words (lower case), in b */
static void split (void)
{
    char* l = b;

    nw = 0;
    while (*l && nw < NW) {
        while (*l == ' ' || *l == ',' || *l == '.' || *l == '!') {
            *l++ = 0;
        }
        if (!*l) {
            break;
        }
        w[nw++] = l;
        while (*l && *l != ' ' && *l != ',' && *l != '.' && *l != '!') {
            if (*l >= 'A' && *l <= 'Z') {
                *l += 'a' - 'A';
            }
            ++l;
        }
    }
}

/* Which of the words (0-ended, then an empty one) is among the line's: its place in the line plus 1, or 0 */
static unsigned char __fastcall__ any (const char* list)
{
    unsigned char i;
    const char* p;

    for (p = list; *p; p += strlen (p) + 1) {
        for (i = 0; i < nw; ++i) {
            if (!strcmp (w[i], p)) {
                return i + 1;
            }
        }
    }
    return 0;
}

/* The paces' words (the last said goes: "walk slowly"); GA_SAID, a pace said outright, a faster one than feeling
** the way: in the dark, gone at there (the world's, its dangers: HOUSE.md's walking in the dark), as a jog, a run and a
** sprint are; "walk" alone, there, the cautious walk */
static const struct { char w[11]; unsigned char g; } paces[] = {
    { "creep", GA_CREEP }, { "creeping", GA_CREEP }, { "sneak", GA_CREEP }, { "sneaking", GA_CREEP },
    { "tiptoe", GA_CREEP }, { "crawl", GA_CREEP | GA_CRAWL }, { "hop", GA_WALK | GA_HOP },
    { "quietly", GA_CREEP }, { "stealthily", GA_CREEP },
    { "slowly", GA_SLOW }, { "slow", GA_SLOW }, { "walk", GA_WALK }, { "walking", GA_WALK },
    { "normal", GA_WALK | GA_SAID }, { "normally", GA_WALK | GA_SAID }, { "quickly", GA_WALK | GA_SAID },
    { "jog", GA_JOG }, { "jogging", GA_JOG }, { "trot", GA_JOG }, { "briskly", GA_JOG },
    { "run", GA_RUN }, { "running", GA_RUN }, { "hurry", GA_RUN }, { "sprint", GA_SPRINT }, { "sprinting", GA_SPRINT },
    { "dash", GA_SPRINT },
};

/* A count's words: one ... ten (a, an: one), or figures; 0 none */
static const char counts[] = "a\0one\0two\0three\0four\0five\0six\0seven\0eight\0nine\0ten\0";

static unsigned char __fastcall__ count_of (const char* s)
{
    const char* p;
    unsigned char k;

    if (*s >= '0' && *s <= '9') {
        k = atoi (s);
        return k ? k : 1;
    }
    for (p = counts, k = 1; *p; p += strlen (p) + 1, ++k) {
        if (!strcmp (s, p)) {
            return k == 1 ? 1 : k - 1;
        }
    }
    return 0;
}

/* The referee's words of the character's state, its pace and watchfulness changed first as a says (0: all of it, and
** the way it believes it faces after them: slice 4.2's bearings) */
static void __fastcall__ described (unsigned char a)
{
    struct req* r = ask_b (RS_DESCRIBE, a, 3);

    if (a) {
        msg (r->text);
    } else {
        blank ();
        say (r->text);
        facing_words ();                        /* (The way it believes it faces: overlay 7's) */
        blank ();
    }
    r->state = REQ_FREE;
}

/* The line typed, if it's the pace's, the watchfulness's, the state's or a small move's: carried out, 1; else 0 (the
** command's; but its pace, said with a move, set first: "run north") */
unsigned char __fastcall__ s_pace_cmd (const char* l)
{
    unsigned char i, k, g = 0xFF, wa, st, n = 0, keep, d, said;
    unsigned mw;

    strncpy (b, l, INMAX);
    split ();
    if (!nw) {
        return 0;
    }
    if (nw == 1 && !strcmp (w[0], "status")) {
        described (0);
        return 1;
    }
    if (nw <= 3 && any ("colour\0color\0colours\0colors\0")) {     /* (The map's colours on or off: "colour off",
                                                **   "no colours", "colours on", slice 4.2's) */
        plain = any ("off\0no\0plain\0none\0") != 0;
        msg (plain ? "The map in plain letters, no colours." : "The map in colours: what you see now bright, what you "
            "remember dim, what's on your map brown.");
        return 1;
    }
    for (i = 0; i < nw; ++i) {
        for (k = 0; k < sizeof paces / sizeof paces[0]; ++k) {
            if (!strcmp (w[i], paces[k].w)) {
                g = paces[k].g;
            }
        }
        if ((d = count_of (w[i])) == 0) {
            continue;
        }
        if (i + 1 == nw) {                      /* (Figures at the end, after a way: squares; alone, a choice's) */
            n = i && *w[i] >= '0' && *w[i] <= '9' ? d : n;
        } else if (any ("space\0spaces\0square\0squares\0step\0steps\0pace\0paces\0") == i + 2) {
            n = d;                              /* (A count and its unit: squares) */
        } else if (any ("foot\0feet\0ft\0") == i + 2) {
            n = (d + 2) / 5 ? (d + 2) / 5 : 1;
        } else if (any ("yard\0yards\0") == i + 2) {
            n = (3 * d + 2) / 5;
        }
    }
    said = g != 0xFF && (g & 7) > GA_WALK ? GA_SAID : 0;  /* (Outright, GA_SAID's or faster than a walk: for the dark;
                                                **   the legs' way, crawling or hopping, the world's: slice 4.1b's) */
    g = g == 0xFF ? g : g & ~GA_SAID;
    wa = HUB->watch;
    if (any ("stop\0")) {                       /* (Stopped: what it names; a pace's, back to a walk) */
        if (g == GA_WALK) {
            g = 0xFF;                           /* ("stop walking": the long task's, the parser's: slice 4.0) */
        }
        if (HUB->pc_task.kind == LT_SEARCH && any ("searching\0search\0")) {
            return 0;                           /* ("stop searching" as a search goes: the long task's too) */
        }
        k = wa;
        wa &= any ("searching\0search\0watching\0looking\0") ? ~WA_TRAPS : 0xFF;
        wa &= any ("listening\0listen\0") ? ~WA_LISTEN : 0xFF;
        wa &= any ("mapping\0map\0") ? ~WA_MAP : 0xFF;
        if (wa == k && g == 0xFF) {
            return 0;
        }
        g = g == 0xFF ? 0xFF : GA_WALK;
        said = 0;
    } else {
        k = any ("go\0move\0moving\0walk\0walking\0going\0travel\0as\0while\0proceed\0");
        if ((any ("trap\0traps\0") && any ("for\0") && (k || !any ("search\0examine\0disarm\0check\0x\0inspect\0study\0"))) ||
            any ("carefully\0careful\0warily\0")) {
            wa |= WA_TRAPS;
        }
        if (any ("listen\0listening\0") && (k || any ("listening\0"))) {
            wa |= WA_LISTEN;                    /* ("Listen as I go", "keep listening"; "listen" alone, "listen at the
                                                **   door", a listen, the command's: way.c's) */
        }
        if (any ("mapping\0") || (any ("map\0") && k)) {
            wa |= WA_MAP;                       /* (The map's side, its materials, the orientation's) */
        }
    }
    st = g != 0xFF || wa != HUB->watch;          /* (A pace said, or the watchfulness changed) */
    if (any ("step\0sidestep\0shuffle\0") == 1 || (nw > 1 && !strcmp (w[0], "back") && !strcmp (w[1], "up"))) {
        n = n ? n : 0xFF;                       /* (A small move: by the pace, if no count, below) */
    }
    if (!st && !n) {
        return 0;                               /* (None of its words: the command's) */
    }
    cq_cmd (l);                                 /* (The direction the command parser's) */
    switch (cmd.action) {                       /* (A command of another kind, said with a manner: the command's,
                                                **   the pace as it was: "open the chest carefully", "close the door
                                                **   quietly", "run away" a flight) */
    case VB_NONE:
    case VB_MOVE_DIRECTION:
    case VB_MOVE_TOWARD:
    case VB_ENTER:
    case VB_APPROACH_OBJECT:
    case VB_STEP_DIRECTION:
    case VB_SET_PACE:
    case VB_SNEAK:
    case VB_PUT_OBJECT:                         /* ("set pace to jog") */
    case VB_SEARCH:
    case VB_LISTEN:
    case VB_META_MAP:
    case VB_STOP:
        break;
    default:
        return 0;
    }
    k = (cmd.action == VB_MOVE_DIRECTION || cmd.action == VB_MOVE_TOWARD || cmd.action == VB_ENTER ||
        cmd.action == VB_APPROACH_OBJECT) && (cmd.dir != 0xFF || cmd.obj.kind);
    st = st && (!k || n || g != HUB->gait || wa != HUB->watch);    /* (Said, unless it goes on as it was) */
    mw = HUB->pc_map;
    wset (WS_PACE, g == 0xFF ? g : g | said, wa);   /* (The world's, set: its request) */
    if (st) {
        described (1);
    }
    if (!mw && HUB->pc_map) {
        msg ("You begin a map.");               /* (Mapping on paper: its first sheet the map, the world's) */
    }
    d = cmd.dir;
    if (nw > 1 && !strcmp (w[0], "back") && !strcmp (w[1], "up")) {
        d = (HUB->pc_face + 4) & 7;             /* ("Back up": behind it, still facing the way it does) */
    } else if (d == 0xFF && n) {
        d = HUB->pc_face;                       /* ("Step": ahead) */
    }
    if (n && d <= H_NW && !cmd.obj.kind) {             /* (A small move: its facing kept backing up, or a step aside) */
        keep = any ("left\0right\0l\0r\0back\0backward\0backwards\0behind\0b\0aside\0sideways\0sidestep\0") &&
            !any ("forward\0forwards\0fwd\0ahead\0front\0f\0");
        if (n == 0xFF) {                        /* (By the pace: a square at a walk or slower, two at a jog, three
                                                **   at a run, four at a sprint; as a search or a map holds it, one) */
            n = HUB->gait <= GA_WALK || (HUB->watch & (WA_TRAPS | WA_MAP)) ? 1 : HUB->gait - GA_WALK + 1;
        }
        intend2 (I_STEP, d | (keep ? STEP_KEEP : 0), n);
        return 1;
    }
    return !k;
}

/* The words p (one or more, a space between) among the line's, one after another: 1 if so (split's) */
static unsigned char __fastcall__ said (const char* p)
{
    unsigned char i, j, n;
    const char* q;

    for (i = 0; i < nw; ++i) {
        for (q = p, j = i; j < nw; ++j, q += n + 1) {
            n = strlen (w[j]);
            if (strnicmp (q, w[j], n) || (q[n] && q[n] != ' ')) {
                break;
            }
            if (!q[n]) {
                return 1;
            }
        }
    }
    return 0;
}

/* A journey to a place the character knows (HOUSE.md's travel, slice 4.2's; the user's, 2026-10-16: "I want to go
** back to the mausoleum"): the parser's move to a thing (main.c's command: "go to", "go back to", "walk back to",
** "head back to", "return to", "travel to", "make my way back to", the aliases' and the grammar's), whose thing is a
** room named (names.c's room_named: "the Mausoleum"), the way in ("the entrance", "the way in", "where I started",
** "the start"; "the way out", said going back), or a kind of room the theme names, the last of them it was in ("the
** kitchen"; said going back, the room words the parser knows: "the hall", "the cell").  The world's (I_TRAVEL): 1 if
** it's one, sent; else 0, the move as it was */
unsigned char __fastcall__ s_travel_go (const char* l)
{
    struct msg m;                               /* (On the C stack: the main RAM's none) */
    char k[16];
    unsigned char e, o = cmd.obj.kind, back, n, best = 0;

    if (o && o != OB_WAY_OUT && o != OB_ROOM) {
        return 0;                               /* (A thing it sees: walked to, as it was) */
    }
    strncpy (b, l, INMAX);
    split ();
    back = any ("back\0return\0travel\0way\0returning\0") != 0;
    memset (&m, 0, sizeof m);
    m.kind = I_TRAVEL;
    if (!o && (e = room_named (l)) != 0xFF) {
        m.b = e;                                /* (A room named: where it is, the names') */
        m.w1 = named_room.x;
        m.w2 = named_room.y;
        m.x[0] = named_room.w;
        m.x[1] = named_room.h;
        m.x[2] = named_room.sn;
    } else if ((!o && (any ("entrance\0start\0started\0began\0beginning\0") || said ("way in") ||
               said ("came in"))) || (o == OB_WAY_OUT && back)) {
        m.b = TRV_IN;
    } else if (!o || back) {
        for (e = 0; e < 8 + TUSES; ++e) {       /* (A kind of room, its words the theme's: the longest said) */
            hub_mem (THEME_BANK, k, e < 8 ? THEME->room[e] : THEME->uses[e - 8], sizeof k);
            k[sizeof k - 1] = 0;
            if (*k && (n = strlen (k)) > best && said (k)) {
                best = n;
                m.b = TRV_KIND | e;
            }
        }
        if (!best) {
            return 0;
        }
    } else {
        return 0;
    }
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "OV3CODE")            /* (Overlay 3's since 2026-10-29: overlay 9's room for the look) */
#pragma rodata-name (push, "OV3RODATA")

/* ---- A line's first look (main.c's, in the overlay's bank: the part's copied in only for a line that may be its):
** 0 if line l has none of the words s_pace_cmd acts on (split's words, the case aside), nor one begun with a figure.
** s_pace_cmd's words that matter: a pace's (paces), "status", the colours', the watchfulness's (without one, wa is the
** hub's), a small move's, a count's unit (a count is only a square's with one, or a figure at the end); so a word it comes
** to match must be here too */
static const char pace_words[] =
    "creep\0creeping\0sneak\0sneaking\0tiptoe\0crawl\0hop\0quietly\0stealthily\0slowly\0slow\0walk\0"
    "walking\0normal\0"
    "normally\0quickly\0jog\0jogging\0trot\0briskly\0run\0running\0hurry\0sprint\0sprinting\0dash\0"
    "status\0stop\0trap\0traps\0carefully\0careful\0warily\0listen\0listening\0mapping\0map\0"
    "colour\0color\0colours\0colors\0"
    "step\0sidestep\0shuffle\0back\0"
    "space\0spaces\0square\0squares\0steps\0pace\0paces\0foot\0feet\0ft\0yard\0yards\0";

unsigned char __fastcall__ o_pacing (const char* l)
{
    static const char* p;
    static unsigned char n, k;

    for (;;) {
        while (*l == ' ' || *l == ',' || *l == '.' || *l == '!') {
            ++l;
        }
        if (!*l) {
            return 0;
        }
        if (*l >= '0' && *l <= '9') {
            return 1;
        }
        for (n = 0; l[n] && l[n] != ' ' && l[n] != ',' && l[n] != '.' && l[n] != '!'; ++n) {
        }
        for (p = pace_words; *p; p += k + 1) {   /* (Each word's length once, its first letter first) */
            if ((k = strlen (p)) == n && *p == (*l | 0x20) && !strnicmp (l, p, n)) {
                return 1;
            }
        }
        l += n;
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
