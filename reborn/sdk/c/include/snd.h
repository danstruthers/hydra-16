/*
** snd.h - the YM2151 sound chip, through the sound driver (#a, at /dev: snd, sndctl).
**
** The chip has 8 channels (0-7), each a voice of 4 FM operators.  A channel plays a patch (an instrument: 0-127
** are General MIDI's, 128-162 drum and percussion sounds) at a MIDI note (60 is middle C, 69 A at 440 Hz).  Its
** volume (0-127) and the master volume scale what it plays, raw register writes too; its bend moves the pitch in
** 64ths of a semitone.  snd_drum plays one of General MIDI's drums (MIDI channel 10's note numbers).
**
** Claim the channels a program uses (snd_claim): other programs' writes to them are then dropped, and the
** console's bell leaves channel 7 alone while it's claimed.  They're given back when the program ends.  Anything
** else: snd_write, a register of the chip's (the YM2151's data sheet).  snd_regs reads back what was written.
** Each call is one request to the driver; snd_writes sends many register/value pairs in one.  The calls open
** /dev/snd (and /dev/sndctl) the first time; they return 0, or -1 with errno set (EBUSY: a channel another
** program has claimed).
**
** Songs: snd_play starts one (a ZSM file: the Commander X16's format, which Furnace exports) in the system's
** player, play, a task of its own, and returns at once with its task: hy_wait waits for its end, and hy_note's
** HY_NOTE_KILL stops it.
*/

#ifndef _SND_H
#define _SND_H

#include <hydracalls.h>

#define SND_CHANNELS        HY_SND_CHANNELS
#define SND_PATCHES         HY_SND_PATCHES  /* 0-127: General MIDI's programs; 128-162: drum and percussion sounds */
#define SND_PAN_LEFT        HY_SND_PAN_LEFT /* snd_pan's speakers */
#define SND_PAN_RIGHT       HY_SND_PAN_RIGHT
#define SND_PAN_BOTH        HY_SND_PAN_BOTH
#define SND_ALL             0xFF            /* snd_claim's mask: every channel */
#define SND_FOREVER         0xFF            /* snd_play's loops: the song's loop till it's stopped */

int snd_open (void);                                                    /* (The other calls open it too) */
void snd_close (void);
int __fastcall__ snd_claim (unsigned char mask);                        /* Bit n: channel n */
int __fastcall__ snd_release (unsigned char mask);
int snd_reset (void);                                                   /* The chip and every setting cleared */
int __fastcall__ snd_volume (unsigned char vol);                        /* The master volume, 0-200 (100: as
                                                                        **   written) */

int __fastcall__ snd_patch (unsigned char ch, unsigned char patch);
int __fastcall__ snd_note (unsigned char ch, unsigned char note);       /* Key on */
int __fastcall__ snd_off (unsigned char ch);                            /* Key off: the note's release */
int __fastcall__ snd_vol (unsigned char ch, unsigned char vol);         /* 0-127 */
int __fastcall__ snd_pan (unsigned char ch, unsigned char pan);         /* SND_PAN_* */
int __fastcall__ snd_bend (unsigned char ch, signed char bend);         /* 64ths of a semitone */
int __fastcall__ snd_drum (unsigned char ch, unsigned char note);       /* A General MIDI drum (35: kick, 38:
                                                                        **   snare, 42: closed hi-hat ...) */

int __fastcall__ snd_write (unsigned char reg, unsigned char val);      /* A chip register */
int __fastcall__ snd_writes (const unsigned char* pairs, unsigned n);   /* n register/value pairs */
int __fastcall__ snd_regs (unsigned char* regs);                        /* All 256, as written */

int __fastcall__ snd_play (const char* song, unsigned char loops);      /* A song (ZSM) in the player, play, and
                                                                        **   its loop that many more times
                                                                        **   (SND_FOREVER): its task, or -1 */

#endif
