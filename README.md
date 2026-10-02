## **Hydra 16**

A multitasking 65C02 computer and its operating system.

The Hydra-16's hardware gives each of 16 tasks its own 32K of RAM, zero page and stack included, with its own RAM and ROM bank selections, so a task switch is one register write.  On top of that runs a small OS:
* **Scheduling and interrupts:** a preemptive scheduler, and IRQ handlers that run in their driver's own task.
* **Memory:** a memory manager per task, plus shared memory between tasks.
* **IO, Plan 9 style:** devices are file servers; each task has fds, a namespace and pipes.
* **The shell:** HyForth, with `|` pipelines.

The code is built with **cc65** (https://cc65.github.io/), and the board is designed in **KiCad 9** (https://www.kicad.org).

### **Quick start**

```
node build.js test                 build everything (needs Node.js and cc65) and run the regression tests
node sim/hydrasim.js -i            use the Hydra in your terminal, in the emulator (Ctrl-A x quits)
```

### **Documentation**

All documentation is in **[docs/](docs/README.md)**.  **[The Hydra-16 in one place](docs/hydra-16.md)** covers every part, hardware and software, with links to the rest; it's also one PDF, **[docs/hydra-16.pdf](docs/hydra-16.pdf)**, with every document in it.

| | |
| :-- | :-- |
| [The Hydra-16 in one place](docs/hydra-16.md) | The master document: every part of the system, with links into the others |
| [First steps](docs/tutorial.md) | A tutorial: the Hydra in the emulator, HyForth, files, a program in C and in assembly |
| [Getting started](docs/getting-started.md) | Building, programming the chips, the serial terminal, first boot, the emulator |
| [HyForth](docs/using/hyforth.md), [WOZMON](docs/using/wozmon.md) | Using the system |
| [Programmer's Guide](docs/programming/README.md) | ROM layout and API, tasks, interrupts, memory, IO, writing drivers |
| [Hardware Reference](docs/hardware.md) | The board in detail |
| [Emulator and tools](docs/tools/emulator.md) | `hydrasim.js`, the regression tests, the HydraFS card tool |
| [Plans](docs/README.md#plans-and-design-notes) | Design notes and what's next |

### **Repository**

| Folder | What |
| :----- | :--- |
| `os_rom/` | The OS ROM: sources, built images (`bin/`) |
| `programs/` | Programs for the Hydra: the C library and samples (`programs/c/`), assembly (`programs/asm/`) |
| `build.js` | The build, on any OS: `node build.js`, `node build.js test` |
| `sim/` | Emulator, regression tests, tools |
| `board/` | KiCad schematics and PCBs: main board, memory daughter card, bus breakout card |
| `docs/` | Documentation |
