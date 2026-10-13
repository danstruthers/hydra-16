# Chapter 17. The Other Devices

This chapter covers the rest of the Hydra's devices: the I/O header and its pins, the I²C bus, the SPI devices, the
clock, and the kernel's own devices, which show the tasks, the environment, the shared memory and the ROM's modules
as files.

> **IN THIS CHAPTER:** You'll read switches and light LEDs on the I/O header, talk to I²C and SPI chips, set the
> clock, and look at the system through the kernel's own files: the tasks, the environment, shared memory and the
> ROM's modules. **You need:** Chapter 12; and for sections 17.1 to 17.4 on a real Hydra, a few parts (an LED, some
> resistors and wire).

## 17.1 The I/O header, J27

J27 brings eight of the VIA's pins out to a header, for your own experiments: switches, lights, sensors and small
circuits. They're the VIA's **port A**, PA0 to PA7, and its two **handshake** lines, CA1 and CA2.

| Pin | Name | Pin | Name |
|---|---|---|---|
| 1 | Ground | 2 | +5 V |
| 3 | PA0, and the I²C bus's SCL | 4 | PA1, and the I²C bus's SDA |
| 5 | PA2 | 6 | PA3 |
| 7 | PA4 | 8 | PA5 |
| 9 | PA6 | 10 | PA7 |
| 11 | CA1 (input only) | 12 | CA2 |

Each pin is protected by J28, an SP720 chip, which stops sparks of static and voltages beyond the supply's from
reaching the VIA. Even so, keep everything you connect between 0 and 5 volts.

> **CAUTION:** A pin can only give out a tiny current: enough for another chip's input, but not enough to light an
> LED properly. To light an LED, connect it the other way round: from +5 V, through a 2.2K resistor and the LED, to
> the pin. Then a 0 on the pin lights it. For anything bigger, like a motor or a relay, use a transistor.

PA0 and PA1 are also the **I²C bus** (section 17.3): while a program is using the I²C bus, they're the bus's, and
`/dev/gpio`'s commands for them don't work.

## 17.2 /dev/gpio: the pins

The `gpio` driver, in task B, serves the pins as `#g`, at `/dev/gpio`:

| File | What it is |
|---|---|
| `0` ... `7` | A pin: reads as `0` or `1`; writing `0` or `1` sets it (and makes it an output) |
| `port` | All eight pins as one byte: bit *n* is pin *n*. Writing a byte sets the outputs (the inputs stay inputs) |
| `ctl` | The pins' directions, and CA1 and CA2 |
| `ca1` | A read waits for a change on CA1, then gives the number of changes so far |

Each pin is an **input** (the Hydra reads it) or an **output** (the Hydra sets it). All are inputs as the Hydra
starts.

| `ctl` command | What it does |
|---|---|
| `in` *n* | Make pin *n* an input |
| `out` *n* | Make pin *n* an output |
| `ddr` *n* | Set all eight at once: bit *k* of *n* is 1 for an output, 0 for an input |
| `ca1 rise` \| `fall` | Count CA1's rises (0 to 1) or falls (1 to 0) |
| `ca2 0` \| `1` | Make CA2 an output, 0 or 1 |
| `ca2 in` | Make CA2 an input |

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

Reading `ctl` gives a line for each pin (its number, its direction, and its level), then CA1's edge and how many it
has counted, then CA2's setting. An input with nothing connected reads 1.

**Waiting for a button.** CA1 is an input that the VIA watches for you. Connect a push button from CA1 (pin 11) to
ground (pin 1), and a 10K resistor from CA1 to +5 V (pin 2). Then each press makes CA1 fall, and a read of
`/dev/gpio/ca1` waits for the next press:

```text
/> cat /dev/gpio/ca1
1
```

(Press <kbd>CTRL</kbd>-<kbd>C</kbd> to stop it.) A button's contacts bounce when it's pressed, so one press may count
as several.

## 17.3 /dev/i2c: the I²C bus

**I²C** (say "eye-squared-see") is a way of connecting small chips with just two wires, a clock (SCL) and data
(SDA), as well as power and ground. Each chip on the bus has an **address**, a number from $08 to $77, so many chips
can share the two wires. Memory chips, clocks, temperature sensors, small displays and the Vera X's keyboard
controller all use it. The Hydra's I²C bus is on J27: SCL on pin 3, SDA on pin 4, with the pull-up resistors already
on the board.

The `gpio` driver serves the bus as `#i`, at `/dev/i2c`. Its directory lists a file for each chip that answers,
named by its address in hex, and `ctl`:

<!-- session --smc
-->

```session
/> ls /dev/i2c
ctl
42
```

(Here, `42` is the Vera X's keyboard controller.) Writing to a chip's file sends it bytes; reading reads bytes from
it: up to 64 at a time. Many chips have **registers**, numbered places inside them; with `ctl`'s `subaddress`, the
file's offset is the register to read or write.

| `ctl` command | What it does |
|---|---|
| `speed` *n* | The bus's speed, in kilohertz (1 to 100; the Hydra can only go about 40) |
| `subaddress 0` | Just bytes |
| `subaddress 1` | The file's offset is a one-byte register number, sent first (most chips) |
| `subaddress 2` | A two-byte register number, high byte first (big memory chips) |

A chip that doesn't answer gives `i/o error`.

## 17.4 /dev/spi: the SPI devices

The storage driver serves the SPI bus itself as `#S`, at `/dev/spi`: a directory for each of the sixteen SPI devices
(0 to 7 on the headers J18 to J25, section 14.2; 8 to f on the slots' cards), each with two files:

| File | What it is |
|---|---|
| `data` | Writing sends bytes to the device, and keeps what it sends back; reading gets those bytes. With none kept, a read clocks in new bytes (sending $FF): up to 256 a read |
| `ctl` | `mode 0` (the clock rests low) or `mode 3` (it rests high); both send and read on the clock's rise |

Each write is one **transaction**: the device is selected, the bytes go back and forth, and it's let go. One program
at a time can have a device's `data` open, and not while it's an SD card the storage driver is using.

> **NOTE:** Don't `cat` a `data` file: it never ends, as a read always clocks in more bytes.

## 17.5 The clock

The kernel counts **ticks**, 200 a second, and keeps the time. Three files, from `#t`, show them:

| File | What it is |
|---|---|
| `/dev/ticks` | The ticks since the Hydra started (counting up to 65,535, then from 0 again) |
| `/dev/time` | The date and time, as `2026-10-12 09:41:00`; write one in the same form to set the clock |
| `/dev/rtc` | Whether there's a DS1747 clock chip: `running`, `stopped` (it needs setting), or `none` (with `battery low` if its battery is running down) |

A **DS1747**, fitted in U7's socket in place of the task RAM chip, is task RAM and a clock in one, with a battery that
keeps the clock running while the Hydra is off. With one, the Hydra sets its clock from the chip as it starts, and
setting `/dev/time` sets the chip too.

## 17.6 null, zero and kmesg

| File | What it is |
|---|---|
| `/dev/null` | Swallows whatever is written to it; reads as empty |
| `/dev/zero` | Reads as zeros, for ever; swallows writes |
| `/dev/kmesg` | The kernel's messages, from the start: the boot's, and anything else the kernel has had to say (section 7.4) |

## 17.7 /env: the environment

Each task has an **environment**: a set of variables, which programs can read. A task starts with a copy of its
parent's, so it's how a shell passes settings like `$path` to the programs it runs. `#e`, at `/env`, shows the
environment of the task that's looking, a file for each variable:

```session
/> ls /env
prompt
path
task
```

A variable is a list of words, with a zero byte after each one:

```session
/> cat /env/path | xd
0000000  2e 00 2f 62 69 6e 00                             ../bin.
```

Writing a file sets the variable, and making a file makes a new variable, in the writer's own environment. A
program that sets a variable doesn't change its parent's.

## 17.8 /proc: the tasks

`#p`, at `/proc`, has a directory for each task in use, named by its number:

```session
/> ls /proc
0/
1/
2/
3/
4/
11/
12/
13/
14/
15/
```

In each are these files:

| File | What it is |
|---|---|
| `status` | One line: the task's name, state, parent, processor time (in ticks) and note group |
| `ctl` | Commands for the task (below) |
| `args` | Its program's arguments |
| `cwd` | Its current directory |
| `fd` | Its current directory, then its open files, a line each: the number, how it's open (`r`, `w`, `rw`), the device, the offset, and the name |
| `ns` | Its namespace, as `ns` shows it |
| `env` | Its environment |
| `regs` | The processor's registers, as it was when it last stopped running |
| `mem` | The 64K of memory it sees: its task RAM, and its RAM and ROM banks as it has them chosen |
| `ram` | Its RAM banks |
| `note` | Writing a note's name (`interrupt`, `kill`, `hangup`, `alarm`) or number sends it the note |

```session
/> cat /proc/2/status
forth wait 1 66 2
/> cat /proc/2/fd
/
0 rw #c 391 #c/cons
1 rw #c 391 #c/cons
2 rw #c 391 #c/cons
/> cat /proc/2/regs
PC=EA8F A=00 X=04 Y=02 S=E7 P=37 W=00 U=00 RAM=00 ROM=1E
```

| `ctl` command | What it does |
|---|---|
| `kill` | End the task (as `kill`) |
| `interrupt` | Interrupt it (as <kbd>CTRL</kbd>-<kbd>C</kbd>) |
| `note` *n* | Send it note *n* |
| `stop` | Stop it: it isn't given any more turns until it's started again |
| `start` | Start it again |
| `step`, `next`, `break`, `nobreak` | For the debugger, `db` (Chapter 29) |

`ps` and `top` read `/proc`; so does the debugger, which reads and writes a program's memory through `mem`.

## 17.9 /dev/seg: shared memory segments

The shared RAM can be divided into **segments**: pieces that several tasks can use together. A program makes one with
a system call, and gets its number. `#s`, at `/dev/seg`, lets a segment have a **name**, so other programs can find
it, and keeps it alive as long as it's named.

| `ctl` command | What it does |
|---|---|
| `name` *name n* | Give segment *n* a name |
| `free` *name* | Take the name away |

Each name is a file, reading as the segment's number and size (in 8K banks):

```session
/> echo name disk-s 15 >/dev/seg/ctl
/> ls /dev/seg
ctl
disk-s
/> cat /dev/seg/disk-s
15 64
```

(Segment 15 is the shared RAM disk: 64 banks, 512K.)

## 17.10 /dev/mod: the ROM's modules

`#m`, at `/dev/mod`, has a file for each module in the paged ROM, holding its program, and a directory, `bin`, of
the modules that are programs (which the namespace adds to `/bin`). `mods` lists them in more detail (Chapter 11).

## 17.11 Pipes, and raw memory

`#|` makes **pipes**: each time it's opened, a new pipe, 512 bytes long, whose two ends are two files. What's written
to one end can be read from the other. The shell makes one for each `|` in a command line. There can be 8 at once.

`#r` is the whole of the Hydra's RAM, for init alone: `task` is all sixteen tasks' 32K, one after another, and `shared`
is the shared RAM. It isn't in the namespace.
