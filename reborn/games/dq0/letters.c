/*
** letters.c - the designators (the user's design, 2026-11-06, DECISIONS.md's: "capital letters for in-the-environment
** things ... and lower-case letters for items carried/worn"; hub.h's): a letter for each thing the character carries or
** wears, the world's (things.c's ix_make, the UI bank's low), shown in the inventory's list, "(a) a torch"; a capital
** for each thing it's seen, the UI's (the UI bank's cap), shown in the look's words and the list of what's in sight.
** Here: the letters kept in the save (part 22's, game.dat's after the shortcuts).
*/

#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "dq0.h"
#include "hub.h"

#pragma static-locals (on)                      /* (Its locals static: the code the less; nothing here is reentered) */

/* ==== Part 22's (seldom: the save, its start) */
#pragma code-name (push, "SW22CODE")
#pragma rodata-name (push, "SW22RODATA")
#pragma bss-name (push, "SW22RODATA")

#define LETS        (offsetof (struct uirec, cap) + sizeof UIREC->cap - offsetof (struct uirec, low))
typedef char lets_fit[offsetof (struct uirec, lowt) == offsetof (struct uirec, low) + sizeof UIREC->low &&
                      offsetof (struct uirec, cap) == offsetof (struct uirec, lowt) + sizeof UIREC->lowt ? 1 : -1];
static char lk[64];                             /* (Through the main RAM, a piece at a time: the swapped part's own) */

/* The letters kept: written to the save's fd (back 0), read back from it (1; fd -1: none, a new game's) */
void __fastcall__ s_letters_keep (int fd, unsigned char back)
{
    static char* at;
    static unsigned n;

    for (at = (char*) UIREC->low; at < (char*) UIREC->low + LETS; at += n) {
        n = (char*) UIREC->low + LETS - at;
        if (n > sizeof lk) {
            n = sizeof lk;
        }
        if (back) {
            if (fd < 0 || read (fd, lk, n) != n) {
                memset (lk, 0, n);
            }
            hub_map (UI_BANK);
            memcpy (at, lk, n);
        } else {
            hub_map (UI_BANK);
            memcpy (lk, at, n);
            hub_map (0);
            write (fd, lk, n);
        }
    }
    hub_map (0);
}

/* The names given, the shortcuts and the letters read back from the save (after its header; fd -1: none, a new game's:
** main.c's, part 1's room) */
void __fastcall__ s_keeps_back (int fd)
{
    names_keep (fd, 1);
    alias_keep (fd, 1);
    s_letters_keep (fd, 1);
}

#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)
