/*
** jukebox.c - a song in the background (snd_play: the player, play) while the program counts, then stopped (a
** kill note), or waited for (hy_wait).  jukebox SONG [SECONDS] (none: to the song's end)
**   % jukebox /rom/songs/test.zsm 5
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
        fputs ("usage: jukebox SONG [SECONDS]\n", stderr);
        hy_exits ("usage");
    }
    task = snd_play (argv[1], 0);
    if (task < 0) {
        perror (argv[1]);
        return 1;
    }
    if (argc < 3) {
        printf ("playing %s (task %d)\n", argv[1], task);
        return hy_wait (task, 0);                   /* (Its code: 0 if it played) */
    }
    for (s = atoi (argv[2]); s > 0; --s) {
        printf ("%u\n", s);
        sleep (1);
    }
    hy_note (task, HY_NOTE_KILL);
    printf ("stopped: %d\n", hy_wait (task, 0));
    return 0;
}
