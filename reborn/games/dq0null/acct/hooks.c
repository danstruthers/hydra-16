/*
** hooks.c - the null ruleset's part of the account of the time between two dungeons (../../dq0/rsmeta.h's acct_* hooks, linked
** into the engine's account as dq0nacct: games/dq0nacct), all of it in the main RAM.  Its rules are flat (dq0null.c's): nothing
** heals between dungeons (its wounds, its sheet's count, as they were: it heals at rest in the dungeon), no one dies of the days,
** there is no practice and no lesson in the work; its living is the middle station's (2d a day: the lodgings' common room), its
** wage the unguilded's (42d a month), its birthday the year's first day, its knacks in town its tests' flat chance (dq0null.c's
** BASE).  Its sheet (META_SHEET, dq0null.c's own lines: name, occupation, pace, wounds, its tongues and scripts) is read for its
** calling and its tongues, and what a master opened in town put on it as a line of its own, as the referee reads one.
*/

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../../dq0/acct/acct.h"

#pragma static-locals (on)

#define BASE            60                      /* (A test's chance with no steps, percent: dq0null.c's) */
#define TONGUES         8                       /* (Its tongues and scripts past its own at most: dq0null.c's load_pc) */

static char calling[20];
static unsigned char ntongue;                   /* (Its tongues and scripts on the sheet) */
static char learnt[BETWEEN_LEARNT][28];         /* (What a master opened, its line: written after the rest) */
static unsigned char nlearnt;

/* Its sheet read: its calling, its tongues and scripts counted; 0 none */
unsigned char acct_sheet_read (void)
{
    static char b[64], l[64];
    int fd, n, i;
    unsigned char k = 0;

    if ((fd = open (in_ (top, META_SHEET), O_RDONLY)) < 0) {
        return 0;
    }
    while ((n = read (fd, b, sizeof b)) > 0) {
        for (i = 0; i < n; ++i) {
            if (b[i] != '\n' && b[i] != '\r') {
                if (k < sizeof l - 1) {
                    l[k++] = b[i];
                }
                continue;
            }
            l[k] = 0;
            k = 0;
            if (!strncmp (l, "occupation ", 11)) {
                strncpy (calling, l + 11, sizeof calling - 1);
            } else if (!strncmp (l, "language ", 9) || !strncmp (l, "script ", 7)) {
                ++ntongue;
            }
        }
    }
    close (fd);
    return 1;
}

/* What a master opened, each a line after the rest (the sheet as it was, else) */
void acct_sheet_write (void)
{
    int fd;
    unsigned char k;

    if (!nlearnt || (fd = open (in_ (top, META_SHEET), O_WRONLY | O_APPEND)) < 0) {
        return;
    }
    for (k = 0; k < nlearnt; ++k) {
        write (fd, learnt[k], strlen (learnt[k]));
        write (fd, "\n", 1);
    }
    close (fd);
}

unsigned char acct_wounds (void)
{
    return 1;
}

unsigned char acct_mended (void)
{
    return 1;
}

unsigned char acct_day (void)
{
    return 1;
}

const char* acct_death (void)
{
    return "";
}

void acct_rested (void)
{
}

const char* acct_calling (void)
{
    return calling;
}

unsigned char acct_wage (void)
{
    return 42;
}

unsigned char acct_habit (void)
{
    return 2;
}

unsigned acct_birthday (void)
{
    return 1 << 8 | 1;
}

unsigned char acct_tool (unsigned char needs)
{
    (void) needs;
    return 0;
}

void acct_first (void)
{
}

void acct_worked (void)
{
}

void acct_practice (void)
{
}

void __fastcall__ acct_town (struct between* t)
{
    t->rhet = t->intr = BASE;
}

/* A tongue or a script opened ("*delvic=81", "sindarin=11"): its line, "script delvic", "language sindarin" (no mastery: the
** null ruleset has none) */
unsigned char __fastcall__ acct_learnt (const char* what)
{
    char* l;

    if (ntongue == TONGUES || nlearnt == BETWEEN_LEARNT) {
        return 0;
    }
    l = learnt[nlearnt++];
    strcpy (l, *what == '*' ? "script " : "language ");
    strncat (l, what + (*what == '*'), strcspn (what + (*what == '*'), "="));
    ++ntongue;
    return 1;
}
