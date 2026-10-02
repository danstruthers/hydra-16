# Status

Where the rebuild stands against the plan's phases
([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md), Part III), what the
spikes measured, and what measuring changed.

## In short

Phases 0 and 1 are done.  The kernel boots in the emulator, runs POST (with the old hardware test a key away),
starts its modules from the paged ROM in tasks of their own, schedules them preemptively, runs calls between tasks
and copies between them, takes every interrupt through one path, manages task RAM, banks and shared segments,
and delivers notes.  The three spikes meet their budgets.  Phase 2 (modules, servers and the console) is next.

```
PASS boot    the kernel boots, POST finds nothing wrong; init runs hello and waits for it
PASS post-t  POST: a T line stuck low (U7)
PASS post-zp POST: only the zero page and stack per task (a decoding fault)
PASS post-ram POST: an address line of RAM module 1 stuck low; the module is left unused
PASS post-sh POST: no shared RAM
PASS hwtest  a T typed during POST starts the hardware test (paged ROM bank 1)
PASS task    tasks and the scheduler: SPAWN, EXITS, WAIT, SLEEP, preemption, PAUSE and WAKE, orphans  (29 checks)
PASS note    notes: the defaults, handlers, a note to oneself, WAIT ended by one, note groups  (29 checks)
PASS mem     memory: BREAK, pages, banks, a shared segment between tasks (and kcopy from it)  (38 checks)
PASS scall   spike S3: calls into a driver's task, its errors, a busy driver, the round trip  (12 checks)
PASS kcopy   spike S2: copying between tasks  (6 checks)
PASS irq     spike S1: 115200 received by an irq entry while tasks spin  (6 checks)
```

The same with the power-up's RAM from other seeds, and with a WDC W65C51N build.  The hardware test, entered from
POST in the emulator, passes its whole quick run, its BIOS and paged ROM checksums included.

## The spikes (3.58 MHz)

| Spike | Budget | Measured | How |
|---|---|---|---|
| S1: the IRQ path | ~100 cycles to the handler | **80 cycles** from the interrupt to the handler's first instruction (7 the CPU's, 25 the COMMON stub's, 48 the dispatcher's) | Counted from the code; the irq test's fastest byte, 100 cycles from arriving to being read, agrees |
| S1: 115200 received | No loss | **2000 of 2000 bytes**, in order, while two tasks spin and the tick switches tasks; each byte read 100-255 cycles after it came (106 on average) of the 320 a character takes | The irq test: the PC sends back to back; the emulator counts each byte's wait and the ACIA's losses |
| No IRQs-off stretch over 200 cycles | 200 | **175**, an interrupt's whole service (stub to `RTI`); the longest stretch of masked code, **169**, a switch into a task waking from a sleep | Every test, from the boot's end |
| S2: kcopy | 40 cycles a byte | **36.6** | 4096 bytes to the kernel task (the kcopy test) |
| S3: SCALL | 200 cycles a round trip | **181.4** | 1000 calls to a driver, less the same loop calling the driver's code in place (the scall test) |

## What measuring changed

The spikes did what the plan meant them to: the first versions missed every budget (SCALL 474 cycles, the tick
450 cycles with sleepers, 23 bytes lost in 2000 at 115200), and these changes brought them in.  They refine the
plan's §10:

1. **The scheduler ends two kinds of waits itself.**  A sleeper is `ST_SLEEP` with its wake time in the kernel
   task's table, and a caller that finds a task busy is `ST_BLOCKED` on it; the scheduler, as it looks for the
   next task, makes one ready when its time has come or its task is free.  So the tick's interrupt only counts,
   and a call's end wakes nobody: no wait masks scanned with IRQs off.
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

## Phase 0: foundations

| Step | | Notes |
|---|---|---|
| 0.1 The tree | Done | `reborn/`; `sdk/c/` and `romfs/` come when there's something to put in them |
| 0.2 Conventions | Done | [conventions.md](conventions.md) |
| 0.3 The specification and `apigen.js` | Done | 36 calls in 9 groups (13 on page 1); the jump table (and the stubs of calls on other pages), `hydra.inc`, `errors.inc`, the error texts, `api.md`, `api.json` |
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
| 1.9 Notes, exits and waiting | Done | `NOTIFY`, `NOTE` (to a task or a note group; `SPAWN_NEWGROUP`), handlers that go on or take the default, the defaults (130 Ctrl-C, 137 kill, 133 a BRK), E_INTR from WAIT, PAUSE, SLEEP and GETC; `EXITS`, `WAIT`, orphans to init.  Plan 9's `NOTED` isn't needed: a handler's C says what it would.  The server's FLUSH for a blocked call comes with servers (phase 2) |

The kernel's layout on the BIOS ROM: page 0 (4518 bytes, 1626 left below the jump table) has what runs often:
the interrupt path, the scheduler, SCALL and kcopy, the console, SPAWN, EXITS and WAIT, the notes' trampoline;
page 1 (1872 bytes) memory, TASKINFO, DBG_PS and NOTE's work; page 4 (1072 bytes) POST.  A call on another page
goes through a 6-byte stub on page 0 (about 90 cycles more), and the kernel's own far calls through the COMMON
block (`FARCALL`).

## Next

1. Phase 2: the module framework and the console driver (the ACIA's interrupt-driven driver, now that S1 shows
   the path can take 115200), the request block, the file layer's first calls, and a blocked call's FLUSH.
2. On the board: the boot, POST, the tick, and a module that echoes the serial port at 115200 from its interrupt.
3. Still open from phase 0: the emulator's call trace.
