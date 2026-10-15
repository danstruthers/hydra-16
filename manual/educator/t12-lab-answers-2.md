# Topic 12. Labs 10 to 15

## Lab 10. Devices Are Files

**1.** Any four, matched to their drivers by the table in section 12.3. For example: `cons`, `consctl`, `wctl` and
`label` are the console's; `snd`, `sndctl`, `psg` and `bell` the sound driver's; `ticks`, `time`, `rtc`, `null`,
`zero` and `kmesg` the kernel's own; and the directories `gpio`, `i2c`, `spi`, `sd` and `vid` are the pins, the
I²C bus, the SPI devices, the disks and the Vera X's screen.

**2.**

```session
/> cat /dev/serctl
b9600
x1
pc 0 req, 0 cut short, 0 unanswered, 0 damaged, 0 nak, 0 gone
line 0 overruns, 0 framing errors, 0 dropped
/> cat /dev/consctl
rawoff
keys hydra
scroll smooth
group 1
window 0
size 80 24
terminal serial
/> cat /dev/sd/0/ctl
sdhc 17 MB 34816 blocks
hydrafs label=HYDRA-16
partition at block 2048
free 15624 KB of 16380 KB
/> cat /dev/time
2000-01-01 00:00:07
```

**3.** `echo` said it, because it was `echo` whose write failed; but the *reason*, `invalid argument`, came back
from the driver, which didn't know the command `fly`.

```session
/> echo fly >/dev/gpio/ctl
echo: write error: invalid argument
```

**4.** `1`, and then `0`.

```session
/> echo out 2 >/dev/gpio/ctl
/> echo 1 >/dev/gpio/2
/> cat /dev/gpio/2
1
/> echo 0 >/dev/gpio/2
/> cat /dev/gpio/2
0
```

**5.** The words `red`, `green` and `blue`, each in its own colour. `ESC [0m` puts the colours and styles back to
normal. Without it, everything printed afterwards (the prompt, and what's typed) stays blue.

**6.**

| The job | The sequence |
|---|---|
| Clear the whole screen | `ESC [2J` (and `ESC [H` to put the cursor at the top left) |
| Row 5, column 10 | `ESC [5;10H` |
| Bright green text | `ESC [92m` |
| Reversed text | `ESC [7m` |

**Questions.**

1. It knows how to work a device's chips, and offers the device to every other program as a few files. If each
   program talked to the chips itself, each would need to know every chip, and two programs using a device at
   once would trample each other. The driver is the one place that knows, and it takes turns.
2. Reading it shows the device's settings; writing a line of text to it gives the device a command.
3. For example: `cat /dev/time` reads the clock with the program made for showing files; `echo` sets a pin, or
   the serial port's speed; `ls` shows which disks there are.
4. `invalid argument`: the driver didn't understand the command, or a number was out of range. `busy`: someone
   else is using the device. `no such device`: it isn't there (no Vera X, no card).
5. The terminal program on the PC has to be set to the same speed. The two ends of a serial line share no
   clock: each just counts time from the start of every character, so they must agree in advance how long a
   bit lasts.

**Challenge.** In HyForth, with the `esc` from step 5:

```text
: banner ( -- ) esc ." [2J" esc ." [12;34H" esc ." [1;33;44m  Sam  " esc ." [0m" cr ;
```

To make it flash, add `5;` to the colours (`[5;1;33;44m`). For the box, `ESC (0` switches to the line-drawing
characters and `ESC (B` back; in between, `l` `q` `k` draw the top (┌─┐), `x` the sides (│) and `m` `q` `j` the
bottom (└─┘):

```text
: line ( n -- ) 0 do [char] q emit loop ;
: boxed ( -- ) esc ." [2J" esc ." (0"
  esc ." [11;32H" [char] l emit 7 line [char] k emit
  esc ." [12;32H" [char] x emit esc ." (B" esc ." [5;1;33;44m  Sam  " esc ." [0m" esc ." (0" [char] x emit
  esc ." [13;32H" [char] m emit 7 line [char] j emit esc ." (B" cr ;
```

## Lab 11. Disks

**1.**

```session
/> ls /dev/sd
0/
x/
r/
s/
/> cat /dev/sd/0/ctl
sdhc 17 MB 34816 blocks
hydrafs label=HYDRA-16
partition at block 2048
free 15624 KB of 16380 KB
/> cat /dev/sd/x/ctl
rom 4 MB 8192 blocks
hydrafs label=ROM
partition at block 1056
free 2 KB of 671 KB
/> cat /dev/sd/r/ctl
ram 256 KB 512 blocks
hydrafs label=RAM
free 253 KB of 255 KB
banks $00-$1f
/> cat /dev/sd/s/ctl
sram 512 KB 1024 blocks
hydrafs label=SRAM
free 510 KB of 511 KB
segment 15
```

| Disk | What kind | Its size | Seen at |
|---|---|---|---|
| `0` | An SD card (`sdhc`) | 17 MB, on the manual's Hydra | `/sd/0` |
| `x` | The ROM disk | 671 KB of files, in the 4 MB of paged ROM | `/rom` |
| `r` | A RAM disk, in RAM banks | 256 KB | `/ram` |
| `s` | A RAM disk, in a shared segment | 512 KB | `/sram` |

**2.** `read-only`. The ROM disk is in ROM chips, which a running Hydra can't write.

```session
/> echo hello >/rom/test
rc: /rom/test: read-only
```

**3.** Two bytes: the `x`, and the new line. But the free space fell by 4 KB. A disk hands out space in whole
**clusters**, and on a card a cluster is 4K: the smallest file takes one, and a file of 4,097 bytes takes two.

```session
/> df
disk  kind   size        free        label
0     sdhc   16380 KB    15624 KB    HYDRA-16
x     rom    671 KB      2 KB        ROM
r     ram    255 KB      253 KB      RAM
s     sram   511 KB      510 KB      SRAM
/> echo x >/sd/0/tiny
/> ls -l /sd/0/tiny
--rw-rw-rw- f0        2 2000-01-01 00:00 /sd/0/tiny
/> df
disk  kind   size        free        label
0     sdhc   16380 KB    15620 KB    HYDRA-16
x     rom    671 KB      2 KB        ROM
r     ram    255 KB      253 KB      RAM
s     sram   511 KB      510 KB      SRAM
```

**4.** No faults: nothing lost, nothing unmarked, nothing used twice.

```session
/> fsck s
hydrafs label=SRAM
free 510 KB of 511 KB
check: lost 0, unmarked 0, twice 0
segment 15
```

**5.** In kilobytes:

```session
/> du /sd/0/sample
14      /sd/0/sample/as
212     /sd/0/sample/c
232     /sd/0/sample
/> du /rom/bin
223     /rom/bin
```

**6.** `HYDRA-16`, on the sample card.

**Questions.**

1. SD cards: big, kept when the power's off, and they can be carried to a PC; the place for anything that
   matters. RAM disks: very fast, and emptied at every start; a scratch pad. The ROM disk: HydraOS's own files,
   always there and never changed.
2. A block is 512 bytes: a disk is read and written a block at a time. A cluster is the unit that space is
   handed out in: 8 blocks (4K) on a card, 2 blocks (1K) on the RAM disks and the ROM disk.
3. An SD card module: a small board with a voltage regulator and a level shifter.
4. A disk is only a long row of blocks. A file system is the agreement about where, in that row, the names, the
   lengths and the contents go. A new card must be given an empty HydraFS first: `mkfs 0`.
5. Any two of: `/pc`, with the PC tool; `xmodem`; taking the card to the PC and using `hydrafs.js` on it or on
   its image; and, on the emulator, a card image or `--pc-dir`.

**Challenge.** The shared RAM disk, `s`, can be stopped and started again at another size. `start` takes a number
of 8K banks, so 16 makes a disk of 128 KB, and 64 puts back the usual 512 KB. Its memory is a shared segment:
`free` shows the segments' share fall to nothing at `stop`, and come back at `start`.

```session
/> echo stop >/dev/sd/s/ctl
/> free
ram     384 KB a task (3 modules)
shared  1024 KB, 0 KB in segments (0), 1024 KB free
/> echo start 16 >/dev/sd/s/ctl
/> df
disk  kind   size        free        label
0     sdhc   16380 KB    15624 KB    HYDRA-16
x     rom    671 KB      2 KB        ROM
r     ram    255 KB      253 KB      RAM
s     sram   127 KB      127 KB      SRAM
/> free
ram     384 KB a task (3 modules)
shared  1024 KB, 128 KB in segments (1), 896 KB free
```

## Lab 12. Sound

**1.** `patch 0 0` gave channel 0 the first instrument, a piano. `note 0 60` started note 60, middle C, on channel
0. `off 0` let it go. The two numbers after `note` are the channel and the note.

**2.** D: 62. The A of 440 Hz: 69. The C above: 72. The C below: 48.

**3.** Any five. Section 15.4 has the list; the numbers are General MIDI's, counted from 0.

**4.** `o4` chooses octave 4, where `c` is middle C. `l8` makes notes eighth notes unless they say otherwise. `>`
goes up an octave, so the last `c` is the C above. The third line played three notes together, C, E and G, one on
each of channels 0, 1 and 2: a chord of C major.

**6.** `A` and `B` are the channels the lines belong to: channel 0 and channel 1. `#tempo` is the speed, in
quarter notes a minute.

**Questions.**

1. Play eight notes at once: chords, or several instruments together.
2. A patch is an instrument: a set of settings for the chip that makes one kind of sound. A channel is one of
   the chip's voices: it plays one note at a time, with one patch.
3. 440 vibrations a second. 880 Hz is the A an octave higher: doubling the frequency goes up exactly one
   octave. (Note 81.)
4. A ZSM file is the list of values to write to the sound chips' registers, and when: all the working-out
   already done. A score is for people to write and read; a ZSM file is for the Hydra to play, quickly, and it's
   what other music programs can produce. `play -o` turns the first into the second.
5. Faster than real time, the Hydra's clock runs fast, so every note is short and the tune races.

**Challenge.** For example, a bass drum on every beat, on a third channel:

```text
C x36 x36 x36 x36 | x36 x36 x36 x36 | x36 x36 x36 x36 | x36 x36 x36 x36
```

The ZSM file is the bigger of the two: it spells out every register write that the score only describes.

## Lab 13. Pictures

**1.** The console: the same text as the serial terminal, in 80 columns and 60 rows, with a bar at the top (the
groups, and the time) and the window's heading. It's different in being a screen of the Hydra's own, with
colours and a font the Hydra chooses.

**2.** On a Hydra with a Vera X:

```text
vera 47.0.2
mode 80x60
cursor blink
border 0
bitmap off
output vga
```

For example, `mode 80x60` is the size of the text, and `bitmap off` says there's no picture under it yet.

**4.** (0, 0) is the top left corner. (319, 239) is the bottom right. *y* goes *down* the screen.

**5.** One house, of many:

```text
clear 0
pen 14
box 100 120 220 200
line 100 120 160 70
line 160 70 220 120
pen 9
bar 150 160 170 200
pen 11
disc 190 145 10
```

**6.** `draw` takes one command on each line it's sent, and doesn't care who sends it. `cat` just copies the
file's lines to the device, exactly as `echo` sent them one at a time.

**Questions.**

1. A screen for a monitor; sprites; more sound (the PSG's sixteen voices, and recorded sound); an SD card
   socket; and, with its controller, a keyboard and a mouse.
2. One dot of the picture, with a colour of its own. 320 × 240 = 76,800.
3. 256. Four bits: 16.
4. `box` and `circle` draw outlines; `bar` and `disc` are filled.
5. A drawing that's text can be kept in a file, changed with the editor, made by a script or by a program in
   any language, and sent from somewhere else. None of that needs a drawing program to exist.

**Challenge.** One way. `calc`'s sum is quoted, to keep the `*` from the shell.

```text
#!/bin/rc
echo bitmap 320 8 >/dev/vid/ctl
echo clear 0 >/dev/vid/draw
for(i in 1 2 3 4 5 6 7 8 9 10) {
	echo pen $i >/dev/vid/draw
	echo circle 160 120 `{calc $i '*' 10} >/dev/vid/draw
}
```

## Lab 14. Pins, and the System's Own Files

**1.** A line for each of the eight pins: its number, whether it's an input or an output, and its level; then
the two handshake lines, CA1 and CA2.

```session
/> echo out 2 >/dev/gpio/ctl
/> echo 1 >/dev/gpio/2
/> cat /dev/gpio/2
1
/> echo 0 >/dev/gpio/2
/> cat /dev/gpio/ctl
0 in 1
1 in 1
2 out 0
3 in 1
4 in 1
5 in 1
6 in 1
7 in 1
ca1 fall 0
ca2 in
```

**2.** PA2 is pin 5 of J27. +5 V is pin 2, and ground is pin 1.

**3.** From pin 2 (+5 V), through the resistor, into the LED's longer leg; out of its shorter leg, to pin 5 (PA2).

**4.** Writing 0 lights it. Current flows from +5 V, through the LED, *into* the pin, so the pin has to be low. It's
wired this way round because a VIA pin can take in much more current than it can give out.

**5.** <kbd>CTRL</kbd>-<kbd>C</kbd> stops it. A smaller number after each `sleep` makes it faster.

**6.** On the manual's Hydra:

```session
/> cat /proc/1/status
forth wait - 98 1
/> cat /proc/1/fd
/
0 rw #c 367 #c/cons
1 rw #c 367 #c/cons
2 rw #c 367 #c/cons
```

`status` is the task's name, its state, its parent, the processor time it's had (in ticks), and its note
group. `fd` starts with its current directory, and then lists its open files: for a shell, the window's
keyboard and screen, as files 0, 1 and 2.

**7.**

```session
/> ls /env
prompt
path
task
prog
/> cat /env/path
./bin/>
```

(`cat` printed `/env/path`'s value with the next prompt stuck to its end: the value has no new line of its own, and
`cat /env/path; echo` adds one.) `/env/prompt` holds the two prompts: the usual one, and the one shown when a
command goes on to another line.

**8.** `none`, on a Hydra with no clock chip: it will have forgotten by tomorrow, and start again at the year
2000.

```session
/> echo 2026-10-12 09:41:00 >/dev/time
/> date
2026-10-12 09:41:01
/> cat /dev/rtc
none
```

**Questions.**

1. An input is read by the Hydra; an output is set by it. A switch or a sensor goes to an input; an LED or (through
   a transistor) a motor to an output.
2. To limit the current. Without one, far too much flows, and the LED or the pin is damaged.
3. They're the I²C bus's two wires, SCL and SDA. While a program is using the bus, the pin commands for them don't
   work; and anything wired to them as ordinary pins would upset the bus.
4. The kernel makes them up, from what it knows about each task, at the moment they're read. `ps` reads them;
   and so does `db`, the debugger, which is an ordinary program that works entirely through `/proc`.
5. One set of tools works on everything. The commands for files (`cat`, `echo`, `ls`) and every language's file
   words can reach every device, with nothing special to learn for each, and a new device needs no new commands.

**Challenge.** One way:

```text
% echo out 2 >/dev/gpio/ctl; echo in 3 >/dev/gpio/ctl
% while(~ 1 1) { if(~ `{cat /dev/gpio/3} 0) echo 0 >/dev/gpio/2; if not echo 1 >/dev/gpio/2 }
```

Without a real Hydra, the task names:

```session
% for(t in /proc/*) cat $t/status
kernel ready - 458 0
forth wait - 97 1
snd idle - 5 12
kdev ready - 41 13
storage idle - 302 14
cons event - 105 15
rc wait 1 31 1
```

## Lab 15. Numbers and Bases

**1.**

```session
/> calc '2^10'
1024
/> calc '2^64'
18446744073709551616
/> calc '10 / 4'
5/2
/> calc '1/3 + 1/6'
1/2
/> calc '0.1 + 0.2'
0.3
/> calc '1/3 * 3'
1
/> calc 'sqrt 16'
4
/> calc 'sqrt 2'
1.41421356237
/> calc 'sqrt(-4)'
2i
/> calc 'abs(3-4i)'
5
/> calc 'sqrt(3+4i)'
2+1i
```

The last two surprise most people. `abs` of a complex number is its distance from 0 (question 6); and `2+1i`
squared is 4 + 4i + i², which is 3 + 4i, because i² is -1.

**2.** It keeps fractions as fractions, exactly, instead of turning them into decimals that have to be cut off
somewhere; and it keeps decimals in base ten, not in binary, so 0.1 really is a tenth.

**3.**

```session
/> calc -d 30 'sqrt 2'
1.41421356237309504880168872421
```

**4.**

| Decimal | Binary | Hex | Octal |
|---|---|---|---|
| 10 | 1010 | A | 12 |
| 100 | 1100100 | 64 | 144 |
| 255 | 11111111 | FF | 377 |
| 1000 | 1111101000 | 3E8 | 1750 |

**5.**

```session
/> calc '#b1010'
10
/> calc '#xFF'
255
/> calc '#b1010 + #x0F'
25
/> calc -b x '#xFF + 1'
100
```

**6.** Base 3 (ternary), with the digits 0, 1 and 2; and base 36, which uses all ten digits and all 26 letters.

```session
/> calc -b t 10
101
/> calc -b z 1000000
LFLS
```

**Questions.**

1. Whole numbers of any size (`2^64`); fractions (`1/3`); decimals (`0.1`); and complex numbers (`2i`).
2. The Hydra keeps the third as the fraction 1/3, and three of those are exactly 1. A calculator keeps
   0.33333333, which is a little less than a third, and three of those are 0.99999999.
3. Eight digits, 0 to 7. The biggest is 7.
4. `A` is `1010` and `5` is `0101`, so $A5 is `1010 0101`.
5. `division by zero`, or words to that effect. A program should check before it divides, or catch the error
   (each language's chapter shows how).

```session
/> calc 1/0
calc: division by zero
```

6. 5 squares. The line from 0 to the point is the long side of a right-angled triangle whose other sides are 3 and
   4, and Pythagoras says its square is 3² + 4² = 25: so it's 5. That's what `abs` works out for any complex
   number.

**Challenge.** 302 digits. `wc -c` says one more, because it counts the new line at the end.

```session
/> calc '2^1000' | wc -c
    303
```

In balanced ternary, written least digit first: 5 is `--+` (−1 − 3 + 9), 6 is `0-+` (0 − 3 + 9), and 7 is `+-+`
(1 − 3 + 9).

```session
/> calc -b c 5
--+
/> calc -b c 6
0-+
/> calc -b c 7
+-+
```
