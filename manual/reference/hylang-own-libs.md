## gpio library

A library, loaded with `(use "gpio")`: the eight pins of the I/O header J27, and its two handshake lines.

#### Functions

| Form | What it does |
|---|---|
| `(gpio pin)` | The pin's level now: 0 or 1 |
| `(gpio! pin level)` | Makes the pin an output and sets it: low for a *level* of 0, high for anything else. `NIL` |
| `(gpio-in pin)`, `(gpio-out pin)` | Makes the pin an input, or an output. `NIL` |
| `(gpio-port)` | All eight pins' levels as one byte: bit *n* is pin *n* |
| `(gpio-port! byte)` | Sets all the outputs at once from a byte. Pins that are inputs stay inputs. `NIL` |
| `(gpio-ddr! byte)` | Sets all eight directions at once: bit *n* is 1 for an output, 0 for an input. `NIL` |
| `(gpio-ca1! edge)` | Chooses the change of CA1 that is counted: `:rise` (0 to 1) or `:fall` (1 to 0). `NIL` |
| `(gpio-ca2! v)` | CA2: 0 makes it an output that is low, 1 an output that is high, and `:in` an input. `NIL` |
| `(gpio-wait)` | Waits for CA1's next counted change, and gives the number of changes counted so far |
| `(gpio-state)` | The state of everything, as a hash |

#### Arguments

*pin*
: 0 to 7: PA0 to PA7 of the VIA, on J27's pins 3 to 10.

#### Description

The library works through the files of `/dev/gpio` (section 17.2 has the pins, the cautions about what may
be connected to them, and the files themselves). Every pin is an input when the Hydra starts, and an input
with nothing connected reads 1.

Pins 0 and 1 are also the I²C bus: while a program is using the bus, they are not the pins' to set.

`gpio-state` gives a hash with a key for each pin, 0 to 7, whose value is a list of its direction and its
level, such as `{:out 1}`; `:ca1`, a list of the edge counted and the count, such as `{:fall 0}`; and `:ca2`,
its level, or `:in`.

`gpio-wait` does not return until the line changes: <kbd>CTRL</kbd>-<kbd>C</kbd> ends the wait.

#### Errors

| Message | Cause |
|---|---|
| `/dev/gpio/8: not found` | A pin that is not 0 to 7, given to `gpio` or `gpio!` |
| `invalid argument` | A pin that is not 0 to 7, given to `gpio-in` or `gpio-out`; a byte that is not 0 to 255 |
| `'gpio-ca1!' expects :rise or :fall` | Another value for the edge |

#### See also

Section 17.2; the `dev` library; `sleep`.

#### Example

```session
hylang> (use "gpio")
=> NIL
hylang> (gpio 2)
=> 1
hylang> (gpio! 2 0)
=> NIL
hylang> (gpio 2)
=> 0
hylang> ((gpio-state) 2)
=> {:out 0}
hylang> (gpio-in 2)
=> NIL
hylang> (dotimes {i 3} (do (gpio! 3 0) (sleep 0.1) (gpio! 3 1) (sleep 0.1)))
=> NIL
hylang> (gpio-in 3)
=> NIL
```

## i2c library

A library, loaded with `(use "i2c")`: the I²C bus on the I/O header.

#### Functions

| Form | What it does |
|---|---|
| `(i2c-devices)` | A list of the addresses of the chips that answer on the bus |
| `(i2c-read addr n [reg])` | Reads *n* bytes, 64 at most, from the chip at *addr*: a list. With *reg*, from that register on |
| `(i2c-write addr bytes [reg])` | Sends bytes, a list or a string's, to the chip. With *reg*, to that register. `NIL` |
| `(i2c-speed khz)` | Sets the bus's speed, in kilohertz: 1 to 100. `NIL` |
| `(i2c-reg-size n)` | Says how a register's number is sent: 0, not at all (plain bytes); 1, as one byte; 2, as two, high byte first. `NIL` |

#### Arguments

*addr*
: The chip's address on the bus, a number: `#x08` to `#x77`.

*reg*
: A register inside the chip: used only after `(i2c-reg-size 1)` or `2`, and 0 if it isn't given.

#### Description

The library works through the files of `/dev/i2c` (section 17.3). Each read or write is one transaction on
the bus. With a register size set, the register's number is sent first, and a read follows it with a
repeated start, which is what most chips with registers expect.

The settings are the bus's, not the program's: they stay as they are left.

#### Errors

| Message | Cause |
|---|---|
| `i/o error` | The chip did not answer: nothing is at that address |
| `invalid argument` | A speed or a register size out of range |

#### See also

Section 17.3; the `gpio` and `dev` libraries.

#### Example

With nothing on the bus:

```session
hylang> (use "i2c")
=> NIL
hylang> (i2c-devices)
=> NIL
hylang> (i2c-read #x50 2)
=> Error: i/o error
hylang> (i2c-speed 40)
=> NIL
```

## spi library

A library, loaded with `(use "spi")`: the sixteen SPI devices.

#### Functions

| Form | What it does |
|---|---|
| `(spi d bytes)` | One transaction: the device is selected, the bytes (a list, or a string's) are sent, and the device is let go. The result is a list of the bytes the device sent back meanwhile, one for each byte sent |
| `(spi-read d n)` | Clocks in *n* bytes, sending `$FF` for each: a list |
| `(spi-mode d mode)` | The clock's mode for the device: 0 (the clock rests low) or 3 (it rests high). `NIL` |

#### Arguments

*d*
: The device, 0 to 15: 0 to 7 are the board's headers J18 to J25, and 8 to 15 the slots' cards.

#### Description

The library works through the files of `/dev/spi` (section 17.4). A device with nothing connected sends back
`$FF`, 255, for every byte.

One program at a time can use a device, and a device that is an SD card the system has started can't be used
this way at all: the error is `busy`.

#### Errors

| Message | Cause |
|---|---|
| `invalid argument` | A mode that is not 0 or 3 |
| `busy` | Another program has the device, or it is a card's |
| *path*`: not found` | A device that is not 0 to 15 |

#### See also

Section 17.4; section 14.2; the `dev` library.

#### Example

With nothing connected to device 3:

```session
hylang> (use "spi")
=> NIL
hylang> (spi-mode 3 0)
=> NIL
hylang> (spi 3 {1 2 3})
=> {255 255 255}
hylang> (spi-read 3 4)
=> {255 255 255 255}
```

## clock library

A library, loaded with `(use "clock")`: setting the clock, and the clock chip.

#### Functions

| Form | What it does |
|---|---|
| `(set-date text)` | Sets the date and time, from a string in the form `"2026-10-13 09:41:00"`. With a clock chip fitted, the chip is set too. `NIL` |
| `(rtc)` | What the system found for a clock chip as it started: a list holding `:running`, `:stopped` (the chip is there and needs setting) or `:none`, and `:battery-low` as well if its battery is running down |

#### Description

Reading the clock needs no library: `time`, `date` and `date-parts` are always there. This one writes
`/dev/time` and reads `/dev/rtc` (section 17.5).

#### Errors

| Message | Cause |
|---|---|
| `invalid argument` | The text is not a date and time in that form |

#### See also

`time`, `date`, `seconds-of`; section 17.5.

#### Example

```session
hylang> (use "clock")
=> NIL
hylang> (rtc)
=> {:none}
hylang> (set-date "2026-10-13 09:41:00")
=> NIL
hylang> (substring (date) 0 16)
=> "2026-10-13 09:41"
hylang> (set-date "nonsense")
=> Error: invalid argument
```

## disk library

A library, loaded with `(use "disk")`: the disks.

#### Functions

| Form | What it does |
|---|---|
| `(disks)` | The disks that have been started: a list of hashes, each with `:disk`, its letter, a string; `:kind`, an atom such as `:sdhc`, `:rom`, `:ram` or `:sram`; `:blocks`, its size in 512-byte blocks; and `:label`, if it holds a file system |
| `(df)` | Each disk's room: a list of hashes with `:disk`, `:label`, and `:free` and `:size` in kilobytes |
| `(cards)` | The SPI devices that have an SD card: a list of numbers. Asking starts each card that is there |
| `(disk-start d)` | Starts a card again, as is needed after one is changed. `NIL` |
| `(disk-stop d)` | Stops a RAM disk, and gives its memory back. `NIL` |

#### Arguments

*d*
: A disk: its letter, as a string (`"0"` to `"f"` for the card on that SPI device, `"x"` the ROM disk, `"r"` the
  RAM disk, `"s"` the shared RAM disk); or, for a card, its SPI device's number.

#### Description

The library works through the files of `/dev/sd` (Chapter 14). `df` gives a program the numbers that the `df`
command prints.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such disk |

#### See also

Chapter 14; the commands `df`, `fsck`, `mkfs` and `label` (Chapter 2 of this book).

#### Example

```session
hylang> (use "disk")
=> NIL
hylang> (map (fn {d} {d :disk}) (disks))
=> {"0" "r" "s" "x"}
hylang> (map (fn {d} {list (d :label) (d :size)}) (df))
=> {{"HYDRA-16" 16380} {"RAM" 255} {"SRAM" 511} {"ROM" 623}}
hylang> (cards)
=> {0}
```

## proc library

A library, loaded with `(use "proc")`: a look inside any task.

#### Functions

| Form | What it does |
|---|---|
| `(task-args task)` | Its program's arguments: a list of strings |
| `(task-cwd task)` | Its current directory: a string |
| `(task-env task)` | Its environment: a hash, each variable's name, a string, and its value, a string (several words with spaces between) |
| `(task-ns task)` | Its namespace: a list of strings, the `bind` and `mount` lines that made it |
| `(task-regs task)` | The processor's registers as they were when the task last stopped running: a hash of numbers, `:pc`, `:a`, `:x`, `:y`, `:s`, `:p`, `:w`, `:u`, `:ram` and `:rom` |
| `(task-mem task addr n)` | *n* bytes of the task's memory from *addr*, as the task sees its 64K: a list |
| `(task-ram task bank offset n)` | *n* bytes of one of the task's RAM banks, from *offset*: a list |

#### Arguments

*task*
: A task's number.

#### Description

The library reads the files of `/proc` (section 17.8). With `ps` and `task-info`, which are always there, it
is what a program needs to watch another: the debugger `db` reads the same files.

The registers and the memory of the kernel's task and of the drivers can't be read.

#### Errors

| Message | Cause |
|---|---|
| *path*`: not found` | There is no such task |
| `not allowed` | `task-regs`, `task-mem` or `task-ram` of the kernel's task, or a driver's |

#### See also

`ps`, `task-info`, `pid`, `peek`; section 17.8.

#### Example

```session
hylang> (use "proc")
=> NIL
hylang> (task-args 1)
=> {"-l"}
hylang> (task-cwd (pid))
=> "/"
hylang> ((task-env (pid)) "prog")
=> "/bin/hylang"
hylang> (fst (task-ns (pid)))
=> "bind '#/' /"
hylang> (hex ((task-regs 1) :pc) 4)
=> "EAA0"
```

## pc library

A library, loaded with `(use "pc")`: is the PC's file server there?

#### Functions

| Form | What it does |
|---|---|
| `(pc?)` | `T` if the PC tool answers on the serial line, so that `/pc` can be used. If nothing answers within a second, `NIL` |

#### Description

`/pc` is a folder, or a disk image, on the PC at the other end of the serial cable, served by the PC tool
(section 9.9). Its files are ordinary files: `read-file`, `ls`, `open` and the rest work on them, and need no
library. This one function lets a program find out, before it tries, whether the tool is running.

#### See also

`exists?`, `ls`; section 9.9.

## dev library

A library, loaded with `(use "dev")`: the few functions that the device libraries are built from.

#### Functions

| Form | What it does |
|---|---|
| `(dev.read path [n])` | One read of the file: a string of at most *n* bytes, or 256 |
| `(dev.lines path)` | One read, as a list of its lines, without the empty ones |
| `(dev.num path)` | One read, as a number: the text is trimmed and given to `val` |
| `(dev.write path data)` | One write to the file, of a string or a list of bytes. `NIL` |
| `(dev.with path mode f)` | Opens the file (*mode* 0 to read, 1 to write, 2 for both), calls `(f fd)` with its file descriptor, closes the file, and gives *f*'s result |
| `(dev.bytes s)` | A string's bytes, as a list |
| `(dev.hex n w)` | A number in hex, *w* digits, in small letters: the way devices are named in their directories |

#### Description

A device's file is not like a disk's: each read is one request to the driver (a pin's level, a
transaction's bytes, a control file's text), and each write is one command. `read-file` may make several
reads and `write-file` several writes; these make exactly one, which is what a device wants. `dev.with` is
for anything longer, such as a seek and then a read, with the `sys-` functions.

Whatever goes wrong, the file is closed before the error is passed on.

These are the pieces to use for a device that has no library of its own, and the libraries themselves, in
`/lib/hylang`, are short examples of their use.

#### See also

`sys`, `read-file`, `write-file`; Chapter 12.

#### Example

```session
hylang> (use "dev")
=> NIL
hylang> (dev.read "/dev/rtc")
=> "none\n"
hylang> (dev.lines "/dev/serctl")
=> {"b9600" "x1" "pc 0 req, 0 cut short, 0 unanswered, 0 damaged, 0 nak, 0 gone" "line 0 overruns, 0 framing errors, 0 dropped"}
hylang> (dev.write "/dev/gpio/ctl" "out 2")
=> NIL
hylang> (dev.num "/dev/gpio/2")
=> 1
hylang> (dev.write "/dev/gpio/ctl" "in 2")
=> NIL
```

## cons library

A library, loaded with `(use "cons")`: the console's windows.

#### Functions

| Form | What it does |
|---|---|
| `(window)` | The number of the window hylang is running in |
| `(window-size)` | That window's size: a list, the columns and the rows |
| `(windows)` | The numbers of all the windows there are: a list |
| `(shown-window)` | The number of the window that is shown now |
| `(show-window n)` | Shows window *n*. `NIL` |
| `(new-window [line])` | Makes a window and shows it, running the command line in it; with none, a shell. It is the `new-window` command, and gives its exit code |
| `(new-group [line])` | The same, in a note group of its own (`new-window -g`) |
| `(window-label [s])` | With a string, sets this window's title (`""` gives it its program's name again), `NIL`. With none, gives the title |
| `(window-status s)` | Sets the text of this window's status line. `NIL` |
| `(window-ctl s)` | Writes any line to the window's control file, `/dev/wctl`. `NIL` |
| `(snarf)`, `(snarf! s)` | The console's cut buffer: its text; and set it |
| `(raw-on)`, `(raw-off)` | Turns raw keys on: each key comes as it is pressed, and is not shown. And off again |
| `(beep)` | Rings the bell |

#### Description

The library works through the console's files in `/dev` (Chapters 10 and 13). The cut buffer is shared by
all the windows: what one program puts there, <kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>Y</kbd> pastes into another.

`raw-on` lasts until `raw-off`: while it is on, the prompt's own line editing is off too, so a program must
turn it off before it ends. For single keys, `key` is simpler, and looks after this itself.

#### See also

`key`, the `screen` library; the command `new-window` (Chapter 2 of this book); Chapter 10.

#### Example

```session
hylang> (use "cons")
=> NIL
hylang> (window)
=> 0
hylang> (window-size)
=> {80 24}
hylang> (windows)
=> {0}
hylang> (snarf! "cut text")
=> NIL
hylang> (snarf)
=> "cut text"
```

## screen library

A library, loaded with `(use "screen")`: the terminal's cursor, colours and clearing.

#### Functions

| Form | What it does |
|---|---|
| `(cls)` | Clears the screen, and puts the cursor at its top left |
| `(at col row)` | Moves the cursor: columns and rows count from 1, from the top left |
| `(clear-line)` | Clears from the cursor to the end of its line |
| `(color fg [bg])` | Sets the colour of the text written from now on, and its background's if given: each one of the atoms `:black`, `:red`, `:green`, `:yellow`, `:blue`, `:magenta`, `:cyan`, `:white` |
| `(bold)`, `(plain)` | Bold text; and back to plain text in the usual colours |
| `(cursor-off)`, `(cursor-on)` | Hides the cursor; shows it |

#### Description

Each writes one of the terminal's control sequences to standard output (section 13.7 lists them), so they
work on the Hydra's console, in any of its windows, and on a PC's terminal alike. They act on the window
hylang is writing to.

What they set stays set after the program ends: a program that changes the colour or hides the cursor
should finish with `(plain)` and `(cursor-on)`.

#### See also

`write`, `key`, the `cons` library; section 13.7.

#### Example

A title in the middle of a cleared screen:

```text
(use "screen")
(cls)
(at 30 12)
(color :yellow :blue)
(write " The Hydra-16 ")
(plain)
(at 1 24)
```

## snd library

A library, loaded with `(use "snd")`: notes and sounds from the YM2151 and, with a Vera X, its PSG.

#### Functions

| Form | What it does |
|---|---|
| `(snd-claim ch ...)`, `(snd-release ch ...)` | Claims channels, so that no other program's sounds reach them; and gives them back |
| `(snd-patch ch n)` | Loads an instrument into the channel: one of the 163 patches, 0 to 162 |
| `(snd-note ch note)` | Starts a note: a MIDI note number, 60 being middle C |
| `(snd-off ch)` | Ends the channel's note (it dies away as its instrument does) |
| `(snd-level ch v)` | The channel's own volume: 0 to 127 |
| `(snd-pan ch pan)` | Which speakers: `:left`, `:right` or `:both` |
| `(snd-bend ch n)` | Bends the pitch, by *n* 64ths of a semitone, up or down |
| `(snd-drum ch n)` | Plays a drum: a General MIDI drum number (35 is a kick) |
| `(snd-freq ch hz)` | Starts a note at a frequency, in hertz; 0 ends it |
| `(snd-glide ch note)` | Moves the sounding note to another pitch without starting it again |
| `(snd-wave ch w [width])` | For a PSG channel: its waveform, `:pulse`, `:saw`, `:triangle` or `:noise`, and a pulse's width, 0 to 63 |
| `(snd-volume v)` | The master volume for all sound: 0 to 200, 100 being as written |
| `(snd-reset)` | Silences the chip and clears every setting |
| `(snd-lfo rate pmd amd wave)` | The YM2151's low-frequency oscillator: its rate, its depths of pitch and of loudness (0 to 127), and its wave (0 saw, 1 square, 2 triangle, 3 noise) |
| `(snd-sens ch pms ams)` | How much the oscillator moves a channel: its pitch (0 to 7) and its loudness (0 to 3) |
| `(snd-noise n)` | Noise on channel 7, at frequency 0 to 31; a negative number turns it off |
| `(snd-reg reg value ...)` | Writes the chip's own registers: a register and its value, in pairs |
| `(snd-regs)` | The chip's 256 registers as they were last written: a buffer |
| `(note-of name)` | A note's MIDI number from its name: `"C4"` is 60, `"C#4"` and `"Db4"` 61, `"A4"` 69 |
| `(tune notes [ch [tempo]])` | Plays a list of notes, each `{note beats}`, in time. A note is a name, a number, or `NIL` for a rest. The channel is 0 and the tempo 120 beats a minute if they aren't given |
| `(play path [times])` | Plays a song file (ZSM) with the `play` program: its exit code |
| `(snd-mml ch mml)`, `(snd-chord ch notes)` | Plays a line of the score language (MML) on a channel, or its notes together as a chord, with the `play` program: its exit code |

#### Arguments

*ch*
: A channel: 0 to 7 are the YM2151's eight voices, and 8 to 23, with a Vera X, the PSG's sixteen.

#### Description

The library works through the sound driver's files, `/dev/snd` and `/dev/sndctl` (Chapter 15 describes the
driver, its channels, the patches and MML). Each function gives `NIL` unless the table says otherwise.

A note sounds until it is ended: `snd-note`, a `sleep`, and `snd-off` make one note of a chosen length.
`tune` does that for a whole list, keeping time by the system's tick.

A channel a program has claimed is its own until it is released or the program ends. `play`, `snd-mml` and
`snd-chord` run another program, which needs the channel free: release it first, or they report
`channels busy`.

#### Errors

| Message | Cause |
|---|---|
| `'snd-pan' expects :left, :right or :both` | Another value for the speakers |
| `'note-of' expects a note: C4, C#4, Db4 ...` | A name that is not a note's |

#### See also

Chapter 15; section 22.6; the command `play` (Chapter 2 of this book).

#### Example

```session
hylang> (use "snd")
=> NIL
hylang> (note-of "A4")
=> 69
hylang> (snd-patch 0 5)
=> NIL
hylang> (snd-note 0 (note-of "C4"))
=> NIL
hylang> (sleep 1/2)
=> NIL
hylang> (snd-off 0)
=> NIL
hylang> (tune {{"C4" 1} {"E4" 1} {NIL 1} {"G4" 2}} 0 240)
=> NIL
```

## video library

A library, loaded with `(use "video")`: drawing on a Vera X's screen, turtle graphics, and the chip's memory,
sprites and mouse.

#### Functions

**The bitmap, and drawing on it.**

| Form | What it does |
|---|---|
| `(bitmap w d)` | Turns on a bitmap under the console's text: *w* is 320 or 640 dots across, and *d* the bits for each dot, 1, 2, 4 or 8 (640 across allows only 1 or 2) |
| `(bitmap-off)` | Turns it off |
| `(pen c)` | The colour drawn in from now on: a palette number |
| `(plot x y)` | One dot |
| `(line x0 y0 x1 y1)` | A line |
| `(box x0 y0 x1 y1)`, `(bar x0 y0 x1 y1)` | A rectangle's outline; a filled rectangle |
| `(circle x y r)`, `(disc x y r)` | A circle's outline; a filled circle |
| `(text x y s)` | Text in the console's 8 × 8 font, 40 characters at most |
| `(clear)` | Clears the bitmap |
| `(border c)` | The colour of the screen's border |

**The turtle.**

| Form | What it does |
|---|---|
| `(cs)`, `(home)` | Clears the bitmap and sends the turtle home; sends it home only: the middle of the screen, facing up |
| `(fd n)`, `(bk n)` | Forward, or back, *n* dots, drawing a line if the pen is down |
| `(rt d)`, `(lt d)` | Turns right, or left, *d* degrees |
| `(pu)`, `(pd)` | Pen up (moves draw nothing); pen down |
| `(heading)`, `(seth d)` | The turtle's heading in degrees, 0 being up and 90 right; and set it |

**The chip.**

| Form | What it does |
|---|---|
| `(vpeek addr)`, `(vpoke addr v)` | A byte of the VERA's memory, at a 17-bit address; and set one |
| `(vram@ addr n)`, `(vram! addr bytes)` | *n* bytes of it, as a list; and a list of bytes written to it |
| `(palette! index rgb)` | Sets a palette entry to a colour, `#xRGB`: four bits each of red, green and blue |
| `(sprite! n bytes)` | Sets sprite *n*'s eight attribute bytes |
| `(sprite-at n x y)`, `(sprite-off n)` | Moves a sprite; hides it |
| `(vsync)` | Waits for the next frame |
| `(mouse)`, `(mouse-wait)` | The mouse now: a list of *x*, *y* and the buttons (1 left, 2 middle, 4 right, added together). And the same after waiting for its next change |

#### Description

The library works through the video driver's files in `/dev/vid` (Chapter 16), and needs a Vera X card; the
emulator has one when it is started with `--vera`. Each function gives `NIL` unless it says otherwise.

The bitmap lies under the console's text, so the prompt, and whatever a program prints, stay on the screen
over the drawing. Positions are in dots, from the top left corner; a dot outside the bitmap is not drawn,
and is not an error. The turtle keeps its place exactly, as a fraction, so a figure of many small steps
closes where it began.

The names are the same as those of HyForth's video library, and in both it is the driver that does the
drawing.

#### Errors

| Message | Cause |
|---|---|
| `invalid argument` | Drawing with no bitmap on; a width or depth that `bitmap` doesn't have |
| `busy` | Another program has claimed the chip |
| `/dev/vid/ctl: not found` | There is no Vera X |

#### See also

Chapter 16; section 22.7; the `screen` and `cons` libraries.

#### Example

A square, a filled circle, and a turtle's hexagon:

```text
hylang> (use "video")
=> NIL
hylang> (bitmap 320 8)
=> NIL
hylang> (pen 5)
=> NIL
hylang> (box 10 10 100 100)
=> NIL
hylang> (disc 200 60 30)
=> NIL
hylang> (cs)
=> NIL
hylang> (dotimes {i 6} (do (fd 40) (rt 60)))
=> NIL
hylang> (heading)
=> 0
hylang> (bitmap-off)
=> NIL
```

## dice library

A library, loaded with `(use "dice")`: dice for games.

#### Functions

| Form | What it does |
|---|---|
| `(die sides)` | One throw of a die with that many sides: 1 to *sides* |
| `(die+ sides mod)` | A throw, with *mod* added |
| `(d3)`, `(d4)`, `(d6)`, `(d8)`, `(d10)`, `(d12)`, `(d20)`, `(d24)`, `(d30)`, `(d60)`, `(d100)` | A throw of the die named |
| `(d6+ mod)`, and the same for each of those | A throw of it, with *mod* added |
| `(dice n sides)`, `(dice+ n sides mod)` | The total of *n* throws; and with *mod* added |
| `(roll n d)` | A list of *n* throws: *d* is a Q-expression, run for each, as `repeat`'s is. `(roll 4 {d6})` |
| `(sumrolls l)` | Throws every die that the list describes, and gives the total. An item is a number of sides, or a list: `{sides}`, `{n sides}` or `{n sides mod}` |
| `(best-of-rolls a b d)`, `(worst-of-rolls a b d)` | The best, or the worst, *a* of *b* throws of *d*: a list |
| `(oe-die sides)`, `(oe-dice n sides)`, and each with `+` and a *mod* | Open-ended throws: a throw of the highest face is thrown again and added |
| `(special-die sides cases)` | A throw looked up with `case`: *cases* is a list of `{face result}` pairs |
| `(dnd-attr-rolls [n])` | Scores for a role-playing character: a list of *n* sets (one, if *n* isn't given), each set the best six of eight scores, greatest first, and each score the total of the best three of four six-sided dice |

#### Description

Every throw comes from `random`, so no two runs give the same numbers.

#### See also

`random`, `repeat`, `best-of`, `sum`.

#### Example

The numbers differ from run to run:

```text
hylang> (use "dice")
=> NIL
hylang> (d6)
=> 4
hylang> (dice 3 6)
=> 11
hylang> (roll 4 {d6})
=> {2 6 3 5}
hylang> (sum (best-of-rolls 3 4 {d6}))
=> 14
hylang> (sumrolls {6 {2 8} {1 4 3}})
=> 17
hylang> (dnd-attr-rolls)
=> {{18 16 16 14 13 12}}
```
