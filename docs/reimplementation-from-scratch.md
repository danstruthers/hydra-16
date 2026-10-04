## **Reimplementing the Hydra-16's software from scratch**

An evaluation of the Hydra-16's current software, and a step-by-step plan for building it again from the ground up, so that it ends at the same place (a 16-task, Plan 9-style computer with files, namespaces, pipes, a shell, storage, sound and, later, video) with a design that is consistent, easy to follow and easy to program for.

**Status:** a plan, written in October 2026 against branch `1.8C_0.6` (OS `1.8C_0.6`).  Nothing in the current tree is changed by it.

**Scope:**
* **Hardware:** the V1 board as built, errata included ([hardware.md](hardware.md#v1-errata)).  Changes that a V2 board could make to simplify the software are in [Appendix G](#appendix-g-v2-hardware-wishes).  The board and its schematics are a fixed reference here.
* **Replaced:** everything in `os_rom/` (the kernel, drivers, servers, the shell, HyForth, WOZMON, the self tests) and the program SDKs in `programs/`.
* **Kept, and built on:** the emulator, the regression test harness and the PC tools in `sim/`, the hardware test, the HydraFS on-disk format (so existing cards keep working), the ROM disk idea, the song tools and the ZSM format, and the `/pc` idea.

**Decisions already taken** (October 2026), which this plan follows:

| Question | Decision |
| :------- | :------- |
| The shell | A small **rc-like Plan 9 shell** is the base.  Over it come two interactive languages: **hylang** (the user's lisp-like language, danlang, reworked for the Hydra), which becomes the preferred command shell, and a **new HyForth**, written from scratch, as an option.  The first stages use rc alone; HyForth and hylang come once every system call they need exists.  WOZMON is dropped |
| Commander X16 programs | A **migration utility** rather than compatibility layers in the OS; if the two machines turn out too different, it waits until the end |
| Hardware baseline | V1 as built, with a V2 appendix |
| Languages | The kernel, drivers, interrupt paths and the language runtimes in **ca65 assembly**; **cc65 C** allowed for userland tools and non-critical servers |

**How to read it:**
* [Part I](#part-i-where-things-stand) is the evaluation: what the system is, what's worth keeping, and every issue found, with the evidence.
* [Part II](#part-ii-the-new-design) is the new design, subsystem by subsystem.
* [Part III](#part-iii-step-by-step) is the step-by-step plan, from an empty tree to the end point, then a feature map from the current system to the new one, and the risks.
* The [appendices](#appendices) hold the detailed formats: the API, the request block, the executable header, error codes, the default namespace, the danlang inventory, the V2 wishes and a glossary.

### **Contents**

**Part I: Where things stand**
1. [Summary](#1-summary)
2. [The current system in one page](#2-the-current-system-in-one-page)
3. [What's worth keeping](#3-whats-worth-keeping)
4. [Issues with the current design](#4-issues-with-the-current-design)

**Part II: The new design**

5. [Goals and principles](#5-goals-and-principles)
6. [The hardware facts that shape the design](#6-the-hardware-facts-that-shape-the-design)
7. [Architecture in one page](#7-architecture-in-one-page)
8. [Memory maps](#8-memory-maps)
9. [The ABI: how every call works](#9-the-abi-how-every-call-works)
10. [The kernel](#10-the-kernel)
11. [Modules: one executable format](#11-modules-one-executable-format)
12. [Files, servers and the protocol](#12-files-servers-and-the-protocol)
13. [Namespaces](#13-namespaces)
14. [Devices and drivers](#14-devices-and-drivers)
15. [Userland: init, rc, tools, SDKs](#15-userland-init-rc-tools-sdks)
16. [HyForth, rebuilt](#16-hyforth-rebuilt)
17. [hylang: danlang on the Hydra](#17-hylang-danlang-on-the-hydra)
18. [Commander X16 programs: the migration utility](#18-commander-x16-programs-the-migration-utility)
19. [Tools, tests and documentation](#19-tools-tests-and-documentation)

**Part III: Step by step**

20. [The phases](#20-the-phases)
21. [From the current system to the new one, feature by feature](#21-from-the-current-system-to-the-new-one-feature-by-feature)
22. [Risks and open questions](#22-risks-and-open-questions)

**Appendices**

A. [The API, by group](#appendix-a-the-api-by-group)
B. [The request block](#appendix-b-the-request-block)
C. [The executable and module header (HYX2)](#appendix-c-the-executable-and-module-header-hyx2)
D. [Error codes](#appendix-d-error-codes)
E. [The default namespace](#appendix-e-the-default-namespace)
F. [danlang to hylang: the inventory](#appendix-f-danlang-to-hylang-the-inventory)
G. [V2 hardware wishes](#appendix-g-v2-hardware-wishes)
H. [Glossary](#appendix-h-glossary)

---

## **Part I: Where things stand**

### **1. Summary**

The current OS is careful, well-tested work: a preemptive 16-task kernel, Plan 9-style IO with per-task namespaces and unions, a real filesystem with a checker, sound, a C toolchain, an emulator that runs the real ROMs and over 80 regression tests.  Its problems are not bugs; they come from **growth by accretion**.  Each feature was added where there was room, with the conventions that suited it at the time, so the system now has:
* **no boundary between the OS and its applications.**  The shell, HyForth, the editor and the song player live in BIOS ROM pages beside the kernel; the shell registers system devices and builds the system namespace; the kernel special-cases the shell's task.
* **several ways to do each thing:** three places a file server can run, seven ways to start a task, two interrupt paths, two directory formats, two kinds of device control, two parent pointers, registers *and* fixed zero page addresses *and* overloaded zero page addresses for parameters, and a carry flag that means success for two calls and failure for the rest.
* **exhausted fixed resources:** BIOS page 0 has 11 bytes free, page A 21, page 1 89 and the COMMON block 21; the OS zero page fills `$00-$B6` in every task, leaving programs 32 bytes; about 450 far-call gates and nearly 80 assembly-order aliases hold the pages together.
* **kernel state where any program can trample it:** the interrupt registration tables, the fd table and the memory manager's tables are in each task's own RAM, and the IRQ dispatcher trusts the interrupted task's copy.

The new design keeps the ideas that work (tasks per hardware slot, servers in their own tasks, files and namespaces Plan 9's way, HydraFS, the emulator) and rebuilds around a few rules applied everywhere:
1. **The BIOS ROM holds the kernel and nothing else.**  Drivers, the shell, languages and tools are **modules in the paged ROM**, executed in place in their own task's paged ROM bank (each task has its own `$01`), and call the kernel through one jump table like any program.
2. **One calling convention** (X16-style: `r0-r15` at `$02-$21`, programs' zero page at `$22-$7F`, carry set means an error in `.A`, always).
3. **One server model:** every device is a server task with the same entry points, built on one server library; every request is the same small block; data moves directly between the client's and the server's memory by one kernel copy routine.
4. **One place for each kind of state:** global kernel state in the kernel task (task 0), per-task kernel state in a fixed per-task area, a server's state in its own task, and shared RAM only for what programs share on purpose.
5. **One source of truth for the API:** a specification file generates the jump table, the assembly include, the C header and glue, the language bindings and the reference documentation.

The plan builds this in eleven phases.  Phases 0-5 reach the current system's everyday function with an rc shell; phase 6 adds HyForth, phase 7 hylang (which then becomes the login shell), phase 8 the Vera X screen and keyboard, phases 9-10 the debugger and the X16 migration utility.

---

### **2. The current system in one page**

| Layer | As built | Where |
| :---- | :------- | :---- |
| **ROMs** | A BIOS ROM of 16 pages of 8K switched by `W` (all code), and a paged ROM (256 banks of 16K switched per task by `$01`) holding HyForth's start-up data, the hardware test and the ROM disk | `os_rom/all.s`, `os_rom_C02.cfg` |
| **Page switching** | A 256-byte COMMON block, identical on every page, with far-call trampolines; about 450 6-byte gates (`FAR_GATE_INLINE`); a thunk table at `$F800` (83 entries) copied on pages 0 and 1 | `kernel/common.s`, the `pageN.s` gate files, `include/thunks.inc` |
| **Kernel** (page 0, parts on 5) | Tasks and a round-robin scheduler at 200 Hz, `TASK_CALL` (run a routine in another task's context), an IRQ dispatcher that runs each handler in the registering task, fast handlers for the VIA, ACIA and YM2151 on page 2, a 4-tier memory manager with handles, shared memory, far pointers and references, semaphores, exit statuses | `kernel/` |
| **IO layer** (page 2) | fds (12 a task, in the task's RAM), H9P requests to servers through a 1.5K "transfer area" per task in shared RAM, per-task namespaces (32 entries) over a system namespace (32), unions, current directory, stdio buffering, console fast paths, `TASK_CLONE` | `io/` |
| **Servers** | In driver tasks: serial/console, sound, pipes, storage (SD, HydraFS, `/dev/spi`, RAM and ROM disks), `/pc` (in the serial task).  In the client's task: null, zero, proc, env, time, ram, gpio, root | `servers/`, `fs/`, `drivers/`, `sound/` |
| **Shell** | HyForth (a Forth that is the shell) on pages 1 and A, with the shell's commands, prompt, redirection and program loader on page 7, the editor on page 8, the song player on page C | `hyforth/`, `shell/`, `sound/player.s` |
| **Programs** | `.hyx` files (16-byte header) loaded at `$0800`, a cc65 library (`programs/c/`), an assembly sample | `programs/` |
| **Tools** | `build.js`, the emulator (`sim/hydrasim.js`, `sim/lib/`), 80+ regression tests, the HydraFS image tool, the ROM disk builder, the song compiler, the PC tool | `sim/` |

---

### **3. What's worth keeping**

These carry straight into the new design, as ideas, as code to port, or as tools used unchanged:
* **The hardware's task model, used fully.**  One `T` write swaps a task's RAM, zero page, stack and bank selections.  Servers and drivers in tasks of their own, with their state in their task, is the right model for this board.
* **`TASK_CALL`'s model**: a request runs the server's code in the server's task, on its stack, preemptibly, one call at a time per server, with the client marked as waiting.  It's a synchronous RPC that needs no message queues.  The new kernel keeps it, made leaner.
* **Uniform waiting**: every wait is a 16-bit task mask plus a retry.  Waking a mask wakes everyone in it; each looks again.  Simple and robust.
* **Plan 9's ideas**: devices as file servers, ctl files, per-task namespaces with unions and `-b/-a/-c`, `/env` and `/proc` as files, exit statuses as a code and a message, `rc -c` for `system()`.
* **HydraFS**: its on-disk format (superblock, 64-byte entries with extents, qids, sparse files, partitions, quick format) is sound and has a PC tool; keep it byte for byte so existing cards work.
* **The disks**: the paged ROM as a read-only disk, RAM disks, each shell's own `/ram`, program caches as union members, disk names as one hex digit for SPI devices and letters for the rest.
* **The device code that's proven on the board**: the bit-banged SPI loops and their timing, the SD card command layer (SDSC and SDHC), the ACIA handling for both chips with paced sending at 115200, the YM2151 library (the shadow, General MIDI volumes, the X16's patch set), the ZSM player's timing, the DS1747 routines, POST's RAM line tests, the `/pc` framing and resend logic.
* **The hardware test**, which runs on its own from paged ROM bank 1 and needs no OS at all.
* **The tools**: the emulator (cycle-counted, devices modelled, faults injectable, a profiler, no Node.js in its core), the regression harness, `hydrafs.js`, `mkromdisk.js`, `hysong.js`, the PC tool, the ROM budget report and the page checker's idea.
* **The engineering habits**: a header on every routine (in, out, preserves), `.assert`s for layouts, the build checking itself, tests that boot the real ROM.

---

### **4. Issues with the current design**

Each issue gives the evidence (file and line where useful) and why it matters.  The section of Part II that addresses it is in brackets.

#### **4.1 Architecture and layering**

**A1. No boundary between the OS and its applications.**  The shell, HyForth, the editor, the song player and the disassembler are code in BIOS ROM pages beside the kernel, and they reach the OS through page-specific gates and trampolines (`FARWORD`, `FW_CALL`) rather than the public calls programs use.  The shell does system work: `SH_BOOT` (`shell/shell.s` lines 22-48) registers the `env`, `ram`, `time`, `gpio` and `root` devices and builds the system namespace, and the shell's buffers live in HyForth's RAM image (`PAGE1::SHBUF`).  The kernel special-cases the shell: a kill restarts task 1 instead of ending it (`kernel/tasks.s` lines 619-631).  So nothing can replace the shell without changing the kernel, and the ROM's own programs aren't examples of how to write a program.  [§7, §15]

**A2. Three places a server runs, each with its own rules.**  In a driver task (serial, sound, pipes, storage); in the client's task (`IO_DEV_CALLER_TASK`: null, zero, proc, env, time, ram, gpio, root); or inside another server's task (`/pc` in the serial task, `/dev/spi` in the storage task).  Client-task servers share 13 bytes of zero page by aliasing (`ZP_CS`, checked by `CS_FITS`), driver servers use the IO layer's own zero page as scratch (the pipe server keeps its client in `ZP_IO_CHUNK`), and blocking works differently in each.  A new server author has to learn all three.  [§12]

**A3. A driver's code is scattered across pages.**  The serial driver is on page 0 (`drivers/serial.s`), page 2 (`servers/ser_srv.s`, `serfast.s`), page 9 (`servers/serctl.s`) and page D (`servers/pc_srv.s`); storage is on pages 0, 3, 6 and D.  Each split was made for room, not for meaning.  [§11]

**A4. Fixed task numbers.**  `kernel.inc` fixes the shell at 1 and the storage, pipe, sound and serial drivers at `$C-$F`; adding a driver means editing `os_main.s` and choosing a number.  [§10.2]

**A5. One giant assembly.**  `all.s` includes every source inside `.scope PAGEn` blocks, in an order dictated by who refers to whom; it then needs 78 alias lines (`HFS_FORGET_P6 = PAGE6::HFS_FORGET` and so on) for references that go the "wrong way", and a page's gate labels silently shadow page 0 routines of the same name.  A routine's page can't be changed without reshuffling gates and aliases.  [§19.1]

#### **4.2 ROM space**

**B1. The fixed areas are full.**  The build's report (`tools/rom_space.js` on the current map) gives free bytes per page: 0: **11**, 1: **89**, 2: 565, 3: 486, 4: 2503, 5: 5844, 6: **118**, 7: 3373, 8: 5221, 9: 642, A: **21**, B: 1177, C: 6514, D: 3915, E and F: 7675, COMMON: **21**.  The kernel's page and the COMMON block can't take another feature.  (The paged ROM, 4 MB, is nearly empty, but nothing executes from it.)  [§8.3]

**B2. The thunk table, twice.**  The public entry points are copied on pages 0 and 1, with two entry kinds (`THUNK`, `THUNK_P0`), and the gates for newer calls go after the table because `GATES_P0` is full (`kernel/thunks.s`).  [§9.5]

**B3. Helpers duplicated per page.**  Ten copies of the same 8-byte bit-mask table (`MMU_BIT_MASKS`, `P2_BIT_MASKS`, `PIPE_BIT_MASKS`, `PROC_BITS`, `GPIO_BITS`, `HFS_BITS` twice, `SEM_BITS` ...) and a page 2 copy of `WRITE_HSTRING`, because reading another page's data costs a far call.  [§8.3]

#### **4.3 The ABI**

**C1. Parameters in four kinds of place.**  Registers (`.A.Y` a pointer, `.X` a mode); fixed zero page (`ZP_IO_BUF/CNT/OFS` at `$06/$08/$0A`, asserted not to move); other zero page addresses that "can move between builds" (`rom-layout.md`, zero page table); and overloaded zero page (`IO_CREATE` takes the new file's mode in `ZP_IO_BUF`'s low byte, `IO_MOUNT` a spec in `ZP_IO_CNT`, `TASK_EXITS` a message in `ZP_IO_BUF`, `TASK_START` an entry point in `ZP_TEMP_VEC`).  [§9]

**C2. Inconsistent registers between related calls.**  `IO_READ` takes the fd in `.A`, `IO_GETC` and `IO_PUTC` in `.X`.  16-bit values are `.A.Y` in the OS and `.A.X` in cc65, so every C binding shuffles them.  [§9.2]

**C3. Inconsistent error convention.**  Every call returns C = 1 with an error in `.A`, except `READ_CHAR` and `GET_CHAR`, where C = 1 means *success* with a key.  The C library's own include file describes `READ_CHAR` a third way (`programs/c/lib/hydra.inc`: ".A = a key, or 0: none").  [§9.3]

**C4. Seven ways to start a task**, each with its own inputs: `TASK_RUN` (`.A.Y` + `.X` page), `TASK_START` (`ZP_TEMP_VEC`, waits), `SPAWN_TASK`, `TASK_CLONE` (copies 30K), `TASK_PREPARE`, `DRV_START`, and `SHELL_CMD`, which is listed among the calls but is "not a call but an entry point".  [§10.6]

**C5. An unordered, unversioned public API.**  The 83 thunks are in the order they were written: self tests (`MMU_TEST`, `SCHED_TEST`, `IO_TEST`), printing helpers (`WRITE_HEX_MASK`, `CLEAR_SCR`), the disassembler and the file calls are interleaved, and entries can never move or go.  [§9.5]

**C6. Programs depend on OS internals.**  The C library's `lseek` reads the fd table at `$7DA0` directly (`programs/c/lib/io/lseek.c`); programs must know that a buffer in `$8000-$9FFF` can't be given to IO (the IO layer maps its transfer area there).  [§9.4, §12.4]

#### **4.4 Zero page and memory layout**

**D1. The OS takes most of every task's zero page.**  `$00-$B6` (183 bytes) is the OS's in every task (`__ZEROPAGE_SIZE__` = `$B7`), `$B7-$DF` is "may go to the OS later", and programs get `$E0-$FF`: 32 bytes, 26 of which cc65's runtime uses.  The X16 gives its programs 94.  [§8.1]

**D2. Drivers' private zero page declared globally.**  The serial driver's, the storage driver's, the sound driver's, the song player's and `/pc`'s task zero page are all declared in `include/zero.s`, so a change to one driver's variables is a change to a shared file, and unrelated code aliases bytes: `EXIT_P = ZP_SLEEP_SCAN` (`kernel/exits.s` line 14), `RTC_ZBUF = ZP_TIME`, HydraFS borrowing the SD server's `SD_*` bytes.  The aliasing is correct today only because the code paths happen not to meet.  [§8.1]

**D3. Kernel tables where a program's stray write lands.**  The IRQ registration tables are replicated into every task's `$7D00-$7D9F` and the dispatcher reads **the interrupted task's copy** (`kernel/irq.s`); the fd table is at `$7DA0` and the memory manager's state at `$7E00`, all in the task's own writable RAM.  A program with a wild pointer can break interrupt dispatch for the whole machine.  Global tables (devices, environments, semaphores, exit records, shared memory) are in shared bank `$00`, which any task can map.  [§10.1]

**D4. A hardware quirk in every task's layout.**  A DS1747 in U7 puts its clock registers at task F's `$7FF8-$7FFF`, so every task's memory manager stops 8 bytes short of `$8000` (`MMU_MAX_HANDLES` is computed against `RTC_REGS`).  [§8.1]

**D5. The IO buffer rule.**  "Never hand the IO calls a buffer in the window": a restriction every program must remember, caused by the transfer-area design.  [§12.4]

#### **4.5 Memory management**

**E1. An allocator bigger than its users need.**  Four tiers (1-3 bytes inside the handle, chunks of 4-64 bytes in five classes, page runs, bank runs), 1-byte handles (101 at most), locking, references to static data, and far pointers with four kinds and a read-only bit (`kernel/mmu.s`, 1478 lines; `kernel/fp.s`).  Yet C programs use cc65's `malloc`, and HyForth its own memory records: a C program runs three allocators.  [§10.5]

**E2. Far pointers exist mostly for the page layout.**  Their main use is reading a caller's name from whichever BIOS page it was on, a consequence of applications living on BIOS pages (A1).  [§10.5]

**E3. Private resources managed like shared ones.**  Each task's RAM banks are private by hardware (`T` selects the task's slice of a module), yet they're allocated through bitmaps with handles as if contended.  [§10.5]

#### **4.6 The IO layer**

**F1. Special cases inside `IO_OPEN`.**  `/` and `/dev` are rewritten to `/dev/root` names (`io/io.s` lines 478-507); a name the namespace doesn't match falls back to a built-in `/dev/` prefix; an open of the console is recognised by its fid *and* by `IO_DEV_IS_SERIAL`, which compares serve-routine addresses with `PC_SERVE` to tell the console from `/pc` (`io/io.s` lines 572-581 and 1245-1278); `IO_CHDIR` is an open with a pseudo-request (`IO_CALL_CHDIR`); `IO_REMOVE` borrows an fd.  [§12, §13]

**F2. Two directory formats.**  A directory reads as text lines (`name size`) or, with `IO_MODE_STAT`, as stat records; every directory server implements both.  [§12.6]

**F3. Two control mechanisms.**  Binary `IO_CTL` codes and text ctl files overlap (`SER_CTL_RATE` and `/dev/ser/ctl`'s `b9600`; `SND_CTL_*`; `SD_CTL_INIT` and `init`).  [§12.5]

**F4. Small fixed limits everywhere.**  A 256-byte IO unit (a 16K program load is 64 requests, 31% of its time per [DISKS.md](plans/DISKS.md#how-the-ram-disks-work)); 12 fds; 16 devices with 8-character names; mount prefixes of 13 characters and targets of 15; a current directory of 63; one union-directory fd per task, with a name of about 50; 8 HydraFS files open in the whole system; 8 pipes; 16 semaphores; 256 bytes of environment a task; `/proc/N/mem` 64 bytes a read.  [§12, §13]

**F5. Every byte copied two or three times.**  Client buffer to transfer area to server cache, and back: 55% of a program's load time from the RAM disk ([DISKS.md](plans/DISKS.md#how-the-ram-disks-work)).  [§12.4]

**F6. Library work inside the kernel.**  stdout buffering (`$0700`), stdin read-ahead (`$0780`), line buffering for the console, and console fast paths that reach into the serial task's rings (`SER_CONS_PUTS`, `SER_CONS_GETC`) are all in the IO layer, with special cases to decide when they apply.  [§12.7, §15.5]

**F7. The stat record exposes an internal number.**  Its byte 33 is "the card" (`IO_ST_CARD`), which for the disks in memory is the storage driver's table index (8, 9, 10), though the rule is that those numbers never show.  [§12.6]

#### **4.7 Namespaces**

**G1. Plan 9's model, with string limits and leftovers.**  Entries are path strings of 13 and 15 characters.  `/dev` still resolves outside the namespace; `newns` keeps `"/ram"` because it's hard-coded (`io/ns.s` line 1367, `NS_RAM_PATH`); `/proc` and `/dev/proc` are two names for one device; `$PATH` and `$LIBPATH` are still searched after `/bin` and `/lib`, contrary to the stated design.  [§13]

**G2. Four copies of the system namespace.**  It's copied into each of the four IO transfer banks (`NS_SYS`, `NS_SYS_SYNC`) so a lookup needn't switch banks, and changing a system union in a task copies its members into the task's own table first.  [§13]

#### **4.8 Tasks, scheduling and interrupts**

**H1. Two interrupt paths.**  The dispatcher plus `TASK_CALL_IRQ` costs about 650 cycles per interrupt, too slow for the ACIA at 115200, so the VIA, the ACIA and the YM2151 each have a bespoke fast handler on page 2 reached by its own COMMON stub, with "quick looks" into the driver's zero page; the registered handlers remain for the rare work.  Every new fast device needs COMMON bytes (21 left).  [§10.3]

**H2. Ad-hoc task switching.**  221 writes to `T` are spread through the code, each a "quick look" whose safety depends on a "no stack use!" comment.  The trick is sound; its scattering isn't.  [§10.1]

**H3. Two parent pointers.**  `TASK_PARENT` (whom to wake) and `ZP_TASK_OWNER` (the family, for signals and permissions), with `TASK_SIGNAL` following owners only 4 levels and `TASK_MAY` the whole chain.  [§10.7]

**H4. Signals are flags, and handlers are fragile.**  Only break and kill exist; a break handler set through a gate keeps a stack pointer 3 bytes too deep, which callers correct by hand (`tasks.md`, `TASK_SET_BREAK`; `shell/edit.s`'s `ED_SP`).  [§10.7]

#### **4.9 The shell and HyForth**

**I1. A non-standard Forth.**  `*` leaves a double, `/` leaves remainder and quotient, `min` leaves both values, `neg` is the one's complement, `cons` and `var` replace `CONSTANT` and `VARIABLE`, control flow has to be loaded from "training scripts" in the paged ROM, numbers print as 4 hex digits by default, and `\` isn't a comment.  Forth books and tutorials don't apply.  [§16]

**I2. A shell grammar grafted onto Forth.**  Each command has a parsing form for typing (`cd games`) and a stack form for code (`"games" (cd)`); only the stack form works in a definition; `|`, `>`, `>>` and `<` are words scanned specially; strings are made as they're read, not compiled.  [§15.3]

**I3. ROM coupling between the two ROMs.**  HyForth's variables are copied from paged ROM bank 0 by address (`COPYTORAM`), and its sample binary words call BIOS addresses, so the two images must always be burned together.  [§11]

**I4. A complex library mechanism.**  Eight ROM libraries as bits of `LIBSET`, four file libraries in `LIBSET2` with their own slots, a separate search order.  [§16]

#### **4.10 Storage**

**J1. A good filesystem on an awkward page split.**  HydraFS is on pages 3 and 6 with about 20 cross-page aliases, borrows the SD server's zero page, and keeps its state at fixed RAM addresses held together by many `.assert`s.  [§14.3]

**J2. Permissions as special cases.**  Each shell's RAM disk area is protected inside the HydraFS server by name (`HFS_AREA_CHECK`), and a task's end reaches from the kernel into the storage task (`TASK_ORPHANS` to `HFS_AREA_END`).  [§14.3]

#### **4.11 Documentation and repository**

**K1. Drift.**  Reference text no longer matches the code in places: `io.md` says `/env` needs no mount (line 288; it's mounted now); the assembly sample says programs' zero page starts at `$A9` (`programs/asm/samples/hello.s`; it's `$E0`); `io.inc` describes `H9_STAT` as a 16-byte record (line 261; it's 48); `hydra.inc` describes `READ_CHAR`'s result differently from the docs.  [§19.4]

**K2. Hard to scan.**  The reference chapters are complete but dense: long sentences with nested parentheses, few examples per call, and conventions explained once, far from where they're used.  A generated API reference and shorter chapters would help newcomers.  [§19.4]

**K3. Build outputs in Git.**  The ROM images and C binaries are committed, so every build changes tracked files and branches conflict on them.  [§19.1]

---

## **Part II: The new design**

### **5. Goals and principles**

**Goals:**
1. **The same end point**: everything the current system does ([§21](#21-from-the-current-system-to-the-new-one-feature-by-feature) maps it feature by feature), plus room for video, input, networking and languages.
2. **Consistent**: one way to do each thing, the same convention in every call, the same structure in every server.
3. **Easy to program for**: a program sees a small, documented API, a large zero page, a simple memory map, ordinary files for everything, and errors it can print.
4. **Easy to follow**: the kernel small enough to read; each subsystem in its own module; the rules written down once and checked by the build.
5. **Fun and functional**: fast enough to feel instant at 3.58 MHz, robust enough that a crashing program doesn't take the machine down, and open to hobbyists' hardware.

**Principles (the rules the design applies everywhere):**

| # | Principle | What it means in practice |
| :- | :-------- | :------------------------ |
| P1 | **Kernel in the BIOS ROM, everything else in modules** | The BIOS ROM's pages hold the kernel only.  Drivers, servers, the shell, languages and tools are modules: in the paged ROM, executed in place, or loaded into RAM |
| P2 | **Everything runs with `W` = 0 except the kernel** | Programs and modules call the kernel through one jump table on page 0.  Only kernel code ever changes `W` |
| P3 | **One ABI** | Arguments in `.A/.X/.Y` and `r0-r15`; C = 1 means an error code in `.A`; no exceptions (§9) |
| P4 | **One place for each kind of state** | Global kernel state in the kernel task; per-task kernel state in the task's OS area; a server's state in its task; shared RAM only for what programs share on purpose (§10.1) |
| P5 | **One server model** | Every device is a server task with the same entry points, built on the server library, answering the same request block (§12) |
| P6 | **One copy** | Data moves once, directly between the client's and the server's memory (§10.4) |
| P7 | **Files for control** | Control is text written to ctl files; there's no binary ctl call (§12.5) |
| P8 | **Names the Plan 9 way** | Devices have `#` names; everything else is built by binds and mounts in a namespace file; no search paths, no built-in prefixes (§13) |
| P9 | **One source of truth** | The API, the error codes and the constants are written once, in a specification, and generated into every place that needs them (§9.5, §19.2) |
| P10 | **A program can only hurt itself** | Nothing a program can reach with an ordinary stray pointer (its own RAM and banks) holds state other tasks depend on.  (There's no hardware protection: a program that writes `T` deliberately can still do anything; the aim is robustness against accidents) |
| P11 | **Measure, then commit** | Each performance-critical mechanism has a budget and an emulator test before the code that depends on it is written (§20, the spikes) |

---

### **6. The hardware facts that shape the design**

These are the facts from [hardware.md](hardware.md) that the design is built on, and what it does with each.

| Fact | Consequence for the design |
| :--- | :------------------------- |
| `T` (`$FFF0`) selects a task's `$0000-$7FFF`: zero page, stack and RAM.  One write switches all of it | A task switch is cheap.  A "quick look" (switch `T`, touch a few bytes with no stack use, switch back, IRQs off) reaches another task's memory: the kernel wraps this in a few primitives (§10.1) instead of scattering it |
| `$00` and `$01` are per-task hardware registers (the RAM bank at `$8000`, the paged ROM bank at `$A000`), mirrored in RAM | **Each task can execute from its own paged ROM bank.**  A module in the paged ROM runs in place in any task that selects it, and an interrupt or a task switch keeps it selected.  This is what lets everything but the kernel leave the BIOS ROM |
| Task RAM banks on the memory modules are per task (`T` selects the slice) | A task's banks are its own: no global allocator is needed for them (§10.5).  Up to 16 banks (128K) per installed module per task |
| Shared RAM is 256 banks of 8K, through `U` (global) and `$00` = `$F0-$FF` | Shared memory and the shared RAM disk.  `U` is global, so it's saved in each task's frame (as now) |
| `W` selects the BIOS ROM page at `$E000-$FEFF`, globally, and isn't reset by hardware | Page switching needs identical code at the same address on every page (the COMMON block).  Keeping `W` = 0 outside the kernel keeps COMMON small (P2) |
| I/O is at `$FF00-$FFEF`; `$FFF0-$FFF3` are `T U V W`; `$FFFE/F` is a 16-entry vector RAM, one vector per IRQ line (index = line XOR 7) | Each line has its own vector, so dispatch can go straight to the line's owner.  BRK reads entry `V0-V3` when no line is active |
| No memory protection, no privileged mode | Robustness by placement (P10), not enforcement.  [Appendix G](#appendix-g-v2-hardware-wishes) suggests a V2 protection bit |
| The 65C51 holds one received byte; at 115200 a byte arrives every 320 cycles (3.58 MHz) | Interrupts may never be off for more than about 200 cycles, anywhere.  This is the system's hardest budget, and it's tested (§19.3) |
| V1 errata: bank bits 2/3 and 6/7 crossed; the paged ROM's 8K halves swapped; the ACIA clock 1.79 MHz; no wait states | Hidden in the image builder and the emulator, as now.  Software always uses logical bank numbers |
| A DS1747 in U7 puts its clock at task F's `$7FF8-$7FFF` | Contained in one place: task F is only ever given to a driver, and a driver's RAM ends below `$7F00` (§10.2) |

---

### **7. Architecture in one page**

```
  BIOS ROM (W)                         paged ROM ($01, per task)              RAM
  +------------------------------+     +------------------------------+     +-----------------------------+
  | page 0: reset, COMMON, IRQ   |     | bank 0: partition table,     |     | each task: ZP, stack, OS    |
  |   entry and dispatch, task   |     |   module directory           |     |   area, its program's RAM   |
  |   switch, SCALL/KCALL, kcopy,|     | bank 1: hardware test        |     | each task's banks ($8000):  |
  |   the API jump table ($F800) |     | banks 2..: modules (16K)     |     |   heaps, buffers, RAM disk r|
  | pages 1-3: the kernel's      |     |   drivers: cons, storage,    |     | task 0's RAM and banks:     |
  |   services (tasks, memory,   |     |   sound, video ...           |     |   all global kernel state   |
  |   files, namespaces, kernel  |     |   programs: init, rc, tools, |     | shared RAM: shared segments,|
  |   devices, loader)           |     |   HyForth, hylang, edit, play|     |   the shared RAM disk s     |
  | page 4: POST and diagnostics |     |   libraries: srvlib, math ...|     +-----------------------------+
  | pages 5-F: room to grow      |     | banks N..255: the ROM disk   |
  +------------------------------+     +------------------------------+

  task 0          kernel task: global tables, kernel devices (#/ #p #e #| #t #m ...), idle
  task 1          init: builds the namespace, starts the late drivers and the console shell, adopts orphans
  tasks F, E, ... drivers (started from the module directory; task F first: the console)
  the rest        programs, shells, pipeline stages
```

How a request flows:

```
  program (task 5, W = 0)
    jsr OPEN / READ ...  ($F8xx: the jump table)
        |
        v
  syscall stub (kernel code, in task 5's context)
    checks arguments; fd -> channel (per-task table)
        |  structural work (open, close, namespace, spawn): KCALL into task 0 (one at a time, no locks)
        |  device work: SCALL straight into the server's task (the kernel task is never held)
        v
  server task (e.g. storage, W = 0, its module in its paged ROM bank)
    serve(request) with the server library: walk, read, write, stat ...
    data: kcopy between its RAM and task 5's buffer, wherever that buffer is
    no data yet: register task 5 in a wait mask, return E_AGAIN; the stub sleeps and retries

  IRQ (any line)
    COMMON stub -> page 0 dispatcher -> switch to the owner task's stack -> its module's irq entry
    -> back (or a task switch, for the tick)        about 100 cycles, one path for every device
```

---

### **8. Memory maps**

#### **8.1 A task's view**

| Addresses | What | Notes |
| :-------- | :--- | :---- |
| `$00`, `$01` | Bank registers (RAM bank at `$8000`, paged ROM bank at `$A000`) | Hardware: written there, read back from RAM |
| `$02-$21` | **r0-r15**: the call registers, 16 bits each | Arguments and results of calls; clobbered by any call.  The same place and use as the X16's |
| `$22-$7F` | **The program's zero page** (94 bytes) | Never touched by the OS.  cc65's runtime goes here (as on the X16 target), as do HyForth's and hylang's |
| `$80-$FF` | **The OS's per-task zero page** (128 bytes) | The task's scheduler state, the syscall stubs' scratch, kcopy's pointers.  About 64 bytes used at first; the rest reserved.  Laid out in one file, owned by the kernel |
| `$0100-$01FF` | The stack | The task's own |
| `$0200-$03FF` | **The task's OS area** (512 bytes) | The fd table (channel numbers), the note handler, the arguments and name passed at spawn, a server's request inbox (`$0200`) and name/stat buffer (`$0300`) |
| `$0400-$07FF` | Free for the program (1K) | As the X16's "golden RAM" |
| `$0800-$7FFF` | **The program** | Code, data, heap, the C stack.  A RAM program loads at `$0800`.  (Task F only: its driver stops below `$7F00`: the DS1747's registers) |
| `$8000-$9FFF` | The task's selected bank: one of its own (`$00-$EF`) or a shared one (`$F0-$FF` with `U`) | |
| `$A000-$DFFF` | The task's selected paged ROM bank | A module's code, when it runs in place |
| `$E000-$FEFF` | BIOS ROM page `W` (always page 0 outside the kernel) | The jump table at `$F800` |
| `$FF00-$FFFF` | I/O, `T U V W`, the vectors | |

Compared with today: programs get **94 bytes of zero page instead of 32**, the whole of `$0800-$7FFF` instead of `$0800-$7BFF` less the memory manager's pages, and no OS tables in their RAM but their own fd table.

#### **8.2 The kernel task (task 0)**

Task 0 is the kernel's: its RAM and its banks hold every global table, out of reach of other tasks' stray writes (P10).

| Where | What |
| :---- | :--- |
| ZP `$22-$7F` | The kernel's own pointers and scratch while it runs a KCALL |
| `$0200-$7FFF` | The task table (16 entries), the channel table (open files, shared by fds), the server registry, the IRQ owner table, the sleep queue, the module directory (cached), mount tables and their string pool, environment blocks, semaphores, exit records, shared-segment map, the kernel devices' state |
| Task 0's banks | Large or cold tables: pipe buffers, the system's error texts, room to grow |

#### **8.3 The ROMs**

**BIOS ROM** (the 128K chip; 16 pages):

| Page | Contents |
| :--- | :------- |
| 0 | Reset entry, POST launcher, COMMON block, IRQ entry and dispatcher, scheduler and task switch, SCALL and KCALL, kcopy, the quick-look primitives, the API jump table at `$F800` |
| 1 | Tasks (spawn, exits, wait, notes), memory |
| 2 | Files: channels, the request path, the syscall stubs for IO |
| 3 | Namespaces, the module loader, the kernel devices |
| 4 | POST's RAM tests and the hardware probe |
| 5-F | Free: kernel growth only (a new device never needs BIOS space) |

Every page starts with the reset stub at `$E000` (as now: `W` isn't reset) and has the COMMON block at a fixed address, which now holds only the IRQ stubs, the IRQ exit, the NMI entry and the kernel's internal far call: far smaller than today's.

**Paged ROM** (4 MB; 256 banks of 16K):

| Banks | Contents |
| :---- | :------- |
| 0 | Block 0: the partition table (as now).  Then the **module directory**: for each module its name, type, bank, flags (boot driver, program, library), version |
| 1 | The hardware test (unchanged: standalone) |
| 2-N | **Modules**, one or more banks each: drivers, programs, libraries (§11) |
| N+1-255 | **The ROM disk**: a HydraFS volume (`/rom`): data files, songs, scripts, the namespace file, help, fonts |

The build writes the image in the chips' order (the A13 half swap, V1's bank-bit swaps), as `mkromdisk.js` does today.

#### **8.4 Shared RAM**

| Shared bank IDs | Use |
| :-------------- | :-- |
| `$00-$7F` | The shared RAM disk (`/sram`), as now, sized at boot |
| `$80-$FF` | Shared segments for programs (§10.5) |
| Any on a chip POST found bad | Reserved |

No kernel tables are in shared RAM (D3), and there are no IO transfer areas (P6).

---

### **9. The ABI: how every call works**

One convention for every call: kernel calls, module entry points, library routines.

#### **9.1 Calling**

* A program calls the kernel with `jsr` to its entry in the jump table on BIOS page 0 (`$F800` up).  Programs and modules always run with `W` = 0, so the table is always there (P2).
* Calls preserve the decimal flag (clear) and the I flag as it was on entry.  They may enable interrupts only if they were enabled.

#### **9.2 Arguments and results**

| What | Where |
| :--- | :---- |
| A small primary argument (an fd, a task, flags, a byte) | `.A` |
| Further small arguments | `.X`, then `.Y` |
| Pointers and 16-bit values | `r0`, `r1`, `r2` ... in the order the call's signature lists them |
| 32-bit values (offsets, times) | A pair of registers: `r0` low word, `r1` high word (or the next pair) |
| A byte result | `.A` |
| A 16-bit result | `.A` low, `.X` high (cc65's convention, so C bindings need no shuffling) |
| More results | `r0`, `r1` ... |
| Success / failure | **C = 0 success; C = 1 failure with the error code in `.A`.  Always.**  "Nothing yet" is an error (`E_AGAIN`), and so is the end of a file read a byte at a time (`E_EOF`); a block read at the end of a file is a success with a count of 0 |

**Clobbered** by every call: `.A`, `.X`, `.Y`, `r0-r15`, N, Z, V.  **Preserved**: `$22-$7F`, the stack, the task's RAM, its bank selections (`$00`, `$01`) and `U`.

#### **9.3 Strings, buffers and limits**

* **Strings** are zero-terminated.  A path is at most 255 bytes, a file name 31 (HydraFS's).  Every call enforces the same limits and returns `E_NAMETOOLONG` beyond them.
* **Buffers and names may be anywhere the caller can see**: its task RAM, its selected RAM bank at `$8000` (own or shared), or (for data the kernel only reads) its selected paged ROM bank.  The kernel reads and writes them with the caller's mappings (§10.4), so there's no rule like today's "not in the window" (D5).
* **Counts** are 16 bits; a read or write may move any count, and returns how much it moved.

#### **9.4 What programs may rely on**

Only the jump table, the zero page split above, the memory map above, the header format ([Appendix C](#appendix-c-the-executable-and-module-header-hyx2)) and the documented contents of the OS area that are marked public (the arguments block, the program's name).  Everything else, the fd table included, is the kernel's, and may change between versions.  `seek` returns the new offset, so no library needs to read kernel tables (C6).

#### **9.5 The jump table and the API specification**

The jump table is **generated** from one specification file, `spec/api.def` (a simple text format; JSON or YAML would do), with one entry per call:

```
call READ  group=file
  in   A=fd  r0=buffer  r1=count
  out  A/X=count done
  err  E_BADF E_INTR E_AGAIN E_IO
  blocks yes
  since 1
  doc  Reads up to count bytes from fd into buffer.  0 is the end of the file.
```

From it the build makes:
* the jump table (`$F800` up), grouped: each group has a base and spare slots, so a group grows without moving another ([Appendix A](#appendix-a-the-api-by-group));
* `hydra.inc` for assembly programs (addresses, constants, error codes, a `CALL name` macro that documents the registers in the listing);
* `hydra.h` and the C bindings' glue for cc65 (one assembly stub per call, made by the generator);
* the binding tables HyForth and hylang use to call the kernel by name;
* the API reference chapter of the documentation;
* the emulator's symbol table, so `--trace-calls` prints `READ(fd=3, count=256) -> 256`;
* a test that every documented call exists and every jump table entry is documented.

**Rules:** entries are appended, never moved; a withdrawn call keeps its slot and returns `E_NOSYS`; `SYSINFO` returns the ABI version, so a program can check it.  Self tests and debugging aids are not in the public API (they're programs, or emulator features).

---

### **10. The kernel**

#### **10.1 Where state lives, and how the kernel reaches it**

| Kind of state | Lives in | Reached by |
| :------------ | :------- | :--------- |
| Global: tasks, channels, servers, mount tables, pipes, environments, semaphores, exit records, sleep queue, IRQ owners, shared-segment map | The kernel task (task 0): its RAM and banks | **KCALL**: a call into task 0, which runs one at a time, so the tables need no locks (only IRQ-off sections for the few fields an interrupt handler also touches).  For a single field, **KGET / KPUT** (a quick look) |
| Per task, hot: scheduler state, status bits, wait bits, call nesting, preemption count | The task's OS zero page (`$80-$FF`) | Directly, in the task; another task's by a quick look (the scheduler, wake-ups) |
| Per task, warm: fd table, note handler, spawn arguments, a server's inbox | The task's OS area (`$0200-$03FF`) | Directly, in the task |
| A server's own | The server's task | The server, in its task |
| Shared between programs | Shared segments | The programs that attached them |

**The quick-look primitives** are a handful of macros and routines in one file, `kernel/kx.s`: `KGET task, addr` and `KPUT task, addr` (a byte, IRQs off, no stack), `KWAKE task`, `KSTATUS task`, and kcopy (§10.4).  No other code writes `T` (the build checks this, as it checks gates today).  This replaces the 221 scattered switches (H2).

#### **10.2 Tasks**

* **The task table** (in task 0): for each task its state, its parent, its note group, its module (or program), its name, its exit record, its CPU time (ticks).
* **States are a small enum** (free, starting, ready, waiting, sleeping, calling, exiting) plus a few independent bits (driver, note pending, guest switched out); the scheduler's test is one table lookup.
* **Numbering:** task 0 is the kernel and the idle task; task 1 is `init`; drivers are given tasks from the top down (`$F`, `$E` ...) as they start, so task F is always a driver (§6, the DS1747); programs get the lowest free task.  No number is fixed anywhere but 0 and 1 (A4).
* **One parent**, the task that spawned it.  When a task ends, its children's parent becomes `init` (as Unix gives orphans to `init`), which also collects their exit records (H3).
* **Note groups** (Plan 9's): a task joins its parent's note group unless it asks for a new one.  The console sends an interrupt to the note group of the window that has the keyboard (§14.2); `kill` of a group ends all of it.  This replaces the owner-chain walks.

#### **10.3 Interrupts: one path, made fast**

* **Every line's vector** points at its stub in the COMMON block (as now).  The stub saves `.A`, loads the line number and jumps to the entry, which saves `.X` and `W`, sets `W` = 0 and enters the dispatcher on page 0.
* **The dispatcher** reads the line's owner from the IRQ owner table (a quick look into task 0), saves the interrupted task's stack pointer in the interrupted task's OS zero page, switches `T` to the owner (which also selects the owner's paged ROM bank, so its module is at `$A000`), loads the owner's stack pointer, and calls the owner's IRQ entry through its OS area's vector (copied there from the module header when the driver started).  The handler returns C = 1 if it serviced the interrupt, and may ask for a task switch.
* **Target: about 100 cycles from the interrupt to the handler's first instruction**, against about 650 today.  At 115200 the ACIA's handler then has room to spare, so **there are no fast handlers and no second path** (H1).  This is spike S1 (§20, phase 1): measured in the emulator before anything depends on it.
* **The tick** (VIA timer 1) is owned by the kernel task: its handler counts ticks and the clock, wakes sleepers whose time has come (the earliest first), and asks for a task switch.
* **Unowned lines** are left pointing at a stub that counts them; a driver must own its device's line before it enables the device's interrupt.  (A stuck line is a hardware fault for the hardware test.)
* **BRK** (no line active: the entry `V0-V3` selects) goes to the kernel as a fault: the task gets a `sys: brk` note (a debugger can catch it; otherwise the task ends with that status).  Software interrupts as a call mechanism are dropped: the jump table is faster.
* **NMI** goes to a kernel handler that sends a note to whichever task registered for it (a slot card's button, a debugger).

#### **10.4 Calls between tasks, and the one copy**

**SCALL** is today's `TASK_CALL`, made lean and given one job: running a server's entry in the server's task.
* The client is marked calling; `T` switches to the server; the server runs on its own stack (below its saved frame), preemptibly; a server serves one call at a time and later callers wait in its wait mask (as today).
* The request travels in registers and a 32-byte request block that the stub writes into the server's inbox (its OS area, `$0200`) with a quick copy ([Appendix B](#appendix-b-the-request-block)).
* Target: under 200 cycles for a round trip with no data (spike S3).

**KCALL** is SCALL into task 0 for the kernel's own structural work.  A KCALL never blocks (it returns `E_AGAIN` and the stub waits), so a slow device can never hold the kernel.

**kcopy** moves bytes directly between two tasks' memory:
* With IRQs off for a burst of at most 8 bytes, it reads a byte with `T` = the source task (so the source's own `$00`, `$01` and `U` apply: a buffer in its RAM bank or its ROM bank works) and writes it with `T` = the destination task, keeping its pointers in each task's OS zero page and each task's partner number there too, so the loop needs no stack and no shared memory.
* Between bursts, IRQs are on for a moment, with `T` back on the running task.
* Target: about 35 cycles a byte (spike S2), one copy where today there are two or three (F5), with every burst well inside the 200-cycle IRQ budget.
* Servers use it through the server library: `CLIENT_READ` (from the client's buffer) and `CLIENT_WRITE` (to it), given the request.

#### **10.5 Memory**

The kernel manages **pages and banks**; fine-grained allocation belongs to the language runtimes (C's `malloc`, Forth's `ALLOT`, hylang's heap), which know their own needs (E1).
* **Task RAM:** `BRK` sets the end of the program's data area (as Unix's `brk`); `PAGES_ALLOC` and `PAGES_FREE` give runs of 256-byte pages above it for a runtime that wants them (a 16-byte bitmap in the OS area).  A program loaded at `$0800` starts with its break above its BSS.
* **Banks:** a task's banks are its own (§6).  `BANKS` returns how many it has (16 per installed module); `BANKS_ALLOC` and `BANKS_FREE` keep a 30-byte bitmap per task only so that libraries in one program don't collide.  No handles, no locking: a program selects a bank by writing `$00`, as on the X16.
* **Shared segments:** `SEG_CREATE` (a number of 8K shared banks) returns a segment number; `SEG_ATTACH` and `SEG_DETACH` count references; the banks go back when the last reference goes, or when the last task holding one ends.  A segment can be given a name in `#s` (§14.1), so tasks find it by file name rather than by passing numbers.  `SEG_MAP` returns the `U` and `$00` values to select a bank of it.
* **No far pointers, no references.**  The kernel reads callers' names and buffers with the caller's own mappings (§10.4), and there are no applications on other BIOS pages (E2).

#### **10.6 Starting and ending tasks**

One call starts a task: **`SPAWN`**.
* **In:** `r0` = the program's path, `r1` = an argument list (a block of zero-terminated strings ending in an empty one), `.A` = flags (as Plan 9's `rfork`: copy the namespace or start a clean one; copy the environment or start empty; join the note group or start one), `r2` = an fd map (which of the parent's fds become the child's 0, 1, 2 ...; others aren't inherited unless marked).
* **Out:** `.A` = the new task.  The child starts at its program's entry with its arguments and name in its OS area (`argc`/`argv` ready for C).
* The **loader** (kernel page 3) opens the file, reads its header ([Appendix C](#appendix-c-the-executable-and-module-header-hyx2)): a RAM program is read into the new task at its load address and its BSS cleared; a module that runs in place has the task's `$01` set to its bank and its data segment copied into RAM.  Either way the same header, the same call.  (As built, phase 4.1, `reborn/kernel/load.s`: a RAM program's task reads its own image, through the file SPAWN opened, given to it as fd 15, so SPAWN doesn't wait for it; the map is used only with the flag `SPAWN_FDMAP`, so a caller's stray `r2` can't hand a child fds.  Phase 4.2: the arguments are the list, 176 bytes at most; the child gets the caller's current directory and a copy of its environment, `SPAWN_NOENV` an empty one; and a module's data and BSS are set up by its own task as it starts, not by the kernel task from outside a byte at a time, which took 257,000 cycles for rc's 4.3K of BSS.)
* **`EXITS`** (`.A` = code, `r0` = message or 0) ends the calling task, as Plan 9's `exits`; returning from the entry point is `EXITS` with 0.  Everything the task had (its channels, pages, banks, segments' references, IRQ ownership, semaphores) is released by the kernel, in one place.
* **`WAIT`** (`.A` = a child, or `$FF` for any child) waits for it to end: `.A` = its task, `.X` = its code, its message into `r0`'s buffer.  A child that ended before is waited for at once (its record is kept until its parent waits, or the parent ends).
* **No `fork`/`clone`.**  The shell runs pipeline stages as programs (rc's way), and a script by spawning rc on it.  (`TASK_CLONE` copied up to 30K a stage.)  If a language later needs a copy of itself, it can spawn its own module with its state in a file or a shared segment.

#### **10.7 Notes (signals)**

As Plan 9's notes, with numbers instead of strings (a message can go with them):
* **Notes:** interrupt (Ctrl-C), kill (not catchable), hangup, alarm, `sys: brk`, `sys: stack`, and user notes 16-31.
* **`NOTIFY`** (`r0` = handler, or 0) sets the task's handler; **`NOTE`** (`.A` = a task or a note group, `.X` = the note) sends one; when the task next runs, the kernel builds a frame on its stack and calls the handler with the note in `.A`; the handler returns C = 0 to continue where the task was, or calls `EXITS`.  Without a handler, the default is to end with an exit status (130 for interrupt, 137 for kill, as now).
* A note also ends a blocked call with `E_INTR` (the server is told with a FLUSH request, so it drops the client from its wait mask).
* This replaces `TASK_SET_BREAK`, `TASK_SIGNAL` and their stack-pointer caveat (H4).

#### **10.8 Scheduling**

* Round robin among ready tasks on the 200 Hz tick (as now), with two refinements kept from today: a task woken by the tick or by a driver for a deadline (a song player) is run next, and `PREEMPT_OFF`/`PREEMPT_ON` hold the CPU without masking interrupts.
* Task 0 runs only when nothing else can, and idles with `wai`.
* Each task's CPU time is counted in ticks for `/proc` (so `top` is possible).

#### **10.9 Time**

* The tick count (`TICKS`), sleeping (`SLEEP` ticks, `SLEEP_UNTIL` a tick count), the clock (seconds since 2000-01-01, as now), and the DS1747 if it's there: found at boot, read and written by the kernel time device (§14.1).  The calendar text (`YYYY-MM-DD hh:mm:ss`) is in one routine, used by `/dev/time` and by `ls`'s library.  (As built, phase 5.4: the kernel keeps the clock as the boot's time and the ticks since (`TIME`, `TIME_SET`) and reaches the DS1747's registers for kdev (`RTC`); the calendar is kdev's, which sets the clock from the chip as it starts, so the boot doesn't wait for the chip's second to turn.  `ls -l`'s dates are toollib's own (`tl_date`, to the minute): a module can't call another's code, so the calendar is in two places.)

---

### **11. Modules: one executable format**

#### **11.1 What a module is**

A **module** is any executable: a driver, a program, a library.  It's one file format, **HYX2** ([Appendix C](#appendix-c-the-executable-and-module-header-hyx2)), with a 48-byte header that says what it is, where it loads, its entry points (main or init, serve, irq, stop), its data and BSS, how much RAM it needs, and its name and version.
* **In the paged ROM**, a module starts at a bank's `$A000` and **runs in place**: the task that runs it selects its bank in `$01`, its initialised data is copied to RAM by the loader, and its code never moves.  A 16K bank is enough for most modules; a larger one spans banks and calls between them with `XCALL`.
* **In a file** (on a card, in `/pc`, on a RAM disk), a module is loaded into RAM at its load address, usually `$0800`.  A driver under development can be loaded this way too, in a task of its own, and serve requests exactly as a ROM driver does.

#### **11.2 The module directory and `#m`**

The paged ROM's bank 0 has the module directory, which the kernel reads at boot.  The kernel's module device, `#m`, shows it as files: `#m/rc`, `#m/cons`, `#m/hylang` ..., each readable as its header and image, and `#m/bin` is a directory of the program modules.  So `bind -a '#m/bin' /bin` puts the ROM's programs in `/bin` like any other directory, and `ls -l '#m'` lists every module with its type and version.  Running `/bin/rc` finds `#m/bin/rc`, whose header says "in place, bank 7", and the loader selects bank 7 instead of copying anything.  (As built: the header says "in place", and the kernel finds the module's bank in the directory by the header's name; `#m` is served by `kdev`, a driver, not task 0.)

#### **11.3 Drivers**

A driver module has `init`, `serve`, `irq` and `stop` entries and a device letter.  Starting one (at boot for those flagged as boot drivers: the console and storage; later by `init` for the rest, from `/rom/lib/drivers`):
1. The kernel takes the highest free task, marks it a driver, selects the module's bank, copies its data, and calls `init` in the task (SCALL).
2. `init` sets up the hardware, claims its IRQ lines (`IRQ_OWN`), and registers its device letter (`SRV_REGISTER`).
3. From then on the task runs only for requests (`serve`) and interrupts (`irq`).  If `init` fails, everything it claimed is released and the task is freed; the boot prints `driver cons: error`.

`stop` is called when a driver is stopped (a ctl write to `#m/ctl`, for development), so drivers can be reloaded without a reset.

#### **11.4 Libraries**

A library module is code other modules call: the server library (§12.3), a big-number library for hylang, a graphics library for the Vera X.  A module calls a routine in another bank with **`XCALL`** (`.A` = bank, `r15` = address; registers pass through), which the kernel does from BIOS page 0, switching the caller's `$01` and back, as the X16's `jsrfar` does.

---

### **12. Files, servers and the protocol**

#### **12.1 Channels and fds**

* An open file is a **channel** in the kernel task: its server, the server's fid, its mode, its offset, its union state (for a union directory), and the set of tasks holding it (a 16-bit mask, so a stale fd number in a wrong task is caught).
* A task's **fd table** is 16 bytes in its OS area: a channel number per fd (16 fds, up from 12).  `DUP` and `DUP2` and inheritance share the channel, so the offset is shared as on Unix and Plan 9.
* The **offset** is the kernel's: `SEEK` (`.A` = fd, `r0:r1` = offset, `.X` = whence) returns the new offset.

#### **12.2 The protocol: requests**

Requests are a small subset of 9P, the same for every server ([Appendix B](#appendix-b-the-request-block) has the block):

| Request | Does |
| :------ | :--- |
| `OPEN` | Walk the name (the rest of the path, after the namespace) and open it with a mode; the server returns a fid |
| `CREATE` | Make a file or directory and open it |
| `READ`, `WRITE` | At an offset, a count; the data moves by kcopy with the client's buffer |
| `CLUNK` | The channel's last reference is gone |
| `STAT`, `WSTAT` | A 64-byte stat record, read or written (rename, mode) |
| `REMOVE` | Remove a file (the kernel sends it on the channel it opens for it, then clunks) |
| `FLUSH` | Forget a waiting client (a note interrupted it) |
| `DUP` | One more channel reference to a fid (servers that count them) |

**Every server answers every request**: what it doesn't support is `E_NOSYS` (from the server library's defaults), so a filesystem request to a device can never "succeed" by accident.

**Blocking, the same everywhere:** a server never waits inside a request.  If it has no data yet, it adds the client to a wait mask and returns `E_AGAIN`; the stub (in the client) sleeps until woken and sends the request again, or, for a non-blocking fd, returns `E_AGAIN`.  This is today's mechanism (it works), made the only one.

#### **12.3 The server library**

Every server is built on **srvlib** (a library module), so every server is structured the same way and a new one is mostly tables:
* **A file tree as data:** a table of names, types and handlers (`/dev/gpio` is a directory of `0-7`, `port`, `ctl`, `ca1`); srvlib walks names against it, makes fids, answers `STAT` and directory reads from it.
* **Text files:** a generator routine writes the text; srvlib slices it at the read's offset (so `cat` works and a server keeps no state between reads).  Today each server does this by hand.
* **ctl files:** a table of command words and handlers; srvlib splits the written text into words and numbers (decimal, `$` hex) and calls the handler; a bad command is `E_INVAL` and changes nothing.
* **Fids:** allocation, reference counting (`DUP`, `CLUNK`), per-fid data.
* **Client data:** `CLIENT_READ`, `CLIENT_WRITE` (kcopy), the client's task and its note group.
* **Wait masks:** `WAIT_ADD` (the client), `WAKE_ALL` (a mask).

A minimal server (`/dev/null`) is a tree of one file with two handlers.  `/proc`, `/env`, `/dev/gpio` and the console's ctl file are a few tables and generators each.

#### **12.4 Data movement**

Data moves once, by kcopy, between the client's buffer (wherever it is in the client's view) and the server's memory (§10.4).  There are no transfer areas and no IO unit: a server moves as much as it can per request (a disk server a block at a time, 512 bytes), and the stub loops until the count is done, the end of the file, or a short read.  A 16K program load from a RAM disk becomes 32 requests of one copy each, against 64 requests of three copies today.

#### **12.5 Control: ctl files only**

Every device's control is text written to a ctl file, and its state is text read from it (P7).  There is no binary ctl call; `IO_CTL`'s codes become words: `/dev/serctl` takes `b19200 l8 pn s1`, `/dev/consctl` takes `rawon`, `rawoff`, `group`; `/dev/wctl` takes `new`, `current 2`; `/dev/sndctl` takes `claim 0x03`, `release 0x03`, `volume 90`, `reset`; the disks' ctl files take `init`, `format`, `label`, `check`, `start`, `stop` as today.  Anything that can write text can control anything: rc's `echo`, Forth, hylang, C's `fprintf`.

#### **12.6 Directories and stat**

* **A directory reads as stat records** (64 bytes each), always, as in Plan 9 (F2).  `ls` and the libraries format them; a server implements one thing.
* **The stat record** (64 bytes): the name (32), the qid (type, version, path), the mode bits, the length, the modification time, the device letter and instance (`f` and `0`, or `f` and `x`: names, never table indexes: F7), and room to grow.  HydraFS's on-disk entries are unchanged; the server builds the record from them.

#### **12.7 Standard input and output**

The kernel has no stdio buffering (F6).  The libraries buffer (C's stdio, the asm SDK's `putc` routines, the languages' own).  For assembly programs, `PUTC`, `PUTS` and `GETC` are kept as convenience calls (a one-byte write or read on fd 1 or 0), the equivalent of the X16's `CHROUT` and `GETIN`.

---

### **13. Namespaces**

Plan 9's model, made complete and uniform (G1, G2):
* **Device names.**  Every server has a `#` name: `#c` the console, `#f` HydraFS, `#p` proc, and so on (§14).  A path starting with `#x` goes to that server directly, bypassing the namespace, as in Plan 9.  Nothing else is built in: there's no `/dev` prefix in the kernel.
* **The root.**  `#/` serves `/`: a directory of empty mount points (`bin dev env lib mnt pc proc ram rom sd sram tmp`).  The namespace file mounts and binds things onto them.  So `ls /` and `cd /dev` need no special case.
* **A task's namespace** is its mount table in the kernel task: up to 32 entries of {path, kind (mount, bind, hide), flags (`-b`, `-a`, `-c`), a device and spec, or a target path}.  Paths and targets are strings of up to 63 bytes in a string pool (not 13 and 15).
* **Resolution:** the longest prefix that matches whole path elements; a bind rewrites and looks again (at most 8 times); a union's members are tried in order for a lookup, the `-c` member for a create.
* **Union directories:** a channel on a union directory keeps which member it's reading, so `ls /bin` reads every member, on any number of fds at once.
* **Inheritance:** `SPAWN` copies the namespace (or starts the child with a clean one built from the namespace file: Plan 9's `newns`).  `bind`, `mount` and `unmount` change the calling task's, so in rc they're built-in commands.
* **No search paths.**  rc runs commands from `.` then `/bin`; libraries come from `/lib/<language>`; `$PATH` and `$LIBPATH` are gone.
* **One name per thing.**  `/proc` only (no `/dev/proc`); the disks in memory only at their mount points and as `/dev/sd/x`, `/dev/sd/r`, `/dev/sd/s`; the SD cards at `/sd/0` ... `/sd/f` by SPI device digit.

**Each shell's `/ram`** stays: `init` (and each new shell) makes the shell's own directory on the RAM disk and binds it at `/ram` in its namespace, and the programs it runs inherit that.  The directory belongs to the shell's note group, and the RAM disk removes it when the group ends.  The ownership check moves from name tests inside HydraFS (J2) into srvlib's generic owner attribute.  (Whether the check is wanted at all on a single-user machine is an [open question](#22-risks-and-open-questions).)

The default namespace file is in [Appendix E](#appendix-e-the-default-namespace).

---

### **14. Devices and drivers**

#### **14.1 The kernel's own devices** (served by task 0)

| Device | Files | Notes |
| :----- | :---- | :---- |
| `#/` root | `/` and its mount points | |
| `#e` env | `/env/NAME` | Each task's environment (copied or empty at spawn); 1K a task |
| `#p` proc | `/proc/N/status`, `ctl`, `note`, `ns`, `fd`, `mem`, `ram`, `regs`, `args`, `cwd`, `env` | Plan 9's set: `mem` and `ram` as today; `regs` (the saved frame), `fd` (open files) and `note` (send one) are new.  `ctl` takes `kill`, `stop`, `start` |
| `#\|` pipe | `/dev/pipe` (made by `PIPE`) | Pipes of 512 bytes in task 0's banks; 16 of them |
| `#t` time | `/dev/time`, `/dev/ticks` | The clock as text (set by writing), the DS1747 |
| `#n` null and zero | `/dev/null`, `/dev/zero` | |
| `#m` modules | `/dev/mod` (bound also as `#m/bin` into `/bin`) | §11.2 |
| `#s` segments | `/dev/seg/NAME` | Named shared segments (§10.5) |
| `#g` GPIO | `/dev/gpio/0-7`, `port`, `ctl`, `ca1` | As today; CA1's interrupt is the kernel's (the VIA is) |
| `#r` raw RAM | `/dev/ram` | Task 0's debugging view, as today, readable only by `init` and the kernel |

(As built, phase 5.7: `#p` is kdev's, not task 0's (2.6); `mem` and `ram` through a new kernel call, `TASKMEM`, for drivers only, a byte at a time with T switched; `regs` and `env` through `TASKREAD`'s `TR_FRAME` and `TR_ENV`; `note` takes a note's name or number.  Who may: any task's but the kernel task's and a driver's, as `NOTE` has it.  `fd` is still to come: a channel keeps no name.)

#### **14.2 The console: `cons`** (a driver module; task F)

* The ACIA (Rockwell, or WDC with timer 2 pacing, as build options), receive and transmit rings, sending paced at 115200 (the current, board-proven logic).
* **`#c`**: `/dev/cons`, `/dev/consctl` (each window's: below), `/dev/wctl`, `/dev/wnew`, `/dev/ser`, `/dev/serctl`.  (As built, phase 5.6: while `/dev/ser` is open for reading, the line is its reader's, for `xmodem`: every byte in is its, the keys the console acts on as they come in too, and the windows' text waits till its last close repaints the window shown.  At 115200 the receive ring, 255 bytes, holds a 128-byte XMODEM block but not a 1K one.)
* **Line discipline, cooked mode, in one place:** the console edits a line (Backspace, Delete, Left, Right, Home, End, Ctrl-U, the history with Up and Down) and delivers it on Enter, so every program gets line editing: rc, Forth, hylang, C's `fgets`.  (Today it's HyForth's alone, and a C program sees raw backspaces.)  Raw mode (`rawon`) gives each key as it comes, with the terminal's cursor and function keys decoded to single codes.
* **Windows, Plan 9's way, not job control.**  There's no foreground group, no `fg`, no stop key.  As rio gives each window a console of its own, `cons` serves several windows on the one terminal, each a whole console: its own `cons` and `consctl`, line editor, raw mode, note group, and its text (the last of its output, a screenful and more).  A window's files are `#c` with its number as the spec (`#c2/cons`, or `mount '#c' /dev 2`), so a shell's namespace gives it its window at `/dev`; plain `#c` is window 0, init's.  One window is shown and gets the keys; Ctrl-] then a digit shows another (Ctrl-] `n` the next), and `cons` repaints the terminal from that window's text.  A window that isn't shown runs on: its output goes into its text, its reads wait for keys.  Ctrl-C sends interrupt, and Ctrl-\\ kill, to the shown window's note group (the group of the program that claimed it: `group` in its `consctl`).  Windows are made by writing `new` to `/dev/wctl`, or by the user: Ctrl-] `c` answers a read of `/dev/wnew` (init's, which starts a shell in the window); a window goes when the last of its `cons` is closed.
* **The bell:** a BEL sent to the console asks the sound driver for its beep (a call from `cons` to `snd`, the only driver-to-driver call, documented as such).
* **`/pc`** (`#P`): the PC folder over the serial line, served in this task, which owns the line.  The framing, CRC, resends and the PC tool's file server stay; the request header changes to Appendix B's, so the protocol's version goes to 2 and the PC tool learns both.  (As built, phase 5.5: the irq entry stays the keys' alone, so the PC's frames come in with the keys and are taken out of the receive ring in the serve entry, and the PC stuffs the keys the irq entry acts on (Ctrl-C, Ctrl-\, Ctrl-]); a frame carries 128 bytes of data, so a reply fits the ring; a reply's wait is timed by timer 2 run on, as the kernel has no timed wait for a server's client.  `docs/plans/PC.md` has version 2.)
* **Later (phase 8):** the console gets a second back end, the Vera X screen with its keyboard, chosen in `consctl` (`screen`, `serial`, `both`), as [VIDEO.md](plans/VIDEO.md) plans: the windows are the same, shown on either.

#### **14.3 Storage: `storage`** (a driver module; task E)

One driver owns the SPI bus and every disk (as built, a module of two banks: HydraFS is its second):
* **SPI** (the current bit-banged loops, 18 cycles a bit in, unchanged) and **`#S`**: `/dev/spi/N/data` and `/dev/spi/N/ctl` (`N` = 0-f: a directory a device, as `#d` has; today's `/dev/spi/N` is the data file, with its ctl under it), arbitrated with the SD cards (a device in use as one isn't the other).
* **The block layer:** SD cards (SDSC and SDHC; the current command layer), the ROM disk `x` (read through the kernel's `ROMREAD`: the driver's own `$01` holds its code, so a kernel routine on page 0 selects the ROM disk's bank, copies the block and puts the driver's bank back), and the RAM disks `r` and `s` (started, sized and stopped through their ctl files, as today).  Two 512-byte block buffers and the metadata buffer.
* **`#d`**: `/dev/sd/N/data` and `/dev/sd/N/ctl` (`N` = one hex digit for SPI devices, `x r s` for the others, as now).
* **`#f`**: HydraFS, mounted with a spec (`mount '#f' /rom x`), the on-disk format unchanged (v1 and v2 read, v2 quick format, partitions, sparse files, the check).  The code is ported from `fs/hfs_*.s` into the module's structure: its state in the storage task's RAM with one layout file, its zero page in `$22-$7F` of its own task (no borrowing), and srvlib for the requests.
* **Open files:** 32 (not 8), each with its copy of the directory entry, as now.

#### **14.4 Sound: `snd`** (a driver module; task C)

* The YM2151 library, ported: the register shadow, the General MIDI volume curve, the X16's patch set (2-clause BSD, its notice kept), notes, bends, drums, claims.
* **`#a`**: `/dev/snd` (register/value pairs and the library's commands, as today: a song's raw stream and a program's notes go the same way), `/dev/sndctl` (`claim`, `release`, `volume`, `reset`, `clock`), reads giving the shadow.
* The **song player** is a program (`play`), not part of the driver: a client of `/dev/snd` like any other, with today's timing (the system tick, a fraction, read ahead without waiting).
* Later: the VERA's PSG and PCM as more channels (phase 8).

(As built, phase 5.1, `reborn/modules/snd`: the library and `#a` as above, with `sndctl`'s `claim`, `release`, `volume` and `reset`, and `#a/bell` for the console's bell.  Claims are a task's (the task's that opened the file), given back as its last file of `#a` closes.  There's no `clock`: the old player had stopped timing songs by timer B (on the board it didn't keep its period), so the driver owns no interrupt and keeps the chip's timers quiet.  Phase 5.2, `reborn/modules/play`: the player as above, `play [-l] song [n]`; `scom` is an rc script on the ROM disk, since rc runs a `#!` file by its interpreter.)

#### **14.5 The rest**

| Device | Where | When |
| :----- | :---- | :--- |
| `#i` I2C: `/dev/i2c/NN` per address, `ctl` (speed) | A small driver module (bit-banged on port A), or in the kernel beside GPIO | Phase 5 |

(As built, phase 5.3, `reborn/modules/gpio`: `#g` and `#i` are one driver module, port A's alone, so their changes to it never meet; `#i`'s `ctl` has `subaddress` too (a device's register written first, from the offset).  CA1's interrupt is the kernel's as far as its stub: a line of its own, `LINE_VIA_CA1`, as timer 2's is, owned by the driver while `/dev/gpio/ca1` is open.)
| `#v` video: `/dev/vid/ctl`, `vram`, `pal`, `sprites`, `frame` | The Vera X driver module ([VIDEO.md](plans/VIDEO.md), steps 2-5, unchanged in substance) | Phase 8 |
| `#k` input: keyboard into `#c`, `/dev/mouse`, `/dev/pads` | The input controller's driver (on I2C, IRQ line 3) | Phase 8 |
| `#N` network (`/net` on a W5500) | A driver module | Later |

---

### **15. Userland: init, rc, tools, SDKs**

#### **15.1 Boot, start to prompt**

1. **Reset** (on any page): `W` = 0, task 0, the stack.  **POST** (polled serial, interrupts off, as today; `T` typed jumps to the hardware test).
2. **The kernel's set-up:** the hardware probe (RAM modules, shared RAM chips, the DS1747), the kernel task's tables, the IRQ vectors, the module directory.
3. **The boot drivers:** `cons` (task F), then `storage` (task E), then `kdev` (task D: the kernel's own devices, a driver of their own as built).  A failure prints a line and the boot goes on.
4. **`init`** (task 1), a module: it mounts and binds from `/rom/lib/namespace` (and a card's `/lib/namespace`), starts the drivers listed in `/rom/lib/drivers` (`snd`, later `vid`, `input`), runs `/rom/lib/profile` (and a card's), and starts the console's shell in window 0 on fds 0-2 = `/dev/cons`; for each window the user asks for (a read of `/dev/wnew`) it starts another shell in that window, with that window at `/dev`.  When window 0's shell exits or is killed, `init` starts another (the kernel no longer special-cases the shell: A1).  `init` also adopts orphans and reaps their records.  (As built, phase 3.6: each shell runs the namespace file too, starting from an empty namespace, as Plan 9's `newns`, `sdk/asm/nslib.s`; binds are resolved as they're made, so `$task`, a shell's `/ram` and its caches are its own only that way.  Phase 4.2: the shells are `rc -l`, which runs `newns` and then `/rom/lib/profile` itself; the profile puts the shell's window at `/dev` by `$window`.  A small program, `wstart`, waits for the window the user asks for, starts `rc -l` there, and ends; init starts it again.)
5. **The tick starts; task 0 idles.**

#### **15.2 rc**

A small rc, after Plan 9's, in assembly (it's spawned for every `system()` and for every command hylang or HyForth hands it, so it must start fast: it runs in place from its module):
* **Commands:** words, quoting with `'...'`, `#` comments, `;`, newlines, `{ }` groups, `&` (background; `$apid`), `|` pipes (and `|[2]`), redirection `<`, `>`, `>>`, `>[2]`, `>[2=1]`, `` `{cmd} `` substitution, `&&`, `||`, `!`.
* **Variables:** lists, `$x`, `$#x`, `$x(n)`, `$"x`; every variable is `/env/x`; `$status` (the last exit status: its message, or its code), `$apid`, `$task`, `$path` (fixed at `(. /bin)` by default, changeable).
* **Control:** `if(...)`, `if not`, `for(x in ...)`, `while(...)`, `switch`/`case`, `fn name {...}`, `~ subject pattern`, globbing `* ? [ ]`.
* **Built-ins** (because they change rc's own task): `cd`, `bind`, `mount`, `unmount`, `newns`, `exit`, `wait`, `eval`, `.` (source), `builtin`, `fn`, `whatis`.
* **Interactive** with the console's line editing; a prompt in `$prompt`.  `rc -c 'line'` runs one line and exits with its status: `system()` in C, `sh` in HyForth and hylang.

(As built, phase 4.2, `reborn/modules/rc`: all of the above, in two banks of the paged ROM.  rc keeps its variables in a heap of its own and writes the changed ones to its environment before it starts a program, as Plan 9's does at `exec`, so `/env/x` is what a child sees; a pipeline stage, a background command or a `` `{...} `` that isn't a plain program runs as `rc -c` on its text, as there's no `fork`; `rc -l` runs `newns` and the profile.  `rc -c` from `SPAWN` to its end takes 124,000 cycles, 35 ms.)

The other shells sit on top of it (§16, §17).

#### **15.3 The tools**

Small programs, each a module in the paged ROM (in place) or a C program in `/rom/bin`:

| Group | Tools |
| :---- | :---- |
| Files | `ls` (`-l`), `cat`, `cp` (`-r`), `mv`, `rm` (`-r`), `mkdir`, `rmdir`, `touch`, `du`, `df` |
| Text | `echo`, `wc`, `head`, `tail`, `grep`, `sort`, `uniq`, `tee`, `more`, `xd` (a hex dump), `cmp` |
| System | `ps`, `kill`, `slay`, `top`, `date`, `sleep`, `ns`, `mods` (the module directory), `free` |
| Disks | `mkfs`, `fsck`, `label` (each a few ctl writes; the work is the storage driver's) |
| Others | `edit` (the line editor, ported; later a screen editor), `play` (songs), `xmodem`, `hwtest` (resets into the hardware test) |

Whether each is assembly or C is chosen by size and speed: C first where it saves time (`sort`, `grep`, `fsck`'s report), assembly for what runs often (`ls`, `cat`, `echo`).

(As built, phase 4.3: the file, text and system tools are modules in assembly, on a library they share (`reborn/sdk/asm/toollib.s`: flags, errors and exit statuses as Plan 9's, buffered output, directories read whole, a tree walked); `mkfs`, `fsck` and `label` are RAM programs on the ROM disk's `/rom/bin`, bound at `/bin` after the caches.  `grep` and `sort` came in C with 4.5, `date` waits for the clock (phase 5), and `hwtest` for a way to reset into it.  `/proc`'s args, cwd and ns came with them (the calls `TASKREAD` and `NSINFO`; ns reads as the binds and mounts that make the namespace, as Plan 9's does), and the module directory now holds 127 modules.)

(As built, phase 4.6: `edit` is a module, as the tools are (its text has the task's RAM from its break up, some 29K), with the old editor's commands; its lines come from the console's cooked mode, edited there, or from a file, and its files keep LF line ends.  Ctrl-C comes back to its prompt through a note handler.)

#### **15.4 Programs in C**

The cc65 target becomes a proper one (`-t hydra`):
* `hydra.cfg`: the header, code and data from `$0800`, **zero page `$22-$7F`** (cc65's runtime needs 26 bytes; the rest is the program's), the C stack at the top of task RAM.
* `crt0.s`: the header, data and BSS set-up, `argc`/`argv` from the OS area, `exit` through `EXITS` with `atexit` functions run.
* **The library:** stdio over fds with buffering in the library; `open`, `read`, `write`, `lseek` (through `SEEK`), `stat` and `fstat` (64-byte records), `dirent.h` (stat records, no text parsing), the environment as `/env` files, `errno` mapped from the error codes 1:1 ([Appendix D](#appendix-d-error-codes) uses POSIX names), `strerror` from the kernel's `ERRSTR`, `time` and `clock`, `system` (`rc -c`), conio over raw mode and ANSI, `signal` over notes, the bank calls for big data.
* `hydra.h` and the binding glue **generated** from the API spec.

(As built, phase 4.5, `reborn/sdk/c`: cc65's own target `none`, with the Hydra's `crt0.s`, `hydra.cfg` and a library over cc65's `none.lib` (a `-t hydra` of its own would be a target in cc65's sources).  The library is the plan's but in four places: the environment is rc's variables through the `ENV_*` calls, as the kernel keeps it, not `/env`'s files; `errno` is cc65's (its 18 values), `spec/errors.def` giving each error code one, and `_oserror` keeps the code itself; `time` counts from the program's start (from 2000-01-01) till the clock (5.4); and the calls' C functions are written by hand on one `hy_call`, with `hydracalls.h` generated (every call's slot, every error code and constant).  stdio's buffering is the library's own (cc65's reads a byte a call): a buffer for each fd, a console's line buffered.  The samples `tones` and `jukebox` wait for the sound driver (5.1); `grep` and `sort` are C programs on the ROM disk.)

#### **15.5 Programs in assembly**

* `hydra.inc` (generated): every call's address, the constants, the error codes, and macros: `CALL name`, `HYX2_HEADER`, `CHECK` (branch on error), `PRINT "text"`.
* `hyx2.cfg`, a `crt0` for assembly (data copy, BSS, entry), and samples: hello, a filter, a server, a module that runs in place.

---

### **16. HyForth, rebuilt**

A new Forth, written from scratch, added in phase 6 when files, the console's modes, memory, spawn and notes all exist.

* **Standard:** Forth 2012: the Core and Core Extension word sets, Exception (`CATCH`/`THROW`, with the OS's errors as throw codes), File Access (on the OS's files), Facility (`KEY?`, `MS`, `TIME&DATE`, `AT-XY`), String, Search-Order (wordlists instead of today's `LIBSET` bits: I4), Programming-Tools (`.S`, `SEE`, `WORDS`, `DUMP`), and later Double and Memory-Allocation.  Decimal by default; `: x 65 . ;` compiles as anyone expects (I1).
* **Implementation:** subroutine threaded (each word a `jsr`, the fastest model on the 65C02), the kernel's code running in place from its paged ROM module, the dictionary and data space in task RAM from `$0800` (with `BRK` moving the end), the data stack in the program zero page split into low and high bytes indexed by `.X` (the classic 6502 layout), the return stack on the 6502 stack.
* **Hydra words,** generated from the API specification (`open-file` and the rest are standard; the others are `sys-NAME` words with the call's registers as stack items), plus `sh ( c-addr u -- status )` (`rc -c`), `run`, and words for banks and segments.
* **Libraries** are source files: `REQUIRE graphics.fs` finds `/lib/forth/graphics.fs` through the namespace; a library defines its words in its own wordlist.
* **Interactive** through the console's cooked mode (line editing for free); `forth` at the rc prompt starts it, and `forth file.fs` runs a script.
* **Tested** with the Forth 2012 test suite (Gerry Jackson's), in the emulator, as a regression test.

(As built, phase 6.1: `modules/forth`, 7.9K of one bank.  Short words are copied into definitions whole (DUP, +, @ ..., and the return stack's, which can't be called); IF's test and literals are inline.  The ROM words' headers are beside their code, in the same chain as the RAM ones.  Input is stdin's lines, so `forth <file` runs a file before 6.4's `forth file.fs`; CATCH and THROW came with the core, as QUIT's error handling.  The suite's preliminary, Core, Core Plus and Core Extension tests pass, from an emulated card.)

(As built, phase 6.2: the Exception, File Access, Facility, String, Search-Order and Programming-Tools word sets, with the suite's tests of them all passing; 15.1K of the bank, so the Hydra words (6.3) need a second.  An ior is -512 less the system's error code, as in Gforth, and QUIT shows its text; a file being included is read a line at a time, READ-LINE's way, and SAVE-INPUT's place is a line's offset; CATCH and THROW unwind the source stack, closing the files they leave.  Word lists replace `LIBSET`'s bits (I4).  RESIZE-FILE needed HydraFS's WSTAT to take a length, as Plan 9's does.)

(As built, phase 6.3: 58 `sys-` words, from the calls' `in:` and `out:` lines (apigen reads their registers), a table in `forth`'s second bank from which their headers are made in RAM as it starts (one bank is at `$A000` at a time); their stack effects are in the reference, and the constants a program uses are `/lib/forth/hydra.fs` on the ROM disk.  `SH` (`rc -c`), `RUN` (a program and its arguments, no shell), `BANK!`, `BANK@`, `SEG-BANK!`, `>Z`.  Ctrl-C is a note forth handles: THROW -28 at the next word, loop or wait.  The first bank is full, so 6.4 moves words to the second.)

(As built, phase 6.4: `forth file.fs [argument ...]` (`ARGC`, `ARG`; code 1 after an error), and a first line `#!...` skipped, so `#!/bin/forth` scripts run by their names; in pipelines too.  A name INCLUDED that isn't in the current directory, with no `/`, is `/lib/forth`'s (the current directory first, as INCLUDED's standard meaning has it).  `LIBRARY name` and `END-LIBRARY` give a library its own word list, first in the search order: `REQUIRE hydra.fs`.  Some words are in the second bank now, their headers in RAM, run through `FAR2`.)

(As built, after 6.4 (6.5, at the user's asking): `forth`'s module is its core, the Core word set and the words that load files, 9.4K of one bank; the other word sets, the Hydra words among them, are libraries pre-compiled at the build (`reborn/forthlib`, `tools/forthlib.js`: relocatable images, `/lib/forth/NAME.fl`) that INCLUDED loads into the dictionary, so `REQUIRE tools.fl` as `REQUIRE graphics.fs`.  `/lib/forth/startup.fs` names those forth starts with (Core Extension, Exception, File Access, Programming-Tools); a card's or the RAM disk's takes the ROM's place.  A library calls only the core; a library file is for the core it was built with.)

---

### **17. hylang: danlang on the Hydra**

#### **17.1 What danlang is today**

danlang (`C:\source\danlang`, C# on .NET 6, by Daniel and Simon Struthers, GPLv3, about 3,800 lines of C# and 1,200 of its own library) is a lisp descended from *Build Your Own Lisp* ("lispy"), with these characteristics ([Appendix F](#appendix-f-danlang-to-hylang-the-inventory) has the full inventory):
* **Syntax:** S-expressions `( )`, Q-expressions `{ }` (quoted lists, run with `eval`), `;` comments, strings including multi-quote "here strings", case-insensitive symbols, atoms `:name`, characters `\name` (`\space`, `\lparen` ...), `T`, `NIL`.
* **Prefix shorthands** before a paren: `'(` list, `^(` head, `$(` tail, `.(` unpack, `|(` join, `=(` set, `:(` def, `@(` fn, `!(` eval, `?(` if, `#(` hash-create, `<(` hash-get, `>(` hash-put, `*(` hash-call, `~(` format (not implemented yet).
* **Evaluation:** built-ins receive their arguments unevaluated (so `if`, `and`, `def` are ordinary built-ins: fexpr style), lambdas (`fn {formals} {body}`) evaluate theirs; **calling a lambda with fewer arguments returns it partially applied** (currying); extra arguments are `&_` (a list) and `&1`, `&2` ...; `def` defines globally, `set` locally; errors are values.
* **Numbers:** arbitrary-precision integers, rationals (`720/84`), fixed-point decimals of any length (`3.14159_26535...`), complex numbers, and numbers in many bases, including balanced, negative and little-endian ones (`#x0ab123`, `#c0-+-0`, `#<-x...`), with `_` separators.
* **Data:** lists, strings, characters, atoms, hashes with tags (`:__locked`, `:__read-only`, `:__private`, `:__not_nil`) and methods called with `hash-call` and `&0`, a small object system.
* **IO:** `print`, `load` (`name.dl` from `lib/`), `save` (a value serialised to a file).  A stream type exists but has no built-ins yet.
* **The library** `lib/globals.dl` (the standard library: `fun`, `let`, `cons`, `map`, `filter`, `foldl`, `cond`, `case`, `do`, `sort`, math, string helpers, dice).  `lib/harn.dl` (a HarnMaster character generator) and `lib/cngh.dl` are example programs, not part of the library.
* Two interpreters are in the tree: the active one (`LVal.cs`, `Builtins.cs`) and an older one (`Interpreter.cs`, namespace `Dep`).  The port follows the active one.

#### **17.2 What changes for a 65C02**

| danlang (.NET) | hylang (65C02) | Why |
| :------------- | :------------- | :-- |
| Values copied on every lookup and assignment (`Copy()`) | Shared, immutable cells, with a garbage collector | Copying is unaffordable at 3.58 MHz; sharing immutable data gives the same semantics |
| `BigInteger` for every integer | **A numeric tower**: 15-bit fixnums in the value itself, 32-bit integers boxed, bignums from a ROM library; rationals and fixed decimals built on them; complex later | Most numbers are small; arbitrary precision stays available |
| .NET strings and dictionaries | Byte strings; small open-hashing tables for environments and hashes | |
| Case-insensitive symbol lookups | Symbols interned once, folded to lower case at read time | Comparison becomes one 16-bit compare |
| Recursion in C# | An explicit evaluation stack in a bank, and proper tail calls | The 6502 stack is 256 bytes; danlang's library recurses (`map`, `nth`) |
| `load "x"` from `lib/` | `load "x"` from the namespace's `/lib/hylang` | Plan 9 names (P8) |
| Console and files of .NET | The Hydra's fds; streams become real (`open`, `read-line`, `write`, `close` over fds) | |

#### **17.3 The runtime**

* **Values are 16 bits.**  Bit 0 set: a fixnum (15 bits signed).  Bit 0 clear: a reference to an object in the heap: bits 13-15 a heap bank (0-7), bits 1-12 a word offset in it.  Dereferencing selects the bank in `$00` and reads at `$8000 + offset`.  A 64K heap in 8 of the task's own banks, extensible to 16 by aligning objects to 4 bytes.  (This scheme is spike S5, to be confirmed by a prototype at the start of phase 7.)
* **Objects** have a type byte and a size: pairs (4 bytes), symbols, strings, bignums, hashes, closures (formals, body, environment), built-ins (a ROM address), errors, streams (an fd).  No object crosses a bank.
* **The collector:** mark and sweep, not moving (native code can hold references), with free lists per size; the mark stack in its own bank.
* **The evaluator** in assembly: special forms (danlang's unevaluated built-ins), closures with partial application, `&_` and `&N`, errors propagated as values, a step counter so Ctrl-C (a note) can stop a runaway loop.
* **The reader and printer** keep danlang's syntax exactly, prefix shorthands and number bases included (the exotic bases' parsing and printing live in a library module, loaded on first use).
* **Where it runs:** the interpreter in place from its paged ROM module (two banks, with `XCALL` for the cold parts), the heap in the task's banks, the evaluation stack in another bank, the program zero page for the registers of the evaluator.

(As built, phase 7.1: spike S5 confirmed the values and the heap, `reborn/modules/hylang/heap.inc`.  A page holds one kind (BIBOP): 4-byte pairs with no header, or cells of 8-128 bytes with a type, so a value's kind is a look-up of its high byte; the marks are a bit for each 4 bytes, in the task's RAM; there are no free lists: the pages with nothing marked are freed, and the rest swept lazily as each kind allocates from them, so a pause is the marking.  At 3.58 MHz a pair takes 141 cycles to make (278 from a swept page), and a collection 142 cycles a live pair: 0.36 s with 9000.)

#### **17.4 hylang as the shell**

hylang becomes the login shell in phase 7, over rc:
* **A line typed at the hylang prompt is hylang if it starts with `(`, `{` or a prefix shorthand, and an rc command line otherwise.**  So `ls -l | wc` works as in rc, and `(map (fn {f} {print f}) (ls "/bin"))` works as lisp.  (`$(` is danlang's tail shorthand, and `$` matters to rc: the rule keeps them apart, since an rc line never starts with `(`.  Whether `$` should become environment access inside hylang is an [open question](#22-risks-and-open-questions).)
* **Programs as functions:** `(run "ls" "-l")` runs a program and gives its exit status; `(sh "ls | wc")` runs an rc line; `(lines (sh-out "ls /bin"))` gives its output as a list of strings; `(env :home)` and `(setenv :home "/sd/0")` use `/env`.
* **Start-up:** `/lib/hylang/globals.hl` (danlang's `globals.dl`, ported) then the user's `/lib/hylang/profile.hl`.
* **Multi-line input** with danlang's open-paren prompt, on top of the console's line editing.
* **The file extension** `.hl` (danlang used `.dl`), a decision to confirm.

#### **17.5 Keeping the two implementations in step**

The C# danlang becomes **the reference**: a conformance suite of `.hl` files (from `tests/`, `globals.dl`'s functions, and new cases) runs on the PC under danlang and on the Hydra under hylang in the emulator, and the outputs are compared, with documented differences (number printing limits, unsupported parts).  The language itself is renamed hylang in both, if the C# version continues.  Its GPLv3 license needs a decision for the ROM ([§22](#22-risks-and-open-questions)).

---

### **18. Commander X16 programs: the migration utility**

As decided, X16 support is a **PC-side utility** and comes last; this section records what it would face, so that the decision whether to build it can be made then.

**How the machines differ:**

| | Commander X16 | Hydra-16 | Effect on porting |
| :- | :------------ | :------- | :---------------- |
| Low RAM | `$0000-$9EFF` (38K) | `$0000-$7FFF` (32K, per task) | Big programs need their data moved to banks |
| Zero page | `$02-$21` r0-r15, `$22-$7F` the program's | The same (§8.1) | Carries over unchanged |
| Program load | `$0801` (a BASIC stub) | `$0800` (a header) | Relink |
| Banked RAM | `$A000-$BFFF`, bank in `$00` | `$8000-$9FFF`, bank in `$00` | The register is the same; the window's address differs: source-level change, unsafe to patch in binaries |
| ROM banks | `$C000-$FFFF`, bank in `$01` | `$A000-$DFFF`, bank in `$01` | Programs that switch ROM banks to call the KERNAL won't port |
| I/O | `$9F00-$9FFF` (VERA `$9F20`, YM2151 `$9F40`) | `$FF00-$FFEF` (VERA `$FF20`, YM2151 `$FF40`) | The chips' registers are in the same order: a base change in source |
| KERNAL | A jump table at `$FF81-$FFF3` and extensions below it | The Hydra's I/O space is there | **No binary compatibility on V1.**  Calls are mapped by name, at source level |
| Files | CBM DOS: device 8, logical files, `SETNAM`/`SETLFS`/`OPEN`, `LOAD`/`SAVE`, PETSCII names | Plan 9 files | A shim library maps them |
| Screen | The screen editor, PETSCII, the VERA as the console | An ANSI console; the Vera X screen from phase 8 | Text programs need a PETSCII console mode or a shim |
| Interrupts | `$0314` vector, VSYNC from the VERA | Per-driver handlers; frames from `/dev/vid/frame` | Rewrite the IRQ hook as a frame wait |

**The utility** (`tools/x16port.js`), in three parts:
1. **An analyser** for a ca65/cc65 source tree or a PRG binary (disassembled from its entry): it lists KERNAL calls, I/O addresses, banked RAM use, ROM bank switching, zero page use and the IRQ hook, and grades the program (ports as is, needs edits, won't port).
2. **A source converter** for ca65 and cc65 projects: an include (`x16compat.inc`) and a cc65 header set (`cx16.h` with the VERA at `$FF20`, `RAM_BANK` at `$00`, `BANK_RAM` at `$8000`) for recompiling, and edits for the patterns it can rewrite safely.
3. **A shim library** (`libx16`) linked into ported programs: `CHROUT`, `CHRIN`, `GETIN`, `SETNAM`, `SETLFS`, `OPEN`, `CLOSE`, `CHKIN`, `CHKOUT`, `CLRCHN`, `LOAD`, `SAVE`, `RDTIM`, `PLOT`, `SCREEN`, `MEMTOP`, `MEMBOT`, the clock and joystick calls, on the Hydra's API, with PETSCII translated for the console.

Binary conversion is heuristic (computed addresses can't be found statically), so the utility patches binaries only with a report of what it couldn't check.  The VERA and YM2151 code, usually the bulk of a game, ports with a base-address change, which is what makes the utility worth building.

---

### **19. Tools, tests and documentation**

#### **19.1 The build**

* **Modules assembled separately** with `.import`/`.export` (no `all.s`, no scope ordering, no aliases: A5): each kernel page is a module linked into its page, each paged ROM module is linked on its own at `$A000`, each program at `$0800`.
* **Kernel-internal far calls** between kernel pages go through gates the build generates from the imports, checked by the page checker (its idea kept, applied to the kernel only).
* **The image builder** (`tools/romimg.js`, from `mkromdisk.js`): the paged ROM image from the module directory, the modules and the ROM disk's files, in the chips' order (the A13 halves, V1's bank-bit swaps), read back through the emulator's mapping and checked, as today; checksums for the hardware test.
* **The budget report** for every build: each BIOS page, COMMON, each module (against its banks), each module's zero page use (`$22-$7F`), the kernel task's tables.
* **Generated files** (the jump table, includes, headers, bindings, docs tables) go into `obj/`, not Git; the ROM images become release artefacts (K3).  (Keeping images in Git for people without a toolchain is the [user's call](#22-risks-and-open-questions).)
* **CI** on every push: build, tests, the variants (7.16 MHz, the WDC ACIA), as `build.js` does now.

#### **19.2 The specification and its generator**

`spec/api.def` and `spec/errors.def` (§9.5) and `tools/apigen.js`, which writes the jump table, `hydra.inc`, `hydra.h`, the C glue, the HyForth and hylang binding tables, `docs/reference/api.md`, and the emulator's call names.  A test fails the build when the ROM's jump table and the specification disagree.

#### **19.3 The emulator and the tests**

The emulator is kept and extended:
* **A debugger**: breakpoints by label (from the `.dbg` files of the kernel and every module), single steps, a task-aware view (each task's registers, frame and banks), watchpoints, and **a call trace** (`--trace-calls`: each jump table call with its arguments and result, by name).
* **Images as modules**: load the kernel image, the paged ROM image, or a single module into a task for testing.
* **Tests** keep today's harness (boot, type, expect), and add:
  * prompt-based waiting everywhere (no fixed cycle waits);
  * **kernel unit tests**: a test module calls kernel routines in the emulator and checks results (the scheduler, kcopy, the allocators, the namespace resolver);
  * **budgets as tests**: the IRQ path's cycles, the longest IRQ-off stretch (under 200 cycles), kcopy's cycles a byte, SCALL's round trip, a program's load time, `ls /bin`'s time;
  * **conformance suites**: Forth 2012's test suite for HyForth, danlang's for hylang (§17.5);
  * **cross-checks with the PC tools**: cards written by the Hydra checked by `hydrafs.js`, as today;
  * **fault injection** as today (RAM lines, stuck IRQs, missing chips).
* **The board**: a short hardware smoke test at the end of each phase (boot, the console at 115200, a card, sound), since the emulator isn't the board.

#### **19.4 Documentation**

Written as the system is built, phase by phase, in plain, short chapters (K2):
* **A system overview** (one page: the picture in §7);
* **The programmer's guide**: the ABI (§9), memory (§8), tasks and notes, files and namespaces, writing a server (with srvlib), writing a driver, modules; each chapter with complete examples;
* **The API reference**, generated from the specification, so it never drifts (K1);
* **User guides**: rc, the tools, HyForth, hylang;
* **The design notes** (`docs/plans/` style) for decisions and their reasons, including this document.

---

## **Part III: Step by step**

### **20. The phases**

Each phase ends with something that runs, a set of tests that pass in the emulator, and (from phase 1) a check on the board.  Sizes are rough: **S** a few days, **M** a few weeks, **L** longer.  The current OS stays buildable and remains the daily system until phase 5's parity checkpoint.

#### **Phase 0: Groundwork** (no 6502 code yet)

| Step | Work | Size |
| :--- | :--- | :--- |
| 0.1 | **A new tree** beside the old (a folder such as `os2/`, or a new repository): `spec/`, `kernel/`, `modules/drivers/`, `modules/sys/`, `modules/lang/`, `modules/lib/`, `sdk/asm/`, `sdk/c/`, `romfs/`, `tools/`.  `sim/` stays shared; its tests split into the old system's and the new one's | S |
| 0.2 | **The conventions document**: this plan's §8 and §9 as rules, the source style (the current column layout, routine headers, CRLF), naming (`UPPER_SNAKE` for calls and constants, a module prefix for internal names), the zero page and OS area layouts, the error rules | S |
| 0.3 | **The specification format and `apigen.js`**, with the first ten calls (`EXITS`, `PUTC`, `PUTS`, `GETC`, `TICKS`, `YIELD`, `SLEEP`, `SYSINFO`, `ERRSTR`, `SPAWN`), generating the jump table, `hydra.inc`, the docs table | M |
| 0.4 | **The build**: separate assembly of modules, the BIOS link config (16 pages, COMMON at a fixed place), the module link config (`$A000`, data copied to RAM), the program link config (`$0800`), the image builder from `mkromdisk.js`, the budget report | M |
| 0.5 | **Emulator additions**: loading the new images, the call trace, breakpoints by label, the task view | M |
| 0.6 | **CI** for the new tree | S |

**Done when:** an empty kernel (reset, polled serial) boots in the emulator from the new build and prints a banner; a test checks the banner; the build prints its budget report.

#### **Phase 1: The kernel core**

| Step | Work | Size |
| :--- | :--- | :--- |
| 1.1 | **Reset and POST**: the reset stub on every page, `T U V W` set, POST's RAM line tests ported from `tests/post.s` and `post_ram.s`, `T` during POST into the hardware test (unchanged), the hardware probe | M |
| 1.2 | **The kernel task and the per-task areas**: task 0's data layout, the OS zero page and OS area layouts, `kernel/kx.s` (the quick-look primitives); the build's check that nothing else writes `T` | S |
| 1.3 | **Spike S1, the IRQ path**: the COMMON stubs, the dispatcher, a handler in an owner task; measured in the emulator.  **Budget: 100 cycles to the handler; 115200 receive with no loss** | M |
| 1.4 | **Spike S2, kcopy**: bursts of 8 bytes, each task's pointers in its OS zero page.  **Budget: 40 cycles a byte; no IRQ-off stretch over 200 cycles** | S |
| 1.5 | **Spike S3, SCALL and KCALL**: the lean call into another task, waiting when it's busy, a guest switched out mid-call.  **Budget: 200 cycles for a round trip with no data** | M |
| 1.6 | **Tasks and the scheduler**: the task table, states, frames (with `U` and `W`), round robin, the run-next rule, `PREEMPT_OFF`/`ON`, wait masks, the tick, `SLEEP`, `YIELD`, CPU time | M |
| 1.7 | **The jump table and the first calls** (from 0.3), with a polled console for bring-up | S |
| 1.8 | **Memory**: `BRK`, `PAGES_*`, `BANKS_*`, shared segments; the probe's results applied (bad chips reserved, missing modules) | M |
| 1.9 | **Notes, exits and waiting**: `NOTIFY`, `NOTE`, the default actions, `EXITS`, `WAIT`, orphans to task 1, note groups | M |

**Tests:** the spikes' budgets; a scheduler test (three tasks interleaving, a preempt-off section, wait and wake); kcopy between tasks with buffers in RAM, in a bank and in a shared bank; the allocators; notes (a handler that continues, the default ending, kill); exit records and orphans; the IRQ-off limit; fault injection at POST.  **On the board:** POST, the tick, a test module that echoes the serial port at 115200 from its interrupt.

#### **Phase 2: Modules, servers and the console**

| Step | Work | Size |
| :--- | :--- | :--- |
| 2.1 | **HYX2 and the module directory**: the header, bank 0's directory, the kernel's scan, `#m` | S |
| 2.2 | **The driver life cycle**: start (task from the top, data copy, `init` by SCALL), `IRQ_OWN`, `SRV_REGISTER`, failure clean-up, `stop` | M |
| 2.3 | **srvlib**: the file tree tables, walks, fids, text files, ctl parsing, stat and directory records, client data, wait masks | M |
| 2.4 | **Channels and the request path**: fd tables, the channel table, `OPEN`, `CREATE`, `READ`, `WRITE`, `CLOSE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `REMOVE`, `DUP`, `DUP2`; blocking and retry; FLUSH on a note | L |
| 2.5 | **Namespaces**: `#` names, `#/`, the mount tables and string pool, resolution, unions and union directories, `BIND`, `MOUNT`, `UNMOUNT`, `CHDIR`, `GETCWD`, inheritance and clean namespaces | L |
| 2.6 | **The kernel devices**: `#n`, `#e`, `#p` (status, ctl, ns, fd, args), `#\|` with `PIPE`, `#t` | M |
| 2.7 | **The console driver**: the ACIA (both chips; pacing at 115200: ported from `drivers/serial.s` and `servers/serfast.s`), the rings, `#c`, cooked mode with line editing and history, raw mode with key decoding, `consctl`, `serctl`, windows (a console each, rio's way: `#c` with a spec, switched with Ctrl-] and a digit, repainted from their text), the break and kill keys | L |
| 2.8 | **A first `init`**: mounts from a namespace built into its module (no disk yet), starts a test shell that echoes lines and runs `ls`-like listings of `#` devices | S |

**Tests:** a server built on srvlib in a test module (every request, errors, text files at offsets, ctl commands); opens through binds, unions (lookup order, creates, union directory reads on two fds), hides, inheritance; pipes between tasks (full, empty, broken, end of file); the console (cooked editing keys, raw keys, Ctrl-C to the shown window's note group, windows made, switched, repainted and gone, a window not shown waiting for keys, 115200 output and a 1000-character paste at 57600 with nothing lost, as today's tests).  **On the board:** the console at 9600 and 115200, both ACIA variants if both chips are available.

#### **Phase 3: Storage**

| Step | Work | Size |
| :--- | :--- | :--- |
| 3.1 | **SPI and `#S`**: the bit loops ported unchanged (their timing is proven), device arbitration, modes 0 and 3 | S |
| 3.2 | **The block layer**: SD cards (ported from `drivers/sd.s`), the ROM disk through the kernel's `ROMREAD`, the RAM disks with `start`/`stop`, the block buffers | M |
| 3.3 | **`#d`**: `/dev/sd/N/data`, `ctl` (`init`, the disk's description) | S |
| 3.4 | **HydraFS (`#f`)**: ported from `fs/hfs_srv.s`, `hfs_write.s`, `hfs_sparse.s`, `hfs_format.s`, `hfs_check.s` into the storage module: walks, reads, writes, create, remove, wstat, partitions, sparse files, quick and full format, label, check and fix; 32 open files; the disks in memory only through a spec | L |
| 3.5 | **The ROM disk image**: `romfs/` and its manifest, the namespace file, the profile, `README` | S |
| 3.6 | **`init` from files**: `/rom/lib/namespace`, a card's `/lib/namespace`, `/rom/lib/drivers`, `/rom/lib/profile`; each shell's `/ram` | S |

**Tests:** today's storage tests, ported: cards made by `hydrafs.js` read and written, then checked by it; fixture images in `sim/cards` (old cards must still read); quick format on a big card, partitions next to FAT, sparse files, the check finding planted faults, the ROM disk read back against its sources, RAM disk start and stop, read throughput budgets (bytes a second from a card, from the RAM disk).  **On the board:** a real card read and written, then checked on the PC.

#### **Phase 4: Programs and the rc shell**

| Step | Work | Size |
| :--- | :--- | :--- |
| 4.1 | **The loader and `SPAWN`**: RAM programs and in-place modules, arguments and name, the fd map, the flags; `WAIT` from the shell's side | M |
| 4.2 | **rc**: the language (§15.2), its built-ins, pipelines (each stage a program), redirection, background tasks and `$apid`, `$status`, `rc -c`, the prompt | L |
| 4.3 | **The core tools** (§15.3, the file and system groups first) | M |
| 4.4 | **The assembly SDK**: generated `hydra.inc`, macros, `hyx2.cfg`, samples | S |
| 4.5 | **The C target and library** (§15.4), the samples ported (`hello`, `upper`, `code`, `keys`, `tones`, `jukebox`, `ctest`) | L |
| 4.6 | **`edit`**, ported as a program (from `shell/edit.s`) | S |

**Tests:** today's shell tests, rewritten for rc (programs by name from `.` and `/bin`, scripts, redirection, pipelines, background tasks, exit statuses, Ctrl-C on a program); `ctest` passing; load-time budgets.

#### **Phase 5: The remaining devices: parity**

| Step | Work | Size |
| :--- | :--- | :--- |
| 5.1 | **Sound** (`snd` module, `#a`): the library ported from `sound/`, `/dev/snd`, `/dev/sndctl`, the bell from `cons` | M |
| 5.2 | **`play`** (ported from `sound/player.s`), songs on the ROM disk, `scom` | S |
| 5.3 | **`#g` GPIO** with CA1, **`#i` I2C** (new) | M |
| 5.4 | **The clock**: the DS1747 (ported from `drivers/rtc.s`), `/dev/time` | S |
| 5.5 | **`/pc`** in the console driver (protocol 2), the PC tool and the emulator's `--pc-dir` updated | M |
| 5.6 | **`xmodem`**, as a program | S |
| 5.7 | **`/proc`'s remaining files**: `mem`, `ram`, `regs`, `note`, `cwd`, `env` | S |

**Parity checkpoint:** every feature in [§21](#21-from-the-current-system-to-the-new-one-feature-by-feature) that isn't a language works on the new system; the tutorial's steps (rewritten for rc) work; the full suite passes; a day of use on the board.  From here the new system is the daily one, and the old tree is frozen.

#### **Phase 6: HyForth**

| Step | Work | Size |
| :--- | :--- | :--- |
| 6.1 | The Forth's core: the inner model (subroutine threaded), the interpreter and compiler, Core and Core Extension | L |
| 6.2 | Exception, File Access, Facility, String, Search-Order, Programming-Tools | M |
| 6.3 | The Hydra words from the specification; `sh` and `run`; bank and segment words | S |
| 6.4 | Libraries from `/lib/forth`; a `forth` program and `.fs` scripts | S |

**Tests:** the Forth 2012 test suite; the Hydra words; scripts in pipelines (`forth script.fs | wc`).

#### **Phase 7: hylang**

| Step | Work | Size |
| :--- | :--- | :--- |
| 7.0 | **The language specification**: from danlang's active interpreter and `globals.dl`, what hylang 1 includes ([Appendix F](#appendix-f-danlang-to-hylang-the-inventory)); the conformance suite started in C# | M |
| 7.1 | **Spike S5**: the value and heap scheme (16-bit tagged values over the task's banks) and the collector, prototyped and measured (pairs a second, collection pauses) | M |
| 7.2 | **The reader, printer and evaluator**: danlang's syntax and shorthands, special forms, closures with partial application, `&_`/`&N`, errors as values, tail calls, interruption by notes | L |
| 7.3 | **Numbers**: fixnums and 32-bit integers, then the bignum library module, rationals, fixed decimals; the bases library | L |
| 7.4 | **Data and IO**: strings, characters, atoms, hashes with tags and methods, streams over fds, `load`, `save` | M |
| 7.5 | **The shell layer**: the line rule (lisp or rc), `run`, `sh`, `sh-out`, environment access, the profile | M |
| 7.6 | **The library**: `globals.dl` ported to `globals.hl`; examples (`harn.hl` as a sample program) | S |
| 7.7 | **hylang as the login shell**: `init` starts it on the console (rc stays the shell of scripts and `system()`) | S |

**Tests:** the conformance suite against the C# danlang; GC stress (long-running loops with allocation); the shell rule; Ctrl-C in a loop.

#### **Phase 8: The Vera X card**

[VIDEO.md](plans/VIDEO.md)'s order of work holds, on the new structure: the emulator's VERA and `--screen`; the carrier card and its timing check; the `vid` driver module (`#v`), detection at boot, the font and the screen console as `cons`'s second back end; claims and direct access; HyForth's and hylang's graphics words and C's `vera.h` (with cc65's TGI driver); the input controller over `#i` and the keyboard into `cons`; the PSG and PCM in `snd`.

#### **Phase 9: Tools and debugging**

The debugger as a program over `/proc` (`regs`, `ctl`'s `stop`, `step`, `break ADDR`, `mem`, symbols from module `.dbg` files copied to the card or `/pc`); a screen editor on conio; an assembler (a program, or HyForth words); more tools (§15.3); networking when a W5500 card exists.

#### **Phase 10: The X16 migration utility**

First the analyser (§18, part 1) run over a set of X16 programs, then the decision: if most interesting programs grade "ports with edits", build the converter and the shim library; if not, document the porting guide and stop.

#### **The order, at a glance**

```
 0 groundwork --> 1 kernel core --> 2 modules, servers, console --> 3 storage --> 4 programs, rc --> 5 devices (parity)
                     (spikes S1-S3)                                                                    |
                                                                  6 HyForth <--------------------------+
                                                                  7 hylang (spike S5) <----------------+
                                                                  8 Vera X  <--------------------------+
                                                                  9 debugger, tools; 10 X16 utility
```

Phases 6, 7 and 8 depend only on phase 5 and can go in any order or in parallel; hylang's becoming the login shell (7.7) is the natural milestone for "the new system is complete".

---

### **21. From the current system to the new one, feature by feature**

| Today | In the new system | Phase |
| :---- | :---------------- | :---- |
| POST, the hardware test | Ported (POST) and kept (hardware test) | 1 |
| WOZMON, the disassembler | Dropped; the debugger (phase 9) and `xd` replace them | 9 |
| Tasks, round robin, `NO_PREEMPT` | Kept, leaner; `PREEMPT_OFF`/`ON` | 1 |
| `TASK_RUN`, `TASK_START`, `TASK_CLONE`, `TASK_PREPARE`, `SPAWN_TASK`, `DRV_START`, `SHELL_CMD` | `SPAWN` (programs and modules), the driver life cycle, `rc -c` | 1, 2, 4 |
| `TASK_EXITS`, `TASK_JOIN`, `/env/status` | `EXITS`, `WAIT`, `$status` | 1, 4 |
| `TASK_SIGNAL`, `TASK_SET_BREAK`, the owner chain | Notes, `NOTIFY`, note groups | 1 |
| `TASK_CALL`, `TASK_GATE` | SCALL, KCALL | 1 |
| IRQ dispatcher plus three fast handlers | One fast path | 1 |
| Software interrupts (`SW_INT`, `SWI_REGISTER`) | Dropped; BRK is a fault note | 1 |
| `MM_*` (4 tiers, handles), `SH_*`, `FP_*`, `MM_REF`, `SH_REF` | `BRK`, `PAGES_*`, `BANKS_*`, segments | 1 |
| Semaphores | Kept (`SEM_*`), in the kernel task | 1 |
| The thunk table on pages 0 and 1 | One generated jump table on page 0 | 0, 1 |
| fds in task RAM, transfer areas, 256-byte IO unit | Channels, kcopy, no unit | 2 |
| `IO_CTL` codes | ctl files | 2 |
| Text directory listings | Stat records only | 2 |
| `/dev/null`, `/dev/zero`, `/dev/pipe`, `/env`, `/proc` (and `/dev/proc`), `/dev/time`, `/dev/ram`, `/`, `/dev` | `#n`, `#\|`, `#e`, `#p` (one name: `/proc`), `#t`, `#r`, `#/` | 2 |
| `/dev/cons`, `/dev/cons/ctl`, `/dev/ser`, `/dev/ser/ctl`, foreground, console keys | `#c`: `cons`, `consctl`, `ser`, `serctl`, note groups; windows in place of the foreground; line editing for everyone | 2 |
| Namespaces: 32 + 32 entries, unions, `hide`, specs, `newns` | Kept, with `#` names, longer paths, union directories on any fd, no `/dev` fallback, no `$PATH` | 2 |
| Current directory | Kept | 2 |
| `/dev/spi` | `#S` | 3 |
| `/dev/sd/N` (data, ctl), HydraFS, partitions, sparse files, check, format | `#d`, `#f`, the same format and commands | 3 |
| ROM disk `/rom`, RAM disks `/ram` (per shell), `/sram`, program caches | Kept; caches as union members of `/bin` and `/lib` | 3 |
| Boot shell's mounts, `/rom/lib/namespace` | `init` and the namespace file | 3 |
| HyForth as the shell, its file commands, `run`, scripts, pipelines, redirection, `&`, `wait`, `send` | rc (and the tools); `send` becomes writing to `/proc/N/note` or a shell's input pipe | 4 |
| `.hyx` (HYX1) programs, the loader, arguments | HYX2 programs and modules (programs rebuilt with the new SDKs) | 4 |
| The C library | The `hydra` cc65 target, generated bindings | 4 |
| `edit` | `edit` (a program) | 4 |
| `/dev/snd`, the sound library, claims, the sound clock, `sndtest`, the bell | `#a` (`snd`, `sndctl`), the bell through `cons` | 5 |
| `play`, ZSM songs | `play` (a program) | 5 |
| `/dev/gpio` | `#g` | 5 |
| DS1747 | In `#t` | 5 |
| `/pc`, the PC tool | `#P`, protocol 2, the PC tool | 5 |
| `/proc/N/mem`, `ram`, `ns`, `cmd`, `pages` | `mem`, `ram`, `ns`, `regs`, `note`, `fd`; `pages` folded into `status` | 5 |
| HyForth (non-standard) | HyForth (Forth 2012) | 6 |
| — | hylang | 7 |
| Vera X (planned) | `#v`, `#k`, the screen console | 8 |
| — | The debugger | 9 |
| — | The X16 migration utility | 10 |

---

### **22. Risks and open questions**

**Risks**, with what's done about each:

| Risk | Mitigation |
| :--- | :--------- |
| The IRQ path, kcopy or SCALL miss their budgets, and 115200 loses bytes | Spikes S1-S3 in phase 1, before anything depends on them; fall-backs exist (a kcopy burst of 4 bytes; a dedicated fast path for the ACIA alone) |
| Running modules in place from the paged ROM has a flaw not seen on paper (a path that leaves `W` non-zero, a bank left selected) | A build check that module code never writes `W` or `T`; an emulator check that every jump table call is made with `W` = 0; an early board test of a module running in place |
| The kernel task becomes a bottleneck | KCALLs never block and are short; device work never goes through task 0; measured in phase 2 with several busy tasks |
| hylang's heap and collector are too slow or too big for the Hydra | Spike S5 before phase 7's main work; the numeric tower lets small programs avoid bignums |
| Porting HydraFS breaks existing cards | Keep the format; the fixture images and `hydrafs.js` cross-checks from the start of phase 3 |
| No memory protection | Placement (P10) keeps accidents contained; [Appendix G](#appendix-g-v2-hardware-wishes) proposes protection for V2 |
| 16 tasks: kernel, `init` and 3-5 drivers leave 10-12 | Small devices live in the kernel task (§14.1); drivers group related devices (storage owns every disk and SPI) |
| A long rewrite stalls with neither system complete | The old tree stays the daily system until phase 5; each phase ends runnable; parity is an explicit checkpoint |
| Programs built for today's ABI stop working | They're rebuilt with the new SDKs (all are in the repository); the change is announced with the parity release |

**Open questions** for the user:
1. **The new system's name**, and whether it lives in this repository (a new folder) or a new one.
2. **Device letters** (§14): the ones proposed follow Plan 9 where there's an equivalent; confirm or change.
3. **hylang**: the file extension (`.hl`?), the rule for telling lisp lines from rc lines (§17.4), and whether `$` becomes environment access inside hylang.
4. **danlang's license**: it's GPLv3 (Daniel and Simon Struthers).  hylang in the ROM makes the ROM image a combined work; choose a license for the Hydra's software as a whole, or relicense hylang.
5. **Each shell's `/ram` permission check**: keep it (generalised in srvlib) or drop it as unnecessary on a single-user machine.
6. **Binaries in Git**: move the ROM images to release artefacts, or keep committing them for people without a toolchain.
7. **Which tools in C and which in assembly** (§15.3), once the C target's code size is measured in phase 4.

---

## **Appendices**

### **Appendix A: The API, by group**

A sketch of the calls; the specification file is the final word.  Each group has its own base in the jump table and spare slots.  Registers follow §9.2: `.A` small first argument, `r0`... pointers and 16-bit values; results in `.A`, `.A/.X`, `r0`...; C = 1 and `.A` = error on failure.

| Group (base, slots) | Calls |
| :------------------ | :---- |
| **System** (`$F800`, 16) | `SYSINFO` (ABI version, RAM modules, task count, clock speed), `ERRSTR` (`.A` = code, `r0` = buffer: its text), `XCALL` (call into another paged ROM bank), `MODINFO` (`r0` = name: a module's header) |
| **Tasks** (`$F830`, 32) | `SPAWN`, `EXITS`, `WAIT`, `GETPID`, `GETPPID`, `NOTIFY`, `NOTE`, `NOTED`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE` |
| **Memory** (`$F890`, 16) | `BRK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP` |
| **Files** (`$F8C0`, 32) | `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `PIPE`, `CHDIR`, `GETCWD`, `FD2PATH` |
| **Names** (`$F920`, 16) | `BIND`, `MOUNT`, `UNMOUNT` (and `SPAWN`'s clean-namespace flag in place of a `NEWNS` call) |
| **Console helpers** (`$F950`, 16) | `PUTC`, `PUTS`, `GETC`, `PUTHEX` (a byte as two digits, for debugging) |
| **Time** (`$F980`, 16) | `CLOCK` (seconds since 2000-01-01 into `r0:r1`), `CLOCK_TEXT` (`r0` = buffer) |
| **Servers and drivers** (`$F9B0`, 32) | `SRV_REGISTER`, `IRQ_OWN`, `IRQ_RELEASE`, `CLIENT_READ`, `CLIENT_WRITE`, `CLIENT_INFO`, `WAIT_ADD`, `WAKE_ALL`, `ROMREAD` |
| Spare | `$FA10` up to the end of the table's space |

The bases above are illustrative: the generator assigns them, and the specification fixes them once published.

### **Appendix B: The request block**

What the stub writes into the server's inbox (its OS area, `$0200`) for each request.  32 bytes:

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 1 | The request (`OPEN`, `CREATE`, `READ`, `WRITE`, `CLUNK`, `STAT`, `WSTAT`, `REMOVE`, `FLUSH`, `DUP`) |
| 1 | 1 | The fid (in); the new fid (out, `OPEN` and `CREATE`) |
| 2 | 1 | The mode: the open mode, or the channel's, on every request |
| 3 | 1 | The client task |
| 4 | 1 | The client's `U`, for a buffer in a shared bank |
| 5 | 1 | Flags: non-blocking; came through a mount with a spec |
| 6 | 4 | The offset |
| 10 | 2 | The count asked for |
| 12 | 2 | The count done (out) |
| 14 | 2 | The buffer, in the client's address space |
| 16 | 1 | `CREATE`: the new file's mode bits |
| 17 | 1 | The name's length (`OPEN`, `CREATE`, `REMOVE`: the name itself is at `$0300`, after the namespace and the spec) |
| 18 | 1 | The client's note group (for ownership checks) |
| 19 | 13 | Reserved |

The reply is in registers: C = 0, or C = 1 with the error in `.A`; the count done and a new fid in the block.  The `/pc` protocol's version 2 sends the same block in its request frames.

### **Appendix C: The executable and module header (HYX2)**

48 bytes, for programs and modules alike:

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 4 | `HYX2` |
| 4 | 1 | The header's size (48) |
| 5 | 1 | The type: program, driver, library |
| 6 | 1 | Flags: runs in place; boot driver; needs banks; wants the console |
| 7 | 1 | The ABI version it was built for |
| 8 | 2 | Load address (`$0800` for a RAM program, `$A000` for a module in place) |
| 10 | 2 | The image's length |
| 12 | 2 | Its initialised data: where it runs (in RAM) ... |
| 14 | 2 | ... and its length (copied there from the image for a module in place) |
| 16 | 2 | Its BSS's length (cleared) |
| 18 | 2 | The top of the RAM it uses (its break starts there) |
| 20 | 2 | Entry: `main` (program) or `init` (driver) |
| 22 | 2 | Entry: `serve` (driver) |
| 24 | 2 | Entry: `irq` (driver) |
| 26 | 2 | Entry: `stop` (driver) |
| 28 | 1 | The device letter (driver) |
| 29 | 1 | Banks the module spans (in place) |
| 30 | 2 | Its version |
| 32 | 16 | Its name, zero-terminated |

### **Appendix D: Error codes**

POSIX names, so C's `errno` maps one to one, and a text for each from `ERRSTR`:

| Range | Codes |
| :---- | :---- |
| General (`$01-$1F`) | `E_PERM`, `E_INVAL`, `E_NOSYS`, `E_AGAIN`, `E_INTR`, `E_NOMEM`, `E_BUSY`, `E_RANGE`, `E_FAULT` (a buffer the caller can't see), `E_NAMETOOLONG`, `E_TOOBIG` |
| Files (`$20-$3F`) | `E_NOENT`, `E_EXIST`, `E_NOTDIR`, `E_ISDIR`, `E_NOTEMPTY`, `E_BADF`, `E_MFILE` (no fd), `E_NFILE` (no channel), `E_NOSPC`, `E_ROFS`, `E_IO`, `E_NODEV`, `E_PIPE`, `E_NOEXEC`, `E_EOF` (a byte read at the end), `E_NOTFS` (no HydraFS), `E_MEDIA` |
| Tasks (`$40-$4F`) | `E_NOTASK` (all 16 in use), `E_SRCH` (no such task), `E_CHILD` (not a child) |
| Names (`$50-$5F`) | `E_NSFULL`, `E_NSLOOP` |
| Devices (`$80-$BF`) | Defined by drivers, with their texts in their modules |

### **Appendix E: The default namespace**

`/rom/lib/namespace`, which `init` runs (and a card's `/lib/namespace` after it).  `$task` is the shell's task, set by `init`:

```
# the root and the devices
bind '#/' /
bind -a '#c' /dev           # cons consctl ser serctl
bind -a '#n' /dev           # null zero
bind -a '#t' /dev           # time ticks
bind -a '#g' /dev/gpio
bind -a '#i' /dev/i2c
bind -a '#a' /dev           # snd sndctl
bind '#d' /dev/sd
bind '#S' /dev/spi
bind '#m' /dev/mod
mount '#e' /env
mount '#p' /proc

# the disks
mount '#f' /sd                  # the cards: /sd/0 ... /sd/f
mount '#f' /rom x               # the ROM disk
mount '#f' /sram s              # the shared RAM disk
mount -c '#f' /ram r/$task      # this shell's own area of the RAM disk

# programs and libraries: unions, the caches first
bind -c /ram/bin /bin
bind -a /sram/bin /bin
bind -a /sd/0/bin /bin          # (added only if the card has one)
bind -a /rom/bin /bin
bind -a '#m/bin' /bin           # the ROM's program modules
bind -c /ram/lib /lib
bind -a /sram/lib /lib
bind -a /sd/0/lib /lib
bind -a /rom/lib /lib

# the PC
mount '#P' /pc
```

### **Appendix F: danlang to hylang: the inventory**

| Feature (danlang) | hylang 1 | Later |
| :---------------- | :------- | :---- |
| S-expressions, Q-expressions, `;` comments | Yes | |
| Strings, multi-quote here strings | Yes | Escapes (danlang's are unfinished) |
| Case-insensitive symbols, atoms `:x`, characters `\name` | Yes | |
| `T`, `NIL`, `exit` | Yes | |
| Prefix shorthands `' ^ $ . \| = : @ ! ? # < > *` | Yes | `~(` format, once danlang defines it |
| `fn`, partial application, `&_`, `&N`, `&0` | Yes | |
| `def`, `set`, environments | Yes | |
| Built-ins with unevaluated arguments; evaluated built-ins | Yes | |
| `if`, `and`, `or`, `<=>` | Yes | |
| `list head tail init end join eval len item-at subset` | Yes | |
| `+ - * /`, `eq neq < > cmp` | Yes | |
| Integers of any size | Fixnums, 32-bit; bignums from the library | |
| Rationals, fixed decimals | From the library | |
| Complex numbers | | Yes |
| Number bases (`#x`, `#b`, balanced, negative, `<`/`>` order, `_`) | Hex, binary, decimal, `_` | The rest from the bases library |
| `to-str`, `val`, `to-fixed`, `to-rational`, `truncate`, `to-sym`, `to-atom`, `rational.n/d` | Yes (with the library for the number types) | |
| `substring`, `char-at`, `str-split`, `index-of`, `last-index-of` | Yes | |
| Hashes: create, get, put, call, clone, keys, values, tags, `to#`, `from#` | Yes | |
| Type predicates | Yes | |
| `print`, `load`, `save`, `error` | Yes | |
| Streams | Over fds: `open`, `read-line`, `read-byte`, `write`, `close`, `seek` | |
| `random`, `fib` | `random` (the kernel's entropy and a generator) | `fib` in the library |
| `globals.dl` | Ported as `globals.hl` | |
| `harn.dl`, `cngh.dl` | Example programs | |
| The REPL's timing display | `(time expr)` | |
| — (new) | `run`, `sh`, `sh-out`, `env`, `setenv`, the shell line rule | |

### **Appendix G: V2 hardware wishes**

Changes a V2 board could make that would simplify or speed up this design, roughly in order of value.  None is needed by the plan.

| Change | What it would give the software |
| :----- | :------------------------------ |
| **RDY wait states** (as [IDEAS.md](plans/IDEAS.md) plans) and a readable clock register | The CPU at 7-14 MHz with the YM2151; timing constants read at boot instead of built in |
| **A UART with a FIFO** (a 16C550 class part) instead of the 65C51, or a 1.8432 MHz ACIA clock | 115200 without pacing, a sixteenth of the interrupts, exact baud rates; the strictest IRQ budget relaxes |
| **Bank register bits in order** | No bit swaps in the image builder and the emulator |
| **A protection bit**: writes to `T U V W` and to `$00/$01` of other tasks allowed only while executing from the BIOS ROM | Real isolation between tasks: a program couldn't switch `T`.  Detectable with `SYNC` and the address decoder |
| **Hardware SPI** (a shift register on the SPI lines, or a dedicated controller) | Card reads several times faster (the bit-banged loop is about 60% of a block read today) |
| **A per-task `U`** (in the bank register file, like `$00` and `$01`) | No `U` in task frames; shared banks selected per task like private ones |
| **An IRQ mask register** | Unowned lines can be masked instead of left counting |
| **An interrupt "owner task" latch**: on an interrupt, the hardware switches `T` to a task set per line, and restores it on `RTI` | Interrupts in driver tasks at almost no cost |
| **Readable bank registers** | No RAM mirror to keep in step |
| **The audio jack's channels** the usual way round | |
| **I/O moved to `$FE00-$FEFF`**, freeing `$FF00-$FFF9` for ROM | A Commodore-style jump table at the X16's addresses; only useful if X16 binaries are a goal |

### **Appendix H: Glossary**

| Term | Meaning |
| :--- | :------ |
| **BIOS ROM** | The 8K-paged ROM at `$E000`, page selected by `W`: the kernel's home |
| **Paged ROM** | The 4 MB of 16K banks at `$A000`, bank selected per task by `$01`: modules and the ROM disk |
| **Module** | An executable in HYX2 format: a program, a driver or a library; in the paged ROM (run in place) or in a file (loaded) |
| **Kernel task** | Task 0, whose RAM and banks hold all global kernel state; also the idle task |
| **OS area, OS zero page** | Each task's `$0200-$03FF` and `$80-$FF`, which the kernel owns |
| **SCALL** | A call into a server's task, running its code on its stack (today's `TASK_CALL`) |
| **KCALL** | An SCALL into the kernel task, for structural kernel work; never blocks |
| **kcopy** | The kernel's copy between two tasks' memory, a few bytes at a time with IRQs off |
| **Quick look** | Switching `T` to another task for a few instructions with IRQs off and no stack use |
| **Channel** | The kernel's record of an open file, shared by the fds that refer to it |
| **srvlib** | The server library every server is built on |
| **Note, note group** | Plan 9's signals, and the set of tasks a note to a group reaches |
| **Spike** | A short prototype that measures a mechanism against its budget before code depends on it |
| **XIP, in place** | Executing a module's code directly from the paged ROM |
