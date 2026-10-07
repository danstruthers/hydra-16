/*
** tones.c - the sound library (snd.h): a patch on four claimed channels, a scale, a chord that fades, a bend and
** some drums; a tune by the notes' names, the bell, a frequency; then the registers read back; a line of MML and
** a chord, by play.  tones [PATCH] (0-162;
** default 0, a piano)
**   % tones 29 &
*/

#include <stdio.h>
#include <stdlib.h>
#include <hydra.h>
#include <snd.h>

static const unsigned char scale[] = { 60, 62, 64, 65, 67, 69, 71, 72 };   /* C major, from middle C */
static unsigned char regs[256];                     /* (Static: a function's locals are 256 bytes at most) */

int main (int argc, char* argv[])
{
    unsigned char patch = argc > 1 ? atoi (argv[1]) : 0;
    unsigned char i, ch;
    signed char bend;

    if (snd_claim (0x0F) < 0) {                     /* Channels 0-3, this program's alone */
        perror ("tones: channels 0-3");
        hy_exits ("busy");
    }
    for (ch = 0; ch < 3; ++ch) {
        snd_patch (ch, patch);
    }
    for (i = 0; i < sizeof scale; ++i) {            /* The scale */
        snd_note (0, scale[i]);
        hy_sleep_ticks (30);
        snd_off (0);
    }
    snd_note (0, 60);                               /* A chord, C E G, fading out */
    snd_note (1, 64);
    snd_note (2, 67);
    hy_sleep_ticks (100);
    for (i = 127; i > 15; i -= 16) {
        for (ch = 0; ch < 3; ++ch) {
            snd_level (ch, i);
        }
        hy_sleep_ticks (10);
    }
    for (ch = 0; ch < 3; ++ch) {
        snd_off (ch);
        snd_level (ch, 127);
    }
    snd_note (0, 69);                               /* A, bent up a whole tone */
    for (bend = 0; bend < 120; bend += 8) {
        snd_bend (0, bend);
        hy_sleep_ticks (2);
    }
    snd_off (0);
    snd_bend (0, 0);
    for (i = 0; i < 8; ++i) {                       /* Kick, snare ... on channel 3 */
        snd_drum (3, i & 1 ? 38 : 36);
        hy_sleep_ticks (25);
    }
    i = snd_tune ("C4 1 E4 1 - 1 G4 2", 1, 600) == 0;      /* A tune on channel 1, a beat a tenth of a second */
    snd_beep ();
    snd_freq (2, 1000);                             /* 1000 Hz: B5 and 13 64ths */
    hy_sleep_ticks (20);
    snd_off (2);
    snd_regs (regs);
    printf ("tones: patch %u, $20 %02X, $28 %02X, C#4 %d, the tune %s, 1000 Hz %02X, ", patch, regs[0x20], regs[0x28],
        snd_note_of ("C#4"), i ? "played" : "not played", regs[0x2A]);
    snd_release (0x0F);                             /* (play's, a task of its own, to have them) */
    printf ("mml %d, chord %d\n", snd_mml (3, "t240 o5 l16 c e g"), snd_chord (0, "t240 o4 l8 c e g"));
    return 0;
}
