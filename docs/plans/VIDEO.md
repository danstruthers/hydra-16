## **Video: the Vera X card in slot 0**

A plan for the Hydra-16's supported video card: a card in **slot 0** carrying the **VERA** (the Versatile Embedded Retro Adapter, the Commander X16's video chip: an iCE40UP5K FPGA with 128K of video RAM, VGA out, a 16-voice PSG and PCM audio).  The card is called **Vera X** here.  Its 32 registers fill slot 0's **I/O ports 2 and 3** (`$FF20-$FF3F`).  Its interrupt is slot 0's **IRQ A, line 2**, and **IRQ B, line 3**, is for its keyboard and mouse controller.  The VERA's source (the module's PCB, gateware v0.9 and its programmer's reference) is in `c:\source\vera-module`.  Steps 1 to 4, and step 7's PSG and PCM, are built in the rebuilt system (`reborn/`, phase 8): see [As built](#as-built-october-2026).

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
* **The emulator's VERA** (step 1): `reborn/sim/lib/vera.js`, the v47.0.2 chip (FX's registers kept, its effects not modelled; its sound, the PSG's and the PCM's, made since with the YM2151's: `sim/lib/audio.js`, `run.js --sound`, `--wav`); `run.js --vera`, `--screen`, `--frame-png`, and `--view` (the screen live in a browser) rather than a web emulator; the vera test.
* **The driver** (step 2) is a module, `vid` (a boot driver, task A), not BIOS page E.  It detects the card itself, as it starts (not POST): the version register, or ADDR0 read back for v0.9, for 0.3 s (the FPGA configuring itself after a reset).  Its font is built in (ISO-8859-15, the X16 ROM's PXLfont), with `/lib/font/cp437` beside it; no boot logo yet.  The frame interrupt goes through the kernel's one IRQ path to vid's irq entry.
* **`/dev/vid`** (step 3): `ctl`, `term`, `vram`, `pal`, `sprites`, `font` and `frame`.  `ctl`'s commands: `mode 80x60`, `mode 80x30`, `mode 40x30`, `cursor blink|on|off`, `border N`, `bitmap 320 D`, `bitmap 640 D`, `bitmap off` (layer 0), `claim`, `claim all`, `release`, `reset`.  `frame` reads as text (the count in decimal, and an LF), as the GPIO's `ca1` does.  Claims as step 5 plans them.
* **The screen console** (step 4): the terminal is vid's (`#v/term`), and the console driver, `cons`, writes the shown window's text there as it sends it to the serial port; consctl's `screen`, `serial` and `both` choose.  The cursor is sprite 0 (an underline at VRAM `$1F800`, blinked by `DC_VIDEO`'s sprite bit).  No keyboard yet (step 6).
* **The PSG** (step 7's first part): sound channels 8-23, the sound driver's (`snd`, `#a`), with the FM channels' commands (a note, off, a level, pan, a bend, a frequency in Hz, a glide; a patch below 4 is a waveform) and one more, `wave` (the waveform and its width).  The VERA stays vid's: snd writes the PSG's registers through `#v/psg` (register/value pairs, a request's in one write), which vid keeps and writes through data port 1 (ADDR0, the cursor's, left alone); while the chip's claimed it only keeps them, and the release writes them (it set the PSG to zeros before).  Volumes go to the chip attenuated by the channel's level and the master volume, as the FM carriers' levels are.  `/dev/psg` (`#a`) takes a song's raw PSG writes, and `play` sends a ZSM's there (its PSG voices claimed, from the header's mask) instead of skipping them.  `sndctl` reads `channels 24` with a card (8 without), and its `claim` and `release` take a second mask, the PSG's.  [SOUND_PARITY.md](SOUND_PARITY.md)'s step 5 has the rest.
* **PCM** (step 7's second part): vid's `/dev/vid/pcm` (the FIFO: a write taken below a quarter full, as much as fits, the rest waiting for the next frame, so the frames feed it rather than AFLOW's interrupt) and `pcmctl` (`rate` in Hz, the VERA's nearest; `bits`, `mono`, `stereo`, `volume`, `reset`, `drain`), one task's at a time.  `play` plays WAV files and a ZSM's PCM extension (its instruments read into RAM if they fit, about 20K).  SOUND_PARITY.md's step 6.
* The programmer's chapter is `reborn/docs/programming/video.md`; the status, `reborn/docs/status.md`'s phase 8.

### **Contents**
1. [Why the VERA](#why-the-vera)
2. [The card](#the-card)
3. [The registers on the Hydra](#the-registers-on-the-hydra)
4. [Sharing one chip between 16 tasks](#sharing-one-chip-between-16-tasks)
5. [The software, in steps](#the-software-in-steps)
6. [The emulator](#the-emulator)
7. [Risks and open questions](#risks-and-open-questions)
8. [Order of work](#order-of-work)

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

**IRQ B (line 3): the input controller.**  A Hydra with a screen wants a keyboard, and the board has none (its console is the serial port).  The card adds a small microcontroller (an RP2040, or an AVR like the X16's SMC) for a PS/2 keyboard and mouse, and optionally two SNES pads:
* It talks over **I2C**, which every slot already has (pins 28 and 30, bit-banged by the VIA, `PA0`/`PA1`).  It needs no I/O port, and ports 2 and 3 are the VERA's.
* It pulls **`IRQB` (line 3)** while it has keys, mouse moves or pad changes waiting, so nothing polls.  Line 3 is just below the VERA in priority, and above the YM2151 (4).
* A keystroke is a few bytes at I2C speed, which is plenty fast for typing.  Mouse packets come at most 100 times a second.
* PS/2 can't be read directly from the VIA instead: its bits come every 60-100 µs, and the ROM sometimes keeps interrupts off for longer than that (the emulator reports runs of over 1,000 cycles, about 300 µs).

**The VERA's SD card slot** is on its own SPI controller, at 12.5 MHz with auto-transfer.  That's much faster than the Hydra's VIA-driven SPI.  It could later carry a second HydraFS card (an SPI back end for `drivers/sd.s`), which makes it a fast card for programs and assets.

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

**6. Keyboard, mouse and pads** (the input controller on IRQ line 3):
* **The firmware**, for an RP2040 (or an AVR, after the X16's SMC firmware), turns PS/2 scan codes into the console's key codes, including the cursor and function keys conio decodes (`CH_CURS_UP` ...).
* **An `input` driver** on line 3 reads the controller over I2C (the first I2C driver: `/dev/i2c` is useful on its own).  It feeds keys to `/dev/cons`, and offers `/dev/mouse` (Plan 9's format: `m x y buttons`) and `/dev/pads`.
* **The mouse** can drive a sprite as its pointer.

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

1. The emulator's VERA, `--screen`, and its tests.
2. The carrier card (option A) with the glue logic: check the timing on the breakout card.
3. Detection, the `vid` driver, the font and the screen console (output only).  Now the Hydra shows its prompt on a monitor.
4. `/dev/vid`, claims, HyForth's `video` library, C's `vera.h`.
5. The input controller and its firmware, `/dev/i2c`, keyboard input: a standalone Hydra.
6. PSG and PCM in `/dev/snd` and the player.
7. Demos and games; the TGI driver; the web emulator.
8. Option B: the one-board Vera X.
