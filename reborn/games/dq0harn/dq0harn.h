/*
** dq0harn.h - (dq0harn) the referee's parts: the character's sheet and the words into a reply (dq0harn.c's), the
** sheet in words and harm taken on it (sheet.c's, overlay 1: called through bank.s's stubs), the banks (bank.s)
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
unsigned conditions (void);                     /* The character's (rs.h's C_*) */

/* sheet.c (overlay 1) */
#define OVLS    1
extern char hurt[200];                          /* The last harm's words */
extern char blow[8];                            /* (A blow's: what it's struck with, its key) */
void sheet (void);                              /* The sheet, into the reply */
void brief (void);                              /*   its short lines */
void __fastcall__ harm (struct req* r);         /* A harm (r a copy, in the main RAM, but its text) */

/* bank.s */
void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[OVLS];

#endif
