/*
** sndplay.c - snd_play(): a song (a ZSM file) in the system's player, play, as rc would start it ("play [-l] SONG
** [N]"), in a task of its own; its task, for hy_wait (and hy_note's HY_NOTE_KILL to stop it).  snd_mml() and
** snd_chord(): a line of MML, play -m and play -c, waited for.  (A module of its
** own, so a program that plays notes doesn't bring hy_spawn with it.)
*/

#include <stdlib.h>
#include <errno.h>
#include <hydra.h>
#include <snd.h>

/* play's -m or -c (flag): the MML on channel ch, waited for: its exit code, or -1 */
static int __fastcall__ play_line (const char* flag, unsigned char ch, const char* text)
{
    char n[4];
    char* argv[5];
    int task;

    argv[0] = "play";
    argv[1] = (char*) flag;
    utoa (ch, n, 10);
    argv[2] = n;
    argv[3] = (char*) text;
    argv[4] = 0;
    task = hy_spawn ("/bin/play", argv, 0);
    if (task < 0 && _oserror == HY_E_NOENT) {
        task = hy_spawn ("#m/play", argv, 0);
    }
    return task < 0 ? -1 : hy_wait (task, 0);
}

int __fastcall__ snd_mml (unsigned char ch, const char* mml)
{
    return play_line ("-m", ch, mml);
}

int __fastcall__ snd_chord (unsigned char ch, const char* notes)
{
    return play_line ("-c", ch, notes);
}

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
