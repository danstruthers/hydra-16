# Status

Where the rebuild stands against the plan's phases
([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md), Part III), what the
spikes measured, and what measuring changed.

## In short

Phases 0, 1 and 3 (storage) are done, phase 2 is all but done, and phase 4 (programs) has its loader, rc, the core tools and the assembly SDK.  The kernel boots in the emulator, runs POST (with the old
hardware test a key away), starts its modules from the paged ROM in tasks of their own, schedules them
preemptively, runs calls between tasks and copies between them, takes every interrupt through one path, manages
task RAM, banks and shared segments, and delivers notes.  The file layer is in: fds, channels, requests to
servers, servers built on srvlib, waits that a server's interrupt can end for a few cycles; and namespaces, Plan 9's:
binds, mounts, unions and union directories, a current directory, shared by a task's children till they change
them.  The console driver (`cons`, task F) serves `#c`: windows, Plan 9's way (rio's, on the serial terminal: a
console each, shown with Ctrl-] and a digit, repainted from its text), lines edited at the console, raw keys,
Ctrl-C and Ctrl-\ to the shown window's note group, and paced sending up to 115200.  The kernel's own devices are a driver of
their own (`kdev`): the root's mount points, null and zero, the ticks, the modules, the tasks, pipes, and the
environment.  The storage driver (`storage`, task E) owns the SPI bus and the disks: `#S` (the SPI devices), `#d`
(SD cards, through a cache of their blocks; the ROM disk; the RAM disks), and HydraFS on them (`#f`, the old
system's, ported: the module's second bank).  The paged ROM holds the ROM disk's volume after the modules (`/rom`:
`romfs/`).  init starts the RAM disks, builds its namespace from `/rom/lib/namespace`, and runs rc in window 0 and
in each window the user asks for (Ctrl-] c); each rc builds its own namespace from the same file as it starts
(Plan 9's `newns`), with its own area of the RAM disk at `/ram`, then runs `/rom/lib/profile`.  `SPAWN` takes a
path through the namespace: a module of the paged ROM (`#m`'s files, at `/bin` by `#m/bin`) runs in place, and a
program in a file (a RAM program: `sdk/asm/hyx2.cfg`) is read into its task's RAM at `$0800` by the task itself as
it starts.  rc is Plan 9's (lists, quoting, `if`, `for`, `while`, `switch`, functions, redirections, pipelines,
background tasks, `$status`, globbing, `rc -c`), with the core tools beside it: files (`ls -l`, `cp -r`, `mv`,
`rm -r`, `du`, `df` ...), text (`wc`, `head`, `tail`, `uniq`, `xd`, `more` ...), tasks (`ps`, `kill`, `top`, `ns`
...), and the disks' (`mkfs`, `fsck`, `label`: RAM programs on the ROM disk, at `/bin` too).

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
PASS dev     the kernel's devices (kdev): #/, #n, #t, #m, #p; pipes; a union keeping what was there  (42 checks)
PASS spi     SPI and #S (storage): transactions, kept bytes, modes 0 and 3, one open at a time, the time a byte takes  (43 checks)
PASS disk    the disks (storage): #d, the ROM disk, SD cards (SDHC and SDSC), RAM disks, their ctl files, the time a byte takes  (61 checks)
PASS fs      HydraFS (#f): files and directories, create, write, holes, remove, rename, format, label, check, old cards, mounts  (82 checks)
PASS rom     the ROM disk: /rom (#f, spec x) walked on the Hydra, every file read back against its source (romfs/romfs.txt)
PASS load    SPAWN by path and the loader: modules in place (#m/bin), RAM programs from a card (arguments, fd maps), errors  (49 checks)
PASS env     environments: ENV_GET, ENV_PUT, ENV_DEL, ENV_NAME, a child's copy, #e (/env) as files  (52 checks)
PASS rc      rc: quoting, lists, redirections, pipelines, if, for, while, switch, functions, globs, scripts, Ctrl-C, its start  (3 checks)
PASS tools   the core tools at rc: files, text, tasks, the disks' (/rom/bin); /proc's args, cwd, ns  (3 checks)
PASS init    init from files: the RAM disks started, the namespace file run, each shell's own namespace and /ram (a window's too)
PASS newns   the default namespace's library (nslib): an old area emptied, a namespace file run (quotes, comments, $task, flags, bad lines)  (13 checks)
PASS cons    the console: lines, editing, history, raw keys, Ctrl-C, windows (shown, repainted, made, gone), 115200
PASS mem     memory: BREAK, pages, banks, a shared segment between tasks (and kcopy from it)  (38 checks)
PASS banks   a module of two banks: calls between them (FAR2, FAR1), registers and C, each bank's data  (6 checks)
PASS scall   spike S3: calls into a driver's task, its errors, a busy driver, the round trip  (12 checks)
PASS kcopy   spike S2: copying between tasks  (6 checks)
PASS irq     spike S1: 115200 received by an irq entry while tasks spin  (6 checks)
```

The same with the power-up's RAM from other seeds; the console, file, namespace and device tests the same with a
WDC W65C51N build, the console test with a 7.16 MHz build, and the SPI, disk, file system, init, load, env, rc and
tools tests with both.  The hardware test, entered from POST in the emulator, passes its whole quick run, its BIOS and
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
| Reading a card | The old system's 298 cycles a byte | **257 cycles a byte** (about 14 KB/s at 3.58 MHz): 4096 bytes in 512-byte reads of `#d/0/data`; a RAM disk **61**.  With the cards' block cache (4.2): **279** the first time (each block kept too), **65** again | The disk test, less the marks' own time |
| Reading a HydraFS file | The card's | **260 cycles a byte**: 8192 bytes of a file on a card, in 512-byte reads; the file system costs next to nothing over the card | The fs test |
| Loading a program | (Phase 4's load-time budgets) | A RAM program read from a card **295 cycles a byte** (16K: `SPAWN` to its first instruction; the card's 257, and the file system's), from the RAM disk **80**.  `SPAWN` of a module in place (`#m/t_child`) **50,000 cycles** (14 ms) to the caller's return, with 40 modules (90,000 when kdev looked each module up through the kernel: 4.3); the same by `/bin/t_child` through a card's `bin` first, **106,000** (30 ms; 400,000 before the cards' block cache: each look read the card's directories again) | The load test, less the marks' own time, with preemption off (the child would run first, now and then) |
| Starting rc; `ls /bin` | (Phase 4's load-time budgets) | `rc -c 'x=1'` from `SPAWN` to its end, **115,000 cycles** (32 ms; 342,000 when the kernel task cleared a module's BSS a byte at a time); `ls /bin` through its union (the RAM disks' empty caches, `/rom/bin`, then `#m/bin`: 36 programs) to `#n/null`, **760,000**, 21,000 a program (212 ms: 107,000 of it HydraFS reading 15 blocks of the RAM disks to open the two caches, 50,000 the ROM disk's bin; about 13,000 more a program.  With phase 4.2's 12 programs it was 511,000, and 708,000 before `#m/bin`'s listing stopped searching the module directory from the start for each name) | The rc test, before rc starts on the console |
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
| 0.1 The tree | Done | `reborn/`; `romfs/` came with the ROM disk (3.5); `sdk/c/` comes with the C target (4.5) |
| 0.2 Conventions | Done | [conventions.md](conventions.md) |
| 0.3 The specification and `apigen.js` | Done | The calls in 9 groups; the jump table (and the stubs of calls on other pages), `hydra.inc`, `errors.inc`, the error texts, `api.md`, `api.json` |
| 0.4 The build | Done | `build.js`: the BIOS link (16 pages, each with its number; COMMON at `$FD00`, the vectors), the module link (`$A000`, data copied to RAM), `romimg.js`, the checks, the budget report.  The RAM program link (`sdk/asm/hyx2.cfg`, `$0800`) came with the loader (4.1): the test RAM programs (`tests/ram/`) are built with it |
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
| 2.6 The kernel's devices | Done, but some of `/proc` | `modules/kdev`, a boot driver on srvlib, not task 0 (a deviation from the plan's §14.1: the kernel task stays a table keeper): `#/` (the mount points, and in `dev` its own), `#n` (null, zero), `#t` (ticks; the time comes with the clock, phase 5), `#m` (a file a module, read as its image, its header first: what `SPAWN` reads; `bin`, the programs alone), `#p` (a directory a task: status, and ctl's kill, interrupt, note N), `#|` with `PIPE` (8 pipes of 512 bytes; a read waits, and ends with the last writer; a write waits for room, and is `E_PIPE` with no reader).  srvlib grew several trees a server (`SRV_TREES`), dynamic directories (`SK_DYN`: a handler's children, each a template's, after the directory's own entries: `#m`'s `bin`), `R_DUP`, and `SRV_STAT` (a server's say in a stat record: a module's length).  `#e` came with rc (4.2): the caller's environment, a file a variable (a raw device, `SK_RAW`: open, create, read and write at offsets, remove, a directory of the names), the kernel keeping each task's.  `/proc`'s args, cwd and ns came with the tools (4.3).  Still to do: `/proc`'s fd, note, mem, regs |
| 2.7 The console driver | Done, but the bell | `modules/cons`: `#c` (`cons`, `consctl`, `ser`, `serctl`); the receive ring from the ACIA's interrupt, the send ring paced by timer 2 (300-19200 and 115200, Rockwell or WDC); cooked lines (Backspace, Delete, Left, Right, Home, End, Ctrl-A, Ctrl-E, Ctrl-U, Up and Down through 8 lines of history, Enter, Ctrl-D), raw keys (the terminal's sequences as `KEY_*`), LF as CR LF out; windows in place of job control (the plan's §14.2, revised: Plan 9's way, no `fg`): 4 windows, each with its own `cons` and `consctl`, line editor and history, raw mode, note group (`group` in its `consctl`) and text (its last 2K of output); `#cN` is window N (a `#` name's spec, as Plan 9's `#I1`); Ctrl-] and a digit shows a window (Ctrl-] `n` the next) and repaints the terminal from its last 24 lines; a window not shown runs on, its output into its text, its reads waiting; Ctrl-C and Ctrl-\ go to the shown window's group; `wctl` (`new`, `current N`), `wnew` (Ctrl-] `c`: a window made and shown, for init's shell starter); a window goes with its last `cons`.  The keys are handed to the windows, and the shown window's text pumped out, by the serve entry before and after each request (srvlib's `SRV_PRE`, `SRV_POST`); a write to the shown window takes what the send ring has room for, so it all goes out before the request ends.  Still to do: the bell (with the sound driver), an Escape alone in raw mode (it waits for the next key) |
| 2.8 A first init | Done | Its fds 0-2 on `#c/cons`; its namespace built in (`#/` at `/`; `#c`, `#n`, `#t` after `#/`'s own at `/dev`; `#m` at `/dev/mod`; `#p` at `/proc`), till the disks brought `/rom/lib/namespace` (3.6: now its fallback); its note handler; hello run; then the shells: `tsh` (`ps`, `ls`, `cat`, `cd`, `pwd`, and a module by its name, Ctrl-C ending it) in window 0, and `tsh w`, which starts one in each window the user asks for (Ctrl-] c); each started again when it ends.  A union's first bind keeps what was at old, as in Plan 9, so `ls /dev` shows `#/`'s mount points too.  rc took tsh's place (4.2): init runs `rc -l` in window 0, and `wstart` for the other windows |

The kernel's layout on the BIOS ROM: page 0 (4068 bytes, 2076 left below the jump table) has what runs often:
the interrupt path's rare parts and the scheduler, SCALL, kcopy and ROMREAD, the console calls, a task's side of
EXITS and WAIT and its first instructions, the notes; the COMMON block (235 bytes of 255) the interrupt entry and
the dispatcher; page 1 (4017 bytes) the kernel task's side of the task calls and the boot, a module's data and BSS
set up as it starts, memory, TASKINFO, TASKREAD, MODINFO, DBG_PS; page 2 (4241 bytes) files, names, pipes and the
environments (4.2); page 3 (3425 bytes) the namespaces' tables and NSINFO, and SPAWN's side and the loader (4.1);
page 4 (1072 bytes) POST.  A call
on another page goes through a 6-byte stub on page 0 (about 90 cycles more), and the kernel's own far calls
through the COMMON block (`FARCALL`).

## Phase 3: storage

| Step | | Notes |
|---|---|---|
| 3.1 SPI and `#S` | Done | `modules/storage`, a boot driver on srvlib (task E, as the plan has it: `kdev` is task D now): the old bit loops (`drivers/spi.s`) unchanged, the receive loop padded at 7.16 MHz; `#S/N/data` (a write is a transaction of up to 256 bytes, the bytes that come back kept; a read gives the kept bytes, or clocks in new ones) and `#S/N/ctl` (`mode 0`, `mode 3`).  A directory a device, with `data` and `ctl` in it, as `#d` has (the old `/dev/spi/N` was the data file itself, with its `ctl` under it: a file that was a directory too) |
| 3.2 The block layer | Done, but the RAM disks at boot | SD cards (the old `drivers/sd.s`, ported: SDHC and SDXC; SDSC, CSD v1 too), the ROM disk `x` (the paged ROM's 256 banks as 8192 blocks, read through the kernel's new `ROMREAD`, as the plan says: the driver runs in place in its own bank, so a routine on page 0 selects the block's bank, copies it and puts the driver's back; a task that owns an IRQ line can't use it), the RAM disks `r` (the driver's own banks: `BANKS_ALLOC`) and `s` (a shared segment: `SEG_CREATE`, each block's bank found by `SEG_MAP`), started by `start SIZE` (8K banks, or `K`, `M`) and stopped by `stop`; one block buffer.  A card started isn't an `#S` device, nor the other way round (`E_BUSY`).  Still to do: the RAM disks started (and formatted) at boot, with HydraFS; `start`'s FROM-TO (`BANKS_ALLOC` and `SEG_CREATE` take the lowest banks free) |
| 3.3 `#d` | Done, but HydraFS's ctl commands | A directory for each disk started (`x` from the boot; a card from its first open): `data`, the disk as bytes at the fd's offset (its first 4 GB; a read is short at the end, a write past it is `E_NOSPC`; writes go to the disk at once), and `ctl`, which reads as the disk (`sdhc 1 MB 2048 blocks`; `sdsc`, `rom`; `ram` and `sram` in KB; or `none`) and takes `init`, `start` and `stop`.  `format`, `label` and `check` come with HydraFS |
| 3.4 HydraFS (`#f`) | Done, but the progress line and the clock | `modules/storage/hfs.s` and `hfs/*.s`: the old `fs/hfs_*.s` ported into the module's second bank (the first module of two banks: `FAR2` and `FAR1`, trampolines in its RAM; `modules/module2.cfg`).  The format is unchanged (versions 1 and 2, partitions, sparse files): walks, reads, writes, create, remove, wstat (a name; read-only, append-only), quick and full format, label, check and fix, 32 open files (the old 8).  `#f` with no spec is the cards' directory (`0`-`f`, those started); a spec names one disk (`x`, `r`, `s`, or a card) or a directory on it (`r/5`), so the disks in memory are reached only through a spec, and a shell's area of the RAM disk is a matter for its namespace, not the server (the old per-task areas and their checks are gone).  Directories read as stat records (Plan 9's way: the old text listing is gone); names come cleaned by the kernel (no `.` or `..`); errors are reborn's codes (`E_NOTFS`, `E_NOSPC` ...).  A RAM disk's `start` puts an empty HydraFS on it.  srvlib grew raw devices (`SK_RAW`: every request for a device to one handler, for a file system) and a ctl command's last word taking the rest of its line (a label with spaces).  Still to do: the progress a long full format or check shows (the old system printed `10% 20% ...`; a driver has no console of its own), stamps from the clock (a counter till phase 5) |
| 3.5 The ROM disk image | Done | `romfs/`: `README`, `lib/namespace` (the plan's appendix E, the devices still to come commented out with their phases), `lib/profile` (for rc), and `doc/api.md` (the calls' reference, made at the build); `romfs.txt`, the manifest.  `tools/romfs.js` (from the old `mkromdisk.js`) makes the HydraFS volume with the PC tool, stamped 2000-01-01, so each build is the same; `romimg.js` puts it in the banks after the modules, with the partition table in block 0 (after the signature line: partition 1 the system's banks, type `$DA`; partition 2 the volume, type `$7F`, to the paged ROM's end), and reads every file back from the image as the CPU sees it; every test's image has it too.  On the Hydra, the rom test walks `/rom` and reads every file, and the harness checks each one's size and CRC against its source.  The files are bytes for the Hydra (LF line ends: `.gitattributes` keeps Git from changing them), so the image is the same on every checkout |
| 3.6 init from files | Done, but the drivers file | `sdk/asm/nslib.s`, Plan 9's `newns` as a library (init and the shells include it, as servers do srvlib): `ns_default` makes the task's own area of the RAM disk (`#fr/N`, emptied if a task before it with that number left one, with `bin` and `lib` in it: its caches), then runs `/rom/lib/namespace` (read as `#fx/lib/namespace`) and a card's (`/sd/0/lib/namespace`, if it has one), with `$task` as the task: `bind` and `mount`, their flags (`-a`, `-b`, `-c`), quotes, comments; a line that fails is said, but for what isn't there (`E_NOENT`, `E_NODEV`, `E_NOTFS`: a card's `bin`, say), as Plan 9's is quiet about it.  init starts the RAM disks (`r` 256K, `s` 512K, each halved till it fits; `s`'s `bin` and `lib`, the shared caches), runs `ns_default` (its built-in namespace only when there's no `/rom/lib/namespace`), and starts its shells with empty namespaces (`SPAWN_NEWNS`): each runs `ns_default` for itself, so its `$task`, its `/ram` and its caches are its own (the plan had init run the file for them: but binds are resolved as they're made, so a shell sharing init's would have init's `/ram`).  `#m/bin` is bound at `/bin` since the loader (4.1); `/rom/bin` waits for programs on the ROM disk (4.3).  The profile came with rc (4.2: `rc -l` runs it, after `newns`).  Still to do: `/rom/lib/drivers` (with the first driver started after the boot: `snd`, phase 5) |

## Phase 4: programs

| Step | | Notes |
|---|---|---|
| 4.1 The loader and `SPAWN` | Done | `kernel/load.s` (page 3): `SPAWN` opens the program's file by its path, through the caller's namespace, and reads its HYX2 header (`HX_*`, now in `hydra.inc` too).  A module of the paged ROM (`HF_INPLACE`) runs in place: the kernel task finds it in the module directory by the header's name and sets it up as at boot.  Any other program is a RAM program: its task gets the file as its fd 15 (`LOAD_FD`) and starts at `K_TASK_LOAD`, which loads it there (`K_LOAD`: the header checked again, the image read to its load address, the file closed, the BSS cleared, the entry and break set), then starts it as every program; so `SPAWN` returns without waiting for the image, and the file's server copies it straight into the child's RAM.  One that can't be loaded ends at once, its code the error (`E_NOEXEC`: a file that ends too soon).  The header is checked for what this kernel runs: `HYX2`, an ABI not later than its own, a program; a RAM program's image from `$0800`, its data where it loads, its BSS from `$0400`, all below its top, and its top below its task's RAM's (task F's stops at `$7F00`).  The fds: the caller's 0, 1 and 2, or with `SPAWN_FDMAP` a map in `r2` (a count, then the caller's fd for each of the child's; a flag, not `r2` = 0, so a caller's stray `r2` can't hand a child fds).  `#m/NAME` reads as the module's image (kdev, by `ROMREAD`), its length in its stat record, and `#m/bin` lists the programs alone, bound after the caches at `/bin` (`romfs/lib/namespace`); tsh (then rc) runs `/bin/NAME`, or a path.  The SDK: `hyx2.inc` with `-D HYX2_RAM` and `sdk/asm/hyx2.cfg` link a RAM program.  The plan's argument list (strings ending with an empty one, for `argc`/`argv`) came with rc (4.2) |
| 4.2 rc | Done | `modules/rc`, a program of two banks (`lex.s` and `parse.s` in the second): Plan 9's rc, as the plan's §15.2 has it.  A line is lexed and parsed into a tree in an arena, given back after each command.  Words are lists, made by concatenation (`^`, and Plan 9's free carets), `$x`, `$#x`, `$x(n)`, `$x(n-m)`, `$"x` and `` `{...} ``, then globbed (`*`, `?`, `[...]`, by reading directories; sorted).  Variables and functions are kept in a heap and written to the environment before a program starts (a list's words each with a 0 after it, a function as `fn#NAME`), and read from it as rc starts (`$path` (. /bin), `$prompt` ('% ' and a tab) and `$task` if it hasn't them).  `if`, `if not`, `for`, `while`, `switch` and `case`, `fn`, `~`, `!`, `&&`, `||`, `{ }`; `<`, `>`, `>>`, `>[n]`, `>[n=m]` (rc moves its own fds and puts them back, keeping what it moves above fd 9); pipelines (`|`, `|[n]`: each stage a program, or `rc -c` for one that isn't); `&`, `$apid` and `wait`; `$status` (a program's exit message, or its code; true is empty or 0); scripts (`rc file args`: `$0`, `$*`), `.`, `eval`, `rc -c`, `exit`; the built-ins `cd`, `bind`, `mount`, `unmount`, `newns`, `builtin`, `whatis` (Plan 9's: a word quoted only if it needs it) and `shift`.  On the console: its line editing, the second prompt for a command that goes on, Ctrl-C back to the prompt (the note goes to the window's group: the program running ends, and rc goes on).  `rc -l` runs `newns` and `/rom/lib/profile` first.  With it: environments (`kernel/env.s`, page 2: `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, any task's by number; 1K a task in the kernel task's RAM from `$3000`, copied by `SPAWN`, `SPAWN_NOENV` for an empty one) and `#e` at `/env`; `SPAWN`'s argument list and the caller's current directory for the child; the first tools, modules: `echo`, `cat`, `ls`, `ps`, `pwd` (a failed write is said, and `write error` is their status, as Plan 9's); `wstart`.  init runs `rc -l` in window 0 and `wstart` for the others, and `tsh` is gone.  Measuring changed two things: a module's data and BSS are set up by its own task as it starts (`K_TASK_DATA`), not by the kernel task from outside a byte at a time (rc's start, `rc -c`, 342,000 cycles to 124,000; the boot test's run 6.6 million to 4.8, the drivers having 36K of BSS); and `#m/bin`'s listing goes on from the name before instead of from the directory's start (`ls /bin` 708,000 to 511,000).  Also the storage driver's cache of the cards' blocks (`storage.s`: `l2_*`): 16 blocks (8K of its RAM, not a bank: a bank taken would halve the RAM disk at boot) under `blk_read` and `blk_write`, written through, so it always holds what's on the card (and `blk`, the block being worked on, is above it as before).  A block used again is hot: a new block takes a free slot, else the oldest cold one's, else the oldest hot one's (12 hot at most), so a file read through doesn't push the directories out.  A card started (or started again by its ctl's `init`) or stopped has nothing kept.  A card's block read costs about 16 cycles a byte more the first time (it's kept), and 65 a byte from then on (279 from the card).  (The rest of `/proc`: 4.3, args, cwd and ns) |
| 4.3 Core tools | Done, but `grep`, `sort`, `date` and `hwtest` | `sdk/asm/toollib.s` (with `toollib.inc` for its zero page), what the tools share, included as nslib is: flags (`-abc`) and arguments, the usage, output 256 bytes at a time with a failed write said (`write error` the status), errors as Plan 9's (`ls: name: why`), decimal numbers, dates, paths, a directory read whole past the break, a depth-first walk of a tree (`tl_walk`), and input a file (or fd 0) at a time.  The tools are modules, a bank each (ROM space is plentiful): files `ls` (`-l`, `-d`), `cat`, `cp` (`-r`; not onto itself), `mv` (a rename by `WSTAT` in its own directory, else copied and removed: a directory can't move to another, as in Plan 9), `rm` (`-r`, `-f`), `mkdir` (`-p`), `rmdir`, `touch` (`WSTAT`: a new stamp), `du` (`-a`), `df` (from `#d`'s ctl files); text `echo`, `wc`, `head`, `tail`, `tee` (`-a`), `uniq` (`-c`), `xd`, `cmp`, `more`; system `ps` (`-a`: the arguments), `kill` (`-i`: interrupted), `slay`, `top`, `sleep`, `ns`, `mods`, `free`.  The disk tools are RAM programs on the ROM disk, `/rom/bin` (from `programs/`; bound at `/bin` after the caches): `mkfs`, `fsck`, `label`, each a few ctl writes.  With them: `/proc/N/args`, `cwd` and `ns` (kdev's; `ns` is the binds and mounts that make the namespace, Plan 9's way, in the order they were made: a data file of kdev's own, as srvlib's text files stop at 255 bytes), from new calls: `TASKREAD` (a task's arguments or current directory: the kernel task reads its OS area with absolute indexed loads, a byte per IRQs-off moment) and `NSINFO` (a namespace's mount entries); and `SEGINFO` (the shared RAM, for `free`) and `DM_APPEND`.  The module directory holds 127 modules (31 before: `$A200`-`$A9FF` of bank 0 now).  Measuring and testing changed three things: kdev keeps its own copy of the module directory, read at init (looking a module up through the kernel each time made `SPAWN` and `ls /bin` grow with every module: `SPAWN` of `#m/t_child` 89,900 cycles to 45,000); SYSINFO held IRQs off over its whole 16-task loop (478 cycles: a look at a time now, found by `free`); and init past entry 15 of the directory started the wrong module (its entry's offset in 8 bits).  Still to do: `grep` and `sort` (in C, 4.5, as the plan has them), `date` (with the clock, phase 5), `hwtest` (a reset into the hardware test), `/proc`'s fd, note, mem and regs |
| 4.4 The assembly SDK | Done | `sdk/asm`: `hydra.inc` (made from the specification), `hyx2.inc` and `hyx2.cfg` (4.1), `macros.inc`, the libraries (`srvlib`, `nslib`, `toollib`), and now a guide, `README.md` (a program, its arguments, calls and errors, its memory, notes and environment; building it; running it), and samples, `samples/`: `hi` (its arguments, `GETPID`, `GETCWD`, `ENV_GET`), `upper` (a filter on toollib) and `tick` (a note handler), built with the system and put on the ROM disk at `/rom/sample` (the tools test runs them).  `node build.js prog DIR` builds a program of one's own from any folder; the build copies the SDK whole to `bin/sdk/asm`, with the generated `hydra.inc`, to build with ca65 and ld65 alone; and `sim/run.js --sd FILE` puts a card image in the emulator (made with `hydrafs.js`, read and written in place), so a program goes from the PC to the Hydra and runs |
| 4.5 The C target and library | | |
| 4.6 edit | | |

## Next

1. Phase 4.5: the C target and its library (the plan's §15.4), the samples ported, `ctest`; `grep` and `sort` in
   C.
2. HydraFS's clusters are 4K (8 blocks), as the old system's were for cards, so a RAM disk holds few files (a 64K
   one, 15 files and directories); its superblock has the cluster's size, so a RAM disk could have smaller ones.
3. `ls /bin` takes about 13,000 cycles more for each program in `#m/bin` (760,000 for 36): kdev's records, made
   and copied one at a time, and the union directory's copy.
4. On the board: the boot, POST, the tick, the console at 115200, and a real card read and written.
5. Still open from phase 0: the emulator's call trace.
