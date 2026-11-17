/*
** help.c - (overlay 1, overlay 6's till 2026-10-28: its room for parts.c's) the commands listed (help, "?"): the game's lib folder's help.txt, each line a paragraph (a line
** begun with # not said; "@own " the ruleset's own stance's word, RS_HELLO's).  (A verb the game doesn't know, or doesn't do yet, said back: refuse.c's since 2026-11-06)
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "dq0.h"

/* The commands, listed: help.txt's lines, each a paragraph, read into lb (400 bytes) a part at a time: a line longer
** than lb said to its last space, then the rest after it, its paragraph going on (say's: the space its gap), never a
** word cut (2026-11-17: three paragraphs past 400 bytes were said as two, two cut mid-word, "the w" "ell"; games/dq0/
** make.js's helpWhole holds every line to it, the build's).  The file opened the first time and kept open (hfd: its
** fd + 1), read from its start each time (an open on the card some 700,000 cycles) */
void o_help (void)
{
    static int fd, n, hfd;
    static unsigned k, i;
    static char* p;
    char w[sizeof rs_own + 2];
    unsigned j;                                 /* (The C stack's: the main RAM's room) */
    unsigned char go, skip, cut, c;             /* (go: a line's rest, its paragraph going on; skip: a comment's) */

    if (!hfd) {
        strcpy (lb, libdir);
        strcat (lb, "/help.txt");
        if ((fd = open (lb, O_RDONLY)) < 0) {
            msg ("The help isn't here (help.txt, beside the game's overlays).");
            return;
        }
        hfd = fd + 1;
    }
    fd = hfd - 1;
    lseek (fd, 0, SEEK_SET);
    for (k = go = 0;;) {
        n = read (fd, lb + k, sizeof lb - 1 - k);
        if (n > 0) {
            k += n;
        }
        for (i = 0; i < k && lb[i] != '\n'; ++i) {
        }
        if (i == k && n > 0 && k < sizeof lb - 1) {
            continue;                           /* (More of the line to come) */
        }
        if (!k) {
            break;
        }
        j = i + (i < k);                        /* (The next part's start: after its newline) */
        if ((cut = i == sizeof lb - 1) != 0) {  /* (No newline in lb: to its last space, the rest from that space; none,
                                                **   all of it) */
            for (j = i - 1; j && lb[j] != ' '; --j) {
            }
            i = j = j ? j : i;
        }
        c = lb[i];
        lb[i] = 0;
        if (i && lb[i - 1] == '\r') {
            lb[i - 1] = 0;
        }
        if (!go) {
            skip = *lb == '#';                  /* (A line begun with # not said, its rest neither) */
        }
        if (!skip) {
            if ((p = strstr (lb, "@own ")) != 0) {   /* (The ruleset's own stance's word and a comma, if it has one:
                                                **   RS_HELLO's, "counter, ") */
                *p = 0;
            }
            if (go) {
                say (lb);
            } else {
                msg (lb);
            }
            if (p) {
                if (*rs_own) {
                    strcpy (w, rs_own);
                    say (strcat (w, ", "));
                }
                say (p + 5);
            }
        }
        lb[i] = c;
        go = cut;
        memmove (lb, lb + j, k - j);
        k -= j;
    }
}
