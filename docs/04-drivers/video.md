# Video: the Vera X

The Hydra's screen, keyboard and mouse come from the **Vera X** card in slot 0 ([the
card](../02-hardware/cards.md#the-vera-x)): the Commander X16's VERA, with VGA at 640 x 480, two layers of text, tiles or
bitmaps, 128 sprites, a 256-colour palette, a 16-voice PSG and PCM audio, and the X16's input controller for a PS/2
keyboard and mouse.  Two drivers have it:

* **vid** (`reborn/modules/vid/`, task A), the driver: it owns the chip, finds the card as the system starts, sets it up
  for the console, and serves it as files, `#v` at `/dev/vid`.  With no card it ends, and `/dev/vid` is empty.
* **kdev** (`reborn/modules/kdev/input.inc`) reads the keyboard and mouse, on the I2C bus it serves, and passes them
  on.  (It was a program, `input`, which init started and which took a task.)

The VERA's own documentation is the X16's: *The Commander X16 Programmer's Reference*, chapter 9 (the registers, VRAM,
layers, sprites, the PSG and PCM) and chapter 10 (FX).

## Contents

1. [Three ways to use the screen](#three-ways-to-use-the-screen)
2. [The files](#the-files)
3. [A picture from a file](#a-picture-from-a-file)
4. [Drawing](#drawing)
5. [The console's terminal](#the-consoles-terminal)
6. [The keyboard and the mouse](#the-keyboard-and-the-mouse)
7. [PCM](#pcm)
8. [Claiming the chip](#claiming-the-chip)
9. [VRAM's layout](#vrams-layout)
10. [In the emulator](#in-the-emulator)

---

## Three ways to use the screen

From the easiest:

1. **As the console.**  The console's windows show on the screen as well as the serial terminal, so anything a program
   prints, colours and cursor moves included (ANSI sequences: conio, HyForth's terminal words, hylang's `screen`),
   shows there.  Nothing to do.
2. **Through `/dev/vid`'s files**: VRAM, the palette, the sprites and the font read and written as files; the screen's
   modes as commands to `ctl`; drawing by commands to `draw`; a frame waited for by reading `frame`.
3. **Directly**: a program **claims** the chip and writes its registers itself, as an X16 program does, for as long as it
   holds the claim.

---

## The files

| File | Read | Write |
| :--- | :--- | :---- |
| `ctl` | The state, a line each: `vera 47.0.2` (the gateware's version), `mode 80x60`, `cursor blink`, `border 0`, `bitmap off`, `output vga`, `claimed` (and the claimer's task, and `all`) | `mode 80x60`, `mode 80x30`, `mode 40x30`; `cursor blink`, `cursor on`, `cursor off`; `border N`; `bitmap 320 D`, `bitmap 640 D`, `bitmap off`; `output vga`, `output ntsc [mono] [240p]`, `output rgb [240p]`; `claim`, `claim all`, `release`; `reset` |
| `term` | The screen's characters, a line a row | Bytes shown as an ANSI terminal shows them (the console writes here) |
| `vram` | VRAM: the offset is the address, `$00000-$1FFFF` | VRAM |
| `pal` | The palette (256 entries of 2 bytes: `$GB`, `$0R`) | The palette |
| `sprites` | The sprites' attributes (128 of 8 bytes) | The attributes (sprite 0 is the console's cursor, sprite 1 the mouse's pointer) |
| `font` | The console's font (256 characters of 8 bytes) | A font: `cat /lib/font/cp437 >/dev/vid/font` for the PC's |
| `frame` | Waits for the next frame (59.5 a second), then gives the frames counted | |
| `draw` | The pen: `pen 15` | Drawing on the bitmap ([below](#drawing)) |
| `psg` | The PSG's 64 registers as written | Register/value pairs: the sound driver's (its channels 8-23) |
| `pcm`, `pcmctl` | `pcmctl`: the PCM's state | Samples, and their format ([below](#pcm)) |
| `mouse` | The mouse, after a change, as Plan 9's `/dev/mouse` | `m X Y`: move it there |
| `mousein` | | The mouse's moves, as `kdev` has them from the controller: `m DX DY B` |
| `mousectl` | `pointer on`, `swap off` | `pointer on`, `pointer off`, `swap on`, `swap off` |

**Modes**: `mode 80x60` (the screen's 640 x 480 in 8 x 8 characters, as it starts), `mode 80x30` and `mode 40x30` (the
characters taller, or bigger).  **Output**: the VERA drives VGA; `output ntsc` and `output rgb` choose composite or RGB
where the card brings them out (`mono`, NTSC without colour; `240p`, progressive).

---

## A picture from a file

A 320 x 240 picture of 8 bits a pixel, its bytes in a file:

```
cat pic.bin >/dev/vid/vram
cat pic.pal >/dev/vid/pal
echo bitmap 320 8 >/dev/vid/ctl
```

shows it on layer 0, under the console's text (layer 1, whose background colour 0 lets layer 0 show through; `bitmap
320` makes the text 40 x 30, as the picture is shown doubled).  `echo bitmap off >/dev/vid/ctl` takes it away.  (`cp`
won't write a device's file: it makes the file it copies to, and a device's files are there already.  Use `cat` and `>`.)
A bitmap 640 across is 1 or 2 bits a pixel: at 4 or 8 it would be more VRAM than a program has.

---

## Drawing

`/dev/vid/draw` draws on the bitmap, the driver doing the work in its own code, so it's quick (a line of 300 pixels is
some 60,000 cycles) and the console stays on the screen over it:

| Command | What it draws |
| :------ | :------------ |
| `pen C` | The colour from now on (one pen for every program; 15 as it starts) |
| `plot X Y` | A point |
| `line X0 Y0 X1 Y1` | A line |
| `box X0 Y0 X1 Y1`, `bar X0 Y0 X1 Y1` | A rectangle's outline; a filled one |
| `circle X Y R`, `disc X Y R` | A circle's outline; a filled one |
| `text X Y STRING` | Text in the console's font, 8 x 8: each character's dots in the pen's colour, the rest left |
| `clear [C]` | All of the bitmap, in colour 0 or C |

Coordinates are the bitmap's pixels (320 x 240, or 640 x 480), each -4096 to 4095; what falls off isn't drawn.  With no
bitmap a command is `E_INVAL`; while the chip's claimed, `E_BUSY`.  With the gateware's FX, a line at 8 or 4 bits a pixel
uses the chip's line helper, and `clear` its 32-bit cache writes.

```
echo bitmap 320 8 >/dev/vid/ctl
echo pen 4 >/dev/vid/draw; echo circle 160 120 50 >/dev/vid/draw; echo text 120 116 Hydra >/dev/vid/draw
```

Each language has the same words, and more:

| What | rc (`/dev/vid/draw`) | HyForth (`lib video`) | hylang (`(use "video")`) | C (`vera.h`) |
| :--- | :--- | :--- | :--- | :--- |
| The bitmap | `echo bitmap 320 8 >/dev/vid/ctl` | `bitmap ( width depth -- )`, `bitmap-off` | `(bitmap 320 8)`, `(bitmap-off)` | `vid_bitmap (320, 8)` (0: off) |
| The pen | `pen C` | `pen ( c -- )` | `(pen c)` | `vid_pen (c)` |
| A point, a line | `plot X Y`, `line X0 Y0 X1 Y1` | `plot ( x y -- )`, `line ( x0 y0 x1 y1 -- )` | `(plot x y)`, `(line x0 y0 x1 y1)` | `vid_plot`, `vid_line` |
| A box, a bar | `box`, `bar X0 Y0 X1 Y1` | `box`, `bar ( x0 y0 x1 y1 -- )` | `(box ...)`, `(bar ...)` | `vid_box`, `vid_bar` |
| A circle, a disc | `circle`, `disc X Y R` | `circle`, `disc ( x y r -- )` | `(circle x y r)`, `(disc x y r)` | `vid_circle`, `vid_disc` |
| Text | `text X Y STRING` | `text ( x y c-addr u -- )` | `(text x y s)` | `vid_text (x, y, s)` |
| Clear | `clear [C]` | `clear` | `(clear)` | `vid_clear ()` |
| The turtle | | `cs`, `home`, `fd`, `bk ( n -- )`, `rt`, `lt ( deg -- )`, `pu`, `pd`, `heading`, `seth` | `(cs)`, `(home)`, `(fd n)`, `(rt deg)` ... | |
| VRAM | `/dev/vid/vram` | `vpoke ( addr bank c -- )`, `vpeek ( addr bank -- c )`, `vram!`, `vram@` | `(vpoke addr v)`, `(vpeek addr)`, `(vram! addr bytes)`, `(vram@ addr n)` | `vera_write`, `vera_read`, `vera_load`; `vpoke`, `vpeek` (claimed) |
| The palette, sprites | `/dev/vid/pal`, `sprites` | `palette! ( index rgb -- )`, `sprite! ( n c-addr -- )`, `sprite-at ( n x y -- )`, `sprite-off ( n -- )` | `(palette! i rgb)`, `(sprite! n bytes)`, `(sprite-at n x y)`, `(sprite-off n)` | `vid_palette`, `vid_sprite`, `vid_sprite_at`, `vid_sprite_off` |
| The next frame | `/dev/vid/frame` | `vsync` | `(vsync)` | `vera_wait_frame ()` |
| The mouse | `/dev/vid/mouse` | `mouse ( -- x y b )`, `mouse-wait` | `(mouse)`, `(mouse-wait)` | `vid_mouse`, `vid_mouse_wait` |

**The turtle** is Logo's: it starts in the middle heading up, its pen down; `fd` draws as it goes, `rt` and `lt` turn
it (degrees, clockwise), `cs` clears and takes it home.  `: square 4 0 do 80 fd 90 rt loop ;` draws a square.  The
palette's entries are `$RGB`, 4 bits each.

**cc65's TGI** (`tgi.h`) draws here too, so cc65's portable graphics programs run: `tgi_install (hydra_tgi)`, then
`tgi_init ()`: 320 x 240 in 256 colours, lines, bars, circles, ellipses and arcs, text.  The samples `shapes` (TGI) and
`sketch` (`vera.h` and the mouse) are on HydraOS's SD card (`/sd/0/sample/c`).

**A game paces itself by `frame`**: a read waits for the next VSYNC.

---

## The console's terminal

The screen shows the console's windows as the serial terminal does ([two
terminals](serial.md#two-terminals-the-serial-port-and-the-screen)).  The console writes the window shown into
`/dev/vid/term`, an ANSI terminal:

* **80 x 60** characters (`mode 80x30` and `mode 40x30` make them bigger), in the ANSI terminal's **16 colours**, 0-15
  as conio numbers them.  On the screen the chrome takes three rows, so a window on the screen alone is 80 x 57.
* It takes CR, LF, BS, TAB, FF; `ESC 7` and `ESC 8`, `ESC D`, `ESC E`, `ESC M`, `ESC c`; the character sets (`ESC ( 0`
  the DEC Special Graphics, the VT100's line drawing; `ESC ( B` ASCII; SO and SI); CSI `A` `B` `C` `D` `E` `F` `G` `d`
  `H` `f` (moves), `J` and `K`, `m` (bold, reverse, the colours 30-37, 40-47, 90-97, 100-107), `r` (a scrolling
  region), `s` and `u`, `?25h` and `?25l`.  Others are taken and dropped.
* **Scrolling** the whole screen moves layer 1's scroll register (the map is a ring of 64 rows), so it costs a row; a
  region scrolls by copying its rows in VRAM.
* **The cursor** is sprite 0, an underline, blinking (`cursor on` steadies it).
* **The font**'s first 32 characters are the DEC Special Graphics; `/lib/font/cp437` is the PC's character set.

---

## The keyboard and the mouse

With the card's **input controller**, the X16's SMC (an ATtiny861 running the X16's firmware, a PS/2 keyboard and
mouse on it, on the I2C bus at `$42`), the screen is a computer of its own.  **`kdev`** reads it, between its calls
(`SRV_NAP`: [the kernel](../03-base/kernel.md#tasks)): 67 times a second while keys or the mouse are coming, and 10
times a second after 2 s of nothing (a look that finds nothing is one byte on the bus).  The controller is looked for
as the system starts, three times a second apart; with none, `kdev` is an ordinary driver, `idle` in `ps`.

* **The keys go to the console**, as the serial terminal's do: `kdev` writes them to `#c/kbin` as a PC terminal (xterm)
  sends them, so a program can't tell which keyboard they came from.  Ctrl-C is the window's interrupt; Ctrl-] and a
  digit shows a window.  The layout is the US one: Shift, Ctrl, Alt, Caps Lock; the keypad's digits with Num Lock on,
  its cursor keys with it off; Scroll Lock the console's hold.  The locks light their LEDs.
* **The mouse in the console's windows**: `kdev` sends each press, release and turn of the wheel to `#c/kbin` too, as
  xterm's reports.  A click focuses the window under it; a program that turns on `?1000` (and `?1006`) gets the reports
  in its raw keys; the wheel over a window scrolls its scrollback.
* **`/dev/vid/mouse`** is the mouse, as Plan 9's `/dev/mouse`: a read waits for a change, then gives 49 bytes: `m` and
  four fields of 11 characters each with a space after it (x, y, the buttons, and the time in milliseconds).  x and y are
  the screen's pixels (640 x 480; 320 x 240 under `bitmap 320`); the buttons are 1 left, 2 middle, 4 right, 8 and 16 the
  wheel.  Button changes are queued (8 of them), so a click between two reads isn't lost.  Writing `m X Y` moves it.
* **The pointer** is sprite 1, an arrow, shown once the mouse has moved.  `mousectl`: `pointer off`, `pointer on`, and
  `swap on` (the left and right buttons swapped).
* **`/dev/vid/mousein`** is where the moves come in: `kdev` writes `m DX DY B`, a line a change.  Anything else that
  reads a mouse can write it too.

```
% cat /dev/vid/mousectl
pointer on
swap off
% echo m 10 -5 1 >/dev/vid/mousein
```

and a reader of `/dev/vid/mouse` gets `m        330         235           1        8215 ` (from the screen's middle,
where the mouse starts).

---

## PCM

The VERA plays samples from a 4K FIFO at a rate of its own: `/dev/vid/pcm` is the FIFO, `/dev/vid/pcmctl` its settings.

```
echo rate 11025 >/dev/vid/pcmctl; echo bits 8 >/dev/vid/pcmctl; echo mono >/dev/vid/pcmctl
cat drums.raw >/dev/vid/pcm; echo drain >/dev/vid/pcmctl
```

* **The samples** are the VERA's: signed, 8 or 16 bits (16 little-endian), stereo left first.  (A WAV file's 8-bit
  samples are unsigned: `play` makes them signed.)
* **The rate** is the VERA's nearest: steps of 381 Hz, up to 48,828 Hz (`rate 22050` plays at 22,126).  `rate 0`
  stops it.  `pcmctl` reads the rate it has.
* **`pcmctl`'s commands**: `rate HZ`, `bits 8`, `bits 16`, `mono`, `stereo`, `volume N` (0-15), `reset` (the FIFO
  emptied at once), `drain` (waits until it's empty, so a program's end doesn't cut its last sounds off).
* **A write** fills the FIFO when it's below a quarter full, and the rest waits for the next frame, so the FIFO holds 3K
  to 4K while a program keeps it fed.
* **`pcm` is one task's at a time**: another's open is `E_BUSY`.
* **How fast**: from a card, 8 bits mono to about 11 kHz; from the RAM disk to about 22 kHz.  16-bit stereo at 44.1 kHz
  is past a 3.58 MHz 65C02.

`play` plays WAV files and a ZSM song's PCM instruments ([sound](sound.md#play-songs-scores-and-wav-files)).

---

## Claiming the chip

The VERA has one set of address registers, so two tasks can't both write it.  A program that wants the chip writes
**`claim`** to `ctl` and keeps the file open: until it writes `release` or closes its last file of `#v` (its end does),
the chip is its own.

* It may write any register (`hw.inc`'s names: `VERA_ADDR_L`, `VERA_DATA0`, `VERA_CTRL` ...).  The driver leaves the
  chip alone, and other tasks' commands and reads of the chip's files get `E_BUSY`.
* The console keeps its windows' screens itself, so it paints the window shown again once the claim ends.
* **VRAM**: a claimer may use `$00000-$1AFFF` (108K) without saying so; with **`claim all`**, all of it, and the console's
  map and font are made again at the release.
* **The release** sets the chip up for the console again: the palette, the sprites (all off but the cursor and the
  pointer), the layers, the scales, the interrupts.
* **The PSG** stays the sound driver's: during a claim its writes are kept and made at the release.  A claimer that wants
  the PSG claims those channels from `/dev/sndctl` too (`claim 0 65535`).
* **Interrupts**: the driver still owns IRQ line 2 during a claim: it counts frames (so `frame` works) and clears the
  VSYNC, LINE and SPRCOL interrupts a claimer turns on.  A program that wants raster effects polls `VERA_ISR` or
  `SCANLINE`, or waits on `frame`.  As on the X16, keep interrupt code on data port 1 and a program's on port 0, and never
  leave CTRL's DCSEL other than 0 for long.

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

X16 code ports to the Hydra with the registers' base changed: `VERA_BASE` is `$FF20` here.

---

## VRAM's layout

| VRAM | What |
| :--- | :--- |
| `$00000-$1AFFF` | Free for programs (108K): a bitmap (320 x 240 x 8 bits is 75K), tiles, sprites |
| `$1B000-$1EFFF` | The console's text map (a ring of 64 rows) |
| `$1F000-$1F7FF` | The console's font |
| `$1F800` | The cursor's image |
| `$1F820` | The pointer's image |
| `$1F9C0` | The PSG's registers |
| `$1FA00` | The palette |
| `$1FC00` | The sprites' attributes |

---

## In the emulator

`node sim/run.js --vera` puts a Vera X in slot 0 (`base/sim/lib/vera.js`: the registers, VRAM, the layers, sprites and
their collisions, the scan's timing and interrupts, FX, the PCM FIFO, the PSG), and `--smc` its input controller.
`--screen` prints the text layer after a run, `--frame-png FILE` saves the screen, and with `-i`, `--view` shows it live
in a browser (http://localhost:8016), its keys and mouse the controller's, while the terminal stays the serial
console.  `--sound` adds the PSG's and the PCM's sound.  [The emulator](../08-emulator.md) has the rest.
