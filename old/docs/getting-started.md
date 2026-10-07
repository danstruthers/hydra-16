## **Getting started**

How to build the ROMs, program the chips, connect a terminal and boot the Hydra-16, or run it in the emulator without the hardware.

### **What you need**

| For | Tool |
| :-- | :--- |
| Building the ROMs | [cc65](https://cc65.github.io/) (`ca65`, `ld65`) |
| The page checker, emulator, tests and card tool | [Node.js](https://nodejs.org) (no packages needed) |
| The board | [KiCad 9](https://www.kicad.org) to view or change the schematics and PCB (`board/`) |
| Real hardware | A chip programmer that handles SST39SF010/040 flash; a serial terminal (9600 8N1) with an RS-232 port or USB adapter |

### **The repository**

| Folder | What |
| :----- | :--- |
| `os_rom/` | The OS ROM: sources, build script, linker config, and the built images in `os_rom/bin/` |
| `sim/` | The emulator (`hydrasim.js`), the regression tests (`regress.js`), tools (`tools/hydrafs.js`, `tools/mkhyx.js`) |
| `programs/` | Programs for the Hydra (`.hyx`): C (`programs/c/`: the library and samples) and assembly (`programs/asm/`), and what to build others with |
| `board/` | KiCad schematics and PCBs: the main board, the memory daughter card, the bus breakout card |
| `docs/` | This documentation |

### **Building**

The build is `build.js`, at the top of the repository, on Windows, Linux or macOS.  It needs Node.js and cc65, which it finds in `CC65_HOME` (cc65's folder), or on the `PATH`, or in `C:\source\cc65\win64_snapshot`:

```
node build.js            everything: the C library and samples, the assembly sample, the ROM images
node build.js rom        just the ROM images
node build.js test       everything, then the regression tests
node build.js rom test   the ROM images, then the tests
```

`os_rom/makeC02.bat` (`makeC02`, `makeC02 test`) runs `build.js rom` too.  The ROM's steps, stopping at the first that fails:
1. `sim/tools/hysong.js` compiles the test song (`songs/test.mml`), and the version (`os_rom/VERSION`, shown at boot) goes into an include file, both in `obj/`.
2. `ca65` assembles `all.s` (which includes every source file) for the 65C02, and `ld65` links with `os_rom_C02.cfg`.
3. `sim/tools/mkromdisk.js` writes the ROM disk (`/rom`'s files, as a HydraFS volume) into the paged ROM image and reads every file back to check it, and `tools/romsum.js` adds the checksums the hardware test checks.
4. `tools/check_pages.js` checks for calls between BIOS ROM pages that bypass a gate (`No cross-page references`); one is an error.
5. `tools/rom_space.js` prints the space left on each BIOS ROM page, and warns when page 0 or COMMON is nearly full (`node tools/rom_space.js --table` in `os_rom` shows where).

GitHub runs `node build.js test` on every push (`.github/workflows/build.yml`).

| Output | What |
| :----- | :--- |
| `os_rom/bin/os_rom_C02.bin` | The BIOS ROM image (128K): burn it into U6 |
| `os_rom/bin/paged_rom_C02.bin` | The paged ROM image (192K: banks 0-11, with `/rom`): burn it at offset 0 of U31 (paged ROM bank 0) |
| `os_rom/obj/` | The object file, listing (`all_C02.txt`), labels (`os_rom_C02.lbl`), map (`os_rom_C02.map`), debug info (`os_rom_C02.dbg`), and the generated includes (`version.inc`, `test_rom.s`); not in source control |

**Build options** (`os_rom/include/hw.inc`):

| Setting | Values |
| :------ | :----- |
| `CPU_CLOCK_MULT` | 1 = 3.58 MHz (default), 2 = 7.16 MHz: must match the CPU clock jumper |
| `SER_ACIA` | `SER_ACIA_ROCKWELL` (default) or `SER_ACIA_WDC`, for a WDC W65C51N ACIA |
| `SR_SELECT`, `SERIAL_RATE`, `SER_RATE_BOOT` | The serial rate at boot (default 9600; change all three together).  After boot, HyForth's `stty` changes it |

### **Programming the chips**

* **Burn both images** after most changes: HyForth's variables (in the paged ROM) are where its code in the BIOS ROM expects them, and its sample binary words call BIOS ROM addresses.
* The BIOS ROM image goes into **U6** (SST39SF010, or a larger '020/'040).
* The paged ROM image goes at offset 0 of **U31**, the chip holding paged ROM banks `$00-$1F`.  The image is already in chip order: the board swaps the 8K halves of each 16K bank.

### **Connecting a terminal**

* **Cable:** use a **straight-through** RS-232 cable with two female ends (the Hydra's DE-9 is wired like a modem), including pins 7 and 8: the ACIA sends only while CTS is asserted.
* **Terminal settings:** 9600 baud, 8 data bits, no parity, 1 stop bit.  Once booted, the Hydra can switch to another rate or format (HyForth: `"b19200" stty`, then switch the terminal); it comes back up at 9600 after a reset.
* **The terminal should:**
  * understand ANSI escape sequences;
  * send CR for Enter;
  * send either BS or DEL for Backspace (both work).

See the [Hardware Reference](../../reborn/docs/hardware.md#acia-65c51-u3-port-1-irq-line-1) for the pinout.

**The PC tool** is a terminal that also serves a folder on the PC to the Hydra, at `/pc`: programs built on the PC run at once, with no card to carry across.  Once, in `sim/`: `npm install` (the `serialport` package); then `node sim/tools/hydrapc.js COM3 C:\hydra` (`--list` shows the ports; Ctrl-A x quits).  See [the PC tool](tools/emulator.md#the-pc-tool-pc) and [/pc](programming/io.md#a-folder-on-the-pc-pc).

### **First boot**

Fit the CPU clock jumper J7 (3.58 MHz) and the RDY jumper J4, then power up.  You should see:

```
POST ZP:T ST:T LO:T 7D:T SH:S P1:4C
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000 20:0/00/0000

Welcome to the HYDRA-16!  OS 1.8C_0.6

HyForth 0.91
No card: /ram keeps your files until a reset.  ls /rom/bin for the programs.

/ram> 
```

* **The first two lines are POST**, the power-on self test ([what they mean](using/wozmon.md#post-the-power-on-self-test)).
* **With no card**, the prompt is `/ram> `: the shell's own area on the RAM disk, where files last until a reset ([the RAM disks](programming/io.md#the-ram-disks-ram)).  **With a HydraFS card in**, `hydrafs 0` comes before HyForth's banner, and the prompt is `0:/> `: you're at card 0's root ([the shell](using/hyforth.md#the-shell-directories-files-and-programs)).
* **A driver that fails to start** prints `NAME FAIL ee` (ee = the [error code](programming/rom-layout.md#error-codes)).
* **Try it:** `1 2 + .` prints ` 0003`.

Then try:
* `words` (every word);
* `ps` (the tasks);
* `words | wc . . .` (a pipeline);
* `sndtest` (the test tune: `sndstop` stops it);
* `bye` (WOZMON).

The [HyForth guide](using/hyforth.md) has the rest.

### **Without the hardware: the emulator**

`sim/hydrasim.js` emulates the whole board, cycle-counted, and boots the same ROM images:

```
node sim/hydrasim.js -i                    use the Hydra from this terminal, in real time
node sim/hydrasim.js -i --sd card.img      with an SD card (an image file)
```

Ctrl-A x quits, and Ctrl-A r presses the reset button.  To make a card image with files on it:

```
node sim/tools/hydrafs.js mkfs card.img 64
node sim/tools/hydrafs.js import card.img myfiles
```

The Hydra sees them at `/sd/0`, and starts there: the prompt is `0:/> `, and `ls` lists the card's root.  A `boot.hys` in it runs at boot.

Without `-i`, the emulator runs a fixed number of cycles with scripted input, then prints a report: the serial output, the last instructions, the hottest code, and each task's stack depth.  That's the mode for debugging and the tests.  See [the emulator](tools/emulator.md).

### **Testing a change**

```
cd os_rom
node build.js rom test
```

This builds, then boots the new images in the emulator for each regression test (`node sim/regress.js --list` lists them): POST, the self tests, HyForth and its libraries, line editing, pipes, tasks, sound, SD cards and their files, the shell and running programs, sleeping, C programs, exit statuses, the sound library and songs, `/rom`, fault injection.  It prints `N of N tests passed`, or the failing test's output and the command that reproduces it.  Then try it on the board.

### **Where to go next**

| To | Read |
| :- | :--- |
| Use the system | [HyForth](using/hyforth.md), [WOZMON](using/wozmon.md) |
| Program it | [Programmer's Guide](programming/README.md) |
| Understand or change the board | [Hardware Reference](../../reborn/docs/hardware.md) |
| Debug with the emulator, write tests | [Emulator and tools](tools/emulator.md) |
| See what's planned | [Plans](../../reborn/docs/design/plans/) |
