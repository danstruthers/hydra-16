/*
** dq0harn.h - (dq0harn) the referee's parts: the character's sheet and the words into a reply (dq0harn.c's), the
** sheet in words and harm taken on it (sheet.c's, overlay 1: called through bank.s's stubs), fights and the
** creatures' sheets (fight.c's, overlay 2), the banks (bank.s)
*/

#ifndef _DQ0HARN_H
#define _DQ0HARN_H

/* dq0harn.c */
extern harn_char pc;                            /* The character */
extern char name[16], calling[20];
extern char* tp;                                /* The words into a reply: where the next goes, */
extern char* tend;                              /*   and the end */
void __fastcall__ put (const char* s);          /* (From the overlay too: s may be in the window) */
void __fastcall__ putn (int v);
unsigned __fastcall__ cond_of (const harn_char* c);        /* Its conditions (rs.h's C_*) */
#define conditions()    cond_of (&pc)           /* The character's */
unsigned char __fastcall__ health_of (const harn_char* c); /* Its health (rs.h's H_*) */
unsigned char __fastcall__ has (const char* key);   /* A tool to hand, by its key ("pick") */
#define NTOOLS  20
extern char tools[NTOOLS][8];                   /* What's to hand (RS_EQUIP's): each one's key, */
extern unsigned char tflag[NTOOLS];             /*   T_HELD, T_WORN, */
extern unsigned char tqual[NTOOLS];             /*   its quality (0-9) */
#define T_HELD  0x01
#define T_WORN  0x02
extern unsigned long tnow;                      /* (The clock, the hub's, as a request's answered) */
extern unsigned char stance;                    /* (The character's stance, the hub's pc_stance, the same) */
extern unsigned long gseed;                     /* (The game's seed) */
extern char bkey[8];                            /* (RS_NEW_ACTOR's key, from the request's text) */

/* A creature's sheet (a denizen's record's: kept in a bank of the referee's, 32 bytes each, by the record's number):
** what it is (the bestiary's entry), its own attributes, its state, its injuries; made the first time it's an actor */
struct beast {
    unsigned char   kind;                       /* Its bestiary entry plus 1 (0: none yet) */
    unsigned char   att[5];                     /* Strength, Stamina, Dexterity, Agility, Will */
    unsigned char   wt;                         /* Its weight, in 4 lb */
    unsigned char   state;                      /* HARN_CONSCIOUS ... (| B_PRONE: fallen) */
    unsigned char   bl;                         /* Bloodloss */
    unsigned char   ninj;
    harn_injury     inj[3];                     /* (More: the last takes their levels) */
    unsigned long   out;                        /* Senseless till then (the clock's tenths) */
};
#define B_PRONE         0x80
#define NSHEETS         256
typedef char beast_fits[sizeof (struct beast) * NSHEETS <= 0x2000 ? 1 : -1];
extern struct beast bs;                         /* (A sheet, copied out of the bank) */
extern unsigned char aden[NACTORS];             /* (Each actor's record: its sheet's number) */
void __fastcall__ sheet_get (unsigned char n);  /* Sheet n into bs */
void __fastcall__ sheet_put (unsigned char n);  /*   and back */

/* sheet.c (overlay 1) */
#define OVLS    2
extern char hurt[200];                          /* The last harm's words */
extern char blow[8];                            /* (A blow's: what it's struck with, its key) */
void sheet (void);                              /* The sheet, into the reply */
void brief (void);                              /*   its short lines */
void __fastcall__ harm (struct req* r);         /* A harm (r a copy, in the main RAM, but its text) */

/* fight.c (overlay 2) */
void __fastcall__ fight (struct req* r);        /* RS_NEW_ACTOR, RS_ATTACK, RS_MORALE, RS_TIME (r a copy, as harm's) */
unsigned char __fastcall__ pc_armour (unsigned char loc, unsigned char aspect);    /* The character's armour at loc
                                                **   against an aspect (HARN_*), all its worn pieces' */

/* bank.s */
void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[OVLS];

#endif
