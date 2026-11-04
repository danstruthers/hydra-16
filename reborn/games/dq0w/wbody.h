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
extern unsigned char bleeds;                    /* The bodies bleeding: bit k, entry k */
extern unsigned bled_at;                        /* When their bloodloss was last reckoned: the clock's low word */
extern unsigned char den_done;                  /* The denizens' turn they've done (their I_DONE's: the hub's den_go) */
extern unsigned char den_saw;                   /* A body fallen: the world to look about again */

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
void den_intents (void);                        /* (Part 14) The denizens' intents carried out (the hub's di), they
                                                **   rung after */
#define DEN_INTENTS()   (HUB->di_tail != HUB->di_head)  /* (Some to carry out: the hub's header in the window) */
void pc_blow (void);                            /* (Part 15) The character's blow at the actor it's posted (the hub's
                                                **   pc_foe) */
void __fastcall__ den_blow (unsigned char def, unsigned char z);  /* (Part 15) Body bd's blow at def (I_ATTACK's) */
void __fastcall__ den_rob (const struct msg* m);    /* (Part 15) The character robbed by body bd, what it takes
                                                **   put down at m's w1, w2, or in its faction's hoard there, or hidden
                                                **   there (I_ROB's) */
void __fastcall__ den_fetch (const struct msg* m);  /* (Part 72: wfetch.c) Body bd's errand: a thing fetched taken
                                                **   up, handed over; a gift taken (I_ROB's a: slice 4.8's) */
void __fastcall__ den_take (const struct msg* m);   /* (Part 81: wtake.c) Body bd's own take, by the take rule (I_TAKE's: a
                                                **   its wants, TW_*; Phase 3C's one path) */
void bled (void);                               /* (Part 15) The bleeding's bloodloss, a minute at a time */
extern unsigned long body_due;                  /* (Part 70's: the referee's next watch, wtend.c's) */
unsigned char __fastcall__ den_first (unsigned char k, unsigned char away);    /* (Part 14) Body k's tactical
                                                **   advantage: backed away (away 1), 2; or its blow first, 1 */
unsigned char __fastcall__ unlit_at (unsigned char sn, unsigned x, unsigned y);  /* (dq0w.c's) A square dark to
                                                **   a denizen: unseen lit, no room's own light (the chunk in the window) */
void equip_now (void);                          /* (Part 49: wlimbs.c) What's to hand told the referee at once */
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
unsigned char __fastcall__ den_draw (unsigned char want);  /* (Part 60) Its weapon drawn from its belt (or one it put
                                                **   up), its hands empty of one: 1 (its round's), or 0; want not 0, a
                                                **   thing of its own of that kind taken in hand, what it holds put up
                                                **   (a launcher, a thing to throw: wshoot.c's) */
void __fastcall__ den_strip (unsigned body);    /* (Part 60) Dead: its things on its body (body, its remains), what
                                                **   it held beside it */
void __fastcall__ limb_news (unsigned w, unsigned char b);  /* (Part 49) The limbs' news, the denizens' ring's (E_BEAT's
                                                **   w; b whose: 0 the character, else the actor's entry plus 1) */
unsigned char __fastcall__ engaged (unsigned char eng, unsigned char def);  /* (dq0w.c's part 25) The engage
                                                **   initiative test: eng engaging def (0 the character, else an
                                                **   actor's entry plus 1) */
extern unsigned char noinit;                    /* (dq0 -I: no engage initiative test: the tests') */

#endif
