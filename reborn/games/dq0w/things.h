/*
** things.h - (dq0w) the item store, the world's alone: things.c (its code in the main RAM, its swapped parts' and
** the catalogue's bank's), kinds.c (the catalogue), items.s (its records reached)
*/

/* ---- The main RAM's (things.c's, items.s's) */
struct item* __fastcall__ item_at (unsigned i); /* Item i's record, its bank in the window (items.s) */
unsigned __fastcall__ items (unsigned i, unsigned char in, unsigned x);  /* The next record after i in use and in
                                                **   one of in's places (IN_*), at x (IN_X) or with one of x's flags
                                                **   (IN_FLAGS): its number (0: none), ip its record, in the window */
extern struct item* ip;
#include "activity.h"                           /* (What the character's doing: struct pc_activity, act) */
#define IN_SQUARE       (1 << IW_SQUARE)
#define IN_ACTOR        (1 << IW_ACTOR)
#define IN_ITEM         (1 << IW_ITEM)
#define IN_PORTAL       (1 << IW_PORTAL)
#define IN_DEN          (1 << IW_DEN)
#define IN_X            0x80
#define IN_FLAGS        0x40
void owned (void);                              /* The things a look saw (the senses' record's) made their own, as
                                                **   the senses did as they saw them (make_own: no two alike) */

/* ---- The item store's paging (ipage.c, part 17's: bank.s's stubs): the chunks whose things are in the store, a
** table in the world's own bank (ovbank[OVBANKS]), and a chunk's things' records as they're read in or written out
** (the map); a chunk's things in its region's file, a group (its place in the region: x, and y times 8; its count; the
** clock as it was written, CLOCK_256's: its lights caught up, burnt down by the minutes since, as it's read in) */
#define NIRES           64
#define NNONE           16
struct ires {
    unsigned char   used, sn;
    unsigned        cx, cy;
    unsigned        stamp;                      /* When it was last wanted */
};
struct ipg {
    struct ires     r[NIRES];
    unsigned        stamp;
    unsigned char   lsn;                        /* (The chunk the character was in, as it_near last looked: its
                                                **   stratum plus 1, 0 none yet) */
    unsigned        lcx, lcy;
    struct {
        unsigned char sn, rx, ry;
    }               none[NNONE];                /* (Regions known to have no file of things yet: none read in
                                                **   for them, a part's swap spared; a region put out, none: sn its
                                                **   stratum plus 1, 0 the entry free) */
    unsigned char   nnext;
    unsigned        map[NITEMS];
};
#define IPG             ((struct ipg*) 0x8000)
#define IP_MAGIC        0x5049                  /* "IP": items.dat's table, after the store's banks */
struct igroup {
    unsigned char   at;
    unsigned        n;
    unsigned        m;
};
unsigned char it_room (void);                   /* A record made free (a chunk's things put out): 1, or 0 */
unsigned char __fastcall__ it_need (unsigned char sn, unsigned cx, unsigned cy);    /* A chunk's things in the store
                                                **   (read in, if they aren't): 1, or 0 */
unsigned char __fastcall__ it_out (unsigned char sn, unsigned cx, unsigned cy);   /* (Part 20's) A chunk's things
                                                **   written to its region's file, their records freed: 1, or 0 */
unsigned char __fastcall__ it_in (unsigned char sn, unsigned cx, unsigned cy);    /* (Part 20's) Read in: 1 (2, its region
                                                **   has no file), or 0 */
void __fastcall__ it_flush (unsigned char sn);   /* (Part 17's) Stratum sn's chunks' things all put out (its slot
                                                **   made way for: dq0w.c's slot_for) */
unsigned char __fastcall__ st_out (unsigned char sn);   /* (Part 22's: sfile.c) A stratum's record to its file */
unsigned char __fastcall__ st_in (unsigned char sn);    /* (Part 22's) Read back into its slot: 1, or 0 none */
unsigned char __fastcall__ stratum_in (unsigned char m);    /* (Part 3's: dq0w.c) Stratum m in its slot: 1, or 0
                                                **   (not made yet) */
void sky_note (void);                           /* (Part 76's: dq0w.c) The sky's opening by the character noted for the reckoning; */
void sky_list (void);                           /*   the daylight's emitters near it made again */
unsigned char __fastcall__ sky_ask (unsigned w);    /* (Part 89's: dq0w.c) The daylight's emitters, a shaft's going, a thing let fall */
#define SA_GO           0x8000
#define SA_UP           0x0100
#define SA_JUMP         0x0200
#define SA_END          0x2000
#define SA_HERE         0x1000
#define SA_WET          0x4000
void it_near (void);                              /* The chunks round the character's in the store, as it comes into
                                                **   one (dq0w.c's see, as it's moved) */
unsigned char rope (void);                      /* (Part 96's: wrope.c: IO_HIDE's, IO_ROPE) */
unsigned char __fastcall__ rope2 (unsigned w);  /* (Part 98's: wrope2.c) */
unsigned char __fastcall__ rope3 (unsigned w);  /* (Part 95's: wrope3.c) */
unsigned char __fastcall__ well (unsigned w);   /* (Part 103's: wwell.c, a well's shaft, dq0w.rop's: read in first) */
#define WL_DOWN         0x0000                  /* (The well within reach's shaft, to climb down: its link, 0xFF none, 0xFE
                                                **   choked, told; */
#define WL_ROPE         0x2000                  /*   a rope's drop down it, wrope.h's R_*'s: 2, or 0; */
#define WL_NEAR         0x4000                  /*   part 104's asks: the well within reach, 1 or 0; its shaft, as WL_DOWN's
                                                **   but untold; its foot there, the window on it: 1 or 0) */
#define WL_SHAFT        0x6000
#define WL_FOOT         0x8000
unsigned char well_put (void);                  /* (Part 104's: wwell2.c, the thing named let fall into a well, down a shaft;
                                                **   its words the things', IR_*) */
#define HF_DEEP         0x40                    /* (A fall's, s_fall's: into deep water, three feet or more, the book's;) */
#define HF_SHALLOW      0x20                    /*   into shallow water: rs.h's GR_*'s, not the referee's HF_*) */
void __fastcall__ start (const char* dir);      /* (Part 3's: dq0w.c) The world's start, once it's attached */
void __fastcall__ keep (unsigned char back);    /* The store kept (items.dat), or read back */
extern unsigned ilim;                           /* (things.c's: the records used, at most) */
extern unsigned ihigh;                          /* (things.c's: every record from it on free; raised as one's taken) */
extern unsigned irs;                            /* (things.c's: a thing's own numbers, made) */
#define OVBANKS         OVLS                    /* (dq0w.cfg's OV1 ..., the build's count: the parts' images; and four more, the world's
                                                **   own: ipage.c's, ovbank[OVBANKS]; the ways across a room's,
                                                **   dq0w.c's, ovbank[OVBANKS + 1]; the traces' store's, wtrace.c's,
                                                **   ovbank[OVBANKS + 2] and [OVBANKS + 3]) */
extern unsigned char ovbank[OVBANKS + 4];

/* ---- The swapped parts' (bank.s's stubs) */
unsigned char serve (void);                     /* (Part 10) The hub's ireq answered (but its state): 1 if what lies
                                                **   about has changed (the world looks again) */
unsigned char things_req (void);                /* The same, what's carried listed, a tool's wear, by part 11 alone */
void __fastcall__ items_begin (const char* dir);    /* (Part 13) The catalogue into the hub (dir's dq0w.cat), the store
                                                **   emptied, a game carried on its read, its light */
void __fastcall__ stock_down (unsigned char k); /* (Part 13) Cache slot k's chunk's things put down (the maker's
                                                **   list, the world bank's stock) */
unsigned __fastcall__ lay_body (unsigned char kind, unsigned char sn, unsigned x, unsigned y);   /* (Part 13) A denizen's
                                                **   remains put down where it fell (KI_BODY, KI_BONES) */
unsigned char puzzle (void);                    /* (Part 16: wgame.c) The hub's ireq, IO_PUZZLE's, answered (but its
                                                **   state): 1 if what lies about has changed */
void __fastcall__ take_go (const struct msg* m);    /* (Part 16) I_TAKE's walk begun; or, m under NTKINDS (its one stub, the
                                                **   main RAM's room), that theme's kind come into the character's hands:
                                                **   its take rule, if it has one */
unsigned char take_up (void);                   /* (Part 16) Its thing taken up, the walk done: 0 not (E_READY to
                                                **   tell), 1 taken (the UI's to go on), 2 walking on to it */
unsigned char take_rest (void);                 /* (Part 4: things.c) A count to take up, walked over to first, taken
                                                **   as it gets there (IR_NONE: none) */
unsigned char __fastcall__ fumbled (unsigned char k);  /* (dq0w.c's part 25) A grope while engaged, a thing of kind
                                                **   k handled: 1 if it failed (its words in the hub's text) */
extern unsigned char noinit;                    /* (dq0 -I: no grope's test while engaged: the tests') */
unsigned char __fastcall__ in_reach (unsigned n);
unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);
/* The reach (HOUSE.md's, 2026-10-26: part 10's): how readily a thing comes to hand, by where it is, its time owed as a
** handling's done, the character's next spell's (dq0w.c's busy, pextra) */
unsigned __fastcall__ handy (unsigned i);       /* (Whether thing i is handy: 0, or the thing it's in that isn't open
                                                **   to the hand) */
void __fastcall__ reach_from (unsigned n);      /* (Thing n got out, to hand) */
void __fastcall__ reach_to (unsigned c);        /* (Container c reached into: a thing put in it, looked for in it) */
unsigned char __fastcall__ tool_reach (const char* keys);    /* (Part 4's: a tool used got out, the first carried of one
                                                **   of those keys, each 0-ended, an empty one last: 1 a step begun) */
unsigned __fastcall__ reach_done (unsigned char said);  /* (What's owed added, its time back; said, a pack taken off
                                                **   for it told; then none owed) */
#define RD_SAID         0x01                    /* (reach_done's: a pack taken off for it told; */
#define RD_LONG         0x02                    /*   a long task's, not added: its length the more, the caller's) */
extern unsigned rtime, roff, pextra;            /* (What's owed: its time, tenths; the pack taken off for it) */
extern unsigned char rsteps, rbody;             /* (Its steps, a fight's rounds: its levels; one, on the body) */
/* The reach's steps (part 84's, 2026-11-02, the user's: each its words, its time, its grope; HOUSE.md's) */
struct step {                                   /* A step of a long task (ENGINE-CRITIQUE.md's step planner, 3.3): */
    unsigned char edge;                         /*   what its end does, SE_* (an edge of the tree: a pack taken off, a level
                                                **   reached into, a thing got out, put back, set down, taken up ...); */
    unsigned char say;                          /*   its words, the UI's: SV_* | SM_* (hub.h's; E_TASK's TE_BEGUN
                                                **   kind; the verb alone I_WAIT's task, the long task's kind); */
    unsigned char what;                         /*   its thing's kind, as it's said; */
    unsigned thing;                             /*   the thing it's done to (its handle: the item); */
    unsigned apart;                             /*   the thing told apart in its words ("the spent torch"; 0 none); */
    unsigned time;                              /*   its time, tenths (engaged, a round) */
};
enum { SE_OFF = 1, SE_IN, SE_ON, SE_DROP, SE_TRY, SE_OUT, SE_LIGHT, SE_STOW, SE_UP, SE_HAND, SE_BACKIN, SE_PLACE, SE_SLING };   /* (A
                                                **   pack taken off; a level reached into, held open, a thing got out;
                                                **   a pack put on; a thing let fall, set down; a spark struck; a light
                                                **   blown out; one lit from another; put in the pack; taken up; a
                                                **   tool got out to hand; put back; put in its place (a sconce, a
                                                **   sheath); slung) */
extern struct step rstep;                       /* (Part 85's: the step under way) */
unsigned char __fastcall__ reach_step (unsigned n, unsigned c);   /* (Thing n got out, container c reached into: 1 if a
                                                **   step's begun, the handling to wait, its caller's nothing done;
                                                **   0 go on.  c RS_TOOL: n a tool, got out, put away after its use) */
#define RS_TOOL         0xFFFF
unsigned char __fastcall__ reach_do (unsigned w);   /* (Part 85's: RO_*, or an intent's tool's keys: s_reach_do's) */
enum { RO_END, RO_ASK, RO_REPLY, RO_FUMBLED, RO_GO, RO_REDIP, RO_SPILL, RO_HONE, RO_NEWL };
#define RS_GO           0xFFFB                  /* (reach_step's c: the step rstep, begun, the caller's) */
#define RS_FIRE         0xFFFC                  /* (reach_step's c: n a fire-maker, a tool's steps and its tries; 2 empty) */
#define RS_BACK         0xFFFE                  /* (reach_step's c: the pack back on, its own step) */
unsigned char __fastcall__ rs_for (unsigned n);  /* (Part 86's: the hand the pack goes in; or RF_*) */
#define RF_DRAW         0xFFF0                  /* (A tool's next step set: 1; 0 on with its use) */
#define RF_FIRE         0xFFF1                  /* (A fire-maker's: the same, its tries; 2 empty) */
#define RF_TRIED        0xFFF2                  /* (A try's end: its roll, told) */
#define RF_CUT          0xFFF3                  /* (The sequence over part way: the pack off told) */
#define RF_PICKS        0xFFF4                  /* (An intent's tool got out first: lockpicks; a digger's; the door's key, wk's) */
#define RF_DIGGERS      0xFFF5
#define RF_DOORKEY      0xFFF7
#define RF_ROLL         0xFFF6                  /* (The dark's test of the hands, its grade's good: 1; in the light 1) */
void rs_did2 (void);                            /* (Part 75's: a step's end, part 86's rs_did's SE_HAND on) */
unsigned char __fastcall__ rs_more (unsigned char w);  /* (Part 90's: 0 hands made free for rs_tool, 1 a step's begun; 1 a
                                                **   step's end, part 86's rs_did's SE_HAND on) */
#define RF_TELL         0x8000                  /* (| TE_*: a word to the UI, E_TASK's, b rstep.what; TE_BEGUN's kind rstep.say) */
extern unsigned rs_tool;                        /* (The tool in use; a fire-maker to hand, part 10's flame's) */
extern unsigned char rs_free;
extern unsigned handled;                        /* (The thing a request handles: never put away to free a hand for its tool) */
extern unsigned left_lit;                       /* (Part 90's: a light the hands' rule set down burning, 0 none) */
void __fastcall__ lit_back (unsigned char k);   /* (Part 90's: it taken up again as a walk starts, k the intent's kind; left behind
                                                **   told, | LB_GO, the walk begun) */
#define LB_GO           0x80
extern unsigned char rs_using;                  /* (A tool's: 1 out; 2 caught, a fire-maker's) */
#define LW_REACH        0xFE                    /* (act.work's: a step of the reach, its end RO_END's) */
extern unsigned rs_pack;                        /* (The pack off for this handling) */
extern unsigned char rs_ok, rs_st, rs_paid, rs_after_on, rs_live;
extern unsigned lw_x, lw_y;                     /* (dq0w.c's: a timed work's square; a redip's, things.c's rs_old, rs_down) */
extern unsigned char den_saw;                   /* (wbody.h's: the world to look about again) */
extern struct msg lw_m;
void equip_now (void);                          /* (wlimbs.c's) */
void __fastcall__ intent_more (struct msg* m);  /* (dq0w.c's part 37) */
void __fastcall__ intent_few (struct msg* m);   /* (dq0w.c's part 53) */
extern unsigned char held_k[2], held_h[2];      /* (The main RAM's, things.c's: what the character holds, the main
                                                **   hand's kind first, and the hands they're in, IH_*: held_kinds') */
void held_kinds (void);
unsigned char __fastcall__ shand_of (unsigned char s);  /* (The hand that holds the thing that strikes: the hub's
                                                **   pc_shand) */
void light_now (void);
void ix_make (void);                            /* (What it carries, for the UI's first words: part 102's) */
unsigned char __fastcall__ tell_apart (unsigned n);
unsigned char __fastcall__ doff_feet (unsigned n);  /* (A thing taken off, no hand free: at its feet, part 48's) */
void __fastcall__ slide (unsigned n);          /* (Part 61's: what's on thing n's lid slid off as it's opened, told; | SL_QUIET,
                                                **   a denizen's opening, untold: every actor's, 2026-11-17) */
#define SL_QUIET        0x8000
void __fastcall__ name_apart (unsigned i, char* d);    /* (Its name told apart, into d: part 102's) */  /* (Its state's word, AW_*, beside another of its kind at hand: part 92's) */
void __fastcall__ go_to (unsigned x, unsigned y);

/* (The things' words: items.h's SA_*, said.txt's SA rows, coded in things_text: narration as data, stage 5) */

/* ---- The lights burning (slice 4.1's: DUNGEON.md's light, HOUSE.md's burning): how each kind burns, and what renews
** it (kinds.c's burns, in the catalogue's bank: a light's whole life and its fuels; a fuel's measure and what it is) */
struct burn {
    unsigned char   kind;                       /* (KI_*) */
    unsigned char   life;                       /* A light's whole life (a fuel's, what a whole one holds): tens of
                                                **   minutes of burning */
    unsigned char   fuels;                      /* FU_*: a light's, those that renew it (0: none, it burns away); a
                                                **   fuel's, what it is */
};
#define NBURNS          12
#define FU_OIL          0x01
#define FU_PITCH        0x02
#define FU_DRAW         0x04                    /* (Used up as it's drawn with: charcoal, chalk, a pencil, a quill, ink;
                                                **   HOUSE.md's things used up, 2026-10-26: no light, no fuel) */
#define BURN_LOW        10                      /* Burning low: its last ten minutes (the house rule's) */
extern const struct burn burns[NBURNS];
struct snap {                                   /* (What breaks as it's used: kinds.c's snaps, a missile's, slice 4.7's) */
    unsigned char   kind;                       /* (KI_*) */
    unsigned char   chance;                     /* (In a hundred, shot or thrown) */
};
#define NSNAPS          1
extern const struct snap snaps[NSNAPS];
unsigned char __fastcall__ s_use_up (unsigned n, unsigned char by);  /* (dq0w.c's part 7: thing n used up by minutes,
                                                **   or a lot by one; gone, told: 1, else 0) */
unsigned body_ask (void);                       /* (Part 70's, wtend.c: the bodies' time told the referee: those dead
                                                **   of it, and times 256 those that bound their bleeding) */
unsigned char __fastcall__ use_key (const char* key);   /* (Part 7's: a thing used up by its key, IO_USE) */
extern unsigned chart_w[5];                     /* (things.c's: the map making's things handy, by their keys, part 26's
                                                **   chart_now's: a sheet, a map, a point, a quill, ink) */
const struct burn* __fastcall__ burn_of (unsigned char k);  /* How kind k burns (a copy), or 0: never (a glow), or no
                                                **   fuel; the catalogue's bank left in the window */
unsigned char fill (void);                      /* (Part 27, its room) The hub's ireq, IO_FILL's: a light renewed */
/* The water, hunger and thirst (part 73's, wwater.c: slice 4.9's) */
#define WADE_MORE       0x8000                  /* (wade's: the time given, not the water's, 2026-11-03) */
unsigned __fastcall__ wade (unsigned step);     /* (Part 73's, dq0w.c's) The character come onto its square: the time
                                                **   more its step takes (wading, swimming) */
unsigned __fastcall__ water (unsigned char k);  /* (Part 74's, wwater.c's s_water: W_FILL a waterskin filled, the
                                                **   things' IO_FILL: IR_*, IR_NONE not a waterskin's) */
enum { W_AT, W_FILL };
unsigned char __fastcall__ water_at (unsigned char well);  /* (Part 75's, wwater.c's: the water's depth where the
                                                **   character stands, feet, | WET_RUNS, 0 dry; well, a well beside
                                                **   it first, 1) */
void burns_now (void);                          /* (Part 27: dq0w.c) The lights burnt down by the minutes gone */
extern unsigned long burn_due;                  /*   (when the next minute's due: the main loop's) */
void chart_now (void);                          /* (Part 13) */
unsigned char near_one (void);                  /* (dq0w.c's: anyone beside the character: zone_step's parts spared) */
extern unsigned char chart_late;                /* (things.c's: the map making's state to make again: the main loop's) */
extern unsigned char burn_ask;                  /*   (or at once: a light's change; | TS_ASK, the time's */
#define TS_ASK          2                       /*   alone: a wait begun or ended, a black-out ..., part 76's; */
#define BA_SCONCES      4                       /*   BA_SCONCES, a minute's second pass, part 27's) */
#define CLOCK_256       (*(unsigned*) ((unsigned char*) &HUB->now + 1))   /* (The clock in 25.6 seconds, 16 bits: its
                                                **   tenths' middle bytes, no long division: ipage.c's stamps) */
/* ---- Where a thing's worn (part 107's: wworn.c, the user's, 2026-11-10; HOUSE.md's worn places) */
unsigned char __fastcall__ worn_at (unsigned n, unsigned k);  /* (Thing n, of kind k, to be worn: 0, its place in wplace;
                                                **   else SA_*, or 0xFF its words said: the catalogue's don's) */
extern unsigned char wplace;
void __fastcall__ worn_put (unsigned n, unsigned char how);    /* (Where thing n's worn, in words: WH_*) */
enum { WH_LIST, WH_LOOK, WH_DEN, WH_THEY };    /* (The inventory's ", on your left hand" (else ", worn"); a look's
                                                **   " on your left hand", after "worn"; a denizen's " on its left hand";
                                                **   a person's " on their left hand", the user's, 2026-11-17) */
unsigned char worn_which (void);                /* ("Take off the ring", worn in two places or more: IR_WHICH, asked by
                                                **   place; else 0) */
