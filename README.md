## **Hydra 16**

A multitasking 65C02 computer and its operating system, HydraOS.

The Hydra-16's hardware gives each of 16 tasks its own 32K of RAM, zero page and stack included, with its own RAM and ROM bank selections, so a task switch is one register write.  HydraOS runs on it, Plan 9's way:
* **One kernel** in the BIOS ROM: a preemptive scheduler, one interrupt path, notes, memory and banks, and a jump table made from a specification.  Everything else is a module of the paged ROM, run in place in a task of its own.
* **A base to build on:** the kernel, a serial console driver, the kernel's devices and Woz's monitor are the base (`base/`), which HydraOS is built on and which boots on its own: [write an OS of your own](base/docs/os.md) on it.
* **Everything a file:** devices are file servers (the console's windows, the disks and HydraFS, sound, GPIO and I2C, the clock, the Vera X, and `/pc`, a folder on the PC over the serial line), and each task has a namespace of its own, built by binds and mounts.
* **Shells and languages:** rc, Plan 9's shell, and the core tools; HyForth (Forth 2012), the login shell; hylang (danlang, a lisp); BASIC (Microsoft's); SDKs for C (cc65) and assembly, and `as`, an assembler on the Hydra itself.
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

| | |
| :-- | :-- |
| [The guide](reborn/docs/hydra-16.md) | The whole system in one place, hardware and software, with links to the rest ([as a PDF](reborn/docs/hydra-16.pdf): the guide, the tutorial, the guides, the programmer's guide and the hardware reference as one book, to print) |
| [The base](base/README.md) | The kernel, its console and devices, the monitor: building, running and testing it, [the monitor](base/docs/monitor.md), [writing an OS on it](base/docs/os.md) |
| [HydraOS](reborn/README.md) | Building, running and testing it, and its tree |
| [The first hour](reborn/docs/tutorial.md) | A tutorial: switching it on, the shell, files, windows, the languages, sound, a program of your own |
| [The guides](reborn/docs/using/README.md) | rc, the tools, HyForth, hylang, BASIC |
| [The programmer's guide](reborn/docs/programming/README.md) | Calls, memory, tasks and notes, files and namespaces, servers and drivers, modules, video |
| [Status](reborn/docs/status.md) | Where it stands, what was measured, what's next |
| [The hardware reference](reborn/docs/hardware.md) | The board (V1), its cards and the Vera X, in detail |
| [The plan and the design notes](reborn/docs/design/README.md) | How HydraOS was designed, and the plans |

### **Repository**

| Folder | What |
| :----- | :--- |
| `base/` | The base: the kernel (the BIOS ROM), the system calls' specification, the console driver `ser`, the kernel's devices `kdev`, the monitor `wozmon`, the assembly SDK's core, the board's emulator and the tests' runner |
| `reborn/` | HydraOS, on the base: its modules, the SDKs, the ROM disk, its tests, and every document (`reborn/docs`: the guide, the hardware reference, the plan and the design notes) |
| `board/` | KiCad schematics and PCBs: main board, memory daughter card, bus breakout card |
| `old/` | The old system (the 1.8C line: `os_rom/`, its programs, emulator, build and documents), frozen at HydraOS 1.0; `node old/build.js test` still builds and tests it |
