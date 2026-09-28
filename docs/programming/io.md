## **Input and output**

All IO on the Hydra goes through **file descriptors**, Plan 9 style:
* A task opens a name (`/dev/cons`, `/dev/sd/0/data`), gets an fd, and reads and writes it.
* Devices are **file servers**: a driver registers its names, and each request runs in the driver's task.
* A read with no data makes the task wait, using no CPU, until the driver wakes it.

This chapter is the application side.  Writing a server is in [servers.md](servers.md), and the design is in [plans/IO_PLAN.md](../plans/IO_PLAN.md).  Sources: `os_rom/io/`.  Part of the [Programmer's Guide](README.md).

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
| `IO_STAT` (`$F881`) | `.A` = fd, `ZP_IO_BUF` = 16-byte buffer | The server's stat block |
| `IO_CTL` (`$F884`) | `.A` = fd, `.X` = control code, `.Y` = argument | `.A` from the server |
| `IO_DUP` (`$F896`) | `.A` = fd | `.A` = a new fd for the same file |
| `IO_DUP2` (`$F890`) | `.A` = fd, `.X` = new fd | fd `.X` now refers to fd `.A`'s file (closed first if open) |
| `IO_PIPE` (`$F893`) | | `.A` = read fd, `.X` = write fd |

**Modes** (`IO_OPEN`'s `.X`): `IO_MODE_READ` (`$01`), `IO_MODE_WRITE` (`$02`), `IO_MODE_RDWR` (`$03`), plus `IO_MODE_NONBLOCK` (`$80`).  With `IO_MODE_NONBLOCK`, a read with no data yet returns `ERR_IO_WOULD_BLOCK` instead of waiting.

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
| `/dev/ser` | Serial driver | The serial port as it is, for any task: no foreground rules, no echo |
| `/dev/ser/ctl` | Serial driver | The port's settings: read `b9600 l8 pn s1`; write commands to change them ([below](#the-serial-port-settings)) |
| `/dev/snd` | Sound driver (`$E`) | The YM2151 (below) |
| `/dev/sd/N/data` | Storage (`$C`) | SD card on SPI device N (0-7), as bytes at the fd's offset (`IO_SEEK`); the first 4 GB.  The card is started at the first open (`ERR_IO_DEVICE` if there's none) |
| `/dev/sd/N/ctl` | Storage | Read: the card as a line, e.g. `sdhc 7580 MB 15523840 blocks` (or `sdsc`, or `none`).  Write: `init` starts the card again (e.g. after changing it) |
| `/dev/pipe` | Pipe server (`$D`) | Made by `IO_PIPE`, not opened by name |
| `/dev/proc` | IO layer (in the reading task) | The tasks (below) |
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
* **After a reset**, the port is back at 9600 8N1.

In HyForth: `q^b19200^ stty`, and `stty?` to show the settings.

#### **Sound: `/dev/snd`**

**Writes** are YM2151 register/value byte pairs.  The driver waits for the chip between writes.

**`IO_CTL` codes:**

| Code | Name | Does |
| :--- | :--- | :--- |
| 1 | `SND_CTL_INIT` | Stop the tune and clear the chip |
| 2 | `SND_CTL_TEST` | Play the test tune in the background, in a player task: the caller goes on at once.  `ERR_TASK_BUSY` if it's playing already.  The tune keeps time by the system tick and sleeps between notes |
| 3 | `SND_CTL_STOP` | Stop the tune |

#### **`/dev/proc`**

| Name | Read | Write |
| :--- | :--- | :---- |
| `/dev/proc` | A line per busy task | |
| `/dev/proc/N`, `/dev/proc/N/status` | Task N's line | |
| `/dev/proc/N/ctl` | | `kill`, `break` or `fg` |

A line is `N S O`: the task, its state (`R` running or runnable, `W` waiting, `P` paused, `D` a driver) and the task that started it (`-` none), then ` *` for the foreground task.

### **Pipes**

`IO_PIPE` (`$F893`) makes a pipe and returns two fds: `.A` reads it, `.X` writes it.
* **Buffer:** 255 bytes.  A writer waits while it's full, and a reader while it's empty.
* **Ends:** readers get end of file once every write fd is closed.  Writers get `ERR_IO_BROKEN` once every read fd is closed.
* **Limit:** there are 8 pipes (`ERR_IO_NO_PIPES` beyond that).

To connect two tasks, make the pipe, then point the child's stdin or stdout at one end with `IO_DUP2` before starting it: it inherits the fds.  Then close the ends each task doesn't use.  HyForth's `|` does exactly this.

### **Namespaces**

Each task has its own namespace of up to 7 entries, which the tasks it starts inherit:

| Call | Does |
| :--- | :--- |
| `IO_MOUNT` (`$F89C`) | `.A.Y` = path (up to 13 characters), `ZP_IO_BUF` = a device's name: names under the path go to that device's server, with the rest of the name (`/z/sub` → device `zero`, name `/sub`) |
| `IO_BIND` (`$F89F`) | `.A.Y` = path, `ZP_IO_BUF` = target (up to 15): names under the path stand for the same names under the target (`/tty` → `/dev/cons`) |
| `IO_UNMOUNT` (`$F8A2`) | `.A.Y` = path: remove its entry |
| `IO_NS_LIST` (`$F8A5`) | Print the entries: `/path -> device` (a mount), `/path = /target` (a bind) |

**How `IO_OPEN` resolves a name:**
* It applies the entry with the **longest matching prefix**, matching whole path elements (`/z` matches `/z/sub`, not `/zz`).
* After a bind it looks again, up to 4 times (`ERR_IO_NS_LOOP` beyond).
* A name no entry matches must be under `/dev`.

### **Stat**

`IO_STAT` returns a 16-byte block from the server.  The devices so far return all zeros; the filesystem ([plans/HYDRAFS.md](../plans/HYDRAFS.md)) will define it.
