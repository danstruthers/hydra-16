## **Hydra-16 documentation**

The Hydra-16 is a multitasking 65C02 computer: a W65C02S whose address decoding gives each of 16 tasks its own RAM, zero page, stack and bank selections, running a small preemptive OS with Plan 9-style IO, and HyForth as its shell.

### **Start here**
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
* [IDEAS.md](plans/IDEAS.md): ideas for later (wait states for board V2, ...).
