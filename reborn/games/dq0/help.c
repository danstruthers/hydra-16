/*
** help.c - (overlay 6) the commands listed (help, "?"): the game's lib folder's help.txt, each line a paragraph (a line
** begun with # not said); and a verb the game doesn't know, or doesn't do yet, said back as typed (main.c's command)
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "dq0.h"

/* The commands, listed: help.txt's lines, each read into lb (400 bytes: a longer line cut) */
void o_help (void)
{
    static int fd, n;
    static unsigned k, i;

    strcpy (lb, libdir);
    strcat (lb, "/help.txt");
    if ((fd = open (lb, O_RDONLY)) < 0) {
        msg ("The help isn't here (help.txt, beside the game's overlays).");
        return;
    }
    for (k = 0;;) {
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
        lb[i] = 0;
        if (i && lb[i - 1] == '\r') {
            lb[i - 1] = 0;
        }
        if (*lb != '#') {
            msg (lb);
        }
        i += i < k;
        memmove (lb, lb + i, k - i);
        k -= i;
    }
    close (fd);
}

/* The command's first word said: "I don't know how to bribe." (f 0: no verb of the game's), or one the game has but
** doesn't do yet, "You can't eat in this game yet." (f 1) */
void __fastcall__ o_unknown (unsigned char f)
{
    unsigned char d;

    for (d = 0; cl[d] && cl[d] != ' ' && d < 15; ++d) {
        lb[d] = cl[d];
    }
    lb[d] = 0;
    sayf (f ? "\nYou can't %s in this game yet." : "\nI don't know how to %s.", lb);
}
