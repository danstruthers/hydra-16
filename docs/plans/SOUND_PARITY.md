## **Sound words: an assessment, and parity everywhere**

An assessment of the sound vocabulary in the rebuilt system (`reborn/`, October 2026): what each place has (the sound driver's files, C, HyForth, hylang, BASIC), how far apart they are, and a plan to bring them all to one set: the largest that exists anywhere the Hydra draws on (its own driver and score language, the old system, and the Commander X16's BASIC, whose chips the Hydra now shares: the YM2151 on the board, the VERA's PSG and PCM on the Vera X).  Nothing here is built yet; the questions at the end are the user's.

### **Contents**
1. [What exists](#what-exists)
2. [The matrix](#the-matrix)
3. [Where they differ today](#where-they-differ-today)
4. [The target: the largest set, in the Hydra's units](#the-target-the-largest-set-in-the-hydras-units)
5. [How each place reaches it](#how-each-place-reaches-it)
6. [The order of work](#the-order-of-work)
7. [Questions](#questions)

---

### **What exists**

**The driver** (`modules/snd`, `#a` at `/dev`), the one owner of the YM2151:
* `/dev/snd`: register/value byte pairs.  The registers the chip doesn't have (below `$20`) are commands for the channel `SND_R_CH` chose: `SND_R_PATCH` (0-162: General MIDI's 128, then 35 drums and percussion), `SND_R_NOTE` (a MIDI note, with the channel's bend), `SND_R_OFF`, `SND_R_VOL` (0-127), `SND_R_PAN` (1 left, 2 right, 3 both), `SND_R_BEND` (signed 64ths of a semitone), `SND_R_DRUM` (a General MIDI drum note, its patch and pitch).  Read: the 256 registers as written (the shadow).
* `/dev/sndctl`: `claim N`, `release N` (a mask of channels), `volume N` (the master, 0-200), `reset`.  Read: the volume and the channels claimed.
* `/dev/bell`: the console's beep (channel 7, unless claimed).

**C** (`sdk/c/include/snd.h`): `snd_claim (mask)`, `snd_release (mask)`, `snd_reset`, `snd_volume (v)`, `snd_patch (ch, p)`, `snd_note (ch, n)`, `snd_off (ch)`, `snd_vol (ch, v)`, `snd_pan (ch, pan)`, `snd_bend (ch, b)`, `snd_drum (ch, n)`, `snd_write (reg, val)`, `snd_writes (pairs, n)`, `snd_regs (buf)` (the shadow read), `snd_play (song, loops)` (a ZSM song in the player, `play`, a task of its own), `snd_open`, `snd_close`.  No beep (a program prints `'\a'`).

**HyForth** (`/lib/forth/sound.fl`, `lib sound`): `snd-reset`, `snd-volume ( v )`, `snd-claim ( mask )`, `snd-release ( mask )`, `snd-reg ( reg val )`, `snd-patch ( ch p )`, `snd-note ( ch n )`, `snd-off ( ch )`, `snd-vol ( ch v )`, `snd-pan ( ch pan )`, `snd-bend ( ch n )`, `snd-drum ( ch n )`, `note-of ( c-addr u -- n )` (`C#4` 61), `tune ( c-addr u ch tempo )` (`C4 1 E4 1 - 1 G4 2`: notes and beats, timed by the tick); `beep` (Facility's).  Songs by `s" play song.zsm" sh`.

**hylang** (`/lib/hylang/snd.hl`, `(use "snd")`): `(snd-claim ch ...)`, `(snd-release ch ...)` (channels, not a mask), `(snd-volume v)`, `(snd-reset)`, `(snd-reg reg value ...)` (pairs), `(snd-patch ch n)`, `(snd-note ch note)`, `(snd-off ch)`, `(snd-vol ch v)`, `(snd-pan ch :left|:right|:both)` (atoms), `(snd-bend ch n)`, `(snd-drum ch n)`, `(note-of "C#4")`, `(tune {{note beats} ...} [ch] [tempo])`, `(play path [times])`; `(beep)` (`cons`).

**BASIC** (EhyBASIC, branch `reborn-basic`, not merged): `SOUND ch, note [, patch [, vol]]` (one statement: a note with its patch and volume first), `SOUND ch` (off), `SOUND "line"` (a line for `/dev/sndctl`: `"claim 255"`, `"volume 150"`, `"reset"`), `BEEP`, `SLEEP s`.  By design one keyword, as every keyword is a name a program loses; not pan, bend, drums by number, registers or songs.  (The original EhyBASIC had only `BEEP`, a square wave from the VIA.)

**The programs**: `play [-l] song [n]` (a ZSM song: its loop n more times, or till Ctrl-C), `scom` (the old riff, a script); the C sample `tones`.

**The score language** (`sim/tools/hysong.js`, on the PC: MML compiled to ZSM), the richest note-level vocabulary the Hydra has: notes with lengths, dots, ties and triplets, rests, octaves (`o`, `>`, `<`), a default length (`l`), the part of a note held (`q`), legato (`&`), slides (`_`), General MIDI drums (`x`), volume (`v`), speakers (`p`), an instrument (`@name`: a patch, or the four operators' settings), transpose (`k`), detune (`D`), the LFO and the channels' sensitivity to it (`L`, `M`), noise (`N`), raw registers (`y`), repeats (`[...]N`), a tempo.

**The old system** (`os_rom/sound`): the same driver commands (the reborn driver is its port), plus a test song and its stop (`SND_CTL_TEST`, `SND_CTL_STOP`), and a sound clock (timer B: dropped, as the player came to use the system's tick).  The old HyForth's `sndinit`, `ywrite`, `patch`, `note`, `noteoff`, `sndtest`, `sndstop`, `play`: renamed to the `snd-` words in 6.8.

**The Commander X16's BASIC** (R47; the same YM2151 patches and General MIDI drum map as the Hydra's, and the VERA's PSG):
* FM: `FMINIT` (the chip cleared and default patches loaded), `FMINST ch, patch` (0-162), `FMNOTE ch, note` (its own code: `$4A` is A4; 0 is the release; a negative note changes the pitch without a new attack), `FMFREQ ch, hz` (17-4434; 0 the release), `FMDRUM ch, drum` (General MIDI's 25-87), `FMVOL ch, vol` (0-63), `FMPAN ch, pan` (1-3), `FMVIB speed, depth` (the LFO), `FMPLAY ch, string` (MML, played to its end), `FMCHORD ch, string` (MML's notes at once, on channels from ch on), `FMPOKE reg, val`.
* PSG: `PSGINIT`, `PSGNOTE voice, note`, `PSGFREQ voice, hz`, `PSGVOL voice, vol` (0-63), `PSGWAV voice, w` (pulse with its duty, sawtooth, triangle, noise), `PSGPAN voice, pan`, `PSGPLAY voice, string`, `PSGCHORD voice, string`.
* Other BASICs, for their words' shape: GW-BASIC's `SOUND freq, duration`, `PLAY string`, `BEEP`; the C128's `SOUND`, `PLAY`, `TEMPO`, `ENVELOPE`, `VOL`, `FILTER`.

---

### **The matrix**

One row a thing to do; ✓ there, ✗ not, ~ partly (the note says how).

| What | sndctl / snd | C | HyForth | hylang | BASIC | X16 BASIC | Score (PC) |
| :--- | :--: | :--: | :--: | :--: | :--: | :--: | :--: |
| Claim, release channels | ✓ ctl | ✓ | ✓ | ✓ | ~ ctl text | | |
| Reset the chip | ✓ ctl | ✓ | ✓ | ✓ | ~ ctl text | ✓ `FMINIT` (and default patches) | |
| Master volume | ✓ ctl | ✓ | ✓ | ✓ | ~ ctl text | | |
| Patch (instrument) | ✓ snd | ✓ | ✓ | ✓ | ~ with a note | ✓ | ✓ (and voices by their operators) |
| Note on (MIDI) | ✓ snd | ✓ | ✓ | ✓ | ✓ | ~ its own note code | ✓ |
| Note off | ✓ snd | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ (`q`) |
| Channel volume | ✓ snd | ✓ | ✓ | ✓ | ~ with a note | ✓ (0-63) | ✓ |
| Pan | ✓ snd | ✓ | ✓ | ✓ (atoms) | ✗ | ✓ | ✓ |
| Bend (64ths) | ✓ snd | ✓ | ✓ | ✓ | ✗ | | ✓ (`D`, `_`) |
| Drum (General MIDI) | ✓ snd | ✓ | ✓ | ✓ | ~ as patches 128-162 | ✓ | ✓ |
| A frequency in Hz | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ | |
| Pitch without a new attack (legato) | ~ bend only | ~ | ~ | ~ | ✗ | ✓ (a negative note) | ✓ (`&`, `_`) |
| The LFO (vibrato, tremolo) | ~ raw registers | ~ | ~ | ~ | ✗ | ✓ `FMVIB` | ✓ (`L`, `M`) |
| Noise (channel 7) | ~ raw registers | ~ | ~ | ~ | ✗ | | ✓ (`N`) |
| Transpose | ✗ | ✗ | ✗ | ✗ | ✗ | | ✓ (`k`) |
| Raw registers written | ✓ snd | ✓ | ✓ | ✓ | ✗ | ✓ `FMPOKE` | ✓ (`y`) |
| Registers read back (the shadow) | ✓ snd | ✓ | ✗ | ✗ | ✗ | | |
| A note by its name (`C#4`) | ✗ | ✗ | ✓ | ✓ | ✗ | | ✓ |
| A tune: notes and beats, timed | ✗ | ✗ | ✓ | ✓ | ✗ | ✓ (MML) | ✓ |
| An MML string played | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ `FMPLAY` | ✓ (compiled) |
| A chord (notes at once, over channels) | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ `FMCHORD` | ~ (a line a channel) |
| A song (ZSM) played | ~ `play` | ✓ | ~ by `sh` | ✓ | ✗ | | |
| The bell | ✓ `/dev/bell` | ✗ | ✓ | ✓ | ✓ | | |
| PSG voices (the Vera X) | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ (8 words) | |
| PCM (the Vera X) | ✗ | ✗ | ✗ | ✗ | ✗ | | |

So: **C, HyForth and hylang are at parity with the driver** (13 words, the same names and order: the channel first), with two small gaps (HyForth and hylang can't read the registers back; C has no note names, tunes or beep).  **BASIC is the one well behind**: of the 13 it has a note and its release, a patch and a volume only with a note, and the four `sndctl` words only as text (`SOUND "claim 255"`); not pan, bend, drums or the registers.  **Nothing on the Hydra itself plays MML**, the vocabulary the score language and the X16's BASIC share; and nothing has a frequency in Hz, legato, the LFO by name, chords, or the PSG.

---

### **Where they differ today**

* **Claims**: C, HyForth and `/dev/sndctl` take a mask (`snd-claim ( 5 -- )`: channels 0 and 2); hylang takes the channels themselves (`(snd-claim 0 2)`).  hylang's is friendlier, the mask quicker; both are fine for their languages, but the documentation should say so where they're compared.
* **Pan**: numbers 1-3 everywhere but hylang's atoms (`:left`).  Also fine for the language; `/dev/sndctl` text should take the words (`left`, `right`, `both`), as hylang does.
* **Two volumes, near names**: `snd-volume` (the master, 0-200) and `snd-vol` (a channel's, 0-127), in every language.  Easy to swap by mistake.  Keep them (they're in three languages and the docs), but say it plainly in each reference; or, if the user prefers, `snd-master` for the master, with `snd-volume` kept as the old name.
* **The X16's units**: notes in its own code (`$4A`), volumes 0-63, drums 25-87.  The Hydra's are MIDI notes, 0-127 (General MIDI's), the same drums and patches.  Keep the Hydra's; the X16 migration utility maps them (`MIDI = 12 * octave + note + 11`; volume x 2).
* **BASIC's songs and tunes** aren't there; its doc defers songs to "the shell later".

---

### **The target: the largest set, in the Hydra's units**

The union of the rows above, each a driver command (so every language reaches it the same way, and another program's write can't come between its parts) or a library function where it needs no driver:

| Word (HyForth / hylang; C `snd_x`) | What | Where it's done |
| :--- | :--- | :--- |
| `snd-claim`, `snd-release`, `snd-reset`, `snd-volume` | As now | `sndctl` |
| `snd-patch`, `snd-note`, `snd-off`, `snd-vol`, `snd-pan`, `snd-bend`, `snd-drum`, `snd-reg` | As now | `snd` (commands) |
| `snd-regs` | The 256 registers as written | `snd`, read |
| **`snd-freq ch hz`** | A note by frequency (0: off), as `FMFREQ`: its key code and fraction worked out by the driver (a table, no floating point) | New command (`SND_R_FREQ`: two bytes) |
| **`snd-glide ch n`** | Pitch to note n without a new attack (legato; MML's `&`), as `FMNOTE`'s negative notes | New command (`SND_R_GLIDE`) |
| **`snd-lfo rate pmd amd wave`**, **`snd-sens ch pms ams`** | The LFO (the whole chip), and a channel's vibrato and tremolo sensitivity (`FMVIB`, MML's `L` and `M`) | New commands, or a library over registers `$18`, `$19`, `$1B`, `$38`+ch |
| **`snd-noise n`** (0 off) | Channel 7's noise (MML's `N`) | Library over register `$0F` |
| `note-of`, `tune` | As HyForth and hylang have them | Library (C: `snd_note_of`, `snd_tune`) |
| **`snd-mml ch string`** | An MML string played to its end, the score language's MML (the X16's `FMPLAY`'s superset) | One interpreter for all: see below |
| **`snd-chord ch string`** | MML's notes at once, on channels from ch on (`FMCHORD`) | The same interpreter |
| `snd-play path [times]` | A ZSM song, by `play` | `play` (HyForth gets a word for it) |
| `beep` | The bell | `/dev/bell` (C gets `snd_beep`) |
| **PSG** (the Vera X): the same words on channels 8-23, and **`snd-wave ch w [pw]`** | The VERA's 16 voices: a note, off, a volume (0-127, scaled to its 0-63), pan, bend, frequency, glide all as the FM channels'; a waveform (pulse and its width, sawtooth, triangle, noise) instead of a patch | The driver (VIDEO.md step 7: "channels 8-23"), one more command |
| PCM | Later (VIDEO.md step 7) | |

**MML in one place.**  The score language's MML is the largest note-level vocabulary there is; the X16's `FMPLAY`/`PSGPLAY` strings (their Appendix A) are mostly within it: notes with `+`/`-`, lengths and dots, `R`, `L`, `O`, `<`, `>`, `T` (the score's `#tempo`), `V` (0-63: the score's `v` is 0-127), `P` (1-3: the score's `p l|r|c`), and three it lacks, `S` (articulation, 0-7: the spacing between notes, `S0` legato; the score's `q` is the held part, the other way round), `K` (a new attack under `S0`), `I` (an instrument by number: a patch, or a PSG waveform).  Writing MML four times (C, Forth, lisp, BASIC) is the wrong way.  The plan: the score language on the Hydra itself, in the program **`play`**, which already times songs by the tick and owns their channels:
* `play song.mml` compiles a score file as it plays it (as `hysong.js` does on the PC), so scores are first-class on the Hydra;
* `play -m CH "T120 O4 CDE"` plays one MML line on a channel, to its end (as `FMPLAY`), and `play -c CH "CEG"` a chord (as `FMCHORD`);
* every language's `snd-mml` and `snd-chord` run it (`run`/`sh`/`system`), waiting for it or not (`&`), so one interpreter is the only one to test.
The score language gains `I` (an instrument by number) and `S`/`K` (articulation), and an X16 mode (`play -x`: the X16's units for `V` and `P`, and its default state) so X16 strings play as they are.  The X16's player keeps its state (tempo, octave, length) from one string to the next; `play` runs afresh each time, so that state would live in the environment (`$mml`, say) or the caller's library would pass it.

**`/dev/sndctl` as text, for all of it.**  `/dev/sndctl` takes every word above as a line too (`patch 0 29`, `note 0 60`, `off 0`, `pan 0 left`, `bend 0 -32`, `drum 9 38`, `freq 0 440`, `lfo 200 10 10 2`, `wave 8 pulse 32` ...), beside today's binary commands on `/dev/snd`.  Then rc can make sound (`echo note 0 60 >/dev/sndctl`), BASIC's `SOUND "..."` reaches everything, a script needs no library, and the binary path stays the fast one for songs and the language libraries.  This is how Plan 9's devices are driven, and it's the cheapest way to parity.

---

### **How each place reaches it**

* **The driver**: `/dev/sndctl`'s text commands (the whole table); new binary commands `SND_R_FREQ`, `SND_R_GLIDE`, and (with the Vera X) `SND_R_WAVE`, at register numbers the chip doesn't have (free: `$00`, `$0B`-`$0E`, `$13`, `$15`-`$17`, `$1A`, `$1C`-`$1F`); the PSG as channels 8-23 when the emulator's VERA plays its PSG (it counts its writes now).  `SND_CHANNELS` grows to 24 with a card, 8 without (`sndctl` says which).
* **C**: `snd_freq`, `snd_glide`, `snd_lfo`, `snd_sens`, `snd_noise`, `snd_wave`, `snd_note_of`, `snd_tune`, `snd_mml`, `snd_chord`, `snd_beep`.
* **HyForth**: `snd-regs`, `snd-freq`, `snd-glide`, `snd-lfo`, `snd-sens`, `snd-noise`, `snd-wave`, `snd-mml`, `snd-chord`, `snd-play` (in `sound.fl`).
* **hylang**: the same names, and `(snd-regs)` as a buffer.
* **BASIC**: the one place where every word costs a keyword (and a name programs lose, even inside longer ones).  Two ways, the user's to choose:
  1. **Few keywords, text for the rest**: `SOUND` as now, plus `PLAY "mml"` (and `PLAY ch, "mml"`, `PLAY "song.zsm"`), and everything else through `SOUND "pan 0 left"` (`/dev/sndctl` text: no new keywords).  Smallest; parity through the driver.
  2. **A keyword each, the system's names**: `PATCH`, `NOTE`, `NOTEOFF`, `VOLUME`, `PAN`, `BEND`, `DRUM`, `FREQ`, `PLAY`, `CHORD` ... (with EhyBASIC's short aliases).  Familiar, but a dozen names gone from programs (`PAN` already breaks `PANEL`), and `NOTE` clashes with the Hydra's notes (signals).
  3. **The X16's keywords**, with the Hydra's units (`FMINST`, `FMNOTE`, `FMVOL`, `FMPAN`, `FMFREQ`, `FMDRUM`, `FMVIB`, `FMPLAY`, `FMCHORD`, `FMPOKE`, `PSG...`).  X16 programs read almost unchanged; the `FM` and `PSG` prefixes rarely collide with names.  The note and volume units would differ from the X16's, which the migration utility handles, or BASIC takes the X16's units for these keywords alone.

---

### **The order of work**

1. **`/dev/sndctl`'s text commands** for the existing words (claim and the rest are there; `patch`, `note`, `off`, `vol`, `pan`, `bend`, `drum`, `reg`), and reading the shadow from HyForth (`snd-regs`) and hylang.  Then BASIC's `SOUND "..."` and rc reach everything that exists.  Small, and it settles the text forms the rest follow.
2. **The missing small words everywhere**: C's `snd_note_of`, `snd_tune`, `snd_beep`; HyForth's `snd-play`; BASIC's (as the user chooses).
3. **The driver's new commands**: `freq`, `glide`, the LFO and the sensitivities, noise; then each language's words for them.
4. **MML on the Hydra** (`play`'s score compiler), then `snd-mml` and `snd-chord` everywhere, BASIC's `PLAY`.
5. **The PSG** with the Vera X: the emulator's PSG made audible to tests (its notes as the YM2151's key-ons are), channels 8-23 in the driver, `snd-wave`; the ZSM player's PSG writes (now skipped) played.
6. **PCM** later, with `play` taking WAV files (VIDEO.md step 7).

Each step with its tests: the driver's commands by their effect on the emulated chips (registers, key-ons, timing), each language's words by a script in the test suite (the `snd`, `hyforth`, `hydev`, `fdev` tests and BASIC's suite grow), and the MML interpreter against `hysong.js`'s output for the same score (the same register stream, tick for tick).

---

### **Questions**

1. **BASIC's words**: way 1 (few keywords, text for the rest), 2 (a keyword each) or 3 (the X16's `FM`/`PSG` keywords)?  (This is the BASIC session's code: it would make the change, or I would, on the user's word.)
2. **MML in `play`**, one interpreter for every language, rather than a word set in each?
3. **`/dev/sndctl` taking the channel commands as text** (rc can make sound)?
4. **The two volumes' names**: keep `snd-volume` and `snd-vol`, or the master as `snd-master`?
5. **Units for X16 programs**: the Hydra's (MIDI notes, 0-127 volumes) everywhere, the migration utility mapping the X16's, as decided for X16 porting?
