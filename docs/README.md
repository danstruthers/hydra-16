## **Hydra-16 documentation**

The Hydra-16 is a multitasking 65C02 computer: a W65C02S whose address decoding gives each of 16 tasks its own RAM, zero page, stack and bank selections, running a small preemptive OS with Plan 9-style IO, and HyForth as its shell.

### **Start here**
* **[The Hydra-16 in one place](hydra-16.md)**: the master document: every part of the machine and its software, the hardware included, in one page, with links into the documents below.  All of the documentation is also one PDF: **[hydra-16.pdf](hydra-16.pdf)** (`node sim/tools/mkpdf.js` remakes it, with Microsoft Edge or Chrome).
* **[First steps](tutorial.md)**: a tutorial for the first hour, in the emulator: HyForth, files and a card, a program in C and one in assembly.
* **[Getting started](getting-started.md)**: build the ROMs, program the chips, connect a terminal, boot, or run it in the emulator.

### **Using the system**
* **[HyForth](using/hyforth.md)**: the shell and language: words, directories and files, scripts and programs, pipelines, tasks.
* **[WOZMON, POST and the self tests](using/wozmon.md)**: the monitor and disassembler, reading the power-on self test, the hardware test.

### **Programming it**
The **[Programmer's Guide](programming/README.md)**, by area:
* [ROM layout and the API index](programming/rom-layout.md): ROM pages, gates, calling conventions, zero page, error codes, every entry point.
* [Tasks and the scheduler](programming/tasks.md)
* [Interrupts](programming/interrupts.md)
* [Memory](programming/memory.md): the MMU, shared memory, far pointers.
* [Input and output](programming/io.md): fds, devices, the console, pipes, namespaces.
* [Drivers and file servers](programming/servers.md)
* [Programs](programming/programs.md): Hydra executables (`.hyx`), and building one.
* [The C Programmer's Guide](programming/c.md): writing programs in C with cc65, from the first build to the library, the console, files, processes, assembly, performance and debugging.

### **The hardware**
* **[Hardware Reference](hardware.md)**: the main board in detail (memory system, address decoding, pseudo-registers, interrupts, clocks, devices, slots, connectors, errata) and the companion cards.

### **Tools**
* **[The emulator and tools](tools/emulator.md)**: `hydrasim.js` (interactive and scripted), the regression tests, the HydraFS card tool, the `.hyx` header tool.

### **Plans and design notes**
The reasoning behind the design, and what's planned.  Parts of them are history (marked done).
* [IO_PLAN.md](plans/IO_PLAN.md): the IO subsystem, scheduler and filesystem steps.
* [MMU_PLAN.md](plans/MMU_PLAN.md): the memory manager, IRQ dispatch, drivers, zero-page convention.
* [HYDRAFS.md](plans/HYDRAFS.md): the SD card filesystem (spec).
* [SHELL.md](plans/SHELL.md): HyForth as a shell: a current directory, commands, running programs (done).
* [REORG_PLAN.md](plans/REORG_PLAN.md): the ROM reorganisation.
* [SOUND.md](plans/SOUND.md): the YM2151: its library (done), a song player, a test song that uses the whole chip, and importing music from other machines.
* [NEXT_STEPS.md](plans/NEXT_STEPS.md): what's missing to make the Hydra fun and useful for hobbyists and programmers, and the milestones to get there.
* [DISKS.md](plans/DISKS.md): RAM and ROM disks as HydraFS volumes: `/ram` (each shell's own area of the RAM disk) and `/sram` (the shared RAM disk), with program caches, and `/rom` (the paged ROM as one disk: built).
* [NAMESPACES.md](plans/NAMESPACES.md): the Plan 9 way: union directories put together with binds and mounts, a default namespace, and `/bin` and `/lib` in place of search paths.
* [PROC.md](plans/PROC.md): `/proc`, the tasks as files, with a task's memory (`/proc/N/mem`, `/proc/N/ram`) for its family and task 0, the system task.
* [VIDEO.md](plans/VIDEO.md): the Vera X video card in slot 0 (the VERA: VGA, sprites, PSG and PCM), its keyboard controller, and the software for them.
* [CODE_REVIEW.md](plans/CODE_REVIEW.md): a review of the whole software tree, with bugs, ROM and zero page budgets, and recommendations.
* [IDEAS.md](plans/IDEAS.md): ideas for later (wait states for board V2, ...).
