/*
** dq0harn.h - (dq0harn) the referee's parts: the character's sheet and the words into a reply (dq0harn.c's), the
** sheet in words and harm taken on it (sheet.c's, overlay 1: called through bank.s's stubs), fights and the
** creatures' sheets (fight.c's, overlay 2), the banks (bank.s)
*/

#ifndef _DQ0HARN_H
#define _DQ0HARN_H

/* dq0harn.c (and limbs.c's, overlay 5's: health_of, limbs_of, cond_of, rise_of) */
extern harn_char pc;                            /* The character */
extern char name[16], calling[20];
extern char* tp;                                /* The words into a reply: where the next goes, */
extern char* tend;                              /*   and the end */
void __fastcall__ put (const char* s);          /* (From the overlay too: s may be in the window) */
void __fastcall__ putn (int v);
unsigned __fastcall__ cond_of (const harn_char* c);        /* Its conditions (rs.h's C_*) */
unsigned char __fastcall__ limbs_of (const harn_char* c);  /* Its limbs (rs.h's LB_*), each one's hurt in lhurt */
signed char __fastcall__ rise_of (const harn_char* h);  /* (Overlay 5's, limbs.c's) Getting up, prone: its grade (the
                                                **   book's Rise, the house's test) */
signed char __fastcall__ bearing_of (signed char steps);   /* (Overlay 5's, bearing.c's) The character's bearings kept
                                                **   (P_BEARING): its grade, bway a failure's d8 less one */
extern unsigned char bway;                      /* (A failure's d8 less one: the world's) */
extern unsigned char lhurt[4];                  /* (The injuries' levels on its left arm, its right, its left leg, its
                                                **   right) */
extern unsigned char cplan;                     /* (A creature's body, cond_of's: BF_*, the bestiary's) */
extern unsigned char hheld;                     /* (The character's hands that hold something, RS_EQUIP's: DH_*) */
#define BF_MINDLESS     0x01                    /* (The bestiary's flags: bones that move, mindless; */
#define BF_BIPED        0x02                    /*   on two legs, its hands free (folk, the dead); */
#define BF_QUAD         0x04                    /*   on four (its forelegs the strike table's arms); neither, no legs
                                                **   to lose: a snake's, a bat's wings, a spider's many); */
#define BF_FOLK         0x08                    /*   folk, its armour only what it wears, its things told: the outfits'
                                                **   gear, DECISIONS.md's answer 7); */
#define BF_BERSERK      0x10                    /*   one that would sooner go berserk than flee (the Gargu-khanu: its
                                                **   breakdown never a flight, the house's, 2026-10-29) */
#define conditions()    cond_of (&pc)           /* The character's */
unsigned char __fastcall__ health_of (const harn_char* c); /* Its health (rs.h's H_*) */
unsigned char __fastcall__ has (const char* key);   /* A tool to hand, by its key ("pick") */
extern struct kind wk;                          /* A thing's kind, the catalogue's (the hub's pc_strike's, SK_THING;
                                                **   RS_WIELD's, each held in turn), */
void __fastcall__ kind_in (unsigned char k);    /*   copied in (from an overlay too: the window put back) */
signed char __fastcall__ grade (unsigned char level);   /* A level of success (harn.h's) as rs.h's grade */
struct pstate {                                 /* (The character's state's words need, the hub's: copied) */
    unsigned char   gait, watch;                /* Its pace, its watchfulness (rs.h's GA_*, hub.h's WA_*) */
    unsigned char   light, dark;                /* Its light's reach (squares), its square unlit (by feel; 2, its pace
                                                **   said outright there, held to nothing) */
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
#define T_LEFT  0x04                            /* (Held in the left hand: RS_EQUIP's marks, slice 4.1b's) */
#define T_RIGHT 0x08                            /*   the right; both, a thing in both hands */
extern unsigned long tnow;                      /* (The clock, the hub's, as a request's answered) */
extern unsigned char stance;                    /* (The character's stance, the hub's pc_stance, the same) */
extern unsigned long gseed;                     /* (The game's seed) */
extern unsigned wealth;                         /* (The family's wealth, its file's: 0 none; RS_OUTFIT's) */
extern char bkey[8];                            /* (RS_NEW_ACTOR's key, from the request's text) */

/* A creature's sheet (a denizen's record's: kept in a bank of the referee's, 32 bytes each, by the record's number):
** what it is (the bestiary's entry), its own attributes, its state, its injuries; made the first time it's an actor */
struct beast {
    unsigned char   kind;                       /* Its bestiary entry plus 1 (0: none yet) */
    unsigned char   att[5];                     /* Strength, Stamina, Dexterity, Agility, Will */
    unsigned char   wt;                         /* Its weight, in 4 lb */
    unsigned char   state;                      /* HARN_CONSCIOUS ... (| BS_DISARMED) */
    unsigned char   bl;                         /* Bloodloss */
    unsigned char   ninj;
    harn_injury     inj[3];                     /* (More: the last takes their levels) */
    unsigned long   out;                        /* Senseless till then (the clock's tenths) */
};
#define BS_DISARMED     0x80                    /* (Its state's: its weapon let fall, its fists till it takes it up:
                                                **   fight.c's, slice 4.1b's) */
#define BS_SHOCKED      0x40                    /* (Its state's, senseless: it comes round in shock, the book's last
                                                **   shock roll's, sheet.c's wake; in shock, its out its shock's
                                                **   healing rate: slice 4.3's) */
#define NSHEETS         256
typedef char beast_fits[sizeof (struct beast) * NSHEETS <= 0x2000 ? 1 : -1];
extern struct beast bs;                         /* (A sheet, copied out of the bank) */
extern unsigned char aden[NACTORS];             /* (Each actor's record: its sheet's number) */
void __fastcall__ sheet_get (unsigned char n);  /* Sheet n into bs */
void __fastcall__ sheet_put (unsigned char n);  /*   and back */

/* sheet.c (overlay 1) */
#define OVLS    6                               /* (The 4th and 5th: the swapped parts' images; the 6th heal.c's) */
extern char hurt[200];                          /* The last harm's words */
void sheet (void);                              /* The sheet, into the reply */
void brief (void);                              /*   its short lines */
void state (void);                              /*   its state: pace, watchfulness, condition, light (ps's) */
signed char __fastcall__ noticing (const struct req* r);  /* A notice's steps by its state (r a copy; ps's) */
void __fastcall__ spent (unsigned t);           /* Time spent (t tenths) at spent_at's pace (or GA_WORK, GA_REST):
                                                **   fatigue */
void __fastcall__ harm (struct req* r);         /* A harm (r a copy, in the main RAM, but its text) */
unsigned char __fastcall__ wound (harn_char* c, unsigned char loc, unsigned char aspect, int impact,
    unsigned char by);                          /* A wound taken (harn_wound's, HARN_W_*), the game's ways round it:
                                                **   the record's room, the shock roll of one in shock; by what dealt
                                                **   it (BY_*: the character's, kept in pc_by) */
void __fastcall__ wound_words (char* w, unsigned char sev, unsigned char loc, unsigned char aspect, unsigned char il,
    unsigned char by);                          /* A wound's words into w, the main RAM's ("a serious bite to your
                                                **   right elbow (S2)"; by | BY_ITS, "to its"; sev HARN_KILL, a death's) */
unsigned __fastcall__ knocked_out (harn_char* c, unsigned long* o, unsigned char* f);  /* Knocked senseless: the
                                                **   tenths till it comes round, *o its time out (AF_AGAIN in *f) */

/* What dealt an injury, for its words (the character's: pc_by, each injury's, kept with its sheet): BY_ASPECT its
** aspect's (a weapon's, a fall's, a trap's); else a creature's own part (the referee's bestiary's weapons, by their
** words: claws and talons alike) */
enum { BY_ASPECT, BY_BITE, BY_CLAWS, BY_STING, BY_HORNS, BY_VENOM };   /* (BY_VENOM: a venom's ache where it's
                                                **   spread, heal.c's) */
#define BY_ITS          0x80                    /* (wound_words's: to a creature, "to its") */
extern unsigned char pc_by[HARN_INJMAX];
#define IF_POOL         0x80                    /* (An injury's flags, the referee's own, the library's never: the
                                                **   lesser hurts past the record's room, folded together: wound's) */

/* fight.c (overlay 2) */
void __fastcall__ fight (struct req* r);        /* RS_NEW_ACTOR, RS_ATTACK, RS_MORALE, RS_TIME, RS_CONTEST (a
                                                **   creature's side: r a copy, as harm's) */
extern unsigned long pc_out;                    /* (The character senseless till then, the clock's tenths) */
extern harn_char cr;                            /* (A creature's sheet unpacked: the fight's, heal.c's too) */
unsigned char __fastcall__ beast_flags (unsigned char kind);   /* (A sheet's kind's BF_*, the bestiary's) */

/* heal.c (overlay 6): wounds over time (slice 4.3) */
#define NEWS_N          120
extern char news[NEWS_N];                       /* (What time did to the character, in words: RS_DESCRIBE b 4's) */
extern char tpart[16];                          /* (RS_TREAT's part said: its text's, copied) */
struct venom {                                  /* (The character's venom: its healing rate, 0 none; where it's got to;
                                                **   its kind, heal.c's; its interval's minutes gone, at rest's) */
    unsigned char   hr, loc, kind;
    unsigned        left;
};
extern struct venom pv;
extern unsigned rest_m;                         /* (Its rest toward its healing rolls, minutes: RS_TIME's at rest) */
void __fastcall__ body_req (struct req* r);     /* RS_HARM, RS_TIME, RS_TREAT (r a copy) */
unsigned char __fastcall__ envenom (const char* key, unsigned char loc, int impact);   /* (fight.c's: a venomous
                                                **   bite; 1 told) */
void body_state (void);                         /* (The status's: sheet.c's o_state's) */
void __fastcall__ venom_time (unsigned m);      /* (Overlay 5's: the venom's course, m minutes at rest's) */
void __fastcall__ body_line (char* l, unsigned char rd);    /* (Overlay 5's: the save's "body" line made, or read) */
extern unsigned out_t;                          /* (Knocked senseless by a venom's ache: RS_TIME's w2) */

/* The bestiary (beasts.c's, overlay 5's read-only data since 2026-10-29: the peoples' room in overlay 2): by the kinds'
** keys, each one's attributes (Strength, Stamina, Dexterity, Agility, Will), its weight (4 lb), its size (1 tiny ... 4 a
** man ... 6 very large), its Initiative (its courage: morale) and Dodge, its hide's protection (blunt, edge, point,
** fire), and its weapons (a word, its ML, its impact, its aspect, the part it strikes with: 0xFF a weapon held), the
** first the one it uses most; an entry copied out into the main RAM as it's wanted (fight.c's beast) */
struct nweapon {
    char            word[7];
    unsigned char   ml, imp, aspect, part;
};
struct bkind {
    char            key[8];
    unsigned char   att[5];
    unsigned char   wt, size, ini, dge;
    unsigned char   arm[4];
    unsigned char   flags;
    struct nweapon  w[2];
};
#define NBEASTS         29                      /* (beasts.c's entries: its check) */
extern const struct bkind bestiary[NBEASTS];    /* (Overlay 5's: read with bread, never in the window here) */

/* A side of a fight, as a blow wants it (fight.c's; gear.c's for the character) */
#define SIDE_BLK        4
#define BARE_ARM        NTOOLS
struct side {
    harn_char*      c;
    unsigned char   who;                        /* 0 the character, else an actor's entry plus 1 */
    unsigned char   size;
    int             aml, dml, dodge;            /* Its attack's EML, its block's (with what it holds), its dodge's */
    signed char     imp[3];                     /* Its weapon's impacts */
    unsigned char   part;                       /* (Its weapon a part of it: the part it strikes with; else 0xFF) */
    unsigned char   bare;                       /* (Its block a bare arm's) */
    unsigned char   nblk;                       /* (The character's: what blocks, each thing it holds that can, else a
                                                **   bare arm: up to SIDE_BLK; each one's */
    unsigned char   bslot[SIDE_BLK];            /*   slot in tools, BARE_ARM a bare arm's, */
    int             bml[SIDE_BLK];              /*   block's EML) */
    unsigned char   aslot;                      /* (And the slot of what it strikes with: BARE_ARM a part of it) */
    unsigned char   dslot;                      /* (And of what it defended with last: BARE_ARM a bare arm, 0xFF a
                                                **   dodge: gear.c's pc_defence) */
    const struct bkind* b;                      /* (A creature's entry; 0: the character) */
    char            word[14];                   /* Its weapon, in a word (a thing named, its kind's name) */
};

/* gear.c (overlay 3): the character's weapons and armour */
void __fastcall__ hit_thing (struct req* q, struct side* s);   /* A blow at a thing (RS_ATTACK at RS_THING: fight.c's,
                                                **   side s the attacker's) */
void __fastcall__ pc_side (struct side* s);     /* Its side of a fight: what it strikes with (pc_way's), blocks with,
                                                **   dodges */
unsigned char __fastcall__ pc_defence (struct side* d, unsigned char f, unsigned char armed);  /* Its defence (HARN_*)
                                                **   against foe f this round (a round's defences, the user's) */
void __fastcall__ pc_wield (struct req* r);     /* RS_WIELD: what it would strike with, of what it holds (r a copy,
                                                **   its words put); its hands empty, the best it carries */
unsigned char __fastcall__ fumbled (const struct side* s, unsigned char att);  /* The hands that let go of what side s
                                                **   fumbled (its attack's, att 1; its block's), rs.h's DH_* */
unsigned char __fastcall__ wound_drops (const struct side* t, unsigned char loc);  /* The hand an arm's wound at loc
                                                **   lets go of what it holds (DH_*) */
unsigned char __fastcall__ pc_armour (unsigned char loc, unsigned char aspect);    /* Its armour at loc against an
                                                **   aspect (HARN_*), all its worn pieces', the last named in arm
                                                **   (""); loc SOLE: its soles' (a kick at a thing) */
#define SOLE    0x7F
extern char arm[16];
#define DPIECES 8                               /* (An actor's pieces worn, at most: the denizens' gear) */
#define DTEXT_N 80
extern unsigned char dwr[NACTORS];              /* (Each actor's weapon held, its row: 0 none, 0xFF untold, the bestiary's;
                                                **   the world's RS_EQUIP for an actor, the outfits' gear) */
extern unsigned char dsh[NACTORS];              /* (Its shield held, its row) */
extern char dshw[14];                           /* (The shield's word, den_side's: a block's words, 2026-10-29; */
extern signed char dshi[3];                     /*   its impacts, a fist's or a claw's that meets it) */
extern unsigned char dpc[NACTORS][DPIECES];     /* (Its pieces worn) */
extern char dtext[DTEXT_N];                     /* (Its marks, RS_EQUIP's text, copied in) */
void __fastcall__ den_equip (unsigned char k);  /* Actor k's things told (dtext's marks) */
void __fastcall__ den_side (struct side* s, unsigned char k);  /* Its blow by the weapon it holds (fight.c's: s's aml
                                                **   and word its kind's own weapon's) */
unsigned char __fastcall__ den_armour (unsigned char k, unsigned char loc, unsigned char aspect);  /* Its armour at
                                                **   loc against an aspect, its pieces' (the last named in arm) */
extern unsigned char pc_way;                    /* (The hub's pc_strike, as a request's answered: SK_*; SK_THING | a
                                                **   kind, its kind in wk) */
extern unsigned char pc_hand;                   /* (The hub's pc_shand: the hand that holds it, items.h's IH_*) */

/* bank.s */
void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[OVLS];

#endif
