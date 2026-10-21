/*
** hmchar.h - (hmchar, and its bank program bank/hmocc: the culture rolled, the deity and the occupation chosen,
** occupations.txt read) hmchar's own that hmocc uses: the character, the options, the occupation's name, and the
** output and input (hmchar's exports, hmchar.exp; lent.s names what hmchar wouldn't link otherwise).
*/

#include <harn.h>

extern harn_char h;                             /* The character */
extern unsigned char quiet;                     /* -q: no questions */
extern const char* cname;                       /* -c's occupation (NULL: none) */
extern const char* uname;                       /* -u's culture (NULL: none) */
extern char occname[20];                        /* The occupation (its name; the player's own, as it's called) */
extern char libdir[48];                         /* hmchar's lib/hmchar (occupations.txt, hmocc.bp, hmchar.exp) */
extern char ab[40];                             /* An answer typed */
extern unsigned char col;                       /* The column the output's at */

void flush (void);
void __fastcall__ ch (char c);
void __fastcall__ s (const char* t);
void __fastcall__ n (int v);
void __fastcall__ named (const char* label, int v, const char* after);
void __fastcall__ err (const char* t);
unsigned char __fastcall__ ask (const char* q, char* b, unsigned char max);     /* A line typed: its length */

/* A skill named (case ignored, a beginning of its name enough if only one has it), among count (among's, or skills
** base on): it, 0xFE (more than one begins so) or 0xFF */
unsigned char __fastcall__ skill_named (const char* w, const unsigned char* among, unsigned char base,
                                        unsigned char count);

/* Points spent, op of them (Character 16's option points; or own, the player's own occupation's) */
void __fastcall__ spend (unsigned char op, unsigned char own);

/* hmocc's entries.  culture: a human's culture rolled, or -u's (its class rolled for it): 0; or 1, -u's not a human
** culture.  occupation: the deity chosen (not -q's), then the occupation and its skills opened (occname): 0; or 1,
** -c's not one offered (or more than one begins so); 2, no occupations.txt */
unsigned char culture (void);
unsigned char occupation (void);
