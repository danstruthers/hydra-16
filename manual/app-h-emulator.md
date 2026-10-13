# Appendix H. The Emulator and the PC Tools

Chapter 2 shows how to use the emulator. This appendix lists everything it can do, and the other tools for the PC
that come with HydraOS, in `reborn/sim/tools`. Each is a Node.js program: run it with `node`, from the HydraOS
folder.

## H.1 The terminal emulator's options

```text
node reborn/sim/run.js [options]
```

**Running it:**

| Option | What it does |
|---|---|
| `-i`, `--interactive` | The terminal is the Hydra's serial console, in real time. Without it, the emulator runs for a while, then prints a report (section 2.4) |
| `--speed N` | With `-i`: run N times faster than real time (0: as fast as it can) |
| `--cycles N` | Stop at cycle N (without `-i`: 30,000,000, about 8.4 seconds of the Hydra's time) |
| `--input TEXT` | Keys to type: `\r` or `\n` is <kbd>RETURN</kbd>, `\w` waits 2 million cycles. One key every 20,000 cycles, from cycle 200,000. Put a `\w` or two before the first key, so that HydraOS's console is running. The emulator's terminal obeys the Hydra's XON and XOFF, so a long input arrives whole. `manual/tools/capture.js` types a script a line at a time, waiting for each prompt |
| `--paste` | Type `--input`'s keys as fast as the serial line takes them (a key that arrives before the last is read is lost, as on a real Hydra) |
| `--flow none` | The emulator's terminal ignores the Hydra's XON and XOFF (section 13.6). Without this it obeys them, as a terminal set to XON/XOFF does |
| `--win32-input` | With `-i` in Windows Terminal: lets <kbd>CTRL</kbd>-<kbd>TAB</kbd> and <kbd>CTRL</kbd>-<kbd>SHIFT</kbd>-<kbd>TAB</kbd> reach the Hydra (after you remove Windows Terminal's own use of them from its settings) |

**The hardware:**

| Option | What it does |
|---|---|
| `--modules N` | RAM modules on the memory cards (2 if it isn't given) |
| `--clock MHz` | The processor's clock: 3.579545 (the normal one), or 7.15909 (jumper J8, for a board with faster parts than Appendix I's; HydraOS must be built with `--clock 2` too) |
| `--acia wdc` | A WDC W65C51N chip for the serial port, instead of a Rockwell one (HydraOS must be built with `--acia wdc` too) |
| `--seed N` | The random values that RAM and the registers have at power-up: the same N gives the same values every time |
| `--bios FILE`, `--prom FILE` | Other ROM images than HydraOS's own (`--prom`: the whole paged ROM, its chips' images one after another) |
| `--sd FILE` | An SD card image, in SD device 0 (then 1, and so on, for more). The Hydra reads and writes the file itself |
| `--no-sd` | No SD card. (Without `--sd` or `--no-sd`, the emulator puts in HydraOS's sample card, `bin/sdcard.img`, keeping its changes only in memory) |

**The Vera X:**

| Option | What it does |
|---|---|
| `--vera [V]` | A Vera X card in slot 0; V is its gateware's version (47.0.2 if it isn't given; 0.9 for the oldest, without the FX registers) |
| `--vera-config MS` | How long the Vera X's FPGA takes to start up, after power-up or a reset (100 milliseconds) |
| `--vera-sd FILE` | A card image in the Vera X's own SD card slot (`/sd/v`) |
| `--smc` | The Vera X's keyboard and mouse controller |
| `--kbd TEXT` | Keys typed on the Vera X's keyboard, as `--input`'s are (`\e` is <kbd>ESC</kbd>, `\b` <kbd>BACKSPACE</kbd>) |
| `--view [PORT]` | With `-i`: the Vera X's screen in a web browser, at http://localhost:8016 (or PORT). With `--smc`, the page's keys and mouse go to the Hydra |
| `--sound [PORT]` | With `-i`: the Hydra's sound in a web browser (the same page as `--view`, if there is one): press its **Sound** button. It plays about 0.15 seconds late |
| `--wav FILE` | Record the sound into a WAV file, as the Hydra's time passes |
| `--screen` | After the report: the Vera X's text, as text |
| `--frame-png FILE` | At the end, a picture of the Vera X's screen (640 by 480); with `-i`, the file <kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>P</kbd> saves to |

**/pc** (section 2.5):

| Option | What it does |
|---|---|
| `--pc-dir FOLDER` | The Hydra sees FOLDER, on your PC, as `/pc` |
| `--pc-read-only` | The Hydra can read `/pc`, but not change it |
| `--pc-log` | List what the Hydra does with `/pc` as it happens |
| `--pc-damage F,...` | Damage some of `/pc`'s messages on the serial line, to test that they're sent again: `qN` the Nth message from the Hydra, `rN` the Nth reply |

**Looking inside** (for finding bugs in HydraOS itself):

| Option | What it does |
|---|---|
| `--trace N` | The last N instructions run, in the report (25) |
| `--trace-calls [LIST]` | Log every system call a program makes: its name, its registers, and what it gave back. LIST limits it: names (`OPEN,READ`), or `t3` for task 3's |
| `--break SPEC` | Stop before the instruction at SPEC (below); with `-i`, open the monitor there |
| `--watch-pc ADDR` | Log each time the processor reaches ADDR (on the kernel's page 0) |
| `--watch ADDR[:T]` | Log each write to ADDR ($0000-$7FFF), by any task, or by task T only |
| `--watch-read ADDR[:T]` | Log each read of ADDR |
| `--log FILE` | Write the log (the watches, the calls) into FILE instead of the terminal |

A SPEC, for `--break` and the monitor's `b`, is a kernel label (`K_OPEN`), a module's label while its task has its
bank (`rc:main`), a kernel ROM page and an address (`0:E000`), or just an address (on any page or bank).

## H.2 The emulator's keys

With `-i`, <kbd>CTRL</kbd>-<kbd>A</kbd> is the emulator's own key; press it, and then:

| Then | What it does |
|---|---|
| <kbd>X</kbd> | Quit |
| <kbd>R</kbd> | Reset the Hydra (the RESET button) |
| <kbd>S</kbd> | Show the state: the tasks, and more |
| <kbd>B</kbd> | Stop, and open the monitor |
| <kbd>V</kbd> | Show the Vera X's screen as text |
| <kbd>P</kbd> | Save the Vera X's screen as a PNG picture |
| <kbd>H</kbd> | Help |
| <kbd>CTRL</kbd>-<kbd>A</kbd> | Send a <kbd>CTRL</kbd>-<kbd>A</kbd> to the Hydra |

## H.3 The monitor

The monitor (<kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>B</kbd>, or a `--break`) stops the whole Hydra, kernel and all,
and takes commands:

| Command | What it does |
|---|---|
| `c` | Continue |
| `n` [N] | Run the next N instructions (1), showing each |
| `r` | The registers |
| `t` | The tasks |
| `m` ADDR [N] [T] | N bytes (64) at ADDR, as task T sees memory (the running task's, without T) |
| `b` [SPEC] | Stop at SPEC; with none, list the stops |
| `d` N | Take away stop number N |
| `w` ADDR[:T] | Stop when ADDR is written (by task T) |
| `x` | Quit the emulator |

The debugger, `db` (Chapter 29), is for debugging a program; the monitor is for when it's HydraOS itself you need to
watch.

## H.4 The web emulator

The web emulator (section 2.1) has its choices on its page: the buttons **Power cycle**, **Reset**, **Pause** and
**Sound**; the speed (**Real time**, **2×**, **4×** or **As fast as it goes**); and the **Setup…** panel, which
chooses a Vera X, its keyboard and mouse, the clock chip, one to three RAM modules, and the SD cards.

## H.5 The PC tools

**hydrafs.js** makes and changes SD card images (section 2.6):

```text
node reborn/sim/tools/hydrafs.js COMMAND IMAGE [ARGS]
```

| Command | What it does |
|---|---|
| `mkfs IMAGE MB [LABEL] [-q] [-p FATMB]` | A new, empty image of MB megabytes. `-q`: as the Hydra's quick format makes one. `-p`: in a partition, after a FAT partition of FATMB megabytes |
| `info IMAGE` | The card's label, size and free space |
| `ls IMAGE [-l] [PATH]` | List a directory (`-l`: with the dates) |
| `put IMAGE FILE PATH` | Copy a file from the PC onto the card |
| `get IMAGE PATH FILE` | Copy a file from the card to the PC |
| `mkdir IMAGE PATH` | Make a directory |
| `rm IMAGE PATH` | Remove a file, or an empty directory |
| `import IMAGE FOLDER [PATH]` | Copy a whole folder from the PC onto the card |
| `check IMAGE` | Check the card for lost or doubly used space |

Paths on the card start at its root: `/games/star.fs` is `/sd/0/games/star.fs` on the Hydra. (In Git Bash, leave out
the first `/`.)

**sdwrite.js** writes an image onto a real SD card in your PC's card reader (section 2.6):

```text
node reborn/sim/tools/sdwrite.js --list
node reborn/sim/tools/sdwrite.js IMAGE DISK [--yes] [--force] [--no-verify] [--dry-run]
```

| Option | What it does |
|---|---|
| `--list` | List the PC's disks; the removable ones (an SD card is one) are marked |
| `--yes` | Don't ask before writing (normally it asks you to type the disk again) |
| `--force` | Allow a disk that isn't removable (an external drive; never the system's own) |
| `--no-verify` | Don't read the card back afterwards to check it |
| `--dry-run` | Make the checks, and say what it would do, but write nothing |

It needs an administrator's rights (on Linux and macOS, `sudo`), and refuses the disk your system is on, or one
smaller than the image.

**hydrapc.js** is the PC tool: a terminal for a real Hydra's serial port, which also serves a folder on the PC as
`/pc` (Chapter 5):

```text
node reborn/sim/tools/hydrapc.js PORT FOLDER [options]
```

| Option | What it does |
|---|---|
| `--list` | List the PC's serial ports |
| `--read-only` | The Hydra can read the folder, but not change it |
| `--baud N` | The serial line's speed (9600, the Hydra's when it starts) |
| `--log` | List the Hydra's requests for `/pc` as they're served |
| `--no-size` | Don't tell the Hydra the window's size |
| `--win32-input` | As the emulator's (H.1) |

Its keys: <kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>X</kbd> quits, <kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>L</kbd> turns the log
on or off, and <kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>CTRL</kbd>-<kbd>A</kbd> sends a <kbd>CTRL</kbd>-<kbd>A</kbd>. It needs
the `serialport` package: run `npm install` in `reborn/sim` once.

**hysong.js** turns a score (section 15.6) into a ZSM song on the PC, as `play -o` does on the Hydra, and can make a
VGM file too, to hear it on the PC:

```text
node reborn/sim/tools/hysong.js SCORE.mml SONG.zsm [--vgm SONG.vgm] [--quiet]
```
