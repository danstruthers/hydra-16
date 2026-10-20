/*
** comp.h - (themec) the compiler (comp.c): a theme's lines into its record (R: theme.h's struct theme, then its
** words), a line at a time, then made whole; and the theme checker
*/

#include "../themegen/data.h"

extern unsigned char R[THEME_SIZE];
void comp_begin (void);
const char* __fastcall__ comp_line (struct line* l);    /* NULL, or what's wrong */
const char* comp_end (void);                    /* NULL, or what's wrong */
unsigned comp_used (void);                      /* Its words' end */
unsigned char comp_check (void);                /* The theme checker: its faults said (fd 2), how many */
