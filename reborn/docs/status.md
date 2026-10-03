# Status

Where the rebuild stands against the plan's phases
([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md), Part III), what the
spikes measured, and what measuring changed.

## In short

Phases 0 and 1 are done, phase 2 is all but done, and phase 3 (storage) is half done.  The kernel boots in the emulator, runs POST (with the old
hardware test a key away), starts its modules from the paged ROM in tasks of their own, schedules them
preemptively, runs calls between tasks and copies between them, takes every interrupt through one path, manages
task RAM, banks and shared segments, and delivers notes.  The file layer is in: fds, channels, requests to
servers, servers built on srvlib, waits that a server's interrupt can end for a few cycles; and namespaces, Plan 9's:
binds, mounts, unions and union directories, a current directory, shared by a task's children till they change
them.  The console driver (`cons`, task F) serves `#c`: windows, Plan 9's way (rio's, on the serial terminal: a
console each, shown with Ctrl-] and a digit, repainted from its text), lines edited at the console, raw keys,
Ctrl-C and Ctrl-\ to the shown window's note group, and paced sending up to 115200.  The kernel's own devices are a driver of
their own (`kdev`): the root's mount points, null and zero, the ticks, the modules, the tasks, and pipes.  init
builds its namespace (the plan's appendix E, as far as its devices go) and runs a test shell (`tsh`) in window 0,
and one in each window the user asks for (Ctrl-] c).  The storage driver (`storage`, task E) owns the SPI bus and
the disks: `#S` (the SPI devices), and `#d` (SD cards, the ROM disk, the RAM disks, through a block buffer);
HydraFS (`#f`) is next.

```
PASS boot    the kernel boots, POST finds nothing wrong; init runs hello and waits for it
PASS post-t  POST: a T line stuck low (U7)
PASS post-zp POST: only the zero page and stack per task (a decoding fault)
PASS post-ram POST: an address line of RAM module 1 stuck low; the module is left unused
PASS post-sh POST: no shared RAM
PASS hwtest  a T typed during POST starts the hardware test (paged ROM bank 1)
PASS task    tasks and the scheduler: SPAWN, EXITS, WAIT, SLEEP, preemption, PAUSE and WAKE, orphans  (29 checks)
PASS note    notes: the defaults, handlers, a note to oneself, WAIT ended by one, note groups  (29 checks)
PASS file    files and servers: OPEN, READ, WRITE, SEEK, STAT, DUP; text, ctl, data, directories; waiting  (54 checks)
PASS ns      namespaces: BIND, MOUNT, UNMOUNT, unions and union directories, CHDIR, clean names, inheritance  (41 checks)
PASS dev     the kernel's devices (kdev): #/, #n, #t, #m, #p; pipes; a union keeping what was there  (35 checks)
PASS spi     SPI and #S (storage): transactions, kept bytes, modes 0 and 3, one open at a time, the time a byte takes  (43 checks)
PASS disk    the disks (storage): #d, the ROM disk, SD cards (SDHC and SDSC), RAM disks, their ctl files, the time a byte takes  (58 checks)
PASS cons    the console: lines, editing, history, raw keys, Ctrl-C, windows (shown, repainted, made, gone), 115200
PASS mem     memory: BREAK, pages, banks, a shared segment between tasks (and kcopy from it)  (38 checks)
PASS scall   spike S3: calls into a driver's task, its errors, a busy driver, the round trip  (12 checks)
PASS kcopy   spike S2: copying between tasks  (6 checks)
PASS irq     spike S1: 115200 received by an irq entry while tasks spin  (6 checks)
```

The same with the power-up's RAM from other seeds; the console, file, namespace and device tests the same with a
WDC W65C51N build, the console test with a 7.16 MHz build, and the SPI and disk tests with both.  The hardware test, entered from POST in the emulator, passes its whole quick run, its BIOS and
paged ROM checksums included.

## The spikes and budgets (3.58 MHz)

| Spike | Budget | Measured | How |
|---|---|---|---|
| S1: the IRQ path | ~100 cycles to the handler | **77 cycles** from the interrupt to the handler's first instruction (7 the CPU's, 22 the COMMON stub's, 48 the dispatcher's, also in the COMMON block); timer 2's 14 more (`IRQ_VIA`) | Counted from the code; the irq test's fastest byte, 100 cycles from arriving to being read, agrees |
| S1: 115200 received | No loss | **2000 of 2000 bytes**, in order, while two tasks spin and the tick switches tasks; each byte read 100-277 cycles after it came (108 on average) of the 320 a character takes | The irq test: the PC sends back to back; the emulator counts each byte's wait and the ACIA's losses |
| No IRQs-off stretch over 200 cycles | 200 | **189**, the console's timer 2 sending a byte (stub to `RTI`); the tick **170**; the longest stretch of masked code, **171**, a switch into a task waking from a sleep | Every test, from the boot's end |
| S2: kcopy | 40 cycles a byte | **36.7** | 4096 bytes to the kernel task (the kcopy test) |
| S3: SCALL | 200 cycles a round trip | **181.3** | 1000 calls to a driver, less the same loop calling the driver's code in place (the scall test) |
| SPI through `#S` | The old bit loops, unchanged | **246 cycles a byte** clocked in (256 bytes a READ), **393** sent; 302 in at 7.16 MHz, where the receive loop is padded to keep SCLK under an SD card's 400 kHz | The spi test, less the marks' own time |
| Reading a card | The old system's 298 cycles a byte | **257 cycles a byte** (about 14 KB/s at 3.58 MHz): 4096 bytes in 512-byte reads of `#d/0/data`; a RAM disk **61** | The disk test, less the marks' own time |
| Sending at 115200 | No overrun; 2 idle bits (Rockwell), 1 (WDC) | **No overruns; at least 6.4 idle bits** between characters (5.4 on the WDC build, 4.2 at 7.16 MHz): about 7,000 characters a second, as the old driver | The cons test, its last line at 115200 |

## What measuring changed

The spikes did what the plan meant them to: the first versions missed every budget (SCALL 474 cycles, the tick
450 cycles with sleepers, 23 bytes lost in 2000 at 115200), and these changes brought them in.  They refine the
plan's §10:

1. **The scheduler ends three kinds of waits itself.**  A sleeper is `ST_SLEEP` with its wake time in the kernel
   task's table, a caller that finds a task busy is `ST_BLOCKED` on it, and a client waiting for a server is
   `ST_EVENT` on the server's event count; the scheduler, as it looks for the next task, makes one ready when its
   time has come, its task is free, or the count has changed.  So the tick's interrupt only counts, a call's end
   wakes nobody, and a server's interrupt wakes its clients with one `inc` (below): no wait masks scanned with
   IRQs off.
2. **The tick is short** (40 cycles, CPU time included): it counts the ticks (32 bits) and charges one to the task
   it interrupted.  The clock (phase 5) will be the boot's time and the ticks since, not a seconds counter in the
   interrupt.
3. **The kernel task is never preempted** (its preemption count is always at least 1; it yields when it idles).
   A KCALL runs to its end without SCALL carrying a "hold" into the task called.  A program's `PREEMPT_OFF`
   holds its own task only: a driver it calls can be switched out.
4. **A KCALL's routine is named in the caller's own zero page**, where the kernel task reads it: naming it needs
   no IRQs off (with a shared name, a KCALL from page 1 held them off for 190 cycles).
5. **Every task has a copy of the IRQ lines' owners** (`TA_OWNERS`), so the dispatcher reads the owner where it
   is, with no quick look; `IRQ_OWN` and `IRQ_RELEASE` are KCALLs that write all 16 copies.  The dispatcher hands
   the entry the task it interrupted, too.
6. **kcopy bursts are 4 bytes, unrolled** (8 would hold IRQs off for over 200 cycles).  And kcopy is only for a
   partner that can't be in a kcopy of its own (the caller of a call, the task called, the kernel task, a task
   not started): its pointer is in the partner's zero page.  Reading any task's name or state uses quick looks.
7. **SCALL carries C in P** across the switch back, and a task with no serve entry is marked `BUSY_NOSERVE`, so
   the fast path has one check.
8. **Scratch has owners** (docs/conventions.md): the tests found the scheduler's counter in a byte that SPAWN
   and kcopy used too (a preemption in the middle of either changed its count), and WAIT's saved argument in a
   byte SCALL uses.
9. **The emulator** ends an IRQs-off stretch when it takes an interrupt (so masked code and interrupt service
   are measured apart; the receive latency measures them together), and counts each received byte's wait.

Phase 2 added these, from the console driver's interrupts (the dispatch leaves an irq entry about 85 of the 200
cycles):

10. **An irq entry can't afford `WAKE`** (about 75 cycles with its call).  A server adds 1 to its event count
    (`TASK_EVENT`, a byte of its OS zero page) instead; `SRV_TAKE` notes the count, and a client answered
    `E_AGAIN` waits for it to change (point 1).  `WAKE` and srvlib's wait masks still end the same wait.
11. **A note to a group is too long for an irq entry** (`NOTE_POST` looks at 15 tasks).  `NOTE_QUEUE` marks it in
    the kernel task (about 40 cycles), and the kernel task, which takes a turn in the round while it has any,
    posts it with IRQs on.  So the console's Ctrl-C costs the interrupt about what a key does.
12. **VIA timer 2 is a line of its own** (`LINE_VIA_T2`): the VIA's stub sends its interrupt straight to its owner,
    instead of the tick's handler passing it on (a second dispatch, 70 cycles).  Owning the line is owning the
    timer; the VIA's shared registers stay the kernel's.
13. **The dispatcher's main path is in the COMMON block**: it saves the jump to page 0 and back on every
    interrupt (7 cycles).
14. **The console paces its sending by timer 2 at every rate**, on both chips: the ACIA's interrupt only ever
    receives, and timer 2's only ever sends, so neither does both in one interrupt.  Its first byte after a quiet
    spell waits a character's time (the bring-up console may have sent one just before).

## Phase 0: foundations

| Step | | Notes |
|---|---|---|
| 0.1 The tree | Done | `reborn/`; `sdk/c/` and `romfs/` come when there's something to put in them |
| 0.2 Conventions | Done | [conventions.md](conventions.md) |
| 0.3 The specification and `apigen.js` | Done | The calls in 9 groups; the jump table (and the stubs of calls on other pages), `hydra.inc`, `errors.inc`, the error texts, `api.md`, `api.json` |
| 0.4 The build | Done, but the program link | `build.js`: the BIOS link (16 pages, each with its number; COMMON at `$FD00`, the vectors), the module link (`$A000`, data copied to RAM), `romimg.js`, the checks, the budget report.  The RAM program link (`$0800`) comes with loading programs from files (phase 4) |
| 0.5 Emulator additions | Mostly | `sim/run.js`: the new images, the task view, labels by page in traces, PC watches by label, live console.  Still to do: the call trace (by `api.json`) |
| 0.6 CI | Done | `.github/workflows/build.yml` builds and tests reborn too |

## Phase 1: the kernel core

| Step | | Notes |
|---|---|---|
| 1.1 Reset and POST | Done | The reset stub on every page, `T U V W` set; POST on page 4: each T line at four places in a task's memory, shared RAM, the W lines, the paged RAM's U, bank, data and address lines, the RAM modules found; a T typed during it starts the hardware test (`os_rom/hwtest`, unchanged, in paged ROM bank 1, its checksums made for reborn's images) |
| 1.2 The kernel task and the per-task areas | Done | `include/layout.inc`; quick looks in `kernel/kdefs.inc`; the build refuses a module that writes T, V or W (`tools/check.js`) |
| 1.3 Spike S1 | Done | Above |
| 1.4 Spike S2 | Done | Above; in RAM, in a RAM bank and in a shared bank |
| 1.5 Spike S3 | Done | Above; a busy driver, and a call switched out mid-way (one that sleeps) |
| 1.6 Tasks and the scheduler | Done | Frames with `U` and `W`, round robin, `PREEMPT_OFF`/`ON`, the tick, `SLEEP`, `SLEEP_UNTIL`, `YIELD`, `PAUSE`/`WAKE`, CPU time per task (`TASKINFO`) |
| 1.7 The jump table and the first calls | Done | With the polled console (9600, Rockwell or WDC) |
| 1.8 Memory | Done | `BREAK` (the plan's `BRK`: that's the 65C02's instruction), `PAGES_ALLOC`/`FREE`, `BANKS`, `BANKS_ALLOC`/`FREE`, `SEG_CREATE`/`ATTACH`/`DETACH`/`MAP`; the modules and shared chips POST found bad left out; a task's segments released at its end |
| 1.9 Notes, exits and waiting | Done | `NOTIFY`, `NOTE` (to a task or a note group; `SPAWN_NEWGROUP`), handlers that go on or take the default, the defaults (130 Ctrl-C, 137 kill, 133 a BRK), E_INTR from WAIT, PAUSE, SLEEP and GETC; `EXITS`, `WAIT`, orphans to init.  Plan 9's `NOTED` isn't needed: a handler's C says what it would |

## Phase 2: modules, servers and the console

| Step | | Notes |
|---|---|---|
| 2.1 The module framework | Done | Drivers (`HYX2_DRIVER`) started at boot in the directory's order, task F down; the boot waits for their inits (their devices registered) before it starts init |
| 2.2 The request block and the file layer | Done | `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2` (page 2): fds, channels (48, the kernel task's), `#x` names to a device's server; `IO_UNIT` a request; a note ends a wait (`R_FLUSH`, `E_INTR`); fds 0-2 to a child, all closed at the end; `PUTC`, `PUTS`, `GETC` on fds 1 and 0 |
| 2.3 The server calls | Done | `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_POST`, `NOTE_QUEUE`; the event count (`TASK_EVENT`, `RQ_EVENT`, `ST_EVENT`) |
| 2.4 srvlib | Done | `sdk/asm/srvlib.inc`, `srvlib.s`: directories as stat records, text files made on each read, ctl files of commands (words, decimal and `$hex` numbers), data files with a handler, fids, wait masks; the test server `t_srv` |
| 2.5 Namespaces | Done | `kernel/ns.s` (page 3) and the names in `file.s`: 8 namespaces of up to 128 entries in all, the mount points and paths shared strings (64 of 64 bytes); `BIND` (`MREPL`, `MBEFORE`, `MAFTER`, `MCREATE`), `MOUNT` (a device and a spec), `UNMOUNT` (all, or one member), `CHDIR`, `GETCWD`; names made whole and clean (`.`, `..`, `//`); unions tried in order, a `CREATE` to the `MCREATE` member; union directories read whole, on any fd; `SPAWN` shares the parent's namespace (copied when the child changes it) or, with `SPAWN_NEWNS`, starts an empty one; init starts with an empty one.  Binds are resolved when they're made, as in Plan 9 (a mount point bound elsewhere brings all its members), so a name is found in one step: the plan's "a bind rewrites and looks again" isn't needed |
| 2.6 The kernel's devices | Done, but `#e` and most of `/proc` | `modules/kdev`, a boot driver on srvlib, not task 0 (a deviation from the plan's §14.1: the kernel task stays a table keeper): `#/` (the mount points, and in `dev` its own), `#n` (null, zero), `#t` (ticks; the time comes with the clock, phase 5), `#m` (a file a module: `MODINFO`), `#p` (a directory a task: status, and ctl's kill, interrupt, note N), `#|` with `PIPE` (8 pipes of 512 bytes; a read waits, and ends with the last writer; a write waits for room, and is `E_PIPE` with no reader).  srvlib grew several trees a server (`SRV_TREES`), dynamic directories (`SK_DYN`: a handler's children, each a template's), and `R_DUP`.  Still to do: `#e` (the environment: rc needs it, phase 4), `/proc`'s args, cwd, fd, ns, note, mem, regs |
| 2.7 The console driver | Done, but the bell | `modules/cons`: `#c` (`cons`, `consctl`, `ser`, `serctl`); the receive ring from the ACIA's interrupt, the send ring paced by timer 2 (300-19200 and 115200, Rockwell or WDC); cooked lines (Backspace, Delete, Left, Right, Home, End, Ctrl-A, Ctrl-E, Ctrl-U, Up and Down through 8 lines of history, Enter, Ctrl-D), raw keys (the terminal's sequences as `KEY_*`), LF as CR LF out; windows in place of job control (the plan's §14.2, revised: Plan 9's way, no `fg`): 4 windows, each with its own `cons` and `consctl`, line editor and history, raw mode, note group (`group` in its `consctl`) and text (its last 2K of output); `#cN` is window N (a `#` name's spec, as Plan 9's `#I1`); Ctrl-] and a digit shows a window (Ctrl-] `n` the next) and repaints the terminal from its last 24 lines; a window not shown runs on, its output into its text, its reads waiting; Ctrl-C and Ctrl-\ go to the shown window's group; `wctl` (`new`, `current N`), `wnew` (Ctrl-] `c`: a window made and shown, for init's shell starter); a window goes with its last `cons`.  The keys are handed to the windows, and the shown window's text pumped out, by the serve entry before and after each request (srvlib's `SRV_PRE`, `SRV_POST`); a write to the shown window takes what the send ring has room for, so it all goes out before the request ends.  Still to do: the bell (with the sound driver), an Escape alone in raw mode (it waits for the next key) |
| 2.8 A first init | Done | Its fds 0-2 on `#c/cons`; its namespace built in (`#/` at `/`; `#c`, `#n`, `#t` after `#/`'s own at `/dev`; `#m` at `/dev/mod`; `#p` at `/proc`), till the disks bring `/rom/lib/namespace`; its note handler; hello run; then the shells: `tsh` (`ps`, `ls`, `cat`, `cd`, `pwd`, and a module by its name, Ctrl-C ending it) in window 0, and `tsh w`, which starts one in each window the user asks for (Ctrl-] c); each started again when it ends.  A union's first bind keeps what was at old, as in Plan 9, so `ls /dev` shows `#/`'s mount points too |

The kernel's layout on the BIOS ROM: page 0 (4115 bytes, 2029 left below the jump table) has what runs often:
the interrupt path's rare parts and the scheduler, SCALL, kcopy and ROMREAD, the console calls, a task's side of SPAWN,
EXITS and WAIT, the notes; the COMMON block (235 bytes of 255) the interrupt entry and the dispatcher; page 1
(3574 bytes) the kernel task's side of the task calls and the boot, memory, TASKINFO, MODINFO, DBG_PS; page 2
(2771 bytes) files, names and pipes; page 3 (2360 bytes) the namespaces' tables; page 4 (1072 bytes) POST.  A call
on another page goes through a 6-byte stub on page 0 (about 90 cycles more), and the kernel's own far calls
through the COMMON block (`FARCALL`).

## Phase 3: storage

| Step | | Notes |
|---|---|---|
| 3.1 SPI and `#S` | Done | `modules/storage`, a boot driver on srvlib (task E, as the plan has it: `kdev` is task D now): the old bit loops (`drivers/spi.s`) unchanged, the receive loop padded at 7.16 MHz; `#S/N/data` (a write is a transaction of up to 256 bytes, the bytes that come back kept; a read gives the kept bytes, or clocks in new ones) and `#S/N/ctl` (`mode 0`, `mode 3`).  A directory a device, with `data` and `ctl` in it, as `#d` has (the old `/dev/spi/N` was the data file itself, with its `ctl` under it: a file that was a directory too) |
| 3.2 The block layer | Done, but the RAM disks at boot | SD cards (the old `drivers/sd.s`, ported: SDHC and SDXC; SDSC, CSD v1 too), the ROM disk `x` (the paged ROM's 256 banks as 8192 blocks, read through the kernel's new `ROMREAD`, as the plan says: the driver runs in place in its own bank, so a routine on page 0 selects the block's bank, copies it and puts the driver's back; a task that owns an IRQ line can't use it), the RAM disks `r` (the driver's own banks: `BANKS_ALLOC`) and `s` (a shared segment: `SEG_CREATE`, each block's bank found by `SEG_MAP`), started by `start SIZE` (8K banks, or `K`, `M`) and stopped by `stop`; one block buffer.  A card started isn't an `#S` device, nor the other way round (`E_BUSY`).  Still to do: the RAM disks started (and formatted) at boot, with HydraFS; `start`'s FROM-TO (`BANKS_ALLOC` and `SEG_CREATE` take the lowest banks free) |
| 3.3 `#d` | Done, but HydraFS's ctl commands | A directory for each disk started (`x` from the boot; a card from its first open): `data`, the disk as bytes at the fd's offset (its first 4 GB; a read is short at the end, a write past it is `E_NOSPC`; writes go to the disk at once), and `ctl`, which reads as the disk (`sdhc 1 MB 2048 blocks`; `sdsc`, `rom`; `ram` and `sram` in KB; or `none`) and takes `init`, `start` and `stop`.  `format`, `label` and `check` come with HydraFS |
| 3.4 HydraFS (`#f`) | Next | |
| 3.5 The ROM disk image | | |
| 3.6 init from files | | |

## Next

1. Phase 3.4: HydraFS (`#f`), ported into the storage module.  With it the module outgrows its 16K bank, so
   modules of more than one bank come first.  Then the ROM disk's image (3.5) and init from files (3.6).
2. Phase 4: rc, with what it needs here first: `#e` and the rest of `/proc`; SPAWN of a program by its path
   (`/bin/NAME` through the namespace).
3. On the board: the boot, POST, the tick, the console at 115200, and a real card read and written.
4. Still open from phase 0: the emulator's call trace.
