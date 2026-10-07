## **Video: the Vera X card in slot 0**

A plan for the Hydra-16's supported video card: a card in **slot 0** carrying the **VERA** (the Versatile Embedded Retro Adapter, the Commander X16's video chip: an iCE40UP5K FPGA with 128K of video RAM, VGA out, a 16-voice PSG and PCM audio).  The card is called **Vera X** here.  Its 32 registers fill slot 0's **I/O ports 2 and 3** (`$FF20-$FF3F`).  Its interrupt is slot 0's **IRQ A, line 2**; **IRQ B, line 3**, stays free, as the keyboard and mouse controller is polled over I2C (step 6).  The VERA's source (the module's PCB, gateware v0.9 and its programmer's reference) is in `c:\source\vera-module`.  Steps 1 to 4, 5's graphics words, 6 (the keyboard and mouse) and 7's PSG and PCM are built in the rebuilt system (`reborn/`, phase 8): see [As built](#as-built-october-2026).  The console that step 4 put on the screen is being rebuilt by the text windows' plan ([WINDOWS.md](WINDOWS.md)): [The console and the text windows](#the-console-and-the-text-windows) says what that changes here.  Next, in the user's order (2026-10-07): the rest ([Order of work](#order-of-work)).

### **As built (October 2026)**

**The card** is the user's **VERA X 6.1** from Joe Burks (wavicle): the VERA with the X16 community's gateware (v47 on, X16Community/vera-module), not v0.9.  So it has the version register (DCSEL 63: `DC_VER0` reads "V", then the major, minor and build numbers) and FX (DCSEL 2-6).  The X16's *Programmer's Reference* (chapters 9 and 10) is its documentation.  It comes with a 2x12 header as the X16's (this plan's J1) or a 2x13 one (the OtterX's); **the user's has the 2x13**: J1's pins two on, with I2C's SCL and SDA on pins 1 and 2 (from the OtterX's schematic, `Vega_EV1/OtterX.kicad_sch`, its `VERA_CONN`).  Its SD card's lines go to a header, not a slot.  The carrier card (option A) isn't built yet: it takes a 2x13 socket, the glue below unchanged, and SCL and SDA to the slot's I2C (pins 28 and 30).

| 2x13 pin | Signal | 2x13 pin | Signal |
| :--- | :--- | :--- | :--- |
| 1 | SCL | 2 | SDA |
| 3 | +5 V | 4 | GND |
| 5-12 | D7-D0 (5 D7, 6 D6 ... 12 D0) | 13 | `CS#` |
| 14 | `RES#` | 15 | `WR#` |
| 16 | `IRQ#` | 17 | A4 |
| 18 | `RD#` | 19, 20 | A2, A3 |
| 21, 22 | A0, A1 | 23, 24 | GND |
| 25 | Audio left | 26 | Audio right |

**Built** in `reborn/`, which differs from this plan (written for the old system's ROM) as follows:
* **The emulator's VERA** (step 1): `reborn/sim/lib/vera.js`, the v47.0.2 chip (FX too, since the rest's work: below; its sound, the PSG's and the PCM's, made since with the YM2151's: `sim/lib/audio.js`, `run.js --sound`, `--wav`); `run.js --vera`, `--screen`, `--frame-png`, and `--view` (the screen live in a browser) rather than a web emulator; the vera test.
* **The driver** (step 2) is a module, `vid` (a boot driver, task A), not BIOS page E.  It detects the card itself, as it starts (not POST): the version register, or ADDR0 read back for v0.9, for 0.3 s (the FPGA configuring itself after a reset).  Its font is built in (ISO-8859-15, the X16 ROM's PXLfont), with `/lib/font/cp437` beside it; no boot logo yet.  The frame interrupt goes through the kernel's one IRQ path to vid's irq entry.
* **`/dev/vid`** (step 3): `ctl`, `term`, `vram`, `pal`, `sprites`, `font` and `frame`.  `ctl`'s commands: `mode 80x60`, `mode 80x30`, `mode 40x30`, `cursor blink|on|off`, `border N`, `bitmap 320 D`, `bitmap 640 D`, `bitmap off` (layer 0), `claim`, `claim all`, `release`, `reset`.  `frame` reads as text (the count in decimal, and an LF), as the GPIO's `ca1` does.  Claims as step 5 plans them.
* **The screen console** (step 4): the terminal is vid's (`#v/term`), and the console driver, `cons`, writes the shown window's text there as it sends it to the serial port; consctl's `screen`, `serial` and `both` choose.  The cursor is sprite 0 (an underline at VRAM `$1F800`, blinked by `DC_VIDEO`'s sprite bit).  No keyboard yet (step 6).
* **The PSG** (step 7's first part): sound channels 8-23, the sound driver's (`snd`, `#a`), with the FM channels' commands (a note, off, a level, pan, a bend, a frequency in Hz, a glide; a patch below 4 is a waveform) and one more, `wave` (the waveform and its width).  The VERA stays vid's: snd writes the PSG's registers through `#v/psg` (register/value pairs, a request's in one write), which vid keeps and writes through data port 1 (ADDR0, the cursor's, left alone); while the chip's claimed it only keeps them, and the release writes them (it set the PSG to zeros before).  Volumes go to the chip attenuated by the channel's level and the master volume, as the FM carriers' levels are.  `/dev/psg` (`#a`) takes a song's raw PSG writes, and `play` sends a ZSM's there (its PSG voices claimed, from the header's mask) instead of skipping them.  `sndctl` reads `channels 24` with a card (8 without), and its `claim` and `release` take a second mask, the PSG's.  [SOUND_PARITY.md](SOUND_PARITY.md)'s step 5 has the rest.
* **PCM** (step 7's second part): vid's `/dev/vid/pcm` (the FIFO: a write taken below a quarter full, as much as fits, the rest waiting for the next frame, so the frames feed it rather than AFLOW's interrupt) and `pcmctl` (`rate` in Hz, the VERA's nearest; `bits`, `mono`, `stereo`, `volume`, `reset`, `drain`), one task's at a time.  `play` plays WAV files and a ZSM's PCM extension (its instruments read into RAM if they fit, about 20K).  SOUND_PARITY.md's step 6.
* **The keyboard and mouse** (step 6, below): the emulator's SMC (`sim/lib/smc.js`: its answers as `x16-smc`'s, a read's made as its address comes and unanswered when there's nothing; `run.js --smc`, `--kbd TEXT`, and `--view`'s keys and mouse); the console's `#c/kbin`; the `input` program (`modules/input`, 1.8K), which init starts after the shells; vid's `/dev/vid/mouse`, `mousein` and `mousectl`, and the pointer.  Where it went otherwise than planned:
  * **The idle rate.**  A look that finds nothing costs some 5,000 cycles (the request to gpio, the bus, the scheduler), 10% of the CPU at 67 a second, so `input` looks 10 times a second after 2 s with nothing (1.8% of the CPU, measured), and the first key after a quiet spell waits a tenth of a second at most.  The SMC's buffer (15 key codes) holds that much typing.
  * **The buttons' changes are queued** in vid (8 of them, each `/mouse` fid reading them in turn), as Plan 9's are, so a click between two reads isn't lost; the moves aren't (a read gives the latest).
  * **The cursor blinks by its z now** (sprite 0's byte 6, through data port 1, ADDR1 kept there), not by DC_VIDEO's sprites bit, which blinked every sprite, the pointer among them.  A scroll's row copy and the PSG's writes borrow ADDR1 and put it back.
  * **`input` opens its files by their devices' names** (`#i/42`, `#c/kbin`, `#v/mousein`), so it runs in any namespace.
  * Tests: the mouse test (vid's files and the input program on the SMC's packets: 48 checks) and the kbd test (keys typed at the SMC reaching HyForth, the login shell).
* **The graphics words** (step 5): drawing is vid's own, `/dev/vid/draw` (`modules/vid/draw.inc`: `pen`, `plot`, `line` (Bresenham's), `box`, `bar`, `circle` and `disc` (the midpoint way), `text` (the console's font), `clear`, on the bitmap at any depth), so it's quick, the console stays over it, and every language has the same words: HyForth's `lib video` (`romfs/lib/forth/video.fs`), hylang's `(use "video")`, C's `vera.h` (`sdk/c/lib/vera.c`), and cc65's TGI through a driver of its own, `hydra_tgi` (`sdk/c/lib/tgihydra.s`: cc65's TGI kernel is in `none.lib` already).  Where it went otherwise than planned:
  * **The plan's `vmode`, `cls`, `spimg` and `tile`** became ctl's commands (`bitmap`, `mode`), `clear`, `sprite!` (a sprite's 8 bytes); tiles are VRAM and the layer's registers, for a claimer.  The pen's colour is `pen`, not `color` (HyForth's and hylang's `color` is the terminal's), and a filled box `bar` (TGI's name; Core has `fill`).
  * **The pen is the driver's**, one for every program: srvlib's commands take 4 words, so a line couldn't carry its colour too, and a shell's lines each open `/dev/vid/draw` anew.
  * **`bitmap 640` is 1 or 2 bits a pixel**: at 4 or 8 it was more than the program's VRAM, and drew over the console's map and font.
  * **cx16-320-8 isn't there**: cc65's X16 driver (`cx320p1`) calls the X16's kernal, so the Hydra's is new, over `/dev/vid/draw`.
  * Samples: `sketch` (`vera.h`, the mouse) and `shapes` (TGI).  The draw test (rc, HyForth, hylang, both samples).
* **The rest** (Order of work's 3), so far:
  * **The output modes**: ctl's `output vga`, `output ntsc [mono] [240p]`, `output rgb [240p]` (DC_VIDEO's bits; the card brings out what it has: the VERA X its VGA).  The emulator had NTSC's and RGB's timing already.
  * **FX in the emulator** (`vera.js`, from x16-emulator's `video.c`): ADDR1's line, polygon and affine modes, 4-bit mode and its nibbles, the 16-bit hop, the 32-bit cache (filled by reads, written 4 bytes at a time under a mask, or a byte at a time cycling), transparent writes, the multiplier and its accumulator, 2-bit polygon poking, the fill length.  The vera test checks it (15 checks).
  * **The VERA's SD card** is the storage driver's disk `v` (`/dev/sd/v`, `/sd/v`): a card as 0-f are (the cache, HydraFS, partitions, `mkfs`), its bytes through VERA_SPI_DATA and CTRL (390 kHz while it starts, 12.5 MHz after) in place of the VIA's bit loops.  The storage driver touches those two registers only, which share nothing with vid's ports, so it needs no claim; with no Vera X, the busy bit never clears and the card isn't there.  Reading a 32K file is 6.6M cycles against the VIA card's 11.7M (each with a prompt's round trip); the rest is HydraFS's and the request's.  The emulator's card is `sd.js`'s, byte by byte (`--vera-sd FILE`).  The vsd test.
  * **The PSG in scores**: channels I-X of the score language are the PSG's voices (sound channels 8-23, the letter less A), in `hysong.js` and `play` (`mml.inc`) alike, byte for byte: the same notes, rests and commands as the YM2151's (but `x`, `M`, `L`, `N`), and instruments of their own, `wave W [WIDTH]` and `env A D S R` (ticks, and the PSG's 0.5 dB steps: each segment a straight line in the volume register, written as it changes; the release runs on through rests, a new attack cuts it, and the song's end waits for the last).  A note's frequency word is the sound driver's for its pitch.  `play -m 8` and `-c` take the PSG's channels (`-x`: PSGPLAY's `I` and `V`), so every language's `snd-mml` does.  `play`'s first bank was full: the driver's patches moved to its second, copied into RAM when wanted.  A score may be some 17K now (the PSG's tracks' tables).  `/rom/songs/vera.mml` uses both chips; the psgmml test.
  * **The VERA in the danlang emulator** (`sim/dl/vera.dl`, and the SMC, `smc.dl`): vera.js but for its sound and its picture (the registers and ports, VRAM and the registers it shadows, the scan and its interrupts, sprite collisions, the PCM FIFO's level, FX, the SPI controller and a card on it, the FPGA configuring itself); I2C devices that acknowledge or not, and the ACIA's keyboard mode, for the SMC.  The harness (`bridge.js`) loads danlang's VRAM and registers into a JS VERA (`vera.js`'s `load`), so a check that looks at the screen draws it the same way.  All ten of the Vera X's tests run in danlang now, and pass (the vera test's counts the same as JS's to the byte).  `sim/dl/run.dl --vera --smc`.
  * **FX in vid's drawing**: a line at 8 or 4 bits a pixel, 320 across, its ends on the bitmap, is FX's line helper (a write a pixel: its slope in 512ths, rounded, so a long line ends where it should); `clear` is 32-bit cache writes.  A 300-pixel line from HyForth went from some 246,000 cycles to 82,000 (the rest is HyForth's and the request's).  ADDR1 is lent meanwhile, as for a scroll.
* The programmer's chapter is `reborn/docs/programming/video.md`; the status, `reborn/docs/status.md`'s phase 8.

### **The console and the text windows**

The text windows' plan ([WINDOWS.md](WINDOWS.md); the user's decisions of 2026-10-07; W1 to W3 built on the branch `reborn-text-windows`, not yet merged) rebuilds the console that step 4 put on the screen.  What it changes for the Vera X:
* **cons is the terminal, and vid's `/term` is one of its back ends.**  Each window is a whole VT100 (and VT102), its screen's cells in task F's RAM banks.  The screen and the serial port are back ends: one *follows* a window's output while it's up to date with the window, and is *painted* from the cells otherwise (a window shown again, a catch-up, chrome).  So vid's terminal keeps the subset step 4 gave it: what it can't do (inserting and deleting lines and characters, SU, SD, REP), cons paints instead.  No more is planned for vid's terminal.
* **While the chip's claimed**, `/term`'s writes get `E_BUSY` (the 1K vid kept for the claim's time is gone), and cons paints the window again after the release.
* **A change of the screen under the console** (a `mode`, a `bitmap`, a `reset`, a claim's end): vid refuses the console's next write, once.  cons then reads the screen's size from vid's `ctl` (`mode 80x60`) and resizes its windows.  A window is sized to the smaller of the terminals that show it, each less its chrome, and its program gets `KEY_RESIZE` (or reads `consctl`'s `size`).  Nothing polls.
* **The DEC special graphics** (`ESC ( 0`, line drawing) are the fonts' first 32 glyphs: vid's built-in font's and `/lib/font/cp437`'s (`tools/decfont.js` puts them there).  A console font for the screen must keep them there.  A read of `/term` gives them as ASCII.
* **Double width and height** are shown a space apart on the screen (the VERA can't scale one row); the serial port's terminal does its own.
* **Chrome**: by default the screen shows the bar, the window's header and its footer (a program's window is 80 x 57 of the 80 x 60), and the serial port none.  A program can turn its window's chrome on or off on either terminal.
* **The keys go through one decoder.**  cons decodes the terminal's key sequences (xterm's, and from W5 their modifiers) into a code each.  The keyboard (step 6) sends exactly what a PC terminal sends, into `#c/kbin`, so its keys go through the same decoder.  It sends Ctrl-Tab as `CSI 9;5u` and Ctrl-Shift-Tab as `CSI 9;6u`, which a PC's terminal can't always send, and Scroll Lock as hold (Ctrl-] h).
* **Two seats (W8)**: the screen with its keyboard, and the serial port.  They're mirrored by default (`both`); independent seats are a `consctl` setting, each with its own group shown, focus, size and keys.  `#c/kbin`'s keys are the screen seat's.
* **The mouse in the console (W8)**: a click focuses a window (in tiles), and xterm's mouse reports (`?1000`, `?1006`) go to the programs that ask for them.  The `input` program sends the buttons, the wheel and drags into `#c/kbin` as SGR reports (`CSI < b;x;y M`), in cells.  Until then the mouse is vid's alone (`/dev/vid/mouse`, step 6).

So W1 to W7 need nothing more from the Vera X; W8 needs step 6 (the keyboard, the mouse); and step 6 needs only `#c/kbin` from cons.  The two sessions agreed `#c/kbin` (2026-10-07): it's built on reborn, and carried into the two-bank cons when `reborn-text-windows` next merges reborn.

### **Contents**
1. [The console and the text windows](#the-console-and-the-text-windows)
2. [Why the VERA](#why-the-vera)
3. [The card](#the-card)
4. [The registers on the Hydra](#the-registers-on-the-hydra)
5. [Sharing one chip between 16 tasks](#sharing-one-chip-between-16-tasks)
6. [The software, in steps](#the-software-in-steps)
7. [The emulator](#the-emulator)
8. [Risks and open questions](#risks-and-open-questions)
9. [Order of work](#order-of-work)

---

### **Why the VERA**

* **It fits slot 0 exactly.**  The VERA has 32 registers, `A0-A4`.  Slot 0's two port selects are ports 2 and 3, `$FF20-$FF2F` and `$FF30-$FF3F`: together 32 bytes in a row, with A4 picking the port.  So the card's chip select is the two selects ANDed, and the VERA's `A0-A4` come straight from the bus.  The hardware reference already marks ports 2 and 3 "intended for video".
* **It's made for an 8-bit bus** (the X16's 65C02 at 8 MHz), with 5 V level shifting on the module and its own 25 MHz clock: the Hydra's 3.58 MHz bus is slow for it.
* **It's a lot of fun for a 65C02:** 640x480 VGA, two layers (text, tiles or bitmaps, 1-8 bits per pixel), hardware scrolling, 128 sprites, a 256-colour palette from 4096, line and frame interrupts, and sprite collisions.  Also 16 PSG voices and a 4K PCM FIFO.
* **It brings software with it.**  The Hydra's song format, ZSM, is the X16's.  An X16 ZSM's PSG writes are skipped today (`sound/player.s`); with the VERA they play, so X16 music plays in full.  cc65 has VERA support for the X16: headers, `vpeek`/`vpoke`, and a TGI graphics driver (`cx16-320-8`).  These are mostly a change of base address, from `$9F20` to `$FF20`.  The X16's documentation, demos and tools teach the chip.
* **It's open:** the gateware, the module's PCB and the reference are all in the repository.

---

### **The card**

#### **The module's connector**

The VERA module (rev 4, `pcb/rev4`) plugs in through a 2x12 header, J1:

| J1 pin | Signal | J1 pin | Signal |
| :----- | :----- | :----- | :----- |
| 1 | +5 V | 2 | GND |
| 3-10 | D7-D0 | 11 | `CS#` |
| 12 | `RES#` (to the FPGA's `CRESET_B`: it reloads its gateware) | 13 | `WR#` |
| 14 | `IRQ#` | 15 | A4 |
| 16 | `RD#` | 17, 18 | A2, A3 |
| 19, 20 | A0, A1 | 21, 22 | GND |
| 23 | Audio left (line level) | 24 | Audio right |

On the module, the data bus goes through a 74LVC4245A (enabled by `CS#`, direction from `RD#`).  The control lines go through a 74CBTD3861 bus switch.  The gateware's bus interface (`fpga/source/top.v`) works like this:
* A read is `CS#` and `RD#` low with `WR#` high.  The data comes from a register mux, and `DATA0`/`DATA1` give the byte fetched ahead of time.
* A write is `CS#` and `WR#` low.  **The data is latched as the write strobe ends** (`negedge bus_write`).
* `IRQ#` is driven both ways (`(ISR & IEN) == 0`), not open-collector.

#### **Option A: a carrier card for the stock module** (first)

A small slot card with a 2x12 socket for an unmodified VERA module, which can be bought as an X16 part or built from `pcb/rev4`, plus a little glue logic:

| VERA | From the slot | How |
| :--- | :------------ | :-- |
| `CS#` | `nIOA_S` (pin 51), `nIOB_S` (pin 49) | AND (74ACT08): low for either port.  Both are already qualified by `PHI2` on the board |
| `RD#` | `PHI2` (pin 39), R/`W` (pin 21) | `NAND(PHI2, R/W)` (74ACT00) |
| `WR#` | `PHI2`, R/`W` | `NAND(PHI2, NOT R/W)` |
| A0-A4 | A0-A4 (pins 62, 60, 58, 56, 54) | Straight through |
| D0-D7 | D0-D7 (pins 18-4) | Straight through (the module buffers them) |
| `IRQ#` | `IRQA` (pin 55): **line 2** | Through an open-collector buffer (74LS07 or 74HCT07) or a Schottky diode: the board pulls the line up, and slot IRQ outputs must be open-collector |
| `RES#` | `RESB` (pin 3) | Straight through.  A reset reloads the FPGA from its flash, which takes a while: the ROM waits for it (below) |
| Audio L/R | `SND_CL0`, `SND_CR0` (pins 26, 24) | Through coupling capacitors and a divider: the WM8524 DAC gives up to 2 V rms, so match it to the mixer (IC9) |
| +5 V, GND | Pins 5, 57 / 1, 19, 61 | The module draws about 200 mA |

**Why `RD#` and `WR#` come from `PHI2`, not just R/W:** the VERA latches write data when the strobe ends.  If `WR#` were only R/W, the write would end when `CS#` goes high.  `CS#` comes through the board's decoding (U14, then the 74LS154 U19), so it rises some tens of nanoseconds after `PHI2` falls, by which time the CPU may have stopped driving the data (the W65C02S holds it about 10 ns).  A fast NAND on `PHI2` ends the strobe within a few nanoseconds of `PHI2` falling, while the data is still there.  This is how the X16 does it.  Check it on the bus breakout card with a logic analyzer before trusting it; the fallback is a 74ACT574 latching the data on `PHI2`'s falling edge.

**The input controller.**  A Hydra with a screen wants a keyboard, and the board has none (its console is the serial port).  The card adds the X16's own: its **SMC**, an ATtiny861 with the X16 community's firmware (`x16-smc`, unchanged), for a PS/2 keyboard and a PS/2 mouse:
* It talks over **I2C**, which every slot already has (pins 28 and 30, bit-banged by the VIA, `PA0`/`PA1`), at address `$42`.  It needs no I/O port, and ports 2 and 3 are the VERA's.
* **It's polled.**  IRQ B (line 3) stays free: the board's IRQ lines are levels and can't be masked, so a line held low till the controller is read over I2C (milliseconds, in gpio's task) would bring the CPU straight back into the interrupt each time it left it.  The X16 polls its SMC too (60 times a second).
* Its other pins (the X16's power supply, reset and NMI buttons, the activity LED) are left unconnected.
* PS/2 can't be read directly from the VIA instead: its bits come every 60-100 µs, and the ROM sometimes keeps interrupts off for longer than that (the emulator reports runs of over 1,000 cycles, about 300 µs).

**The VERA's SD card slot** is on its own SPI controller, at 12.5 MHz with auto-transfer.  That's much faster than the Hydra's VIA-driven SPI.  It carries a second HydraFS card (an SPI back end for the storage driver's cards: disk `v`, As built), which makes it a fast card for programs and assets.

#### **Option B: Vera X on one board** (later)

The same circuit laid out on a full Hydra slot card with the glue logic and the input controller: one board, no module.  That circuit is the iCE40UP5K (with its 128K of video RAM inside), its flash, the audio DAC, the VGA resistor DACs and the level shifters.  `pcb/rev4` is the reference.  Do this only after option A has proven the timing.

#### **The gateware**

Start with the module's gateware as it is (v0.9 in `c:\source\vera-module`; the Commander X16 community has later releases, and the Vera X built runs one of them: v47 on).  Running it unchanged is itself a feature, because it keeps X16 code and documentation valid.  Possible changes later, since the source is here:
* An ID/version register to detect the card by.  The X16 community's gateware has one (DCSEL 63); for v0.9, the driver detects the card by writing and reading back `ADDR0` instead.
* An optional second interrupt pin on option B, for raster (line) interrupts on their own.

The gateware already fills most of the UP5K, so check the utilisation report before adding anything bigger, such as a PS/2 controller.

---

### **The registers on the Hydra**

The VERA's register `n` is at `$FF20 + n`:

| Hydra | VERA | Port | Hydra | VERA | Port |
| :---- | :--- | :--- | :---- | :--- | :--- |
| `$FF20` | `ADDRx_L` | 2 | `$FF30` | `L0_HSCROLL_L` | 3 |
| `$FF21` | `ADDRx_M` | 2 | `$FF31` | `L0_HSCROLL_H` | 3 |
| `$FF22` | `ADDRx_H` (increment, DECR, bit 16) | 2 | `$FF32` | `L0_VSCROLL_L` | 3 |
| `$FF23` | `DATA0` | 2 | `$FF33` | `L0_VSCROLL_H` | 3 |
| `$FF24` | `DATA1` | 2 | `$FF34` | `L1_CONFIG` | 3 |
| `$FF25` | `CTRL` (reset, DCSEL, ADDRSEL) | 2 | `$FF35` | `L1_MAPBASE` | 3 |
| `$FF26` | `IEN` | 2 | `$FF36` | `L1_TILEBASE` | 3 |
| `$FF27` | `ISR` | 2 | `$FF37`-`$FF3A` | `L1_HSCROLL_L` ... `L1_VSCROLL_H` | 3 |
| `$FF28` | `IRQLINE_L` / `SCANLINE_L` | 2 | `$FF3B` | `AUDIO_CTRL` | 3 |
| `$FF29`-`$FF2C` | `DC_VIDEO`, `DC_HSCALE`, `DC_VSCALE`, `DC_BORDER` (DCSEL 0); `DC_HSTART` ... `DC_VSTOP` (DCSEL 1) | 2 | `$FF3C` | `AUDIO_RATE` | 3 |
| `$FF2D` | `L0_CONFIG` | 2 | `$FF3D` | `AUDIO_DATA` | 3 |
| `$FF2E` | `L0_MAPBASE` | 2 | `$FF3E` | `SPI_DATA` | 3 |
| `$FF2F` | `L0_TILEBASE` | 2 | `$FF3F` | `SPI_CTRL` | 3 |

In `include/hw.inc`: `VERA_BASE = $FF20` and the register names (`VERA_ADDR_L` ...), as `VIA_*` and `ACIA_*` are now.  `IRQ_NUMBER(2)` (`5`) is its vector index, and `IRQ_NUMBER(3)` (`4`) the input controller's.

**Speed at 3.58 MHz:** an unrolled copy loop (`lda (zp),y` / `sta VERA_DATA0` / `iny`) moves about 300K a second into VRAM.  A full 80x60 text screen (9,600 bytes) takes about 30 ms, two frames.  A 320x240 bitmap at 8 bits per pixel (76,800 bytes) takes about a quarter of a second.  Scrolling is free: a write to `VSCROLL`.

---

### **Sharing one chip between 16 tasks**

The VERA has one set of address pointers (`ADDR0`, `ADDR1`, chosen by `CTRL`'s ADDRSEL) and one DCSEL.  A task that's switched out between setting an address and writing `DATA0` loses it to whichever task touches the chip next.  So:

* **A driver owns it.**  The `vid` driver (a resident driver task, like the sound task) sets the chip up, runs the screen console, and serves `/dev/vid`.
* **Programs claim it for direct access.**  A program that wants speed opens `/dev/vid` and claims the chip (an `IO_CTL`, as `/dev/snd` claims channels).  Until it closes it, or ends (a task's end releases its claims, as with semaphores and sound channels), it may write the registers directly; the driver leaves them alone.  Console output meanwhile still goes to the serial port; the screen console picks up again on release.  Only one claim at a time; others get `ERR_IO_BUSY`, or wait.
* **The interrupt handler keeps the program's state.**  The line 2 handler always reads and clears `ISR`.  Anything that also touches VRAM (moving sprites at the frame's start, say) first saves `CTRL` and `ADDR1` and puts them back afterwards.  The registers read back, so that's about 40 cycles.  The convention is that interrupt code uses data port 1 and programs use port 0, as on the X16.
* **VRAM is shared by agreement.**  The console keeps its map and font at the top of VRAM, and programs get the rest:

| VRAM | What |
| :--- | :--- |
| `$00000-$1AFFF` (108K) | The program's: bitmaps (320x240x8 bits is 75K), tiles, sprite images |
| `$1B000-$1EFFF` (16K) | The console's text map: 128x64 entries of 2 bytes (80x60 shown; the extra rows make scrolling a `VSCROLL` write) |
| `$1F000-$1F7FF` (2K) | The console's font: 256 characters, 8x8, 1 bit per pixel |
| `$1F800-$1F9BF` | Free |
| `$1F9C0-$1FFFF` | The PSG, palette and sprite attributes (the chip's) |

A program that wants all 128K says so when it claims (`VID_CLAIM_ALL`).  The driver then reloads the font and clears the map on release.

---

### **The software, in steps**

Each step is usable on its own and has regression tests in the emulator.

**1. The emulator's VERA** (first: everything else is developed against it).  See [below](#the-emulator).

**2. Detection and the driver** (BIOS ROM **page E**, which is empty).
* **POST** looks for the card.  After a reset the FPGA reloads its gateware for a while: wait up to about half a second for `ADDR0_L` to read back what was written.  Then the boot report says `VERA` (or nothing, with no card).
* **`vid`, a resident driver task**, registers a handler on IRQ line 2 and sets up the chip:
  * it clears the register area `$1F9C0-$1FFFF`, as the reference advises, since VRAM powers up random;
  * it loads the font (a file: `/rom/lib/font8x8`, so a card's `/lib/font8x8` can replace it);
  * it sets layer 1 to 80x60 text, 16 colours;
  * it shows a boot logo (also a `/rom` file).
* **The frame interrupt** counts frames and wakes the tasks waiting for one.  A 60 Hz handler through the normal dispatcher (`TASK_CALL_IRQ` into the driver's task) is cheap enough.

**3. `/dev/vid`**, Plan 9 style.  Its files:

| File | Read | Write |
| :--- | :--- | :--- |
| `/dev/vid/ctl` | The mode, as text (`80x60 text`, `320x240x8 bitmap` ...) | Commands: `mode text`, `mode bitmap 320 8`, `layer 0 on`, `border 6`, `claim`, `release` ... |
| `/dev/vid/vram` | VRAM: the offset is the address (`IO_SEEK`) | VRAM, through `DATA0` with auto-increment, 256 bytes a request |
| `/dev/vid/pal` | The palette: 2 bytes an entry | The palette |
| `/dev/vid/sprites` | The sprite attributes: 8 bytes a sprite | The sprite attributes |
| `/dev/vid/frame` | Waits for the next frame, then gives its number (4 bytes) | |

So `cp /sd/0/pics/logo.pal /dev/vid/pal`, or a C program's `fopen("/dev/vid/vram")`, `fseek` and `fwrite`, load images with no special code.  For speed, programs claim the chip and write it directly (step 5).

**4. The screen console.**  A terminal emulator on the text layer, understanding the ANSI sequences the console already uses: conio's (`lib/conio`), the editor's, HyForth's `Acls`/`Ascr`/`Acol`.  So every program that works on a serial terminal works on the screen, colours included.  It has:
* a cursor (blinked by the frame interrupt);
* scrolling by `VSCROLL`;
* `/dev/cons` output mirrored to the screen and the serial port, or sent to one of them (`/dev/cons/ctl`: `screen`, `serial`, `both`);
* keys from the input controller (step 6) and the serial port both reaching `/dev/cons`.

With a keyboard, the Hydra is a standalone computer: switch on, get a prompt on the monitor.

**5. Programming it.**
* **HyForth, a `video` library:**
  * the chip: `vpoke` and `vpeek` (a 17-bit address as an address and a bank bit), `pal`, `vmode`, `cls`;
  * drawing: `plot`, `line`, `box`, `circle`;
  * sprites and tiles: `sprite`, `spimg`, `tile`;
  * timing: `vsync` (waits for a frame);
  * **turtle graphics**: `fd`, `bk`, `rt`, `lt`, `pu`, `pd`, `home`, the classic way into graphics for a new programmer.
* **C, `vera.h`:**
  * the chip: the register block as a struct at `$FF20` (cc65's `cx16.h` `VERA` layout), `vpoke`/`vpeek` (from cc65's cx16 library);
  * sharing it: `vera_claim()`/`vera_release()`, `vera_wait_frame()`;
  * data: `vera_load(path, addr)` to load a file into VRAM;
  * sprite helpers;
  * a **TGI driver** made from cc65's `cx16-320-8`, so cc65's portable graphics programs run.
* **Assembly:** `hw.inc`'s names, and a page in the Programmer's Guide (`docs/programming/video.md`): the registers, the sharing rules, the VRAM map, and the interrupt conventions.

**6. Keyboard and mouse** (the input controller, polled over I2C):
* **The controller is the X16's SMC** (option A, above), or anything that answers its registers (a later one could take USB keyboards).  Its protocol, as `x16-smc` has it:
  * a read that names no register first gets the *default request*'s answer (`$40` sets it): `$41`, a key code; `$43`, a key code (0: none) and a mouse packet (0: none).  With nothing to give, it doesn't acknowledge its address, so a look that finds nothing costs an address byte;
  * key codes are the IBM PC/AT's key numbers (1-127: a key's place, not its character), with bit 7 set for a release;
  * mouse packets are the PS/2 mouse's: 3 bytes, or 4 with a wheel (`$20` asks for a mouse's mode, `$22` reads the one it got);
  * `$30`-`$32` are its version; `$1A` sends the keyboard a command (`$ED`: its LEDs).
* **`input`, a program, is its driver** (a user-level driver, as 9front's `nusb/kb` is): srvlib's servers run only when a request comes, and this needs to poll.  init starts it as the shells start, in a note group of its own (so a Ctrl-C at the keyboard can't reach it); with no controller it ends at once.  Every 3 ticks (67 times a second) it reads the controller through gpio's `/dev/i2c/42` till there's nothing more; after 2 s with nothing, 10 times a second (As built, above).
* **The keys go to the console.**  `input` turns the key numbers into what a PC terminal (xterm) sends, from the modifiers (Shift, Ctrl, Alt as an ESC first, AltGr, Caps Lock, Num Lock) and a keymap (the US layout built in), and writes them to cons's `#c/kbin`.  cons takes them as it takes the serial port's (Ctrl-C and Ctrl-\ as notes, Ctrl-] and its key as the console's, the rest to the window shown), from a ring of their own: [The console and the text windows](#the-console-and-the-text-windows).  The keyboard's own repeat is kept (PS/2 keyboards repeat by themselves).  Caps Lock's, Num Lock's and Scroll Lock's LEDs follow their state.
* **The mouse is vid's** (as Plan 9's is the screen's).  `input` writes its moves and buttons to `/dev/vid/mousein`.  `/dev/vid/mouse` reads as Plan 9's (`m`, then x, y, the buttons and the time in milliseconds, each 11 digits and a space), a read waiting for a change (the buttons' changes queued); a write of `m x y` moves the pointer.  x and y are the screen's pixels (640 x 480; 320 x 240 in `mode 40x30`).  The pointer is a sprite (an arrow, its image in VRAM's free `$1F820`), shown once the mouse moves, and left to a claimer while the chip's claimed (it reads `/dev/vid/mouse` and draws its own).  `/dev/vid/mousectl`: `pointer on`, `pointer off`, `swap` (the buttons, left-handed).
* **The pads** (SNES): the SMC has none (the X16's are on its VIA).  Later, on a controller of our own or the GPIO header.
* **In the emulator**: the SMC on the I2C bus (`sim/lib/smc.js`, its answers as `x16-smc`'s), keys typed at it in tests, and the browser view's keyboard and mouse (`--view`).
* **On the bench**: the SMC on the protoboard beside the glue, on the breakout card's I2C header (J10): [vera-wiring.md](../../vera-wiring.md).

**7. Sound.**
* **The PSG**: 16 more voices for `/dev/snd` (channels 8-23: frequency, waveform, pulse width, volume, pan).  The ZSM player plays a song's PSG writes instead of skipping them (`sound/player.s`).  HyForth's `note` and C's `snd.h` reach them as they reach the FM channels.
* **PCM**: the `AFLOW` interrupt (the FIFO below a quarter full) refills the 4K FIFO from a buffer, so `play` takes WAV files (8 or 16 bits, mono or stereo, up to 48 kHz) and ZSM's PCM extension.
* **Both chips at once**: the YM2151 on the board and the VERA's PSG and PCM, mixed on the board through slot 0's audio pair.

**8. Fun things to make with it** (samples in `programs/`, and in `/rom/bin` once they're small and good):
* a sprite demo (bouncing balls, using collision interrupts);
* a smooth tile scroller (a map bigger than the screen);
* Snake, Breakout and Tetris in C, with the PSG for effects;
* a paint program with the mouse;
* a Mandelbrot in 256 colours;
* a ZSM jukebox with the song's channels drawn as bars, from the frame interrupt;
* and ports of X16 programs, which mostly need the new base address and the Hydra's file calls.

---

### **The emulator**

`hydrasim.js` gets a VERA on slot 0 (`--card 0:vera`), modelled from the reference.  The X16's own emulator (x16-emulator, `video.c`) is a good guide to the details.  It covers:
* the registers;
* the 128K of VRAM and the address increments;
* the palette;
* the layers: 1 bpp text in both colour modes, 2/4/8 bpp tiles, and bitmaps;
* the sprites;
* the frame (60 Hz: about 59,700 cycles at 3.58 MHz) and line interrupts, `ISR` clearing, and sprite collisions;
* the PCM FIFO's level and `AFLOW`.

**Seeing it:**
* **In tests:** `--screen` prints the text layer as text in the report, so regression tests check the screen just as they check serial output, with no image comparison.  `--frame-png FILE` saves the last frame as a PNG (Node's zlib does the compression) for tests that need pixels and for documentation.
* **Live:** the emulator's core gets separated from Node's terminal and file system (see the [code review](CODE_REVIEW.md#the-emulator)), so the same core runs in a web page: a canvas for the VERA, a terminal (xterm.js) for the serial port, and keys for the input controller.  That page is also how most people will first try the Hydra.

**Tests:** detection (with and without the card); the console's text, colours and scrolling (`--screen`); `/dev/vid`'s files; a claim and its release (the console coming back); the frame interrupt waking `/dev/vid/frame`; the PSG voices (writes counted, as the YM2151's are); a ZSM's PSG part played; the emulator's own interrupt timing.

---

### **Risks and open questions**

* **Write timing** (above): confirm `WR#` from `PHI2` with a logic analyzer on the breakout card before building more than one.
* **Reset time:** the FPGA reloading after `RESB` must be waited for (POST), and a VERA `CTRL` reset (bit 7) does the same.
* **The CPU at 7.16 MHz:** the VERA is fine (the X16 runs at 8 MHz); the YM2151 still isn't (no wait states on V1).
* **Audio level** into the board's mixer: measure, and size the divider.
* **ROM space:** the driver and terminal fit BIOS page E (8K).  Fonts, the logo and help text go in `/rom`.  Page 0 and COMMON are full, so the driver uses gates.  A fast interrupt path (for raster effects at line rate) would need a stub in COMMON (1 byte free).  Merging `PEEK_D_XAM` and `FP_PEEK_PAGE` into one routine would make room ([code review](CODE_REVIEW.md#rom-space)).
* **The namespace:** `/dev/vid` lives under `/dev` (no mount), so it costs no namespace entries.
* **Which gateware:** v0.9 from this repository first.  Later X16 releases add features but must still fit the module's flash and the Hydra's expectations.

---

### **Order of work**

**Done** (phase 8, `reborn/docs/status.md`): the emulator's VERA (step 1); detection, `vid`, the font and the screen console (2 and 4); `/dev/vid` and claims (3); the PSG and PCM (7); and, for the bench, the card wired through a bus breakout card ([vera-wiring.md](../../vera-wiring.md)).

**From here**, in the user's order (2026-10-07):
1. **The keyboard and mouse** (step 6): done (As built, above), and the SMC in the wiring guide.  The mouse's words in each language come with the graphics words.
2. **The graphics words** (step 5): done (As built, above), the mouse's words with them.
3. **The rest of the Vera X**: FX in the emulator and in vid's lines and clear, the output modes (VGA, composite, RGB, the 240p line doubling, in `ctl`), the VERA's SD card (disk `v`), the PSG in scores (`play`'s MML) and the VERA in the danlang emulator are done (As built, above); then demos (step 8).
4. **With the text windows**: their W8 (the seats, the keyboard and the mouse in the console) once 1 is in.
5. **The hardware**: the carrier card (option A), its timing checked on the bus; then option B, the one-board Vera X.
