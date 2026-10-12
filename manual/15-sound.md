# Chapter 15. Sound

The Hydra has a real music synthesizer chip on its board: the **Yamaha YM2151**, the chip that played the music in
many 1980s arcade games and keyboards. With a Vera X card, it has a second sound chip too: the Vera X's **PSG**, with
sixteen simple voices, and **PCM**, which plays recorded sounds. This chapter shows how to play notes, songs and
sounds, and describes the sound driver's files.

## 15.1 The sound hardware

The **YM2151** (U38) makes sound by **FM synthesis**. It has 8 **channels**, so it can play 8 notes at once. Each
channel has 4 **operators**: little oscillators, each making a tone, which change each other's sound as they're
combined. Different settings of the operators make sounds like a piano, a flute, a bass guitar or a drum. A set of
settings is called a **patch**, or an instrument.

The YM2151 doesn't make sound directly: it sends numbers to the **YM3012** (U41), which turns them into a voltage
for the left and right speakers. The **TL074** (U42) smooths and boosts them, and the **mixer** adds the sound from
slot 0 (the Vera X's) and from **Audio In** (J29). The **NJM4556A** (IC9) is the headphone amplifier, and the sound
comes out of J26, a 3.5 mm stereo jack, for headphones or powered speakers.

The **Vera X's PSG** (programmable sound generator) has 16 voices. Each makes a simple wave (a pulse, a sawtooth, a
triangle or noise) at any pitch and volume, in the left or right speaker or both. Its **PCM** plays sampled sounds:
recordings, like a WAV file.

The sound driver, `snd` (task C), calls these all **channels**:

| Channels | Chip |
|---|---|
| 0 to 7 | The YM2151 |
| 8 to 23 | The Vera X's PSG voices 0 to 15 (only with a Vera X) |

## 15.2 Playing notes from the shell

The simplest way to make a sound is to write commands to `/dev/sndctl`. Choose an instrument, play a note, and stop
it:

```text
/> echo patch 0 0 >/dev/sndctl
/> echo note 0 60 >/dev/sndctl
/> echo off 0 >/dev/sndctl
```

`patch 0 0` gives channel 0 patch 0, the acoustic piano. `note 0 60` plays note 60, **middle C**, on channel 0.
`off 0` lets it go. Notes are numbered the way electronic instruments number them (the **MIDI** numbers): each
number is a **semitone** (one key on a piano, black or white) higher than the one before, and every 12 numbers is an
**octave**:

| Note | C | C# | D | D# | E | F | F# | G | G# | A | A# | B |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Octave 3 | 48 | 49 | 50 | 51 | 52 | 53 | 54 | 55 | 56 | 57 | 58 | 59 |
| Octave 4 | **60** | 61 | 62 | 63 | 64 | 65 | 66 | 67 | 68 | **69** | 70 | 71 |
| Octave 5 | 72 | 73 | 74 | 75 | 76 | 77 | 78 | 79 | 80 | 81 | 82 | 83 |

Note 60 is middle C, and note 69 is the A above it, which sounds at 440 vibrations a second (440 Hz): the note an
orchestra tunes to.

It's quicker to play a tune with `play -m`, which takes a line of **MML** (section 15.6):

```text
/> play -m 0 'o4 l8 c d e f g a b > c'
/> play -m 0 'I40 o5 l4 c c g g a a g2'
```

The first plays a scale of C on channel 0. The second plays "Twinkle, Twinkle, Little Star" on patch 40, the violin.
`play -c` plays a **chord**, a note on each channel from the one given:

```text
/> play -c 0 c e g
```

## 15.3 /dev/sndctl: the sound driver's commands

| Command | What it does |
|---|---|
| `patch` *ch p* | Give channel *ch* patch *p* (section 15.4) |
| `note` *ch n* | Play note *n* on channel *ch* |
| `off` *ch* | Let channel *ch*'s note go |
| `level` *ch v* | Channel *ch*'s volume, 0 to 127 |
| `pan` *ch* `left` \| `right` \| `both` | Which speakers channel *ch* plays in (or 0 to 3: none, left, right, both) |
| `bend` *ch b* | Bend channel *ch*'s pitch, -128 to 127: 64 is a semitone up |
| `drum` *ch n* | Play General MIDI drum *n* on channel *ch* (section 15.4) |
| `freq` *ch hz* | Play a tone of *hz* vibrations a second on channel *ch* (0: stop) |
| `glide` *ch n* | Change channel *ch*'s note to *n* smoothly, without playing it again (**legato**) |
| `wave` *ch* `pulse` \| `saw` \| `triangle` \| `noise` [*width*] | A PSG channel's wave (or 0 to 3); a pulse's width, 0 to 63 (63: a square wave) |
| `sens` *ch pms ams* | How much the LFO (below) changes channel *ch*'s pitch (0-7) and volume (0-3) |
| `lfo` *rate pmd amd wave* | The YM2151's **LFO**, a slow wobble for vibrato and tremolo: its rate (0-255), its pitch and volume depths (0-127), and its wave (0 saw, 1 square, 2 triangle, 3 noise) |
| `noise` *n* \| `off` | The YM2151's noise (on channel 7), at frequency *n* (0-31), or off |
| `reg` *r v* [*r v* ...] | Write value *v* to YM2151 register *r* |
| `volume` *n* | The master volume of everything, 0 to 200: 100 is normal, more is louder |
| `reset` | Silence the chips, and clear every setting |
| `claim` *n* [*p*] | Claim channels: *n*'s bits are channels 0-7, *p*'s channels 8-23 |
| `release` *n* [*p*] | Release them |

Numbers can be written in hex with `$`: `claim $FF` claims all eight YM2151 channels.

Reading `sndctl` shows the master volume, how many channels there are (8, or 24 with a Vera X), and which channels
are claimed:

```session
/> cat /dev/sndctl
volume 100
channels 8
claimed
```

**Claims.** Any program can play on any channel, which is fine until two of them play on the same one. A program
that's playing a song claims the channels it uses; while they're claimed, other programs' commands for them are
ignored. A claim belongs to the program that made it, and ends when it releases the channels, or closes its last
sound file. So a claim made with `echo` ends straight away, when `echo` ends!

## 15.4 Patches and drums

The sound driver has 163 patches, the same as the Commander X16's. Patches 0 to 127 are the instruments of **General
MIDI**, a standard list used by electronic instruments everywhere:

| No. | Instrument | No. | Instrument | No. | Instrument | No. | Instrument |
|---|---|---|---|---|---|---|---|
| 0 | Acoustic Piano | 32 | Acoustic Bass | 64 | Soprano Sax | 96 | FX 1 Rain |
| 1 | Bright Acoustic Piano | 33 | Electric Finger Bass | 65 | Alto Sax | 97 | FX 2 Soundtrack |
| 2 | Electric Grand Piano | 34 | Electric Picked Bass | 66 | Tenor Sax | 98 | FX 3 Crystal |
| 3 | Honky-Tonk Piano | 35 | Fretless Bass | 67 | Baritone Sax | 99 | FX 4 Atmosphere |
| 4 | Electric Piano 1 | 36 | Slap Bass 1 | 68 | Oboe | 100 | FX 5 Brightness |
| 5 | Electric Piano 2 | 37 | Slap Bass 2 | 69 | English Horn | 101 | FX 6 Goblins |
| 6 | Harpsichord | 38 | Synth Bass 1 | 70 | Bassoon | 102 | FX 7 Echoes |
| 7 | Clavinet | 39 | Synth Bass 2 | 71 | Clarinet | 103 | FX 8 Sci-Fi |
| 8 | Celesta | 40 | Violin | 72 | Piccolo | 104 | Sitar |
| 9 | Glockenspiel | 41 | Viola | 73 | Flute | 105 | Banjo |
| 10 | Music Box | 42 | Cello | 74 | Recorder | 106 | Shamisen |
| 11 | Vibraphone | 43 | Contrabass | 75 | Pan Flute | 107 | Koto |
| 12 | Marimba | 44 | Tremolo Strings | 76 | Blown Bottle | 108 | Kalimba |
| 13 | Xylophone | 45 | Pizzicato Strings | 77 | Shakuhachi | 109 | Bagpipes |
| 14 | Tubular Bells | 46 | Harp | 78 | Whistle | 110 | Fiddle |
| 15 | Dulcimer | 47 | Timpani | 79 | Ocarina | 111 | Shanai |
| 16 | Drawbar Organ | 48 | String Ensemble 1 | 80 | Lead 1 Square | 112 | Tinkle Bell |
| 17 | Percussive Organ | 49 | String Ensemble 2 | 81 | Lead 2 Saw | 113 | Agogo |
| 18 | Rock Organ | 50 | Synth Strings 1 | 82 | Lead 3 Triangle | 114 | Steel Drum |
| 19 | Church Organ | 51 | Synth Strings 2 | 83 | Lead 4 Chiff Sine | 115 | Woodblock |
| 20 | Reed Organ | 52 | Choir Aahs | 84 | Lead 5 Charang | 116 | Taiko Drum |
| 21 | Accordion | 53 | Doo | 85 | Lead 6 Voice | 117 | Melodic Tom |
| 22 | Harmonica | 54 | Synth Choir | 86 | Lead 7 Fifths | 118 | Synth Drum |
| 23 | Bandoneon | 55 | Orch Hit | 87 | Lead 8 Solo | 119 | Reverse Cymbal |
| 24 | Acoustic Nylon Guitar | 56 | Trumpet | 88 | Pad 1 Fantasia | 120 | Guitar Fret Noise |
| 25 | Acoustic Steel Guitar | 57 | Trombone | 89 | Pad 2 Warm | 121 | Breath Noise |
| 26 | Electric Jazz Guitar | 58 | Tuba | 90 | Pad 3 Poly | 122 | Seashore |
| 27 | Electric Clean Guitar | 59 | Muted Trumpet | 91 | Pad 4 Choir | 123 | Bird Tweet |
| 28 | Electric Muted Guitar | 60 | French Horn | 92 | Pad 5 Bowed | 124 | Telephone |
| 29 | Electric Overdriven Guitar | 61 | Brass Section | 93 | Pad 6 Metallic | 125 | Helicopter |
| 30 | Electric Distorted Guitar | 62 | Synth Brass 1 | 94 | Pad 7 Halo | 126 | Applause |
| 31 | Electric Guitar Harmonics | 63 | Synth Brass 2 | 95 | Pad 8 Sweep | 127 | Gunshot |

Patches 128 to 162 are drum sounds:

| No. | Drum sound | No. | Drum sound | No. | Drum sound |
|---|---|---|---|---|---|
| 128 | Silent | 140 | Pedal Hi-Hat | 152 | Long Guiro |
| 129 | Snare Roll | 141 | Open Hi-Hat | 153 | Mute Cuica |
| 130 | Snap | 142 | Crash | 154 | Open Cuica |
| 131 | High Q | 143 | Ride Cymbal | 155 | Mute Triangle |
| 132 | Scratch | 144 | Splash Cymbal | 156 | Open Triangle |
| 133 | Square Click | 145 | Tambourine | 157 | Jingle Bell |
| 134 | Kick | 146 | Vibraslap | 158 | Bell Tree |
| 135 | Rim | 147 | Bongo | 159 | Mute Surdo |
| 136 | Snare | 148 | Maracas | 160 | Pure Sine |
| 137 | Clap | 149 | Short Whistle | 161 | Timbale |
| 138 | Tom | 150 | Long Whistle | 162 | Open Surdo |
| 139 | Closed Hi-Hat | 151 | Short Guiro |  |  |

On a PSG channel (8 to 23), a patch below 4 chooses a wave: 0 pulse, 1 sawtooth, 2 triangle, 3 noise.

**Drums.** `drum` *ch n* plays a drum by its **General MIDI drum number**, which picks the right patch and pitch. The
most useful are:

| Number | Drum | Number | Drum |
|---|---|---|---|
| 35, 36 | Bass drum (kick) | 49, 57 | Crash cymbal |
| 37 | Side stick (rim) | 51, 59 | Ride cymbal |
| 38, 40 | Snare drum | 54 | Tambourine |
| 39 | Hand clap | 55 | Splash cymbal |
| 41, 43, 45, 47, 48, 50 | Toms, low to high | 56 | Cowbell |
| 42 | Closed hi-hat | 60, 61 | Bongos |
| 44 | Pedal hi-hat | 69 | Cabasa |
| 46 | Open hi-hat | 70 | Maracas |

## 15.5 The other sound files

**/dev/snd** is the YM2151 itself. Each pair of bytes written to it is a **register** number and a value: the chip
has 256 registers, and every setting of every operator is in one. The driver keeps a copy of what was written, which
a read gives back (the chip's own registers can't be read). The driver adjusts the levels for the channels' and the
master volume, so a song written as register writes still follows the volume settings.

The YM2151's main registers are:

| Register | What it holds |
|---|---|
| $08 | Key on and off: bits 6-3 the operators to start (0: stop), bits 2-0 the channel |
| $0F | Noise: bit 7 on, bits 4-0 its frequency |
| $18 | The LFO's rate |
| $19 | The LFO's depth: bit 7 0 for volume, 1 for pitch; bits 6-0 the depth |
| $1B | Bits 1-0 the LFO's wave; bits 7-6 the CT1 and CT2 outputs (J9) |
| $20-$27 | For each channel: bits 7-6 the speakers (R, L), bits 5-3 the feedback, bits 2-0 the **algorithm** (how the operators are joined) |
| $28-$2F | Each channel's pitch: its octave (bits 6-4) and note (bits 3-0) |
| $30-$37 | Each channel's pitch, finer: in 64ths of a semitone (bits 7-2) |
| $38-$3F | Each channel's LFO sensitivity: pitch (bits 6-4), volume (bits 1-0) |
| $40-$5F | Each operator's detune 1 (bits 6-4) and frequency multiple (bits 3-0) |
| $60-$7F | Each operator's **total level**: its loudness, 0 loudest to 127 silent |
| $80-$9F | Each operator's key scaling (bits 7-6) and attack rate (bits 4-0) |
| $A0-$BF | Each operator's LFO volume on (bit 7) and first decay rate (bits 4-0) |
| $C0-$DF | Each operator's detune 2 (bits 7-6) and second decay rate (bits 4-0) |
| $E0-$FF | Each operator's first decay level (bits 7-4) and release rate (bits 3-0) |

In the operators' registers, the 32 registers of each block are 8 channels for each of 4 operators: register
$40 + 8 × *operator* + *channel*. The full story is in Yamaha's YM2151 application manual. The register numbers
the chip doesn't have, from $02 to $0E (but $08, key on), are the driver's own commands, the same ones as
`sndctl`'s, for programs that want to send them as bytes.

**/dev/psg** is the Vera X's PSG: pairs of a register (0 to 63) and a value, four registers for each voice:

| Register | What it holds |
|---|---|
| 4*v* + 0, 4*v* + 1 | Voice *v*'s frequency, low byte then high byte |
| 4*v* + 2 | Bit 7 the right speaker, bit 6 the left; bits 5-0 the volume (0 to 63) |
| 4*v* + 3 | Bits 7-6 the wave (0 pulse, 1 saw, 2 triangle, 3 noise); bits 5-0 a pulse's width |

**/dev/bell** rings the bell: a short beep on channel 7, unless a program has claimed channel 7. The console writes
it when a program prints the BEL character (7).

## 15.6 MML: writing music

**MML**, the **music macro language**, is a short way of writing a tune as text. Each letter is a note, and numbers
say how long. `play -m` plays a line of it, and a **score** file holds a whole song.

| MML | What it does |
|---|---|
| `c d e f g a b` | A note; `+` or `#` after it makes it sharp, `-` flat: `c#`, `b-` |
| *note length* | A length after a note: 1 a whole note, 2 a half, 4 a quarter, 8 an eighth, 16, 32, 64 (and 3, 6, 12, 24, 48 for triplets): `c4` |
| `.` | After a length: half as long again (`c4.`) |
| `^` *length* | Tie more on: `c4^16` |
| `r` [*length*] | A rest: silence |
| `o` *n* | The octave: `o4 c` is middle C |
| `>` `<` | Up an octave; down an octave |
| `l` *n* | The length for notes that don't give one |
| `v` *n* | The volume, 0 to 127 |
| `q` *n* | How much of each note is held, in eighths (1 to 8; usually 7) |
| `p l` \| `r` \| `c` \| `0` | The speakers: left, right, both (centre), none |
| `@`*name* | Use the score's instrument *name* |
| `I` *n* | Use patch *n* as the instrument |
| `x` *n* [*length*] | A General MIDI drum |
| `&` | Between notes: join them (no new attack) |
| `_` | Before a note: slide to it from the last |
| `k` *n* | Transpose by *n* semitones |
| `D` *n* | Detune by *n* 64ths of a semitone |
| `M` *pms*,*ams* | The channel's LFO sensitivity |
| `L` *rate*,*pmd*,*amd*,*wave* | The LFO |
| `N` *n*, `N-` | The noise on, at frequency *n*; off |
| `y` *reg*,*val* | Write any register |
| `[` ... `]` *n* | Repeat what's inside *n* times |
| `|` | A bar line (it does nothing, but helps you read) |

A **score** is a text file whose name ends in `.mml`. Each line is one of these:

| Line | What it is |
|---|---|
| `; ...` | A comment |
| `#tempo` *n* | The speed: *n* quarter notes a minute (120 unless you say) |
| `#rate` *n* | The song's ticks a second (200, the Hydra's own tick) |
| `#title` *text* | The song's title |
| `@`*name* `{` ... `}` | An instrument (below) |
| `A` *mml* ... `H` *mml* | MML for channel 0 ... 7 (the YM2151); a channel's lines are joined, in order |
| `I` *mml* ... `X` *mml* | MML for channel 8 ... 23 (the PSG) |

An instrument is either `gm` *n*, one of the patches, or the YM2151 settings themselves: `alg` *a* `fb` *f* `pms`
*p* `ams` *a*, then for each operator, `m1`, `m2`, `c1` and `c2`, its `mul`, `dt1`, `dt2`, `tl`, `ks`, `ar`, `d1r`,
`d2r`, `d1l`, `rr` and `am` (the registers' fields, as in section 15.5). A PSG instrument is `wave` *w* [*width*]
and `env` *a d s r*, a volume **envelope**: *a* ticks rising to the note's volume, *d* ticks falling by *s* steps,
held till the note is let go, then *r* ticks falling to silence.

Here's a little score, with a tune, a bass line and drums:

```text
; A little tune
#tempo 120
@piano { gm 0 }
@bass { gm 33 }
A @piano o4 l8 c d e f g4 g4 a a a a g2
B @bass o2 l4 c c f c g g c2
C x35 4 x38 4 x35 4 x38 4 x35 4 x38 4 x35 4 x38 4
```

Save it as `/ram/tune.mml` (with `edit`), and play it:

```text
/> play /ram/tune.mml
```

## 15.7 play

`play` plays scores, and **ZSM** files: the song format of the Commander X16, which Furnace, a free music program for
PCs, can write. A ZSM file is a song already turned into register writes, so it takes less work to play.

| Command | What it does |
|---|---|
| `play` *song* | Play a score (`.mml`), a ZSM file, or a WAV file (on a Vera X) |
| `play` *song n* | Play it, and its repeating part *n* more times |
| `play -l` *song* | Play it over and over, until you press <kbd>CTRL</kbd>-<kbd>C</kbd> |
| `play -o` *score.mml song.zsm* | Turn a score into a ZSM file, instead of playing it |
| `play -m` *ch mml* ... | Play a line of MML on channel *ch* |
| `play -c` *ch notes* ... | Play a chord: each note on a channel, from *ch* on |
| `play -x` ... | With `-m` or `-c`: the Commander X16's MML (from its BASIC's `FMPLAY` and `PSGPLAY`) |

<!-- session
echo '; A little tune' >/ram/tune.mml
echo '#tempo 120' >>/ram/tune.mml
echo '@piano { gm 0 }' >>/ram/tune.mml
echo '@bass { gm 33 }' >>/ram/tune.mml
echo 'A @piano o4 l8 c d e f g4 g4 a a a a g2' >>/ram/tune.mml
echo 'B @bass o2 l4 c c f c g g c2' >>/ram/tune.mml
echo 'C x35 4 x38 4 x35 4 x38 4 x35 4 x38 4 x35 4 x38 4' >>/ram/tune.mml
-->

```session
/> play -o /ram/tune.mml /ram/tune.zsm
/> ls -l /ram/tune.zsm
--rw-rw-rw- fr      703 2000-01-01 00:00 /ram/tune.zsm
```

While it plays, `play` claims the channels the song uses, so other programs can't spoil it. The SD card that comes
with HydraOS has more songs, in `/sd/0/songs`.

> **TIP:** On a PC, the `hysong.js` tool in `reborn/sim/tools` turns a score into a ZSM file too, and into a VGM file
> you can listen to on the PC.

## 15.8 Recorded sound

A Vera X can play **samples**: recorded sound, as numbers. `play` plays a WAV file, of 8 or 16 bits, mono or stereo,
at up to 48,828 samples a second. Programs can play samples themselves through the Vera X's `pcm` and `pcmctl` files
(Chapter 16).

## 15.9 Audio In

J29, **Audio In**, mixes sound from another device into the Hydra's: pin 1 is one channel, and pin 2 the other. Each
goes into the mixer through a 10K resistor, as the YM2151's and slot 0's sound do. The other device's ground must be
connected to the Hydra's ground too: at the I/O header, J27 pin 1, for example. Keep the sound's level to a line
level, like a PC's line output: never connect a speaker output.
