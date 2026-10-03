# Hydra-16 reborn

The Hydra-16's operating system, rebuilt from the ground up by the plan in
[docs/reimplementation-from-scratch.md](../docs/reimplementation-from-scratch.md): one kernel in the BIOS ROM,
everything else modules in the paged ROM run in place in tasks of their own, one way to call the system (a
jump table made from a specification), one interrupt path, and every performance-critical mechanism measured
in the emulator before anything depends on it.  The old system (`os_rom/`) is untouched beside it.

Where it stands, step by step against the plan, and the spikes' measurements: [docs/status.md](docs/status.md).
The rules every source here follows: [docs/conventions.md](docs/conventions.md).

## Build, run, test

Node.js 18 or later, and cc65 (`ca65`, `ld65`, and for the C programs `cc65`, `ar65` and its `include`, `asminc` and
`lib`): `$CC65_BIN`, `$CC65_HOME/bin`, `C:\source\cc65\win64_snapshot\bin`, or the PATH.

```
node build.js                       the BIOS ROM (bin/bios.bin), the paged ROM (bin/prom.bin: the modules, and the old
                                    hardware test in bank 1), the budget report
node sim/run.js                     boot them in the emulator for a while, then report (the output, the tasks)
node sim/run.js -i                  the Hydra's serial console, live (Ctrl-A x quits, Ctrl-A h helps)
node sim/run.js -i --sd card.img    the same with a card in SD device 0 (an image: ../sim/tools/hydrafs.js)
node build.js prog DIR              a program of your own, DIR/*.s into DIR/NAME.hyx (sdk/asm/README.md), or
                                    DIR/*.c (and *.s), a C program (sdk/c/README.md)
node sim/test.js                    the regression tests, with their time budgets
node sim/test.js irq -v             one test, with its output
```

`build.js --clock 2` builds for a 7.16 MHz board (jumper J7); `--acia wdc` for a WDC W65C51N in the serial
port.  `bin/` and `obj/` are the build's: not in Git (`bin/sdk/asm` and `bin/sdk/c` are the SDKs, whole, to take
away).

## The tree

| Folder | What's there |
|---|---|
| `spec/` | `api.def`, the system calls (each call's group, slot, registers, errors and words), and `errors.def`, the error codes: the one source for the jump table, `hydra.inc`, the reference and the emulator's names |
| `include/` | `hw.inc`, the board as the software sees it (facts only); `layout.inc`, where every piece of the kernel's state lives |
| `kernel/` | The kernel: the BIOS ROM's 16 pages (`bios.cfg`), the reset stub and vectors on each, the COMMON block (and the far call), interrupts, the scheduler, calls between tasks and kcopy, tasks and modules, notes, the console calls (to fds 0 and 1, or the bring-up console), the small calls (page 0); the kernel task's side of tasks, the boot, memory, TASKINFO, the clock and the DS1747 (page 1); files, names, the server calls and the environments (page 2); namespaces, `SPAWN` and the loader (page 3); POST (page 4) |
| `modules/` | The paged ROM's modules, a folder each (`init`, `hello`, `cons`: the console driver, `storage`: the SPI bus, the disks and HydraFS, a module of two banks, `kdev`: the kernel's devices, `snd`: the sound driver (the YM2151, `#a`), `gpio`: the VIA's port A (`#g`) and the I2C bus on it (`#i`), `rc`: the shell, of two banks, `wstart`: rc in each window the user asks for, the core tools: `ls`, `cp`, `mv`, `rm`, `ps`, `top`, `wc`, `more` ..., and `edit`, the line editor, `play`, the song player, `date`); `module.cfg`, their link (`module2.cfg`, a module of two banks); `rom.txt`, which go in the ROM and which is init |
| `programs/` | The ROM disk's programs (`/rom/bin`: RAM programs, a folder each: `mkfs`, `fsck`, `label`, and `grep` and `sort` in C), and `diskctl.s`, what the disk tools share |
| `romfs/` | The ROM disk's files (`/rom`: `README`, `lib/namespace`, `lib/profile`; its `doc/api.md` and `bin` come from the build) and `romfs.txt`, its manifest; bytes for the Hydra, with LF line ends (`.gitattributes`: not text to Git) |
| `sdk/asm/` | For programs in assembly: `hyx2.inc` (the module header), `hyx2.cfg` (a RAM program's link), `macros.inc`, `srvlib.inc` and `srvlib.s` (the server library), `nslib.s` (a task's default namespace: Plan 9's `newns`), `toollib.inc` and `toollib.s` (what the tools share), `samples/` (on the ROM disk, `/rom/sample`), and `README.md`, the SDK's guide; with `obj/sdk/hydra.inc`, made from `spec/` |
| `sdk/c/` | For programs in C (cc65): `include/hydra.h`, `hydra.cfg` (the link), `lib/` (the library's sources: the start-up, files and buffered stdio, the environment, `system`, `signal`, conio ...; built with cc65's `none.lib` into `obj/sdk/c/hydra.lib`), `samples/` (`/rom/sample/c`, `ctest` the library's test), and `README.md`, its guide; with `obj/sdk/c/hydracalls.h`, made from `spec/` |
| `tools/` | `apigen.js` (the specification's outputs), `romimg.js` (the paged ROM image), `romfs.js` (the ROM disk: its HydraFS volume, made with `../sim/tools/hydrafs.js`, and its files read back from the image), `budget.js` (sizes and room left), `check.js` (only the kernel writes T, V and W) |
| `sim/` | `run.js` and `test.js` over `lib/`, the emulator (a copy of `../sim/lib`, with a receive-latency counter added to the ACIA, IRQs-off stretches ended where an interrupt is taken, and an I2C bus on port A: `i2c.js`) |
| `tests/` | `tests.js`, the tests and their budgets; `mod/`, the test modules (each runs as init) and `testlib.inc`; `ram/`, the test RAM programs (put on an emulated card by the load test) |
| `docs/` | `conventions.md`, `status.md` |
