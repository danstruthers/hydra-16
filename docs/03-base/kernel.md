# The kernel

How the kernel works: where it lives, how it runs tasks, handles interrupts, gives out memory, and lets tasks work
together.  Each section names its source in `base/kernel/`; the design behind all of it is in
`docs/design/reimplementation-from-scratch.md`.

## Contents

1. [The kernel's principles](#the-kernels-principles)
2. [The BIOS ROM's pages](#the-bios-roms-pages)
3. [A task's memory](#a-tasks-memory)
4. [Tasks](#tasks): states, the scheduler, starting and ending, programs and drivers
5. [Interrupts](#interrupts)
6. [Reaching other tasks](#reaching-other-tasks): SCALL, KCALL, kcopy, quick looks
7. [Memory](#memory): the break, pages, banks, shared segments
8. [Semaphores](#semaphores)
9. [Notes](#notes)
10. [Time](#time)
11. [Debugging support](#debugging-support)

---

## The kernel's principles

* **The kernel is the BIOS ROM, and nothing else.**  Drivers, even the kernel's own devices, are modules in the paged
  ROM running in tasks of their own.
* **Every task is kept apart by the hardware.**  A task's `$0000-$7FFF` is its own; it reaches another task's memory
  only through the kernel.
* **Nothing outside the kernel changes `T`, `V` or `W`.**  Programs run on BIOS page 0 (`W = 0`) always.
  (`base/tools/check.js` checks every module for writes to them.)
* **The kernel task is never preempted.**  Its work (a KCALL) runs to its end, so the kernel's tables need no locks.
* **A server never waits.**  When it can't answer yet, it says so, and the kernel makes the client wait
  ([files](files.md#waiting)).
* **Interrupts are never off for long**: no stretch of code anywhere holds them off for more than 200 cycles, since a
  character arrives every 320 cycles at 115200 baud.
* **Every call answers the same way**: C = 0 success, C = 1 failure with the error in `.A`.
* **One specification** (`base/spec/api.def`) makes the jump table, the SDK's includes, the languages' bindings and the
  reference.

---

## The BIOS ROM's pages
*`bios.cfg`, `common.s`, `far.s`*

The kernel is 128K: 16 pages of 8K, all at `$E000-$FFFF`, chosen by `W`.

| Page | What's there |
| :--- | :----------- |
| 0 | What runs often: the interrupt path, the scheduler, SCALL and kcopy, the waiting side of the calls, the bring-up console, and the **jump table** at `$F800` (192 slots of 3 bytes) |
| 1 | Tasks and memory: the kernel task's side of the task calls (setting a task up, the boot's drivers), memory, notes, `TASKINFO` |
| 2 | Files, environments, pipes |
| 3 | Namespaces, `SPAWN` and the loader |
| 4 | POST |
| 5-15 | Empty: room to grow |

On **every** page:

* `$E000`: the reset stub ([the boot](boot.md#the-reset-stub)).
* `$FD00-$FDFE`: the **COMMON block**, the same bytes on every page: the interrupt entry and exit, and `K_FAR`, the
  kernel's far call between pages.  Since changing `W` changes the code under the CPU, every page change happens in here.
* `$FDFE`: the serial chip the BIOS is built for (`BIOS_ACIA`: 0 a Rockwell R65C51, 1 a WDC W65C51N), which the
  paged ROM's serial drivers read.
* `$FDFF`: the page's number (POST reads it).
* `$FEFE`: two bytes that make the page's checksum (the hardware test's CRC of `$E000-$FEFF`) 0, set by the build.
* `$FFFA`: the vectors.

Page 0 is the scarce one (about 1.5K free); the others have plenty.  A system call whose routine is on another page is
marked `far` in `api.def`: its jump table slot goes to a 6-byte stub on page 0 that makes the far call, which costs
about 100 cycles.  `node tools/budget.js` shows each page's use.

---

## A task's memory
*`base/include/layout.inc`*

Every task has its own `$0000-$7FFF`, laid out the same way:

| Where | What |
| :---- | :--- |
| `$00`, `$01` | Its bank registers: the RAM bank at `$8000`, the paged ROM bank at `$A000` |
| `$02-$21` | `r0`-`r15`, the **call registers**: two bytes each, low first |
| `$22-$7F` | **The program's own zero page**: no call ever touches it |
| `$80-$FF` | The OS zero page: the task's state for the kernel (`TK_*`), the far call's and kcopy's pointers, the call stubs' scratch.  One byte is the program's to write: its event count, `TASK_EVENT` (`$BD`), for servers |
| `$0100-$01FF` | Its stack.  A task that isn't running has its saved frame on top |
| `$0200-$03FF` | The OS area (`TA_*`): a server's request being served (`TASK_INBOX`), its entries (serve, irq, note handler), its name, its copy of the interrupt lines' owners, its page and bank maps, the request it's making, its fds, its current directory, its arguments (`TASK_ARGS`, `$0350`) |
| `$0400-$7FFF` | **The program's**: its data and BSS, its break, and pages given out from the top down.  A RAM program loads at `$0800` |
| `$8000-$9FFF` | The RAM bank `$00` selects: one of the task's own, or a shared one |
| `$A000-$DFFF` | The paged ROM bank `$01` selects: a module running in place has its bank here (its header at `$A000`, or at its page of a bank it shares) |
| `$E000-$FFFF` | The BIOS ROM, page 0: the jump table at `$F800`; the I/O at `$FF00` |

In task F, the top of RAM is `$7EFF`: the DS1747's clock registers may be at `$7FF8-$7FFF`.  The **kernel task** (task 0)
keeps the kernel's state: its zero page (`K0_*`) and its RAM from `$0400` (the `K_*` tables: tasks, channels, devices,
namespaces, semaphores, segments), and its own RAM banks (the tasks' environments).  Every fixed address is in
`layout.inc`, and nowhere else.

---

## Tasks
*`sched.s`, `task.s`, `load.s`*

### States

A task is in one state at a time (`TK_STATE`):

| State | Meaning |
| :---- | :------ |
| `FREE` | Not in use |
| `NEW` | Set up, not yet run |
| `READY` | Runnable: the only state the scheduler runs |
| `WAIT` | Waiting for a `WAKE` |
| `SLEEP` | Waiting for a tick count |
| `CALL` | Calling another task (a server), which runs the call in its own task |
| `BLOCKED` | Waiting to call a task that's busy with another call |
| `EVENT` | Waiting for a server's event count to change (a read with nothing to read yet) |
| `IDLE` | A driver between calls |

A task that's **stopped** (by the debugger, or `/proc/N/ctl`'s `stop`) keeps its state but isn't run.

**Waiting is always done the same way**: set the state, `YIELD`, and look again when back.  A wake for nothing costs a
look.  A wake that comes before the wait makes the wait return at once, so nothing is lost between deciding to wait and
waiting.

### The scheduler

* **A task that isn't running** keeps one frame on its own stack, whatever stopped it (an interrupt, `YIELD`, a wait):
  `U Y W X A P PCL PCH`.  A switch saves the stack pointer, picks the next task, writes `T`, loads that task's stack
  pointer, and unwinds its frame.  No memory is copied.
* **Round robin** over tasks 1-15: each `READY` task in turn.  A task runs until it waits, yields, or the tick takes the
  CPU away: **preemption**, 200 times a second.
* **The scheduler ends three kinds of wait itself**, as it looks for the next task: `SLEEP` once the time has come (so
  the tick's interrupt only counts), `BLOCKED` once the task it wants to call is free, and `EVENT` once the server's
  event count has changed (so an interrupt handler wakes its clients by adding 1 to a byte).
* **The foreground** (`TASKFORE`, opt-in): a program can put itself, or a task it started, in the foreground
  (`TF_FORE`).  The scheduler picks a foreground task that can run before the round robin's, at the next switch (a tick
  at most) rather than after a round of every busy task, and a driver serving a foreground task's call counts as one
  with it, and for `FORE_SRV` (2) ticks after the call ends (the work the call left it, a console's words, out at
  once rather than behind the background's).  Its turns go by the ticks: the foreground has `FORE_RUN` (8) ticks at most, then the background the rest
  of a tick, its tasks in turn, so a foreground task busy for good still leaves every other task its turn.  It's for a
  program whose answer to a key runs through tasks of its own while others work in the background (Dingus Quest Zero:
  its UI, parser, world and senses while a command is in hand; its denizens' minds after).  The flag goes with the task:
  cleared as it ends or `EXEC`s.  With no task in the foreground the scheduler is the round robin alone, as before.
* **Task 0** runs only when nothing else can (its idle loop: `WAI`), or when interrupt handlers have queued notes for it
  to deliver.
* **`PREEMPT_OFF`** holds the CPU for timing-critical work (interrupts still run; a switch that comes due waits for
  `PREEMPT_ON`).  Keep it short: a few ticks at most.

### Starting and ending

* **`SPAWN`** starts a program in a task of its own: the **lowest** free task (init is task 1).  It opens the program's
  file through the caller's namespace and reads its header:
  * **a module of the paged ROM runs in place**: its task's `$01` selects the module's bank, so its code is at `$A000`
    in that task alone; only its data is copied into the task's RAM, and its BSS cleared, by the task itself as it
    starts.  Sixteen tasks can run the same module at once.  A `SPAWN` of a module takes about 14 ms.
  * **any other program is a RAM program**: the new task gets the file as an fd and loads itself, at `$0800`, as it
    starts, so `SPAWN` doesn't wait for the load.
  * **a bank program** (`HF_BANK`) loads itself the same way, but into a RAM bank of its task's own, at `$8000`
    (below).
* **Drivers** take the **highest** free task, task F first.  A driver runs its `init`, then is `IDLE`, running only for
  calls (requests) and interrupts.  A driver with work of its own between its calls (a device to look at now and
  then, things to do when a key comes) doesn't return from its `init`: it waits in **`SRV_NAP`**, for a time or till
  its own event count changes.  While it waits there it serves its calls as any driver does; when it runs again it's
  busy (a call waits) till its next `SRV_NAP`.  So such work takes no task of its own: HydraOS's `kdev` looks at the
  Vera X's keyboard this way, and its `cons` hands keys to the windows with no program asking.
* **The child gets** the caller's fds 0, 1 and 2 (or a map of them), its current directory, its namespace (shared, or
  empty), its note group (or a new one), and a copy of its environment.  Its parent is the caller.
* **`EXEC`** runs another program **in the calling task**, in place of the one that calls it (Plan 9's `exec`).  The
  program is found as `SPAWN` finds it; the task keeps its number, its parent, its note group, every fd, its
  directory, its namespace and its environment; what was the old program's goes as at its end (its interrupt lines,
  segments, semaphores, served devices, note handler, pages and banks).  The parent's `WAIT` gets the new program's
  status.  rc uses it for the last command of `rc -c`, so a command typed at HyForth's prompt takes one task, not two.
* **`RUN`** runs a **bank program in the calling task**, as a subroutine, and waits for it: no task is made, and the
  program stays loaded for the next time.  [Bank programs](#bank-programs), below.  `SPAWN` with `SPAWN_RUN` does
  the same for a caller that would wait for the program at once, and gives back its own task, whose `WAIT` gets the
  program's status; a program that isn't a bank program starts in a task of its own as ever.
* **`EXITS`** ends a task with a status: a code (0 for success) and a message (31 characters).  Returning from a
  program's entry point is `EXITS` with 0.  Everything the task had is given back: its fds, pages, banks, segments,
  semaphores, interrupt lines.
* **`WAIT`** takes a child's status.  The status of a child that ended first waits for its parent's `WAIT`, and its task
  isn't reused until then.  A parent that ends first leaves its children **nobody's**: each is let go as it ends, its
  task free at once.  (They used to go to init, which had to stay to wait for them.)
* **Init again.**  `INIT_RUN` asks the kernel task to start init again in a task of its own, with a number as its
  argument (HydraOS's console: the window the user made); and the kernel does it itself, with `0`, when the task init
  was in has `EXEC`ed and then ends ([the boot](boot.md#step-5-the-tick-init-and-the-idle-loop)).

---

## Bank programs
*`run.s` (page 5, its calls' entries on page 0)*

A **bank program** is a program built to run from a RAM bank: an o65 module (as ld65 links it, its text at `$8000`,
its zero page `$22` up, its relocations kept) after a HYX2 header with `HF_BANK` set.  Most of the tools in
`/rom/bin` are bank programs.  The kernel can run one in a task of its own, as `SPAWN` runs any program, or in the
**caller's own task** (`RUN`):

* **The code cache.**  The kernel keeps a task's bank programs in RAM banks of the task's own (eight at most, from
  `BANKS_ALLOC`), several to a bank, one above the other.  Each bank's last 256 bytes are its directory: where each
  program is, its file's identity (`STAT`'s qid, version, length and time, so a changed file is loaded again), when it
  was last used, and whether it's running.  A program is put in the lowest gap it fits; with none, in a new bank;
  with no bank to be had, the program used least lately is given up, and the next, till it fits.  The cache goes
  with the task (at its end, or an `EXEC`).
* **Loading.**  The first time, the program is read into its place and relocated there: its code and data by where it
  went, its zero page by where that goes.  A program placed where it was linked, at a bank's start, isn't relocated at
  all; in a task of its own it's read in one piece, its text landing where it was linked (its file's `HX_LENGTH`,
  the build's, says how much).  A copy of its data is kept, so each run starts with its data as it was loaded and its BSS cleared, as a new
  load's would.
* **Running in place.**  The program runs on the caller's stack, with its bank at `$8000` and its zero page where the
  caller's program's is (the caller's bytes kept and put back).  It gets what `SPAWN` would give it: its arguments at
  `r0`, the caller's fds 0, 1 and 2 or a map of them, no note handler.  With `RUN_LEND` it may move the break past
  the caller's (its memory for `BREAK`; put back after), so a caller says whether the RAM past its break is free.  It
  ends as any program does, returning or `EXITS` or a note's default action (Ctrl-C), and then everything is the
  caller's again: its fds (the program's closed), note handler, zero page, banks and break.  A note that ended the
  program (but a kill) is posted again, so the caller's own handler sees it.  A program running in place can `RUN`
  another.
* **What can't run in place**: a program marked `HF_TASK` (one that changes what's the caller's, or needs more RAM
  than a caller can lend) runs only in a task of its own; and a program that isn't a bank program at all (a RAM
  program, a module) is `SPAWN`ed.

**Children** are bank programs of a program's own: modules whose functions the program calls itself, on its own C
stack (`sdk/c/include/bankprog.h`; Dingus Quest Zero's pieces).  `BANK_LOAD` puts one in the same code cache, its
imports found by name in the caller's exports file (`NAME.exp`, the build's), its zero page where the caller says,
and tells the caller its bank and directory entry; the caller maps the bank (`$00` alone), calls the entry and maps
its own bank back, checking first that the entry is still the same load.  As it loads a child the kernel borrows a
RAM bank of the task's, the **loader bank**: the child's imports' names and addresses (each export looked for through
hash chains), and the child's file and the exports' file read 2K at a time; it's given back at the load's end.  Only
the child's exports' table stays past its BSS, so a child's code, data and BSS may fill a bank's 7.5K.  `BANK_FLUSH`
empties the cache (and sets how many banks it may have).

---

## Interrupts
*`irq.s`, `common.s`*

There's **one path for every interrupt line**:

1. The line's entry in the vector RAM points to its stub in the COMMON block, the same on every page.
2. The stub saves `W` and goes to the dispatcher on page 0, with `.A` = the line, and the interrupted task's frame begun
   on its own stack.
3. The dispatcher finds the line's **owner** and switches `T` to it: the owner's interrupt entry runs **in its own
   task**, with its zero page, its stack and its banks (its module at `$A000`), with `.A` = the line and `.X` = the
   task that was interrupted.
4. The entry returns `.A = 0`, or `IRQ_RESCHED` to ask for a task switch (the tick always asks).  The interrupted task is
   switched out, unless it holds preemption.

**The rules for an interrupt entry:**

* It runs with interrupts off, and **never waits**.
* It has **about 85 cycles**: the whole path takes about 115 of the 200 that interrupts may be held off.
* It wakes its clients by **adding 1 to its event count** (`inc TASK_EVENT`), never with `WAKE`, and sends a note to a
  note group with `NOTE_QUEUE` (the console's Ctrl-C).
* A line must be owned (`IRQ_OWN`) **before** its device interrupts: a line nobody owns is counted and ignored, and, being
  level-triggered, comes straight back.

**The lines:**

| Line | Owner | Notes |
| :--- | :---- | :---- |
| 0 | The kernel | The VIA: timer 1 is the tick.  The stub checks the **ACIA first**, since at 115200 a byte can't wait behind the tick |
| 1 | The serial driver (`ser` or `cons`) | The ACIA |
| 2 | `vid` (HydraOS) | Slot 0's A: the Vera X's VSYNC |
| 3 | Free | Slot 0's B |
| 4 | Free | The YM2151: the sound driver doesn't use its interrupt (songs are timed by the tick) |
| 5-14 | Free | The other slots |
| 15 | (none) | No hardware: the vector entry `BRK` uses |
| 16 | The serial driver | **VIA timer 2**, a line of its own: owning it is owning the timer (the serial port's paced sending) |
| 17 | `kdev` (HydraOS), while `/dev/gpio/ca1` is open | **VIA CA1**, a line of its own |

**`BRK`** comes through the same path as line 15's: a `BRK` in a program is the note `NOTE_BRK` to that program (or,
under the debugger, a breakpoint).

---

## Reaching other tasks
*`call.s`, `kdefs.inc`*

Tasks are kept apart, but the kernel has to reach across.  It does so in four ways:

* **SCALL** runs a task's **serve entry in that task**: its zero page, stack and banks.  It's how a request reaches a
  server: the client is `CALL` meanwhile, and the server `READY`, so the call can be preempted like any code.  A task
  serves one call at a time; a second caller is `BLOCKED` until it's free.
* **KCALL** is an SCALL into the kernel task, to a kernel routine.  Since the kernel task is never preempted, KCALLs run
  one at a time, to their ends, and the kernel's tables need no locks.
* **kcopy** copies bytes between this task's memory and another's, each side as its own task sees it (its RAM, its bank
  at `$8000`, its ROM bank at `$A000`): `T` switched for each byte's read and write, interrupts off for 4 bytes at a
  time.  `CLIENT_READ` and `CLIENT_WRITE` are kcopy.
* **Quick looks**: a moment in another task's memory with `T` switched and interrupts off, no stack used (the stack
  page changes with `T`), then back.  For a byte or two of another task's state.

`T` is written only by quick looks, the interrupt path, the scheduler, SCALL and kcopy.

---

## Memory
*`mem.s`*

The kernel manages memory in **pages** (256 bytes) and **banks** (8K).  Finer allocation (`malloc`, Forth's `ALLOT`,
hylang's heap) is each language's own.

**A task's RAM:**

* `BREAK` moves the end of the program's data (its break), as Unix's `brk`: up from where its BSS ends.
* `PAGES_ALLOC` gives a run of pages from the **top** of RAM down; `PAGES_FREE` gives them back.  The break can't rise
  over a page given out.

**Banks**: each task has 16 banks of 8K **for each memory card** (module) installed: module m's are `$m0-$mF`, and
they're the task's alone.

* `BANKS` says how many, and which modules are good.
* `BANKS_ALLOC` gives a run from the lowest free (`BANKS_ALLOC_IN` from a range), `BANKS_FREE` gives them back.  The
  bitmap is only so that libraries in one program don't collide: a program **selects a bank by writing `$00`**, and
  reads and writes it at `$8000-$9FFF`.

```
            lda         #4
            jsr         BANKS_ALLOC                         ; Four banks: .A = the first
            bcs         @nomem
            sta         $00                                 ; The first at $8000
            lda         #$5A
            sta         $8000
```

**Shared segments**: a run of shared banks that several tasks attach to.

* `SEG_CREATE` makes one of `.A` banks, attached to the caller; `SEG_ATTACH` and `SEG_DETACH` count tasks in and out; the
  last out (or the last to end) frees it.
* `SEG_MAP` says how to reach bank `.X` of it: `.A` for `U` (`$FFF1`) and `.X` for `$00`.
* Segments use the shared bank IDs `$80-$FF`; `$00-$7F` are HydraOS's shared RAM disk's.
* To find a segment by name, `kdev` keeps names for them: `name NAME N` written to `/dev/seg/ctl`, and any task reads
  `/dev/seg/NAME` for its number.

```
            lda         seg
            ldx         #0                                  ; Its first bank
            jsr         SEG_MAP
            sta         $FFF1                               ; U: the shared macro-page
            stx         $00                                 ; Then the bank: at $8000
```

Memory POST found bad is never given out.

---

## Semaphores
*`sem.s`*

The kernel keeps **16 semaphores**, every task's by number.  Each is a count and the tasks waiting on it.

| Call | What it does |
| :--- | :----------- |
| `SEM_NEW` | Make one: `.A` its count; `.X` 0, or `SEM_MUTEX` for a mutex (a count of 1, given back only by the task that took it) |
| `SEM_ACQUIRE` | Take one of its count, waiting (using no CPU) until there's one.  A note ends the wait (`E_INTR`) |
| `SEM_TRY` | The same without the wait: `E_AGAIN` if there's none |
| `SEM_RELEASE` | Give one back; the waiters wake, and the first to run takes it |
| `SEM_WAIT` | `SEM_ACQUIRE` with a time limit (`.X` ticks): `E_AGAIN` if the time comes first; a release ends the wait at once |
| `SEM_FREE` | Free it; its waiters wake with `E_INVAL` |

A task's end frees the semaphores it made and gives back the mutexes it holds.  A mutex's holder that asks again gets
`E_BUSY`, not a wait for ever.

---

## Notes
*`notes.s`*

Notes are Plan 9's signals, numbered:

| Note | Name | Default | Sent by |
| :--- | :--- | :------ | :------ |
| 1 | `interrupt` | ends the task, status 130 | Ctrl-C at the console, to the window's note group |
| 2 | `kill` | ends the task, status 137; **never** the handler's | Ctrl-\\, `kill`, `/proc/N/ctl`'s `kill` |
| 3 | `hangup` | ends the task | |
| 4 | `alarm` | ends the task | |
| 5 | `brk` | ends the task | a `BRK` instruction in the program |
| 16-31 | a program's own | ends the task | |

* **`NOTE`** sends one to a task, or to a **note group** (`NOTE_GROUP | g`): every task in it.  A child is in its
  parent's group unless `SPAWN_NEWGROUP` gives it one of its own.
* **A task takes its notes in its own code, never inside the kernel**: when the scheduler switches to it and its frame
  is in its own code, or at the end of a system call that waits, which then returns `E_INTR`.
* **`NOTIFY`** sets a handler: it's called with `.A` = the note, and returns C = 0 to go on where the task was, or C = 1
  for the default.  With no handler, the default ends the task with 128 + the note's Unix number.
* A note that comes while the handler runs waits for it (except a kill).
* A program that runs code in more than one bank keeps its handler in its RAM: the kernel calls it with whatever bank is
  at `$A000` when the note comes.

---

## Time
*`sched.s`, `time.s`*

* **The tick**: 200 a second (`TICK_HZ`), from VIA timer 1.  `TICKS` is its count, 16 bits (it wraps after about 5.5
  minutes).
* **`SLEEP`** waits a number of ticks; **`SLEEP_UNTIL`** until a tick count, for steady timing; **`YIELD`** lets the
  others run.
* **The clock**: `TIME` is seconds since 2000-01-01 (in `r0/r1`), kept as the boot's time plus the ticks since.
  `TIME_SET` sets it.  With a DS1747 in U7, `RTC` reads and sets the chip, and kdev sets the clock from it at boot
  ([the devices](../02-hardware/devices.md#the-clock-chip)).

---

## Debugging support
*`debug.s`, `info.s`*

* **`TASKINFO`** says what the kernel knows of a task: its state, flags, parent, CPU time, name, note group.  **`TASKREAD`**
  reads its arguments, current directory, environment, saved registers, or an open fd.  `ps` and `/proc` are built on
  them.
* **`TASKSTOP`** stops a task where it is, or starts it again.
* **`TASKSTEP`** runs one instruction of a stopped task **out of line**: a copy of the instruction in the task's own zero
  page with a `BRK` after it, run as the task, which stops it again.  `JMP`, `JSR`, `RTS` and `RTI` it does itself, on the
  task's frame; a `JSR` into the kernel always runs whole.  So a step needs no trace flag and patches no ROM.
* **Breakpoints** are the debugger's: a `BRK` written into the program's RAM.  With breakpoints on, any `BRK` stops the
  task instead of sending it the `brk` note.
* `SPAWN_STOPPED` starts a program stopped at its entry point.

These are a driver's calls (kdev's): programs use them through `/proc/N/ctl` ([files](files.md#the-kernels-own-devices)),
and `db`, the debugger, is built on that ([the tools](../07-tools.md)).
