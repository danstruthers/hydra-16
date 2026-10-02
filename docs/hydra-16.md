## **The Hydra-16: the whole system in one place**

The Hydra-16 is a multitasking 65C02 computer: a W65C02S whose address decoding gives each of 16 tasks its own RAM, zero page, stack and bank selections, running a small preemptive OS with Plan 9-style IO, and HyForth as its shell.

This page covers every part of it, hardware and software, with a short account of each and links to the documents that cover it in full.  Read it top to bottom for the whole picture, or jump to a part from the contents.  [All the documents](#16-all-the-documents) are listed at the end.

### **Contents**

1. [The machine in one page](#1-the-machine-in-one-page)
2. [Getting started](#2-getting-started)
3. [Hardware](#3-hardware)
4. [The ROMs](#4-the-roms)
5. [The kernel: tasks, interrupts, memory](#5-the-kernel-tasks-interrupts-memory)
6. [Input and output](#6-input-and-output)
7. [Namespaces](#7-namespaces)
8. [Storage: disks and HydraFS](#8-storage-disks-and-hydrafs)
9. [The shell: HyForth](#9-the-shell-hyforth)
10. [WOZMON, POST and the self tests](#10-wozmon-post-and-the-self-tests)
11. [Programs: assembly and C](#11-programs-assembly-and-c)
12. [Sound](#12-sound)
13. [Tools: the build and the emulator](#13-tools-the-build-and-the-emulator)
14. [The source tree](#14-the-source-tree)
15. [Plans and status](#15-plans-and-status)
16. [All the documents](#16-all-the-documents)

---

### **1. The machine in one page**

| | |
| :-- | :-- |
| **CPU** | WDC W65C02S, 3.58 MHz as built |
| **Tasks** | 16, each with its own 32K of RAM (`$0000-$7FFF`, zero page and stack included) and its own RAM and ROM bank selections: a task switch is one write to `T` |
| **Memory** | 512K task RAM; 2 MB shared RAM and up to 15 x 2 MB task RAM modules, through an 8K window at `$8000`; 4 MB paged ROM in 16K banks at `$A000`; an 8K-paged BIOS ROM at `$E000` |
| **Interrupts** | 16 prioritised IRQ lines, each with its own vector |
| **Devices** | 65C22 VIA (the tick, SPI, I2C, GPIO), 65C51 ACIA (the serial console), YM2151 (stereo FM sound), SD cards on SPI, an optional DS1747 clock |
| **Expansion** | 6 slots, 8 SPI device headers, a GPIO/I2C header |
| **OS** | A preemptive scheduler (200 Hz), IRQ handlers that run in their driver's task, a memory manager per task, shared memory, semaphores, exit statuses |
| **IO** | Plan 9 style: devices are file servers; each task has 12 fds, a namespace of union mounts and binds, and pipes |
| **Storage** | HydraFS on SD cards (`/sd/N`), the paged ROM as a read-only disk (`/rom`), RAM disks (`/ram`, `/sram`) |
| **Shell** | HyForth: a Forth that's also a shell, with a current directory, `\|` pipelines, redirection, and scripts and programs run by name |
| **Programs** | `.hyx` executables in assembly or C (cc65, with a library for the Hydra's calls) |
| **Tools** | An emulator that runs the real ROMs, regression tests, a hardware test, card and ROM disk image tools, a score compiler for songs |

How the software fits together:

```
  task 1: HyForth shell      task N: programs, pipeline stages, more shells
      |  IO_READ/IO_WRITE/...  (fds, namespace)
      v
  IO layer (page 2) ---TASK_CALL---> server in its driver task:  $F serial  $E sound  $D pipes  $C storage
      |                                                              ^
      v                                                              | IRQ handlers run in the driver's task
  kernel (page 0): scheduler, TASK_CALL, IRQ dispatch, MMU, shared memory
```

More: [the system in one page](programming/README.md#the-system-in-one-page) (the Programmer's Guide), [the hardware overview](hardware.md#overview).

---

### **2. Getting started**

* **[First steps](tutorial.md)**: a tutorial for the first hour, in the emulator: [switching it on](tutorial.md#1-switch-it-on), [HyForth](tutorial.md#2-hyforth), [files](tutorial.md#3-files), [a card](tutorial.md#4-a-card), [a program in C](tutorial.md#5-a-program-in-c) and [one in assembly](tutorial.md#6-a-program-in-assembly).
* **[Getting started](getting-started.md)**: [what you need](getting-started.md#what-you-need), [building](getting-started.md#building) (`node build.js`, with cc65 and Node.js), [programming the chips](getting-started.md#programming-the-chips), [connecting a terminal](getting-started.md#connecting-a-terminal) (9600 8N1, a straight-through cable), [the first boot](getting-started.md#first-boot), [the emulator](getting-started.md#without-the-hardware-the-emulator), [testing a change](getting-started.md#testing-a-change).

```
node build.js test                 build everything and run the regression tests
node sim/hydrasim.js -i            use the Hydra in your terminal, in the emulator (Ctrl-A x quits)
```

---

### **3. Hardware**

The **[Hardware Reference](hardware.md)** describes the main board from its schematics, sheet by sheet, and the companion cards.  The KiCad 9 projects are in `board/`.

#### **The memory system**

| Addresses | What | Selected by |
| :-------- | :--- | :---------- |
| `$0000-$7FFF` | Task RAM: each task's own 32K (`$00` and `$01` are its RAM and ROM bank registers: written there, read back from RAM) | `T` |
| `$8000-$9FFF` | Paged RAM: a task bank (`$00-$EF`) or a shared bank (`$F0-$FF`) | `$00`; `T` or `U` |
| `$A000-$DFFF` | Paged ROM, 16K banks | `$01` |
| `$E000-$FEFF` | BIOS ROM, 8K pages | `W` |
| `$FF00-$FFEF` | I/O ports 0-14, 16 bytes each | |
| `$FFF0-$FFF3` | The pseudo-registers `T`, `U`, `V`, `W` | |
| `$FFFA-$FFFD` | NMI and RESET vectors (BIOS ROM) | `W` |
| `$FFFE-$FFFF` | IRQ/BRK vector RAM, 16 entries | the active IRQ, or `V` |

* [The CPU view: memory map](hardware.md#the-cpu-view-memory-map).
* [The pseudo-registers T, U, V, W](hardware.md#the-pseudo-registers-t-u-v-w): the task (`T`), the shared macro-page (`U`), the vector select (`V`), the BIOS ROM page (`W`).  They have no reset: the ROM sets them.
* [Task RAM and the bank registers](hardware.md#task-ram-and-the-bank-registers): one HM628512 (or a DS1747, which adds a clock), and the per-task bank registers in 74LS219s.
* [The paged RAM window](hardware.md#the-paged-ram-window): task RAM modules on memory daughter cards, and the 2 MB of shared RAM on the board.
* [The paged ROM](hardware.md#the-paged-rom): eight SST39SF040, 256 banks of 16K, with the halves of each bank swapped.
* [The BIOS ROM](hardware.md#the-bios-rom): an SST39SF010/020/040 paged by `W`; changing `W` changes the code being run.

#### **I/O, interrupts, clocks and reset**

* [I/O space](hardware.md#io-space): 15 device ports of 16 bytes, and the system port.
* [Interrupts](hardware.md#interrupts): 16 lines through two 74LS148s into the vector RAM; the index is the line number XOR 7; line 15 is for software interrupts.
* [Clocks](hardware.md#clocks): a 14.318 MHz crystal divided down; the CPU clock is set by a jumper (J7, 3.58 MHz, as built).
* [Reset, power and bus control](hardware.md#reset-power-and-bus-control): ATX power, the front panel, the reset supervisor, `RDY`, DMA.

#### **The devices**

| Device | Port | IRQ line | Used for |
| :----- | :--- | :------- | :------- |
| [VIA, 65C22 (U2)](hardware.md#via-65c22-u2-port-0-irq-line-0) | 0, `$FF00` | 0 | Timer 1: the scheduler's tick; port A: GPIO and I2C (header J27); port B: [the SPI bus](hardware.md#spi-bus-via-port-b), with 8 device headers (J18-J25) |
| [ACIA, 65C51 (U3)](hardware.md#acia-65c51-u3-port-1-irq-line-1) | 1, `$FF10` | 1 | The serial console, on a DE-9 (DCE wiring) |
| [YM2151 (U38)](hardware.md#ym2151-sound-u38-port-4-irq-line-4) | 4, `$FF40` | 4 | 8-voice FM sound, through a YM3012 DAC and a mixer to a stereo jack |
| [Expansion slots](hardware.md#expansion-slots) 0-5 | 2-3, 5-14 | 2-3, 5-14 | Cards on the 62-pin bus: video in slot 0 (planned), and anything else |

#### **Cards, connectors and the board as built**

* [Companion cards](hardware.md#companion-cards): the [memory daughter card](hardware.md#memory-daughter-card) (a 2 MB task RAM module: `board/MemoryDaughterCard`) and the [bus breakout card](hardware.md#bus-breakout-card) (`board/HydraBusBreakoutCard`).
* [Connectors and jumpers](hardware.md#connectors-and-jumpers): fit J4 (RDY) and one CPU clock jumper (J7).
* [V1 errata](hardware.md#v1-errata): bank register bits 2/3 and 6/7 are crossed (the software and tools allow for it), no wait states, the audio jack's channels, the ACIA's clock.
* [Parts by function](hardware.md#parts-by-function).

#### **The schematics**

| Sheet (`board/`) | What | In the reference |
| :--------------- | :--- | :--------------- |
| `hydra-16.kicad_sch` | The root sheet: the CPU, the BIOS ROM, task RAM, the VIA, the ACIA | [Overview](hardware.md#overview) |
| `AddressDecode` | Address decoding, the I/O ports | [I/O space](hardware.md#io-space) |
| `FFF_Registers` | `T`, `U`, `V`, `W` | [The pseudo-registers](hardware.md#the-pseudo-registers-t-u-v-w) |
| `ZPMirrorRAM` | The bank registers `$00` and `$01` | [Task RAM and the bank registers](hardware.md#task-ram-and-the-bank-registers) |
| `SharedMem` | Shared RAM, the module selects | [The paged RAM window](hardware.md#the-paged-ram-window) |
| `BankedROM` | The paged ROM | [The paged ROM](hardware.md#the-paged-rom) |
| `IRQ_P_E` | The IRQ priority encoder and the vector RAM | [Interrupts](hardware.md#interrupts) |
| `Clocks` | The oscillator and its dividers | [Clocks](hardware.md#clocks) |
| `Buffers` | The address, data and R/W buffers | [Reset, power and bus control](hardware.md#reset-power-and-bus-control) |
| `Sound`, `Mixer` | The YM2151, its DAC, the mixer | [YM2151](hardware.md#ym2151-sound-u38-port-4-irq-line-4) |
| `Connectors` | The slots, headers, power | [Expansion slots](hardware.md#expansion-slots), [connectors](hardware.md#connectors-and-jumpers) |

---

### **4. The ROMs**

Everything the Hydra runs at boot is in two ROM images, from one build ([the two ROM images](programming/rom-layout.md#the-two-rom-images)):
* **The BIOS ROM** (`os_rom/bin/os_rom_C02.bin`, 128K): 16 pages of 8K, selected by `W`.
* **The paged ROM** (`os_rom/bin/paged_rom_C02.bin`): the whole paged ROM is one disk, the ROM disk, whose files are `/rom`; its banks 0 and 1 also hold HyForth's start-up data and the hardware test.

| Page | Contents |
| :--- | :------- |
| 0 | The kernel: reset, tasks, the scheduler, IRQ dispatch, the MMU, shared memory; the serial and sound drivers; the thunks |
| 1 | HyForth's interpreter, and its words' headers |
| 2 | The IO layer: fds, namespaces, `/dev/cons`, `/dev/ser` |
| 3 | Storage: SPI, the disks' block layer (cards, the ROM and RAM disks), `/dev/sd`, HydraFS's format and check |
| 4 | POST, the self tests, WOZMON |
| 5 | Far pointers, semaphores, exit statuses |
| 6 | The HydraFS server |
| 7 | The shell: boot, the prompt, the file commands, running programs, redirection |
| 8 | The text editor (`edit`) |
| 9 | Servers: `/proc`, `/env`, `/dev/time`, `/dev/ram`, the pipes, the serial port's settings |
| A | HyForth's far words, its error messages, the disassembler |
| B | Sound: the YM2151 library, `/dev/snd`, the patches |
| C | The song player |
| D-F | Free |

* [BIOS ROM pages](programming/rom-layout.md#bios-rom-pages): the full table, with the sources, and the fixed addresses on every page (the reset entry at `$E000`, the thunks at `$F800`, the COMMON block at `$FD00`).
* [Calling across ROM pages](programming/rom-layout.md#calling-across-rom-pages): gates and far calls.
* [Calling conventions](programming/rom-layout.md#calling-conventions): pointers in `.A.Y`; errors as C = 1 with the code in `.A`.
* [Zero page](programming/rom-layout.md#zero-page): the OS's, a task's own, a program's (`$E0-$FF`).
* [Error codes](programming/rom-layout.md#error-codes).
* [API index: the thunks](programming/rom-layout.md#api-index-the-thunks): every public entry point.
* [Adding code](programming/rom-layout.md#adding-code): where new code goes, and the space left on each page.
* The plan that set the pages' roles: [REORG_PLAN.md](plans/REORG_PLAN.md).

---

### **5. The kernel: tasks, interrupts, memory**

#### **Tasks**

| Task | Use |
| :--- | :-- |
| `$0` | The system task: boot, then the idle task |
| `$1` | The boot shell |
| `$2-$B` | Free: programs, pipeline stages, more shells |
| `$C` | The storage driver: `/dev/sd`, HydraFS |
| `$D` | The pipe server |
| `$E` | The sound driver |
| `$F` | The serial driver |

[Tasks and the scheduler](programming/tasks.md):
* [What a task is](programming/tasks.md#what-a-task-is): its hardware, its memory map, the task numbers.
* [Scheduling](programming/tasks.md#scheduling): preemptive, round robin at 200 Hz.
* [Starting tasks](programming/tasks.md#starting-tasks) and [ending them](programming/tasks.md#ending-tasks).
* [Exit statuses](programming/tasks.md#exit-statuses): Plan 9's `exits`, a code and a message.
* [Waiting and sleeping](programming/tasks.md#waiting-and-sleeping), [semaphores](programming/tasks.md#semaphores).
* [Running code in another task: `TASK_CALL`](programming/tasks.md#running-code-in-another-task-task_call): how a client's request runs in a server's task.
* [Signals: break and kill](programming/tasks.md#signals-break-and-kill), [the console's foreground task](programming/tasks.md#the-consoles-foreground-task).
* [Drivers: resident tasks](programming/tasks.md#drivers-resident-tasks), and the boot sequence.

#### **Interrupts**

[Interrupts](programming/interrupts.md): every IRQ runs the handler registered for its line, in the task that registered it.
* [IRQ lines](programming/interrupts.md#irq-lines), [how an interrupt is handled](programming/interrupts.md#how-an-interrupt-is-handled).
* [The fast handlers](programming/interrupts.md#the-fast-handlers-the-tick-the-serial-port-and-the-sound-clock): the tick, the serial port and the sound clock.
* [Registering a handler](programming/interrupts.md#registering-a-handler), [writing one](programming/interrupts.md#writing-a-handler).
* [Software interrupts](programming/interrupts.md#software-interrupts), [NMI](programming/interrupts.md#nmi), [timing notes](programming/interrupts.md#timing-notes).

#### **Memory**

[Memory](programming/memory.md):
* [The MMU](programming/memory.md#the-mmu-a-tasks-own-memory): each task's allocations, as handles, from its own RAM and banks.
* [Shared memory](programming/memory.md#shared-memory): the 256 shared banks, between tasks.
* [Far pointers](programming/memory.md#far-pointers), [references](programming/memory.md#references-far-pointers-as-handles), [copying](programming/memory.md#copying).
* The design: [MMU_PLAN.md](plans/MMU_PLAN.md).

---

### **6. Input and output**

Everything is a file.  A task opens names and reads and writes fds; the IO layer passes each request to the device's server, in the server's task (`TASK_CALL`), with the data in the task's **IO transfer area** in shared RAM (1.5K a task: the request, the data, the namespace).

[Input and output](programming/io.md):
* [Files and fds](programming/io.md#files-and-fds): 12 fds a task, the calls, inheritance.
* [stdio](programming/io.md#stdio): fds 0-2, buffered.
* [Devices](programming/io.md#devices):

| Name | What |
| :--- | :--- |
| `/dev/cons`, `/dev/cons/ctl` | [The console](programming/io.md#the-console), raw or cooked |
| `/dev/ser`, `/dev/ser/ctl` | The serial port, and [its settings](programming/io.md#the-serial-port-settings) |
| `/dev/snd` | [The YM2151](programming/io.md#sound-devsnd) |
| `/dev/sd/N/data`, `/dev/sd/N/ctl` | A disk as bytes, and its control file |
| `/sd/N/...` | [The files on a card](programming/io.md#the-files-on-a-card) (HydraFS) |
| `/proc/N/...` | [The tasks](programming/io.md#the-tasks-proc): status, `ctl`, `ns`, `cmd` (`send`) |
| `/env/NAME` | [The environment](programming/io.md#the-environment-env) |
| `/dev/time` | [The clock](programming/io.md#the-clock-devtime) |
| `/dev/ram` | [The RAM itself](programming/io.md#the-ram-itself-devram) (task 0's only) |
| `/rom/...` | [The ROM's files](programming/io.md#the-roms-files-rom) |
| `/ram/...`, `/sram/...` | [The RAM disks](programming/io.md#the-ram-disks-ram) |
| `/dev/null`, `/dev/zero`, `/dev/pipe` | The usual two, and [pipes](programming/io.md#pipes) |

* [The current directory](programming/io.md#the-current-directory), [stat](programming/io.md#stat).
* **Writing a device:** [drivers and file servers](programming/servers.md): [the model](programming/servers.md#the-model), [registering a device](programming/servers.md#registering-a-device), [the serve routine](programming/servers.md#the-serve-routine), [the request block and the transfer area](programming/servers.md#the-request-block-and-the-transfer-area), [waiting](programming/servers.md#waiting-when-theres-no-data-yet), [names and ctl files](programming/servers.md#names-and-ctl-files), [a checklist](programming/servers.md#checklist-for-a-new-device).
* The design: [IO_PLAN.md](plans/IO_PLAN.md).

---

### **7. Namespaces**

Names are put together the Plan 9 way: binds and mounts build one tree, and entries at the same path make a **union**, whose members are looked in, in order.  There are no search paths: programs are found in `.` then `/bin`, libraries in `/lib`, and `/bin` and `/lib` are unions of the caches, the card and the ROM.

* **Two tables:** each task's own (32 entries, inherited by the tasks it starts) and the **system namespace** (32 entries every task sees, under its own).  A name resolves to the longest matching prefix in either; on a tie, the task's own wins.
* **Mounts with a spec**, as Plan 9's `mount` takes one: `mount -s hfs /rom x` serves the ROM disk at `/rom`, and each shell's `mount hfs /ram r/N` gives it its own area of the RAM disk at `/ram`.
* **The default namespace** comes from `/rom/lib/namespace`, then a card's `lib/namespace`; `ns` prints the current one as the lines that would make it:

```
mount -s hfs /sd
mount -s env /env
mount -s proc /proc
mount -s hfs /rom x
mount -s hfs /sram s
bind -cs /ram/bin /bin
bind -as /sram/bin /bin
bind -as /sd/0/bin /bin
bind -as /rom/bin /bin
...
mount hfs /ram r/1
```

* In full: [namespaces](programming/io.md#namespaces) (the calls, the flags, how a name resolves, the namespace at boot).
* In the shell: [HyForth's files and devices](using/hyforth.md#files-and-devices) (`mount`, `bind`, `unmount`, `ns`).
* In C: [files and devices](programming/c.md#5-files-and-devices) (`hy_mount`, `hy_bind`, `hy_unmount`).
* The design, and what's left: [NAMESPACES.md](plans/NAMESPACES.md).

---

### **8. Storage: disks and HydraFS**

| Disk | Where | What |
| :--- | :---- | :--- |
| `0`-`7` | `/sd/N`, `/dev/sd/N` | An SD card on SPI device N (its number as one hex digit) |
| `x` | `/rom`, `/dev/sd/x` | The paged ROM, read-only: programs, libraries, songs, `boot.hys` |
| `r` | `/ram`, `/dev/sd/r` | The RAM disk: each shell's own area (`r/N`), kept until a reset |
| `s` | `/sram`, `/dev/sd/s` | The shared RAM disk, the same for every task |

* **HydraFS**, the filesystem on all of them: directories, extents, sparse files, partitions, a checker.  The spec: [HYDRAFS.md](plans/HYDRAFS.md).  On the Hydra: [the files on a card](programming/io.md#the-files-on-a-card), [the HydraFS server](programming/servers.md#the-hydrafs-server), [the storage layer](programming/servers.md#the-storage-layer-for-the-filesystem-server).
* **The ROM disk**, `/rom`: [io.md](programming/io.md#the-roms-files-rom), [its design](plans/DISKS.md#the-rom-disk); built from `os_rom/romfs.txt` by `sim/tools/mkromdisk.js` ([the ROM disk tool](tools/emulator.md#the-rom-disk)).
* **The RAM disks**, `/ram` and `/sram`: [io.md](programming/io.md#the-ram-disks-ram), [their design](plans/DISKS.md#ram-disks), [who can use which area](plans/DISKS.md#who-can-use-which-area), [the program caches](plans/DISKS.md#the-program-caches), [booting with or without a card](plans/DISKS.md#booting-finding-the-disks).
* **Cards from a PC:** [HydraFS card images](tools/emulator.md#hydrafs-card-images) (`sim/tools/hydrafs.js`), [putting a program on a card](programming/programs.md#putting-it-on-a-card).

---

### **9. The shell: HyForth**

HyForth is the Hydra's shell and its language: a Forth whose words include a current directory, file commands (`cd`, `ls`, `cp` ...), `|` pipelines, redirection, scripts (`.hys`), programs (`.hyx`) run by name, more shells in other tasks (`shell`), background tasks, and `send N line` to type a line into another shell.  `boot.hys` runs at boot.

[HyForth: the Hydra's shell](using/hyforth.md):
* [The basics](using/hyforth.md#the-basics), [numbers](using/hyforth.md#numbers), [strings](using/hyforth.md#strings), [defining words](using/hyforth.md#defining-words), [control flow](using/hyforth.md#control-flow-the-training-scripts).
* [Word reference](using/hyforth.md#word-reference), [the base and its libraries](using/hyforth.md#the-base-and-its-libraries).
* [The shell: directories, files and programs](using/hyforth.md#the-shell-directories-files-and-programs).
* [Files and devices](using/hyforth.md#files-and-devices), [pipelines](using/hyforth.md#pipelines).
* [Tasks and the console](using/hyforth.md#tasks-and-the-console), [background tasks and exit statuses](using/hyforth.md#background-tasks-and-exit-statuses).
* [Errors and keys](using/hyforth.md#errors-and-keys), [how HyForth uses memory](using/hyforth.md#how-hyforth-uses-memory).
* The design: [SHELL.md](plans/SHELL.md).

---

### **10. WOZMON, POST and the self tests**

[WOZMON, the disassembler, POST and the self tests](using/wozmon.md):
* [Getting to WOZMON](using/wozmon.md#getting-to-wozmon) (`bye` from HyForth), and [its commands](using/wozmon.md#commands), the disassembler among them.
* [Self tests](using/wozmon.md#self-tests): the MMU, the scheduler, IO.
* [POST: the power-on self test](using/wozmon.md#post-the-power-on-self-test): the two lines at boot, and what they mean.
* [The hardware test](using/wozmon.md#the-hardware-test): the registers, the memory, the ROMs' checksums and the devices, run from paged ROM bank 1.

---

### **11. Programs: assembly and C**

A program is a `.hyx` file: a header and code.  It's loaded into a task of its own, with the shell's fds, namespace, current directory and environment, and its arguments.

* [Programs](programming/programs.md): [the file](programming/programs.md#the-file), [what a program gets](programming/programs.md#what-a-program-gets), [building one](programming/programs.md#building-one) (`programs/asm/`), [C programs](programming/programs.md#c-programs), [putting it on a card](programming/programs.md#putting-it-on-a-card), and how `run` loads it.
* **[The C Programmer's Guide](programming/c.md)** (cc65, `programs/c/`): [quick start](programming/c.md#1-quick-start), [the machine as C sees it](programming/c.md#2-the-machine-as-c-sees-it), [building](programming/c.md#3-building), [arguments, environment, exit status](programming/c.md#4-a-programs-life-arguments-environment-exit-status), [files and devices](programming/c.md#5-files-and-devices), [the console](programming/c.md#6-the-console-stdio-and-conio), [running other programs](programming/c.md#7-running-other-programs), [time](programming/c.md#8-time), [semaphores](programming/c.md#9-tasks-working-together-semaphores), [errors](programming/c.md#10-errors), [memory](programming/c.md#11-memory), [assembly in C](programming/c.md#12-assembly-in-a-c-program), [performance](programming/c.md#13-performance), [debugging](programming/c.md#14-debugging-and-testing), [limits](programming/c.md#15-limits-and-gotchas), [`hydra.h`](programming/c.md#16-reference-hydrah), [working on the library](programming/c.md#17-working-on-the-library).
* **ROM programs:** the text editor (`edit`, page 8) and the song player (`play`, page C) run in tasks of their own, like any program.
* [Hydra executables](tools/emulator.md#hydra-executables): `sim/tools/mkhyx.js`, the header tool.

---

### **12. Sound**

A YM2151 (8 FM voices of 4 operators each) on port 4, mixed with the slots' audio to a stereo jack.

* The chip and its audio path: [YM2151](hardware.md#ym2151-sound-u38-port-4-irq-line-4).
* `/dev/snd`, the device: [io.md](programming/io.md#sound-devsnd).  In C: [`snd.h`](programming/c.md#sound-sndh).  In HyForth: `patch`, `note`, `noteoff`, `play`, `sndtest` ([the word reference](using/hyforth.md#word-reference)).
* The song player (ZSM files), and the score language that `os_rom/songs/test.mml` and `programs/songs/` are written in: [songs: the score compiler](tools/emulator.md#songs-the-score-compiler).
* The design, phase by phase, and importing music from other machines: [SOUND.md](plans/SOUND.md).

---

### **13. Tools: the build and the emulator**

* **The build** (`build.js`): the ROMs, the programs, the ROM disk, the cross-page call checker and the ROM space report ([building](getting-started.md#building)).
* **[The emulator and tools](tools/emulator.md):**
  * [Using the Hydra from your terminal](tools/emulator.md#using-the-hydra-from-your-terminal), and [its options](tools/emulator.md#usage), the profiler (`--profile`) among them.
  * [Regression tests](tools/emulator.md#regression-tests) (`sim/regress.js`, `sim/tests/`), which boot the real ROMs.
  * [HydraFS card images](tools/emulator.md#hydrafs-card-images), [Hydra executables](tools/emulator.md#hydra-executables), [the ROM disk](tools/emulator.md#the-rom-disk), [songs](tools/emulator.md#songs-the-score-compiler).
  * [What it models](tools/emulator.md#what-it-models), [inside the emulator](tools/emulator.md#inside-the-emulator).
* **The documentation as one PDF** (`sim/tools/mkpdf.js`): every document in `docs/` and the top README, made into [hydra-16.pdf](hydra-16.pdf) by a headless Microsoft Edge or Chrome, with the links between them kept and bookmarks for each document and section.  `node sim/tools/mkpdf.js` remakes it after a change.

---

### **14. The source tree**

| Folder | What |
| :----- | :--- |
| `os_rom/all.s`, `os_rom/os_rom_C02.cfg` | Includes everything, page by page; the linker config |
| `os_rom/include/` | Constants and macros: `hw.inc`, `kernel.inc`, `io.inc`, `zero.s`, `macros.inc` ... |
| `os_rom/kernel/` | Reset and boot, tasks, IRQs, the MMU, shared memory, far pointers, semaphores, exits, COMMON, gates, thunks |
| `os_rom/io/` | The IO layer, namespaces, pipes |
| `os_rom/servers/` | The console and serial port, `/dev/sd`, the RAM disks, `/proc`, `/env`, `/dev/time`, `/dev/ram` |
| `os_rom/drivers/` | Serial, sound, the VIA, SPI, SD cards and disks, the clock chip, the storage task |
| `os_rom/fs/` | HydraFS: the server, writing, sparse files, format, check |
| `os_rom/shell/` | Boot, the prompt, the file commands, running programs, redirection, the editor |
| `os_rom/hyforth/` | HyForth |
| `os_rom/sound/` | The YM2151 library, `/dev/snd`, the patches, the song player |
| `os_rom/monitor/` | WOZMON and the disassembler |
| `os_rom/tests/`, `os_rom/hwtest/` | POST and the self tests; the hardware test |
| `os_rom/romfs/`, `romfs.txt`, `songs/` | The ROM disk's files, and the test song |
| `os_rom/tools/` | The cross-page call checker, the ROM space report, the ROM checksums |
| `programs/asm/`, `programs/c/`, `programs/songs/` | Programs in assembly; the C library and samples; songs |
| `sim/` | The emulator (`hydrasim.js`, `lib/`), the regression tests, the tools |
| `board/` | KiCad 9: the main board, the memory daughter card, the bus breakout card |
| `docs/` | This documentation |

More: [where things are in the source](programming/README.md#where-things-are-in-the-source), [the repository](getting-started.md#the-repository).

---

### **15. Plans and status**

The plans record the design's reasoning and what's to come; parts of them are history, marked done.

| Plan | What | Status |
| :--- | :--- | :----- |
| [NEXT_STEPS.md](plans/NEXT_STEPS.md) | What's missing to make the Hydra fun and useful, and the [milestones](plans/NEXT_STEPS.md#milestones) | The roadmap |
| [IDEAS.md](plans/IDEAS.md) | The next features in order, wait states for board V2, open issues | Ongoing |
| [NAMESPACES.md](plans/NAMESPACES.md) | Unions, the default namespace, `/bin` and `/lib` | Mostly done |
| [PROC.md](plans/PROC.md) | `/proc`, and a task's memory as files | `/proc` done; the memory files to do |
| [DISKS.md](plans/DISKS.md) | The ROM and RAM disks, the program caches | Done |
| [VIDEO.md](plans/VIDEO.md) | The Vera X card in slot 0 | Planned |
| [SOUND.md](plans/SOUND.md) | The YM2151 library, the player, the test song | Done; importing music to do |
| [HYDRAFS.md](plans/HYDRAFS.md) | The filesystem's spec | Done |
| [IO_PLAN.md](plans/IO_PLAN.md) | The IO subsystem, the scheduler, the filesystem's steps | Done |
| [MMU_PLAN.md](plans/MMU_PLAN.md) | The memory manager, IRQ dispatch, drivers | Done |
| [SHELL.md](plans/SHELL.md) | HyForth as a shell | Done |
| [REORG_PLAN.md](plans/REORG_PLAN.md) | The ROM's reorganisation | Done |
| [CODE_REVIEW.md](plans/CODE_REVIEW.md) | A review of the whole software tree | Its recommendations [done](plans/CODE_REVIEW.md#what-was-done) |

---

### **16. All the documents**

| Document | Covers |
| :------- | :----- |
| [README.md](README.md) | The documentation's index |
| [tutorial.md](tutorial.md) | First steps, in the emulator |
| [getting-started.md](getting-started.md) | Building, the chips, a terminal, the first boot |
| [hardware.md](hardware.md) | The Hardware Reference |
| [using/hyforth.md](using/hyforth.md) | HyForth, the shell |
| [using/wozmon.md](using/wozmon.md) | WOZMON, POST, the self tests, the hardware test |
| [programming/README.md](programming/README.md) | The Programmer's Guide |
| [programming/rom-layout.md](programming/rom-layout.md) | ROM pages, gates, conventions, zero page, error codes, the API |
| [programming/tasks.md](programming/tasks.md) | Tasks and the scheduler |
| [programming/interrupts.md](programming/interrupts.md) | Interrupts |
| [programming/memory.md](programming/memory.md) | The MMU, shared memory, far pointers |
| [programming/io.md](programming/io.md) | fds, devices, pipes, namespaces |
| [programming/servers.md](programming/servers.md) | Drivers and file servers |
| [programming/programs.md](programming/programs.md) | `.hyx` programs |
| [programming/c.md](programming/c.md) | The C Programmer's Guide |
| [tools/emulator.md](tools/emulator.md) | The emulator and tools |
| [The plans](#15-plans-and-status) | Above |
