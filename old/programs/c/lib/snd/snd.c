/*
** snd.c - snd.h: the YM2151 through /dev/snd.  Register/value pairs are written to it (the ROM's library takes
** the registers the chip doesn't have as its commands: SND_R_*, in lib/hydra.inc), its IO_CTL codes claim and
** release channels, clear the chip and set the master volume, and a read gives back the registers as written.
** One fd, opened the first time it's needed, and closed at the end (which gives the claimed channels back).
*/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <snd.h>

#define IO_MODE_RDWR        0x03
#define SND_CTL_INIT        1
#define SND_CTL_CLAIM       4
#define SND_CTL_RELEASE     5
#define SND_CTL_VOLUME      6
#define SND_R_CH            0x02
#define SND_R_PATCH         0x03
#define SND_R_NOTE          0x04
#define SND_R_OFF           0x05
#define SND_R_VOL           0x06
#define SND_R_PAN           0x07
#define SND_R_BEND          0x09
#define SND_R_DRUM          0x0A

int __fastcall__ _hy_open (const char* name, unsigned char mode);
int __fastcall__ _hy_read (int fd, void* buf, unsigned count);
int __fastcall__ _hy_write (int fd, const void* buf, unsigned count);
int __fastcall__ _hy_ctl (int fd, unsigned char code, unsigned char arg);

static int fd = -1;
static unsigned char cmd[4];

int __fastcall__ snd_open (void)
{
    if (fd < 0) {
        fd = _hy_open ("/dev/snd", IO_MODE_RDWR);
    }
    return fd < 0 ? -1 : 0;
}

void snd_close (void)
{
    if (fd >= 0) {
        close (fd);
        fd = -1;
    }
}

static int __fastcall__ ctl (unsigned char code, unsigned char arg)
{
    if (snd_open () < 0 || _hy_ctl (fd, code, arg) < 0) {
        return -1;
    }
    return 0;
}

int __fastcall__ snd_claim (unsigned char mask)
{
    return ctl (SND_CTL_CLAIM, mask);
}

int __fastcall__ snd_release (unsigned char mask)
{
    return ctl (SND_CTL_RELEASE, mask);
}

int snd_reset (void)
{
    return ctl (SND_CTL_INIT, 0);
}

int __fastcall__ snd_volume (unsigned char vol)
{
    return ctl (SND_CTL_VOLUME, vol);
}

int __fastcall__ snd_writes (const unsigned char* pairs, unsigned n)
{
    if (snd_open () < 0 || _hy_write (fd, pairs, n * 2) < 0) {
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
    cmd[0] = SND_R_CH;
    cmd[1] = ch;
    cmd[2] = reg;
    cmd[3] = val;
    return snd_writes (cmd, 2);
}

int __fastcall__ snd_patch (unsigned char ch, unsigned char patch)
{
    return command (ch, SND_R_PATCH, patch);
}

int __fastcall__ snd_note (unsigned char ch, unsigned char note)
{
    return command (ch, SND_R_NOTE, note);
}

int __fastcall__ snd_off (unsigned char ch)
{
    return command (ch, SND_R_OFF, 0);
}

int __fastcall__ snd_vol (unsigned char ch, unsigned char vol)
{
    return command (ch, SND_R_VOL, vol);
}

int __fastcall__ snd_pan (unsigned char ch, unsigned char pan)
{
    return command (ch, SND_R_PAN, pan);
}

int __fastcall__ snd_bend (unsigned char ch, signed char bend)
{
    return command (ch, SND_R_BEND, (unsigned char) bend);
}

int __fastcall__ snd_drum (unsigned char ch, unsigned char note)
{
    return command (ch, SND_R_DRUM, note);
}

int __fastcall__ snd_regs (unsigned char* regs)
{
    unsigned n = 0;
    int r;

    if (snd_open () < 0 || lseek (fd, 0, SEEK_SET) < 0) {
        return -1;
    }
    while (n < 256) {
        r = _hy_read (fd, regs + n, 256 - n);
        if (r <= 0) {
            return r < 0 ? -1 : _directerrno (EIO);
        }
        n += r;
    }
    return 0;
}
