# The screen: the Vera X

The Hydra-16's video card is the **Vera X**: the X16's VERA (an iCE40UP5K FPGA with 128K of video RAM, VGA out at
640x480, two layers of text, tiles or bitmaps, 128 sprites, a 256-colour palette, a 16-voice PSG and PCM) in slot 0.
Its 32 registers are slot 0's I/O ports 2 and 3, `$FF20-$FF3F`, and its interrupt is IRQ line 2.  The card is Joe
Burks's VERA X, which runs the X16 community's gateware (v47 on: X16Community/vera-module), so the X16's
documentation is this chip's: *The Commander X16 Programmer's Reference*, chapter 9 (the registers, VRAM, layers,
sprites, the PSG and PCM) and chapter 10 (FX).  The plan behind it is [../../../docs/plans/VIDEO.md](../../../docs/plans/VIDEO.md).

A driver, **vid** (`modules/vid`), owns the chip: it finds the card as the system starts, sets it up for the console
and serves it as files.  A program uses the screen three ways, from the easiest:

1. **As the console.**  The console's windows are shown on the screen as well as the serial terminal, so anything a
   program prints, colours and cursor moves included (ANSI sequences: conio, HyForth's terminal words, hylang's
   `screen.hl`), shows there.  Nothing to do.
2. **Through `/dev/vid`'s files**: VRAM, the palette, the sprites and the font, read and written as files; the
   screen's modes as commands to `ctl`; a frame waited for by reading `frame`.
3. **Directly**: a program *claims* the chip and writes its registers itself, as an X16 program does, for as long as
   it holds the claim.

## The files

`#v`, bound at `/dev/vid` by `/rom/lib/namespace` (with no card there's no `#v`, and `/dev/vid` is empty):

| File | Read | Write |
| :--- | :--- | :--- |
| `ctl` | The state, a line each: `vera 47.0.2` (the gateware's version), `mode 80x60`, `cursor blink`, `border 0`, `bitmap off`, `claimed` (and the claimer's task, and `all`) | Commands: `mode 80x60`, `mode 80x30`, `mode 40x30`; `cursor blink`, `cursor on`, `cursor off`; `border N`; `bitmap 320 D`, `bitmap 640 D`, `bitmap off`; `claim`, `claim all`, `release`; `reset` |
| `term` | The screen's characters, a line a row (its columns, then an LF) | Bytes shown as an ANSI terminal shows them (the console writes here) |
| `vram` | VRAM: the offset is the address, `$00000-$1FFFF` | VRAM, through the chip's data port |
| `pal` | The palette (VRAM `$1FA00`: 256 entries of 2 bytes, `$GB` then `$0R`) | The palette |
| `sprites` | The sprites' attributes (VRAM `$1FC00`: 128 of 8 bytes) | The attributes (sprite 0 is the console's cursor) |
| `font` | The console's font (VRAM `$1F000`: 256 characters of 8 bytes, a byte a row) | A font: `cat /lib/font/cp437 >/dev/vid/font` |
| `frame` | Waits for the next frame (59.5 a second), then gives the frames counted, in decimal | |
| `psg` | The PSG's 64 registers (VRAM `$1F9C0`: 16 voices of 4) as written here | Register/value pairs: the sound driver's (its channels 8-23); kept while the chip's claimed, and written as the claim ends |

So a picture is a file copy away: a 320x240 picture of 8 bits a pixel, its bytes in a file, then

```
cat pic.bin >/dev/vid/vram
cat pic.pal >/dev/vid/pal
echo bitmap 320 8 >/dev/vid/ctl
```

shows it on layer 0, under the console's text (which is layer 1: its cells' background colour 0 lets layer 0 show
through; `bitmap 320` makes the text 40x30, as the picture is shown 2x).  `echo bitmap off >/dev/vid/ctl` takes it
away.  (`cp` won't write a device's file: it makes the file it copies to, and a device's files are there already.)

A game paces itself by `frame`: a read waits for the next VSYNC.  In C, `fread` a line from it; in HyForth,
`read-line`.

## The console's terminal

The screen shows the console's windows as the serial terminal does: the window shown, with the keys (Ctrl-] and a
digit shows another, painted on both from its screen, which the console keeps).  A window is the smaller terminal's
size, the serial port's 80 x 24 with both on: on the screen its rows at the top, the scrolling region kept to them.  `consctl` chooses where: `screen`, `serial` or `both` (every window's; it
starts `both`), and reads with a line `terminal both`.  With `screen` alone, output isn't paced by the serial line.

The terminal is 80x60 (`mode 80x30` and `mode 40x30` make the characters bigger), in 16 colours: the ANSI ones, 0-15
as conio numbers them.  It takes CR, LF, BS, TAB, FF; ESC 7 and ESC 8, ESC D (index), ESC E (next line), ESC M
(reverse index), ESC c; CSI `A` `B` `C` `D` `E` `F` `G` `d` `H` `f` (moves), `J` and `K` (0, 1, 2), `m` (0, 1
bold, shown bright, 22, 7 reverse, 27, 30-37, 39, 40-47, 49, 90-97, 100-107), `r` (the scrolling region, as a
VT100's: `CSI 2;23r`, then an LF at row 23 scrolls rows 2-23 alone and ESC M at row 2 scrolls them down; `CSI r`
the whole screen again), `s` and `u`, `?25h` and `?25l`.  Others are taken and dropped.  The whole screen scrolls by
moving layer 1's `VSCROLL` (the map is a ring of 64 rows), so a scroll costs a row; a region scrolls by copying its
rows in VRAM (some 1,500 cycles a row).  The cursor is sprite 0, an underline, blinking (`cursor on` steadies it).

The screen has no keyboard yet: keys still come from the serial terminal.  A PS/2 keyboard, through an input
controller on IRQ line 3, is planned (VIDEO.md).

## Claiming the chip

The VERA has one set of address registers, so two tasks can't both write it.  A program that wants the chip writes
`claim` to `ctl` and keeps the file open: till it writes `release`, or closes its last file of `#v` (its end does),
the chip is its own.  It may write any register; the driver leaves the chip alone, and another task's commands and
reads of the chip's files get `E_BUSY` (a second `claim` too).  The console's output meanwhile waits in the driver
(its last 1K), and is shown when the claim ends.

The VRAM a claimer may use without saying so is `$00000-$1AFFF` (108K: a 320x240 bitmap of 8 bits is 75K); the
console's map is at `$1B000-$1EFFF`, its font at `$1F000-$1F7FF`, the cursor's image at `$1F800`.  With `claim all`
all of VRAM is the program's, and the console's map and font are made again at the release.  The release sets the
chip up for the console: the palette, the sprites (all off but the cursor), the layers, the scales, the interrupts.
The PSG is the sound driver's (`/dev/snd`'s channels 8-23, through `/dev/vid/psg`): during a claim its writes are
kept, not made, and the release writes them, so a song's voices pick up where they are; a claimer that wants the
PSG for itself claims those channels from `/dev/sndctl` too (`claim 0 65535`).

```
            LDR         r0, s_ctl                           ; "/dev/vid/ctl"
            lda         #O_WRITE
            jsr         OPEN
            sta         ctl
            LDR         r0, s_claim                         ; "claim"
            LDR         r1, 5
            lda         ctl
            jsr         WRITE
            ; ... the chip is this program's: VERA_ADDR_L ... (hw.inc's names)
```

**Interrupts.**  The driver still owns IRQ line 2 during a claim: its entry counts frames (so `frame` works), and
clears the VSYNC, LINE and SPRCOL interrupts a claimer turns on (so they don't hold the line), and turns AFLOW off
(the PCM FIFO's: nobody fills it yet).  A program that wants raster effects polls `VERA_ISR` or `SCANLINE`, or waits
on `frame`; one that uses `frame` keeps VSYNC on in `VERA_IEN` (it's on as the claim starts).  As on the X16, keep interrupt code on data port 1 and a program's on port 0, and never leave CTRL's DCSEL
other than 0 long.

**The registers** are `include/hw.inc`'s, the X16's names at the Hydra's base: `VERA_BASE` `$FF20`, `VERA_ADDR_L`,
`VERA_ADDR_M`, `VERA_ADDR_H`, `VERA_DATA0`, `VERA_DATA1`, `VERA_CTRL`, `VERA_IEN`, `VERA_ISR`, `VERA_IRQ_LINE_L`,
`VERA_DC_VIDEO` ... `VERA_SPI_CTRL`, and `VERA_PSG_BASE`, `VERA_PALETTE_BASE`, `VERA_SPRITES_BASE`.  X16 code ports
with the base changed.

## In the emulator

`node sim/run.js --vera` puts a Vera X in slot 0 (`sim/lib/vera.js`: the registers, VRAM, the layers, sprites and
their collisions, the scan's timing and interrupts, the PCM FIFO, the PSG's registers; not FX, and no sound is
made).  `--screen` prints the text layer after the report, `--frame-png FILE` saves the screen, and with `-i`,
`--view` shows it live in a browser (http://localhost:8016) while the terminal stays the serial console; Ctrl-A v
prints it, Ctrl-A p saves it.  Tests set `machine: { vera: true }`; `m.vera.text()` is the screen's text,
`m.vera.psg` the PSG's registers and `m.vera.psgOns` its voices' starts.
