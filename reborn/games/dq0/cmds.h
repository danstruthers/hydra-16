/*
** cmds.h - Dingus Quest Zero's command handlers, by meaning (COMMANDS-CRITIQUE.md's §5.2, step L, stage 3,
** 2026-11-08): a file a group, each group an entry a bank it's in, called by main.c's s_command with its handler's
** number (dq0.h's H_*: the dispatch table's, actions.c's row; and its takes times 256).  Where a group is is its
** pragma's: a part's entry is s_NAME, called as NAME (the build's routing: straight from inside its part, through its
** stub from outside it, games/dq0/pack.js); an overlay's o_NAME, through its stub (bank.s's OVFN).
**
**   hthings.c  the things' verbs: take, drop, handle (hold, stow, put, wear, light ...), hide, eat and drink (part
**              2's things_do); a thing filled, tied, looked into, one that closes worked, a hide begun (their timed
**              works: overlay 1's things_work, overlay 16's till stage 3)
**   hplaces.c  the features' and the doors': a door's work, climb, examine, search, a look one way (part 2's)
**   hmove.c    moving: go (a way, a thing, a place in the room), face, jump (part 2's)
**   hdens.c    the denizens': a blow, the missiles, disengage, the sway, a gift and the market (part 2's)
**   hself.c    the body's and the meta commands: the limbs, a feel about, a look, wait, stop, tend, dig, what's
**              carried, the sheet and the map, the time, pause and save, help, quit (part 2's)
**
** All five in part 2 so far, the commands' own (s_command's: no copy of a part between the line and its handler): a
** group moved is its pragmas alone.
*/

#ifndef _CMDS_H
#define _CMDS_H

/* The groups' entries (h: the row's handler, its takes times 256) */
void __fastcall__ things_do (unsigned h);
unsigned char __fastcall__ things_work (unsigned char h);   /* (1 carried out; 0, H_SHUT's: none that closes of that
                                                **   name, a door's work then) */
void __fastcall__ places_do (unsigned h);
void __fastcall__ moving_do (unsigned h);
void __fastcall__ dens_do (unsigned h);
void __fastcall__ self_do (unsigned h);

/* What they share */
void __fastcall__ after (unsigned char was, unsigned t, unsigned char op);   /* (hthings.c's) */
void __fastcall__ handle (unsigned char op, const char* x, const char* y, unsigned char on);
void put_in (void);                             /* (hthings.c's: a thing put in another, or let fall in a well) */
void __fastcall__ portal (unsigned char pa);    /* (hplaces.c's: a portal worked) */
void __fastcall__ climb (unsigned char h);      /* (hplaces.c's: up or down, H_CLIMB's and a jump's) */
unsigned char __fastcall__ grip_go (unsigned char g);  /* (hdens.c's: a grapple's move sent, HOUSE.md's grappling) */
void __fastcall__ minds (unsigned char h);      /* (minds.c's, part 27: say, knock, wake, taunt, orate, pretend; h H_*) */

/* main.c's */
void __fastcall__ work (unsigned char what, unsigned char obj);
void __fastcall__ intend (unsigned char kind, unsigned char a);
unsigned char __fastcall__ deal (unsigned char how);
unsigned char __fastcall__ things (unsigned char op, const char* name);
unsigned char __fastcall__ othings (unsigned char op, const char* name, const char* into, unsigned char to);
void to_hand (void);
void gear_look (void);
void limbs_cmd (void);
unsigned char hold_first (void);
unsigned char errand (void);
unsigned char __fastcall__ hide (const char* w);
unsigned char missiles (void);
extern unsigned char barw;                      /* (A bar to hand, its width: the portal's, a door barred) */
extern volatile unsigned char quit;
extern char nil[1];                             /* (An empty name in the main RAM: an overlay's to give othings) */
#define SW_QUIET    0x80                        /* (deal's: none found, said nothing) */

#endif
