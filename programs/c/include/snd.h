/*
** snd.h - the YM2151 sound chip, through /dev/snd and the ROM's sound library (programs/c/lib/snd).
**
** The chip has 8 channels (0-7), each a voice of 4 FM operators.  A channel plays a patch (an instrument: 0-127
** are General MIDI's, 128-162 drum and percussion sounds) at a MIDI note (60 is middle C, 69 A at 440 Hz).
** Its volume (0-127) and a master volume scale what it plays, raw register writes too; its bend moves the pitch
** in 64ths of a semitone.  snd_drum plays one of General MIDI's drums (MIDI channel 10's note numbers).
**
** Claim the channels a program uses (snd_claim): other programs' writes to them are then dropped, and the
** console's bell leaves channel 7 alone while it's claimed.  They're given back when the program ends.
** Anything else: snd_write, a register of the chip's (the YM2151's datasheet).  snd_regs reads back what was
** written.  Each call is one request to the sound driver; snd_writes sends many register/value pairs in one.
** The calls open /dev/snd the first time; they return 0, or -1 with errno set.
**
** Songs: snd_play starts one (a ZSM file: the Commander X16's format, which Furnace exports) in the ROM's player,
** a task of its own, and returns at once with its task: hy_wait waits for its end, hy_kill stops it.
*/

#ifndef _SND_H
#define _SND_H

#define SND_CHANNELS        8
#define SND_PATCHES         163         /* 0-127: General MIDI's programs; 128-162: drum and percussion sounds */
#define SND_PAN_LEFT        1           /* snd_pan's speakers */
#define SND_PAN_RIGHT       2
#define SND_PAN_BOTH        3
#define SND_ALL             0xFF        /* snd_claim's mask: every channel */
#define SND_FOREVER         0xFF        /* snd_play's loops: play the song's loop until it's stopped */

int __fastcall__ snd_open (void);                                       /* (The other calls open it too) */
void snd_close (void);
int __fastcall__ snd_claim (unsigned char mask);                        /* Bit n: channel n.  EBUSY: one's
                                                                        **   another program's */
int __fastcall__ snd_release (unsigned char mask);
int snd_reset (void);                                                   /* The chip and every setting cleared */
int __fastcall__ snd_volume (unsigned char vol);                        /* The master volume, 0-127 */

int __fastcall__ snd_patch (unsigned char ch, unsigned char patch);
int __fastcall__ snd_note (unsigned char ch, unsigned char note);       /* Key on */
int __fastcall__ snd_off (unsigned char ch);                            /* Key off: the note's release */
int __fastcall__ snd_vol (unsigned char ch, unsigned char vol);         /* 0-127 */
int __fastcall__ snd_pan (unsigned char ch, unsigned char pan);         /* SND_PAN_* */
int __fastcall__ snd_bend (unsigned char ch, signed char bend);         /* 64ths of a semitone */
int __fastcall__ snd_drum (unsigned char ch, unsigned char note);       /* A General MIDI drum (35: kick,
                                                                        **   38: snare, 42: closed hi-hat ...) */

int __fastcall__ snd_write (unsigned char reg, unsigned char val);      /* A chip register */
int __fastcall__ snd_writes (const unsigned char* pairs, unsigned n);   /* n register/value pairs */
int __fastcall__ snd_regs (unsigned char* regs);                        /* All 256, as written */

int __fastcall__ snd_play (const char* song, unsigned char loops);      /* A song (ZSM), and its loop that
                                                                        **   many more times (SND_FOREVER):
                                                                        **   its task, or -1 */

#endif
