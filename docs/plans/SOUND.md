## **Hydra 16: Sound (the YM2151)**

The plan for the Hydra's sound: a library for the YM2151, a song player, a test song that uses everything the chip can do, and importing music made for other machines.  Phases 1-4 are done; importing is planned.  How to use what exists is in [io.md](../programming/io.md#sound-devsnd), [HyForth](../using/hyforth.md#tasks-and-the-console) and [the C guide](../programming/c.md#sound-sndh).

### **The chip on the Hydra**

* A **YM2151** (OPM: 8 channels, 4 operators each, 8 algorithms, feedback, an LFO with 4 waveforms, noise on channel 7, 2 timers) and a **YM3012** stereo DAC, then op-amps to `AUDIO_L`/`AUDIO_R` and the mixer ([hardware](../hardware.md)).  CT1/CT2 (two output bits, `$1B`) are on a header.
* **Clock:** `SND_CLK`, 3.579545 MHz, the same as the Commander X16's and many arcade boards' (Capcom CPS1, Sega System 16, Konami, Namco), so their key codes give the same pitches: MIDI note 69 (A, 440 Hz) is key code `$4A`.  The X68000 ran its YM2151 at 4 MHz: its music needs retuning (below).
* **I/O:** port 4, `$FF40` (register) and `$FF41` (data, and the status: busy, the timers' flags).  After a data write the chip is busy for 64 of its clocks (about 64 CPU cycles at 3.58 MHz); `YM_WRITE` waits for it.  No wait states on board V1, so the chip can't be used with the CPU at 7.16 MHz.
* **IRQ line 4:** the timers' interrupt: timer B is the sound clock (phase 4).

### **Phase 1: the library** *(done)*

**Where:** BIOS ROM page B (`os_rom/sound/`), in the sound task; page 2 gave up the sound code it had (about 800 bytes back).  Page B has about 750 bytes left.

**The interface is `/dev/snd`**, as Plan 9 would have it: register/value pairs written to a file.
* **The shadow.**  The chip's registers can't be read, so the library keeps each as written (`SND_SHADOW`, 256 bytes of the sound task's RAM), and reads of `/dev/snd` give it back.
* **Commands at the registers the chip doesn't have** (`$02-$07`, `$09`, `$0A`): a channel, a patch, a note, key off, volume, speakers, bend, a drum.  One protocol carries both raw register writes and the library's commands, so anything that can write pairs (HyForth's `ywrite`, a C program, a song's register stream) can use either, in one request.
* **Volume without a special path.**  A carrier's level (TL) gets the channel's and the master volume's attenuation as it's written; the shadow keeps the level as written.  So the volumes work on raw streams too (a song played from someone else's register dump), and changing a volume writes the channel's four levels again.  Which operators are carriers depends on the algorithm, so a new algorithm writes them again too.  The curve is General MIDI's: 40 log10 (v / 127) dB, in the chip's 0.75 dB steps.
* **Pitch:** MIDI notes, and a bend in 64ths of a semitone, which is the chip's own key fraction.
* **Patches:** the Commander X16's General MIDI set (128 instruments and 35 drum and percussion sounds, 26 bytes each, and its General MIDI drum map), 2-clause BSD, with its notice in `sound/patches.s`.
* **Claims, tied to an open file:** a fd claims channels (`SND_CTL_CLAIM`); other fds' writes to them are dropped; the last close gives them back.  A task's end closes its fds, so a program that dies can't hold a channel.  The console's bell (channel 7) stays off a claimed channel.  This is how music and sound effects share the chip, and how two programs can't cut into each other's notes.
* **Not in interrupts:** register writes run in the server, with interrupts on.  Each takes about 64 cycles of the chip's busy time, and a song's tick can have dozens; with interrupts off that would lose serial bytes at 115,200 baud (a byte every 320 cycles, and the 65C51 holds one).
* **Also:** HyForth's `patch`, `note` and `noteoff`; C's `snd.h`; the emulator's `--ym-dump` and `--ym-vgm` (listen to what the ROM played); the `sound-lib` test.

### **Phase 2: the song player** *(done)*

**Built:** `os_rom/sound/player.s` on BIOS ROM page C (627 bytes; it uses only gates, so it has a page of its own and page B keeps its room).  `play song [n] [&]`, a song by its name (`theme` finds `theme.zsm`, after `.hyx` and `.hys`), `run song.zsm` (the shell knows a song by its `zm`, as it knows an executable by `HYX1`), and C's `snd_play` (with `hy_kill` to stop one).  `n` plays the song's loop n more times; none: the song once, to its end; 0: forever.  Tests: `songs` (a song made in the test: its timing, loops, a claim, Ctrl-C, `jukebox`).

**Measured** in the emulator with songs from the X16's tools (zsound's `BGM.ZSM`, 564 notes at 60 Hz; zsmkit's `SONG1.ZSM`, 935 notes at 62 Hz, with PSG parts skipped): every note played, on average 1.5 ms from the song's time, with no drift over a minute (under 2 ms at the end).  A note is late when its tick carries a lot of register writes before its key-on (the song's first tick, setting up every channel, 40-50 ms; a big patch change, 15 ms): the chip takes the writes one at a time.  What made it that close:
* the system tick was 0.35% fast (the timer's latch had 64 cycles taken off for "the handler's time", but a free-running VIA timer counts latch + 2 cycles whatever the handler does), which also made the clock gain 5 minutes a day; fixed (`TIMER_TASK_INT`, `kernel.inc`), and the `sleep` test's window narrowed so it can't come back;
* the song's time in 8.16 fixed point (200 x 65536 / the rate), from a half, so each tick rounds to the nearest system tick;
* the song's start on a tick boundary, two ticks after the player starts (the shell that started it is waiting by then);
* the file read ahead while the player waits (a card's block can take 10 ms), not when a tick's writes run out.

The design, as planned:

**The format: ZSM**, the Commander X16's "Zsound" music format ([spec](https://github.com/X16Community/x16-docs/blob/master/X16%20Reference%20-%20Appendix%20G%20-%20ZSM%20File%20Format.md)), as it is:
* It's a stream of YM2151 register writes and delays, made for this chip at this clock: a 16-byte header (`zm`, a version, a loop point, the channels it uses, a tick rate in Hz, 60 by default), then commands: `$41-$7F` n register/value pairs, `$81-$FF` a delay of n ticks, `$80` the end, `$00-$3F` a VERA PSG write, `$40` an extension (PCM, sync events).
* **Furnace exports it**, and so do the X16's tools, so a composer can write for the Hydra today in [Furnace](https://tildearrow.org/furnace/) (a free multi-system tracker with a YM2151 system), and X16 music plays at the right pitch.
* The Hydra plays the FM part.  PSG writes and PCM are skipped (the Hydra has neither); sync events could become notes to the player's client later.
* A register stream needs no instrument logic in the player, so the player is small, and anything a composer can make the chip do plays back exactly.

**The player** runs in a task of its own, as a **client of `/dev/snd`** (not in the driver): `play song.zsm`, `play song.zsm &`, or C's `snd_play`.  Like `run`: the file on an fd, the player (ROM, page B) in a new task, the shell waiting unless `&`.  So the usual machinery applies: `wait`, `kill`, Ctrl-C (status 130), the exit status, a background song while the shell works.
* It claims the channels the header lists (`SND_CTL_CLAIM`): a second song, or a program's sound effects on other channels, can't collide with it.  Its end (or a kill) closes `/dev/snd`, which keys its channels off.
* It reads the file in 256-byte blocks, collects each tick's register pairs, and sends them in one write (one request a tick), then sleeps to the next tick.
* **Timing:** the system tick is 200 Hz (5 ms), so a 60 Hz song's ticks land 15 or 20 ms apart; with a running target (`TASK_SLEEP_UNTIL` and a fraction), the tempo is exact on average.  Phase 4 made each tick exact.
* **Cost:** a request is about 2,000 cycles, and the server about 150 a pair: a 60 Hz song with 10 writes a tick is about 6% of the CPU.  Songs are streams: 0.5-2 KB a second, so a 3-minute song is 100-300 KB, read from the card as it plays (well within the SD card's rate).
* **Loops:** the header's loop point, a number of times (`play song.zsm 3`), or forever until stopped.
* A text `/dev/snd/ctl` (`play`, `stop`, `volume 90`, a status line) may follow, as Plan 9's audio devices have one.
* The driver keeps the timers' interrupt enables (`$14`) off whatever a song writes: a register dump from an arcade game has its driver's timers in it, and an interrupt nothing clears would stop the machine.

### **Phase 3: a test song that uses the whole chip** *(done)*

**Built:**
* **The score compiler**, `sim/tools/hysong.js` ([emulator.md](../tools/emulator.md#songs-the-score-compiler)): a score is instruments (the ROM's patches by number, or a voice's registers by name: `alg`, `fb`, and each operator's `mul`, `dt1`, `tl`, `ar` ...) and a line of MML for each channel (notes, rests, lengths, octaves, legato, slides, repeats, volume, speakers, detune, the LFO, noise, General MIDI drums, and raw register writes).  It works out the patches and volumes itself, so its ZSM is plain register writes; it can write a VGM for listening, and the ROM's source.
* **The test song**, `os_rom/songs/test.mml`: 31 bars (66 s) in A minor, 852 notes on all 8 channels, 14 KB.  The build compiles it to `songs/test.zsm`, a file on the ROM disk (`/rom/songs/test.zsm`), and `sndtest` (`SND_CTL_TEST`) plays it: the song player (page C) opens it as any song file, in its own task (`ZSM_PLAY_TEST`).  (It was in paged ROM bank 2, read by a mode of the player's own, until the ROM disk.)  In the emulator all 852 notes play, 1.3 ms from the song's time on average.
* **The old riff** (the first `sndtest`, `snd_test.s`) is `programs/songs/scom.mml` (`scom.zsm`: on a card, it runs as the command `scom`), a demo of the score language; its ROM code is gone (page B has about 1,200 bytes free).

**What the song uses** (each checked in its register stream when it was written: all 8 algorithms, feedback 0-7, the four LFO waves, left, right and both, noise, CSM; no key-on on a silent channel):
* **all 8 channels** at once, with **all 8 algorithms**, from 1 carrier (a 4-operator stack) to 4 (organ-like);
* **feedback** (M1's self-modulation, 0-7), **detune** (DT1, and DT2's inharmonic ratios for bells), **multipliers**, **key scaling** (brighter, faster notes higher up);
* **envelopes:** fast and slow attacks, the two decay rates and sustain level, releases;
* **the LFO:** vibrato (PMD, PMS) and tremolo (AMD, AMS, and each operator's AMS-EN), its four waveforms (saw, square, triangle, noise) and its rate;
* **noise** on channel 7 (its frequency: hi-hats, snares, wind);
* **stereo:** channels panned left, right and both, and moving;
* **the timers:** CSM (timer A keying all operators, the chip's speech trick) in the coda: an A major chord pulsed at 110 Hz, then 82 (the emulator doesn't model CSM's sound, so this is for the board, or a VGM player: `--ym-vgm`); the player's tick is timer B (phase 4), so timer A is free for this;
* pitch bends and slides (key fractions), and a drum part on the General MIDI drum patches.

The instruments were written for it (`@lead` algorithm 4, `@bell` 5, `@pad` 6, `@organ` 7, `@bass` 0, `@synbass` 1, `@brass` 2, `@pluck` 3, and noise ones for channel 7), and the drums are the ROM's General MIDI kit.  It hasn't been heard on the board yet: its voices are first drafts, to adjust by ear.

### **Phase 4: the chip's own clock** *(done)*

**Built:** the **sound clock**: the YM2151's timer B, at the song's rate, drives the player (`SND_CTL_CLOCK`; `sound/ymfast.s`, the player's `ZSM_CLOCK_START` and `ZSM_CLOCK_WAIT`).
* **Timer B, not A:** timer A's steps are finer (18 us), but songs use it for CSM (the test song's coda does), and a song from a VGM may program it; timer B's are 286 us, and the rate comes out exact anyway: each period is K or K + 1 units, as a 16-bit fraction carries (Bresenham's way), so the ticks are never more than one unit (0.3 ms) from where they belong and the average is exact.  Range: 14-3,495 Hz.
* **A fast handler**, as the VIA's and the ACIA's (the dispatcher's 650 cycles would be too long beside serial input at 115,200): a stub in the COMMON block (`YM_IRQ_STUB`: 6 bytes found by letting the last IRQ stub and `FAR_INLINE` run on instead of jumping), then `YM_IRQ_FAST` on page 2: timer B's flag reset, the next period, the tick counted, and the player woken with a task switch when its time comes.  About 150-300 cycles, as long as the chip keeps it waiting to be written.
* **The player** asks for the clock after it claims its channels, and sleeps until the clock reaches its time (its `ZSM_AT`, which the interrupt looks at).  If another program has the clock, or the rate is out of its range, it times the song by the system tick as before.  The clock is the fd's that started it, stops at its last close, and survives `SND_CTL_INIT`.  A song's own writes to timer B are dropped while it runs.
* **Measured** (emulator): zsound's `BGM.ZSM` (60 Hz), 564 notes, 0.63 ms from the song's time on average (1.42 by the system tick), 0.2 ms at the end; the test song (200 Hz, the system tick's rate) as before, 1.4 ms.  The worst notes are still the ones behind a tick of many register writes.  The `irqs-off` test passes (the longest stretch is still the task-end cleanup's).
* Page 0 moved the bell's gate after the thunks (for `IRQ_INIT`'s 9 bytes); COMMON has 1 byte left.

### **Importing music from other machines** *(investigation: nothing built yet)*

**The approach:** convert on the PC, to ZSM, with a Node tool (`sim/tools/hymusic.js`, no packages: Node's own `zlib` reads `.vgz`).  The Hydra only ever plays ZSM.  Converting on the PC keeps the ROM small, and the hard parts (retuning, translating other chips, choosing voices) need memory and arithmetic the Hydra hasn't got to spare.  Every route below ends in the same file, testable in the emulator (`--ym-vgm` renders it) and on the board.

| Source | Where it comes from | Route | How faithful |
| :----- | :------------------ | :---- | :----------- |
| **ZSM** (`.zsm`) | Commander X16 music; Furnace's export | Plays as it is | Exact for the FM part; its PSG and PCM parts are lost, or (an option) the PSG voices moved onto spare FM channels as simple square-ish patches |
| **Furnace** (`.fur`), DefleMask (`.dmf`) | Furnace, with a YM2151 system (or the X16's) | Export ZSM from Furnace | Exact.  The best route for new music.  Furnace also opens many other trackers' files, so it's a hub for music in other formats |
| **VGM** (`.vgm`, `.vgz`) with YM2151 writes | [vgmrips.net](https://vgmrips.net): thousands of arcade (Capcom CPS1, Sega System 16/18/X, Konami, Namco System 1/2, Atari) and X68000 soundtracks | `hymusic vgm2zsm` | The FM exactly.  Their samples (drums on the OKI MSM6295/6258, Sega PCM) are lost: the Hydra has no PCM.  Other chips' writes are skipped |
| **MDX** (`.mdx`, `.pdx`) | Sharp X68000 music (MXDRV), a very large library | [mdxtools](https://github.com/vampirefrog/mdxtools)' `mdx2vgm`, then `vgm2zsm` | As VGM from an X68000: retuned from 4 MHz; the PDX drums (ADPCM) lost |
| **VGM for other FM chips** (YM2612/OPN2: Mega Drive; YM2203, YM2608: PC-88/98; YM2413) | vgmrips.net | `vgm2zsm` with a translation | Close for the FM: OPN2 and OPM are both 4-operator chips with the same algorithms, but their registers, detune and frequency encodings differ, and OPN2's SSG-EG and DAC channel have no OPM equivalent.  A second stage of the tool |
| **MIDI** (`.mid`) | Everywhere | `hymusic mid2zsm` | General MIDI: the X16 patch set (as in the ROM), 8 voices allocated from the song's notes (oldest note stolen first; drums on their own channel or two), velocity and channel volume as levels, pan as left/right/both, pitch bend as key fractions, modulation as the LFO's vibrato.  As good as the patches and the 8-voice limit allow: a MIDI file with 20 simultaneous notes loses some |
| **Patches** (`.opm` VOPM, `.dmp` DefleMask, `.fui` Furnace, `.tfi`, `.y12`) | Instrument libraries | `hymusic patch` | Exact for OPM patches; OPN patches translated.  To YMP (26 bytes): into a song, as register writes, or (later) into user patch slots that `SND_R_PATCH` can load |
| Sample-based (MOD, XM, S3M, IT, WAV), SID, NES, SNES, AY | | Not a direct route | The Hydra can't play samples, and other synthesis doesn't map onto FM.  A person can remake one in Furnace |

**The converter's parts:**
* **VGM reading:** the header (`Vgm `, version, the YM2151 clock at `0x30`, the data at `0x34` + its value, or `0x40` before version 1.50, the loop offset at `0x1C`, the GD3 tag's title and author); commands `0x54 aa dd` (YM2151), waits `0x61 nn nn`, `0x62` (1/60 s), `0x63` (1/50 s), `0x7n` (n + 1 samples), `0x66` (the end), data blocks (`0x67`, skipped), and every other chip's command skipped by its length.  Two YM2151s (bit 30 of the clock) can't be played on one: the second's writes are dropped, or its channels merged onto unused ones where there are some.
* **Timing:** VGM counts 44,100ths of a second.  The tool finds the song's own rate (the waits' common step: 60 Hz, 50 Hz or a tempo's) and quantizes to it, choosing the ZSM tick rate so nothing moves by more than a millisecond or two.  Writes in one tick go out together.
* **Retuning** for a clock that isn't 3.579545 MHz (the X68000's 4 MHz is 1.92 semitones sharp at the same key code): each channel's key code and fraction are tracked and rewritten, shifted by the ratio in 64ths of a semitone (123 for 4 MHz).  The LFO rate, the noise frequency and the timers scale too.  Notes past the top of the range fold down an octave.
* **Size:** repeated register values are dropped (the tool keeps its own shadow), and loops found in the VGM become ZSM's loop point.
* **Checking:** the emulator plays the ZSM and writes `--ym-vgm`; the tool compares that stream with the source's (after retiming), and reports the differences.

**Fidelity limits to expect:** no samples (a lot of arcade drums, X68000 percussion, Mega Drive voice clips); 8 channels.

**Copyright:** music ripped from games and computers belongs to its composers and publishers.  The tools convert files the user has; the repository and the ROM carry only music written for the Hydra (the test song) or under a license that allows it.

### **Order of work**

1. ~~Phase 2, the player~~ (done).
2. ~~Phase 3: the score compiler and the test song~~ (done).
3. The importers, in the order they pay off: `vgm2zsm` for YM2151 VGMs (arcade and, with retuning, X68000), then `mid2zsm`, then patch conversion, then the OPN family.
4. ~~Phase 4, the chip's timer as the player's clock~~ (done).
