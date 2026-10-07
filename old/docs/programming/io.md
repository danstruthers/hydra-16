## **Input and output**

All IO on the Hydra goes through **file descriptors**, Plan 9 style:
* A task opens a name (`/dev/cons`, `/dev/sd/0/data`), gets an fd, and reads and writes it.
* Devices are **file servers**: a driver registers its names, and each request runs in the driver's task.
* A read with no data makes the task wait, using no CPU, until the driver wakes it.

This chapter is the application side.  Writing a server is in [servers.md](servers.md), and the design is in [plans/IO_PLAN.md](../../../reborn/docs/design/plans/IO_PLAN.md).  Sources: `os_rom/io/`.  Part of the [Programmer's Guide](README.md).

### **Files and fds**

Each task has **12 fds** (0-11), in its task system page.  An fd holds the device, the server's handle for the open file (the fid), the open mode, and a 32-bit offset.

| Call | In | Out |
| :--- | :- | :-- |
| `IO_OPEN` (`$F86C`) | `.A.Y` = name (zero-terminated, up to 255 characters), `.X` = mode | `.A` = fd (the lowest free one) |
| `IO_CLOSE` (`$F86F`) | `.A` = fd | (closed even if the server reports an error) |
| `IO_READ` (`$F872`) | `.A` = fd, `ZP_IO_BUF` = buffer, `ZP_IO_CNT` = bytes wanted | `ZP_IO_CNT` = bytes read (0 = end of file) |
| `IO_WRITE` (`$F875`) | `.A` = fd, `ZP_IO_BUF` = buffer, `ZP_IO_CNT` = bytes | `ZP_IO_CNT` = bytes written |
| `IO_GETC` (`$F878`) | `.X` = fd | `.A` = byte; C = 1, `ERR_IO_EOF` at the end |
| `IO_PUTC` (`$F87B`) | `.X` = fd, `.A` = byte | |
| `IO_SEEK` (`$F87E`) | `.A` = fd, `ZP_IO_OFS` = 32-bit offset | Sets the offset for the next read or write |
| `IO_STAT` (`$F881`) | `.A` = fd, `ZP_IO_BUF` = 48-byte buffer | The server's stat record ([below](#stat)) |
| `IO_CTL` (`$F884`) | `.A` = fd, `.X` = control code, `.Y` = argument | `.A` from the server |
| `IO_DUP` (`$F896`) | `.A` = fd | `.A` = a new fd for the same file |
| `IO_DUP2` (`$F890`) | `.A` = fd, `.X` = new fd | fd `.X` now refers to fd `.A`'s file (closed first if open) |
| `IO_PIPE` (`$F893`) | | `.A` = read fd, `.X` = write fd |
| `IO_CREATE` (`$F8C9`) | `.A.Y` = name, `.X` = mode, `ZP_IO_BUF` (low byte) = the new file's mode bits | `.A` = fd: a new file or directory, opened ([below](#the-files-on-a-card)) |
| `IO_REMOVE` (`$F8CC`) | `.A.Y` = name | Removes a file, or an empty directory |
| `IO_WSTAT` (`$F8CF`) | `.A` = fd, `ZP_IO_BUF` = a 48-byte stat record | Renames the file, sets its mode bits |
| `IO_CHDIR` (`$F8D2`) | `.A.Y` = a directory's path | It's the current directory ([below](#the-current-directory)) |
| `IO_GETCWD` (`$F8D5`) | `ZP_IO_BUF` = a 64-byte buffer | The current directory, a zero-terminated absolute path |

**Names** given to `IO_OPEN`, `IO_CREATE`, `IO_REMOVE` and `IO_CHDIR` are relative to the task's current directory unless they start with `/`.

**Modes** (`IO_OPEN`'s `.X`): `IO_MODE_READ` (`$01`), `IO_MODE_WRITE` (`$02`), `IO_MODE_RDWR` (`$03`), plus `IO_MODE_STAT` (`$04`), `IO_MODE_TRUNC` (`$08`) and `IO_MODE_NONBLOCK` (`$80`).  With `IO_MODE_NONBLOCK`, a read with no data yet returns `ERR_IO_WOULD_BLOCK` instead of waiting.  `IO_MODE_STAT` asks a directory for stat records instead of text, and `IO_MODE_TRUNC` (with `IO_MODE_WRITE`) empties a file as it's opened ([below](#the-files-on-a-card)).

**Reads and writes:**
* **Units:** they move up to 256 bytes per request; the IO layer splits bigger ones.
* **A read** returns when the count is done, at the end of the file, or when the server has less (the keyboard: the keys typed so far).
* **A write** returns when the count is done.  A server that takes part of it is offered the rest again.
* **The offset** advances by what was moved.
* **Buffers:** `ZP_IO_BUF` must point into task RAM (`$0000-$7FFF`), since the data is copied through shared RAM.

**Names** are resolved through the task's namespace (below).  A name nothing matches must be `/dev/<device>` or `/dev/<device>/<rest>`: the device's server gets `<rest>` to open.  Names are read as the caller sees them (through a far pointer), so a name can be in RAM, in the paged ROM, or on the caller's own BIOS page.

**Inheritance.**  A new task gets copies of its parent's open fds, and all of a task's fds are closed when it ends.

```
            lda     #<NAME              ; NAME: .byte "/dev/zero", 0
            ldy     #>NAME
            ldx     #IO_MODE_READ
            jsr     IO_OPEN
            bcs     @error
            sta     FD
            lda     #<BUF               ; 16 bytes of zeros into BUF
            sta     ZP_IO_BUF
            lda     #>BUF
            sta     ZP_IO_BUF + 1
            lda     #16
            sta     ZP_IO_CNT
            stz     ZP_IO_CNT + 1
            lda     FD
            jsr     IO_READ
            lda     FD
            jsr     IO_CLOSE
```

### **stdio**

fds **0, 1 and 2** are stdin, stdout and stderr.  The shell opens all three on `/dev/cons`, and the tasks it starts inherit them.

| Call | Does |
| :--- | :--- |
| `WRITE_CHAR` (`$F803`) | Write `.A` to fd 1 (or straight to the serial port, for a task without fd 1: the system task and drivers) |
| `GET_CHAR` (`$F88D`) | Wait for a byte from fd 0: C = 1, `.A` = the byte; C = 0 with an error (e.g. `ERR_IO_EOF`) |
| `READ_CHAR` (`$F800`) | A byte from fd 0 if there is one, without waiting: C = 1, `.A` = the byte; C = 0: none |
| `IO_FLUSH` | Write out the stdout buffer |

**Output buffering.**  `WRITE_CHAR` collects output in a 128-byte buffer (`$0700`), for every kind of fd 1.  The buffer is written out:
* when it's full;
* for the console (`/dev/cons`, `/dev/ser`), at each LF too: it's line-buffered, like a terminal in C;
* before stdin is read (`GET_CHAR`, `READ_CHAR`), so a prompt always shows;
* before the task sleeps (`TASK_SLEEP`) or starts a task;
* before any `IO_WRITE` by the task, so output through another fd comes out in order;
* when fd 1 is closed or replaced, so when the task ends.

A program that prints a partial line and then computes for a long time without any of those can call `IO_FLUSH` itself.

**The console's fast paths.**  From the foreground task, console output and input skip the IO request entirely:
* **Output:** a flush of `/dev/cons` copies the buffer straight into the serial driver's transmit ring (`SER_CONS_PUTS`).
* **Input:** `GET_CHAR` and `READ_CHAR` take a key straight from the receive ring and echo it (`SER_CONS_GETC`).
* **Speed:** each costs well under 100 cycles a byte, where an IO request costs about 2,000.  At 115200 baud a byte takes 320 cycles.
* **Otherwise the IO path:** anything they can't do (a task that isn't in the foreground, a full ring, the end-of-input and erase keys) goes through the normal IO request.  So the rules (waiting for the foreground, waiting for room) are the same as before.

`/dev/cons` fds are marked when they're opened (`IO_FD_FLAGS`, `IO_FDF_CONS`), and the mark travels with the fd when it's duplicated or inherited.

**Input read-ahead.**  When fd 0 isn't the console, `GET_CHAR` and `READ_CHAR` read ahead 128 bytes (`$0780`).  So a pipe carries a block per request instead of a byte.  The read-ahead is dropped when fd 0 is closed or replaced.  Reading fd 0 directly after `GET_CHAR` has read ahead misses what was read ahead.

### **Devices**

| Name | Server (task) | What |
| :--- | :------------ | :--- |
| `/dev/cons` | Serial driver (`$F`) | The console (below) |
| `/dev/cons/ctl` | Serial driver | The console's mode, as Plan 9's `consctl`: read `rawon` or `rawoff`; write `rawon` or `rawoff` (below) |
| `/dev/ser` | Serial driver | The serial port as it is, for any task: no foreground rules, no echo |
| `/dev/ser/ctl` | Serial driver | The port's settings: read `b9600 l8 pn s1`; write commands to change them ([below](#the-serial-port-settings)) |
| `/dev/snd` | Sound driver (`$E`) | The YM2151 (below) |
| `/dev/sd/N/data` | Storage (`$C`) | Disk N as bytes at the fd's offset (`IO_SEEK`); the first 4 GB.  N is one character: for an SD card, its SPI device's number as a hex digit, `0`-`7` (`8`-`f`, slot cards' SPI devices, are planned), started at the first open (`ERR_IO_DEVICE` if there's none); other disks are letters that aren't hex digits: `x`, the ROM disk, the paged ROM ([read-only](#the-roms-files-rom)); `r` and `s`, the [RAM disks](#the-ram-disks-ram) |
| `/dev/sd/N/ctl` | Storage | Read: the disk as a line, e.g. `sdhc 7580 MB 15523840 blocks` (or `sdsc`, `rom`, or `none`), and for a HydraFS its label, free space and last check.  Write: `init` starts the card again (e.g. after changing it); `format`, `label`, `check` ([below](#the-files-on-a-card)) |
| `/dev/spi/N`, `/dev/spi/N/ctl` | Storage (page D) | SPI device N (`0`-`f`) as a file: a transaction a write, the bytes it sends back read after ([below](#spi-devices-devspi)) |
| `/dev/gpio/N`, `port`, `ctl`, `ca1` | IO layer (in the reading task: page D) | The VIA's port A on J27: its pins, CA1's edges, CA2 ([below](#gpio-devgpio)) |
| `/sd/N/...` | Storage | The **files** on disk N: the HydraFS server (the device `hfs`, mounted at `/sd`; [below](#the-files-on-a-card)) |
| `/pc/...` | Serial driver (page D) | The **files** in a folder on the PC, through the serial port: the device `pc`, mounted at `/pc` ([below](#a-folder-on-the-pc-pc)) |
| `/dev/pipe` | Pipe server (`$D`) | Made by `IO_PIPE`, not opened by name |
| `/dev/proc` | IO layer (in the reading task) | The tasks (below) |
| `/dev/ram` | The shell registers it (in the reading task) | The RAM itself, as the CPU selects it: task 0's only (any other task's open: `ERR_IO_PERM`); read-only ([below](#the-ram-itself-devram)) |
| `/dev/time` | The shell registers it (in the reading task) | The clock: read `2026-09-29 18:05:00`; write a date and time to set it (below) |
| `/`, `/dev` | The shell registers it (`root`, in the reading task) | The root and `/dev` as directories, as Plan 9's root device is: `/` lists `dev` and the first element of each namespace entry (the task's and the system's, each once; not one the task hides), `/dev` the device table; text lines, or stat records with `IO_MODE_STAT` (`ls -l`).  `IO_OPEN` sends `/` and `/dev` there when no namespace entry has them (as `/dev/root` and `/dev/root/dev`), so `cd /dev` works too.  The devices with files under them list them too, each its own server: `/dev/sd` (the disks started: `0/`, `x/`, `r/`, `s/`), `/dev/sd/N` (`data`, `ctl`), `/dev/gpio`, `/dev/spi`; and `/sd`, the cards started |
| `/dev/null` | IO layer | Reads give end of file; writes are taken and dropped |
| `/dev/zero` | IO layer | Reads give zeros |

**Blocks and caching:** the SD card is read and written a 512-byte block at a time through a one-block cache, and writes go straight through to the card.  `IO_CTL` code `SD_CTL_INIT` (1) on either file starts the card again.

#### **The console**

`/dev/cons` is the terminal on the serial port:
* **Reads** get the keyboard input, but only for the **foreground task**; other readers wait until they're brought to the front.
* **Echo:** it echoes what it reads, like a terminal, and turns the DEL key (what many terminals send for Backspace) into a backspace.  So a program reading a pipe sees no echo.
* **Writes:** only the foreground task and the tasks it started write; the others wait.
* **Buffering:** the serial driver buffers both ways (256-byte rings) and sends from its transmit interrupt.
* **Switching the foreground:** `IO_CTL` code `SER_CTL_FOREGROUND` (1), `.Y` = task, or `CONS_SET_FG` ([tasks.md](tasks.md#the-consoles-foreground-task)).
* **Settings:** the port starts at 9600 baud, 8 data bits, no parity, 1 stop bit (below).
* **Raw mode** (`/dev/cons/ctl`, as Plan 9's `consctl`): write `rawon` to it, and the console's reads get each key as it's typed, with no echo, DEL kept as it is, and Ctrl-D and Ctrl-Z as characters, not the end of input.  It stays raw while the ctl file is open (any fd of it, in any task: its copies count), and goes back to cooked when the last one is closed, or on `rawoff`.  So a program that ends, or is killed, can't leave the console raw.  Ctrl-C, Ctrl-\\ and Ctrl-] still work.  C's `conio` uses it ([programs.md](programs.md#c-programs)).

**Console keys** (acted on by the serial driver as they arrive, so they work on a task stuck in a loop):

| Key | Does |
| :-- | :--- |
| Ctrl-C | Break: the foreground task goes to its break handler (HyForth: back to its prompt with `!BREAK!`); the tasks it started are killed |
| Ctrl-\\ | Kill: the foreground task and the tasks it started end; the shell starts again from scratch |
| Ctrl-D, Ctrl-Z | End of input: a read of `/dev/cons` returns end of file |
| Ctrl-] then `0-F` | Bring that task to the front (prints `[N]`; a bell if it can't be) |
| Ctrl-] then `l` | List the tasks that can be brought to the front (`[1* B ]`: * = the foreground one) |
| Ctrl-] Ctrl-] | Type a Ctrl-] |

**The bell:** whenever the console sends a BEL (Ctrl-G), the YM2151 beeps too (880 Hz on channel 7, skipped while the sound driver is busy).

#### **The serial port settings**

The serial port starts at 9600 baud, 8 data bits, no parity, 1 stop bit.  Its settings are a text file, `/dev/ser/ctl`:
* **Reading it** gives them, e.g. `b9600 l8 pn s1`.
* **Writing it** changes them, with commands separated by spaces:

| Command | Sets |
| :------ | :--- |
| `bN` | The baud rate: 300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200, 115200 |
| `lN` | The data bits: 5-8 |
| `pX` | The parity: `n` none, `o` odd, `e` even, `m` mark, `s` space |
| `sN` | The stop bits: 1 or 2 |

`b19200`, `l7 pe`, and `b4800 l8 pn s1` are all valid.  A write with a bad command changes nothing and fails with `ERR_IO_BAD_REQ`.

**For programs**, `IO_CTL` on any of the serial files (`/dev/cons`, `/dev/ser`, `/dev/ser/ctl`) does the same:

| Code | Name | `.Y` |
| :--- | :--- | :----- |
| 2 | `SER_CTL_RATE` | The rate: `SER_RATE_300` (0) ... `SER_RATE_9600` (8), `SER_RATE_19200` (9), `SER_RATE_115200` (10) |
| 3 | `SER_CTL_FORMAT` | The format: data bits - 5 (bits 0-1), `SER_PAR_*` << 2 (bits 2-4), `SER_FMT_STOP2` (`$20`); `SER_FMT_8N1` = 3 |

**How a change takes effect:**
* **Output first:** the change waits until what's already queued has been sent, so a message printed before it comes out whole.  It then applies to both directions.  Switch the terminal to match afterwards.
* **Actual speed:** the rates are 2.9% slow on the board (the ACIA's 1.79 MHz clock), which terminals accept.
* **Not possible:**
  * 2 stop bits with 8 data bits and parity: the 65C51 sends 1.
  * With a WDC ACIA build, rates below about 1200: a character's time must fit VIA timer 2.
* At 115200 (Rockwell ACIA builds), sending is paced by VIA timer 2, with idle bits between characters (`SER_PACE_GAP` in `hw.inc`; [hardware](../../../reborn/docs/hardware.md#acia-65c51-u3-port-1-irq-line-1)).  At the other rates the ACIA's TDRE interrupt sends each byte as soon as it can.
* **After a reset**, the port is back at 9600 8N1.

In HyForth: `"b19200" stty`, and `stty?` to show the settings.

#### **Sound: `/dev/snd`**

The YM2151 (8 FM channels of 4 operators, at 3.58 MHz), through the sound library in the sound task (`os_rom/sound/`, BIOS ROM page B).

**The volume:** `/dev/snd/volume` is the master volume as text, a percentage: `cat /dev/snd/volume` shows it (100 at boot: songs as written), and `echo 150 > /dev/snd/volume` sets it (0-200; words before the number are skipped).  Below 100 is quieter; above it louder, up to about 24 dB at 200, by lowering the carriers' levels (TL), as far as the chip's loudest: a song written quiet (background music often is) gets the most from it.  `SND_CTL_VOLUME` (below) and C's `snd_volume` set the same.

**Writes** are register/value byte pairs.  The driver waits for the chip between writes, and keeps each register as written (the chip's can't be read back).  The register numbers the chip doesn't have are the **library's commands**, each for the channel `SND_R_CH` last chose (one choice for every fd: send it in the same write as the command):

| Register | Name | Value |
| :------- | :--- | :---- |
| `$02` | `SND_R_CH` | The channel (0-7) for the commands after it |
| `$03` | `SND_R_PATCH` | Load patch n into the channel: 0-127 are General MIDI's instruments, 128-162 drum and percussion sounds (the Commander X16's set).  The channel keeps its speakers |
| `$04` | `SND_R_NOTE` | Key on MIDI note n (60: middle C; 69: A, 440 Hz), with the channel's bend |
| `$05` | `SND_R_OFF` | Key off (the note's release) |
| `$06` | `SND_R_VOL` | The channel's volume, 0-127 (General MIDI's curve) |
| `$07` | `SND_R_PAN` | Its speakers: 1 left, 2 right, 3 both (`SND_PAN_*`) |
| `$09` | `SND_R_BEND` | Its pitch bend: signed, in 64ths of a semitone (-2 to +2 semitones); the note playing moves too |
| `$0A` | `SND_R_DRUM` | A General MIDI drum (MIDI channel 10's note numbers: 36 kick, 38 snare, 42 closed hi-hat ...): its patch and pitch, keyed on |

Everything else goes to the chip as written (except the timers' interrupt enables, `$14`'s bits 2 and 3, which stay off: a register dump from another machine may have them on, and nothing here would clear the interrupt; and timer B, `$12` and its bits of `$14`, while the sound clock has it), with one exception: a **carrier's level** (TL, `$60-$7F`, for the operators the channel's algorithm sounds) gets the channel's and the master volume's attenuation added.  So the volumes work on raw register writes too (a song's), and a new algorithm (`$20-$27`) writes the levels again.

**Reads** give the registers as written (the levels before the volumes): a 256-byte file, a byte a register.

**Claims:** a fd can claim channels (`SND_CTL_CLAIM`): the other fds' writes and commands for them are dropped, and the console's bell leaves channel 7 alone while it's claimed.  The claim lasts until `SND_CTL_RELEASE` or the fd's last close (a task's end closes its fds), which keys the channels off.  So a game can have its music on some channels and its sound effects on others, each program's safe from the rest.

**`IO_CTL` codes:**

| Code | Name | Does |
| :--- | :--- | :--- |
| 1 | `SND_CTL_INIT` | Stop the tune and clear the chip and the library's settings (the claims stay) |
| 2 | `SND_CTL_TEST` | Play the test song (`/rom/songs/test.zsm`: `/rom` is in the system namespace, which every task sees, the sound task's too) in the background, in the song player (a task of its own, which claims the channels): the caller goes on at once.  `ERR_TASK_BUSY` if it's playing already |
| 3 | `SND_CTL_STOP` | Stop the tune |
| 4 | `SND_CTL_CLAIM` | `.Y` = a mask of channels (bit n: channel n), this fd's alone; `ERR_IO_BUSY` if another fd has one of them (none taken) |
| 5 | `SND_CTL_RELEASE` | `.Y` = a mask of channels to give back |
| 6 | `SND_CTL_VOLUME` | `.Y` = the master volume, a percentage, 0-200: 100 (at boot) plays songs as written, less is quieter (the volume curve), more is louder, up to about 24 dB at 200 (the carriers' levels lowered, as far as the chip's loudest).  The file `/dev/snd/volume` is the same, as text |
| 7 | `SND_CTL_CLOCK` | The sound clock, a song player's tick: the YM2151's timer B, its interrupt counting ticks (`SND_CLK`, from 0) and waking the waiting player at its time (`sound/ymfast.s`).  `.Y` = K: a period of K units of 1,024 of the chip's clocks (286 us) or K + 1, as often as the fraction written to `SND_R_CLOCK_F` (`$0C`, and `$0D`, the high byte: 65536ths) says, so the rate is exact on average; 0 stops it.  This fd's while it runs (`ERR_IO_BUSY`: another's); its last close stops it.  Timer A stays the clients' (CSM)  The song player doesn't use it (on the board, timer B didn't keep its period: [hardware](../../../reborn/docs/hardware.md#ym2151-sound-u38-port-4-irq-line-4)); a read of `/dev/snd` shows its numbers in the shadow's spare bytes (`snd_srv.s`: `SND_NUMBERS`) |

HyForth's `patch`, `note`, `noteoff` and `ywrite` ([HyForth](../using/hyforth.md#tasks-and-the-console)) and C's `snd.h` ([the C guide](c.md#sound-sndh)) use it.

#### **SPI devices: `/dev/spi`**

The SPI bus's 16 devices (0-7 the board's headers J18-J25, 8-f the slots' cards: [hardware](../../../reborn/docs/hardware.md#spi-bus-via-port-b)), each a file, as Plan 9 has them.  Served in the storage task, which owns the bus, so a transfer never meets an SD card's (`servers/spi_srv.s`, BIOS ROM page D).

| Name | Read | Write |
| :--- | :--- | :---- |
| `/dev/spi/N` | The bytes the device sent back during the last write; or, with none kept, as many as asked, clocked in as a transaction of their own (sending `$FF`) | Its bytes, sent with the device selected: one transaction a write request (up to 256 bytes), and the bytes that come back meanwhile are kept |
| `/dev/spi/N/ctl` | `mode 0` or `mode 3` | `mode 0` (SCLK idles low) or `mode 3` (it idles high); 1 and 2 are `ERR_IO_BAD_REQ`.  Kept until changed |

* **A command and its answer** are one write and one read: write the command and as many dummy bytes as the answer has, then read them all back (the first bytes are what came back while the command went out).  A read with nothing kept is for a device that just talks: `read` 4 bytes of an ADC's sample.
* **Bits:** MSB first, sent and sampled on SCLK's rising edge (modes 0 and 3), at about 100 kHz (bit-banged on the VIA's port B: `drivers/spi.s`).  The device is deselected after each request.
* **One open at a time:** another open of the same device is `ERR_IO_BUSY` (the fd's dups share it).  So is a device with an SD card started on it (`/dev/sd/N`), and while `/dev/spi/N` is open, the SD driver doesn't start a card there (`ERR_IO_BUSY`, and its `ctl` reads `none`).
* **From HyForth:** `"/dev/spi/1" 3 open`, then `write` and `read` on the fd ([files and devices](../using/hyforth.md#files-and-devices)); from C, `open`, `write`, `read`.

#### **GPIO: `/dev/gpio`**

The VIA's port A on header J27 ([hardware](../../../reborn/docs/hardware.md#via-65c22-u2-port-0-irq-line-0)): 8 pins, PA0-PA7, and the handshake lines CA1 (an input, which can interrupt) and CA2.  Served in its client's task (`servers/gpio_srv.s`, BIOS ROM page D), with interrupts off around each change to the VIA's registers.

| Name | Read | Write |
| :--- | :--- | :---- |
| `/dev/gpio/N` | Pin N (`0`-`7`): `0` or `1` and CR LF, from the offset (`cat` reads it once; a program polling it seeks back to 0) | `0` or `1`: the pin's level, and the pin an output |
| `/dev/gpio/port` | All 8 pins, a byte (any offset) | A byte: the outputs' levels (the inputs keep theirs) |
| `/dev/gpio/ctl` | A line a pin (`2 out 1`, `3 in 0`), then `ca1 fall 0003` (CA1's active edge, and the edges counted), then `ca2 in` (or `ca2 0`, `ca2 1`: an output) | A command: `in N`, `out N`, `ddr HH` (all 8 directions, hex: 1 = out), `ca1 rise`, `ca1 fall`, `ca2 0`, `ca2 1`; anything else is `ERR_IO_BAD_REQ` |
| `/dev/gpio/ca1` | Waits for CA1's next active edge (one since this task last read it, or opened it), then gives the edges counted so far: `0003` and CR LF.  A non-blocking fd gets `ERR_IO_WOULD_BLOCK` instead of waiting | |

* **The pins start as inputs** (the VIA's reset).  PA0 and PA1 are the I2C bus's SCL and SDA too, pulled up on the board; they're plain pins until an I2C driver uses them.
* **CA1's interrupt** is on only while `/dev/gpio/ca1` is open, so a floating CA1 can't flood the system.  The VIA's handler (`VIA_IRQ_HANDLER`, in task 0) counts each edge and wakes the readers waiting.  A switch's bounce counts too: a button wants a capacitor, or a program that ignores edges close together.
* **Port A is read and written without the handshake** (`ORA` at register `$F`), which leaves CA1's flag alone.
* **From HyForth:** `echo 1 > /dev/gpio/2`, `cat /dev/gpio/3`, `echo out 4 > /dev/gpio/ctl`; a program opens the files and reads and writes them.  The [tutorial](../tutorial.md#7-an-led-and-a-button) wires an LED and a button.

#### **The tasks: `/proc`**

The device `proc`, which the boot shell mounts at `/proc`, as Plan 9 has it (`/dev/proc` is the same files):

| Name | Read | Write |
| :--- | :--- | :---- |
| `/proc` | A line per busy task | |
| `/proc/N`, `/proc/N/status` | Task N's line | |
| `/proc/N/ctl` | | `kill`, `break` or `fg`: for N's family only (below) |
| `/proc/N/cwd` | Its current directory | |
| `/proc/N/env` | Its environment, as `/env`'s list | |
| `/proc/N/pages` | `pages PP floor FF` (hex): the MMU pages it has (not counting the ones every task has marked: `$00-$07`, `$7D-$7F`), and its page floor; `-` for a free task | |
| `/proc/N/ns` | Its namespace, as `ns` prints it (the lines that would make it) | |
| `/proc/N/cmd` | | A line for task N's shell to run, as if typed at its prompt: for N's family only (below) |
| `/proc/N/mem` | Its address space as it sees it: offset = address, 64K (below) | Its RAM, `$0000-$9FFF`: for N's family only |
| `/proc/N/ram` | Its banks on the RAM modules: offset = bank * 8K + offset in it (below) | The same: for N's family only |

A line is `N S O`: the task, its state (`R` running or runnable, `W` waiting, `P` paused, `D` a driver) and the task that started it (`-` none), then ` *` for the foreground task.  `/proc` and `/env` are served in their client's task, from ROM page 9 (`servers/proc_srv.s`, `servers/env_srv.s`); `pages` is counted in task N itself (`TASK_CALL`).

**Who may write `ctl` and `cmd`, and use `mem` and `ram`:** task N itself, a task that started N (directly or not: a shell and what it runs), and task 0; any other task gets `ERR_IO_PERM` (`TASK_MAY`, page 5), and a free task `ERR_IO_NOT_FOUND`.  So a shell can't kill, drive or read another shell's programs, and only task 0 reaches a driver's memory.

**`mem` and `ram`:** task N's memory as a debugger, a memory dump or a core file wants it, so `cp /proc/3/mem core3` saves a task's whole address space.
* **`mem`'s offsets are N's addresses:** `$0000-$7FFF` its task RAM (zero page, stack, program); `$8000-$9FFF` the bank it has there (its `$00`; a shared bank with its `U`); `$A000-$DFFF` its paged ROM bank (its `$01`); `$E000-$FEFF` the BIOS ROM page it's on (its `W`); `$FF00-$FFFF` zeros (the I/O space isn't read: a read can have effects).  Past `$FFFF` is the end.  It writes `$0000-$9FFF`; a write anywhere else is `ERR_IO_MODE`.
* **`ram`'s offset `b * $2000 + o`** is byte `o` of N's bank `b` (`$00-$EF`: its banks on the RAM modules, as it numbers them).  Only the banks it has are there (the MMU's bank map: `MM_ALLOC` with `AI_PAGED`); another gives `ERR_IO_NOT_FOUND`.  Past bank `$EF` is the end.  `IO_STAT`'s size is up to its last bank (`mem`'s: 64K).
* **A copy is a snapshot:** N may run between two reads (a debugger stops it first).  Each read or write is up to 64 bytes, and stops at a 256-byte page's end; ask again for the rest, as `cp` does.  It's copied with `T` = N and IRQs off (`MEM_COPY`, `servers/ram_srv.s`, which `/dev/ram` uses too), so it sees N's own zero page, stack and banks, and the zero page bytes and `$00` the copy borrows read (and are written) as N has them.
* **`U` and `W`** are in N's frame on its stack while the scheduler has it switched out.  A task in a `TASK_CALL` (waiting for a server), a driver, or the asker itself has none there: its `$8000` shared bank is read with `U` 0, and its BIOS page is page 0.

**`cmd`:** a write is one line (up to its CR, LF or 0; 39 characters at most).  It waits in task N's IO transfer area until N's shell is at its prompt; if it's there now, waiting for the console (in the background, say), a break wakes it, a quiet one with no `!BREAK!`, and it runs the line, shown after its prompt.  One line waits at a time (`ERR_IO_BUSY`).  HyForth's `send N line` writes it.  A task that isn't a HyForth shell never takes the line.

#### **The clock: `/dev/time`**

The Hydra keeps the date and time as seconds since 2000-01-01 00:00:00, counted by the scheduler's tick.  It has no clock that runs while it's off, so the time starts at 2000-01-01 00:00:00 at power-up, until it's set:
* **Reading** `/dev/time` gives the date and time and CR LF: `cat /dev/time` shows `2026-09-29 18:05:00`.
* **Writing** `YYYY-MM-DD hh:mm:ss` sets it; the seconds can be left out, or the whole time (midnight): `echo 2026-09-29 18:05 > /dev/time`.  2000-01-01 to 2135-12-31; a date that isn't one (`2023-02-29`) is `ERR_IO_BAD_REQ`.
* **From code:** `CLOCK_GET` and `CLOCK_SET` (page 9, `servers/time_srv.s`: `.X` = a zero page address, the 4 bytes of seconds there).  HydraFS stamps files with it ([plans/HYDRAFS.md](../../../reborn/docs/design/plans/HYDRAFS.md#time-stamps)).
* **The clock chip:** a DS1747 in U7 keeps the time while the Hydra's off (`drivers/rtc.s`, page 9).  Its clock registers are task F's `$7FF8-$7FFF` (`RTC_REGS`, `hw.inc`), which nothing else in the ROM writes, in any task.  At boot the shell looks for it (`RTC_BOOT`): its registers must hold a date and time, and its seconds must change within 1.1 s; then `ZP_CLOCK` is set from it at the start of one of its seconds, and `RTC_STATE` (in the system's shared bank) says it's there.  Writing `/dev/time` sets it too (`RTC_SAVE`: the W bit), and looks for it again if it wasn't found; reading `/dev/time` takes its seconds first (`RTC_LOAD`, the R bit, then `CLOCK_ADJUST`, which keeps the tick clock's place in the second).  Each access is a few bytes with IRQs off and `T` switched to F and back for each.

#### **The environment: `/env`**

Each task has an environment: variables, `NAME=value`, which the tasks it starts get a copy of (so a change in a task stays in it and the tasks it starts later).  They're files, as in Plan 9, under `/env`, which every task has with no mount (the IO layer sends names under it to the `env` device, as it does `/dev`):

| Name | Read | Write | Create, remove |
| :--- | :--- | :---- | :------------- |
| `/env` | The variables, a line each: `NAME=value` | | |
| `/env/NAME` | Its value (not found: `ERR_IO_NOT_FOUND`) | Its value: a write at the file's start replaces it, one after that adds to it; CRs and LFs are left out | `IO_CREATE` makes it (or empties it); `IO_REMOVE` removes it |

So `echo /sd/0/bin > /env/PATH` sets one, `cat /env/PATH` shows it, `rm /env/PATH` removes it, and `ls /env` lists them.  A name is 1-30 characters, not `/` or `=`; a task's variables share 256 bytes (`ERR_IO_FULL` beyond).  The shell uses `PATH` (directories, `:` between them, to find programs by name) and `HOME` (`cd` alone), and sets `status` (the last program's exit status: its message, or its code, or empty for success) and `apid` (the task of the last program started with `&`), as Plan 9's `rc` does.  C's `getenv` and `setenv` read and write these files; conio reads `COLUMNS` and `LINES`.

How: each task's environment is a 256-byte block in the system's shared bank (`ENV_BLOCKS`, `$8D00`), entries one after another; `IO_INHERIT` copies the parent's block for a new task (`ENV_COPY`).  An open variable is one of 16 slots (its task and name), so 16 can be open at once.

#### **The RAM itself: `/dev/ram`**

Every byte of RAM, as the CPU selects it, for task 0 (the system's task) and no other: a program can't read another's memory through it (a task's own family will have `/proc/N/mem`: [plans/PROC.md](../../../reborn/docs/design/plans/PROC.md)).  Read-only; `servers/ram_srv.s`, page 9.

| Offsets | What |
| :------ | :--- |
| `$0000000-$007FFFF` | Task RAM: task t's `$0000-$7FFF` at `t * $8000` |
| `$0080000-$027FFFF` | Shared RAM: shared bank ID s at `$80000 + s * $2000` |
| `$0280000-$207FFFF` | The RAM modules: module m's bank b for task t (its bank ID `m << 4 \| b`) at `$280000 + m * $200000 + t * $20000 + b * $2000` |

A read gives up to 64 bytes, and stops at a 256-byte page's end (a short read: read again); past the last module, nothing (end of file).  How: it switches `T`, and the bank (and `U`) the bytes are in, with IRQs off, and copies them straight into task 0's transfer area, which also holds the copy's numbers (the request block's spare bytes: the one RAM every task sees with `$00` at the IO transfer bank).  The task's `$00` and the zero page bytes the copy uses are put back, and a copy of them reads as they were.  The IO self test (WOZMON `F88AR`) has task 0 read a task's RAM, a module bank and a shared bank through it (`TASK_CALL`), and checks that the task itself is refused.

#### **The ROM's files: `/rom`**

Files kept in the paged ROM, read-only, there with no card: programs, songs and (later) libraries.  The paged ROM is a disk, the **ROM disk** (the block device `/dev/sd/x`), with a HydraFS volume on it, which the boot shell mounts at `/rom` with a spec, as Plan 9's `mount` takes one: `mount -s hfs /rom x` (the HydraFS server's disk `x`), in the system namespace, so every task sees it.  It's mounted if the ROM disk has a volume (a paged ROM image without one has no table: no `/rom`, and everything else works).  The disks in memory aren't under `/sd`, which has the cards alone: HydraFS serves them only through a mount with a spec (below).

| Name | Read |
| :--- | :--- |
| `/rom`, `/rom/bin`, ... | A directory: a line per entry, `name size` (a directory: `name/`), or stat records (`IO_CTL_READ_STATS`: `ls -l`, C's `readdir`) |
| `/rom/README`, `/rom/bin/hello.hyx`, ... | The file's bytes; `IO_SEEK` and `IO_STAT` work as on a card (the stamps are 2000-01-01) |

Writing, creating and removing give `ERR_IO_MODE` (the shell's `!IO ERR!`), as do `format` and `label` on `/dev/sd/x/ctl` and writes to `/dev/sd/x/data`: they're refused before anything is changed, even in the block cache.  `cd /rom/bin` works, and the shell looks in `/rom/bin` last for a program by its name (and in `/rom/lib` for a library: [HyForth](../using/hyforth.md#the-shell-directories-files-and-programs)).  So `hello`, `jukebox` or `scom` run on a machine with no card.

**The ROM disk.**  Block n of the ROM disk is paged ROM bank n / 32, at `$A000 + (n % 32) * 512`, as the CPU sees it: 8192 blocks, the whole 4 MB.  A block never crosses a bank, or the 8K halves the board swaps, so the storage driver (`SD_ROM_READ` in `drivers/sd.s`) selects one bank, copies 512 bytes from one place, and puts the storage task's bank back.  A file whose blocks are in several banks is no different from one on a card: HydraFS asks for it a block at a time, so no pointer ever runs from one bank into the next.

| Blocks | Banks | What |
| :----- | :---- | :--- |
| 0 | 0 (`$A000-$A1FF`) | The partition table (an MBR, as a card's), and a line saying what the disk is |
| 1-63 | 0-1 | Partition 1, type `$DA` (not a file system): the system's banks: HyForth's variables, the hardware test |
| 64-8191 | 2-255 | Partition 2, type `$7F`: the HydraFS volume, label `ROM`, read-only |

What's in it comes from `os_rom/romfs.txt`, a list of files (their names in `/rom`, and where the build finds them).  After the link, the build's `sim/tools/mkromdisk.js` makes the volume with the HydraFS PC tool (`hydrafs.js`, stamped 2000-01-01 so each build is the same), writes the table and the volume's blocks into the paged ROM image, then reads the image back the way the CPU would (through the emulator's bank mapping) and checks every file against its source: a mismatch fails the build.  `node sim/tools/mkromdisk.js os_rom/romfs.txt os_rom/bin/paged_rom_C02.bin --list` (from `os_rom`, after a build) also shows which banks each file is in.  The `rom-copy` test copies every file from `/rom` to a card on the emulated machine and compares them with their sources.

#### **The RAM disks: `/ram`**

Two disks in RAM, each with a HydraFS on it: fast, there with no card, and **empty after a reset** (each is quick-formatted when it starts).  The plan they come from is [plans/DISKS.md](../../../reborn/docs/design/plans/DISKS.md).

| Disk | Names | Memory | At boot |
| :--- | :---- | :----- | :------ |
| `r`, the RAM disk | `/dev/sd/r`; a shell's own area at `/ram` | The storage task's own 8K banks on the RAM modules | 256K (on a machine with less: half that, and so on) |
| `s`, the shared RAM disk | `/dev/sd/s`; `/sram` | Shared RAM banks, from the lower half (IDs `$01-$7F`: `SH_ALLOC` takes from the top) | 512K (the same) |

Each shell mounts **its own area** of the RAM disk at `/ram` (below), and the boot shell mounts the shared disk at `/sram`, both with a spec: `ns` shows `mount hfs /ram r/1` (the shell's own namespace) and `mount -s hfs /sram s` (the system's).  So `/ram` means "my files in RAM" in every shell, as Plan 9 gives each process its own names, and the tasks a shell starts (programs, scripts, pipelines) inherit it.

**Areas.**  The RAM disk's top directories are the tasks' areas: `N`, a hex digit (`0`-`9`, `a`-`f`), is task N's.  A shell mounts its own when it starts (`SH_OWN_AREA`: `mount hfs /ram r/N`, N its task: the boot shell's is `r/1`) and makes it, with `bin` and `lib` in it.  To see them all, mount the whole disk: `mount hfs /a r`, then `ls /a`.  **With no memory modules** there's no RAM disk (its banks are theirs), so a shell's area is on the shared RAM disk instead, `s/ram/N` in `/sram/ram` (`ns`: `mount hfs /ram s/ram/1`): `/ram` works the same, but the area is kept when the shell ends, and every task can use it, as `/sram`.
* **Who:** anything in area N (opening, creating, removing, `ls`) is for task N, the tasks it started (and theirs, up the owner chain: `ZP_TASK_OWNER`), and task 0, the system's (the kernel's `TASK_MAY`).  Any other task gets `ERR_IO_PERM` (`$88`, the shell's `!IO ERR! not allowed`).  So a program the shell runs, or a stage of a pipeline, uses the shell's area, and two shells' families can't see into each other's.
* **Other names** in the root (`zz`) aren't areas: refused.  The root's listing is everyone's.  Renaming an area, or changing its mode, is refused too (but for task 0).
* **Owners:** when a task ends, the tasks it started get its owner instead (`TASK_ORPHANS`, as Unix gives orphans to `init`), so a new task in its slot isn't taken for their parent.
* **An area goes when its task ends,** with everything in it: after the task's fds are closed, and before its parent (a shell waiting for it) goes on (`TASK_AREA_END`, then `HFS_AREA_END` in the storage task).  If a task it started still has a file in it open, it stays, and goes when task N ends next.  (The storage task keeps a bit for each area that may be there, `RAMD_AREAS`, so a task without one ends as fast as before.)

**With no card,** the boot shell's current directory is its own area, `/ram` (the prompt `/ram> `), so files can be saved there (until a reset), and it runs `/rom/boot.hys` (a line about it) in place of a card's `boot.hys`.

**Before `boot.hys`,** the boot shell runs `/rom/lib/namespace`, the default namespace's `/bin` and `/lib` ([namespaces](io.md#namespaces)), then the card's `lib/namespace`, if it has one: lines of `bind` and `mount` for the card's own directories (`bind -a /sd/0/tools /bin`), and `#` comments.

**Program caches.**  The boot shell makes `/sram/bin` and `/sram/lib` (the shared caches), and each shell's area has its own `bin` and `lib`.  A program typed by its name is looked for in the current directory (Plan 9's `.` first), then in `/bin`, a union ([namespaces](#namespaces)) of:
1. the shell's cache, `/ram/bin` (its `-c` member: a copy into `/bin` goes there; a bind to `/ram/bin`, so in each shell it's that shell's);
2. the shared cache, `/sram/bin`;
3. the boot card's `/bin`;
4. `/rom/bin`;

then in `$PATH`'s directories.  Caching a program is copying it there: `cp /sd/0/bin/game.hyx /bin/game.hyx` (or to `/sram/bin`, for every shell), and then `game` loads from RAM, about three and a half times as fast as from a card (a 16K program: 1.34 million cycles, against 4.69 million: [DISKS.md](../../../reborn/docs/design/plans/DISKS.md) has where the time goes).  Libraries are found the same way, in the `lib` directories (`lib name`).  A copy isn't checked against the card's: after rebuilding a program, copy it again (or remove the cached one).

**Starting and stopping** them is a write to the ctl file (`echo stop > /dev/sd/s/ctl`, or HyForth's `ctl`, which reports errors: `q^/dev/sd/s/ctl^ q^stop^ ctl`):

| Write | Does |
| :---- | :--- |
| `start SIZE [FROM-TO]` | Take SIZE (8K banks; or `K` or `M` after it: `256K`, `1M`), from the RAM modules FROM-TO (`r`: `start 256K 1-2`, the storage task's banks on modules 1 and 2) or the shared bank IDs FROM-TO (`s`: `start 1M $20-$9F`), or from anywhere; then make an empty HydraFS on it.  Numbers are decimal, or hex after a `$`.  `ERR_IO_BUSY`: it's started (stop it first); `ERR_IO_FULL`: no run of free banks that long there; `ERR_IO_BAD_REQ`: not a size or a range |
| `stop` | Give its banks back; its files are lost.  `ERR_IO_BUSY` while a file on it is open |

Reading the ctl file gives `ram 256 KB 512 blocks` (`sram` for the shared one), `banks $10-$2F` (its banks, or its shared bank IDs), then its HydraFS's lines; `none` when it's stopped.  `format` and `check` work as on a card, and `/dev/sd/r/data` is the disk's bytes.  How: the storage driver reads or writes a block by mapping its bank at `$8000` in the storage task (the RAM disk's bank in its own `$00`; the shared one's with `U`), copying 512 bytes and putting them back (`SD_RAM_READ`, `SD_RAM_WRITE` in `drivers/sd.s`); the starting and stopping are in `servers/ramdisk.s`, with the memory manager's `MM_BANK_ALLOC_IN` (task banks from a range) and `SH_BANK_ALLOC` (shared banks by number, with no handle).

#### **A folder on the PC: `/pc`**

`/pc` is a folder on the PC, served over the serial port by the PC tool, which is the terminal too ([plans/PC.md](../../../reborn/docs/design/plans/PC.md)):

```
node sim/tools/hydrapc.js COM3 C:\hydra [--read-only]     (on the PC; once, in sim/: npm install)
```

Its files work as a card's do ([below](#the-files-on-a-card)): read, write, create (a directory too), remove, rename, `ls -l`'s sizes and times (the PC's), a program run from it (`/pc/game`, or by name after `bind -a /pc/bin /bin`).  The device `pc` runs in the serial task, and `/rom/lib/namespace` mounts it (`mount -s pc /pc`).  Its requests and replies are frames on the serial line, between the console's bytes: the PC tool doesn't show them, and the Hydra's serial handler takes the PC's before the console sees them.
* **Speed:** the line's, about 900 bytes a second each way at 9600 baud: a 2K program in about 2.5 s.
* **Songs** play from it in time (`play /pc/song.zsm`): the player reads ahead without waiting ([SOUND.md](../../../reborn/docs/design/plans/SOUND.md)).  But a song that needs more than the line carries (a dense one needs about 2 KB a second), or a tick with more than about 500 bytes of writes, waits for it: copy such a song to `/ram` or `/sram` first.
* **Errors:** a damaged frame, or one lost, is sent again; with no PC tool on the line (or after 3 tries) a request fails with `ERR_IO_DEVICE` (`no answer`) after a second or two, and the attach frame's 7 bytes show on the terminal.  With the tool's `--read-only`, every change is `ERR_IO_PERM` (`not allowed`).  The other errors are HydraFS's (`ERR_IO_NOT_FOUND`, `ERR_IO_EXISTS`, `ERR_IO_NOT_EMPTY`, `ERR_IO_BUSY`: removing a file the Hydra has open).
* **One request at a time:** tasks using `/pc` together take turns.
* **The emulator** plays the PC tool's part: `--pc-dir FOLDER` ([emulator](../tools/emulator.md)).

### **Pipes**

`IO_PIPE` (`$F893`) makes a pipe and returns two fds: `.A` reads it, `.X` writes it.
* **Buffer:** 255 bytes.  A writer waits while it's full, and a reader while it's empty.
* **Ends:** readers get end of file once every write fd is closed.  Writers get `ERR_IO_BROKEN` once every read fd is closed.
* **Limit:** there are 8 pipes (`ERR_IO_NO_PIPES` beyond that).

To connect two tasks, make the pipe, then point the child's stdin or stdout at one end with `IO_DUP2` before starting it: it inherits the fds.  Then close the ends each task doesn't use.  HyForth's `|` does exactly this.

### **Namespaces**

Each task has its own namespace of up to 32 entries, which the tasks it starts inherit, on top of the **system namespace**, 32 more that every task sees.  It works as Plan 9's does ([NAMESPACES.md](../../../reborn/docs/design/plans/NAMESPACES.md)): entries with the same path are a **union**, whose members are looked in, in order, for a name.

| Call | Does |
| :--- | :--- |
| `IO_MOUNT` (`$F89C`) | `.A.Y` = path (up to 13 characters), `ZP_IO_BUF` = a device's name, `.X` = flags: names under the path go to that device's server, with the rest of the name (`/z/sub` → device `zero`, name `/sub`).  With `NS_SPEC`, `ZP_IO_CNT` = a spec, as Plan 9's `mount` takes one (up to 14 characters): the server gets the spec's path, then the rest (`mount hfs /rom x`: `/rom/bin` → `/x/bin`) |
| `IO_BIND` (`$F89F`) | `.A.Y` = path, `ZP_IO_BUF` = target (up to 15), `.X` = flags: names under the path stand for the same names under the target (`/tty` → `/dev/cons`).  `.X` = `NS_HIDDEN`: a **hide**, no target: nothing under the path is found |
| `IO_UNMOUNT` (`$F8A2`) | `.A.Y` = path, `.X` = 0 (or `NS_SYSTEM`): remove all its entries.  Other bits in `.X`: just the member `ZP_IO_BUF` names (a bind's target, or a mount's device).  `.X` = `NS_FRESH` (the path: `"/"`): a fresh namespace, as Plan 9's `newns`: every entry of the task's own goes but its `/ram` mount, so it sees the system namespace and its area, as a new shell does |
| `IO_NS_LIST` (`$F8A5`) | Print the entries as the lines that would make them, as Plan 9's `ns` does: `mount [-acs] device /path [spec]`, `bind [-acs] /target /path`, `hide /path`; the system namespace's first (`-s`), then the task's own |

**The flags** (`.X`, `include/io.inc`): none (0) replaces the path's entries with this one, as a plain bind or mount always did; `NS_BEFORE` (`-b`) puts it before the path's members, `NS_AFTER` (`-a`) after them; `NS_CREATE` (`-c`) makes it the member a new file in the union is made in.  A union with no `-c` member makes no files (`ERR_IO_MODE`).  `NS_SYSTEM` (`-s`) makes the entry the system namespace's: for task 0 and the boot shell only (`ERR_IO_PERM` for others).

**The system namespace** is a table of 32 entries every task sees, under its own: the boot shell writes it at boot (its mounts, `/rom/lib/namespace`'s lines), and a copy of it sits in each IO transfer bank (`NS_SYS`, `$9800`), so a name resolves without switching banks.  A task's own entry for a path wins over the system's, so a plain `bind` or `mount` at a system path replaces it for that task (and the tasks it starts).  Changing a system union in a task (`bind -a`, `-b`, or `unmount` of one member) first copies the union's members into the task's own table (copy on write); `unmount` of a system path leaves a hide in the task's.  So every task behaves as if it had a copy of the system's entries, and only what a task changes costs it entries of its own.

**How `IO_OPEN` resolves a name:**
* It applies the entry with the **longest matching prefix**, matching whole path elements (`/z` matches `/z/sub`, not `/zz`), in the task's table or the system's (a tie: the task's).
* After a bind it looks again, up to 4 times (`ERR_IO_NS_LOOP` beyond).
* In a union, it opens the name in the first member; if that member hasn't got it (`ERR_IO_NOT_FOUND`), the next, and so on.  A create goes to the `-c` member.  A hide's path gives `ERR_IO_NOT_FOUND`.
* **A union's directory** is read member by member, as in Plan 9: when an fd reading one reaches the end of a member's listing, the IO layer opens the same name in the next member that has it, on the same fd, and the read goes on (`IO_UNION_NEXT` in `io/io.s`).  A name in two members shows twice.  One such fd a task at a time (the last opened; the name up to 50 characters): another fd on a union's directory reads only its first member.
* A name no entry matches must be under `/dev`: the IO layer sends `/dev/<device>/...` to the device in every task, with no mount.  Everything else is an entry's.  `/` and `/dev` themselves are the `root` device's directories ([above](#devices)): `ls /` lists the namespace's first names, `ls /dev` the devices.
* Through a mount with a spec, the request says so (`IO_BLK_SPEC`): HydraFS serves its disks in memory (`x`, `r`, `s`) only then (or for task 0), so `/sd` has the cards alone.

**The namespace at boot.**  In the system namespace, the boot shell mounts `/sd` (`hfs`), `/env` (`env`), `/proc` (`proc`), and `/rom` and `/sram` (`hfs`, with the specs `x` and `s`, if they're there), and binds the boot card's `bin` and `lib` at `/bin` and `/lib`; in its own, it mounts its area at `/ram`.  Then, before `boot.hys`, it runs `/rom/lib/namespace` (on the ROM disk: `os_rom/romfs/namespace`), whose `-s` lines put the program caches before the card's and the ROM's after, and mount `/pc` ([above](#a-folder-on-the-pc-pc)), and then the card's own `lib/namespace` if it has one (lines of the shell's `bind` and `mount`, and `#` comments).  With a card, `ns` shows:

```
mount -s hfs /sd
mount -s env /env
mount -s proc /proc
mount -s hfs /rom x
mount -s hfs /sram s
bind -cs /ram/bin /bin
bind -as /sram/bin /bin
bind -as /sd/0/bin /bin
bind -as /rom/bin /bin
bind -cs /ram/lib /lib
bind -as /sram/lib /lib
bind -as /sd/0/lib /lib
bind -as /rom/lib /lib
mount -s pc /pc
mount hfs /ram r/1
```

That's 14 of the system's 32 entries and 1 of the shell's own 32.  `/bin`'s first member is `/ram/bin`, which each task resolves in its own namespace: each shell's cache.  A card's `lib/namespace` adds with `-s` for everyone (`bind -as /sd/0/tools /bin`), or without it for the boot shell and what it starts.  Every task sees the same, so a C program's `fopen ("/bin/x.hyx")` finds what the shell does.

### **The current directory**

Each task has a current directory, which the tasks it starts inherit (a copy: changing it later doesn't change theirs).  It's kept in the task's IO transfer area, beside its namespace.

* **Relative names:** before a name is looked up, the IO layer puts the current directory in front of it (unless it starts with `/`) and tidies the result: `.` elements go, `..` takes the element before it away (at `/` it stays at `/`), doubled and trailing `/`s go.  So `../notes` from `/sd/0/games` is `/sd/0/notes`, and a server only ever sees absolute names.  The tidy, absolute name can be up to 63 characters (`ERR_IO_NAME` beyond).
* **`IO_CHDIR`** makes a path the current directory: a directory on a card (its stat record says so: `ERR_IO_NOT_DIR` otherwise), or `/`.  A failure leaves the current directory as it was.
* **`IO_GETCWD`** copies it out: `/` at the top.
* A new task (`TASK_RUN`, `TASK_CLONE`) gets its parent's.  The boot shell sets its own to the first card with a HydraFS on it ([hyforth.md](../using/hyforth.md#the-shell-directories-files-and-programs)).

### **The files on a card**

The **HydraFS** server (the device `hfs`) serves the files on the SD cards.  The shell mounts it at `/sd` at startup, and the tasks it starts inherit the mount, so `/sd/0` is card 0's root directory and `/sd/0/games/star.frt` is a file on it.  The format, and the host tool that makes cards, are in [plans/HYDRAFS.md](../../../reborn/docs/design/plans/HYDRAFS.md) and [tools/emulator.md](../tools/emulator.md#hydrafs-card-images).

* **Names** are case-sensitive, 1-31 characters, any byte but `/` and 0.  `.` and `..` are understood while walking (a path may be up to 8 elements deep); `..` at a card's root stays there.
* **Reading a file** works as it does on `/dev/sd/N/data`, except that a read stops at the end of the file.
* **Writing a file** at the fd's offset grows it past its end, and an append-only file is always written at its end.  A write that *starts* past the end (after a seek) makes the file that long first, with zeros: whole 4 KB clusters of them are a **hole**, which takes no space on the card, and reads as zeros ([sparse files](../../../reborn/docs/design/plans/HYDRAFS.md#sparse-files)); a write into a hole takes a cluster for it.  Writing runs at about 3.5 KB/s.
* **Reading a directory** gives a line per entry, `name size` (or `name/` for a directory), then CR LF, so `"/sd/0" 1 open 0 fdup2 cat | cat` lists it (as the shell's `ls` does).  Opened with `IO_MODE_STAT` it gives stat records instead; read a multiple of 48 bytes to get whole ones.  Either way the listing is made again from the card at every read, so the server keeps no state for it, and a directory that changes between two reads of one listing can give a torn one.
* **Making and removing files:**
  * `IO_CREATE` makes a file (mode bits 0; `HFS_M_APPEND` `$40` append-only, `HFS_M_RO` `$01` read-only) or a directory (`HFS_M_DIR` `$80`) in a directory that's there, and opens it; a directory is opened for reading whatever the mode says.  A *file* that's there already is emptied and opened instead, as in Plan 9.
  * `IO_REMOVE` removes a file that isn't open, or an empty directory; it borrows a free fd for the request.
  * `IO_WSTAT` renames a file in its directory (the record's name: a name, not a path; a 0 first byte keeps it) and sets its mode bits (`HFS_M_APPEND`, `HFS_M_RO`; `$FF` keeps them).  The record's other fields are left alone.  HyForth's `mv` fills in the record for you.
  * The mode bits are checked when a file is opened: a read-only file can't be opened for writing, but the fd that made it can write it.
* **What reaches the card when:** the data at once; the file's size when the last fd on it is closed, and whenever it gets a new 4 KB cluster.  So close a file you've written before taking the card out.  A crash can leave a file shorter than was written, never a damaged card.
* **Formatting:** write `format [-f] [-p] [-s size] [LABEL]` to `/dev/sd/N/ctl` to make an empty HydraFS on the card (everything on it is lost), and `label NAME` to change the label.  Plain `format` is quick (a version 2 HydraFS, whose free map is written as it's used); `-f` writes the whole map now, printing its progress on the console; `-s` limits the size (megabytes, or `4G`) ([plans/HYDRAFS.md](../../../reborn/docs/design/plans/HYDRAFS.md#formatting-and-tools)).
* **Partitions:** a card with a partition table has its HydraFS in its partition of type `$7F`, and the other partitions (a FAT one for a PC, say) are left alone.  `format` on such a card formats that partition; `format -p` makes one on a card without one, after its other partitions (or with a new table, from block 2048).  The ctl file then shows `partition at block N` ([plans/HYDRAFS.md](../../../reborn/docs/design/plans/HYDRAFS.md#partitions)).
* **Checking:** write `check` to `/dev/sd/N/ctl`, then read the file: it counts the clusters marked in use that nothing uses (lost), in use but marked free (unmarked), and used twice, and recounts the free space.  `check fix` also repairs the free map (not a cluster used twice: that's reported for a person to sort out).  In HyForth: `"/dev/sd/0/ctl" "check" ctl`, then `ls /dev/sd/0/ctl` (or `0 fsck`, which does both).  It takes a pass per 256 MB of card, and with 16 passes or more prints its progress on the console as it goes ([plans/HYDRAFS.md](../../../reborn/docs/design/plans/HYDRAFS.md#the-check)).
* **Errors:** `ERR_IO_NOT_FS` (`$80`) if the card holds no HydraFS, `ERR_IO_DEVICE` (`$79`) if there's no card, `ERR_IO_NOT_FOUND` (`$70`) for a name that isn't there (or a path through a file), `ERR_IO_NO_FDS` (`$75`) when all 8 HydraFS files are open (they're shared by every task), `ERR_IO_MODE` (`$72`) for writing a directory or a read-only file, `ERR_IO_FULL` (`$81`), `ERR_IO_EXISTS` (`$82`: creating a directory where there's a name already, a file where there's a directory, or renaming to a name that's taken), `ERR_IO_NOT_EMPTY` (`$83`) and `ERR_IO_BUSY` (`$84`: removing an open file).  The shell's commands add `ERR_IO_NOT_DIR` (`$85`: `cd` or `rmdir` on a file) and `ERR_IO_IS_DIR` (`$86`: `rm` or `cp` on a directory).  Any other device refuses these calls with `ERR_IO_BAD_REQ`.

### **Stat**

`IO_STAT` fills a 48-byte record from the server.  A device with nothing to say returns all zeros; HydraFS fills it in from the file's directory entry:

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 32 | The name, zero-terminated |
| 32 | 1 | Mode: bit 7 = a directory, bit 6 = append-only, bit 0 = read-only |
| 33 | 1 | The disk's number (0-7: the card `0`-`7`; 8: the ROM disk, `x`; 9 and 10: the RAM disks, `r` and `s`) |
| 34 | 2 | The qid's version: up by 1 at every change |
| 36 | 4 | The qid's id: unique on the card, never reused, and the same across renames |
| 40 | 4 | The size in bytes |
| 44 | 4 | The modification stamp: the clock's time at the last change (seconds since 2000-01-01; [/dev/time](#the-clock-devtime)) |

A directory read with `IO_MODE_STAT` returns these same records, one per entry.
