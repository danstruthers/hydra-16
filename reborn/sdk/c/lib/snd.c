/*
** snd.c - snd.h: the YM2151 through the sound driver.  Register/value pairs are written to /dev/snd (the driver's
** library takes the registers the chip doesn't have as its commands: HY_SND_R_*), a read of it gives back the
** registers as written, and /dev/sndctl takes the words that claim and release channels, set the master volume and
** clear the chip.  Each opened the first time it's needed, and closed at the end (which gives the claimed channels
** back).  The PSG's channels (8-23) take the same commands.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <hydra.h>
#include <snd.h>

int __fastcall__ _hy_open (const char* name, unsigned char mode);

static int fd = -1, ctlfd = -1;
static unsigned char cmd[8];
static char line[16];

int snd_open (void)
{
    if (fd < 0) {
        fd = _hy_open ("/dev/snd", HY_O_RDWR);
    }
    return fd < 0 ? -1 : 0;
}

void snd_close (void)
{
    if (fd >= 0) {
        close (fd);
        fd = -1;
    }
    if (ctlfd >= 0) {
        close (ctlfd);
        ctlfd = -1;
    }
}

/* sndctl's word, and a number after it if there's one */
static int ctl (const char* word, unsigned char has, unsigned n)
{
    unsigned k;

    if (ctlfd < 0 && (ctlfd = _hy_open ("/dev/sndctl", HY_O_WRITE)) < 0) {
        return -1;
    }
    strcpy (line, word);
    if (has) {
        k = strlen (line);
        line[k] = ' ';
        utoa (n, line + k + 1, 10);
    }
    return write (ctlfd, line, strlen (line)) < 0 ? -1 : 0;
}

int __fastcall__ snd_claim (unsigned char mask)
{
    return ctl ("claim", 1, mask);
}

int __fastcall__ snd_release (unsigned char mask)
{
    return ctl ("release", 1, mask);
}

/* (The PSG's mask: sndctl's second, after the YM2151's) */
int __fastcall__ snd_claim_psg (unsigned mask)
{
    return ctl ("claim 0", 1, mask);
}

int __fastcall__ snd_release_psg (unsigned mask)
{
    return ctl ("release 0", 1, mask);
}

int snd_reset (void)
{
    return ctl ("reset", 0, 0);
}

int __fastcall__ snd_volume (unsigned char vol)
{
    return ctl ("volume", 1, vol);
}

int __fastcall__ snd_writes (const unsigned char* pairs, unsigned n)
{
    if (snd_open () < 0 || write (fd, pairs, n * 2) < 0) {
        return -1;
    }
    return 0;
}

int __fastcall__ snd_write (unsigned char reg, unsigned char val)
{
    cmd[0] = reg;
    cmd[1] = val;
    return snd_writes (cmd, 1);
}

/* A command for a channel: the channel, then the command (one write: another program's can't come between) */
static int __fastcall__ command (unsigned char ch, unsigned char reg, unsigned char val)
{
    cmd[0] = HY_SND_R_CH;
    cmd[1] = ch;
    cmd[2] = reg;
    cmd[3] = val;
    return snd_writes (cmd, 2);
}

int __fastcall__ snd_patch (unsigned char ch, unsigned char patch)
{
    return command (ch, HY_SND_R_PATCH, patch);
}

int __fastcall__ snd_note (unsigned char ch, unsigned char note)
{
    return command (ch, HY_SND_R_NOTE, note);
}

int __fastcall__ snd_off (unsigned char ch)
{
    return command (ch, HY_SND_R_OFF, 0);
}

int __fastcall__ snd_level (unsigned char ch, unsigned char level)
{
    return command (ch, HY_SND_R_VOL, level);
}

int __fastcall__ snd_pan (unsigned char ch, unsigned char pan)
{
    return command (ch, HY_SND_R_PAN, pan);
}

int __fastcall__ snd_bend (unsigned char ch, signed char bend)
{
    return command (ch, HY_SND_R_BEND, (unsigned char) bend);
}

int __fastcall__ snd_drum (unsigned char ch, unsigned char note)
{
    return command (ch, HY_SND_R_DRUM, note);
}

int __fastcall__ snd_freq (unsigned char ch, unsigned hz)
{
    cmd[0] = HY_SND_R_CH;
    cmd[1] = ch;
    cmd[2] = HY_SND_R_FREQ_LO;
    cmd[3] = hz & 0xFF;
    cmd[4] = HY_SND_R_FREQ;
    cmd[5] = hz >> 8;
    return snd_writes (cmd, 3);
}

int __fastcall__ snd_glide (unsigned char ch, unsigned char note)
{
    return command (ch, HY_SND_R_GLIDE, note);
}

int __fastcall__ snd_wave (unsigned char ch, unsigned char wave, unsigned char width)
{
    return command (ch, HY_SND_R_WAVE, wave << 6 | (width & 63));
}

/* The chip's own registers: the LFO ($18 its rate, $19 its depths: pitch's with bit 7, $1B its wave), a channel's
** sensitivities ($38 + ch), the noise ($0F) */
int __fastcall__ snd_lfo (unsigned char rate, unsigned char pmd, unsigned char amd, unsigned char wave)
{
    cmd[0] = 0x18;
    cmd[1] = rate;
    cmd[2] = 0x19;
    cmd[3] = 0x80 | pmd;
    cmd[4] = 0x19;
    cmd[5] = amd & 0x7F;
    cmd[6] = 0x1B;
    cmd[7] = wave & 3;
    return snd_writes (cmd, 4);
}

int __fastcall__ snd_sens (unsigned char ch, unsigned char pms, unsigned char ams)
{
    return snd_write (0x38 + ch, (pms & 7) << 4 | (ams & 3));
}

int __fastcall__ snd_noise (signed char n)
{
    return snd_write (0x0F, n < 0 ? 0 : 0x80 | (n & 31));
}

int snd_beep (void)
{
    int bell = _hy_open ("/dev/bell", HY_O_WRITE);

    if (bell < 0) {
        return -1;
    }
    if (write (bell, "\a", 1) < 0) {
        close (bell);
        return -1;
    }
    return close (bell);
}

int __fastcall__ snd_regs (unsigned char* regs)
{
    unsigned n = 0;
    int r;

    if (snd_open () < 0 || lseek (fd, 0, SEEK_SET) < 0) {
        return -1;
    }
    while (n < 256) {
        r = read (fd, regs + n, 256 - n);
        if (r <= 0) {
            return r < 0 ? -1 : _directerrno (EIO);
        }
        n += r;
    }
    return 0;
}
