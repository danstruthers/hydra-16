/*
** themegen.h - (themegen, and its bank programs: bank/histgen, goalgen, linegen) the history as it's made, in
** themegen's main RAM, where its bank programs work on it; and themegen's own that they call: the words' slots
** filled, the data's words picked, the lines emitted, a hand-written theme's lines, names (namegen's).
*/

#include "data.h"

/* ---- The history (HISTORY.md), as it's made */
#define WK_DEEP         6                       /* (The works: theme.h's W_*, and the natural caves') */
extern const char* const works[];               /* "keep" ... "deep" */
extern const char* const endings[];             /* "abandoned" ... "held" (EN_*) */
extern const char* const dids[];                /* "built" "ruined" "reused" (TL_*) */
extern char land[12], rname[28], noun[12], rock[12], water[12], climate[12];
extern unsigned char ne, ns, nf;
struct era {
    unsigned char   what;                       /* Its people (the data's people line: 0 ...) */
    unsigned char   tongue, work, end, ago;
    unsigned char   took;                       /* The older work it made its own (WK_*; 0xFF: none) */
    unsigned char   own;                        /* (Its words a hand-written theme's) */
    char            kind[12];                   /* What they are ("dwarves") */
    char            people[12], pthe[16];       /* Their name ("Khazrim"), and said ("the Khazrim") */
    char            great[10], place[10];       /* Its great one, its place */
    char            words[200];                 /* What it did (a clue's words: no "centuries ago") */
};
extern struct era er[TERAS];
struct str {
    char            name[20];
    char            styles[3];
    unsigned char   work, sub;                  /* (Its work, WK_*, and its place among the work's strata: 0 the top) */
    unsigned char   purpose;                    /* Its first layer's */
    unsigned char   nl;
    struct tlayer   l[TLAYERS];
};
extern struct str st[TSTRATA];
struct fac {
    char            name[12];
    unsigned char   d0, d1, lair, leader, n;
    char            kinds[32], att[2], others[48];
};
extern struct fac fa[TFACTIONS];
extern unsigned char gk;                        /* The goal kind (the data's goal line: 0 ...), 0xFF none */
extern unsigned char gs, ge, gg, gu, geu;       /* Its stratum, its era, its guardians (fa's: 0xFF none), a stratum
                                                **   above it and its era */
extern char gA[40], gB[40], gM[12], ga[16], gb[16], title[32], gmission[8];
extern int ga_line, gb_line, goal_line;         /* (Its things' lines, its goal line, in the data) */
extern unsigned long seed;                      /* (The seed it's made from) */
unsigned char __fastcall__ era_now (unsigned char k);   /* Stratum k's era now (its last layer built or reused's) */
unsigned char __fastcall__ lives (unsigned char b, unsigned char k);    /* The bestiary's kind b can live in k */
int __fastcall__ work_of (const char* s);       /* WK_*, or -1 */
const char* __fastcall__ purpose_words (unsigned char p);

/* ---- Words: the slots (%A ...: history.txt's, goals.txt's) filled; the data's words picked */
extern const char* sl[58];
#define SL(c)           sl[(c) - 'A']
void __fastcall__ fill (char* out, const char* tpl, unsigned max);
char* __fastcall__ pick (const char* s, char* out);     /* One of s's words (spaces between) at random, into out */
unsigned char __fastcall__ among (const char* s, const char* list);    /* s one of list's words */
extern char W[LINE_TEXT];                       /* (Words made, for a line: the bank programs' room) */

/* ---- The lines: a line out (begin_line, val, end_line: emitted, written to theme.txt and said) */
void __fastcall__ begin_line (const char* key);
void __fastcall__ val (const char* s);
void __fastcall__ valn (unsigned n);
void end_line (void);
void __fastcall__ emit (struct line* l);
extern unsigned wused;                          /* (The theme's words so far, of the room after its record's head) */

/* A hand-written theme's groups (themegen -t): what it gives, its lines given to fn */
enum { GR_HEAD, GR_REGION, GR_HIST, GR_STRATA, GR_WORDS, GR_DENS, GR_GOAL, GROUPS };
extern unsigned char given[GROUPS];
void __fastcall__ given_each (unsigned char g, void __fastcall__ (*fn) (struct line*));

/* The bank programs reach the arrays (and D) through pointers: an import's address and an offset taken whole, its
** high byte with a carry from the low (er[0].place's), is what o65 (ld65's) can't relocate, and the build refuses */
extern struct era* const erp;
extern struct str* const stp;
extern struct fac* const fap;
extern struct beast* const beastp;
extern struct line* const Dp;
#ifdef BANKPROG
#define er              erp
#define st              stp
#define fa              fap
#define beasts          beastp
#define D               (*Dp)
#endif

/* namegen (the bank program): a name, of a tongue and form, from a seed */
enum { NG_PERSON, NG_PLACE, NG_PEOPLE };
const char* __fastcall__ name_of (unsigned char tongue, unsigned char form, unsigned long seed);
