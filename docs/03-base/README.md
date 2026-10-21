# 3. The base: the kernel and the system calls

The **base** is the part of the Hydra-16's software that every system on the board shares: the kernel in the BIOS ROM,
its system calls, the interrupt path, a serial console driver, the kernel's own devices, and a monitor.  HydraOS is
built on it, and it's enough to build an operating system of your own on.  It lives in `base/`.

This chapter is the kernel as a whole: how the machine starts, how tasks, memory and interrupts work, the file model
everything else is built on, how a program calls the system, and the monitor the base starts when it runs alone.
[Chapter 4](../04-drivers/README.md) is the drivers that serve the devices as files, and [chapter
9](../09-programming/README.md) how to write programs and drivers of your own.

## Contents

1. [The boot](boot.md): from reset to init: the reset stub, the kernel task, POST, the drivers, init.
2. [The kernel](kernel.md): the BIOS ROM's pages, tasks and the scheduler, interrupts, memory and banks, shared
   segments, semaphores, notes, the clock, how the kernel reaches other tasks.
3. [Files, servers and namespaces](files.md): fds and channels, servers and requests, device letters, namespaces and
   unions, the environment, the kernel's own devices.
4. [The system calls](calls.md): the jump table, the registers, the groups of calls, errors, and calling from each
   language.
5. [The monitor](monitor.md): Woz's monitor, the base's own init.

## The base in one page

Booted on its own, the base is four tasks:

| Task | Module | What it is |
| :--- | :----- | :--------- |
| 0 | the kernel | The BIOS ROM (`base/bin/bios_rockwell.bin` or `bios_wdc.bin`, from `base/kernel/`): the boot and POST, the scheduler and the tick, tasks, memory and banks, semaphores, shared segments, notes, the clock, files and the server protocol, namespaces, the loader; 89 system calls in a jump table of 192 slots at `$F800` |
| F | `ser` | The console driver: the serial port as files (`#c`: `cons`, `consctl`, `ser`, `serctl`) |
| E | `kdev` | The kernel's devices: the modules (`#m`), `null` and the kernel's messages (`#n`), the tasks (`#p`), pipes (`#\|`), the environment (`#e`), shared segments by name (`#s`), the ticks (`#t`), raw RAM (`#r`), the root (`#/`) |
| 1 | `wozmon` | The monitor, as init: [Woz's Apple 1 monitor](monitor.md), with a disassembler |

Tasks 2 to D are free for whatever runs on it.  The paged ROM image (`base/bin/prom0.bin`) has the module directory,
the hardware test, and the three modules; it fills one chip.

**HydraOS uses the same kernel**, byte for byte: its BIOS ROM is the base's.  It replaces `ser` with `cons` (the same
files and more: windows), adds its own drivers, and starts `init` instead of the monitor ([chapter
4](../04-drivers/README.md)).  A program written for the base runs on HydraOS as it is.

## The base's tree

| Folder | What's there |
| :----- | :----------- |
| `base/kernel/` | The kernel: `bios.cfg` (the BIOS ROM's 16 pages), `reset.s` (the reset stub, the vectors, the boot), `common.s` (the COMMON block: the IRQ entry and exit, the far call), `sched.s`, `irq.s`, `call.s` (SCALL, KCALL, kcopy), `task.s`, `load.s` (SPAWN and the loader), `mem.s`, `sem.s`, `notes.s`, `time.s`, `file.s`, `ns.s`, `env.s`, `info.s`, `debug.s`, `cons.s` (the bring-up console), `sys.s`, `post.s`, `far.s` |
| `base/include/` | `hw.inc` (the board as the software sees it: every register's name) and `layout.inc` (where every piece of the kernel's state is) |
| `base/spec/` | `api.def` (the system calls: the jump table, `hydra.inc` and the [reference](../09-programming/calls-reference.md) are made from it) and `errors.def` (the error codes and their texts) |
| `base/modules/` | The base's modules, a folder each (`ser`, `kdev`, `wozmon`); `rom.txt`, the paged ROM's list and its init; `module.cfg` ... `module8.cfg`, a module's link (one bank to eight) |
| `base/lib/` | Sources shared with HydraOS's modules: `serial.inc` (the serial port's layer: `ser`'s and `cons`'s), `dis.inc` and `w65c02.inc` (the disassembler) |
| `base/sdk/asm/` | The assembly SDK's core: `hydra.inc` (the calls, the errors, the constants: made from `spec/`, and in Git), `hyx2.inc` and `hyx2.cfg` (a program's header and link), `macros.inc`, `srvlib.inc` and `srvlib.s` (the server library) |
| `base/tools/` | `apigen.js` (the spec into the jump table, the error texts, `hydra.inc`), `romimg.js` (a paged ROM image), `check.js`, `budget.js` |
| `base/sim/` | The emulator, cycle by cycle ([chapter 8](../08-emulator.md)) |
| `base/tests/` | The base's tests and their modules |
| `base/bin/` | The images, in Git: `bios_rockwell.bin` and `bios_wdc.bin` (one for each serial chip), `prom0.bin` |

**Building it** needs Node.js 18 or later and cc65 (`ca65` and `ld65`), found in `$CC65_BIN`, `$CC65_HOME/bin`, on Windows
in the repository (`base/tools/cc65/win64_hydra`: cc65 with the project's four fixes; its README says how to build it
elsewhere), or on the PATH:

```
$ cd base
$ node build.js          # the kernel (bin/bios_*.bin), the modules, the paged ROM (bin/prom0.bin), hydra.inc
$ node sim/run.js -i     # the base in the emulator, its serial console in your terminal (Ctrl-A x quits)
$ node sim/test.js       # the base's tests
```

The images are in Git, so `node sim/run.js -i` works without cc65.
