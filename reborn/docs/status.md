# Status

Where the rebuild stands against the plan's phases
([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md), Part III), what the
spikes measured, and what measuring changed.

## In short

Phase 0 is done but for CI.  Phase 1's core is done and measured: the kernel boots in the emulator, starts its
modules from the paged ROM in tasks of their own, schedules them preemptively, runs calls between tasks and
copies between them, and takes every interrupt through one path; the three spikes meet their budgets.  Still to
come in phase 1: POST, the memory calls and notes.

```
PASS boot    the kernel boots; init runs hello and waits for it
PASS task    tasks and the scheduler: SPAWN, EXITS, WAIT, SLEEP, preemption, PAUSE and WAKE, orphans  (23 checks)
PASS scall   spike S3: calls into a driver's task, its errors, a busy driver, the round trip  (12 checks)
PASS kcopy   spike S2: copying between tasks  (6 checks)
PASS irq     spike S1: 115200 received by an irq entry while tasks spin  (6 checks)
```

The same with the power-up's RAM from other seeds, and with a WDC W65C51N build.

## The spikes (3.58 MHz)

| Spike | Budget | Measured | How |
|---|---|---|---|
| S1: the IRQ path | ~100 cycles to the handler | **78 cycles** from the interrupt to the handler's first instruction (7 the CPU's, 25 the COMMON stub's, 46 the dispatcher's) | Counted from the code; the irq test's fastest byte, 98 cycles from arriving to being read, agrees |
| S1: 115200 received | No loss | **2000 of 2000 bytes**, in order, while two tasks spin and the tick switches tasks; each byte read 98-265 cycles after it came (104 on average) of the 320 a character takes | The irq test: the PC sends back to back; the emulator counts each byte's wait and the ACIA's losses |
| No IRQs-off stretch over 200 cycles | 200 | **181**, a tick's whole service (stub to `RTI`); the longest stretch of masked code, **159**, a switch into a task waking from a sleep | Every test, from the boot's end |
| S2: kcopy | 40 cycles a byte | **36.6** | 4096 bytes to the kernel task (the kcopy test) |
| S3: SCALL | 200 cycles a round trip | **181.5** | 1000 calls to a driver, less the same loop calling the driver's code in place (the scall test) |

## What measuring changed

The spikes did what the plan meant them to: the first versions missed every budget (SCALL 474 cycles, the tick
450 cycles with sleepers, 23 bytes lost in 2000 at 115200), and these changes brought them in.  They refine the
plan's §10:

1. **The scheduler ends two kinds of waits itself.**  A sleeper is `ST_SLEEP` with its wake time in the kernel
   task's table, and a caller that finds a task busy is `ST_BLOCKED` on it; the scheduler, as it looks for the
   next task, makes one ready when its time has come or its task is free.  So the tick's interrupt only counts
   (35 cycles), and a call's end wakes nobody: no wait masks scanned with IRQs off.
2. **The kernel task is never preempted** (its preemption count is always at least 1; it yields when it idles).
   A KCALL runs to its end without SCALL carrying a "hold" into the task called.  A program's `PREEMPT_OFF`
   holds its own task only: a driver it calls can be switched out.
3. **Every task has a copy of the IRQ lines' owners** (`TA_OWNERS`), so the dispatcher reads the owner where it
   is, with no quick look; `IRQ_OWN` and `IRQ_RELEASE` are KCALLs that write all 16 copies.
4. **kcopy bursts are 4 bytes, unrolled** (8 would hold IRQs off for over 200 cycles).
5. **SCALL carries C in P** across the switch back, and a task with no serve entry is marked `BUSY_NOSERVE`, so
   the fast path has one check.
6. **Scratch has owners** (docs/conventions.md): the tests found the scheduler's counter in a byte that SPAWN
   and kcopy used too (a preemption in the middle of either changed its count), and WAIT's saved argument in a
   byte SCALL uses.
7. **The emulator** ends an IRQs-off stretch when it takes an interrupt (so masked code and interrupt service
   are measured apart; the receive latency measures them together), and counts each received byte's wait.

## Phase 0: foundations

| Step | | Notes |
|---|---|---|
| 0.1 The tree | Done | `reborn/`; `sdk/c/` and `romfs/` come when there's something to put in them |
| 0.2 Conventions | Done | [conventions.md](conventions.md) |
| 0.3 The specification and `apigen.js` | Done | 23 calls in 9 groups; the jump table, `hydra.inc`, `errors.inc`, the error texts, `api.md`, `api.json` |
| 0.4 The build | Done, but the program link | `build.js`: the BIOS link (16 pages, COMMON at `$FD00`, the vectors), the module link (`$A000`, data copied to RAM), `romimg.js`, the budget report.  The RAM program link (`$0800`) comes with loading programs from files (phase 4) |
| 0.5 Emulator additions | Mostly | `sim/run.js`: the new images, the task view, labels in traces, PC watches by label, live console.  Still to do: the call trace (by `api.json`) |
| 0.6 CI | Not yet | A step in `.github/workflows/build.yml` (outside `reborn/`): `node reborn/build.js` and `node reborn/sim/test.js` |

## Phase 1: the kernel core

| Step | | Notes |
|---|---|---|
| 1.1 Reset and POST | Reset done; POST not yet | The reset stub on every page, `T U V W` set, the RAM modules probed.  POST's RAM line tests and the hardware test still to port |
| 1.2 The kernel task and the per-task areas | Done | `include/layout.inc`; quick looks in `kernel/kdefs.inc`.  The build's check that nothing else writes `T`: not yet |
| 1.3 Spike S1 | Done | Above |
| 1.4 Spike S2 | Done | Above; in RAM and in a RAM bank (a shared bank: not yet tested) |
| 1.5 Spike S3 | Done | Above; a busy driver, and a call switched out mid-way (one that sleeps) |
| 1.6 Tasks and the scheduler | Done, but CPU time | Frames with `U` and `W`, round robin, `PREEMPT_OFF`/`ON`, the tick, `SLEEP`, `SLEEP_UNTIL`, `YIELD`, `PAUSE`/`WAKE`.  CPU time per task: not yet |
| 1.7 The jump table and the first calls | Done | With the polled console (9600, Rockwell or WDC) |
| 1.8 Memory | Not yet | `BRK`, `PAGES_*`, `BANKS_*`, shared segments |
| 1.9 Notes, exits and waiting | Exits and waiting done; notes not yet | `EXITS`, `WAIT` (any child, messages), orphans to init, exit records held till waited for |

Also in the kernel: modules started from the paged ROM's directory (boot drivers with `HF_BOOT`, then init),
`SPAWN "#m/NAME"` with arguments, `SYSINFO`, `ERRSTR`, `IRQ_OWN`, `IRQ_RELEASE`, `DBG_SCALL`, `DBG_KCOPY`,
`DBG_PS`.  The kernel is 4129 bytes of BIOS ROM page 0, with 2015 left below the jump table (`node build.js`).

## Next

1. The rest of phase 1: POST, CPU time, the memory calls (1.8), notes (1.9), kcopy with a shared bank, the `T`
   writer check, CI.
2. Phase 2: the module framework and the console driver (the ACIA's interrupt-driven driver, now that S1 shows
   the path can take 115200), the request block, the file layer's first calls.
3. On the board: the boot, the tick, and a module that echoes the serial port at 115200 from its interrupt.
