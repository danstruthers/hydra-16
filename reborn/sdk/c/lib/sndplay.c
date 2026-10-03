/*
** sndplay.c - snd_play(): a song (a ZSM file) in the system's player, play, as rc would start it ("play [-l] SONG
** [N]"), in a task of its own; its task, for hy_wait (and hy_note's HY_NOTE_KILL to stop it).  (A module of its
** own, so a program that plays notes doesn't bring hy_spawn with it.)
*/

#include <stdlib.h>
#include <errno.h>
#include <hydra.h>
#include <snd.h>

int __fastcall__ snd_play (const char* song, unsigned char loops)
{
    char n[4];
    char* argv[5];
    unsigned char i = 1;
    int task;

    argv[0] = "play";
    if (loops == SND_FOREVER) {
        argv[i++] = "-l";
    }
    argv[i++] = (char*) song;
    if (loops && loops != SND_FOREVER) {            /* (None: to the song's end, once) */
        utoa (loops, n, 10);
        argv[i++] = n;
    }
    argv[i] = 0;
    task = hy_spawn ("/bin/play", argv, 0);
    if (task < 0 && _oserror == HY_E_NOENT) {
        task = hy_spawn ("#m/play", argv, 0);      /* (No /bin: the ROM's) */
    }
    return task;
}
