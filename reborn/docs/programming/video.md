# The screen: the Vera X

The Hydra-16's video card is the **Vera X**: the X16's VERA (an iCE40UP5K FPGA with 128K of video RAM, VGA out at
640x480, two layers of text, tiles or bitmaps, 128 sprites, a 256-colour palette, a 16-voice PSG and PCM) in slot 0.
Its 32 registers are slot 0's I/O ports 2 and 3, `$FF20-$FF3F`, and its interrupt is IRQ line 2.  The card is Joe
Burks's VERA X, which runs the X16 community's gateware (v47 on: X16Community/vera-module), so the X16's
documentation is this chip's: *The Commander X16 Programmer's Reference*, chapter 9 (the registers, VRAM, layers,
sprites, the PSG and PCM) and chapter 10 (FX).  The plan behind it is [../design/plans/VIDEO.md](../design/plans/VIDEO.md).
Wiring the card to the board through a bus breakout card, till the carrier card exists:
[../vera-wiring.md](../vera-wiring.md).

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
| `ctl` | The state, a line each: `vera 47.0.2` (the gateware's version), `mode 80x60`, `cursor blink`, `border 0`, `bitmap off`, `output vga`, `claimed` (and the claimer's task, and `all`) | Commands: `mode 80x60`, `mode 80x30`, `mode 40x30`; `cursor blink`, `cursor on`, `cursor off`; `border N`; `bitmap 320 D`, `bitmap 640 D`, `bitmap off`; `output vga`, `output ntsc [mono] [240p]`, `output rgb [240p]` (the VERA's output: composite and RGB where the card brings them out; `mono`, NTSC without colour; `240p`, progressive); `claim`, `claim all`, `release`; `reset` |
| `term` | The screen's characters, a line a row (its columns, then an LF) | Bytes shown as an ANSI terminal shows them (the console writes here) |
| `vram` | VRAM: the offset is the address, `$00000-$1FFFF` | VRAM, through the chip's data port |
| `pal` | The palette (VRAM `$1FA00`: 256 entries of 2 bytes, `$GB` then `$0R`) | The palette |
| `sprites` | The sprites' attributes (VRAM `$1FC00`: 128 of 8 bytes) | The attributes (sprite 0 is the console's cursor, sprite 1 the mouse's pointer) |
| `font` | The console's font (VRAM `$1F000`: 256 characters of 8 bytes, a byte a row) | A font: `cat /lib/font/cp437 >/dev/vid/font` |
| `frame` | Waits for the next frame (59.5 a second), then gives the frames counted, in decimal | |
| `psg` | The PSG's 64 registers (VRAM `$1F9C0`: 16 voices of 4) as written here | Register/value pairs: the sound driver's (its channels 8-23); kept while the chip's claimed, and written as the claim ends |
| `pcm` | | Samples into the PCM FIFO (below: "PCM") |
| `pcmctl` | The PCM's state: `rate 22126`, `bits 8`, `mono`, `volume 15`, `claimed` (and the task that has `pcm`) | `rate HZ`, `bits 8`, `bits 16`, `mono`, `stereo`, `volume N` (0-15), `reset` (the FIFO emptied), `drain` (waits till it's empty) |
| `mouse` | The mouse, after a change, as Plan 9's `/dev/mouse` (below: "The keyboard and the mouse") | `m X Y`: the mouse moved there |
| `mousein` | | The mouse's moves, as the `input` program has them: `m DX DY B` |
| `mousectl` | `pointer on`, `swap off` | `pointer on`, `pointer off`, `swap on`, `swap off` |
| `draw` | The pen: `pen 15` | Drawing on the bitmap (below: "Drawing"): `pen C`, `plot X Y`, `line X0 Y0 X1 Y1`, `box`, `bar`, `circle X Y R`, `disc`, `text X Y STRING`, `clear [C]` |

So a picture is a file copy away: a 320x240 picture of 8 bits a pixel, its bytes in a file, then

```
cat pic.bin >/dev/vid/vram
cat pic.pal >/dev/vid/pal
echo bitmap 320 8 >/dev/vid/ctl
```

shows it on layer 0, under the console's text (which is layer 1: its cells' background colour 0 lets layer 0 show
through; `bitmap 320` makes the text 40x30, as the picture is shown 2x).  `echo bitmap off >/dev/vid/ctl` takes it
away.  (`cp` won't write a device's file: it makes the file it copies to, and a device's files are there already.)
A bitmap 640 across is 1 or 2 bits a pixel: at 4 or 8 it would be more than the program's 108K of VRAM.

## Drawing

`/dev/vid/draw` draws on the bitmap, the driver doing the drawing in its own code, so it's quick (a line of 300
pixels is some 60,000 cycles) and the console stays on the screen over it.  A write is a command: `pen C` (the
colour: the driver's, one for every program, `pen 15` as it starts; a read gives it), `plot X Y`, `line X0 Y0 X1
Y1`, `box X0 Y0 X1 Y1` (its outline), `bar X0 Y0 X1 Y1` (filled), `circle X Y R`, `disc X Y R` (filled), `text
X Y STRING` (the console's font, 8 x 8: each character's dots in the pen's colour, the rest left as it is) and `clear
[C]` (all of the bitmap, in colour 0 or C).  Coordinates are the bitmap's pixels (320 x 240, or 640 x 480), each
-4096 to 4095; what falls off the bitmap isn't drawn.  With no bitmap a command is `E_INVAL`; while the chip's
claimed, `E_BUSY` (a claimer draws for itself).  With FX (the gateware v47 on) a line at 8 or 4 bits a pixel, 320
across, its ends on the bitmap, is the chip's line helper, a write a pixel, and `clear` its 32-bit cache writes.

```
echo bitmap 320 8 >/dev/vid/ctl
echo pen 4 >/dev/vid/draw; echo circle 160 120 50 >/dev/vid/draw; echo text 120 116 Hydra >/dev/vid/draw
```

Each language has the same words for it, and more:

| What | rc (`/dev/vid/draw`) | HyForth (`lib video`) | hylang (`(use "video")`) | C (`vera.h`) |
| :--- | :--- | :--- | :--- | :--- |
| The bitmap | `echo bitmap 320 8 >/dev/vid/ctl` | `bitmap ( width depth -- )`, `bitmap-off` | `(bitmap 320 8)`, `(bitmap-off)` | `vid_bitmap (320, 8)` (0: off) |
| The pen's colour | `pen C` | `pen ( c -- )` | `(pen c)` | `vid_pen (c)` |
| A point, a line | `plot X Y`, `line X0 Y0 X1 Y1` | `plot ( x y -- )`, `line ( x0 y0 x1 y1 -- )` | `(plot x y)`, `(line x0 y0 x1 y1)` | `vid_plot`, `vid_line` |
| A box, a bar (filled) | `box`, `bar X0 Y0 X1 Y1` | `box`, `bar ( x0 y0 x1 y1 -- )` | `(box ...)`, `(bar ...)` | `vid_box`, `vid_bar` |
| A circle, a disc (filled) | `circle`, `disc X Y R` | `circle`, `disc ( x y r -- )` | `(circle x y r)`, `(disc x y r)` | `vid_circle`, `vid_disc` |
| Text (the console's font) | `text X Y STRING` | `text ( x y c-addr u -- )` | `(text x y s)` | `vid_text (x, y, s)` |
| All of it cleared | `clear [C]` | `clear` | `(clear)` | `vid_clear ()` |
| The turtle | | `cs`, `home`, `fd`, `bk ( n -- )`, `rt`, `lt ( deg -- )`, `pu`, `pd`, `heading`, `seth` | `(cs)`, `(home)`, `(fd n)`, `(rt deg)` ... | |
| VRAM | `/dev/vid/vram` | `vpoke ( addr bank c -- )`, `vpeek ( addr bank -- c )`, `vram!`, `vram@ ( addr bank c-addr u -- )` | `(vpoke addr v)`, `(vpeek addr)`, `(vram! addr bytes)`, `(vram@ addr n)` | `vera_write`, `vera_read`, `vera_load`; `vpoke`, `vpeek` (claimed) |
| The palette, sprites | `/dev/vid/pal`, `sprites` | `palette! ( index rgb -- )`, `sprite! ( n c-addr -- )`, `sprite-at ( n x y -- )`, `sprite-off ( n -- )` | `(palette! i rgb)`, `(sprite! n bytes)`, `(sprite-at n x y)`, `(sprite-off n)` | `vid_palette`, `vid_sprite`, `vid_sprite_at`, `vid_sprite_off` |
| The next frame | `/dev/vid/frame` | `vsync` | `(vsync)` | `vera_wait_frame ()` |
| The mouse | `/dev/vid/mouse` | `mouse ( -- x y b )`, `mouse-wait` | `(mouse)`, `(mouse-wait)` | `vid_mouse`, `vid_mouse_wait` |

HyForth's turtle is Logo's: it starts in the middle heading up, its pen down; `fd` draws as it goes, `rt` and `lt`
turn it (degrees, clockwise), `cs` clears and takes it home.  `: square 4 0 do 80 fd 90 rt loop ;` draws a square.
hylang's keeps its place in rationals, so it never drifts.  The palette's entries are `$RGB`, 4 bits each.

**cc65's TGI** (`tgi.h`) draws there too, so cc65's portable graphics programs run: `tgi_install (hydra_tgi)`,
then `tgi_init ()`: 320 x 240 in 256 colours (TGI's colour n the palette's entry its palette gives), lines, bars,
circles, ellipses and arcs, text in the console's font (or TGI's vector fonts), `tgi_getpixel`.  The driver is
`sdk/c/lib/tgihydra.s`, over `/dev/vid/draw`.  The samples: `shapes` (TGI) and `sketch` (`vera.h` and the
mouse: `/sd/0/sample/c/sketch`).

A game paces itself by `frame`: a read waits for the next VSYNC.  In C, `fread` a line from it; in HyForth,
`read-line`.

## The console's terminal

The screen shows the console's windows as the serial terminal does: the window shown, with the keys (Ctrl-] and a
digit shows another, painted on both from its screen, which the console keeps).  A window is the smaller terminal's
size, the serial port's 80 x 24 with both on: on the screen its rows at the top, below the chrome, the scrolling region kept to them.  The chrome is the bar (the windows and the time) and the window's header and footer, a row each, on the screen by default (none on the serial port: `wctl`'s `chrome`), so a window on the screen alone is 80 x 57.  `consctl` chooses where: `screen`, `serial` or `both` (every window's; it
starts `both`), and reads with a line `terminal both`.  With `screen` alone, output isn't paced by the serial line.
`seats` keeps both on but makes each a seat of its own: the screen shows its own window, with the keyboard's keys and
Ctrl-C, and the serial port its own; a group's windows are sized to the terminals showing it (80 x 57 on the screen
alone), and Ctrl-] and its keys act in the seat they came from.  It reads `terminal seats`; `both` is one seat again.

The terminal is 80x60 (`mode 80x30` and `mode 40x30` make the characters bigger), in 16 colours: the ANSI ones, 0-15
as conio numbers them.  A font's first 32 glyphs are the DEC Special Graphics (`tools/decfont.js` puts them in the
console's fonts, `/lib/font/cp437` too); reading `term` gives them as ASCII (`-`, `|`, `+` ...), as the console's
`/text` does.  It takes CR, LF, BS, TAB, FF; ESC 7 and ESC 8, ESC D (index), ESC E (next line), ESC M
(reverse index), ESC c, the character sets (`ESC ( 0` and `ESC ) 0` the DEC Special Graphics, the VT100's line
drawing, `ESC ( B` ASCII; SO and SI choose G1 or G0); CSI `A` `B` `C` `D` `E` `F` `G` `d` `H` `f` (moves), `J` and `K` (0, 1, 2), `m` (0, 1
bold, shown bright, 22, 7 reverse, 27, 30-37, 39, 40-47, 49, 90-97, 100-107), `r` (the scrolling region, as a
VT100's: `CSI 2;23r`, then an LF at row 23 scrolls rows 2-23 alone and ESC M at row 2 scrolls them down; `CSI r`
the whole screen again), `s` and `u`, `?25h` and `?25l`.  Others are taken and dropped.  The whole screen scrolls by
moving layer 1's `VSCROLL` (the map is a ring of 64 rows), so a scroll costs a row; a region scrolls by copying its
rows in VRAM (some 1,500 cycles a row).  The cursor is sprite 0, an underline, blinking by its z (`cursor on` steadies
it); the sprites are on all along, so a program's, and the mouse's pointer, don't blink with it.

## The keyboard and the mouse

With the card's input controller, the X16's SMC (an ATtiny861 with its firmware: a PS/2 keyboard and a PS/2 mouse,
on the I2C bus at `$42`), the screen is a computer of its own.  The `input` program, which init starts, reads it
67 times a second while keys or the mouse are coming, 10 times a second after 2 s of nothing (each look costs some
5,000 cycles, so a quiet system pays 1.8% of the CPU for it); with no controller it ends at once.

* **The keys go to the console**, as the serial terminal's do: `input` writes them to `#c/kbin` (the console's
  keyboard) as a PC terminal (xterm) sends them, so a program can't tell which keyboard they came from.  A raw read
  gets the cursor and function keys as one code each (`KEY_*`); Ctrl-C is the window's interrupt; Ctrl-] and a digit
  shows a window.  The layout is the US one: Shift, Ctrl, Alt (an ESC first), Caps Lock; the keypad's digits with Num
  Lock on (as it starts) and its cursor keys with it off; Scroll Lock the console's hold.  The locks light their LEDs.
* **The mouse in the console's windows**: `input` sends each press, release and turn of the wheel to `#c/kbin` too, as
  xterm's reports (`CSI < B ; X ; Y M` or `m`, the cell where the pointer was).  A click focuses the window under it
  (a tile, a popup); a program that turns on `?1000` (and `?1006`) gets the reports in its raw keys at its own cells;
  the wheel over a window that doesn't scrolls its scrollback's view.
* **The mouse** is `/dev/vid/mouse`, as Plan 9's `/dev/mouse`: a read waits for a change, then gives 49 bytes, `m`
  and four fields of 11 characters each with a space after it: x, y, the buttons and the time in milliseconds.  x and
  y are the screen's pixels (640 x 480; 640 x 240 in `mode 80x30`, 320 x 240 in `mode 40x30` or under `bitmap 320`);
  the buttons are 1 left, 2 middle, 4 right, 8 and 16 the wheel up and down (each pressed, then let go).  An open's
  first read gives the mouse at once; a non-blocking fd gets `E_AGAIN` when there's nothing new.  The buttons'
  changes are queued (8 of them, each fd reading them in turn), so a click between two reads isn't lost; the moves
  aren't (a read gives the latest).  Writing `m X Y` moves it.
* **The pointer** is sprite 1, an arrow (its image at VRAM `$1F820`, in the grey ramp: palette offset 1), shown once
  the mouse has moved.  `/dev/vid/mousectl` takes `pointer off` and `pointer on`, and `swap on` (the left and
  right buttons swapped) and `swap off`.  While the chip's claimed the pointer is off, the claimer's to draw (it reads
  `/dev/vid/mouse` as anyone does), and the release shows it again.
* **`/dev/vid/mousein`** is where the moves come in: `input` writes `m DX DY B`, a line a change (y down, the
  buttons as `mouse` gives them).  Anything else that reads a mouse can write it too.

```
% cat /dev/vid/mousectl
pointer on
swap off
% echo m 10 -5 1 >/dev/vid/mousein
```

and a reader of `/dev/vid/mouse` gets `m        330         235           1        8215 ` (from the screen's middle,
320 by 240, where the mouse starts).

## PCM

The VERA plays samples from a 4K FIFO at a rate of its own: `/dev/vid/pcm` is the FIFO, `/dev/vid/pcmctl` its
settings.  A program sets the format and rate, then writes the samples:

```
echo rate 11025 >/dev/vid/pcmctl; echo bits 8 >/dev/vid/pcmctl; echo mono >/dev/vid/pcmctl
cat drums.raw >/dev/vid/pcm; echo drain >/dev/vid/pcmctl
```

* **The samples are the VERA's:** signed, 8 or 16 bits (16 little-endian), stereo left first.  (A WAV file's 8-bit
  samples are unsigned: `play` makes them signed.)
* **The rate** is the VERA's nearest: 48,828.125 Hz / 128 steps, 381 Hz each, up to 48,828 (`rate 22050` plays at
  22,126, `rate 8000` at 8,011); `rate 0` stops it.  `pcmctl` reads the rate it has.
* **A write** goes into the FIFO when it's below a quarter full, as much as it has room for, and the rest waits for
  the next frame (59.5 a second); a non-blocking fd gets `E_AGAIN`.  So the FIFO holds 3K to 4K while a program
  keeps it fed: at 11 kHz, 8-bit mono, a third of a second.
* **`pcm` is one task's at a time**: another's open is `E_BUSY` till its last fd of it closes, and its `pcmctl`
  commands too (anyone's, while nobody has `pcm`).
* **`drain`** waits till the FIFO's empty (a program's end shouldn't cut its last sounds off); `reset` empties it
  at once.
* **How fast:** the driver puts some 22 cycles into each byte, and a card reads at about 12K a second, so from a card
  8 bits mono to about 11 kHz is comfortable; from the RAM disk (`/ram`) to about 22 kHz.  16 bits stereo at 44.1
  kHz (176K a second) is past a 3.58 MHz 65C02.
* A claim of the chip stops it: `pcm`'s writes wait, and the release sets its format, volume and rate again (its
  FIFO emptied).

`play` plays WAV files (8 or 16 bits, mono or stereo; PCM, not compressed or floating point) and a ZSM song's PCM
instruments (their data read into RAM when it fits, about 20K, else read from the file as it plays).

## Claiming the chip

The VERA has one set of address registers, so two tasks can't both write it.  A program that wants the chip writes
`claim` to `ctl` and keeps the file open: till it writes `release`, or closes its last file of `#v` (its end does),
the chip is its own.  It may write any register; the driver leaves the chip alone, and another task's commands and
reads of the chip's files get `E_BUSY` (a second `claim` too), and so does any write to `term`, the claimer's too.
The console keeps its windows' screens itself, so it paints the window shown again once the claim ends.  The first
write to `term` after the screen changed under the console (a `mode`, a `bitmap`, a `reset`, a claim's end) is
refused once with `E_BUSY` too: the console then reads the size from `ctl`'s `mode` line, sizes its windows to it,
and paints the window shown again.

The VRAM a claimer may use without saying so is `$00000-$1AFFF` (108K: a 320x240 bitmap of 8 bits is 75K); the
console's map is at `$1B000-$1EFFF`, its font at `$1F000-$1F7FF`, the cursor's image at `$1F800`, the pointer's at `$1F820`.  With `claim all`
all of VRAM is the program's, and the console's map and font are made again at the release.  The release sets the
chip up for the console: the palette, the sprites (all off but the cursor and the pointer), the layers, the scales, the interrupts.
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
(the PCM FIFO's, a level that would hold the line: `pcm`'s writers wait for frames instead).  A program that wants
raster effects polls `VERA_ISR` or `SCANLINE`, or waits on `frame`; one that uses `frame` keeps VSYNC on in `VERA_IEN`
(it's on as the claim starts).  As on the X16, keep interrupt code on data port 1 and a program's on port 0, and never
leave CTRL's DCSEL other than 0 long.

**The registers** are `base/include/hw.inc`'s, the X16's names at the Hydra's base: `VERA_BASE` `$FF20`, `VERA_ADDR_L`,
`VERA_ADDR_M`, `VERA_ADDR_H`, `VERA_DATA0`, `VERA_DATA1`, `VERA_CTRL`, `VERA_IEN`, `VERA_ISR`, `VERA_IRQ_LINE_L`,
`VERA_DC_VIDEO` ... `VERA_SPI_CTRL`, and `VERA_PSG_BASE`, `VERA_PALETTE_BASE`, `VERA_SPRITES_BASE`.  X16 code ports
with the base changed.

## In the emulator

`node sim/run.js --vera` puts a Vera X in slot 0 (`base/sim/lib/vera.js`: the registers, VRAM, the layers, sprites and
their collisions, the scan's timing and interrupts, the PCM FIFO, the PSG's registers; not FX).  `--screen` prints
the text layer after the report, `--frame-png FILE` saves the screen, and with `-i`, `--view` shows it live in a
browser (http://localhost:8016) while the terminal stays the serial console; Ctrl-A v prints it, Ctrl-A p saves it.
Its sound, the PSG's and the PCM's, is made with the YM2151's (`base/sim/lib/audio.js`) when it's asked for: `-i
--sound` plays it in the browser (the same page as `--view`'s, its Sound button) and `--wav FILE` keeps it.  Tests
set `machine: { vera: true }`; `m.vera.text()` is the screen's text, `m.vera.psg` the PSG's registers and
`m.vera.psgOns` its voices' starts; with `vera: { pcmLog: true }`, `m.vera.pcmLog` is the bytes the FIFO took, and
`m.vera.pcmUnderruns` counts its runs dry while it played; with `sound: true` too, `start(m)` can listen
(`m.audio.on(fn)`: the sound test's).
