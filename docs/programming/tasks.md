## **Tasks and the scheduler**

Tasks, how they're scheduled, how to start, stop, wait for and signal them, and how drivers run in tasks of their own.  Sources: `os_rom/kernel/tasks.s`, `os_rom/drivers/via.s` (the tick).  Part of the [Programmer's Guide](README.md).

### **What a task is**

The Hydra has **16 tasks**, `T` = `$0-$F`.  The hardware gives each one its own:
* **RAM** `$0000-$7FFF`: its zero page, stack page and program RAM;
* **bank registers**: which RAM bank is at `$8000-$9FFF` (`$00`) and which paged ROM bank at `$A000-$DFFF` (`$01`);
* **task RAM banks** on the memory modules.

Selecting a task is one write to `T` (`$FFF0`).  Everything else is shared: the ROMs, shared RAM, I/O, and the pseudo-registers `U`, `V`, `W`.  So a task switch saves and restores `W` and `U` in the task's stack frame.

The software gives each task:
* an **MMU area** for its allocations ([memory.md](memory.md));
* a **task system page** with the IRQ tables and its 12 file descriptors;
* an **IO transfer area** in shared RAM, holding its IO requests and its namespace ([io.md](io.md)).

**Per-task memory map** (all tasks alike):

| Addresses | Use |
| :-------- | :-- |
| `$00`, `$01` | Bank registers (RAM bank, ROM bank) |
| `$02-...` | OS zero page (`include/zero.s`), the same layout in every task |
| `...-$FF` | The task's own zero page, reserved with `TASK_ZP` ([rom-layout.md](rom-layout.md#zero-page)) |
| `$0100-$01FF` | The stack |
| `$0200-$06FF` | Buffers (in the shell: HyForth's input buffer and stacks, WOZMON's input buffer) |
| `$0700-$07FF` | stdio buffers: stdout (`$0700`) and the stdin read-ahead (`$0780`) |
| `$0800-$7CFF` | Program RAM.  The MMU hands out pages from the top down; below its *page floor* a program can use RAM directly (HyForth's variables are at `$0800`, and its dictionary grows up after them) |
| `$7D00-$7DFF` | Task system page: IRQ tables (`$7D00-$7D8F`), unclaimed-IRQ counters (`$7D90-$7D9F`), fd table (`$7DA0-$7DFF`) |
| `$7E00-$7FFF` | MMU area: allocation maps and the handle table |

**Task numbers:**

| Task | Use |
| :--- | :-- |
| `$0` | System task: boot, then the **idle task** (runs `wai` when no other task can run) |
| `$1` | The shell (HyForth, then WOZMON), the foreground task at boot |
| `$2-$B` | Free: started with `TASK_RUN`, `TASK_START`, `TASK_CLONE`, HyForth's `shell` or pipelines |
| `$C` | Storage driver: `/dev/sd` and HydraFS (`/sd`) (Resident) |
| `$D` | Pipe server (Resident) |
| `$E` | Sound driver: `/dev/snd` (Resident) |
| `$F` | Serial driver: `/dev/cons`, `/dev/ser`, `/dev/ser/ctl` (Resident) |

### **Task status**

`TASK_STATUS_REG` (`$03` in each task's zero page) holds the task's state.  `TASK_STATUS` (`$F866`, `.A` = task → `.A` = status) reads another task's.

| Bit | Flag | Meaning |
| :-- | :--- | :------ |
| 0 | `TASK_BUSY_FLAG` (`$01`) | In use (0 = free) |
| 1 | `TASK_PAUSED_FLAG` (`$02`) | Not to be run: being set up, or waiting for a task it started (`TASK_START`) |
| 2 | `TASK_WAITING_FLAG` (`$04`) | Waiting (IO, `TASK_WAIT`, a sleep) until woken |
| 3 | `TASK_RESIDENT_FLAG` (`$08`) | A driver: only runs from IRQs and calls into it |
| 4 | `TASK_BREAK_FLAG` (`$10`) | A break is due |
| 5 | `TASK_KILL_FLAG` (`$20`) | A kill is due |
| 6 | `TASK_CALLING_FLAG` (`$40`) | In a `TASK_CALL`: its call is running in another task |
| 7 | `TASK_GUEST_OUT_FLAG` (`$80`) | Switched out in the middle of another task's call |

### **Scheduling**

**The tick.**  VIA timer 1 interrupts 200 times a second (`SCHED_TICK_HZ`, every 5 ms: `SCHED_START`).  At each tick the IRQ dispatcher switches to the next runnable task, round robin over tasks 1-15.  Task 0 runs only when no other task can; it runs `wai`, so the CPU sleeps until the next interrupt.

**Runnable** means busy, and not paused, waiting, resident or in a call.  A resident task switched out in the middle of a call is runnable too, until it finishes that call.

**The task frame.**  A task that isn't running keeps one frame on its own stack, whatever stopped it (the tick, `YIELD`, or waiting):

```
U, ZP_TC_TASK, ZP_TC_VEC+1, ZP_TC_VEC, Y, W, X, A, P, PCL, PCH     (top of the stack first)
```

Its stack pointer is kept in its zero page.  A switch saves the SP, picks the next task, writes `T`, loads that task's SP and unwinds its frame.

**Holding the CPU:**

| Way | Effect | Use for |
| :-- | :----- | :------ |
| `sei` ... `cli` (or `php` / `sei` ... `plp`) | No interrupts, so no task switch | Short sections only (tens of cycles): it holds off serial input and the tick |
| `NO_PREEMPT` ... `PREEMPT` (`$F857` / `$F85A`) | Interrupts run, but no task switch; a switch that came due happens at `PREEMPT` | Longer sections; nestable (a counter) |
| `YIELD` (`$F854`) | Give up the CPU now | Polling loops |

**How `NO_PREEMPT` behaves:**
* **Blocking:** a task that blocks (IO, `TASK_WAIT`, `YIELD`) while holding it still gives up the CPU: it can't make progress anyway.
* **Calls into other tasks:** it holds across `TASK_CALL`s, such as the IO requests the task makes; the server running its request isn't switched out either.
* **The kernel's own use:** the MMU calls and task reset hold `NO_PREEMPT` rather than turning interrupts off.
* **The scheduler:** it scans for the next task with interrupts on between its looks at each task.

So interrupts, above all the serial port's, are rarely held off for long ([interrupts.md](interrupts.md#timing-notes)).

### **Starting tasks**

| Call | Does |
| :--- | :--- |
| `TASK_RUN` (`$F863`) | `.A.Y` = entry point, `.X` = its ROM page (0 for RAM or page 0 code) → `.A` = the new task.  It runs **alongside** the caller; it ends when its entry point returns |
| `TASK_START` | `ZP_TEMP_VEC` = entry point (RAM or page 0) → runs it in a new task and **waits** for it to finish (the caller is paused) |
| `TASK_CLONE` (`$F899`) | Like `fork`: a new task with a **copy** of the current one (below), starting at `.A.Y` on page `.X` |
| `SHELL_CMD` (`$F8F6`) | Not a call but an entry point for `TASK_RUN` (page 0): a **command shell**, HyForth running the command lines on its stdin with no banner or prompt, as Plan 9's `rc -c`.  It ends at its input's end, with the last command's exit status.  C's `system` gives it a pipe with the line in it |
| `DRV_START` | Start a driver in a given task (below) |

Every new task gets a copy of its parent's **open fds**, **namespace**, **current directory** and **environment** (each server is told: `H9_DUP`), and records its parent (`ZP_TASK_OWNER`; when a task ends, the tasks it started get its owner instead, and its area on the RAM disk, `/ram/N`, is removed: `TASK_ORPHANS`).  `TASK_MAY` (page 5) answers "may task A use task N's things" from the owner chain, task 0 always yes: the RAM disk's areas use it.  So `TASK_RUN` from the shell gives a task that prints on the console and reads the keyboard when it's in front.  Output buffered by the parent is written out first (`IO_FLUSH`), so it comes out in order.

**`TASK_CLONE`** copies:
* the task's RAM, `$0200-$7CFF` and the MMU area `$7E00-$7FFF`.  Not copied: the stack page, the task system page, and the free pages between the MMU's page floor and its lowest allocated page;
* its task zero page (everything above the OS zero page);
* its fds and namespace.

The copy goes a page at a time through the IO transfer area, about 1/400 s per page at 3.58 MHz, before the new task runs.  HyForth uses it for pipelines (each stage but the last runs in a copy of the shell) and for `run`ning a script.

**Waiting for a task started alongside:** `TASK_JOIN` (`$F8F3`, below) gives the task the console if the caller has it, makes the caller the task's parent (`TASK_PARENT`, in the task's zero page) and pauses, as `TASK_START` does, so the task's end wakes it; then it takes the console back and returns the task's exit status.  The shell's `run` starts a program with `TASK_RUN` (or `TASK_CLONE`) and waits for it this way (`SH_WAIT` in `os_rom/shell/run.s`; [programs.md](programs.md)).

**From WOZMON**, `addrS` starts a task at `addr` and waits for it (`TASK_START`).

### **Ending tasks**

A task ends when its entry point returns (`TASK_EXIT`), or when it calls `TASK_EXITS` with an exit status (below).  Everything it had is freed at once:
* its fds are closed (each server gets `H9_CLUNK`);
* its MMU area is reset (all its pages, chunks and banks);
* its shared memory references are dropped;
* its IRQ and software interrupt handlers are removed.

If it was the foreground task, the task that started it gets the console back (`CONS_RELEASE`).  A parent waiting in `TASK_START` or `TASK_JOIN` continues.

### **Exit statuses**

As Plan 9's `exits` and `wait`: a task ends with a **code** (0-255, 0 for success) and a **message** (up to 30 characters, `EXIT_MSG_MAX`; none for most), and the task that started it gets them when it waits for it.

| Call | Does |
| :--- | :--- |
| `TASK_EXITS` (`$F8F0`) | End this task: `.A` = the code, `ZP_IO_BUF` = the message (zero-terminated; a high byte of 0: none).  Doesn't return |
| `TASK_JOIN` (`$F8F3`) | `.A` = a task this one started: wait for it to end (it has the console meanwhile, if this task has it) → `C` = 0, `.A` = its code, and its message at `ZP_IO_BUF` (a buffer of 31 bytes; a high byte of 0: not wanted).  A task that's ended already isn't waited for.  `C` = 1, `ERR_BAD_TASK`: not a task |

| How a task ends | Code | Message |
| :--------------- | :--- | :------ |
| Its entry point returns | 0 | none |
| `TASK_EXITS` | `.A` | `ZP_IO_BUF`'s |
| A break (Ctrl-C) with no break handler | 130 (`EXIT_BREAK`) | `interrupt` |
| A kill (Ctrl-\\, `kill`, its starter's break) | 137 (`EXIT_KILLED`) | `killed` |

The status is written as the task ends, before its parent is woken, to a record for each task in the system's shared bank (`EXIT_TABLE`, `$9D20`: 16 × 32 bytes, the code and the message; `os_rom/kernel/exits.s`, page 5).  It stays there until the task's number is used again.  The shell keeps the status of each program it waits for: HyForth's `status`, and `/env/status` (Plan 9's `$status`: the message, or the code if there's none, or empty for success).  A script's is that of its last command, or its error's number, or what `exits` gives ([HyForth](../using/hyforth.md#background-tasks-and-exit-statuses)).

### **Waiting and sleeping**

| Call | Does |
| :--- | :--- |
| `TASK_WAIT` (`$F85D`) | Sleep until another task or an IRQ handler calls `IO_WAKE` for it |
| `IO_WAKE` (`$F860`) | `.A` = task: make it runnable again (from any task, or an IRQ handler) |
| `TASK_SLEEP` | Sleep for `.A.Y` ticks (200 a second, up to 32767) |
| `TASK_SLEEP_UNTIL` | Sleep until the tick count reaches `.A.Y` (up to 32767 ticks ahead) |
| `TICKS_GET` | `.A.Y` = the tick count (200 a second; wraps after about 5.5 minutes) |

A sleeping task uses no CPU; the system task's tick handler wakes it (`SLEEP_CHECK`).  A break or kill ends a sleep early.  For steady timing (e.g. music), use `TASK_SLEEP_UNTIL` with a running target, so delays don't add up: the sound driver's tune player does this.

IO waits happen by themselves: a read with no data makes the task wait until the server wakes it ([io.md](io.md)).

**How the kernel waits.**  Every wait is the same: the task's bit goes in a 16-bit **wait mask** (bit = task), it sets `TASK_WAITING_FLAG` and `YIELD`s, and when it's woken it looks again, waiting again if it has to.  Waking a mask (`TASK_WAKE_MASK`) wakes every task in it and clears it; each looks again, so a wake that turns out to be for nothing costs a look and nothing more.  Everything that waits is built this way: a busy server's callers (`ZP_TC_WAITERS`, `TC_WAIT_FREE`), sleepers (`ZP_SLEEPERS`, woken by the tick when their time comes), a pipe's readers and writers, the console's, and semaphores.  (The serial port's interrupt wakes its masks with a copy of `TASK_WAKE_MASK` that uses no stack.)

### **Semaphores**

For tasks that share something, or wait for each other: a semaphore is a count and the tasks waiting for it.  `SEM_ACQUIRE` takes one, or waits (using no CPU) until `SEM_RELEASE` gives one back.  A **mutex** is a semaphore of 1 with a holder: only the task that took it can release it.

| Call | Does |
| :--- | :--- |
| `SEM_NEW` (`$F8D8`) | `.A` = its count (how many can take it before a task has to wait: 0-255), `.Y` = 0, or `SEM_MUTEX` (`$80`: a mutex, count 1) → `.A` = the semaphore (1-16) |
| `SEM_ACQUIRE` (`$F8DB`) | `.A` = semaphore: take one, waiting until there is one.  A break or kill ends the wait |
| `SEM_TRY` (`$F8DE`) | The same, but `ERR_SEM_BUSY` at once instead of waiting |
| `SEM_RELEASE` (`$F8E1`) | `.A` = semaphore: give one back, and wake the tasks waiting for it |
| `SEM_FREE` (`$F8E4`) | `.A` = semaphore: free it (any task can); the tasks waiting for it get `ERR_SEM_BAD` |

* **All tasks see the same ones**, by number: they're in the system's shared bank (`SEM_TABLE`, `$8360`), 16 of them.  So a task can make one and hand its number to the tasks it starts, or to a pipeline's stages.
* **Errors:** `ERR_SEM_BAD` (`$60`: not a semaphore, or freed while waited for), `ERR_SEM_NONE` (`$61`: all 16 in use), `ERR_SEM_BUSY` (`$62`), `ERR_SEM_NOT_HELD` (`$63`: a mutex this task doesn't hold), `ERR_SEM_FULL` (`$64`: its count is 255).
* **When a task ends,** the semaphores it made are freed, and the mutexes it holds are released (`SEM_RESET_TASK`, from `MM_TASK_RESET`).  A counting semaphore it took one of stays one down: nothing records who took what.
* **Inside:** `kernel/sem.s` (page 5).  Each call works on the table with IRQs off, for a few hundred cycles at most; a wait is the kernel's usual one (above): a release wakes every waiter, the first to run takes it, and the others wait again.

**What semaphores don't replace.**  The kernel's short updates of shared tables (the MMU's, shared memory's, the environments') hold `NO_PREEMPT` or keep IRQs off instead.  That keeps a Ctrl-C from stopping a task halfway through an update, which a lock can't do: a break ends a task's work where it is, and a table half changed would stay that way.  Semaphores are for waits that can take a while, where a break is fine: a task waiting for another, or for its turn at something they share.

### **Running code in another task: `TASK_CALL`**

`TASK_CALL` runs a routine in another task's context: its zero page, its stack (below its saved frame), its RAM bank and its MMU area.  The IO layer uses it to run servers, the IRQ dispatcher to run handlers, and `DRV_START` to run a driver's init.

* **In:** `ZP_TC_VEC` = the routine (page 0), `ZP_TC_TASK` = the task, and `.A`, `.X`, `.Y`, C as the routine's inputs.
* **Out:** the routine's `.A`, `.X`, `.Y` and flags.
* The routine runs with the caller's I flag.  The target must not be running, unless it's the current task (then it's a plain call).
* **While the routine runs**, the caller is marked in a call (`TASK_CALLING_FLAG`) and isn't scheduled.  The target can be preempted like any task, so a long request (an SD card block takes about 40 ms) doesn't hold up the others.
* **One call at a time:** a task serves one call at a time.  A task that calls it while it's busy with another waits its turn (`TC_WAIT_FREE`).  IRQ handlers are the exception: they run at once, as an interrupt would.
* **From another page:** `TASK_GATE name, routine, task` makes a gate that does all this.

### **Signals: break and kill**

`TASK_SIGNAL` (`$F8AB`) is like a Plan 9 note:
* **In:** `.A` = `TASK_BREAK_FLAG` or `TASK_KILL_FLAG`, `.X` = task.
* The task is flagged and stops waiting, and **the tasks it started** (and theirs, 4 levels deep) get a kill.
* Task 0 and the drivers can't be signalled.
* The signal takes effect when the task next runs: it continues at `BREAK_ENTRY` instead of where it was.  A task in the middle of another task's call finishes that first.

| Signal | Effect |
| :----- | :----- |
| Break | The task continues at its **break handler** (`TASK_SET_BREAK`), with the stack pointer it had when the handler was set.  With no handler, the task ends |
| Kill | The task ends.  The shell (task 1) instead starts again from scratch (a fresh HyForth) |

`TASK_SET_BREAK` (`$F8A8`): `.A.Y` = handler, `.X` = its ROM page (`.A.Y` = 0: no handler).  Called through a far-call gate (from another ROM page), the stack pointer it keeps is inside the gate, 3 bytes deeper than the caller's: a handler that goes on as its caller would (rather than starting afresh) sets the stack pointer it wants itself, as the editor does (`shell/edit.s`: `ED_SP`).  The handler never returns: it's where the task goes on after a break.  HyForth's handler goes back to its prompt with `!BREAK!`, keeping the dictionary.

The console keys send them: **Ctrl-C** a break, **Ctrl-\\** a kill to the foreground task ([io.md](io.md#the-console)).  So do `/dev/proc/N/ctl` (`break`, `kill`) and HyForth's `kill`.

### **The console's foreground task**

One task at a time is in the **foreground**: `/dev/cons` gives the keyboard to it, and only it and the tasks it started write to the console.  The others wait until they're brought to the front, like Unix job control.
* **Changing it:** `CONS_SET_FG` (`$F8AE`, `.A` = task: 1-15, busy, not a driver) brings a task to the front.  So do Ctrl-] then the task's number, `/dev/proc/N/ctl` (`fg`), and HyForth's `fg`.
* **When it ends**, the task that started it gets the console back (or else the shell).

### **Drivers: resident tasks**

A driver runs in a task of its own, marked **Resident**: it's never scheduled as a main loop.  Its code runs from its IRQ handlers, and from calls into it (its server's requests, `TASK_CALL`).  Its state lives in its task's zero page and RAM.

```
MY_DRIVER:  .word  MY_INIT          ; DriverInfo::init: runs in the driver's task; C = 0 OK, or C = 1, .A = error
            .word  MY_STOP          ; DriverInfo::stop: reserved (not called yet)
            .word  MY_NAME          ; DriverInfo::name: an HString, for messages
```

* **`DRV_START`** (`.A.Y` = the `DriverInfo`, `.X` = the task) marks the task busy and resident, and runs `init` in it.  `init` typically registers the driver's IRQ handlers (`IRQ_REGISTER`, [interrupts.md](interrupts.md)) and its devices (`DEV_REGISTER`, [servers.md](servers.md)); they then run in that task.
* **If `init` fails**, the task is freed again, and its IRQ handlers and devices are removed.

**Boot** (`kernel/os_main.s`):
1. POST.
2. The IRQ tables, tasks, `cli`, the MMU, the IO layer's own devices, the VIA.
3. The serial driver (task `$F`), then the welcome message.
4. The sound (`$E`), pipe (`$D`) and storage (`$C`) drivers, with `DRV_BOOT`, which prints `NAME FAIL ee` if one fails.
5. The shell is set up in task 1, the tick starts, and task 0 yields to the shell and becomes the idle task.

To add a driver, write its `DriverInfo` and `init` (page 0, or `init` on page 0 calling into its page through a gate, like `drivers/storage.s`), pick a free task number, and start it at boot in `os_main.s` with `DRV_BOOT`.
