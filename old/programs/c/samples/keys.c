/*
** keys.c - conio on the Hydra's console: the screen (clrscr, gotoxy, colours, reverse, screensize) and the keys
** (cgetc, raw: each key's code as it's typed, with the cursor and function keys decoded: hydra.h's CH_ codes),
** until q.
*/

#include <conio.h>
#include <hydra.h>

int main (void)
{
    unsigned char w, h;
    char c;

    clrscr ();
    screensize (&w, &h);
    textcolor (COLOR_YELLOW);
    cprintf ("keys: a %ux%u screen; type keys, q to end\n", w, h);
    textcolor (COLOR_WHITE);
    revers (1);
    cputsxy (0, 2, "codes:");
    revers (0);
    while ((c = cgetc ()) != 'q') {
        cprintf (" %02X", (unsigned char) c);
    }
    cprintf ("\nended at %u,%u\n", wherex (), wherey ());
    return 0;
}
