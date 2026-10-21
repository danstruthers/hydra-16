# 8. The emulator and the PC tools

You can do everything in this book without a board: the **emulator** runs the Hydra-16 cycle by cycle on a PC, with the
same ROM images the board runs.  It's what the tests run on, and what the system was developed in.  This chapter is the
emulator (in a terminal and in a browser), the build, the tests, and the PC's tools for the real board: the PC tool
(a terminal and `/pc`), card images, and writing a card.

Everything here needs **Node.js 18 or later**; building the ROMs needs **cc65** too.

## Contents

1. [Running it](#running-it)
2. [The keys](#the-keys)
3. [The options](#the-options)
4. [The monitor](#the-monitor)
5. [Debugging a program](#debugging-a-program)
6. [The screen and the sound](#the-screen-and-the-sound)
7. [In a browser](#in-a-browser)
8. [What's modelled](#whats-modelled)
9. [Building the ROMs](#building-the-roms)
10. [The tests](#the-tests)
11. [The benchmarks](#the-benchmarks)
12. [The PC tools](#the-pc-tools)
13. [The emulator in danlang](#the-emulator-in-danlang)

---

## Running it

From `reborn/` (HydraOS) or `base/` (the base alone):

```
$ node sim/run.js -i
```

boots the system's images (`bin/bios_rockwell.bin`, or `bios_wdc.bin` with `--acia wdc`, and `bin/prom*.bin`) in **real time**, with your terminal as the Hydra's serial
console.  Without `-i`, it runs for a while (30 million cycles: 8.4 s of the Hydra's time, as fast as the PC can) and
**reports**: the console's output, each task's state, the longest stretches with interrupts off, the stacks' depths,
and the last instructions run.

```
$ node sim/run.js --cycles 20000000 --input 'ls /rom\r'
```

runs for 20 million cycles, typing `ls /rom` and Enter once the system's up, then reports.  `--input`'s text takes `\r`
(Enter) and `\w` (a pause of 2 million cycles), so a whole session can be scripted; the examples in this book were
captured this way.

`reborn/sim/run.js` is the base's (`base/sim/run.js`) run with HydraOS's folder, plus `/pc`.

---

## The keys

With `-i`, **Ctrl-A** and a key are the emulator's:

| Keys | What they do |
| :--- | :----------- |
| Ctrl-A x | Quit |
| Ctrl-A r | The reset button |
| Ctrl-A s | Show the state (the tasks, the registers) |
| Ctrl-A b | Stop, and the [monitor](#the-monitor) |
| Ctrl-A v | Show the Vera X's screen as text |
| Ctrl-A p | Save the Vera X's screen as a PNG |
| Ctrl-A h | Help |

Every other key goes to the Hydra.  The terminal's size is told to the Hydra as the PC tool tells it, and again as the
window changes.

---

## The options

| Option | What it does |
| :----- | :----------- |
| `-i`, `--interactive` | The terminal is the serial console, in real time |
| `--speed N` | With `-i`: N times real time (0: as fast as it goes) |
| `--cycles N` | Stop at cycle N (default 30,000,000; with `-i`, never) |
| `--input TEXT` | Keys to type (`\r` Enter, `\w` a wait), one every 20,000 cycles from cycle 200,000; `--paste` as fast as the line goes |
| `--clock MHz` | The CPU's clock (3.579545; `7.15909` for a J8 board with faster parts than the stated ones, with a `build.js --clock 2` build: the stated parts run at 3.58 MHz or lower) |
| `--acia wdc` | A WDC W65C51N in the serial port, and its BIOS ROM (`bin/bios_wdc.bin`) |
| `--modules N` | Memory cards installed (default 2) |
| `--seed N` | The power-up's random RAM and registers, repeatable |
| `--sd FILE` | A card image in SD device 0 (again for 1, 2 ...), read and written in the file itself.  With none, the system's own card (HydraOS's `bin/sdcard.img`), its writes kept in memory; `--no-sd`: no card |
| `--pc-dir DIR` | (HydraOS) `/pc` is the folder DIR: the emulator plays the PC tool's part.  `--pc-read-only`, `--pc-log` |
| `--vera [V]` | A Vera X in slot 0 (its gateware version: 47.0.2 by default); `--vera-sd FILE` a card in its SD slot; `--vera-config MS` its start-up time |
| `--smc` | The Vera X's input controller, with a PS/2 keyboard and mouse; `--kbd TEXT` keys typed on it |
| `--screen` | After the report, the Vera X's text screen as text; `--frame-png FILE` the screen as a PNG |
| `--view [PORT]` | With `-i`: the Vera X's screen live in a browser (http://localhost:8016), its keys and mouse the controller's |
| `--sound [PORT]` | With `-i`: the sound in a browser (its Sound button); `--wav FILE` into a WAV file |
| `--bios FILE`, `--prom FILE` | Other images |
| `--trace N` | The last N instructions in the report (25) |
| `--trace-calls [L]` | Log each system call a program makes, by name, with its registers and what it gives back (L: only these calls or tasks) |
| `--break SPEC` | Stop before an instruction: a kernel label (`K_OPEN`), a module's (`rc:main`), `page:ADDR`, or `ADDR` |
| `--watch ADDR[:T]`, `--watch-read ADDR[:T]`, `--watch-pc ADDR` | Log each write (or read) of an address, by any task or task T's; each time the PC reaches an address |
| `--log FILE` | The log's lines into a file |

The top of `base/sim/run.js` describes every option.

---

## The monitor

**Ctrl-A b** (or a `--break` reached) stops the Hydra and gives a command line of the emulator's own:

| Command | What it does |
| :------ | :----------- |
| `c` | Continue |
| `n [N]` | The next N instructions, each shown |
| `r` | The registers |
| `t` | The tasks |
| `m ADDR [N] [T]` | N bytes (64) at ADDR, in task T's view (the running task's) |
| `b [SPEC]` | A break (none: the list) |
| `d N` | The Nth break gone |
| `w ADDR[:T]` | A write watch |
| `x` | Quit |

---

## Debugging a program

The emulator sees everything, so it's the easiest place to find a bug:

* **`--trace-calls`** logs every system call by name, its arguments (a string's text) and what it gave back: a program
  that fails with `not found` shows which `OPEN` failed and on what name.  `--trace-calls OPEN,t4` traces only `OPEN`,
  only task 4's.
* **`--break rc:main`** stops at a module's label (the build's label files name every routine), and the monitor steps
  from there.
* **`--watch`** catches whatever writes an address: a variable trampled, a bank register changed.
* On the Hydra itself, `db` does the same for a program's own code ([db](07-tools.md#db-the-debugger)).

---

## The screen and the sound

* **`--vera`** puts a Vera X in slot 0, and **`--smc`** its keyboard and mouse controller.
* **`-i --view`** shows the screen live at http://localhost:8016, its keys and mouse the controller's, while the
  terminal stays the serial console.
* **`-i --sound`** plays the sound in a browser (the same page as `--view`'s; press its Sound button), some 0.15 s
  behind: the YM2151 (a sample-exact port of ymfm), and with `--vera` the PSG and the PCM.  **`--wav FILE`** records it.
* **`--screen`** prints the text screen after a run, and **`--frame-png FILE`** saves it.

```
$ node sim/run.js -i --vera --smc --view --sound
```

is the whole machine: open http://localhost:8016 for the screen and sound, and type in either window.

---

## In a browser

```
$ node sim/web.js
```

builds **`bin/hydra-16.html`**: one file holding the page, the emulator (in a Web Worker) and the ROM images, which
runs HydraOS in Chrome, Edge, Firefox or Safari with nothing installed and nothing sent anywhere, opened straight from
the disk.  `build.js` makes it too, after the images, and it's in Git with them (the same images make the same page:
it names them by their checksum, not the commit), so it's there ready made, with no Node.js.  `--serve` serves it
at http://localhost:8017.

* The serial console is a terminal in the page, with the Vera X's screen beside it.
* **Setup** has the Vera X card, its keyboard and mouse, the clock chip, the memory cards, and SD cards: HydraOS's card
  as it starts, a new blank one, or an image loaded from the PC.  Cards are kept in the browser, and saved as image
  files.
* The page is one file: copy it anywhere, or put it on a web server.

---

## What's modelled

The emulator (`base/sim/lib/machine.js` and its parts) is the board, cycle by cycle:

* the W65C02S, every instruction and its cycles;
* `T`, `U`, `V` and `W`, random at power-up as the latches are; each task's `$00`, `$01` and RAM;
* the RAM window's task banks (only the memory cards installed: `--modules N`; the rest float) and the shared banks;
* the paged ROM with its halves swapped and the board's crossed bank bits; the BIOS ROM's pages;
* the I/O ports and the system port; the interrupt lines, their priority and the vector RAM (`n ^ 7`);
* the VIA: its timers, port A's pins and CA1, I2C devices on PA0/PA1, SD cards and other devices on its SPI port;
* the ACIA (Rockwell's, or `--acia wdc`), the YM2151 (its timers, its busy time, and with `--sound` its sound);
* a DS1747 in U7; the reset button (Ctrl-A r);
* the Vera X: its registers, VRAM, the layers, sprites and collisions, the scan's timing and interrupts, FX, the PCM FIFO,
  the PSG, its SD card, and the input controller.

Not modelled: the analog side (the audio path's filters, the serial line's levels), wait states (the board has none),
and DMA.

---

## Building the ROMs

```
$ cd reborn
$ node build.js
```

builds the base first, then HydraOS: the BIOS ROM, one for each serial chip (`bin/bios_rockwell.bin` and
`bin/bios_wdc.bin`: the kernel is built twice), the paged ROM's chips (`bin/prom0.bin` ...: the same for both), the ROM
disk, the SD card's image (`bin/sdcard.img`), the SDKs (`bin/sdk`), and a report of each part's size and room left.  It
needs cc65's `ca65` and `ld65` (and `cc65` and `ar65` for the C programs), found in `$CC65_BIN`, `$CC65_HOME/bin`, on
Windows in the repository (`base/tools/cc65/win64_hydra`: cc65 with the project's four fixes; its README says how to
build it elsewhere), or on the PATH.

| Command | What it builds |
| :------ | :------------- |
| `node build.js` | Everything, for a 3.58 MHz board (a BIOS ROM for each ACIA, Rockwell's and WDC's) |
| `node build.js --clock 2` | For a 7.16 MHz board (jumper J8): one with faster parts than the stated ones, which run at 3.58 MHz or lower |
| `node build.js --acia wdc` | The same images, with `obj/` (the labels, the objects the tests use) the WDC build's, not the Rockwell's: `sim/test.js` then runs the tests on a W65C51N |
| `node build.js prog DIR` | A program of your own: `DIR/*.s` (or `*.c`) into `DIR/NAME.hyx` ([programming](09-programming/README.md)) |
| `node ../base/build.js` | The base alone (from `reborn/`), or `node build.js` in `base/` |

The images in `bin/` are in Git, so the emulator and the chips need no toolchain.  A change to what's in them is
committed with them rebuilt.

---

## The tests

```
$ node sim/test.js
```

runs the base's tests and HydraOS's: each boots its own ROM image (the system's modules and a test module as init), runs,
and is judged on its output (`ok` and `not ok` lines, and `PASS`), its time budgets (the cycles between its marks), the
longest stretch with interrupts off, and its own checks.  They run as many at a time as the PC has cores.

| Command | What it does |
| :------ | :----------- |
| `node sim/test.js` | All of them |
| `node sim/test.js irq -v` | One test, with its output |
| `node sim/test.js --build` | Build first |
| `node sim/test.js -j 1` | One at a time |
| `node sim/test.js --dl` | In the danlang emulator ([below](#the-emulator-in-danlang)) |

The tests are in `base/tests/tests.js` and `reborn/tests/tests.js`; HyForth's (the Forth 2012 suite), hylang's (danlang's
suite) and HyBASIC's suites run among them.

---

## The benchmarks

```
$ node sim/bench.js
```

runs the twenty benchmarks in HyForth, hylang and HyBASIC (and HyBASIC's inline assembly) in the emulator, checks every
result is the same, and prints a table of their times by kind ([how fast](06-languages/README.md#how-fast)).
`--quick` uses the small sizes; `--only NAME,...` and `--kind KIND,...` pick some.  On the board:
`forth /sd/0/bench/bench.fs`, `hylang /sd/0/bench/bench.hl`, `basic /sd/0/bench/bench.bas`.

---

## The PC tools

In `reborn/sim/tools/`:

| Tool | What it does |
| :--- | :----------- |
| `hydrapc.js` | **The PC tool**: a terminal on the Hydra's serial port that also serves a folder of the PC as `/pc` ([/pc](04-drivers/serial.md#pc-a-folder-on-the-pc)) |
| `hydrafs.js` | **Card images**: make one, list it, put files in and get them out |
| `sdwrite.js` | **Write an image onto a card** in the PC's reader |
| `hysong.js` | The score language into a ZSM song (and a VGM, to hear on the PC) |
| `pcfs.js` | `/pc`'s file server (the PC tool's, and the emulator's) |

### hydrapc.js: the terminal and /pc

```
$ cd reborn/sim; npm install; cd ..                    # once: the serialport package
$ node sim/tools/hydrapc.js --list                    # the PC's serial ports
$ node sim/tools/hydrapc.js COM3 work                 # a terminal on COM3, and the folder work as /pc
```

`--baud N` sets the rate (9600 as the Hydra starts), `--read-only` keeps the folder from being changed, `--log` lists
`/pc`'s requests (and the frames from the Hydra that came damaged or stopped part-way, and the replies it asked for
again: a bad line shows there; Ctrl-A l turns it on and off).  Ctrl-A s says the counts so far, and quitting says them too.  Ctrl-A x
quits.  It tells the Hydra its window's size, so the Hydra's windows fit it.  In Windows
Terminal, `--win32-input` lets Ctrl-Tab reach the Hydra.

### hydrafs.js: card images

```
$ node sim/tools/hydrafs.js mkfs card.img 16 MYCARD   # a new 16 MB HydraFS image, labelled
$ node sim/tools/hydrafs.js put card.img hello.hyx bin # a PC file into the image's /bin
$ node sim/tools/hydrafs.js import card.img myfolder   # a whole folder tree
$ node sim/tools/hydrafs.js ls card.img -l bin
$ node sim/tools/hydrafs.js get card.img notes.txt notes.txt
$ node sim/tools/hydrafs.js check card.img
```

`mkfs ... -p FATMB` puts the HydraFS in a partition after a FAT one, so the card works in a PC too.  Paths start at the
card's root (in Git Bash, leave the first `/` off: it turns into a Windows path).  An image is what `--sd` takes in the
emulator, and what `sdwrite.js` writes to a card.

### sdwrite.js: an image onto a card

```
$ node sim/tools/sdwrite.js --list                    # the PC's disks; the removable ones marked
$ node sim/tools/sdwrite.js bin/sdcard.img 3          # the image onto disk 3 (Windows' number)
```

It writes the image to the card's first blocks, raw, then reads them back and compares.  It asks for the disk's number
again before writing; it **refuses** the disk the system is on, one smaller than the image, and one that isn't removable
(without `--force`).  It needs an administrator's prompt on Windows, root on Linux (`/dev/sdb`, the card's partitions
unmounted), an administrator on macOS (`/dev/disk4`).  `--dry-run` shows what it would do.

---

## The emulator in danlang

`reborn/sim/dl/` is the whole emulator written again in **danlang** (the language hylang is), run by danlang's C#
interpreter: the CPU, the memory map, the ACIA, the VIA, the YM2151, the SD cards, the I2C bus, the DS1747, the Vera X and
its input controller.  It runs every
test (`node sim/test.js --dl`) and the serial console (`danlang sim/dl/run.dl -i`), about seven times slower than the
JavaScript one.  It's a test of danlang, and of the emulator, by writing it twice.
