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
struct pstate {                                 /* (The character's state's words need, the hub's: copied) */
    unsigned char   gait, watch;                /* Its pace, its watchfulness (rs.h's GA_*, hub.h's WA_*) */
    unsigned char   light, dark;                /* Its light's reach (squares), its square unlit (by feel) */
    unsigned char   full;                       /* 1 all of it; 0 the pace and watchfulness only (changed) */
    char            lamp[14];                   /* What gives its light */
    unsigned char   go;                         /* (The pace it's going at: the hub's pc_going, for noticing) */
};
extern struct pstate ps;
extern unsigned tired, rested;                  /* The character's exertion toward the next fatigue level, its rest
                                                **   toward the next gone (sheet.c's units) */
extern unsigned char spent_at;
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
#define OVLS    3
extern char hurt[200];                          /* The last harm's words */
void sheet (void);                              /* The sheet, into the reply */
void brief (void);                              /*   its short lines */
void state (void);                              /*   its state: pace, watchfulness, condition, light (ps's) */
signed char __fastcall__ noticing (const struct req* r);  /* A notice's steps by its state (r a copy; ps's) */
void __fastcall__ spent (unsigned t);           /* Time spent (t tenths) at spent_at's pace (or GA_REST): fatigue */
void __fastcall__ harm (struct req* r);         /* A harm (r a copy, in the main RAM, but its text) */

/* fight.c (overlay 2) */
void __fastcall__ fight (struct req* r);        /* RS_NEW_ACTOR, RS_ATTACK, RS_MORALE, RS_TIME (r a copy, as harm's) */

/* A side of a fight, as a blow wants it (fight.c's; gear.c's for the character) */
struct bkind;
struct side {
    harn_char*      c;
    unsigned char   who;                        /* 0 the character, else an actor's entry plus 1 */
    unsigned char   size;
    int             aml, dml, dodge;            /* Its attack's EML, its block's (with what it holds), its dodge's */
    signed char     imp[3];                     /* Its weapon's impacts */
    unsigned char   part;                       /* (Its weapon a part of it: the part it strikes with; else 0xFF) */
    unsigned char   bare;                       /* (Its block a bare arm's) */
    const struct bkind* b;                      /* (A creature's entry; 0: the character) */
    char            word[8];                    /* Its weapon, in a word */
};

/* gear.c (overlay 3): the character's weapons and armour */
void __fastcall__ pc_side (struct side* s);     /* Its side of a fight: what it strikes with (pc_way's), blocks with,
                                                **   dodges */
unsigned char __fastcall__ pc_armour (unsigned char loc, unsigned char aspect);    /* Its armour at loc against an
                                                **   aspect (HARN_*), all its worn pieces', the last named in arm
                                                **   (""); loc SOLE: its soles' (a kick at a thing) */
#define SOLE    0x7F
extern char arm[16];
extern unsigned char pc_way;                    /* (The hub's pc_strike, as a request's answered: SK_*) */
extern unsigned char at_thing;                  /* (Its blow at a thing: its best the tool that hits it hardest) */

/* bank.s */
void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[OVLS];

#endif
