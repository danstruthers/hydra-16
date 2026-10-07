/*
** sndplay.c - snd_play(): a song (a ZSM file) in the ROM's player, started as the shell's play would (a command
** shell runs "play SONG N"), in a task of its own; its task, for hy_wait and hy_kill.  (A module of its own, so
** a program that plays notes doesn't bring the command line with it.)
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hydra.h>
#include <snd.h>

int __fastcall__ snd_play (const char* song, unsigned char loops)
{
    char cmd[5 + HY_PATH_MAX + 5];
    unsigned n = strlen (song);

    if (n >= HY_PATH_MAX) {
        return _directerrno (EINVAL);
    }
    strcpy (cmd, "play ");
    strcpy (cmd + 5, song);
    if (loops) {                                    /* (None: to the song's end, once) */
        cmd[5 + n] = ' ';
        utoa (loops == SND_FOREVER ? 0 : loops, cmd + 6 + n, 10);
    }
    return hy_spawn (cmd);
}
