/*
** jukebox.c - a song in the background (snd_play: the ROM's ZSM player) while the program counts, then stopped
** (hy_kill), or waited for (hy_wait).  jukebox SONG [SECONDS] (none: to the song's end)
**     0:/> jukebox bgm.zsm 5
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hydra.h>
#include <snd.h>

int main (int argc, char* argv[])
{
    int task;
    unsigned s;

    if (argc < 2) {
        printf ("usage: jukebox SONG [SECONDS]\n");
        return 2;
    }
    task = snd_play (argv[1], 0);
    if (task < 0) {
        perror (argv[1]);
        return 1;
    }
    if (argc < 3) {
        printf ("playing %s (task %d)\n", argv[1], task);
        return hy_wait (task, 0);                   /* (Its status: 0 if it played) */
    }
    for (s = atoi (argv[2]); s > 0; --s) {
        printf ("%u\n", s);
        sleep (1);
    }
    hy_kill (task);
    printf ("stopped: %d\n", hy_wait (task, 0));
    return 0;
}
