/*
** wbody.h - (dq0w) the denizens' bodies (DENIZENS.md: in the tasks), the world's as the character's is: what the
** main code (dq0w.c) and their swapped parts share, in the main RAM: wbody.c's part 14 (their intents carried out),
** wfight.c's part 15 (the blows, the bleeding).
*/

#ifndef _WBODY_H
#define _WBODY_H

extern struct actor bd;                         /* The body at hand, copied out of the hub's header (bd_get) */
extern unsigned char bk;                        /*   its entry */
extern unsigned bpx, bpy;                       /*   and the character's square and stratum, as the hub has them */
extern unsigned char bps;
extern struct msg bev;                          /* An event told */
extern unsigned char evb;                       /* (dq0w.c's: the world's events' one sender, work.h's too: kind, who,
                                                **   w1, evb its b; the hub's header in the window after) */
void __fastcall__ event (unsigned char kind, unsigned char who, unsigned w1);
extern unsigned char bleeds;                    /* The bodies bleeding: bit k, entry k */
extern unsigned bled_at;                        /* When their bloodloss was last reckoned: the clock's low word */
extern unsigned char den_done;                  /* The denizens' turn they've done (their I_DONE's: the hub's den_go) */
extern unsigned char den_saw;                   /* A body fallen: the world to look about again */

/* A square's time to walk into, every actor's (dq0w.c's, the main RAM's: one body rule, GAME-CRITIQUE.md's E4,
** 2026-11-17): its step s, its rise dz, how (SC_*: on steps, a diagonal, its multiplier) */
unsigned __fastcall__ step_cost (unsigned char s, int dz, unsigned char how);
#define SC_STEPS        0x01                    /* (Onto steps: half again) */
#define SC_DIAG         0x02                    /* (A diagonal: seven feet to a square's five) */
#define SC_MUL(m)       ((m) << 2)              /* (Its time times m: a slow pace's, lame, crawling, feeling its way) */

/* Body k copied out (bd, bk; the character's place, bpx ...), and back (its body's bytes only: its mind's the
** denizens'); the hub's header in the window after (each part its own: BD_GET, BD_PUT) */
#define BD_GET(k)       (bk = (k), hub_map (0), memcpy (&bd, &HUB->actor[bk], sizeof bd), bpx = HUB->pc_x, \
                         bpy = HUB->pc_y, bps = HUB->pc_stratum)
#define BD_PUT()        (hub_map (0), memcpy (&HUB->actor[bk], &bd, offsetof (struct actor, name)))

/* Hiding oneself (HOUSE.md's: dq0w.c's part 94, hid_op's op): the character's, begun (I_RISE's POS_HIDE, the UI's), its time
** up, a stop at a walk's end, a search, a blow, ended (HO_END | HW_*: its words); a denizen's, body bd (HO_DEN | HD_*: its mind's) */
#define HO_GO           0
#define HO_DONE         1
#define HO_MOVED        2
#define HO_SEARCH       3
#define HO_BLOW         4                       /* (Body bd in a blow, the attacker if att is its entry plus 1) */
#define HO_BLOWN        5                       /*   (the same, bd the defender) */
#define HO_THING        6                       /* (A thing handled, the hub's ireq: if it's a doing) */
#define HO_END          0x20
#define HO_STOP         0x80                    /* (A stop with no long task: | the kind it names) */
#define HO_DEN          0x40
void __fastcall__ hid_op (unsigned char op);

/* A blow's missile (wfight.c's: RS_MISSILE's, set before part 15's shot_blow by wshoot.c's, 0 after): what it's shot
** with or the thing thrown, its range's band, MS_* (rs.h's; its range none, 2026-10-30: no referee's) */
struct mshot {
    unsigned char   kind, band;
    unsigned        flags;
};
extern struct mshot mshot;
#define AIM_FIRE        0x20                    /* (den_blow's z: a missile's blow, mshot set: part 65's) */
unsigned char __fastcall__ shot_op (unsigned char op, unsigned char k, unsigned char z);   /* (Part 65's: wshoot.c)
                                                **   The missiles' calls: */
unsigned char __fastcall__ pc_shot (unsigned char k);  /* (Part 67's) The character's shot at actor k; 0xFF: the
                                                **   nearest foe it can shoot at, plus 1 (0 none); PS_DEN body bd's
                                                **   shot or throw at the character; PS_UNBOW its launcher let fall */
#define PS_DEN          0xFE
#define PS_UNBOW        0xFD
#define PS_AUTO         0xFC                    /* (The nearest foe within the auto-target range, the hub's pc_auto, its
                                                **   launcher taken as what it shoots with: plus 1, 0 none) */
enum { SO_ARMS, SO_REACH, SO_DEN };             /* what an actor shoots with (k: 0 the character, 1 body bd); whether
                                                **   the character can shoot at k (plus 1: 2, 1 senseless, 0 not);
                                                **   what body bd has to shoot or throw, and a shot (DS_*) */
#define DS_BOW          0x01                    /* (SO_DEN's: its launcher and a missile, not in hand; in hand; */
#define DS_HELD         0x02
#define DS_THROW        0x04                    /*   a thing to throw in hand; to draw; */
#define DS_DRAW         0x08
#define DS_SPENT        0x40                    /*   its launcher spent in its hand; a shot from there) */
#define DS_LINE         0x80
struct stray {                                  /* (A miss's stray's: part 67's, from where it was shot at whom, the
                                                **   thing thrown) */
    unsigned char   sn;
    unsigned        sx, sy, tx, ty, thr;
};
unsigned __fastcall__ gath_op (unsigned char op, unsigned char a);  /* (Part 66's) The missiles gathered (begun, a
                                                **   their family; its next, 2 on) */
enum { GO_GATHER, GO_ON };
unsigned load_go (void);                        /* (Part 68's) A launcher loaded: its time */

/* ---- The parts' (bank.s's stubs) */
extern unsigned char kit_off;                   /* (dq0w.c's: a denizen's things given way to a command, 2; and 1, its lair's keys made) */
unsigned char ui_first (void);                  /* (dq0w.c's) The UI's command in hand: the denizens' intents wait */
unsigned char ui_long (void);                   /* (And a long job, a line being typed too) */
unsigned char den_held (void);                  /* (dq0w.c's) The denizens' intents held: as main's job a line's keys
                                                **   too, else the command in hand alone (ui_first's) */
void den_intents (void);                        /* (Part 14) The denizens' intents carried out (the hub's di), they
                                                **   rung after */
#define DEN_INTENTS()   (HUB->di_tail != HUB->di_head)  /* (Some to carry out: the hub's header in the window) */
void pc_blow (void);                            /* (Part 15) The character's blow at the actor it's posted (the hub's
                                                **   pc_foe) */
void __fastcall__ den_blow (unsigned char def, unsigned char z);  /* (Part 15) Body bd's blow at def (I_ATTACK's) */
void __fastcall__ den_close (unsigned char def, unsigned char z);  /* (Part 106: wfight.c) Body bd's attack begun at def,
                                                **   its aim z: closed with the character, told first (DE_CLOSES) */
extern unsigned long blow_t;                    /* (wfight.c's: the moment body bd's blow was meant, its intent's (its
                                                **   ready as part 14 took it); 0, the clock's: 2026-11-13) */
extern unsigned char grip;                      /* (wfight.c's: the actor the character holds, plus 1, | GH_PIN pinned;
                                                **   0 none: HOUSE.md's grappling, 2026-11-09) */
#define GRIP_OF(g)      ((g) & 0x0F)
void __fastcall__ grip_off (unsigned char told);    /* (Part 37) The hold let go (told 1: "You let go of ...", or
                                                **   none held; 0 quietly) */
void __fastcall__ grip_held (struct msg* m);    /* (Part 106) Body bd's intent m, if it's held: its try to break free */
struct gleft {                                  /* (A blow's, for grip_left: its attacker, defender, aim, what it left */
    unsigned char   att, def, z, ff;            /*   them in) */
};
extern struct gleft glf;
void __fastcall__ grip_left (unsigned char out);    /* (Part 106) The hold as a blow left it (glf's; how it came out) */
void __fastcall__ den_rob (const struct msg* m);    /* (Part 15) The character robbed by body bd, what it takes
                                                **   put down at m's w1, w2, or in its faction's hoard there, or hidden
                                                **   there (I_ROB's) */
unsigned char __fastcall__ den_pry (unsigned n, unsigned char how);    /* (Part 69: wfight.c) Thing n, in another,
                                                **   reached by body bd by its compartments' rules (how: PR_*): 1, or 0
                                                **   left; its time on its ready (2026-11-17) */
#define PR_KNOWN        0x01                    /* (A hidden compartment the character's found reached: an errand's) */
#define PR_QUIET        0x02                    /* (A lock not forced: an errand's, a robbery unawares) */
#define PR_TIMED        0x04                    /* (Each opened its two seconds: not a robbery's, its reach's level's) */
unsigned __fastcall__ tool_of (unsigned char who, unsigned char k, unsigned id);   /* (Part 80: wport.c) A thing of
                                                **   kind k carried by actor who (0 the character, else body bd), a key
                                                **   one that fits lock id: its number, or 0 (2026-11-21) */
signed char __fastcall__ act_test (unsigned char who, unsigned char p, signed char steps);
void __fastcall__ den_fetch (const struct msg* m);  /* (Part 72: wfetch.c) Body bd's errand: a thing fetched taken
                                                **   up, handed over; a gift taken (I_ROB's a: slice 4.8's) */
void __fastcall__ den_take (const struct msg* m);   /* (Part 81: wtake.c) Body bd's own take, by the take rule (I_TAKE's: a
                                                **   its wants, TW_*; Phase 3C's one path) */
void bled (void);                               /* (Part 15) The bleeding's bloodloss, a minute at a time */
void __fastcall__ den_told (unsigned char what, unsigned char loud, unsigned x, unsigned y);   /* (Part 80: wport.c) Body
                                                **   bd's doing (DE_*), how loud (0-9, | DT_*), at square x, y, told:
                                                **   E_DEN, a door worked, a tunneller's, a builder's (wdig.c's) */
#define DT_SEEN         0x80                    /* (den_told's loud |: seen whatever its square shows now: a dug square */
#define DT_DOOR         0x40                    /*   as it was; a door's work: what lies between from the door, seen too
                                                **   if the body's own square is) */
extern unsigned long body_due;                  /* (Part 70's: the referee's next watch, wtend.c's) */
unsigned char __fastcall__ den_first (unsigned char k, unsigned char away);    /* (Part 14) Body k's tactical
                                                **   advantage: backed away (away 1), 2; or its blow first, 1 */
unsigned char __fastcall__ unlit_at (unsigned char sn, unsigned x, unsigned y);  /* (dq0w.c's) A square dark to
                                                **   a denizen: unseen lit, no room's own light (the chunk in the window) */
void equip_now (void);                          /* (Part 49: wlimbs.c) What's to hand told the referee at once */
unsigned __fastcall__ out_still (unsigned char who);    /* (Part 52: dq0w.c) A senseless one's shock rolls due
                                                **   asked (RS_ROUSE): the tenths it's senseless still, 0 come round */
void __fastcall__ drop_held (unsigned char h, unsigned char ring);   /* (Part 49) What the character holds in hands h
                                                **   (DH_*) let go of, fumbled: told in the denizens' ring (ring 1) or
                                                **   the world's */
void harm_fx (void);                            /* (Part 49) A harm's effects: dropped, prone; a fall's, its bearings */
void __fastcall__ bearings_off (unsigned feet); /* (Part 52: dq0w.c) The character's bearings shaken: 0, a harm's (a
                                                **   fall's), or BO_OUT, knocked senseless (slice 4.2's) */
#define BO_OUT          0xFFFFU
unsigned char den_reach (void);                 /* (Part 60: wlimbs.c) Body bd's weapon let fall, in its reach */
void den_took (void);                           /* (Part 60) Taken up again, told */
void __fastcall__ den_word (const char* t, unsigned char n);   /* (Part 51) Its weapon's word: line n of t */
unsigned char __fastcall__ look_more (unsigned n, unsigned char mode);   /* (things.c's stub: LM_ROPES, dq0w.rop read in) */
void __fastcall__ den_drop (unsigned char b);                           /* (Part 60) Its weapon on the floor, told */
unsigned char den_legs (void);                  /* (Part 51) Its legs at a move's start: 1 no move */
void __fastcall__ den_other (const struct msg* m);  /* (Part 60) A denizen's things made (I_KIT's) */
void __fastcall__ den_more (const struct msg* m);    /* (Part 80: wport.c) Body bd's rarer intents (part 14's room):
                                                **   come along (I_CLIMB), a robbery (I_ROB), its door work (I_PORTAL:
                                                **   Phase 3C's one path, the character's parts); and a lair's doors'
                                                **   keys made (I_KIT's KT_LAIR: part 51's) */
unsigned char __fastcall__ pc_dig (unsigned char w);    /* (Part 84: dq0w.c) The character's dig's own */
void __fastcall__ den_dig (const struct msg* m);    /* (Part 79: wdig.c) A tunneller's dig (I_KIT's KT_DIG: slice
                                                **   4.11's), a spell at a time */
void __fastcall__ den_enter (const struct msg* m);  /* (Part 60) Come near (I_ENTER's): its body's from it, its
                                                **   things in the store, the referee told */
unsigned char __fastcall__ den_draw (unsigned char want);  /* Its weapon drawn from its belt (or one it put
                                                **   up), its hands empty of one: 1 (its round's), or 0; want not 0, a
                                                **   thing of its own of that kind taken in hand, what it holds put up
                                                **   (a launcher, a thing to throw: wshoot.c's).  The main RAM's: part
                                                **   60's den_draws asked only of one with a thing of its own */
unsigned char __fastcall__ den_draws (unsigned char want);  /* (Part 60) den_draw's work */
void __fastcall__ den_strip (unsigned body);    /* (Part 60) Dead: its things on its body (body, its remains), what
                                                **   it held beside it */
void __fastcall__ limb_news (unsigned w, unsigned char b);  /* (Part 49) The limbs' news, the denizens' ring's (E_BEAT's
                                                **   w; b whose: 0 the character, else the actor's entry plus 1) */
unsigned char __fastcall__ engaged (unsigned char eng, unsigned char def);  /* (dq0w.c's part 25) The engage
                                                **   initiative test: eng engaging def (0 the character, else an
                                                **   actor's entry plus 1) */
void __fastcall__ time_kept (unsigned n);       /* (Part 99's stub, wtime.c's: the clock's minute n; past any minute,
                                                **   POOL_OP's codes, lamp oil's pools (the user's H15) and the flasks'
                                                **   oil (H16), wpool.c's pool_op: no stub of their own, the main RAM's
                                                **   room) */
#define POOL_OP         0xFFF0                  /* (The character stepped onto its square, + 1 body bd: slipped in it,
                                                **   prone, or burnt in it) */
#define POOL_ADD        0xFFF9                  /* (+ 0 the character, 1 body bd: oil poured out where it stands) */
#define POOL_TIME       0xFFFB                  /* (Their minute) */
#define POOL_MARK       0xFFFC                  /* (The things' numbers so far marked, an outfit to be made) */
#define POOL_FILL       0xFFFD                  /* (The flasks of oil made since the mark filled) */
extern unsigned char npools;                    /* (dq0w.c's: how many pools there are, its low bits; | PC_WOKE, the
                                                **   character struck in its sleep, K11's, wfight.c's) */
#define PC_WOKE         0x80
extern unsigned char noinit;                    /* (dq0 -I: no engage initiative test: the tests') */

/* The step and its hazards, every actor's (wstep.c, part 120: the one-path plan's stage 2, 2026-11-23) */
unsigned char __fastcall__ den_hz (unsigned char d);   /* (dq0w.c's) Body bd's step its way d (before it: 1 it goes no
                                                **   further, in its pit, a trap ahead noticed), or DT_ON (on its square
                                                **   after: 1 dead of it): the step's cheap test (a pit it's in, a chunk
                                                **   with traps, a pool), then part 120's den_trap */
#define DT_ON           0xFF
unsigned char __fastcall__ den_trap (unsigned char op, unsigned char d);  /* (Part 120) den_hz's work: its step's way d */
#define DT_NOTICE       0                       /* (Before its step: in its pit, a round's climb; a hidden trap ahead
                                                **   noticed, it stops short) */
#define DT_SPRING       1                       /* (On x, y: the trap there set off) */
#define DT_PIT          2                       /* (den_trap's: in a pit, a round's climb out) */
void den_harm (void);                           /* (Part 121) Body bd's harm (RS_HARM, its actor: the world's slot's
                                                **   request made, workpart.h's harm's) */
void __fastcall__ trap_beat (unsigned char kind, unsigned w1);  /* (Part 120) A trap's outcome for body bd, told (an
                                                **   actor's work's E_TRAP: dq0w.c's work_told) */
extern unsigned char bclimb[NACTORS];           /* (wbody.c's: each body's climb out of its pit so far, feet) */

#endif
