# Hydra-16 reborn

The Hydra-16's operating system, rebuilt from the ground up by the plan in
[docs/reimplementation-from-scratch.md](../docs/reimplementation-from-scratch.md): one kernel in the BIOS ROM,
everything else modules in the paged ROM run in place in tasks of their own, one way to call the system (a
jump table made from a specification), one interrupt path, and every performance-critical mechanism measured
in the emulator before anything depends on it.  The old system (`os_rom/`) is untouched beside it.

Where it stands, step by step against the plan, and the spikes' measurements: [docs/status.md](docs/status.md);
HyForth's, from step 6.6 on: [docs/forth-status.md](docs/forth-status.md).
The rules every source here follows: [docs/conventions.md](docs/conventions.md).

## Build, run, test

Node.js 18 or later, and cc65 (`ca65`, `ld65`, and for the C programs `cc65`, `ar65` and its `include`, `asminc` and
`lib`): `$CC65_BIN`, `$CC65_HOME/bin`, `C:\source\cc65\win64_snapshot\bin`, or the PATH.

```
node build.js                       the BIOS ROM (bin/bios.bin), the paged ROM (bin/prom0.bin, prom1.bin ...: a 512K
                                    image for each socket it fills; the modules, the old hardware test in bank 1, the
                                    ROM disk), the budget report
node sim/run.js                     boot them in the emulator for a while, then report (the output, the tasks)
node sim/run.js -i                  the Hydra's serial console, live (Ctrl-A x quits, Ctrl-A h helps)
node sim/run.js -i --sd card.img    the same with a card in SD device 0 (an image: ../sim/tools/hydrafs.js)
node sim/run.js -i --pc-dir DIR     the same with the folder DIR at /pc (the emulator plays the PC tool:
                                    ../sim/tools/hydrapc.js on a real PC)
danlang sim/dl/run.dl -i            the serial console in the danlang emulator (the same Ctrl-A keys; Ctrl-C is the
                                    Hydra's); --input "\phylang\r" starts hylang at the first prompt
node build.js prog DIR              a program of your own, DIR/*.s into DIR/NAME.hyx (sdk/asm/README.md), or
                                    DIR/*.c (and *.s), a C program (sdk/c/README.md)
node sim/test.js                    the regression tests, with their time budgets: as many at a time as the CPU has
                                    cores, each in a process of its own (-j N: N at a time; -j 1, one after another)
node sim/test.js irq -v             one test, with its output
node sim/test.js --dl               the same tests in the danlang emulator (sim/dl: danlang's Release build, ../danlang
                                    or $DANLANG), judged the same way
```

`build.js --clock 2` builds for a 7.16 MHz board (jumper J7); `--acia wdc` for a WDC W65C51N in the serial
port.  `bin/` and `obj/` are the build's: not in Git (`bin/sdk/asm` and `bin/sdk/c` are the SDKs, whole, to take
away).

## The tree

| Folder | What's there |
|---|---|
| `spec/` | `api.def`, the system calls (each call's group, slot, registers, errors and words), and `errors.def`, the error codes: the one source for the jump table, `hydra.inc`, the reference and the emulator's names |
| `include/` | `hw.inc`, the board as the software sees it (facts only); `layout.inc`, where every piece of the kernel's state lives |
| `kernel/` | The kernel: the BIOS ROM's 16 pages (`bios.cfg`), the reset stub and vectors on each, the COMMON block (and the far call), interrupts, the scheduler, calls between tasks and kcopy, tasks and modules, notes, the console calls (to fds 0 and 1, or the bring-up console), the small calls (page 0); the kernel task's side of tasks, the boot, memory, TASKINFO, TASKREAD and TASKMEM (a task's state and memory, for /proc), the clock and the DS1747 (page 1); files, names, the server calls and the environments (page 2); namespaces, `SPAWN` and the loader (page 3); POST (page 4) |
| `modules/` | The paged ROM's modules, a folder each (`init`, `hello`, `cons`: the console driver and `/pc`, `storage`: the SPI bus, the disks and HydraFS, a module of two banks, `kdev`: the kernel's devices, `snd`: the sound driver (the YM2151, `#a`), `gpio`: the VIA's port A (`#g`) and the I2C bus on it (`#i`), `rc`: the shell, of two banks, `wstart`: the shell in each window the user asks for (`/lib/shell`'s, else rc), the core tools: `ls`, `cp`, `mv`, `rm`, `ps`, `top`, `wc`, `more` ..., and `edit`, the line editor, `play`, the song player, `date`, `xmodem`, `forth`, HyForth's core (and a shell: `forth -l`); `hylang`, the lisp, is being written again from scratch: phase 7); `module.cfg`, their link (`module2.cfg` to `module4.cfg`, a module of two to four banks; a folder's own `NAME.cfg`, for one with library modules of its own); `rom.txt`, which go in the ROM and which is init |
| `forthlib/` | HyForth's libraries, a word set each (`NAME.s`, built into `/lib/forth/NAME.fl` on the ROM disk), and `forthlib.inc` and `forthlib.cfg`, a library's definitions and its link |
| `programs/` | The ROM disk's programs (`/rom/bin`: RAM programs, a folder each: `mkfs`, `fsck`, `label`, and `grep` and `sort` in C), and `diskctl.s`, what the disk tools share |
| `romfs/` | The ROM disk's files (`/rom`: `README`, `lib/namespace`, `lib/profile`; its `doc/api.md` and `bin` come from the build) and `romfs.txt`, its manifest; bytes for the Hydra, with LF line ends (`.gitattributes`: not text to Git) |
| `sdk/asm/` | For programs in assembly: `hyx2.inc` (the module header), `hyx2.cfg` (a RAM program's link), `macros.inc`, `srvlib.inc` and `srvlib.s` (the server library), `nslib.s` (a task's default namespace: Plan 9's `newns`), `toollib.inc` and `toollib.s` (what the tools share), `samples/` (on the ROM disk, `/rom/sample`), and `README.md`, the SDK's guide; with `obj/sdk/hydra.inc`, made from `spec/` |
| `sdk/c/` | For programs in C (cc65): `include/hydra.h`, `hydra.cfg` (the link), `lib/` (the library's sources: the start-up, files and buffered stdio, the environment, `system`, `signal`, conio ...; built with cc65's `none.lib` into `obj/sdk/c/hydra.lib`), `samples/` (`/rom/sample/c`, `ctest` the library's test), and `README.md`, its guide; with `obj/sdk/c/hydracalls.h`, made from `spec/` |
| `tools/` | `apigen.js` (the specification's outputs: the jump table, `hydra.inc`, the C header, the reference, HyForth's sys- words and `hydra.fs`), `forthlib.js` (HyForth's libraries: the core's labels and id, each library's file), `romimg.js` (the paged ROM image), `romfs.js` (the ROM disk: its HydraFS volume, made with `../sim/tools/hydrafs.js`, and its files read back from the image), `budget.js` (sizes and room left), `check.js` (only the kernel writes T, V and W) |
| `sim/` | `run.js` and `test.js` over `lib/`, the emulator (a copy of `../sim/lib`, with a receive-latency counter added to the ACIA, IRQs-off stretches ended where an interrupt is taken, an I2C bus on port A: `i2c.js`, and nothing allocated for an instruction run, which makes it about six times as fast: some 20 MHz of the Hydra's cycles), and `lib/pchost.js`, the PC tool's part of `/pc` (its file server: `../sim/tools/pcfs.js`) |
| `sim/dl/` | The emulator again, in danlang (`../danlang`, its `feature/speed` branch): `cpu.dl` (the W65C02S: its code, the ROMs' and RAM's, decoded into blocks, kept), `machine.dl` (the memory map, the IRQ lines, the run loop), `devices.dl` (the ACIA, the VIA, the YM2151), `spi.dl` (SD cards, echo devices), `i2c.dl`, `rtc.dl` (the DS1747); `hydra.dl`, a test run from the spec `bridge.js` writes (`sim/test.js --dl`: the PC's end of `/pc` and XMODEM stays in JS, over danlang's stdin and stdout; `$HYDRA_DL` runs a copy of these files); `run.dl`, a run on its own, or the serial console live (`-i`, as `run.js`'s, from danlang's `key?`, `key` and `on-note`); `image.js`, a test's paged ROM image |
| `tests/` | `tests.js`, the tests and their budgets; `mod/`, the test modules (each runs as init) and `testlib.inc`; `ram/`, the test RAM programs (put on an emulated card by the load test); `forth/`, the Forth 2012 test suite (its README); `hylang/`, danlang's regression suite, which hylang runs as its phases land (its README) |
| `docs/` | `conventions.md`, `status.md`, `hylang.md` (hylang, phase 7: the language, danlang's, and its design on the Hydra), `hyforth.md` (HyForth's additions, 6.6-6.21: the old HyForth's pieces back, HyForth as a shell, hylang's Hydra words and device libraries, compile-only words, the word lists' index, the standard's other word sets, faster loading, the storage driver's walk cache), `using/hyforth.md` (the guide for using HyForth), `forth-status.md` (where HyForth stands from 6.6 on) |
