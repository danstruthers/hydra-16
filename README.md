## **Hydra 16**

A multitasking 65C02 computer and its operating system, HydraOS.

The Hydra-16's hardware gives each of 16 tasks its own 32K of RAM, zero page and stack included, with its own RAM and ROM bank selections, so a task switch is one register write.  HydraOS runs on it, Plan 9's way:
* **One kernel** in the BIOS ROM: a preemptive scheduler, one interrupt path, notes, memory and banks, and a jump table made from a specification.  Everything else is a module of the paged ROM, run in place in a task of its own.
* **A base to build on:** the kernel, a serial console driver, the kernel's devices and Woz's monitor are the base (`base/`), which HydraOS is built on and which boots on its own: [write an OS of your own](docs/09-programming/your-own-os.md) on it.
* **Everything a file:** devices are file servers (the console's windows, the disks and HydraFS, sound, GPIO and I2C, the clock, the Vera X, and `/pc`, a folder on the PC over the serial line), and each task has a namespace of its own, built by binds and mounts.
* **Shells and languages:** rc, Plan 9's shell, and the core tools; HyForth (Forth 2012), the login shell; hylang (danlang, a lisp); HyBASIC (QuickBASIC's kind, every number exact); SDKs for C (cc65) and assembly, and `as`, an assembler on the Hydra itself.
* **Tools:** a screen editor (`edit`), a debugger (`db`), a song player that plays ZSM songs and compiles scores as it plays.

The code is built with **cc65** (https://cc65.github.io/), and the board is designed in **KiCad 9** (https://www.kicad.org).

### **Quick start**

```
cd base
node build.js                      the base alone: the kernel, ser, kdev, the monitor (bin/bios.bin, bin/prom0.bin)
node sim/run.js -i                 its monitor in your terminal (Ctrl-A x quits)

cd ../reborn
node build.js                      build HydraOS (needs Node.js and cc65): bin/bios.bin, bin/prom0.bin ...
node sim/run.js -i                 use the Hydra in your terminal, in the emulator (Ctrl-A x quits)
node sim/web.js                    the emulator in a browser: one file, obj/web/hydra-16.html, to open
node sim/test.js                   the regression tests
```

The ROM images are in Git (`reborn/bin/`) and on each GitHub release, so the chips can be programmed without a toolchain.

### **Documentation**

**[The Hydra-16 book](docs/README.md)** is everything, in order: what the machine is, the board and how to build it
(the parts, the layout, the quirks), the kernel and its calls, the drivers, the shell, the languages, the tools, the
emulator, and the programming guide.  It's also [one PDF](docs/hydra-16.pdf), to print.

| Chapter | |
| :------ | :- |
| [1. The Hydra-16 at a glance](docs/01-overview.md) | The ideas, the software in layers, and a first hour in the emulator |
| [2. The hardware](docs/02-hardware/README.md) | The architecture, the devices, the parts, building it, the layout, the quirks, the cards |
| [3. The base](docs/03-base/README.md) | The boot, the kernel, files and namespaces, the system calls, the monitor |
| [4. The drivers](docs/04-drivers/README.md) | The serial console, the kernel's devices, storage, sound, GPIO, video |
| [5. The shell: rc](docs/05-rc.md) | Plan 9's shell |
| [6. The languages](docs/06-languages/README.md) | HyForth, hylang, HyBASIC, and the numbers they share |
| [7. The tools](docs/07-tools.md) | The core tools, the editors, the debugger, the assembler and disassembler |
| [8. The emulator](docs/08-emulator.md) | The emulator, the build, the tests, the PC tools |
| [9. The programming guide](docs/09-programming/README.md) | Assembly, C, servers and drivers, libraries, your own OS, every system call |

How it was designed, and where it stands: [the plan and the design notes](docs/design/README.md).

### **Repository**

| Folder | What |
| :----- | :--- |
| `base/` | The base: the kernel (the BIOS ROM), the system calls' specification, the console driver `ser`, the kernel's devices `kdev`, the monitor `wozmon`, the assembly SDK's core, the board's emulator and the tests' runner |
| `reborn/` | HydraOS, on the base: its modules, the SDKs, the ROM disk, the SD card's image, its tests |
| `docs/` | The book (its chapters, `docs/build`: the tools that make its parts list, layout drawings, call reference and PDF), and the design notes (`docs/design`) |
| `board/` | KiCad schematics and PCBs: the main board, the memory daughter card, the bus breakout card, the Vera X's glue board |
| `old/` | The old system (the 1.8C line: `os_rom/`, its programs, emulator, build and documents), frozen at HydraOS 1.0; `node old/build.js test` still builds and tests it |
