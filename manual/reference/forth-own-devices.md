## gpio, gpio!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio    ( pin -- level )
gpio!   ( pin level -- )
```

#### Description

The eight pins of the I/O header J27, numbered 0 to 7 (section 17.2). `gpio` leaves a pin's level now, 0 or
1. `gpio!` makes the pin an output and sets it: low for a *level* of 0, high for anything else.

Every pin is an input when the Hydra starts, and an input with nothing connected reads 1.

The library's words work through the files of `/dev/gpio`, and a failure is thrown, with the word's name:
`gpio: not found` for a pin that is not 0 to 7.

#### See also

`gpio-in`, `gpio-port`, `gpio-state`, `ctl`.

#### Example

```session
/> lib gpio
/> 2 gpio .
1
/> 2 0 gpio!  2 gpio .
0
/> 2 gpio-in
```

## gpio-in, gpio-out, gpio-ddr!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio-in     ( pin -- )
gpio-out    ( pin -- )
gpio-ddr!   ( byte -- )
```

#### Description

Set the pins' directions. `gpio-in` makes one pin an input, and `gpio-out` an output. `gpio-ddr!` sets all
eight at once: bit *n* of the byte is 1 to make pin *n* an output, and 0 for an input.

#### See also

`gpio!`, `gpio-port!`, `gpio-state`.

## gpio-port, gpio-port!

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio-port    ( -- byte )
gpio-port!   ( byte -- )
```

#### Description

All eight pins at once, as one byte: bit *n* is pin *n*. `gpio-port` leaves their levels. `gpio-port!` sets
the levels of the pins that are outputs; those that are inputs stay inputs, and their bits are ignored.

#### See also

`gpio`, `gpio-ddr!`.

#### Example

```session
/> lib gpio
/> 15 gpio-ddr!  5 gpio-port!
/> gpio-port .
245
/> 0 gpio-ddr!
```

## gpio-ca1!, gpio-ca2!, gpio-wait

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio-ca1!   ( rise? -- )
gpio-ca2!   ( n -- )
gpio-wait   ( -- count )
```

#### Description

The header's two handshake lines. `gpio-ca1!` chooses which change of CA1 is counted: its rises, from 0 to
1, for a true flag, and its falls for a false one. `gpio-wait` waits for the next such change, and leaves
the number counted so far: this is how a program waits for a button without watching it.

`gpio-ca2!` sets CA2: 0 makes it an output that is low, 1 an output that is high, and −1 an input.

#### See also

`gpio-state`, `key?`; section 17.2.

## gpio-state

The devices; in the `gpio` library, a file of Forth in `/lib/forth` (`lib gpio`).

#### Stack

```text
gpio-state   ( -- c-addr u )
```

#### Description

Leaves the text of the pins' control file: a line for each pin, with its number, its direction and its
level, then CA1's edge and its count, and CA2's setting. The text is in a buffer of the library's, good
until its next word is used.

#### See also

`gpio`, `gpio-ddr!`.

#### Example

```session
/> lib gpio
/> gpio-state type
0 in 1
1 in 1
2 in 1
3 in 1
4 in 1
5 in 1
6 in 1
7 in 1
ca1 fall 0
ca2 in
```

## i2c-read, i2c-write, i2c-devices, i2c?, i2c-speed, i2c-reg-size

The devices; in the `i2c` library, a file of Forth in `/lib/forth` (`lib i2c`).

#### Stack

```text
i2c-read       ( addr reg c-addr u -- )
i2c-write      ( addr reg c-addr u -- )
i2c-devices    ( -- )
i2c?           ( addr -- flag )
i2c-speed      ( khz -- )
i2c-reg-size   ( n -- )
```

#### Description

The I²C bus on the I/O header (section 17.3). *addr* is a chip's address on the bus, `$08` to `$77`.

`i2c-read` reads *u* bytes, 64 at most, from the chip into memory at *c-addr*, and `i2c-write` sends *u*
bytes from there. *reg* is the register inside the chip to start from; it is used only when a register size
has been set, and is otherwise given as 0.

`i2c-devices` prints the addresses of the chips that answer, in hex, and `i2c?` leaves true if one answers at
the address given.

`i2c-speed` sets the bus's speed in kilohertz, 1 to 100. `i2c-reg-size` says how a register's number is
sent before the data: 0 not at all, 1 as one byte, 2 as two, high byte first.

A chip that does not answer is the error `i/o error`, thrown with the word's name.

#### See also

`gpio`, `spi`, `ctl`.

#### Example

With nothing on the bus:

```session
/> lib i2c
/> $50 i2c? .
0
/> $50 0 pad 2 i2c-read
i2c-read: i/o error
```

## spi, spi-read, spi-mode

The devices; in the `spi` library, a file of Forth in `/lib/forth` (`lib spi`).

#### Stack

```text
spi        ( dev c-addr u -- )
spi-read   ( dev c-addr u -- )
spi-mode   ( dev mode -- )
```

#### Description

The SPI devices, 0 to 15 (section 17.4): 0 to 7 are the board's headers, and 8 to 15 the slots' cards.

`spi` is one transaction: the device is selected, the *u* bytes at *c-addr* are sent, and the bytes the
device sent back meanwhile are put in their place, in the same memory. `spi-read` clocks *u* bytes in,
sending `$FF` for each. `spi-mode` sets the clock's mode for the device, 0 or 3.

A device with nothing connected gives `$FF` for every byte. One that is an SD card the system is using
can't be opened: `busy`.

#### See also

`i2c-read`, `gpio`.

#### Example

With nothing connected to device 3:

```session
/> lib spi
/> create tx  1 c, 2 c, 3 c,
/> 3 tx 3 spi
/> tx c@ . tx 1+ c@ . tx 2 + c@ .
255 255 255
```

## set-date, rtc

The devices; in the `clock` library, a file of Forth in `/lib/forth` (`lib clock`).

#### Stack

```text
set-date   ( c-addr u -- )
rtc        ( -- c-addr u )
```

#### Description

`set-date` sets the clock from a string in the form `2026-10-13 09:41:00`; with a clock chip fitted, the
chip is set too. `rtc` leaves the text that says what the system found for a clock chip as it started:
`running`, `stopped` (it needs setting) or `none`, with `battery low` added if its battery is running down.

Reading the clock needs no library: `time&date`, in the Facility library, gives it.

#### See also

`time&date`; section 17.5.

#### Example

```session
/> lib clock
/> rtc type
none
/> s" nonsense" set-date
set-date: invalid argument
```

## disk-ctl, disk-cmd, disk-start, disk-stop, cards

The devices; in the `disk` library, a file of Forth in `/lib/forth` (`lib disk`).

#### Stack

```text
disk-ctl     ( disk -- c-addr u )
disk-cmd     ( c-addr u disk -- )
disk-start   ( disk -- )
disk-stop    ( disk -- )
cards        ( -- mask )
```

#### Description

The disks (Chapter 14). A disk is named by its letter, as a character: `char 0` to `char f` for the card on
that SPI device, `char x` for the ROM disk, `char r` for the RAM disk and `char s` for the shared one.

`disk-ctl` leaves the text of the disk's control file: what it is and its size, and its file system's label
and free space. `disk-cmd` sends that file a command, any of those Chapter 14 lists (`check`, `label` ...).
`disk-start` starts a card again, as is needed after one is changed, and `disk-stop` stops a RAM disk and
gives its memory back.

`cards` leaves a mask of the SD cards that are there: bit *n* is set for one on SPI device *n*.

#### See also

The commands `df`, `fsck` and `label`; Chapter 14.

#### Example

```session
/> lib disk
/> char x disk-ctl type
rom 4 MB 8192 blocks
hydrafs label=ROM
partition at block 1056
free 1 KB of 735 KB
/> cards .
1
```

## window, windows, new-window, new-group, show-window, window-label, window-status, window-ctl

The devices; in the `cons` library, a file of Forth in `/lib/forth` (`lib cons`).

#### Stack

```text
window          ( -- n )
windows         ( -- c-addr u )
show-window     ( n -- )
new-window      ( c-addr u -- )
new-group       ( c-addr u -- )
window-label    ( c-addr u -- )
window-status   ( c-addr u -- )
window-ctl      ( c-addr u -- )
```

#### Description

The console's windows (Chapter 10). `window` leaves the number of the window HyForth is in. `windows` leaves
the text that lists them all, a line each, with a `*` on the one that is shown. `show-window` shows window
*n*.

`new-window` makes a window, shows it, and runs the command line given in it; an empty string gives it a
shell. `new-group` does the same with the window in a note group of its own.

`window-label` sets this window's title (an empty string puts back its program's name), `window-status` the
text of its status line, and `window-ctl` writes any line to its control file.

#### See also

`send`, `form`, `k-resize`; the command `new-window`.

#### Example

```session
/> lib cons
/> window .
0
/> windows type
0 0 80 24 *
```

## task-args, task-cwd, task-env, task-ns, task-regs, task-mem, task-ram

The devices; in the `proc` library, a file of Forth in `/lib/forth` (`lib proc`).

#### Stack

```text
task-args   ( task -- c-addr u )
task-cwd    ( task -- c-addr u )
task-env    ( task -- c-addr u )
task-ns     ( task -- c-addr u )
task-regs   ( task -- c-addr u )
task-mem    ( task addr c-addr u -- )
task-ram    ( task bank offset c-addr u -- )
```

#### Description

A look inside any task, through its files in `/proc` (section 17.8). The first five leave text, in a buffer
of the library's, 512 characters at most: the task's arguments; its current directory; its environment, a
line for each variable; its namespace, as the `bind` and `mount` lines that made it; and the processor's
registers as they were when it last stopped running.

`task-mem` reads *u* bytes of the task's memory, from *addr* as the task sees its 64K, into memory at
*c-addr*; `task-ram` reads from one of its RAM banks. The memory and registers of the kernel's task and of
the drivers can't be read: `not allowed`.

#### See also

`sys-getpid`, `note`; the commands `ps` and `db`.

#### Example

```session
/> lib proc
/> 1 task-args type
-l
/> 1 task-cwd type
/
/> 9 task-args type
task-args: not found
```

## pc?

The devices; in the `pc` library, a file of Forth in `/lib/forth` (`lib pc`).

#### Stack

```text
pc?   ( -- flag )
```

#### Description

True if the PC tool answers on the serial line, so that the files of `/pc` can be used; false, after a wait
of a second, if nothing answers. `/pc`'s files themselves are ordinary files, for the file words.

#### See also

`open-file`, `file-status`; section 9.9.

## snd-claim, snd-release, snd-claim-psg, snd-release-psg

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-claim         ( mask -- )
snd-release       ( mask -- )
snd-claim-psg     ( mask -- )
snd-release-psg   ( mask -- )
```

#### Description

Claim sound channels, so that no other program's sounds reach them, and give them back. The mask has a bit
for each channel: bit *n* is channel *n* of the YM2151's eight, and for the `-psg` pair, voice *n* of the
Vera X's PSG, which is channel 8 + *n*.

A claim lasts until it is released or HyForth ends. While HyForth has a channel, the `play` program can't
use it: `snd-mml`, `snd-chord` and `snd-play` want it released first.

#### See also

`snd-note`, `snd-reset`; Chapter 15.

## snd-patch

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-patch   ( ch p -- )
```

#### Description

Loads an instrument into a channel: patch *p*, one of the 163 that the sound driver holds, 0 to 162
(Chapter 15). The notes played on the channel afterward have that instrument's sound.

A channel is 0 to 7 for the YM2151's voices, and 8 to 23 for the PSG's, with a Vera X.

#### See also

`snd-note`, `snd-wave`, `snd-level`.

## snd-note, snd-off

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-note   ( ch n -- )
snd-off    ( ch -- )
```

#### Description

`snd-note` starts a note on a channel: *n* is a MIDI note number, 60 being middle C and each semitone one
more. `snd-off` ends the channel's note, which then dies away as its instrument does.

A note sounds until it is ended, so a note of a given length is `snd-note`, a wait, and `snd-off`.

#### See also

`note-of`, `snd-patch`, `snd-freq`, `tune`, `ms`.

#### Example

```session
/> lib sound
/> lib facility
/> 0 5 snd-patch
/> 0 60 snd-note  500 ms  0 snd-off
```

## snd-level, snd-volume, snd-pan

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-level    ( ch v -- )
snd-volume   ( v -- )
snd-pan      ( ch pan -- )
```

#### Description

`snd-level` sets one channel's volume, 0 to 127. `snd-volume` sets the master volume for all sound, 0 to
200, where 100 is "as written". `snd-pan` chooses a channel's speakers: 1 the left, 2 the right, 3 both.

#### See also

`snd-note`, `snd-reset`.

## snd-bend, snd-glide

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-bend    ( ch n -- )
snd-glide   ( ch n -- )
```

#### Description

`snd-bend` bends the pitch of a channel's note by *n* 64ths of a semitone, up for a positive number and
down for a negative one. `snd-glide` moves the sounding note to note *n* without starting it again: the
pitch changes, and the note's attack is not repeated.

#### See also

`snd-note`, `snd-freq`.

## snd-drum

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-drum   ( ch n -- )
```

#### Description

Plays a drum on a channel: *n* is a General MIDI drum number, 35 being a kick drum and 38 a snare.

#### See also

`snd-note`, `tune`.

## snd-freq

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-freq   ( ch hz -- )
```

#### Description

Starts a note at a frequency given in hertz, for a pitch that is not one of the scale's notes. A frequency
of 0 ends it.

#### See also

`snd-note`, `snd-bend`.

## snd-wave

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-wave   ( ch w width -- )
```

#### Description

Sets the waveform of a PSG channel, 8 to 23: *w* is 0 for a pulse, 1 a sawtooth, 2 a triangle, 3 noise.
*width* is a pulse's width, 0 to 63, where 63 is a square wave.

#### See also

`snd-patch`, `snd-note`.

## snd-reset

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-reset   ( -- )
```

#### Description

Silences the sound chips and clears every setting: patches, volumes, bends. The channels a program has
claimed stay claimed.

#### See also

`snd-off`, `snd-volume`, `snd-release`.

## snd-regs

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-regs   ( c-addr -- )
```

#### Description

Copies the YM2151's 256 registers, as they were last written, into memory at *c-addr*, which must have room
for 256 bytes. The chip itself can't be read: these are the driver's record of what has been sent to it.

#### See also

`snd-reg`, `dump`.

#### Example

```session
/> lib sound
/> create regs 256 allot
/> 32 199 snd-reg  regs snd-regs  regs 32 + c@ .
199
```

## note-of

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
note-of   ( c-addr u -- n )
```

#### Description

Leaves the MIDI number of a note given by name: a letter A to G, a `#` for a sharp or a `b` for a flat if
there is one, and the octave. `C4` is middle C, 60.

#### Errors

| Throw code | Message | Cause |
|---|---|---|
| −24 | `invalid numeric argument` | The string is not a note's name |

#### See also

`snd-note`, `tune`.

#### Example

```session
/> lib sound
/> s" A4" note-of .
69
/> s" C#4" note-of . s" Db4" note-of .
61 61
```

## tune

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
tune   ( c-addr u ch tempo -- )
```

#### Description

Plays a tune on a channel, and returns when it is over. The string is a list of notes and their lengths,
with spaces between: a note's name as `note-of` takes it, or `-` for a rest, and then its length in beats,
1 to 255. *tempo* is the number of beats in a minute.

Each note is started on its beat and ended just before the next, in time with the system's tick.

#### See also

`note-of`, `snd-mml`, `snd-patch`.

#### Example

```session
/> lib sound
/> 0 5 snd-patch
/> s" C4 1 E4 1 - 1 G4 2" 0 240 tune
```

## snd-mml, snd-chord, snd-play

Sound; in the `sound` library (`lib sound`).

#### Stack

```text
snd-mml     ( ch c-addr u -- status )
snd-chord   ( ch c-addr u -- status )
snd-play    ( c-addr u times -- status )
```

#### Description

Play music with the `play` program, and leave its exit status: 0 if all went well.

`snd-mml` plays a line of MML, the score language of section 15.6, on a channel. `snd-chord` plays the
notes of such a line together, as a chord, on that channel and the ones after it. `snd-play` plays a song
file, a ZSM, the number of times given; 0 plays its loop until <kbd>CTRL</kbd>-<kbd>C</kbd>.

Each waits until the music is over. `play` is another program, and needs the channels free: release any that
HyForth has claimed first.

#### See also

`tune`, `snd-claim`; the command `play`.

#### Example

```session
/> lib sound
/> 0 s" cde" snd-mml .
0
/> s" /nothere.zsm" 1 snd-play .
play: /nothere.zsm: not found
1
```

## bitmap, bitmap-off

The screen; in the `video` library (`lib video`).

#### Stack

```text
bitmap       ( width depth -- )
bitmap-off   ( -- )
```

#### Description

`bitmap` turns on a bitmap on a Vera X's screen, under the console's text, for the drawing words. *width* is
320 or 640 dots across, giving 240 or 480 down; *depth* is the number of bits for each dot, 1, 2, 4 or 8,
which gives 2, 4, 16 or 256 colours. At 640 across only 1 or 2 bits are possible. `bitmap-off` turns it off.

The library needs a Vera X card (the emulator has one with `--vera`). Without one, its words are the error
`not found`. A drawing word used with no bitmap on is `invalid argument`.

#### See also

`pen`, `plot`, `clear`; Chapter 16.

#### Example

A square, a filled circle, and the turtle's hexagon:

```text
lib video
320 8 bitmap
5 pen  10 10 100 100 box  200 60 30 disc
cs  : hexagon  6 0 do 40 fd 60 rt loop ;  hexagon
bitmap-off
```

## pen, border

The screen; in the `video` library (`lib video`).

#### Stack

```text
pen      ( colour -- )
border   ( colour -- )
```

#### Description

`pen` sets the colour that the drawing words draw in from now on: a number of the palette, within what the
bitmap's depth allows. `border` sets the colour of the screen's border, outside the picture.

#### See also

`plot`, `palette!`, `bitmap`.

## plot

The screen; in the `video` library (`lib video`).

#### Stack

```text
plot   ( x y -- )
```

#### Description

Draws one dot in the pen's colour. Positions count from 0 0 at the top left corner of the bitmap, *x*
across and *y* down. A dot outside the bitmap is not drawn, and is not an error.

#### See also

`line`, `pen`, `bitmap`.

## line, box, bar

The screen; in the `video` library (`lib video`).

#### Stack

```text
line   ( x0 y0 x1 y1 -- )
box    ( x0 y0 x1 y1 -- )
bar    ( x0 y0 x1 y1 -- )
```

#### Description

Draw, in the pen's colour: `line` a straight line from one point to the other; `box` the outline of the
rectangle with those two points as opposite corners; `bar` the same rectangle filled.

#### See also

`plot`, `circle`, `fd`.

## circle, disc

The screen; in the `video` library (`lib video`).

#### Stack

```text
circle   ( x y r -- )
disc     ( x y r -- )
```

#### Description

Draw a circle with its centre at *x*, *y* and radius *r*, in the pen's colour: `circle` its outline, and
`disc` filled.

#### See also

`box`, `plot`.

## clear

The screen; in the `video` library (`lib video`).

#### Stack

```text
clear   ( -- )
```

#### Description

Clears the bitmap: every dot becomes colour 0. The console's text over it is not touched.

#### See also

`cs`, `page`, `bitmap`.

## text

The screen; in the `video` library (`lib video`).

#### Stack

```text
text   ( x y c-addr u -- )
```

#### Description

Draws a string on the bitmap, in the pen's colour and the console's 8 by 8 font, with the top left of its
first character at *x*, *y*. At most 40 characters are drawn.

Text drawn this way is part of the picture: unlike what `type` prints, it does not scroll.

#### See also

`type`, `at-xy`, `pen`.

## cs, home, fd, bk, rt, lt, pu, pd, heading, seth

The screen; in the `video` library (`lib video`).

#### Stack

```text
cs        ( -- )            home      ( -- )
fd        ( n -- )          bk        ( n -- )
rt        ( degrees -- )    lt        ( degrees -- )
pu        ( -- )            pd        ( -- )
heading   ( -- degrees )    seth      ( degrees -- )
```

#### Description

Turtle graphics, as in the language Logo. The turtle is a point on the bitmap with a heading and a pen.
`home` puts it in the middle of the screen, heading up, with its pen down, and `cs` clears the bitmap first.
`fd` moves it forward *n* dots along its heading, drawing a line in the pen's colour if the pen is down, and
`bk` moves it back. `rt` and `lt` turn it right and left by a number of degrees. `pu` lifts the pen, so that
moves draw nothing, and `pd` puts it down.

`heading` leaves the heading, 0 to 359 degrees, 0 being up and 90 to the right; `seth` sets it.

The turtle's place is kept to a sixteenth of a dot, so a figure of many small moves does not drift.

#### See also

`line`, `pen`, `bitmap`.

## vpeek, vpoke, vram@, vram!

The screen; in the `video` library (`lib video`).

#### Stack

```text
vpeek   ( addr bank -- c )
vpoke   ( addr bank c -- )
vram@   ( addr bank c-addr u -- )
vram!   ( addr bank c-addr u -- )
```

#### Description

The VERA's own memory, the video RAM: 128K, so an address is 17 bits, given as a cell and a *bank* of 0 or
1. `vpeek` leaves the byte there and `vpoke` stores one. `vram@` reads *u* bytes from it into memory at
*c-addr*, and `vram!` writes *u* bytes to it.

This is the chip's raw memory: the bitmap, the tiles and the sprites' pictures are all in it (Chapter 16
gives the layout).

#### See also

`palette!`, `sprite!`, `bitmap`.

## palette!, sprite!, sprite-at, sprite-off, vsync

The screen; in the `video` library (`lib video`).

#### Stack

```text
palette!     ( index rgb -- )
sprite!      ( n c-addr -- )
sprite-at    ( n x y -- )
sprite-off   ( n -- )
vsync        ( -- )
```

#### Description

`palette!` sets a palette entry to a colour written as `$RGB`: four bits each of red, green and blue.
`sprite!` sets sprite *n*'s eight attribute bytes from memory at *c-addr*; `sprite-at` moves a sprite to
*x*, *y*; `sprite-off` hides it.

`vsync` waits for the start of the next frame, of which there are about sixty a second. A program that
moves things on the screen does so once a frame, after `vsync`, so that the movement is smooth.

#### See also

`vram!`, `pen`, `ms`.

## mouse, mouse-wait

The screen; in the `video` library (`lib video`).

#### Stack

```text
mouse        ( -- x y buttons )
mouse-wait   ( -- x y buttons )
```

#### Description

The mouse of a Vera X. `mouse` leaves where the pointer is now, in the screen's dots, and which buttons are
held: 1 for the left, 2 the middle, 4 the right, added together. `mouse-wait` waits until the mouse moves or
a button changes, and then leaves the same three.

#### See also

`ekey`, `vsync`; section 16.8.

## rand, rand32

Random numbers; in the `random` library (`lib random`).

#### Stack

```text
rand     ( -- u )
rand32   ( -- ud )
```

#### Description

The next random number from the library's generator: `rand` 16 bits of it, as an unsigned cell, and `rand32`
all 32, as an unsigned double.

The generator is seeded from the clock and the tick count as the library loads, so each run gives a
different series, unless `rseed` sets it.

#### See also

`random`, `rseed`, `nrandom`.

## random

Random numbers; in the `random` library (`lib random`).

#### Stack

```text
random   ( u -- u' )
```

#### Description

A random number from 0 to *u* − 1: `6 random 1+` is the throw of a die. For a *u* of 0 the result is 0.

#### See also

`rand`, `rseed`, `nrandom`.

#### Example

```session
/> lib random
/> 42. rseed  6 random . 6 random .
1 1
```

## rseed

Random numbers; in the `random` library (`lib random`).

#### Stack

```text
rseed   ( ud -- )
```

#### Description

Starts the generator from a number of the program's choosing, a double. The same seed gives the same series
again, for testing. (A seed of 0 is taken as 1.)

#### See also

`rand`, `random`.

#### Example

```session
/> lib random
/> 42. rseed rand u.
17704
/> 42. rseed rand u.
17704
```

## tbit

Bits; in the `bits` library (`lib bits`).

#### Stack

```text
tbit   ( n b -- n flag )
```

#### Description

Tests one bit of a cell: the flag is true if bit *b* of *n* is 1. Bit 0 is the lowest and bit 15 the
highest. *n* stays on the stack, under the flag, so several bits can be tested in turn.

Only the low four bits of *b* count: bit 16 is bit 0.

#### See also

`sbit`, `and`, `nbit?`.

#### Example

```session
/> lib bits
/> 5 0 tbit . drop
-1
/> 5 1 tbit . drop
0
```

## sbit, cbit

Bits; in the `bits` library (`lib bits`).

#### Stack

```text
sbit   ( n b -- n' )
cbit   ( n b -- n' )
```

#### Description

`sbit` leaves *n* with its bit *b* set to 1, and `cbit` with it cleared to 0. The other bits are as they
were.

#### See also

`tbit`, `or`, `and`, `lshift`.

#### Example

```session
/> lib bits
/> 0 3 sbit .
8
/> 15 0 cbit .
14
```

## disasm

The disassembler; in the `disasm` library (`lib disasm`).

#### Stack

```text
disasm   ( addr u -- )
```

#### Description

Disassembles machine code: shows *u* instructions, starting at *addr*, a line each, with the instruction's
address, its bytes, and the instruction as the assembler `as` would write it. A call or a jump to a Forth
word has the word's name after it.

While the library is loaded, `see` uses it too: a word of the ROM that is written in assembly language is
shown as its instructions, between `code` and `end-code`.

The address of a word's code is its execution token, from `'`. For a colon definition whose code is in a
code bank, the token's address holds only the call that leads there.

#### See also

`see`, `dump`, `sys`; the commands `dis` and `db`; Chapter 28.

#### Example

```session
/> lib disasm
/> see dup
code dup
 A299  CA        dex
 A29A  B5 23     lda $23,x
 A29C  95 22     sta $22,x
 A29E  B5 43     lda $43,x
 A2A0  95 42     sta $42,x
 A2A2  60        rts
end-code
```
