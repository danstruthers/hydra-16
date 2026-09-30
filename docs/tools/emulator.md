## **The emulator and tools**

A minimal Hydra-16 emulator for debugging the OS ROM without the hardware. It boots the real ROM images
built by `os_rom/makeC02.bat` (`os_rom/bin/os_rom_C02.bin` and `os_rom/bin/paged_rom_C02.bin`).

Requires [Node.js](https://nodejs.org). No other dependencies.  The emulator, the regression tests and the card tool are in `sim/`; the commands below are run from there.

### **Using the Hydra from your terminal**

```
node hydrasim.js -i
node hydrasim.js -i --sd card.img        with an SD card (a file: see tools/hydrafs.js to make one)
```

`-i` (`--interactive`) makes the terminal the Hydra's serial terminal.
- **What happens:** it boots (POST, then the HyForth prompt) and runs in real time. What you type goes to the serial port and the output comes straight back, so it's like sitting at the board.
- **Keys:** the console keys work as on the board: Ctrl-C breaks, Ctrl-\\ restarts the shell, Ctrl-] then a task number switches tasks, Ctrl-D ends input. `bye` leaves HyForth for WOZMON.
- **Emulator commands** (Ctrl-A is the emulator's own prefix key, as in QEMU or `screen`):

| Keys | Does |
| :--- | :--- |
| Ctrl-A x | Quit |
| Ctrl-A r | Press the reset button (RAM and the SD cards keep their contents, as on the board) |
| Ctrl-A s | Show the state: time, task, ROM page, PC |
| Ctrl-A h | List these |
| Ctrl-A Ctrl-A | Type a Ctrl-A |

- **Serial speed:** output arrives at the Hydra's serial rate, as on the board: 9600 baud at boot, about 930 characters a second.  `q^b115200^ stty` speeds it up; the emulated terminal follows any rate and format, so nothing needs switching.
- **Speed:** `--speed N` runs N times real time (`--speed 0`: as fast as the PC can go, about 20 times). Timings the Hydra shows (`sleep`, the test tune's tempo) keep the Hydra's time either way.
- **Other options:** most options below work too, e.g. `--modules`, `--acia wdc`, `--seed`. `--cycles` stops it after that many cycles.
- **Piped input:** input can be piped in, e.g. `printf '1 2 + .\n' | node hydrasim.js -i`. Line ends become Enter, and it stops 3 seconds after the input runs out.
- **Not modelled:** there's no sound; the YM2151 is timed but silent, so the bell only reaches you through the terminal's own BEL.

### **Usage**

```
node hydrasim.js [options]
```

| Option | Description |
| :----- | :---------- |
| `-i`, `--interactive` | Use the Hydra from the terminal, in real time (above) |
| `--speed N` | Interactive: N times real time (default 1; 0 = as fast as it goes) |
| `--paste` | Type the input at the ACIA's full line rate, back to back like a paste, whether the ROM keeps up or not: bytes that arrive while the last one is still unread are lost, as on the chip, and counted in the report (default: each key waits until the ROM has read the last) |
| `--rom DIR` | ROM images directory (default: `../os_rom/bin`) |
| `--cycles N` | CPU cycles to run (default 20,000,000; about 5.6 seconds at 3.58 MHz) |
| `--input TEXT` | Serial input to type, from cycle 200,000 on, a key every 20,000 cycles; `\r` = CR, `\xNN` = the byte NN (e.g. `\x03` = Ctrl-C), `\w` = wait 2M cycles before the next key (booting to the HyForth prompt takes about 0.9M cycles, so start with one) |
| `--clock 3.58\|7.16` | The CPU clock in MHz, as the ROM was built for (`CPU_CLOCK_MULT` in `os_rom/include/hw.inc`; default 3.58).  It sets the ACIA's and the YM2151's timing in CPU cycles, and the seconds in the report |
| `--modules N` | RAM modules installed: banks `$00` to `N*16-1` (default 3) |
| `--shared-u N` | Shared RAM installed for `U` macro-pages 0 to N-1 (default 16; each 512K chip is 4 macro-pages) |
| `--acia-line N` | IRQ line the ACIA interrupts on (default 1) |
| `--acia rockwell\|wdc` | The ACIA chip: the Rockwell R65C51 (default), or the WDC W65C51N with its transmitter bug (TDRE always reads 1, no TX interrupt; for a ROM built with `SER_ACIA = SER_ACIA_WDC`, which paces sending with VIA timer 2).  In WDC mode the emulator counts bytes written while one is still being sent (they'd be garbled on the chip) and reports them at the end.  The report also gives the shortest idle time on the line between characters sent, in bits, since the rate was last set (0: back to back)  (VIA timer 2 is modelled too: one-shot) |
| `--stuck-irq N` | Hold IRQ line N active the whole time |
| `--sd [N:]FILE[@B]` | An SD card (SDHC) on SPI device N (0-7, the board's SPI headers J18-J25; default 0), backed by the image FILE (512-byte blocks; writes go to the file).  Up to 8 cards, one per device, e.g. `--sd card0.img --sd 3:C:/images/card3.img`.  `@B`: the card says it has B blocks, more than the file (a big card from a small file: blocks past the file's end read as zeros, and writing one makes the file longer), e.g. `--sd card.img@500170752` for a 244 GB card.  Models the VIA's port B SPI bit by bit (device select as the board's 74HC138 does it), and the SD commands the ROM uses (CMD0, 8, 9, 16, 17, 24, 55, 58, ACMD41; CMD9's CSD gives the image's size) |
| `--sdsc N` | Make the card on device N a standard capacity one (SDSC): byte addresses, and a v1 CSD register |
| `--ram-fault BANK:An:high\|low` | Address line An (0-12) stuck high or low on the RAM chip holding BANK (a shared chip holds 4 bank IDs, e.g. `F0-F3`; a task RAM module 16 banks), e.g. `F0:A0:high`.  The POST `RAM` line should report it |
| `--model M` | Hardware what-ifs: `sharedlow`, `nostack`, `zponly`, `noshared` |
| `--raw` | Print serial output as-is (by default ESC shows as `<ESC>`) |
| `--trace N` | Show the last N instructions (default 25) |
| `--dump ADDR[:LEN][@TASK]` | Hex dump task RAM after the run, e.g. `--dump 7D90:16@1` |
| `--watch ADDR[@TASK]` | Report every write to a task RAM address: the old and new value, and the PC that wrote it |
| `--mark TEXT` | Report the cycle each time the serial output ends with `TEXT` (`\r` = CR), e.g. `--mark "/> "` to time a command from prompt to prompt |
| `--profile N` | From cycle `N` on, count the instructions each task runs in each routine (named from the build's debug info, `os_rom/obj/os_rom_C02.dbg`), and report the top 30, e.g. `--profile 2800000 --input '\wwords \| wc . . .\r'` |
| `--ym-log` | List every YM2151 key-on (channel and cycle) in the report, not just the first 8.  The report also gives the longest gap between key-ons and the time from the first to the last (a late note shows as a long gap) |
| `--seed N` | Power up RAM and the pseudo-registers from random number seed `N`, so a run repeats exactly (by default each run powers up differently) |
| `--pc [PAGE:]ADDR` | Report the registers each time the PC reaches `ADDR` (on BIOS ROM page `PAGE`, if given); addresses are in `os_rom/obj/os_rom_C02.lbl` |

Example: boot to Forth and run a command (Forth starts after `COPYTORAM`, so allow plenty of cycles):

```
node hydrasim.js --cycles 60000000 --input "1 2 + .\r"
```

The report shows the serial output, the last instructions executed (`W T PC A X Y S P`), the hottest PCs
(a stuck loop shows up at the top), the longest stretches with IRQs off from the first key typed (where
they start and end: what holds off the serial port), each task's lowest stack pointer (its free stack bytes, and the `W:PC`
that got it there), and the final pseudo-register and vector RAM state.

### **Regression tests**

`regress.js` boots the ROM in the emulator once per test, types each test's input, and checks the serial
output for what it expects: POST, the self tests (MMU, scheduler, IO; also with 1 RAM module and 1 shared
macro-page), POST with hardware faults, the hardware test (all of it; and from POST, with faults injected), HyForth and its libraries, pipelines, files and namespaces, tasks and console
switching, Ctrl-C, background sound and the bell, `sleep`, the serial settings, `/dev/sd` (on a blank card
image; also two shells reading it at once), HydraFS reading, writing, checking and quick formatting (on fixture card images kept in `sim/cards`, and on cards made by `tools/hydrafs.js`, and checked with it afterwards), partitions, the clock (`/dev/time`) and files' stamps, sparse files, and the shell: the volume chosen at boot, `boot.hys`, `cd`, the prompt, the file commands, `include`, running programs (`.hyx` executables and `.hys` scripts, by name and from `/bin`, their arguments, Ctrl-C), redirection, `echo`, the editor, and each task's environment (`/env`, `PATH`, `HOME`, `/dev/proc`).  One test runs a small program of its own instead of the ROM, and
checks the CPU's cycle counts against WDC's table.  Four watch timing: a 1000-character paste at 57600 with
nothing lost, console output at 115200 inside a cycle budget, SD read throughput inside a cycles-a-byte
budget, and a limit on how long the ROM ever holds interrupts off.  The emulators run in parallel; the whole
set takes about 10 seconds.

```
node regress.js              all the tests (exit code 1 if any fails)
node regress.js pipes io     only the tests whose names contain "pipes" or "io"
node regress.js --list       what each test checks
node regress.js --random     a new random power-up each run (default: --seed 1, so runs repeat exactly)
node regress.js --verbose    show every test's serial output, not just the failures'
```

Or from `os_rom`: `makeC02 test` builds the ROM and then runs them.  A failure shows what was missing (or
found when it shouldn't be), the `hydrasim.js` command that reproduces it, and the serial output.  Every
test also fails if a task's stack got within 32 bytes of its bottom, and the summary shows the deepest stack
of the run (about 70 of the 256 bytes so far, with IRQ frames on top of far calls).  To add a
test, add an entry to the `TESTS` list at the top of `regress.js` (its header describes the fields).  A test
that wants SD cards lists them under `sd`; a card with `hfs` gets a HydraFS made on it (`quick`: as the
Hydra's quick format makes one), and the function is handed the volume (the `Volume` class below) to put
files in; `claim` makes a card say it's bigger than its image (`--sd FILE@B`); `image` starts a card from one
of the fixture images in `sim/cards` (a copy), cards made before that the ROM must go on reading ([their
README](../../sim/cards/README.md)).

### **HydraFS card images**

`tools/hydrafs.js` makes and reads HydraFS images (the Hydra's SD card filesystem, [plans/HYDRAFS.md](../plans/HYDRAFS.md)) on
the PC, for the emulator's `--sd` or for writing to a real card with a disk imager:

```
node tools/hydrafs.js mkfs card.img 64 GAMES      a new, empty 64 MB image (-q: as the Hydra's quick format)
node tools/hydrafs.js mkfs card.img 64 GAMES -p 32   ... in a partition, after a 32 MB FAT one (unformatted)
node tools/hydrafs.js import card.img myfiles     copy a folder tree in
node tools/hydrafs.js put card.img star.frt games copy a file into /games
node tools/hydrafs.js ls card.img games           list a directory (ls card.img -l games: with dates)
node tools/hydrafs.js get card.img games/star.frt star.frt
node tools/hydrafs.js check card.img              check the free map against the files
```

The Hydra reads the same images through its `hfs` server ([io.md](../programming/io.md#the-files-on-a-card)):
`node hydrasim.js -i --sd card.img` boots with the card's root as the current directory (`0:/> `), and `ls`
lists it.

`node tools/hydrafs.js` alone lists every command.  Card paths start at the card's root; in Git Bash, leave
out their first `/` (Git Bash turns `/games` into a Windows path).  From Node, `require('./tools/hydrafs.js')`
gives `mkfs` and `Volume`.

### **Hydra executables**

`tools/mkhyx.js` puts the 16-byte `.hyx` header on a raw binary linked for a fixed address, so the shell can
run it ([programs.md](../programming/programs.md)).  (Programs built with ca65 and `programs/hyx.cfg` have
the header already.)

```
node tools/mkhyx.js prog.bin prog.hyx $0800        the header on a binary (entry point: the load address)
node tools/mkhyx.js prog.bin prog.hyx $0800 $0810  ... with another entry point
node tools/mkhyx.js --info prog.hyx                show a .hyx file's header
```

From Node, `require('./tools/mkhyx.js')` gives `hyx(load, code, entry)`, which `regress.js` uses for its test
programs.

### **What it models**

* W65C02S CPU, including the WDC additions (`STZ`, `BRA`, `PHX`/`PLY`, `TSB`/`TRB`, `BBR`/`BBS`, `RMB`/`SMB`,
  `WAI`, `STP`, `(zp)` addressing, the 1-byte NOPs).  Cycles are counted from WDC's table, with its extras:
  +1 for an indexed read that crosses a page, +1 for a branch taken and +1 more if it lands on another page,
  +1 for `ADC`/`SBC` in decimal mode, and 7 for an interrupt.  The board has no wait states (`RDY` only has a
  pull-up), so every access is at full speed.  A read or write of an I/O device happens at the instruction's
  last cycle (the devices are brought up to that cycle first), and `WAI` sleeps until the next device event
  (which also makes idle time fast to simulate).
* `T`: each task has its own `$0000-$7FFF` (zero page, stack, task RAM) and `$00`/`$01` bank registers.
* RAM bank window `$8000-$9FFF`: banks `$00-$EF` per task (only the installed modules; others read back
  floating-bus values), banks `$F0-$FF` shared, 16 macro-pages selected by `U`.
* Paged ROM `$A000-$DFFF` (16K banks selected by `$01`), including the board's A13 half-swap: `$A000` reads
  ROM offset `$2000`, `$C000` reads ROM offset `$0000`.
* BIOS ROM `$E000-$FFFF` in 8K pages selected by `W`; I/O at `$FF00-$FFEF`; `T`/`U`/`V`/`W` at `$FFF0-$FFF3`.
* IRQ vector RAM at `$FFFE`/`$FFFF`: written at index `V[0..3]`, read at index `IRQ_NUMBER(n)` (`n ^ 7`)
  of the lowest active IRQ line, or `V[0..3]` when no line is active (and for `BRK`).
* Rockwell 65C51 ACIA at `$FF10` on IRQ line 1: transmit and receive with interrupts, output captured,
  input from `--input`.  A character takes the time set by the registers the ROM programs: the baud rate
  (from its clock, the board's `SER_CLK`, 1.790 MHz, so every rate is 2.9% slow as on the board; or that
  clock / 16 for 115200), the word length, parity and stop bits.  A programmed reset (a status write)
  clears the command register's bits 0-4.  The transmit interrupt comes as TDRE goes on (when a byte
  has gone), as on the board: turning it on while TDRE is already on doesn't interrupt.
* VIA timer 1 (one-shot and free-running, latches, interrupt flag and enable registers) on IRQ line 0: the
  scheduler's tick; timer 2 (one-shot); the shift register's timing and flag (its CB1/CB2 lines aren't
  brought out: shifting in reads 1s); port B as the SPI bus (see `--sd`); port A's inputs read high (the
  I2C bus's pull-ups; no I2C devices).  The handshake lines aren't modelled.
* YM2151: busy (status bit 7) for 64 of its clocks (3.58 MHz) after each data write.  A write while it's busy
  would be lost on the chip: the report counts them.  Key-ons are reported (`--ym-log`).  Its timers A and B
  (registers `$10-$14`): an enabled timer's overflow sets its status flag (bits 0, 1), which holds IRQ
  line 4 until it's reset.
* RAM and the pseudo-registers power up with random values, like the hardware.

It is a model, not the hardware: anything it doesn't simulate (the YM2151's sound, the SD card's
own delays, card slots, the timing of each bus cycle inside an instruction) can still behave differently on
the board.
