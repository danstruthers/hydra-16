## **Next steps: making the Hydra fun and useful**

What the Hydra-16 has, what's missing for hobbyists and programmers, and an order to do it in.  It builds on [IDEAS.md](IDEAS.md) (the running list), [VIDEO.md](VIDEO.md) (the Vera X card) and the [code review](CODE_REVIEW.md).

### **Contents**
1. [Where it stands](#where-it-stands)
2. [What a newcomer runs into](#what-a-newcomer-runs-into)
3. [The gaps, by theme](#the-gaps-by-theme)
4. [Milestones](#milestones)

---

### **Where it stands**

The Hydra already does more than most homebrew 65C02 machines:
* **The system:**
  * 16 hardware tasks, preemptive scheduling, per-task memory and shared memory;
  * Plan 9-style IO: files, pipes, namespaces, `/env`, `/proc`, exit statuses, background jobs, semaphores.
* **Files:** SD cards with a real filesystem (HydraFS: directories, sparse files, a checker, partitions), a clock chip, and files built into the ROM (`/rom`).
* **The shell and languages:**
  * a shell that's also a language (HyForth: pipelines, redirection, scripts, libraries from files);
  * a C toolchain (cc65, with a library for the Hydra's calls and a guide);
  * a line editor;
  * WOZMON with a disassembler.
* **Sound:** the YM2151, with a library, `/dev/snd`, a ZSM song player and a score language.
* **Tools:** an emulator that runs the real ROM, 74 regression tests, a hardware test, and documentation for every part.

What's missing is mostly **on-ramps** (ways to start without the author's setup) and **things to make** (a screen, input, games, hardware projects, languages beginners know).

---

### **What a newcomer runs into**

| A newcomer wants to ... | Today | What would help |
| :---------------------- | :---- | :-------------- |
| Try it without building a board | Install Node and cc65, clone, build, run the emulator in a terminal | **A web emulator**: a page that boots the ROM, with a sample card |
| Build it on Linux or macOS | Windows `.bat` files only | **`build.js`** for any OS; CI; release images |
| Plug in a monitor and keyboard | A serial terminal is the only console | **Vera X** video and a keyboard ([VIDEO.md](VIDEO.md)) |
| Get programs onto it | Take the SD card out and copy with `hydrafs.js` | **A PC folder as a filesystem over serial** (`/pc`), then XMODEM |
| Type a first Forth program | `: x 65 . ;` silently breaks, and `.` prints hex | **HyForth fixes**: literals compiled, decimal output, history ([review](CODE_REVIEW.md#hyforth)) |
| Write BASIC | No BASIC | **hy-basic**, adapted to run as a program |
| Blink an LED, read a sensor | The VIA's port A and I2C are on a header, but there's no driver | **`/dev/i2c`, `/dev/spi`, `/dev/gpio`** |
| Make a game | No graphics, no joystick | Vera X sprites and tiles, pads, a small game library |
| Learn how it works | Complete references, which assume a lot | **Tutorials** first, then the references |

---

### **The gaps, by theme**

Sizes are rough: **S** a few days, **M** a few weeks, **L** longer.

#### **1. Easy to try and to build**

* **A web emulator (M).**  `hydrasim.js` is JavaScript already.  Separate its core from Node ([review](CODE_REVIEW.md#the-emulator)), and serve:
  * the ROM images and a sample card image in a static page, with xterm.js as the terminal;
  * later, the VERA's canvas.

  Host it with the project's GitHub Pages.  This is the single biggest way to reach people.
* **`build.js` and CI (S-M).**  One build script for any OS, finding cc65 by `CC65_HOME` or the `PATH`.  A GitHub Actions job builds and runs the tests on every push.  Tagged releases carry the ROM images, a card image, and the C library.
* **A starter card image (S).**  A HydraFS image with:
  * `boot.hys` and `PATH`/`HOME` set up;
  * the samples in `/bin`, libraries in `/lib` and songs in `/songs`;
  * the tutorials' files.

  Make it from a folder in the repository with `hydrafs.js import`, at every build.
* **Tutorials (M, ongoing).**  Before the references:
  * the first hour with HyForth;
  * a first C program, from the PC to the card to running it;
  * a first assembly program;
  * writing a small device server;
  * once Vera X exists, the first sprite.
* **A contributor's guide (S).**  `CONTRIBUTING.md`:
  * the code style: the column layout, `IN:`/`OUT:` headers, CRLF;
  * the recipes: how to add a thunk, a gate, a server, a HyForth word or a test;
  * the rules: `board/` is read-only, run the tests before a push.

  Much of it is in the Programmer's Guide already; this gathers the rules in one place.

#### **2. A standalone computer**

* **Video: the Vera X card (L).**  640x480 VGA, text and tiles, 128 sprites, the screen as the console ([VIDEO.md](VIDEO.md)).
* **Keyboard, mouse and game pads (M).**  Vera X's input controller on IRQ line 3, over I2C ([VIDEO.md](VIDEO.md#the-card)).
* **Files from the PC without the card (M).**  Two ways, both worth having:
  * **XMODEM** ([IDEAS.md](IDEAS.md) item 6): send and receive single files with any terminal program.
  * **`/pc`**: a small Node program on the PC serves a folder over the serial port, and the Hydra mounts it as a file server (9P-style requests, which is what the IO layer speaks inside).  Then `cc65` output is runnable at once (`/pc/bin/game`), with no copying.  The emulator can serve a folder the same way.  For a programmer this is the biggest workflow win.
* **A full-screen editor (M).**  `edit` is a line editor.  A small screen editor (nano-like: arrows, insert, search, save) using conio works on a terminal today and on the Vera X screen later.

#### **3. Languages and tools on the machine**

* **HyForth fixes (S-M):** *(Done: [CODE_REVIEW.md](CODE_REVIEW.md#what-was-done).)*
  * numbers compiled inside definitions;
  * decimal output (`decimal`, `hex`, `u.`);
  * line history and editing;
  * readable error messages;
  * `syscall` fixed ([review](CODE_REVIEW.md#bugs)).
* **BASIC: EhyBASIC (M).**  What most hobbyists expect, and there's one already: EhyBASIC ([burntcouch/ehybasic](https://github.com/burntcouch/ehybasic)), Microsoft BASIC from the mist64/msbasic and Ben Eater line with Applesoft touches, built today as a paged ROM image started from WOZMON (`A000R`) against the 1.8Ce ROM.  To be adapted to the system as it is now: its zero page into what a program has (`$E0-$FF`: Microsoft BASIC uses well over 100 bytes, `CHRGET` among them), its calls through the current thunks, and `EXIT` back to the shell.  As a `.hyx`:
  * `INPUT`/`PRINT` through stdio (so it works in pipelines and from scripts);
  * `LOAD`/`SAVE` on HydraFS files (and `/pc`'s, once it's there);
  * a `/rom/bin/basic`, so it's there with no card.

  Later, `SOUND`, `SPRITE` and `PLOT` statements that use `/dev/snd` and Vera X.
* **An assembler on the Hydra (M).**  A 65C02 assembler as a program (source file in, `.hyx` out), or as HyForth words (`code ... end-code`).  Then programs can be written without a PC at all.  WOZMON could get a one-line mini-assembler like the Apple II monitor's.
* **A debugger through `/proc` (M).**  Plan 9's way ([PROC.md](PROC.md)): `/proc/N/mem` (read and write a task's RAM), `/proc/N/regs` (its saved frame), `/proc/N/ctl` (`stop`, `start`, `step`, `break ADDR`).  A debugger is then just a program, in C or HyForth, and it debugs other tasks while the system runs.
* **Tools as programs (S each).**  [IDEAS.md](IDEAS.md) item 10, and more:
  * reading files: `more`, `head`, `tail`, `grep`, `xd` (a hex dump);
  * managing them: `cp -r`, `find`, `sort`, `diff`, `df`, `du`, `touch`;
  * the system: `date`, `which`, `top` (from `/dev/proc`).

  Most are small C programs.  They make good examples for newcomers and good `/rom/bin` residents.
* **More C (S-M):** `MM_ALLOC` from C (paged memory for bigger programs).  (`_stroserror` gives the OS's messages now.)

#### **4. Making things: hardware projects**

The board already has the connections: the VIA's port A on J27 (6 free pins and CA1/CA2), I2C on J27 and every slot, 8 SPI device headers, and 6 slots.  It needs drivers and examples, in this order:
* **`/dev/i2c` (M):** the bit-banged bus as a file per address (`/dev/i2c/50` an EEPROM, `/dev/i2c/68` a sensor), with reads and writes as transfers.  Vera X's input controller needs it anyway.
* **`/dev/spi` (S-M):** raw access to SPI devices 0-f (8-f are decoded by cards), for displays, ADCs and radio modules.  *(Built: `/dev/spi/N`, a transaction a write and the bytes that came back read after, modes 0 and 3: [io.md](../programming/io.md#spi-devices-devspi).)*
* **`/dev/gpio` (S):**  *(Built: [io.md](../programming/io.md#gpio-devgpio), and the [tutorial](../tutorial.md#7-an-led-and-a-button)'s LED and button.)*
  * `echo 1 > /dev/gpio/2` lights an LED;
  * `cat /dev/gpio/3` reads a button;
  * `/dev/gpio/ctl` sets directions;
  * CA1 can be an interrupt (`/dev/gpio/ca1`).
* **Networking (L).**  A WIZnet W5500 module (Ethernet with its own TCP/IP, on SPI) on one of the SPI headers, served as Plan 9's `/net` (`/net/tcp/clone`, `ctl`, `data`).  Then:
  * telnet into the Hydra (a shell per connection: the Hydra has 16 tasks);
  * fetch files over HTTP;
  * an IRC client.

  An ESP32 modem card (a serial port in a slot with AT commands) is a simpler alternative.
* **A prototyping guide (S):** how to build a slot card, using the breakout card, what the bus timing is, and the open-collector rule for IRQs.  The [hardware reference](../hardware.md) has the facts; a guide turns them into a first project.

#### **5. Sound and music**

* **Importing music** from other machines (VGM, MIDI files, trackers) into ZSM: planned in [SOUND.md](SOUND.md).
* **The VERA's PSG and PCM** ([VIDEO.md](VIDEO.md)): 16 more voices, sampled sound, and X16 songs that play in full.
* **A tracker (M-L)** once there's a screen: write songs on the Hydra itself, saving ZSM.
* **MIDI (M):** a slot card with a UART at 31,250 baud, as `/dev/midi`; play the YM2151 from a keyboard, or record into the score language.

#### **6. Games and demos**

* **A game library (M)** for C and HyForth:
  * sprites, tiles, input and sound under one small API, in the manner of a fantasy console;
  * on Vera X, with `/dev/snd`;
  * a frame loop paced by the VERA's frame interrupt.
* **Samples to learn from and play:** Snake, Breakout, Tetris, a scrolling shooter, a Mandelbrot, a paint program, the music visualiser.  Each is a few hundred lines, and good in `/rom/bin`.
* **Multitasking is the Hydra's party trick:** a game runs while the music plays in another task and a shell stays at Ctrl-]'s reach.  Show it off in the demos.

#### **7. The OS**

From [IDEAS.md](IDEAS.md), still open:
* **Notes with handlers** (Plan 9's `notify`): a program catches Ctrl-C to clean up;
* **notices** when background tasks end, and `wait` for all of them;
* **`rename`** across directories.

New:
* **`/dev/sysname` and a version** (the build's stamp: [review](CODE_REVIEW.md#the-build));
* **`/dev/proc/N/ctl`** (`kill`, `stop`, `start`), on the way to the debugger above;
* **memory statistics** for `top`.

#### **8. Board V2**

[IDEAS.md](IDEAS.md) has the RDY wait states, and the V1 errata are listed in the [hardware reference](../hardware.md#v1-errata).  For a V2 wish list:
* **wait states**, so the CPU can run at 7-14 MHz with the YM2151;
* **a 1.8432 MHz clock for the ACIA**, so the standard baud rates are exact;
* **the bank register bits** in order;
* **the audio jack's channels** the usual way round;
* **a PS/2 or USB keyboard controller on the board** (or keep it on Vera X);
* **a footprint or header for the VERA module** on slot 0's position, if the card proves itself.

---

### **Milestones**

Each milestone leaves the project in a state worth showing.

**1. Easy to start** (mostly software, no new hardware):
1. *(Done with the code review's fixes: [CODE_REVIEW.md](CODE_REVIEW.md#what-was-done).)*  HyForth's fixes and `syscall`; `build.js` and CI; the ROM budget report and room on every page; the emulator in modules, with no Node.js in its core; the first tutorial ([tutorial.md](../tutorial.md)).
2. Releases with ROM and card images (a starter card), once the CI runs on GitHub.
3. The web emulator with the starter card.
4. `CONTRIBUTING.md`.

**2. Storage and names, then programming and making things** (the order set for it):
1. RAM and ROM disks ([DISKS.md](DISKS.md)).  *Done:* the paged ROM is the disk `x`, a HydraFS volume mounted at `/rom`; the RAM disks `r` and `s` (each shell's own area at `/ram`, and `/sram`), the tasks' areas, the program caches; the test song a file; with no card, `/ram` and `/rom/boot.hys`; `/dev/ram` for task 0.
2. Namespaces the Plan 9 way ([NAMESPACES.md](NAMESPACES.md)).  *Done:* 32 entries a task and 32 in the system namespace every task sees (`-s`), mounts with a spec, unions (`bind -a`, `-b`, `-c`) and their listings, `hide`, `unmount new old`, `ns` as `bind` lines; the default namespace (`/rom/lib/namespace`, a card's `lib/namespace`), `.` then `/bin` in place of search paths; C's `hy_bind` and the rest; `newns [file]`, a fresh namespace (C's `hy_newns`).
3. `/proc` ([PROC.md](PROC.md)).  *Done:* `/proc` mounted, `ns`, `pages`, `ctl` for the family only, and `/proc/N/cmd` (`send N line`: shell N runs it as if typed); `/proc/N/mem` and `/proc/N/ram`, a task's memory as files, for its family and task 0.  *To do:* `regs` and `fd`, then the debugger's `ctl` commands with the debugger.
4. `/dev/spi`.  *Done:* `/dev/spi/N` and its `ctl` (modes 0 and 3), shared with the SD cards (a device with a card started is busy, and a card isn't started on an open one).
5. `/dev/gpio`, with a hardware project tutorial.  *Done:* the pins (`/dev/gpio/N`, `port`), `ctl` (directions, CA1's edge, CA2), `/dev/gpio/ca1` (a read waits for CA1's edge: its interrupt), and the tutorial's LED and button.
6. **Next:** `/pc`: a PC folder over the serial port, then XMODEM.
7. Not yet placed in the order: EhyBASIC, adapted to the system (a `.hyx`, and `/rom/bin/basic`), and `/dev/i2c`.

**3. A computer on its own:**
1. The emulator's VERA, then the Vera X carrier card.
2. The screen console; `/dev/vid`; HyForth's `video` words with turtle graphics.
3. The input controller (on `/dev/i2c`), keyboard input.
4. A full-screen editor.
5. The game library and the first games; the VERA's PSG and PCM.
6. Tools as programs; an assembler.

**4. Connected:**
1. `/net` on a W5500; telnet and a file fetcher.
2. The debugger through `/proc`.
3. Music import; a tracker.
