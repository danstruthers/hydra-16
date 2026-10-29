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

/* A blow's missile (wfight.c's: RS_MISSILE's, set before part 15's shot_blow by wshoot.c's, 0 after): what it's shot
** with or the thing thrown, its range's band, its range, MS_* (rs.h's), the circumstances' steps */
struct mshot {
    unsigned char   kind, band, dist;
    unsigned        flags;
    signed char     steps;
};
extern struct mshot mshot;
unsigned char __fastcall__ shot_op (unsigned char op, unsigned char k, unsigned char z);   /* (Part 65's: wshoot.c)
                                                **   The missiles' calls: */
void __fastcall__ pc_shot (unsigned char k);    /* (Part 67's) The character's shot at actor k */
enum { SO_ARMS, SO_REACH, SO_FOE, SO_DEN, SO_UNBOW };  /* what the character shoots with; whether it can shoot at k
                                                **   (plus 1: 2, 1 senseless, 0 not); the nearest foe it can, plus 1;
                                                **   body bd's shot at the character (aim z); its launcher let fall */
unsigned __fastcall__ gath_op (unsigned char op, unsigned char a);  /* (Part 66's) The missiles gathered (begun, a
                                                **   their family; its next, 2 on) */
enum { GO_GATHER, GO_ON };
unsigned load_go (void);                        /* (Part 68's) A launcher loaded: its time */

/* ---- The parts' (bank.s's stubs) */
void den_intents (void);                        /* (Part 14) The denizens' intents carried out (the hub's di), they
                                                **   rung after */
#define DEN_INTENTS()   (HUB->di_tail != HUB->di_head)  /* (Some to carry out: the hub's header in the window) */
void pc_blow (void);                            /* (Part 15) The character's blow at the actor it's posted (the hub's
                                                **   pc_foe) */
void __fastcall__ den_blow (unsigned char def, unsigned char z);  /* (Part 15) Body bd's blow at def (I_ATTACK's) */
void __fastcall__ den_rob (const struct msg* m);    /* (Part 15) The character robbed by body bd, what it takes
                                                **   put down at m's w1, w2, or in its faction's hoard there, or hidden
                                                **   there (I_ROB's) */
void bled (void);                               /* (Part 15) The bleeding's bloodloss, a minute at a time */
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
void den_drop (void);                           /* (Part 60) Its weapon on the floor, told */
unsigned char den_legs (void);                  /* (Part 51) Its legs at a move's start: 1 no move */
void __fastcall__ den_other (const struct msg* m);  /* (Part 60) A denizen's things made (I_KIT's) */
void __fastcall__ den_enter (const struct msg* m);  /* (Part 60) Come near (I_ENTER's): its body's from it, its
                                                **   things in the store, the referee told */
unsigned char den_draw (void);                  /* (Part 60) Its weapon drawn from its belt, its hands empty of one: 1
                                                **   (its round's), or 0 */
void __fastcall__ den_strip (unsigned body);    /* (Part 60) Dead: its things on its body (body, its remains), what
                                                **   it held beside it */
void __fastcall__ limb_news (unsigned w, unsigned char b);  /* (Part 49) The limbs' news, the denizens' ring's (E_BEAT's
                                                **   w; b whose: 0 the character, else the actor's entry plus 1) */
unsigned char __fastcall__ engaged (unsigned char eng, unsigned char def);  /* (dq0w.c's part 25) The engage
                                                **   initiative test: eng engaging def (0 the character, else an
                                                **   actor's entry plus 1) */
extern unsigned char noinit;                    /* (dq0 -I: no engage initiative test: the tests') */

#endif
