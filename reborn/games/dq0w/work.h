/*
** work.h - (dq0w) the world's work on the portals and the traps (DUNGEON.md: portals; traps, pits and other nasty
** surprises): what the main code (dq0w.c: door_work, its work ()) and the work's swapped parts (wdoor.c, wblow.c,
** wwin.c, wtrap.c: ../dq0/swap.inc) share, in the main RAM; and the change records written (store.c).
*/

#ifndef _WORK_H
#define _WORK_H

/* ---- store.c: the change records (world.h's), the world's alone to write */
void __fastcall__ change (unsigned char sn, unsigned x, unsigned y, unsigned char tile, signed char elev,
    unsigned char link);                        /* Kept, and made in its chunk (the window moved) */
void stocks (void);                             /* The chunks the maker stocked marked so (STOCKED) */
void __fastcall__ kept_wait (unsigned char n);  /* (Part 28's) Stratum n's populations waited for (the denizens') */
unsigned char __fastcall__ sconce_lights (unsigned char reach);  /* (Part 28's: wkept.c) The rooms' lights by their
                                                **   sconces' (the chunks round the character, reach 0xFF; the chunk just
                                                **   stocked, SL_STOCKED, its first square rec's; else those within
                                                **   reach): 1 if one changed */
#define SL_STOCKED      0xFE
void answer (void);                             /* The hub's wreq answered (part 4's) */
void world_keep (void);                         /* (Part 16's: wgame.c) The world's part of the save, world.dat */
void pc_asked (void);                           /* (Part 16's) The hub's wreq, WQ_START's or WQ_SET's, answered */
void __fastcall__ store_begin (unsigned char resume);   /* The game's generation, its stamps (part 18's: wfile.c) */
void store_put (void);                          /* (The record in rec kept: part 38's, as change is) */

/* ---- The work: what's to be done (dq0w.c's work () sets it), and what came of it (the parts') */
struct work {
    unsigned char   sn;                         /* The character's stratum */
    unsigned char   what;                       /* PA_*, or W_STEP; a battering's W_BLOW, W_GONE as it goes */
    unsigned char   pd;                         /* The way from where the character stands to the portal */
    unsigned char   link;                       /* PA_BAR's: the widest bar to hand, its width (0 none); a search's:
                                                **   1, as it goes (quietly) */
    unsigned        px, py;                     /* The portal's square (or NO_SQ: about the character) */
    unsigned        fx, fy;                     /* Where the character stands (or came from); the reply's where it is
                                                **   after (a pit's: where it climbs out to) */
    unsigned        tx, ty;                     /* (The trap's square) */
    unsigned long   tt;                         /* The time it took (and it was out for), tenths */
    unsigned char   climb;                      /* The reply's: a pit's climb out (feet, or 0); */
    unsigned char   res;                        /*   1 if nothing was there to work (2: somewhere new, a window
                                                **   climbed through); a search's as it goes, 1 if it found something */
    unsigned char   armed;                      /* (The portal's trap: 1 known of, | TS_HIDDEN not; 0 none) */
    unsigned char   through;                    /* (Climbed through a window: on its far side) */
    unsigned char   hb;                         /* (The striker's own hurt by its blows at a thing: the worst, told */
    unsigned        hw;                         /*   once they're done; hw's E_OUT, E_STOPPED) */
    unsigned char   hm, hs;                     /* (The part battered's harm: its mask, its shift in the portal's; */
    unsigned char   was;                        /*   its harm as the battering began) */
    unsigned char   open;                       /* (An opening's words, PO_*, held till the look after it: told with what
                                                **   it shows, dq0w.c's revealed; 0 none) */
    struct portal   pq;                         /* (The portal worked, as it was) */
    struct trap     tq;                         /* (The trap) */
    struct msg      m;                          /* (An event told) */
};
extern struct work wk;
#define W_BLOW          0xFD                    /* (A battering under way: its next blow as the character's free; */
#define W_GONE          0xFE                    /*   the part given way at the last, told as it's free) */
#define BLOWS           12                      /* (A battering's blows at most: its long task's length) */
#define BLOW_T          30                      /* (A blow's time, tenths: its strenuous work, the referee's to tire
                                                **   the character by) */
extern unsigned char pc_cut;                    /* (The character's long task stopped, hard or by the player: dq0w.c's;
                                                **   a battering's blows given up, wblow.c's) */

void doors (void);                              /* (Part 5's) A door worked, or a window's shutters */
void blows (void);                              /* (Part 6's) A portal broken by blows: begun, or its next blow */
void portals (void);                            /* (Part 7's) A portcullis or a window worked */
void traps (void);                              /* (Part 8's) A trap stepped onto, disarmed; a search */
void revealed (void);                           /* (Part 41's: dq0w.c) An opening told, with what it shows beyond */

/* dq0w.c's: the referee asked (kind, a, b; its w1 ww1, its steps wsteps, 0 after): its grade, wres its result */
signed char __fastcall__ ask_w (unsigned char kind, unsigned char a, unsigned char b);
extern unsigned char wres;
extern signed char wsteps;

#endif
