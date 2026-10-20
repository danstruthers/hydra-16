/*
** data.h - (themegen, themec) what the two have alike: a theme's lines (read from its file), themegen's data file
** (themegen.dat, make.js's: the history's words, the goal kinds, the bestiary) in banks of their own, and the bestiary
** from it.  data.c is theirs both (themec's includes it).
*/

#include "../dq0/hub.h"

/* ---- A line: its key and its values (pointing into its text) */
#define MAXV            16
struct line {
    char*           v[MAXV];                    /* (First: a bank program reaches them from D's own address) */
    char            key[10];
    unsigned char   n;
};
#define LINE_TEXT       1100                    /* (A line's text, at most: a story's) */

/* A theme's file read a line at a time (a key and its values; a line begun with a space goes on with the one
** before): lines_open's 0 (or -1), then read_line's 1 a line (its text in t), 0 at its end */
int __fastcall__ lines_open (const char* path);
unsigned char __fastcall__ read_line (struct line* l, char* t);
void lines_close (void);

/* ---- The data file (themegen.dat: make.js's), its keys (make.js's KEYS); its text in banks of the program's own,
** each read leaving the window as it was (a bank program's in it, as it calls) */
enum { TK_PEOPLE = 1, TK_LAND, TK_WORK, TK_NAME, TK_BUILT, TK_REUSED, TK_NOUN, TK_ENDED, TK_CLUE, TK_ROOMS, TK_USES,
    TK_LIGHT, TK_FOLK, TK_NOW, TK_OPEN, TK_AGO, TK_GOAL = 20, TK_THING, TK_EXIT, TK_TITLE, TK_CALLED, TK_THEME = 30,
    TK_BEAST = 40, TK_FACTION };
#define TK_THEMES       7                       /* (A goal's theme lines: kind piece rule story lock opened ending) */
int __fastcall__ tg_load (const char* path);    /* Read in: 0, or -1 */
void __fastcall__ tg_get (int i, struct line* l, char* text);  /* Line i's values */
int __fastcall__ tg_find (unsigned char key, const char* first, unsigned char k);  /* The kth line (0 ...) of a key
                                                **   (whose first value is first, if it's not NULL), or -1 */
unsigned char __fastcall__ tg_count (unsigned char key, const char* first);
unsigned char __fastcall__ data (unsigned char key, const char* first, unsigned char k);   /* Into D: 1, or 0 */
extern struct line D;
extern char Dt[LINE_TEXT];
extern unsigned tg_lines;
extern unsigned char tg_key[];

/* The bestiary (the data's beast and faction lines: kinds.txt's, factions.txt's) */
#define NBEASTS         16
struct beast {
    char            key[8];
    unsigned char   styles;                     /* A bit each: ST_* (b n m x) */
    unsigned char   d0, d1, n0, n1, lairs;
    unsigned char   faction;                    /* (Its faction, the data's: 0 ...) */
};
extern struct beast beasts[NBEASTS];
extern unsigned char nbeasts;
void bestiary (void);
unsigned char __fastcall__ style_bits (const char* s);
int __fastcall__ beast_of (const char* key);    /* Its number (0 ...), or -1 */

void __fastcall__ say (const char* s);          /* (Fd 2) */
const char* __fastcall__ num (unsigned n);      /* A number in words (a static buffer of its own) */
void __fastcall__ setbank (unsigned char b);
signed char __fastcall__ find_word (const char* const* list, const char* s);  /* s's place in list (NULL-ended), or -1 */
int __fastcall__ purpose_of (const char* s);    /* A purpose's words (halls, keep fortified ...): P_*, P_FORT; or -1 */
unsigned char getbank (void);
