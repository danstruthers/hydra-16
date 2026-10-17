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

/* ---- The parts' (bank.s's stubs) */
void den_intents (void);                        /* (Part 14) The denizens' intents carried out (the hub's di), they
                                                **   rung after */
#define DEN_INTENTS()   (HUB->di_tail != HUB->di_head)  /* (Some to carry out: the hub's header in the window) */
void pc_blow (void);                            /* (Part 15) The character's blow at the actor it's posted (the hub's
                                                **   pc_foe) */
void __fastcall__ den_blow (unsigned char def, unsigned char z);  /* (Part 15) Body bd's blow at def (I_ATTACK's) */
void bled (void);                               /* (Part 15) The bleeding's bloodloss, a minute at a time */

#endif
