# Sound

**snd** (`reborn/modules/snd/`, task C) makes the Hydra's sound chips one synthesizer of 24 channels: the YM2151's 8 FM
channels on the board, and, with a Vera X, its PSG's 16 voices.  Every language makes sound the same way, by writing
text commands to `/dev/sndctl`.  The Vera X's PCM (sampled sound) is the video driver's ([video](video.md#pcm)), and
`play` plays songs, scores and WAV files on all of it.

## Contents

1. [The chips](#the-chips)
2. [The files](#the-files)
3. [Making a sound from the shell](#making-a-sound-from-the-shell)
4. [The commands](#the-commands)
5. [Claiming channels](#claiming-channels)
6. [play: songs, scores and WAV files](#play-songs-scores-and-wav-files)
7. [Scores](#scores)
8. [From each language](#from-each-language)
9. [How it works](#how-it-works)

---

## The chips

| Channels | Chip | What it has |
| :------- | :--- | :---------- |
| 0-7 | **The YM2151** (OPM), on the board | 8 channels of 4-operator FM: 8 algorithms, envelopes, an LFO, a noise generator.  The sound of 1980s arcade machines and the Commander X16 |
| 8-23 | **The Vera X's PSG** | 16 voices: pulse (with a width), sawtooth, triangle or noise, a volume and a stereo place each; no envelope of its own |

Without a Vera X there are 8 channels; commands to 8-23 fail (`E_NODEV`).  Both chips come out of the board's mixer
and its headphone jack ([the audio path](../02-hardware/devices.md#the-ym2151-and-the-audio-path)).

---

## The files

`#a`, bound at `/dev`:

| File | Read | Write |
| :--- | :--- | :---- |
| `sndctl` | The state: `volume 100`, `channels 24`, then `claimed` and the channels claimed | Commands, a line each ([below](#the-commands)) |
| `snd` | The YM2151's registers as they were written (256 bytes: the chip's can't be read) | Register/value byte pairs, for the YM2151 |
| `psg` | The PSG's 64 registers as written | Register/value pairs for the PSG (0-63), as ZSM songs write them |
| `bell` | | Any write: a short beep on channel 7 (the console rings it at a BEL) |

---

## Making a sound from the shell

```
% echo note 0 60 >/dev/sndctl            # middle C on channel 0, in its patch
% echo off 0 >/dev/sndctl                # and off
% echo patch 1 40 >/dev/sndctl           # channel 1: General MIDI's violin (patch 40)
% echo note 1 69 >/dev/sndctl            # the A above middle C, 440 Hz
% echo note 8 72 >/dev/sndctl            # the PSG's first voice (with a Vera X)
% echo volume 50 >/dev/sndctl            # everything quieter
% play -m 0 t180 o4 l8 c d e f g         # a line of a tune
% play /rom/songs/scom.zsm               # a song
```

---

## The commands

Write them to `/dev/sndctl`.  A channel `CH` is 0-23; notes are MIDI's (60 is middle C, 69 the A at 440 Hz).

| Command | What it does |
| :------ | :----------- |
| `patch CH P` | The channel's sound: one of the driver's patches (the X16's, General MIDI's numbering).  On the PSG, a patch below 4 is a waveform |
| `note CH N` | Note N on (a new note on the channel replaces the last) |
| `off CH` | The note off (it fades as its envelope says) |
| `level CH V` | The channel's volume, 0-127 |
| `pan CH left\|right\|both` | Which speakers (or 0-3) |
| `bend CH B` | Bend the pitch, -128 to 127 |
| `drum CH N` | A General MIDI drum sound, N |
| `freq CH HZ` | A frequency, not a note (0: off) |
| `glide CH N` | Slide to note N (legato) |
| `wave CH pulse\|saw\|triangle\|noise [WIDTH]` | A PSG voice's waveform (and a pulse's width, 0-63, 63 a square) |
| `sens CH PMS AMS` | The YM2151 channel's sensitivity to the LFO (pitch, amplitude) |
| `lfo RATE PMD AMD WAVE` | The YM2151's LFO |
| `noise N\|off` | The YM2151's noise generator (on channel 7) |
| `reg R V [R V ...]` | Registers written directly |
| `volume N` | The master volume, 0-200 (100 as written; more is louder, to 23 dB) |
| `claim N [P]`, `release N [P]` | Claim channels ([below](#claiming-channels)) |
| `reset` | The chips and every setting cleared (claims stay) |

The master volume and each channel's level apply to everything, a song's raw register writes too: the driver adds them
to what reaches the chip and keeps what was written.

---

## Claiming channels

A program that wants channels to itself claims them: `claim N [P]`, N a mask of the YM2151's channels (bit n, channel
n) and P of the PSG's (bit n, channel 8 + n).  A claimed channel is its claimer's alone: other tasks' writes to it are
dropped, until it's released or the claimer closes its last file of `#a` (its end does: the channels are keyed off
then).  Claiming a channel another task has gives `E_BUSY`, and claims nothing.  `play` claims the channels a song uses.

---

## play: songs, scores and WAV files

| Command | What it does |
| :------ | :----------- |
| `play [-l] song.zsm [n]` | A **ZSM song** (the X16's format; Furnace exports it) on the YM2151, and with a Vera X its PSG and PCM parts: once, its loop n more times, or `-l` until Ctrl-C |
| `play [-l] score.mml` | A **score** ([below](#scores)), compiled as it plays |
| `play -o score.mml song.zsm` | The score compiled into a ZSM file instead |
| `play file.wav` | A **WAV file** on the Vera X's PCM: 8 or 16 bits, mono or stereo, up to 48,828 Hz (from a card, 8 bits mono to about 11 kHz) |
| `play [-l] -m ch MML ...` | A line of MML on channel ch: `play -m 0 t180 o4 l8 c d e f g` |
| `play [-l] -c ch NOTES ...` | A chord: each note on the next channel from ch: `play -c 0 o4 l2 I0 c e g` |
| `play -x ...` | With `-m` or `-c`: the X16's own MML (FMPLAY's and PSGPLAY's): `play -x -m 0 T120 O4 L8 CDEFG` |

`/rom/songs` has a few songs; HydraOS's SD card has more in `/sd/0/songs`.  `scom` plays one.

---

## Scores

A score is music as text, the language of the PC's score compiler too (`reborn/sim/tools/hysong.js`).  `play` plays one
as it is, compiling it as it goes into the same register writes at the same ticks as `hysong.js` would put in a ZSM file:

```
; A score: ; to the line's end is a comment
#tempo 120                        ; quarter notes a minute (120); #rate N: its ticks a second (200)
@piano { gm 0 }                   ; an instrument: one of the driver's patches, or a voice's operators
@lead { wave pulse 24 env 2 14 12 18 }   ; a PSG instrument: a waveform, an envelope (in the song's ticks)
A @piano o4 l8 c d e f g4 r4 c2   ; channel 0 (A-H: 0-7, the YM2151's); a channel's lines are joined in order
B @piano o3 l2 c [g e]2 c         ; [ ... ]N: repeated N times
I @lead o5 l8 e a b > c           ; channel 8 (I-X: 8-23, the Vera X's PSG's voices 0-15)
```

**Notes and rests:** `c` to `b` (`+` or `#` sharp, `-` flat), a length (1 a whole note ... 64; 3, 6, 12, 24, 48
triplets), dots, `^` ties; `r` a rest; `x N` a General MIDI drum.

**Commands:** `o`, `>`, `<` the octave; `l` the default length; `q` the part of a note held (eighths); `v` the volume
(0-127); `p l|r|c|0` the speakers; `k` transpose; `D` detune (64ths of a semitone); `M` and `L` the LFO; `N` the noise;
`y reg,val` a register; `_` a slide to the next note; `&` legato; `I N` the driver's patch N as the instrument.

**The PSG's channels** (I-X) take the same notes, lengths, rests, `o` `l` `q` `v` `p` `k` `D`, slides and legato, and
instruments of their own: `wave W [WIDTH]` (`pulse`, `saw`, `triangle`, `noise`) and `env A D S R` (A ticks rising
from silence to the note's volume, D falling by S steps to what it holds until the note's off, then R falling to
silence).  `x`, `M`, `L` and `N` are the YM2151's alone.

A score is read whole into memory (about 17K at most); a mistake is reported with its channel (`play: x.mml: channel 2:
no such drum`).  `/sd/0/songs/vera.mml` uses both chips.

---

## From each language

| Language | How |
| :------- | :-- |
| rc | `echo note 0 60 >/dev/sndctl`, `play` |
| C | `snd.h`: `snd_note`, `snd_off`, `snd_patch`, `snd_mml`, `snd_chord` ... ([C](../09-programming/c.md)) |
| HyForth | `lib sound`: `snd-note`, `snd-off`, `snd-patch`, `snd-mml`, `snd-chord` ... ([HyForth](../06-languages/hyforth.md)) |
| hylang | `(use "snd")`: `(snd-note ch n)`, `(snd-mml ch "...")` ... ([hylang](../06-languages/hylang.md)) |
| HyBASIC | `SOUND` and `PLAY` ([HyBASIC](../06-languages/hybasic.md)) |

---

## How it works

* **The YM2151's library** is the first Hydra software's, ported: a shadow of every register as written (the chip can't
  be read back); each carrier's total level written with its channel's volume and the master volume added, so a volume
  works on anything; register numbers below `$20` (which the chip doesn't have) are the driver's commands, which
  `sndctl`'s text turns into.
* **Pitch**: the YM2151's key code at its 3.58 MHz clock (octave and note), and a key fraction of 1/64 of a semitone for
  bends and detune.
* **The PSG** belongs to the video driver (the VERA is `vid`'s): `snd` keeps the PSG's registers as written and sends
  each request's changes to `vid` in one write (`#v/psg`).  A note sets its voice's frequency and full volume; off sets
  the volume to 0; volumes have the same attenuation as the YM2151's.
* **No interrupts**: the driver owns no line.  The YM2151's timer B didn't keep its period on the board
  ([the quirks](../02-hardware/quirks.md#the-ym2151s-timer-b)), so `play` times songs by the system's tick.
* **In the emulator**, `node sim/run.js -i --sound` plays it in a browser, and `--wav FILE` records it ([the
  emulator](../08-emulator.md)).
