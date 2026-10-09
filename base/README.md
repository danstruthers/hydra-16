## **The Hydra-16 base**

The base is the part of the Hydra-16's software that every system on the board shares: the kernel in the BIOS ROM and
its system calls, the IRQ handling, a serial console driver, the kernel's device server, and a monitor.  It is the
base HydraOS 1.0 runs on (`../reborn`, which builds on this folder), and it is enough to write an operating system of
your own on: [writing an OS on the base](docs/os.md).

Booted on its own, it is three tasks and a monitor:

| Task | Module | What it is |
| :--- | :----- | :--------- |
| 0 | the kernel | The BIOS ROM (`bin/bios.bin`, `kernel/`): the boot and POST, the scheduler and the tick, tasks, memory and banks, semaphores, shared segments, notes, the clock, files and the server protocol, namespaces, the loader; 192 system call slots at `$F800` |
| F | `ser` | The console driver: the serial port (`#c`: `cons`, `consctl`, `ser`, `serctl`), on the serial layer HydraOS's console shares (`lib/serial.inc`) |
| E | `kdev` | The kernel's devices: the modules (`#m`, which `SPAWN` reads), null and the kernel's messages (`#n`), the tasks (`#p`), pipes (`#|`), the environment (`#e`), named shared segments (`#s`), the ticks (`#t`), raw RAM (`#r`) |
| 1 | `wozmon` | The monitor, as init: [Woz's Apple 1 monitor](docs/monitor.md), with a disassembler |

The rest of the tasks, 2 to D, are free for whatever runs on it.

```
Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000
POST ok
RAM modules: 02
task F: ser
task E: kdev
task 1: wozmon

Hydra-16 monitor (Woz's): XXXX examine, XXXX.YYYY a block, XXXX: BB store, XXXXR run;
L instructions, K bytes; Escape the line, Ctrl-C back here.
T1 00>
```

### **Building, running and testing it**

With Node.js 18 or later and cc65 (`ca65`, `ld65`: `$CC65_BIN`, `$CC65_HOME/bin`, `C:\source\cc65\win64_snapshot\bin`, or
the PATH):

```
node build.js            the kernel (bin/bios.bin), the modules (ser, kdev, wozmon), the paged ROM (bin/prom0.bin),
                         the assembly SDK's hydra.inc (obj/sdk), the budgets
node sim/run.js -i       the base in the emulator, its serial console in your terminal (Ctrl-A x quits, Ctrl-A h helps)
node sim/test.js         the base's tests (the boot, the monitor, the kernel's own tests)
```

The images are in Git (`bin/`), so `node sim/run.js -i` works without cc65.  On the board, `bin/bios.bin` goes into
the BIOS ROM's socket and `bin/prom0.bin` into the first paged ROM socket (U31), as for HydraOS (its BIOS ROM is this
one, byte for byte); a terminal at 9600 baud on the serial port is the console.  `node ../reborn/build.js` builds this
first, then HydraOS on it.

### **The tree**

| Folder | What's there |
| :----- | :----------- |
| `kernel/` | The kernel: the BIOS ROM's 16 pages (`bios.cfg`), the reset stub and vectors on each, the COMMON block (the IRQ entry and exit, the far call), the scheduler, tasks, memory, semaphores, notes, the clock, files, namespaces, the loader, POST |
| `include/` | `hw.inc`, the board as the software sees it; `layout.inc`, where every piece of the kernel's state is |
| `spec/` | `api.def` (the system calls: the jump table, `hydra.inc` and the reference are made from it) and `errors.def` (the error codes and their texts) |
| `modules/` | The base's modules, a folder each: `ser`, `kdev`, `wozmon`; `rom.txt`, the paged ROM's list and its init; `module.cfg` ... `module8.cfg`, a module's link (one bank to eight) |
| `lib/` | Sources shared with HydraOS's modules: `serial.inc` (the serial port's layer: `ser`'s and `cons`'s), `dis.inc` and `w65c02.inc` (the disassembler and the W65C02S's instructions: `wozmon`'s and HydraOS's asm library's) |
| `sdk/asm/` | The assembly SDK's core: `hyx2.inc` and `hyx2.cfg` (a module's or a RAM program's header and link), `macros.inc`, `srvlib.inc` and `srvlib.s` (the server library a driver is built on); the build adds `obj/sdk/hydra.inc` |
| `tools/` | `apigen.js` (the spec into the jump table, the error texts, `hydra.inc`, `api.json`), `romimg.js` (a paged ROM image: the module directory, the hardware test, the modules), `check.js` (only the kernel writes `T`, `V`, `W`), `budget.js` |
| `sim/` | The board's emulator, cycle by cycle (`lib/`: the W65C02S, `T`/`U`/`V`/`W`, the ACIA, the VIA, the YM2151, the DS1747, a Vera X card, SD cards), `run.js` (any system's images, live or for a while; the monitor, breaks, the call trace), `view.js` (the screen and sound in a browser), `test.js` (the tests' runner, HydraOS's too) |
| `tests/` | `tests.js`, the base's tests; `mod/`, their modules (and `testlib.inc`, HydraOS's test modules' too) |
| `bin/` | The images (in Git): `bios.bin`, `prom0.bin` |
| `docs/` | [The monitor](docs/monitor.md); [writing an OS on the base](docs/os.md) |

The bank-1 hardware test is the old system's, unchanged (`../old/os_rom/bin/paged_rom_C02.bin`'s bank 1); POST starts
it if T is typed during the boot.  The kernel's design and its calls are documented with HydraOS: [the
conventions](../reborn/docs/conventions.md), [calling the system](../reborn/docs/programming/calls.md), [modules and
servers](../reborn/docs/programming/modules.md), [the hardware reference](../reborn/docs/hardware.md); the plan for
this folder is [BASE.md](../reborn/docs/design/plans/BASE.md).
