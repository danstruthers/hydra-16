## **Hydra-16 Programmer's Guide**

For programmers writing code for the Hydra-16: ROM code (the OS, drivers, servers), programs in RAM, or HyForth words that call the OS.  Using the system from its shells is in [HyForth](../using/hyforth.md) and [WOZMON](../using/wozmon.md); the machine itself is in the [Hardware Reference](../hardware.md).

| Chapter | Covers |
| :------ | :----- |
| [ROM layout and the API index](rom-layout.md) | The two ROM images and the BIOS ROM pages, calling across pages (gates), calling conventions, zero page, error codes, **every public entry point** |
| [Tasks and the scheduler](tasks.md) | The 16 tasks, scheduling, starting and ending tasks, waiting and sleeping, `TASK_CALL`, break and kill, the console's foreground, driver tasks, boot |
| [Interrupts](interrupts.md) | The IRQ dispatcher, writing and registering handlers, software interrupts |
| [Memory](memory.md) | The MMU (handles, allocation), shared memory, far pointers and references |
| [Input and output](io.md) | fds and the IO calls, stdio, the devices, the console, pipes, namespaces |
| [Drivers and file servers](servers.md) | Writing a device: registration, the serve routine, the request block, waiting, the storage layer |
| [Programs](programs.md) | Hydra executables (`.hyx`): the header, what a program gets, building one, putting it on a card, how `run` loads it |

### **The system in one page**

**Hardware.**  A W65C02S where the task register `T` swaps the whole lower 32K of RAM, zero page and stack included, together with each task's RAM bank and ROM bank selections.  A task switch is one register write plus the CPU registers.  The 8K window at `$8000` shows a task's own RAM banks or the shared RAM.  The 8K BIOS ROM at `$E000` is paged by `W`.  16 prioritised IRQ lines each have their own vector.

**Kernel** (BIOS ROM page 0):
* a **preemptive scheduler**, round robin at 200 Hz, with task 0 as the idle task;
* an **IRQ dispatcher** that runs each handler in the task that registered it;
* a **memory manager** per task, plus **shared memory** between tasks.

**Drivers** run in **resident tasks** of their own (serial `$F`, sound `$E`, pipes `$D`, storage `$C`).  Their state lives in their task, and they run only from their IRQs and from calls into them.

**IO**, Plan 9 style: everything is a file:
* Devices are **file servers**; a task opens `/dev/cons`, `/dev/sd/0/data`, `/sd/0/games/star.frt`, ... and reads and writes fds.
* The IO layer (page 2) passes each request to the server's task with `TASK_CALL`, and the data through the client's transfer area in shared RAM.
* Reads that have to wait put the task to sleep.
* Each task has a **namespace** (mount, bind), inherited by the tasks it starts, like its open fds.

**The shell** is HyForth (task 1) with `|` pipelines, a current directory on the SD cards, file commands (`cd`, `ls`, `cp` ...), scripts (`.hys`) and programs (`.hyx`) run by name; it can start more shells in other tasks, and falls back to WOZMON.

```
  task 1: HyForth shell      task N: programs, pipeline stages, more shells
      |  IO_READ/IO_WRITE/...  (fds, namespace)
      v
  IO layer (page 2) ---TASK_CALL---> server in its driver task:  $F serial  $E sound  $D pipes  $C storage
      |                                                              ^
      v                                                              | IRQ handlers run in the driver's task
  kernel (page 0): scheduler, TASK_CALL, IRQ dispatch, MMU, shared memory
```

### **Where things are in the source**

| Folder | What |
| :----- | :--- |
| `os_rom/all.s` | Includes everything, page by page (`.scope PAGEn`) |
| `os_rom/os_rom_C02.cfg` | The linker config: memory areas per ROM page, segments |
| `os_rom/include/` | Constants and macros: `hw.inc` (hardware), `kernel.inc` (tasks, errors, far pointers), `io.inc` (IO), `zero.s` (the OS zero page), `macros.inc`, `ascii.inc` |
| `os_rom/kernel/` | Reset and boot (`os_main.s`), tasks and scheduler (`tasks.s`), IRQs (`irq.s`), MMU (`mmu.s`), shared memory (`shared.s`), far pointers (`fp.s`), the COMMON block (`common.s`), gates, thunks, printing |
| `os_rom/io/` | The IO layer (`io.s`, `io_p0.s`), namespaces (`ns.s`) and the servers: console and serial (`ser_srv.s`), sound (`snd_srv.s`), pipes, `/dev/proc`, `/dev/sd`, HydraFS (`hfs_srv.s`, `hfs_write.s`, `hfs_check.s`, on ROM page 6; `hfs_format.s`, on page 3) |
| `os_rom/drivers/` | Serial, sound, VIA, SPI, SD card, storage task |
| `os_rom/monitor/` | WOZMON and the disassembler |
| `os_rom/hyforth/` | HyForth |
| `os_rom/shell/` | The shell's page 7 part: boot (the volumes, `boot.hys`), the prompt, the file and card commands, running programs |
| `os_rom/tests/` | POST and the self tests |
| `os_rom/tools/check_pages.js` | The cross-page call checker the build runs |
| `sim/` | The emulator, the regression tests, `tools/hydrafs.js` and `tools/mkhyx.js` ([tools](../tools/emulator.md)) |
| `programs/` | A sample program (`.hyx`), and the header and link config for building others ([programs.md](programs.md)) |

The **plans** in [../plans](../plans/) record the design reasoning, and what's still to come.
