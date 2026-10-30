# The system call reference

Every system call the kernel has, with its registers, its errors, and its forms in HyForth and hylang.  [The system
calls](../03-base/calls.md) explains how calling works; this page is the reference to keep open while you write.

**The rules for every call:**

* Call with `jsr NAME` (assembly: `hydra.inc`), `hy_call (HY_NAME, &regs)` or a wrapper (C: `hydra.h`), `sys-name` (HyForth
  and hylang), or `SYS "NAME"` (HyBASIC).
* Arguments and results in `.A`, `.X`, `.Y` and the call registers `r0`-`r15` (`$02-$21`); a 16-bit value is `.A` (low)
  and `.X` (high), or a call register.  `r0/r1` is a 32-bit value, `r0` the low half.
* **C = 0 is success; C = 1 is failure, with the error code in `.A`**, for every call.
* A call may change `.A`, `.X`, `.Y`, `r0`-`r15` and the flags.  It never touches `$22-$7F`.
* A call that **waits** can be ended by a note: it returns `E_INTR`, and the note is taken on the way out.

**In the HyForth column**, the stack effect lists the registers in order: `( r0 r1 .A -- .A ior )` takes `r0`, `r1` and
`.A` and gives `.A` and an ior (0, or -512 less the error code).  **In the hylang column**, each argument is
`REGISTER=KIND` (`s` a string, `n` a number, `buf(N)` a buffer the call fills, `?` may be left out) and `->` what the
function gives.

<!-- calls:begin -->

Made from `base/spec/api.def` and `base/spec/errors.def` by `node docs/build/callref.js`: 97 calls in 9 groups.

**The groups:** [system](#system-f800), [task](#task-f830), [memory](#memory-f890), [file](#file-f8c0), [name](#name-f920), [cons](#cons-f950), [time](#time-f980), [server](#server-f9b0), [dbg](#dbg-fa10); [the error codes](#the-error-codes); [the constants](#the-constants).

---

## system (`$F800`)

System information and services: 16 slots from `$F800`, 5 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`SYSINFO`](#sysinfo) | `$F800` | What the system has. |
| [`ERRSTR`](#errstr) | `$F803` | An error code as text, for messages. |
| [`KMESG`](#kmesg) | `$F806` | The kernel's messages. |
| [`REBOOT`](#reboot) | `$F809` | Start the system again, as the reset button does (the VIA's and ACIA's interrupts quieted first: only the button resets the chips); with REBOOT_HWTEST, into the hardware test, as a T typed during POST does. |
| [`XCALL`](#xcall) | `$F80C` | Call a routine in another bank of the paged ROM (a library module's: HT_LIBRARY, its bank from MODINFO), as the X16's jsrfar. |

### SYSINFO

`$F800`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A = the ABI version (1); .X = how many RAM modules are installed; r0 = the free tasks (bit = task) |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-sysinfo ( -- .A .X r0 )` |
| **hylang** | `sys-sysinfo -> .A .X r0` |

What the system has.

### ERRSTR

`$F803`.

| | |
| :-- | :-- |
| **In** | .A = an error code; r0 = a buffer (32 bytes) |
| **Out** | the error's text in the buffer, zero-terminated ("error $xx" for an unknown code) |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-errstr ( .A r0 -- )` |
| **hylang** | `sys-errstr .A=n r0=buf(32) -> buf=z` |

An error code as text, for messages.

### KMESG

`$F806`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a buffer; r1 = its size, the most it copies; r2 = an offset in the messages held, 0 the oldest |
| **Out** | .A/.X = the bytes held, KMESG_SIZE at most; the buffer has those from r2, r1 at most |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-kmesg ( r0 r1 r2 -- .A/.X )` |
| **hylang** | `sys-kmesg r0=buf r1=count r2=n? -> buf .A/.X` |

The kernel's messages: what it printed on the bring-up console (the banner, POST, the boot's tasks, a driver's own lines), its last KMESG_SIZE bytes.  /dev/kmesg (kdev's #n/kmesg) is them as a file.

### REBOOT

`$F809`.

| | |
| :-- | :-- |
| **In** | .A = 0, or REBOOT_HWTEST |
| **Out** | (doesn't return) |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-reboot ( .A -- )` |
| **hylang** | `sys-reboot .A=n? ->` |

Start the system again, as the reset button does (the VIA's and ACIA's interrupts quieted first: only the button resets the chips); with REBOOT_HWTEST, into the hardware test, as a T typed during POST does. Every task ends where it is: files written are on their disks already (the disks write through).

### XCALL

`$F80C`.

| | |
| :-- | :-- |
| **In** | r15 = a routine's address ($A000-$DFFF); r14 (its low byte) = the paged ROM bank it's in; .A, .X, .Y, the flags and r0-r13: the routine's |
| **Out** | .A, .X, .Y, the flags and r0-r15 as the routine leaves them |
| **Errors** | - |
| **Waits** | no |

Call a routine in another bank of the paged ROM (a library module's: HT_LIBRARY, its bank from MODINFO), as the X16's jsrfar: the caller's bank register ($01) is set to the routine's for the call and back after it, and everything else passes through both ways.  The routine runs in the caller's task, with its RAM and zero page.  A note may come while it runs: a program that makes far calls keeps its note handler in RAM, as a module of several banks does.

---

## task (`$F830`)

Tasks: starting and ending them, waiting, scheduling, ticks: 32 slots from `$F830`, 29 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`EXITS`](#exits) | `$F830` | End the calling task, with an exit status (Plan 9's exits). |
| [`WAIT`](#wait) | `$F833` | Wait for a child to end, and take its exit status. |
| [`SPAWN`](#spawn) | `$F836` | Start a program in a task of its own. |
| [`GETPID`](#getpid) | `$F839` | The calling task's number (0-15). |
| [`YIELD`](#yield) | `$F83C` | Let the other tasks run; come back when it's this one's turn again (at once, if nothing else can run). |
| [`SLEEP`](#sleep) | `$F83F` | Sleep for a number of ticks. |
| [`SLEEP_UNTIL`](#sleep_until) | `$F842` | Sleep until the tick count reaches a value. |
| [`TICKS`](#ticks) | `$F845` | The scheduler's tick count. |
| [`PREEMPT_OFF`](#preempt_off) | `$F848` | Hold the CPU. |
| [`PREEMPT_ON`](#preempt_on) | `$F84B` | Undo PREEMPT_OFF; a task switch that came due meanwhile happens now. |
| [`TASKINFO`](#taskinfo) | `$F84E` | What the kernel knows of a task, for ps (and /proc's source). |
| [`NOTIFY`](#notify) | `$F851` | Set the task's note handler (Plan 9's notify). |
| [`NOTE`](#note) | `$F854` | Send a note (Plan 9's postnote). |
| [`MODINFO`](#modinfo) | `$F857` | A module in the paged ROM (SPAWN's #m/NAME), as the module directory has it. |
| [`ENV_GET`](#env_get) | `$F85A` | A variable of a task's environment (each task has its own: ENV_MAX bytes; SPAWN copies its parent's). |
| [`ENV_PUT`](#env_put) | `$F85D` | Set a variable, or make it (offset 0). |
| [`ENV_DEL`](#env_del) | `$F860` | Remove a variable. |
| [`ENV_NAME`](#env_name) | `$F863` | The variables of an environment, one at a time, in the order they were made. |
| [`TASKREAD`](#taskread) | `$F866` | Another task's arguments, current directory, environment, registers or open files, as they are now. |
| [`GETPPID`](#getppid) | `$F869` | The task that started this one (Plan 9's getppid). |
| [`SEM_NEW`](#sem_new) | `$F86C` | Make a semaphore, for tasks that share something (a shared segment, a device) or wait for each other. |
| [`SEM_ACQUIRE`](#sem_acquire) | `$F86F` | Take one of a semaphore's count; if there's none, wait (using no CPU) till SEM_RELEASE gives one back. |
| [`SEM_TRY`](#sem_try) | `$F872` | SEM_ACQUIRE without the wait. |
| [`SEM_RELEASE`](#sem_release) | `$F875` | Give one back to a semaphore's count, and wake the tasks waiting for it. |
| [`SEM_FREE`](#sem_free) | `$F878` | Free a semaphore (any task may). |
| [`EXEC`](#exec) | `$F87B` | Run a program in this task, in place of the one that calls it (Plan 9's exec). |
| [`SEM_WAIT`](#sem_wait) | `$F87E` | SEM_ACQUIRE with a time limit. |
| [`RUN`](#run) | `$F881` | Run a bank program in this task, as a subroutine, and wait for it. |
| [`TASKFORE`](#taskfore) | `$F884` | Put a task in the foreground, or take it out. |

### EXITS

`$F830`.

| | |
| :-- | :-- |
| **In** | .A = the exit code (0: success); r0 = a message (zero-terminated, 31 characters at most), or 0 for none |
| **Out** | (doesn't return) |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-exits ( .A r0 -- )` |
| **hylang** | `sys-exits .A=n? r0=s? ->` |

End the calling task, with an exit status (Plan 9's exits).  Returning from a program's entry point is EXITS with code 0 and no message.  Everything the task had is released; its parent can WAIT for the status.

### WAIT

`$F833`.

| | |
| :-- | :-- |
| **In** | .A = a task this one started, or $FF for any; r0 = a buffer for the message (32 bytes), or 0 |
| **Out** | .A = the task that ended; .X = its exit code; its message in r0's buffer ("" for none) |
| **Errors** | E_CHILD (no such child, or none at all), E_INTR (a note came: taken on the way out) |
| **Waits** | yes |
| **HyForth** | `sys-wait ( .A r0 -- .A .X ior )` |
| **hylang** | `sys-wait .A=n r0=buf(32) -> .A .X buf=z` |

Wait for a child to end, and take its exit status.  A child that has ended already isn't waited for.

### SPAWN

`$F836`.

| | |
| :-- | :-- |
| **In** | r0 = the program's path (through the namespace: /bin/NAME; or #m/NAME, a module in the paged ROM); r1 = its arguments (zero-terminated strings, an empty one after the last: 176 bytes at most), or 0; .A = flags (SPAWN_NEWGROUP, SPAWN_NEWNS, SPAWN_FDMAP, SPAWN_NOENV, SPAWN_STOPPED, SPAWN_RUN, RUN_LEND); r2 = an fd map (with SPAWN_FDMAP) |
| **Out** | .A = the new task (SPAWN_RUN: this one, if the program ran in it) |
| **Errors** | E_NOENT (no such file), E_NOEXEC (not a program this kernel runs), E_NOTASK (no free task), E_TOOBIG (the arguments), E_INVAL (the map), E_MFILE, E_INTR (and what else OPEN and READ give) |
| **Waits** | yes |
| **HyForth** | `sys-spawn ( r0 r1 .A r2 -- .A ior )` |
| **hylang** | `sys-spawn r0=s r1=args? .A=n? r2=map? -> .A` |

Start a program in a task of its own.  SPAWN opens the file and reads its HYX2 header (HX_*): a module of the paged ROM (HF_INPLACE: #m's files) runs in place, in its bank (the module directory's of its name); any other program is read into the new task's RAM at its load address ($0800), by the task itself as it starts, and its BSS cleared (one that can't be read ends at once, with the error as its exit code).  It starts at its entry point with r0 pointing to its arguments (at TASK_ARGS: the list), its name the header's.  Its fds: the caller's 0, 1 and 2; or with SPAWN_FDMAP, the map's: a count (SPAWN_FDS at most), then for each of the child's fds from 0 the caller's fd that becomes it ($FF, or a closed one: closed).  Each shares the caller's channel and offset; the rest are closed.  Its parent is the caller, which can WAIT for it.  It's in the caller's note group unless SPAWN_NEWGROUP starts one (its number: the child's), and it shares the caller's namespace unless SPAWN_NEWNS gives it an empty one.  Its environment is a copy of the caller's (SPAWN_NOENV: empty).  SPAWN_STOPPED: it's stopped at its entry point, for the debugger (TASKSTEP; /proc/N/ctl's step, or start).  A bank program (HF_BANK) is loaded into a RAM bank of its task's own and runs there.  SPAWN_RUN, for a caller that waits for the program at once: a bank program that may run in place (not HF_TASK) runs in this task (RUN, with the map and RUN_LEND as given) before SPAWN returns, and the task given back is this one, whose WAIT gives the program's exit status; any other program, or with SPAWN_NEWGROUP, SPAWN_NEWNS, SPAWN_NOENV or SPAWN_STOPPED, starts in a task of its own as ever.  rc runs a simple command this way.

### GETPID

`$F839`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A = this task's number |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-getpid ( -- .A )` |
| **hylang** | `sys-getpid -> .A` |

The calling task's number (0-15).

### YIELD

`$F83C`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-yield ( -- )` |
| **hylang** | `sys-yield ->` |

Let the other tasks run; come back when it's this one's turn again (at once, if nothing else can run).

### SLEEP

`$F83F`.

| | |
| :-- | :-- |
| **In** | .A/.X = ticks (TICK_HZ a second; 0-32767) |
| **Out** | - |
| **Errors** | E_INTR (a note woke it early: taken on the way out) |
| **Waits** | yes |
| **HyForth** | `sys-sleep ( .A/.X -- ior )` |
| **hylang** | `sys-sleep .A/.X=n ->` |

Sleep for a number of ticks.  The other tasks run meanwhile, or the system idles.

### SLEEP_UNTIL

`$F842`.

| | |
| :-- | :-- |
| **In** | .A/.X = a tick count (at most 32767 ticks ahead) |
| **Out** | - |
| **Errors** | E_INTR (a note woke it early: taken on the way out) |
| **Waits** | yes |
| **HyForth** | `sys-sleep-until ( .A/.X -- ior )` |
| **hylang** | `sys-sleep-until .A/.X=n ->` |

Sleep until the tick count reaches a value: for steady timing (a time that's passed returns at once).

### TICKS

`$F845`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A/.X = the tick count (TICK_HZ a second; it wraps after about 5.5 minutes) |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-ticks ( -- .A/.X )` |
| **hylang** | `sys-ticks -> .A/.X` |

The scheduler's tick count.

### PREEMPT_OFF

`$F848`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-preempt-off ( -- )` |
| **hylang** | `sys-preempt-off ->` |

Hold the CPU: no task switch until the matching PREEMPT_ON (they nest).  Interrupts keep running.

### PREEMPT_ON

`$F84B`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-preempt-on ( -- )` |
| **hylang** | `sys-preempt-on ->` |

Undo PREEMPT_OFF; a task switch that came due meanwhile happens now.

### TASKINFO

`$F84E`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task (0-15); r0 = a buffer (TI_SIZE bytes) |
| **Out** | the buffer: TI_STATE, TI_FLAGS, TI_PARENT, TI_CPU (3 bytes: ticks), TI_BANK, TI_TYPE, TI_NAME (16) |
| **Errors** | E_SRCH (not a task) |
| **Waits** | no |
| **HyForth** | `sys-taskinfo ( .A r0 -- ior )` |
| **hylang** | `sys-taskinfo .A=n r0=buf(TI_SIZE) -> buf` |

What the kernel knows of a task, for ps (and /proc's source).  A task's CPU time is the ticks that interrupted it.  TI_STATE is 0 for a free task.

### NOTIFY

`$F851`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = the note handler, or 0 for none |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |

Set the task's note handler (Plan 9's notify).  A note runs it in the task, in its own code (never in the middle of a system call), with .A = the note: it returns C = 0 to go on where the task was, or C = 1 for the default; it may call EXITS.  A note that comes meanwhile waits for it to return.

### NOTE

`$F854`.

| | |
| :-- | :-- |
| **In** | .A = a task, or NOTE_GROUP \| a note group; .X = the note (1-31: NOTE_*) |
| **Out** | - |
| **Errors** | E_INVAL (no such note), E_SRCH (no task, or nobody in the group), E_PERM (the kernel task, a driver) |
| **Waits** | no |
| **HyForth** | `sys-note ( .A .X -- ior )` |
| **hylang** | `sys-note .A=n .X=n ->` |

Send a note (Plan 9's postnote).  It wakes the task if it waits: WAIT, PAUSE, SLEEP and GETC end with E_INTR.  With no handler, and for NOTE_KILL always, the task ends with 128 + the note's Unix number (130 for NOTE_INTERRUPT, 137 for NOTE_KILL) and its name as the message.

### MODINFO

`$F857`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an entry of the module directory (0 on); r0 = a buffer (ME_SIZE bytes) |
| **Out** | the buffer: ME_BANK, ME_BANKS, ME_TYPE (HT_*), ME_FLAGS (HF_*), ME_NAME (11, zero-padded), ME_PAGE |
| **Errors** | E_NOENT (past the last) |
| **Waits** | no |
| **HyForth** | `sys-modinfo ( .A r0 -- ior )` |
| **hylang** | `sys-modinfo .A=n r0=buf(ME_SIZE) -> buf` |

A module in the paged ROM (SPAWN's #m/NAME), as the module directory has it.  A module of a bank may share it with others: its header is at $A000 + 256 * ME_PAGE there (0 for a module of banks).

### ENV_GET

`$F85A`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task ($FF: this one); r0 = a variable's name; r1 = a buffer; r2 = its size; r3 = an offset in the value |
| **Out** | .A/.X = the value's length from the offset (0 at its end or past it); as much of it as fits, in the buffer |
| **Errors** | E_NOENT (no such variable), E_SRCH (no such task), E_INVAL (an empty name, or one with a /), E_NAMETOOLONG |
| **Waits** | no |
| **HyForth** | `sys-env-get ( .A r0 r1 r2 r3 -- .A/.X ior )` |
| **hylang** | `sys-env-get .A=n r0=s r1=buf r2=size r3=n? -> .A/.X buf=.A/.X` |

A variable of a task's environment (each task has its own: ENV_MAX bytes; SPAWN copies its parent's). A value is any bytes (rc's lists: each word and a 0).  #e serves the caller's as files, /env/NAME.

### ENV_PUT

`$F85D`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task ($FF: this one); r0 = a variable's name; r1 = bytes; r2 = their count; r3 = where in its value they go (the value cut there first: 0 for a new value; its length at most) |
| **Out** | - |
| **Errors** | E_NOMEM (no room in the environment: ENV_MAX bytes; or no RAM module for one) E_INVAL (past the value's end; a bad name), E_SRCH E_NOENT (an offset given, and no such variable), E_NAMETOOLONG |
| **Waits** | no |
| **HyForth** | `sys-env-put ( .A r0 r1 r2 r3 -- ior )` |
| **hylang** | `sys-env-put .A=n r0=s r1=bytes(r2) r3=n? ->` |

Set a variable, or make it (offset 0).  An offset is for writing a long value in parts.

### ENV_DEL

`$F860`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task ($FF: this one); r0 = a variable's name |
| **Out** | - |
| **Errors** | E_NOENT, E_SRCH, E_INVAL, E_NAMETOOLONG |
| **Waits** | no |
| **HyForth** | `sys-env-del ( .A r0 -- ior )` |
| **hylang** | `sys-env-del .A=n r0=s ->` |

Remove a variable.

### ENV_NAME

`$F863`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task ($FF: this one); .X = which variable (0 on); r0 = a buffer (ENV_NAME_MAX + 1 bytes) |
| **Out** | its name in the buffer, zero-terminated; .A/.X = its value's length |
| **Errors** | E_NOENT (past the last), E_SRCH |
| **Waits** | no |
| **HyForth** | `sys-env-name ( .A .X r0 -- .A/.X ior )` |
| **hylang** | `sys-env-name .A=n .X=n r0=buf(ENV_NAME_MAX+1) -> buf=z .A/.X` |

The variables of an environment, one at a time, in the order they were made.

### TASKREAD

`$F866`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task ($FF: this one); .X = what: TR_ARGS (its arguments: ARGS_MAX bytes), TR_CWD (its current directory: PATH_MAX + 1 bytes), TR_ENV (its environment's first variables, whole, and a 0: ENV_SIZE bytes at most), TR_FRAME (its state and the frame it left as it was switched out: TF_SIZE bytes, TF_*) or TR_ENVAT (part of its environment), or TR_FD (its fd r2: FI_SIZE bytes, FI_*); r0 = a buffer that big; r1 = TR_ENVAT's count (the most it copies); r2 = TR_ENVAT's offset, or TR_FD's fd |
| **Out** | the bytes in the buffer; .A/.X = TR_ENVAT's: the environment's bytes in use (its 0 too: it copied as many as there are from r2, r1 at most) |
| **Errors** | E_SRCH, E_INVAL (another what), E_BADF (TR_FD: not an open fd) |
| **Waits** | no |
| **HyForth** | `sys-taskread ( .A .X r0 r1 r2 -- .A/.X ior )` |
| **hylang** | `sys-taskread .A=n .X=n r0=buf r1=count r2=n? -> buf .A/.X` |

Another task's arguments, current directory, environment, registers or open files, as they are now: /proc's args, cwd, env, regs and fd, ps.  The kernel task copies them a byte at a time, with IRQs off for each.  An environment may be ENV_MAX bytes: TR_ENVAT reads one whole, a part at a time.

### GETPPID

`$F869`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A = this task's parent ($FF: none) |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-getppid ( -- .A )` |
| **hylang** | `sys-getppid -> .A` |

The task that started this one (Plan 9's getppid): init once that one has ended; none for init, the drivers and the kernel task.

### SEM_NEW

`$F86C`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = its count: how many can take it before a taker waits (0-255); .X = 0, or SEM_MUTEX: a mutex (its count 1, and it has a holder: only the task that took it gives it back) |
| **Out** | .A = the semaphore (0 to SEM_MAX - 1) |
| **Errors** | E_NOMEM (none free: SEM_MAX in all) |
| **Waits** | no |
| **HyForth** | `sys-sem-new ( .A .X -- .A ior )` |
| **hylang** | `sys-sem-new .A=n .X=n? -> .A` |

Make a semaphore, for tasks that share something (a shared segment, a device) or wait for each other. Every task sees the same ones, by number.  It lasts till SEM_FREE, or the end of the task that made it.

### SEM_ACQUIRE

`$F86F`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a semaphore |
| **Out** | - |
| **Errors** | E_INVAL (not a semaphore, or freed while this task waited), E_BUSY (a mutex this task holds already), E_INTR (a note came: taken on the way out) |
| **Waits** | yes |
| **HyForth** | `sys-sem-acquire ( .A -- ior )` |
| **hylang** | `sys-sem-acquire .A=n ->` |

Take one of a semaphore's count; if there's none, wait (using no CPU) till SEM_RELEASE gives one back.  A mutex's holder is then this task.

### SEM_TRY

`$F872`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a semaphore |
| **Out** | - |
| **Errors** | E_INVAL (not a semaphore), E_BUSY (a mutex this task holds already), E_AGAIN (none to take) |
| **Waits** | no |
| **HyForth** | `sys-sem-try ( .A -- ior )` |
| **hylang** | `sys-sem-try .A=n ->` |

SEM_ACQUIRE without the wait.

### SEM_RELEASE

`$F875`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a semaphore |
| **Out** | - |
| **Errors** | E_INVAL (not a semaphore), E_PERM (a mutex this task doesn't hold), E_RANGE (its count is 255) |
| **Waits** | no |
| **HyForth** | `sys-sem-release ( .A -- ior )` |
| **hylang** | `sys-sem-release .A=n ->` |

Give one back to a semaphore's count, and wake the tasks waiting for it: the first to run takes it.

### SEM_FREE

`$F878`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a semaphore |
| **Out** | - |
| **Errors** | E_INVAL (not a semaphore) |
| **Waits** | no |
| **HyForth** | `sys-sem-free ( .A -- ior )` |
| **hylang** | `sys-sem-free .A=n ->` |

Free a semaphore (any task may): its number can be made again, and the tasks waiting for it wake with E_INVAL.  The end of the task that made it frees it too, and a task's end gives back the mutexes it holds.

### EXEC

`$F87B`.

| | |
| :-- | :-- |
| **In** | r0 = the program's path (as SPAWN's); r1 = its arguments (as SPAWN's: 176 bytes at most), or 0 |
| **Out** | (doesn't return, once the program is found: it runs in this task's place) |
| **Errors** | E_NOENT (no such file), E_NOEXEC (not a program this kernel runs), E_TOOBIG (the arguments), E_PERM (a driver), E_INTR (and what else OPEN and READ give) |
| **Waits** | yes |
| **HyForth** | `sys-exec ( r0 r1 -- ior )` |
| **hylang** | `sys-exec r0=s r1=args? ->` |

Run a program in this task, in place of the one that calls it (Plan 9's exec): the program is found as SPAWN finds it (a module of the paged ROM runs in place; any other is read into this task's RAM at its load address), and starts at its entry point with r0 pointing to its arguments, its name the header's.  The task keeps what isn't the old program's: its number, its parent (whose WAIT gets the new program's exit status), its note group, every fd (a RAM program's file takes fd 15 as it loads), its current directory, its namespace and its environment, and its CPU time.  What was the old program's goes as at its end: its IRQ lines, shared segments, semaphores (and the mutexes it holds), and the device letters it served; its note handler; its pages and banks.  A note that comes meanwhile waits for the new program.  If the program can't be found or isn't one, EXEC returns the error and the caller goes on; if it's found but can't be read (a RAM program's file cut short), the task ends with the error as its code.  rc runs the last command of rc -c this way, and its exec built-in.

### SEM_WAIT

`$F87E`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a semaphore; .X = the most ticks to wait (TICK_HZ a second; 0: none, as SEM_TRY) |
| **Out** | - |
| **Errors** | E_AGAIN (the time came and none to take), E_INVAL (not a semaphore, or freed while this task waited), E_BUSY (a mutex this task holds already), E_INTR (a note came: taken on the way out) |
| **Waits** | yes |
| **HyForth** | `sys-sem-wait ( .A .X -- ior )` |
| **hylang** | `sys-sem-wait .A=n .X=n ->` |

SEM_ACQUIRE with a time limit: take one of a semaphore's count; if there's none, wait (using no CPU) till SEM_RELEASE gives one back or the ticks have passed, whichever comes first.  A doorbell with a timeout: a task with work of its own to do every so often (a clock to keep) sleeps on its semaphore, and is woken at once when it's rung.

### RUN

`$F881`.

| | |
| :-- | :-- |
| **In** | r0 = a bank program's path (as SPAWN's); r1 = its arguments (as SPAWN's), or 0; .A = flags (SPAWN_FDMAP, RUN_LEND); r2 = an fd map (with SPAWN_FDMAP, as SPAWN's).  None of them in the window ($8000-$9FFF) |
| **Out** | .A = its exit code; r0 = its exit message (31 characters at most: TA_SCRATCH, "" for none) |
| **Errors** | E_NOEXEC (not a bank program that runs in place: SPAWN it), E_NOENT, E_BUSY (running already), E_NOMEM (no bank for it), E_TOOBIG, E_FAULT (a pointer in the window), E_INTR (and what STAT and READ give) |
| **Waits** | yes |
| **HyForth** | `sys-run ( r0 r1 .A r2 -- .A r0 ior )` |
| **hylang** | `sys-run r0=s r1=args? .A=n? r2=map? -> .A` |

Run a bank program in this task, as a subroutine, and wait for it.  A bank program (HF_BANK, not HF_TASK) is kept in this task's code cache: RAM banks of its own (eight at most, BANKS_ALLOC's), several programs to a bank, one above the other, each loaded the first time it's run (its file's o65 module, relocated where it's put) and kept for the next; when there's no room the one run least lately is given up.  The program is known by its file (STAT's), so a changed file is loaded again.  It runs with its bank at $8000 and its zero page in the program's ($22 up), on this task's stack: its arguments at r0, its fds this task's 0, 1 and 2 (SPAWN_FDMAP: the map's), no note handler (the task keeps its name).  It ends as a program does (returning from main, EXITS, a note's default: Ctrl-C), and then the task is the caller's again: its fds (the program's closed), note handler, zero page, banks at $8000 and $A000 and break; a note that ended the program (but a kill) is posted again, for the caller's own handler.  RUN_LEND lets the program move the break past the caller's (BREAK: memory the caller doesn't use; it's put back after); without it, BREAK fails.  The banks and pages the program takes (and not gives back) are the task's till it ends.  A program running in place can RUN another.

### TASKFORE

`$F884`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task: this one, or one it started; .X = 1 to put it in the foreground, 0 to take it out |
| **Out** | - |
| **Errors** | E_PERM (the kernel task, a driver, or another program's task), E_SRCH (not a task, or not started) |
| **Waits** | no |
| **HyForth** | `sys-taskfore ( .A .X -- ior )` |
| **hylang** | `sys-taskfore .A=n .X=n ->` |

Put a task in the foreground, or take it out.  The scheduler picks a task in the foreground before the others whenever it can run: at the next switch (a tick at most), not after a round of everything else that's busy; and a driver serving its call goes first with it.  For a program whose answer to a key runs through tasks of its own while others work in the background (a game's: its UI, parser and world first; its monsters' minds after).  It can't starve the rest: the foreground has FORE_RUN ticks at most, then the background the rest of a tick (its tasks in turn), so every task gets its turn.  A task waits in the foreground as any task waits (sleeping, on a semaphore: no CPU).  TASKINFO's TI_FLAGS has TF_FORE while it's in.  It goes with the task: gone when the task ends, or EXECs another program.

---

## memory (`$F890`)

Memory: the break, pages, banks, shared segments (phase 1.8): 16 slots from `$F890`, 15 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`BREAK`](#break) | `$F890` | Move the end of the program's data area. |
| [`PAGES_ALLOC`](#pages_alloc) | `$F893` | A run of pages of the task's RAM, above the break, taken from the top down. |
| [`PAGES_FREE`](#pages_free) | `$F896` | Give pages back. |
| [`BANKS`](#banks) | `$F899` | The task's own RAM banks. |
| [`BANKS_ALLOC`](#banks_alloc) | `$F89C` | A run of the task's banks, from the lowest. |
| [`BANKS_FREE`](#banks_free) | `$F89F` | Give banks back. |
| [`SEG_CREATE`](#seg_create) | `$F8A2` | A shared segment. |
| [`SEG_ATTACH`](#seg_attach) | `$F8A5` | Attach the calling task to a segment (attaching twice is once). |
| [`SEG_DETACH`](#seg_detach) | `$F8A8` | Detach the calling task from a segment; the last task to go frees it. |
| [`SEG_MAP`](#seg_map) | `$F8AB` | How to reach a bank of a segment. |
| [`SEGINFO`](#seginfo) | `$F8AE` | The shared RAM, as free shows it. |
| [`BANKS_ALLOC_IN`](#banks_alloc_in) | `$F8B1` | BANKS_ALLOC, from a range of the task's banks (the RAM disk's start FROM-TO: the modules its banks are on). |
| [`SEG_CREATE_IN`](#seg_create_in) | `$F8B4` | SEG_CREATE, from a range of the shared RAM (the shared RAM disk's start FROM-TO). |
| [`BANK_LOAD`](#bank_load) | `$F8B7` | A child (a bank program of a caller's, bankprog.h's: its functions called by the caller itself) in this task's code cache, as RUN keeps programs there (several to a bank, the one used least lately given up when there's no room), loaded if it isn't. |
| [`BANK_FLUSH`](#bank_flush) | `$F8BA` | Give up every program and child in this task's code cache (but a program running), and free the banks left empty. |

### BREAK

`$F890`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = the new end of the program's data (the first byte past it), or 0 to ask |
| **Out** | r0 = the end |
| **Errors** | E_INVAL (below the program's own data), E_NOMEM (over pages PAGES_ALLOC gave, or past the top of RAM) |
| **Waits** | no |
| **HyForth** | `sys-break ( r0 -- r0 ior )` |
| **hylang** | `sys-break r0=n? -> r0` |

Move the end of the program's data area: Unix's brk (BRK is the 65C02's instruction, so the call is BREAK).  It starts at the top of the module's data and BSS.  The top of a task's RAM is $7FFF ($7EFF in task F).

### PAGES_ALLOC

`$F893`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = pages (256 bytes each) |
| **Out** | r0 = the first one's address; .A = its page |
| **Errors** | E_INVAL (0), E_NOMEM |
| **Waits** | no |
| **HyForth** | `sys-pages-alloc ( .A -- r0 .A ior )` |
| **hylang** | `sys-pages-alloc .A=n -> r0 .A` |

A run of pages of the task's RAM, above the break, taken from the top down: for a language runtime's heap, or a buffer.

### PAGES_FREE

`$F896`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = pages; r0 = the first one's address |
| **Out** | - |
| **Errors** | E_INVAL (not pages PAGES_ALLOC gave this task) |
| **Waits** | no |
| **HyForth** | `sys-pages-free ( .A r0 -- ior )` |
| **hylang** | `sys-pages-free .A=n r0=n ->` |

Give pages back.

### BANKS

`$F899`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A = the banks the task has (16 for each good RAM module); r0 = the good modules (bit = module) |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-banks ( -- .A r0 )` |
| **hylang** | `sys-banks -> .A r0` |

The task's own RAM banks: module m's are $m0-$mF, selected by writing $00 (the X16's way).  Modules POST found bad aren't counted.

### BANKS_ALLOC

`$F89C`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = banks |
| **Out** | .A = the first (for $00) |
| **Errors** | E_INVAL (0), E_NOMEM |
| **Waits** | no |
| **HyForth** | `sys-banks-alloc ( .A -- .A ior )` |
| **hylang** | `sys-banks-alloc .A=n -> .A` |

A run of the task's banks, from the lowest.  Only bookkeeping, so that the libraries in one program don't use the same banks: a program may use its banks without it.

### BANKS_FREE

`$F89F`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = the first bank; .X = how many |
| **Out** | - |
| **Errors** | E_INVAL (not banks BANKS_ALLOC gave this task) |
| **Waits** | no |
| **HyForth** | `sys-banks-free ( .A .X -- ior )` |
| **hylang** | `sys-banks-free .A=n .X=n ->` |

Give banks back.

### SEG_CREATE

`$F8A2`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = banks (1-128, 8K each) |
| **Out** | .A = the segment (0-15) |
| **Errors** | E_INVAL (0), E_NOMEM |
| **Waits** | no |
| **HyForth** | `sys-seg-create ( .A -- .A ior )` |
| **hylang** | `sys-seg-create .A=n -> .A` |

A shared segment: shared RAM banks any task can attach to, with the calling task attached.  It stays while a task is attached (SEG_DETACH, or the task's end).

### SEG_ATTACH

`$F8A5`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a segment |
| **Out** | - |
| **Errors** | E_INVAL (no such segment) |
| **Waits** | no |
| **HyForth** | `sys-seg-attach ( .A -- ior )` |
| **hylang** | `sys-seg-attach .A=n ->` |

Attach the calling task to a segment (attaching twice is once).

### SEG_DETACH

`$F8A8`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a segment |
| **Out** | - |
| **Errors** | E_INVAL (not attached) |
| **Waits** | no |
| **HyForth** | `sys-seg-detach ( .A -- ior )` |
| **hylang** | `sys-seg-detach .A=n ->` |

Detach the calling task from a segment; the last task to go frees it.

### SEG_MAP

`$F8AB`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a segment; .X = one of its banks (0 on) |
| **Out** | .A = the value for U; .X = the value for $00 |
| **Errors** | E_INVAL (not attached), E_RANGE (no such bank in it) |
| **Waits** | no |
| **HyForth** | `sys-seg-map ( .A .X -- .A .X ior )` |
| **hylang** | `sys-seg-map .A=n .X=n -> .A .X` |

How to reach a bank of a segment: write U, then $00, and it's at $8000-$9FFF.

### SEGINFO

`$F8AE`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A = the shared banks segments can have (8K each: the good chips'); .X = those segments have now; r0 = the segments in use (bit = segment) |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-seginfo ( -- .A .X r0 )` |
| **hylang** | `sys-seginfo -> .A .X r0` |

The shared RAM, as free shows it.

### BANKS_ALLOC_IN

`$F8B1`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = how many; .X = the lowest bank it may take; .Y = the highest (module m's are $m0-$mF) |
| **Out** | .A = the first |
| **Errors** | E_INVAL (0), E_NOMEM (no run that long there) |
| **Waits** | no |
| **HyForth** | `sys-banks-alloc-in ( .A .X .Y -- .A ior )` |
| **hylang** | `sys-banks-alloc-in .A=n .X=n .Y=n -> .A` |

BANKS_ALLOC, from a range of the task's banks (the RAM disk's start FROM-TO: the modules its banks are on).

### SEG_CREATE_IN

`$F8B4`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = how many banks (1-128); .X = the lowest shared bank ID it may take; .Y = the highest (U << 4 \| the bank's low nibble; segments have $80-$FF) |
| **Out** | .A = the segment |
| **Errors** | E_INVAL (0), E_NOMEM (no run that long there) |
| **Waits** | no |
| **HyForth** | `sys-seg-create-in ( .A .X .Y -- .A ior )` |
| **hylang** | `sys-seg-create-in .A=n .X=n .Y=n -> .A` |

SEG_CREATE, from a range of the shared RAM (the shared RAM disk's start FROM-TO).

### BANK_LOAD

`$F8B7`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a child's path (an o65 module, linked at $8000: sdk/c/child.cfg); r1 = its caller's exports (NAME.exp), or 0; .A = where its zero page goes; .X = the bytes there it may have.  None of them in the window |
| **Out** | .A = its bank; .X = its directory entry (RB_DIR + 16 x .X in its bank: RE_GEN its load, RE_TABLE its table) |
| **Errors** | E_NOENT (its file; or an import the exports haven't), E_NOEXEC (not a child), E_RANGE (its zero page), E_IO (the exports), E_NOMEM, E_TOOBIG (no room), E_FAULT (a pointer in the window) |
| **Waits** | yes |
| **HyForth** | `sys-bank-load ( r0 r1 .A .X -- .A .X ior )` |
| **hylang** | `sys-bank-load r0=s r1=s? .A=n .X=n -> .A .X` |

A child (a bank program of a caller's, bankprog.h's: its functions called by the caller itself) in this task's code cache, as RUN keeps programs there (several to a bank, the one used least lately given up when there's no room), loaded if it isn't: its imports found by name in the caller's exports (its NAME.exp, the build's: "BX", 1, then each export its name's length, its name and its address, a 0 after the last), its zero page where the caller says.  Its exports' table (RE_TABLE: each its name, a 0, its address; a 0 after the last) gives its entries.  A call: its bank in the window ($00 alone), the entry's jsr, the bank put back; RE_GEN, as it was when it was found, says it's still there (it may have been given up for another meanwhile: then BANK_LOAD again), and the caller sets RE_STAMP from TK_RCLOCK (one more) as it calls.  On a failure .X says why (BL_*), and a missing import's name is in TA_SCRATCH.

### BANK_FLUSH

`$F8BA`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = the banks the code cache may have from now on (RC_BANKS to start with), or 0 (as it was) |
| **Out** | - |
| **Errors** | - |
| **Waits** | yes |
| **HyForth** | `sys-bank-flush ( .A -- )` |
| **hylang** | `sys-bank-flush .A=n ->` |

Give up every program and child in this task's code cache (but a program running), and free the banks left empty.

---

## file (`$F8C0`)

Files (phase 2): 32 slots from `$F8C0`, 17 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`OPEN`](#open) | `$F8C0` | Open a file. |
| [`CREATE`](#create) | `$F8C3` | Make a file or a directory, and open it. |
| [`CLOSE`](#close) | `$F8C6` | Close an fd. |
| [`READ`](#read) | `$F8C9` | Read from the fd's offset, and move it on. |
| [`WRITE`](#write) | `$F8CC` | Write at the fd's offset, and move it on. |
| [`SEEK`](#seek) | `$F8CF` | Move an fd's offset (shared by the fds that DUP or SPAWN made of it). |
| [`STAT`](#stat) | `$F8D2` | A file's stat record. |
| [`FSTAT`](#fstat) | `$F8D5` | An open file's stat record. |
| [`WSTAT`](#wstat) | `$F8D8` | Change a file from a stat record. |
| [`FWSTAT`](#fwstat) | `$F8DB` | Change an open file from a stat record. |
| [`REMOVE`](#remove) | `$F8DE` | Remove a file, or an empty directory. |
| [`DUP`](#dup) | `$F8E1` | Another fd for an open file. |
| [`DUP2`](#dup2) | `$F8E4` | Make fd .X name the same channel as fd .A, closing what it named first. |
| [`CHDIR`](#chdir) | `$F8E7` | Change the current directory. |
| [`GETCWD`](#getcwd) | `$F8EA` | The current directory. |
| [`PIPE`](#pipe) | `$F8ED` | A pipe (#\|). |
| [`FD2PATH`](#fd2path) | `$F8F0` | The name an fd's file was opened by (Plan 9's fd2path), whole and clean, as OPEN or CREATE made it (an fd SPAWN or DUP gave shares its channel, and its name). |

### OPEN

`$F8C0`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a path; .A = the mode (O_READ, O_WRITE, O_RDWR; \| O_TRUNC, O_NONBLOCK) |
| **Out** | .A = an fd |
| **Errors** | E_NOENT, E_NODEV (no such device), E_NAMETOOLONG, E_MFILE (no fd free), E_NFILE (no channel free) |
| **Waits** | yes |
| **HyForth** | `sys-open ( r0 .A -- .A ior )` |
| **hylang** | `sys-open r0=s .A=n? -> .A` |

Open a file.  Its server may answer other errors.  (Until namespaces, a path names a device: "#x", then "/" and the path in it.)

### CREATE

`$F8C3`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a path; .A = the mode; .X = the new file's mode, high byte (DM_DIR: a directory) |
| **Out** | .A = an fd |
| **Errors** | E_NOENT, E_NODEV, E_NAMETOOLONG, E_MFILE, E_NFILE, E_EXIST |
| **Waits** | yes |
| **HyForth** | `sys-create ( r0 .A .X -- .A ior )` |
| **hylang** | `sys-create r0=s .A=n? .X=n? -> .A` |

Make a file or a directory, and open it.

### CLOSE

`$F8C6`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd |
| **Out** | - |
| **Errors** | E_BADF (not open) |
| **Waits** | yes |
| **HyForth** | `sys-close ( .A -- ior )` |
| **hylang** | `sys-close .A=n ->` |

Close an fd.  The last fd naming a channel (DUP, SPAWN share them) tells its server.

### READ

`$F8C9`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd; r0 = a buffer; r1 = a count |
| **Out** | .A/.X = the count read (0: the end of the file) |
| **Errors** | E_BADF (not open for reading), E_AGAIN (non-blocking: nothing yet), E_INTR (a note: taken on the way out) |
| **Waits** | yes |
| **HyForth** | `sys-read ( .A r0 r1 -- .A/.X ior )` |
| **hylang** | `sys-read .A=n r0=buf r1=count -> buf=.A/.X` |

Read from the fd's offset, and move it on.  A count short of the one asked for is all there is for now (a console line, say).  A directory reads as stat records (SR_SIZE bytes each).

### WRITE

`$F8CC`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd; r0 = the data; r1 = its count |
| **Out** | .A/.X = the count written |
| **Errors** | E_BADF (not open for writing), E_AGAIN, E_INTR |
| **Waits** | yes |
| **HyForth** | `sys-write ( .A r0 r1 -- .A/.X ior )` |
| **hylang** | `sys-write .A=n r0=bytes(r1) -> .A/.X` |

Write at the fd's offset, and move it on.

### SEEK

`$F8CF`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd; r0, r1 = an offset (low word, high word: signed); .X = from: 0 the start, 1 the offset, 2 the end |
| **Out** | r0, r1 = the new offset |
| **Errors** | E_BADF, E_INVAL (before the start, or no such from) |
| **Waits** | yes |
| **HyForth** | `sys-seek ( .A r0/r1 .X -- r0/r1 ior )` |
| **hylang** | `sys-seek .A=n r0/r1=i .X=n? -> r0/r1` |

Move an fd's offset (shared by the fds that DUP or SPAWN made of it).

### STAT

`$F8D2`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a path; r1 = a buffer (SR_SIZE bytes) |
| **Out** | the file's stat record in the buffer (SR_*) |
| **Errors** | E_NOENT, E_NODEV, E_NAMETOOLONG, E_MFILE, E_NFILE |
| **Waits** | yes |
| **HyForth** | `sys-stat ( r0 r1 -- ior )` |
| **hylang** | `sys-stat r0=s r1=buf(SR_SIZE) -> buf=stat` |

A file's stat record: its name, qid, mode, length, time, device letter.

### FSTAT

`$F8D5`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd; r0 = a buffer (SR_SIZE bytes) |
| **Out** | the stat record in the buffer |
| **Errors** | E_BADF |
| **Waits** | yes |
| **HyForth** | `sys-fstat ( .A r0 -- ior )` |
| **hylang** | `sys-fstat .A=n r0=buf(SR_SIZE) -> buf=stat` |

An open file's stat record.

### WSTAT

`$F8D8`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a path; r1 = a stat record |
| **Out** | - |
| **Errors** | E_NOENT, E_NODEV, E_NAMETOOLONG, E_MFILE, E_NFILE, E_NOSYS (its server changes nothing) |
| **Waits** | yes |
| **HyForth** | `sys-wstat ( r0 r1 -- ior )` |
| **hylang** | `sys-wstat r0=s r1=stat ->` |

Change a file from a stat record: its name, its mode, its length (what its server allows: HydraFS a file's length, longer with zeros or shorter).  What's all $FF in the record (an empty name) is left as it is.

### FWSTAT

`$F8DB`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd; r0 = a stat record |
| **Out** | - |
| **Errors** | E_BADF, E_NOSYS |
| **Waits** | yes |
| **HyForth** | `sys-fwstat ( .A r0 -- ior )` |
| **hylang** | `sys-fwstat .A=n r0=stat ->` |

Change an open file from a stat record.

### REMOVE

`$F8DE`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a path |
| **Out** | - |
| **Errors** | E_NOENT, E_NODEV, E_NAMETOOLONG, E_NOTEMPTY, E_NOSYS |
| **Waits** | yes |
| **HyForth** | `sys-remove ( r0 -- ior )` |
| **hylang** | `sys-remove r0=s ->` |

Remove a file, or an empty directory.

### DUP

`$F8E1`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd |
| **Out** | .A = a new fd (the lowest free), on the same channel (the same offset) |
| **Errors** | E_BADF, E_MFILE |
| **Waits** | no |
| **HyForth** | `sys-dup ( .A -- .A ior )` |
| **hylang** | `sys-dup .A=n -> .A` |

Another fd for an open file.

### DUP2

`$F8E4`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd; .X = another (0-15) |
| **Out** | - |
| **Errors** | E_BADF |
| **Waits** | yes |
| **HyForth** | `sys-dup2 ( .A .X -- ior )` |
| **hylang** | `sys-dup2 .A=n .X=n ->` |

Make fd .X name the same channel as fd .A, closing what it named first.

### CHDIR

`$F8E7`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a directory's path |
| **Out** | - |
| **Errors** | E_NOTDIR, E_NOENT, E_NODEV, E_NAMETOOLONG |
| **Waits** | yes |
| **HyForth** | `sys-chdir ( r0 -- ior )` |
| **hylang** | `sys-chdir r0=s ->` |

Change the current directory: the name a relative path starts from.  It's kept whole and clean (GETCWD).

### GETCWD

`$F8EA`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = a buffer (PATH_MAX + 1 bytes) |
| **Out** | .A = the current directory's length; the directory in the buffer, zero-terminated |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-getcwd ( r0 -- .A )` |
| **hylang** | `sys-getcwd r0=buf(PATH_MAX+1) -> buf=z` |

The current directory.

### PIPE

`$F8ED`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A = an fd to read from; .X = an fd to write to |
| **Errors** | E_NODEV (no pipe server), E_MFILE, E_NFILE, E_NOMEM (no pipe free) |
| **Waits** | yes |
| **HyForth** | `sys-pipe ( -- .A .X ior )` |
| **hylang** | `sys-pipe -> .A .X` |

A pipe (#|): what's written to one end is read from the other, 512 bytes held between them.  A read waits for something to read, and is the end (0) once no writer is left; a write waits for room, and is E_PIPE once no reader is left.

### FD2PATH

`$F8F0`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = an fd; r0 = a buffer (PATH_MAX + 1 bytes) |
| **Out** | the name in the buffer, zero-terminated |
| **Errors** | E_BADF (not an open fd) |
| **Waits** | no |
| **HyForth** | `sys-fd2path ( .A r0 -- ior )` |
| **hylang** | `sys-fd2path .A=n r0=buf(PATH_MAX+1) -> buf=z` |

The name an fd's file was opened by (Plan 9's fd2path), whole and clean, as OPEN or CREATE made it (an fd SPAWN or DUP gave shares its channel, and its name): the namespace's, or a # name; a pipe's #|/pipe.

---

## name (`$F920`)

Namespaces (phase 2): 16 slots from `$F920`, 4 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`BIND`](#bind) | `$F920` | Bind new at old. |
| [`MOUNT`](#mount) | `$F923` | Mount a device's server, with a spec (which of its file trees: '#f' with x is the ROM disk), at old, as BIND does. |
| [`UNMOUNT`](#unmount) | `$F926` | Undo a bind or a mount at old. |
| [`NSINFO`](#nsinfo) | `$F929` | A namespace's mount entries, one at a time, in the order of the kernel's table. |

### BIND

`$F920`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = new (a path); r1 = old (a path: absolute); .A = MREPL, MBEFORE or MAFTER, and MCREATE |
| **Out** | - |
| **Errors** | E_NOENT, E_INVAL, E_NSFULL, E_NODEV, E_NAMETOOLONG |
| **Waits** | yes |
| **HyForth** | `sys-bind ( r0 r1 .A -- ior )` |
| **hylang** | `sys-bind r0=s r1=s .A=n? ->` |

Bind new at old: in old's place (MREPL), or a union with what's there, new first (MBEFORE) or last (MAFTER); with MCREATE, a CREATE in the union comes to new.  New is what it is now: a mount point itself is all of its union's members, anything else the first of its candidates that's there.  A union's first bind at a directory that's there (and isn't a mount point itself) keeps it as a member.

### MOUNT

`$F923`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .X = a device letter; r0 = its spec (a string, 8 characters at most), or 0; r1 = old; .A = BIND's flags |
| **Out** | - |
| **Errors** | E_NODEV, E_NOENT, E_INVAL, E_NSFULL, E_NAMETOOLONG |
| **Waits** | yes |
| **HyForth** | `sys-mount ( .X r0 r1 .A -- ior )` |
| **hylang** | `sys-mount .X=n r0=s? r1=s .A=n? ->` |

Mount a device's server, with a spec (which of its file trees: '#f' with x is the ROM disk), at old, as BIND does.

### UNMOUNT

`$F926`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = new, or 0; r1 = old, or 0 |
| **Out** | - |
| **Errors** | E_NOENT, E_INVAL, E_NSFULL, E_NAMETOOLONG |
| **Waits** | yes |
| **HyForth** | `sys-unmount ( r0 r1 -- ior )` |
| **hylang** | `sys-unmount r0=s? r1=s? ->` |

Undo a bind or a mount at old: all of what's there (r0 = 0), or new's member (as BIND finds new).  With no old either (r1 = 0), everything: the caller has an empty namespace of its own (the tasks it shared one with keep theirs): what Plan 9's newns starts from, and nslib's ns_default does first.

### NSINFO

`$F929`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task ($FF: this one); .X = which entry of its namespace (0 on); r0 = a buffer (NI_SIZE bytes) |
| **Out** | the entry in the buffer (NI_*): its mount point, its device letter and spec, the path in that device, its flags (MCREATE) and its place among the union's members |
| **Errors** | E_RANGE (past the last), E_SRCH |
| **Waits** | no |
| **HyForth** | `sys-nsinfo ( .A .X r0 -- ior )` |
| **hylang** | `sys-nsinfo .A=n .X=n r0=buf(NI_SIZE) -> buf` |

A namespace's mount entries, one at a time, in the order of the kernel's table: each a member of the union at its mount point, the members in NI_SEQ's order (lowest first).  /proc's ns, and ns.

---

## cons (`$F950`)

Console helpers: a byte or a string to stdout, a byte from stdin: 16 slots from `$F950`, 4 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`PUTC`](#putc) | `$F950` | Write a byte to stdout (the X16's CHROUT). |
| [`PUTS`](#puts) | `$F953` | Write a string to stdout. |
| [`GETC`](#getc) | `$F956` | Read a byte from stdin, waiting for one (the X16's CHRIN, unechoed). |
| [`PUTHEX`](#puthex) | `$F959` | Write a byte to stdout as two hex digits (for debugging). |

### PUTC

`$F950`.

| | |
| :-- | :-- |
| **In** | .A = a byte |
| **Out** | - |
| **Errors** | - |
| **Waits** | yes |
| **HyForth** | `sys-putc ( .A -- )` |
| **hylang** | `sys-putc .A=n ->` |

Write a byte to stdout (the X16's CHROUT).

### PUTS

`$F953`.

| | |
| :-- | :-- |
| **In** | r0 = a string, zero-terminated |
| **Out** | - |
| **Errors** | E_INTR (a note came while it waited: taken on the way out) |
| **Waits** | yes |
| **HyForth** | `sys-puts ( r0 -- ior )` |
| **hylang** | `sys-puts r0=s ->` |

Write a string to stdout.

### GETC

`$F956`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A = a byte |
| **Errors** | E_EOF (the end of stdin), E_INTR (a note came: taken on the way out) |
| **Waits** | yes |
| **HyForth** | `sys-getc ( -- .A ior )` |
| **hylang** | `sys-getc -> .A` |

Read a byte from stdin, waiting for one (the X16's CHRIN, unechoed).

### PUTHEX

`$F959`.

| | |
| :-- | :-- |
| **In** | .A = a byte |
| **Out** | - |
| **Errors** | - |
| **Waits** | yes |
| **HyForth** | `sys-puthex ( .A -- )` |
| **hylang** | `sys-puthex .A=n ->` |

Write a byte to stdout as two hex digits (for debugging).

---

## time (`$F980`)

The clock (phase 5): 16 slots from `$F980`, 3 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`TIME`](#time) | `$F980` | The clock. |
| [`TIME_SET`](#time_set) | `$F983` | Set the clock (from now: the boot's time becomes the clock less the ticks since). |
| [`RTC`](#rtc) | `$F986` | The DS1747 in U7, its registers a byte at a time (task F's $7FF8-$7FFF). |

### TIME

`$F980`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | - |
| **Out** | r0/r1 = the clock: seconds since 2000-01-01 00:00:00 |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-time ( -- r0/r1 )` |
| **hylang** | `sys-time -> r0/r1` |

The clock: the boot's time and the ticks since.  It starts at 0 (2000-01-01) unless kdev finds a DS1747 with a date and time as it starts; a write of /dev/time sets it (and the chip).

### TIME_SET

`$F983`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0/r1 = seconds since 2000-01-01 00:00:00 |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |
| **HyForth** | `sys-time-set ( r0/r1 -- )` |
| **hylang** | `sys-time-set r0/r1=n ->` |

Set the clock (from now: the boot's time becomes the clock less the ticks since).  The DS1747 is the caller's to set (RTC).

### RTC

`$F986`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = 0, read, or 1, write; r0 = 8 bytes: the DS1747's registers (RTC_REGS, hw.inc), BCD |
| **Out** | - |
| **Errors** | E_INVAL (.A neither) |
| **Waits** | no |
| **HyForth** | `sys-rtc ( .A r0 -- ior )` |
| **hylang** | `sys-rtc .A=n r0=io(8) -> buf` |

The DS1747 in U7, its registers a byte at a time (task F's $7FF8-$7FFF): read with its updates halted (R) for the copy; written with W set, the control (its century) last with W clear, which starts it from them.  No chip: the RAM there, as it is.  A read must check the fields hold a date and time.

---

## server (`$F9B0`)

Servers and drivers: 32 slots from `$F9B0`, 17 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`IRQ_OWN`](#irq_own) | `$F9B0` | Own an IRQ line. |
| [`IRQ_RELEASE`](#irq_release) | `$F9B3` | Give an IRQ line back. |
| [`WAKE`](#wake) | `$F9B6` | Make a waiting task (PAUSE) runnable. |
| [`PAUSE`](#pause) | `$F9B9` | Wait until another task or an irq entry WAKEs this one, or a note comes (taken on the way out). |
| [`NOTE_POST`](#note_post) | `$F9BC` | NOTE without taking the caller's own notes. |
| [`SRV_REGISTER`](#srv_register) | `$F9BF` | Serve a device. |
| [`SRV_TAKE`](#srv_take) | `$F9C2` | In the serve entry. |
| [`SRV_REPLY`](#srv_reply) | `$F9C5` | At the end of the serve entry. |
| [`CLIENT_READ`](#client_read) | `$F9C8` | Bytes from the client of the request being served (kcopy). |
| [`CLIENT_WRITE`](#client_write) | `$F9CB` | Bytes to the client of the request being served (kcopy). |
| [`NOTE_QUEUE`](#note_queue) | `$F9CE` | A note to a note group, from an irq entry (a console's Ctrl-C). |
| [`ROMREAD`](#romread) | `$F9D1` | Copy from any bank of the paged ROM, for a module run in place (the storage driver's ROM disk). |
| [`TASKMEM`](#taskmem) | `$F9D4` | Bytes from a task's memory into the buffer, or from the buffer into it. |
| [`TASKSTOP`](#taskstop) | `$F9D7` | Stop a task where it is (the scheduler passes it by: its waits go on, and end, but it doesn't run), or start it again. |
| [`TASKSTEP`](#taskstep) | `$F9DA` | The debugger's step (/proc's ctl: kdev). |
| [`SRV_NAP`](#srv_nap) | `$F9DD` | For a driver with work of its own between its calls (a device to look at now and then). |
| [`INIT_RUN`](#init_run) | `$F9E0` | Start init (the module the directory names) again, in a task of its own, with the number in decimal as its argument. |

### IRQ_OWN

`$F9B0`.

| | |
| :-- | :-- |
| **In** | .A = an IRQ line (0-14), or LINE_VIA_T2 (VIA timer 2), or LINE_VIA_CA1 (VIA CA1) |
| **Out** | - |
| **Errors** | E_BUSY (another task owns it), E_RANGE (not a line), E_INVAL (the task has no IRQ entry) |
| **Waits** | no |

Own an IRQ line: its interrupts run the calling task's irq entry, in the calling task.  The entry gets the line in .A, and returns .A = 0, or IRQ_RESCHED to ask for a task switch (it woke a task).  It runs with interrupts off and must not make calls that wait, and with the dispatch's own cycles it must stay under the 200-cycle budget: about 70 cycles of its own (TASK_EVENT, not WAKE, to wake clients; NOTE_QUEUE for a group).  A task's lines are released when it ends.  LINE_VIA_T2 is VIA timer 2: owning it sets the timer one-shot with its interrupt on; the owner starts it (T2CL, then T2CH), and its entry clears the interrupt (reading T2CL, or starting it again).  LINE_VIA_CA1 is CA1: owning it turns CA1's interrupt on (off as it's given back), and the entry clears CA1's flag (IFR).

### IRQ_RELEASE

`$F9B3`.

| | |
| :-- | :-- |
| **In** | .A = an IRQ line |
| **Out** | - |
| **Errors** | E_PERM (not the caller's), E_RANGE |
| **Waits** | no |

Give an IRQ line back.

### WAKE

`$F9B6`.

| | |
| :-- | :-- |
| **In** | .A = a task |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |

Make a waiting task (PAUSE) runnable.  From a task or an irq entry.  Waking a task that isn't waiting does nothing.

### PAUSE

`$F9B9`.

| | |
| :-- | :-- |
| **In** | - |
| **Out** | - |
| **Errors** | - |
| **Waits** | yes |

Wait until another task or an irq entry WAKEs this one, or a note comes (taken on the way out). (Wake-ups can come early: look again after.)

### NOTE_POST

`$F9BC`.

| | |
| :-- | :-- |
| **In** | .A = a task, or NOTE_GROUP \| a note group; .X = the note |
| **Out** | - |
| **Errors** | E_INVAL, E_SRCH, E_PERM |
| **Waits** | no |

NOTE without taking the caller's own notes: the note posted, the tasks woken.  From a task, or an irq entry for a task (a group is too many tasks to look at with interrupts off: NOTE_QUEUE).

### SRV_REGISTER

`$F9BF`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a device letter (printable; not #) |
| **Out** | - |
| **Errors** | E_INVAL (not a letter), E_EXIST (another task's), E_NOMEM (no room) |
| **Waits** | no |

Serve a device: its requests ("#x...") come to the calling task's serve entry.  A task may serve several letters (the request block names it: RQ_DEV).  Freed when the task ends.

### SRV_TAKE

`$F9C2`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | - |
| **Out** | .A = the request (R_*); .Y = the client; the request block in TASK_INBOX, and (R_OPEN, R_CREATE, R_REMOVE) the name in TASK_PATH |
| **Errors** | - |
| **Waits** | no |

In the serve entry: take the request being served from the client (srvlib does it).  It notes the server's event count (TASK_EVENT) in RQ_EVENT: a client answered E_AGAIN waits for it to change.

### SRV_REPLY

`$F9C5`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | TASK_INBOX's RQ_FID, RQ_DONE, RQ_PERM, RQ_EVENT: what the request returns; .A, C: the answer |
| **Out** | .A and C kept |
| **Errors** | - |
| **Waits** | no |

At the end of the serve entry: what the request returns, back to the client (srvlib does it).

### CLIENT_READ

`$F9C8`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = here; r1 = there, in the client's view; r2 = a count |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |

Bytes from the client of the request being served (kcopy).

### CLIENT_WRITE

`$F9CB`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | r0 = here (the data); r1 = there, in the client's view; r2 = a count |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |

Bytes to the client of the request being served (kcopy).

### NOTE_QUEUE

`$F9CE`.

| | |
| :-- | :-- |
| **In** | IRQs off; .X = a note group (0-15); .A = the note's bit: 1 << (note - 1), for a note 1-8 |
| **Out** | .X kept |
| **Errors** | - |
| **Waits** | no |

A note to a note group, from an irq entry (a console's Ctrl-C): queued, and posted by the kernel task as soon as it runs.  Short enough for an irq entry: nothing is checked.

### ROMREAD

`$F9D1`.

| | |
| :-- | :-- |
| **In** | .A = a paged ROM bank; r0 = where in it ($A000-$DFFF); r1 = a buffer (not at $A000-$DFFF); r2 = a count |
| **Out** | the bytes in the buffer |
| **Errors** | E_PERM (the task owns an IRQ line: its irq entry would find the wrong bank at $A000) |
| **Waits** | no |

Copy from any bank of the paged ROM, for a module run in place (the storage driver's ROM disk): the bank is the task's own meanwhile, and its module's is put back.

### TASKMEM

`$F9D4`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task; .X = TM_READ or TM_WRITE, \| TM_BANK; r0 = a buffer; r1 = an address in the task's view, $0000-$DFFF (with TM_BANK: $8000-$9FFF, in its RAM bank r3); r2 = a count (the range ends at $E000, or with TM_BANK at $A000, at most) |
| **Out** | - |
| **Errors** | E_PERM (the caller isn't a driver), E_SRCH (not a task), E_INVAL (past the range) |
| **Waits** | no |

Bytes from a task's memory into the buffer, or from the buffer into it: /proc's mem and ram (kdev, which decides who may: a program reaches another task's memory only through those files).  Its task RAM, and its RAM bank and paged ROM bank as it has them selected; or with TM_BANK any of its RAM banks (its bank register points at it meanwhile: not for a driver, whose irq entry might run).  A byte at a time, IRQs off for each.

### TASKSTOP

`$F9D7`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task; .X = 1 to stop it, 0 to start it again |
| **Out** | - |
| **Errors** | E_PERM (the caller isn't a driver; the kernel task, a driver), E_SRCH (not a task, or not started) |
| **Waits** | no |

Stop a task where it is (the scheduler passes it by: its waits go on, and end, but it doesn't run), or start it again: /proc's ctl (kdev, which decides who may), for the debugger.  TASKINFO's TI_FLAGS has TF_STOPPED while it's stopped.  A kill note starts it, to end it.

### TASKSTEP

`$F9DA`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | .A = a task; .X = TS_STEP, TS_NEXT, TS_BREAKS or TS_NOBREAKS |
| **Out** | - |
| **Errors** | E_PERM (the caller isn't a driver; the kernel task, a driver), E_SRCH (not a task, or not started), E_INVAL (another .X; to step: a BRK or STP), E_BUSY (to step: not stopped, or a step still out) (or in the kernel: in a call, its PC at $E000 or above, or W not 0) |
| **Waits** | no |

The debugger's step (/proc's ctl: kdev).  TS_STEP: a stopped task runs its next instruction and stops again (TI_FLAGS: TF_STOPPED once it has).  JMP, JSR, RTS and RTI are done at once, on its frame; others run out of line, in its zero page, a BRK after them.  TS_NEXT: the same, but a JSR runs its subroutine whole, as a JSR to the kernel's jump table always does (a subroutine that reads bytes after its JSR can't be stepped over).  TS_BREAKS: from now any BRK stops the task, its PC back on the BRK (the debugger's breakpoints, BRKs it writes into the program); TS_NOBREAKS: a BRK is the program's own again (NOTE_BRK).  SPAWN_STOPPED starts a program stopped at its entry point.

### SRV_NAP

`$F9DD`.

| | |
| :-- | :-- |
| **In** | .A/.X = ticks (1-32767); or 0, and .Y = the driver's event count (TASK_EVENT) as it last saw it |
| **Out** | - |
| **Errors** | - |
| **Waits** | yes |

For a driver with work of its own between its calls (a device to look at now and then): its init doesn't return, but waits here between its turns.  While it waits it serves its calls, as a driver that has started does (the boot goes on); when the time has come it runs again, busy (a call to it waits) till its next SRV_NAP, or till its init returns (it's an ordinary driver from then).  With 0 ticks it waits till its own event count isn't .Y any more (its irq entry adds to it, or one of its calls): for work that follows what happens, not the clock.  ps shows it asleep, or waiting for an event.

### INIT_RUN

`$F9E0`.

| | |
| :-- | :-- |
| **In** | .A = a number, 0-15 |
| **Out** | - |
| **Errors** | - |
| **Waits** | no |

Start init (the module the directory names) again, in a task of its own, with the number in decimal as its argument: as the boot starts it (no parent, no fds, an empty namespace and environment, a note group of its own), but the boot's has no argument.  The kernel task starts it as it next runs; with no task free it isn't started (but 0's, which waits for one).  The kernel itself asks for 0's when the task init was last started in for 0 (or by the boot) has EXECed and then ends: so an init that EXECs the login shell is started again when that shell ends (an init that ends itself is left ended).  HydraOS's console asks for n's when the user makes window n.  From a task or an irq entry.

---

## dbg (`$FA10`)

Debugging and tests: unstable, may change in any version: 16 slots from `$FA10`, 3 used.

| Call | Address | What it does |
| :--- | :------ | :----------- |
| [`DBG_SCALL`](#dbg_scall) | `$FA10` | Call a driver's serve entry directly, in the driver's task (the file layer will make these calls in phase 2). |
| [`DBG_KCOPY`](#dbg_kcopy) | `$FA13` | Copy bytes between this task's memory and another's (the kernel's kcopy). |
| [`DBG_PS`](#dbg_ps) | `$FA16` | Print a line for each task in use. |

### DBG_SCALL

`$FA10`.

| | |
| :-- | :-- |
| **In** | .Y = a driver task; .A, .X = what its serve entry gets |
| **Out** | .A, .X and C from its serve entry |
| **Errors** | E_SRCH (not a task), E_NODEV (it's free or ended, or has no serve entry: a program) |
| **Waits** | yes |

Call a driver's serve entry directly, in the driver's task (the file layer will make these calls in phase 2).  A driver serves one call at a time: a second caller waits.

### DBG_KCOPY

`$FA13`.

| | |
| :-- | :-- |
| **In** | .A = another task; r0 = an address in this task; r1 = an address in that task; r2 = a byte count; C = 0 to copy from this task to that one, C = 1 the other way |
| **Out** | - |
| **Errors** | E_SRCH (not a task) |
| **Waits** | no |

Copy bytes between this task's memory and another's (the kernel's kcopy).

### DBG_PS

`$FA16`, a far call (its routine on another BIOS page).

| | |
| :-- | :-- |
| **In** | - |
| **Out** | - |
| **Errors** | - |
| **Waits** | yes |

Print a line for each task in use: its number, state, flags, parent, CPU time and name.

---

## The error codes

A call that fails returns C = 1 and one of these in `.A`.  `ERRSTR` gives the text; the C library sets `errno` to the code's POSIX equivalent (and `_oserror` to the code itself).

| Code | Name | Text |
| :--- | :--- | :--- |
| `$01` | `E_PERM` | not allowed |
| `$02` | `E_INVAL` | invalid argument |
| `$03` | `E_NOSYS` | no such call |
| `$04` | `E_AGAIN` | not yet: try again |
| `$05` | `E_INTR` | interrupted |
| `$06` | `E_NOMEM` | out of memory |
| `$07` | `E_BUSY` | busy |
| `$08` | `E_RANGE` | out of range |
| `$09` | `E_FAULT` | bad address |
| `$0A` | `E_NAMETOOLONG` | name too long |
| `$0B` | `E_TOOBIG` | too big |
| `$20` | `E_NOENT` | not found |
| `$21` | `E_EXIST` | already exists |
| `$22` | `E_NOTDIR` | not a directory |
| `$23` | `E_ISDIR` | is a directory |
| `$24` | `E_NOTEMPTY` | directory not empty |
| `$25` | `E_BADF` | bad file descriptor |
| `$26` | `E_MFILE` | no free file descriptor |
| `$27` | `E_NFILE` | too many open files |
| `$28` | `E_NOSPC` | disk full |
| `$29` | `E_ROFS` | read-only |
| `$2A` | `E_IO` | i/o error |
| `$2B` | `E_NODEV` | no such device |
| `$2C` | `E_PIPE` | broken pipe |
| `$2D` | `E_NOEXEC` | not a program |
| `$2E` | `E_EOF` | end of file |
| `$2F` | `E_NOTFS` | no file system |
| `$30` | `E_MEDIA` | medium error |
| `$40` | `E_NOTASK` | no free task |
| `$41` | `E_SRCH` | no such task |
| `$42` | `E_CHILD` | no such child |
| `$50` | `E_NSFULL` | namespace full |
| `$51` | `E_NSLOOP` | too many binds |

---

## The constants

What `hydra.inc` defines besides the calls and the error codes (`hydracalls.h` has them for C, with `HY_` before each name).

| Name | Value | What it is |
| :--- | :---- | :--------- |
| `ABI_VERSION` | `1` | The ABI version this include describes |
| `TICK_HZ` | `200` | Ticks a second |
| `INIT_TASK` | `1` | init's task (and its note group: the console's foreground at first) |
| `PROG_ZP` | `$22` | A program's own zero page: $22-$7F |
| `PROG_ZP_END` | `$80` |  |
| `RB_DIR` | `$9F00` | A code cache bank's directory (BANK_LOAD): its entries at RB_DIR + 16 x n, n 1-15 |
| `RE_START` | `0` | A cache directory entry: where its module is (2) |
| `RE_END` | `2` | A cache directory entry: the first byte past it (2) |
| `RE_TABLE` | `4` | A cache directory entry: a child's exports' table (2) |
| `RE_GEN` | `6` | A cache directory entry: its load's number (0: free) |
| `RE_FLAGS` | `7` | A cache directory entry: RF_* |
| `RE_STAMP` | `8` | A cache directory entry: when it was last used (TK_RCLOCK) (2) |
| `TK_RCLOCK` | `$EC` | A task's code cache's uses counted (2): its zero page's |
| `RC_BANKS` | `8` | A task's code cache's banks at most, to start with (BANK_FLUSH) |
| `BL_FILE` | `2` | BANK_LOAD's why (.X): its file not there, or not read |
| `BL_FORMAT` | `3` | BANK_LOAD's why (.X): not a child (o65, linked at $8000, in a bank) |
| `BL_ZP` | `4` | BANK_LOAD's why (.X): its zero page more than the caller gives |
| `BL_EXPORTS` | `5` | BANK_LOAD's why (.X): the caller's exports' file not there, or not of its form |
| `BL_IMPORT` | `6` | BANK_LOAD's why (.X): an import the caller hasn't (its name in TA_SCRATCH) |
| `BL_BANK` | `7` | BANK_LOAD's why (.X): no bank, or no room in one |
| `TA_SCRATCH` | `$0280` | A task's call scratch (32 bytes): an exit message, BANK_LOAD's missing import |
| `TASK_ARGS` | `$0350` | A task's arguments: zero-terminated strings, an empty one after the last (176 bytes at most) |
| `ARGS_MAX` | `176` | SPAWN's arguments: their bytes at most (each string's 0, and the empty one that ends them) |
| `TASK_NAME` | `$0230` | A task's name, zero-terminated |
| `IRQ_RESCHED` | `$80` | An irq entry's answer: a task switch, please |
| `LINE_VIA_T2` | `16` | IRQ_OWN's line for VIA timer 2 (the hardware's lines: hw.inc) |
| `LINE_VIA_CA1` | `17` | IRQ_OWN's line for VIA CA1 (J27's pin 11): owning it turns CA1's interrupt on; the owner clears its flag |
| `TASK_EVENT` | `$BD` | A task's event count: a server adds 1 (inc TASK_EVENT) when what its clients wait for has happened |
| `HT_PROGRAM` | `1` | HYX2 module types |
| `HT_DRIVER` | `2` |  |
| `HT_LIBRARY` | `3` |  |
| `HF_INPLACE` | `$01` | HYX2 flags: runs in place in the paged ROM |
| `HF_BOOT` | `$02` | HYX2 flags: a driver the kernel starts at boot |
| `HF_BANK` | `$04` | HYX2 flags: a bank program: an o65 module after the header, run in a RAM bank (RUN) |
| `HF_TASK` | `$08` | HYX2 flags: a bank program that runs only in a task of its own (not in place: RUN) |
| `HX_MAGIC` | `0` | The HYX2 header (HX_SIZE bytes, a program's first): HYX2 |
| `HX_HSIZE` | `4` | HYX2: the header's size (48) |
| `HX_TYPE` | `5` | HYX2: the type (HT_*) |
| `HX_FLAGS` | `6` | HYX2: the flags (HF_*) |
| `HX_ABI` | `7` | HYX2: the ABI version it was built for |
| `HX_LOAD` | `8` | HYX2: its load address (2: $A000 in place, $0800 a RAM program) |
| `HX_LENGTH` | `10` | HYX2: its image's length (2: in its last bank, in place) |
| `HX_DATA_LOAD` | `12` | HYX2: its data, where it is in the image (2) ... |
| `HX_DATA_RUN` | `14` | HYX2: ... where it runs (2) ... |
| `HX_DATA_LEN` | `16` | HYX2: ... and its length (2) |
| `HX_BSS` | `18` | HYX2: its BSS (2) ... |
| `HX_BSS_LEN` | `20` | HYX2: ... and its length (2: cleared as it starts) |
| `HX_TOP` | `22` | HYX2: the top of the RAM it uses (2: its break starts there) |
| `HX_MAIN` | `24` | HYX2: entry: main (a program), init (a driver) |
| `HX_SERVE` | `26` | HYX2: entry: serve (a driver) |
| `HX_IRQ` | `28` | HYX2: entry: irq (a driver) |
| `HX_STOP` | `30` | HYX2: entry: stop (a driver) |
| `HX_DEVICE` | `32` | HYX2: its device letter (a driver) |
| `HX_BANKS` | `33` | HYX2: the banks it spans (in place) |
| `HX_VERSION` | `34` | HYX2: its version (2) |
| `HX_NAME` | `36` | HYX2: its name (12: zero-terminated) |
| `HX_SIZE` | `48` |  |
| `ME_BANK` | `0` | MODINFO's answer (ME_SIZE bytes): the module's first paged ROM bank |
| `ME_BANKS` | `1` | MODINFO: the banks it spans |
| `ME_TYPE` | `2` | MODINFO: its type (HT_*) |
| `ME_FLAGS` | `3` | MODINFO: its flags (HF_*) |
| `ME_NAME` | `4` | MODINFO: its name (11 bytes, zero-padded: 10 characters at most) |
| `ME_PAGE` | `15` | MODINFO: its header's page in its first bank, 0-63 ($A000 + 256 * it: a module of a bank, packed with others) |
| `ME_SIZE` | `16` |  |
| `TI_STATE` | `0` | TASKINFO's answer: the task's state (0: free, 1 ready, 2 wait, 3 call, 4 idle, 5 new, 6 sleep, 7 blocked, 8 event) |
| `TI_FLAGS` | `1` | TASKINFO: its flags (TF_DRIVER, TF_FORE, TF_STOPPED) |
| `TF_DRIVER` | `$01` | TI_FLAGS: a driver |
| `TF_STOPPED` | `$80` | TI_FLAGS: stopped (/proc's ctl: TASKSTOP) |
| `TF_FORE` | `$02` | TI_FLAGS: in the foreground (TASKFORE): the scheduler picks it first |
| `FORE_RUN` | `8` | The foreground's turn at most, in ticks (TASKFORE): then the background has the rest of a tick |
| `TI_PARENT` | `2` | TASKINFO: its parent ($FF: none) |
| `TI_CPU` | `3` | TASKINFO: its CPU time, in ticks (3 bytes) |
| `TI_BANK` | `6` | TASKINFO: its module's paged ROM bank ($FF: none) |
| `TI_TYPE` | `7` | TASKINFO: its module's type (HT_*; 0: none) |
| `TI_NAME` | `8` | TASKINFO: its name, zero-terminated (16 bytes) |
| `TI_GROUP` | `24` | TASKINFO: its note group |
| `TI_SIZE` | `25` | TASKINFO: the answer's size |
| `NOTE_INTERRUPT` | `1` | Notes: Ctrl-C (default: the end, 130) |
| `NOTE_KILL` | `2` | Notes: kill, never caught (137) |
| `NOTE_HANGUP` | `3` | Notes: hangup (129) |
| `NOTE_ALARM` | `4` | Notes: alarm (142) |
| `NOTE_BRK` | `5` | Notes: sys: brk, a BRK instruction (133) |
| `NOTE_USER` | `16` | Notes: 16-31 are the programs' own (128 + the note) |
| `NOTE_GROUP` | `$80` | NOTE's .A: a note group, not a task |
| `SEM_MUTEX` | `$01` | SEM_NEW's .X: a mutex (its count 1, given back only by its holder) |
| `REBOOT_HWTEST` | `$01` | REBOOT's .A: into the hardware test |
| `SEM_MAX` | `16` | Semaphores: as many as there can be at once |
| `SPAWN_NEWGROUP` | `$01` | SPAWN's flags: the child starts a note group of its own |
| `SPAWN_NEWNS` | `$02` | SPAWN's flags: the child starts with an empty namespace of its own (else it shares its parent's) |
| `SPAWN_FDMAP` | `$04` | SPAWN's flags: the child's fds are r2's map (else the caller's 0, 1 and 2) |
| `SPAWN_NOENV` | `$08` | SPAWN's flags: the child starts with an empty environment (else a copy of its parent's) |
| `SPAWN_STOPPED` | `$10` | SPAWN's flags: the child stops at its entry point, for the debugger (/proc/N/ctl's step) |
| `SPAWN_RUN` | `$20` | SPAWN's flags: a bank program that runs in place does, in this task (RUN), and SPAWN gives this task back |
| `RUN_LEND` | `$40` | RUN's flags (and SPAWN_RUN's): the program may move the break past the caller's |
| `ENV_SIZE` | `1024` | TASKREAD's TR_ENV: its buffer, an environment's first variables, whole, and a 0 |
| `ENV_MAX` | `8192` | An environment's bytes at most: each variable its name's length, its name, its value's length (2), its value; a 0 |
| `ENV_NAME_MAX` | `31` | A variable's name's length at most |
| `SPAWN_FDS` | `15` | SPAWN's fd map: the child's fds it can give, at most (its fd 15 is its loader's as it starts) |
| `MREPL` | `0` | BIND's and MOUNT's flags: in place of what's at old |
| `MBEFORE` | `1` | BIND's and MOUNT's flags: a union, new first |
| `MAFTER` | `2` | BIND's and MOUNT's flags: a union, new last |
| `MCREATE` | `4` | BIND's and MOUNT's flags: a CREATE in the union comes to new |
| `EXIT_MSG_MAX` | `31` | An exit message's length at most (WAIT's buffer: 32 bytes) |
| `KMESG_SIZE` | `4096` | KMESG: the kernel's messages held at most (then the oldest go) |
| `TR_ARGS` | `0` | TASKREAD: a task's arguments (ARGS_MAX bytes) |
| `TR_CWD` | `1` | TASKREAD: a task's current directory (PATH_MAX + 1 bytes) |
| `TR_ENV` | `2` | TASKREAD: a task's environment's first variables, whole, and a 0 (ENV_SIZE bytes at most) |
| `TR_FRAME` | `3` | TASKREAD: a task's state and frame (TF_SIZE bytes) |
| `TR_ENVAT` | `4` | TASKREAD: part of a task's environment (r1 bytes at most, from offset r2; .A/.X its bytes in use) |
| `TR_FD` | `5` | TASKREAD: a task's fd r2 (FI_SIZE bytes) |
| `FI_NAME` | `0` | TR_FD's: the name its file was opened by (PATH_MAX + 1 bytes, zero-terminated) ... |
| `FI_MODE` | `64` | TR_FD's: its open mode (O_*) ... |
| `FI_DEV` | `65` | TR_FD's: its server's device letter ... |
| `FI_QTYPE` | `66` | TR_FD's: its qid type (QT_DIR ...) ... |
| `FI_OFFSET` | `67` | TR_FD's: and its offset (4 bytes, low first) |
| `FI_SIZE` | `71` |  |
| `TF_STATE` | `0` | TR_FRAME's: the task's state (0 free, 1 ready, 2 waiting, 3 calling, 4 idle, 5 new, 6 sleeping, 7 blocked, 8 waiting for an event) |
| `TF_S` | `1` | TR_FRAME's: its stack pointer, as it was before the frame |
| `TF_U` | `2` | TR_FRAME's: the frame: U |
| `TF_Y` | `3` |  |
| `TF_W` | `4` |  |
| `TF_X` | `5` |  |
| `TF_A` | `6` |  |
| `TF_P` | `7` |  |
| `TF_PC` | `8` | TR_FRAME's: PC (2) |
| `TF_RAM` | `10` | TR_FRAME's: its RAM bank register ($00) |
| `TF_ROM` | `11` | TR_FRAME's: its paged ROM bank register ($01) |
| `TF_SIZE` | `12` |  |
| `TM_READ` | `0` | TASKMEM: from the task's memory |
| `TM_WRITE` | `$01` | TASKMEM: to the task's memory |
| `TM_BANK` | `$80` | TASKMEM's flag: its RAM bank r3, at $8000-$9FFF |
| `TS_STEP` | `0` | TASKSTEP: one instruction |
| `TS_NEXT` | `1` | TASKSTEP: one instruction, a JSR's subroutine whole |
| `TS_BREAKS` | `2` | TASKSTEP: from now, the task's BRKs stop it (the debugger's breakpoints) |
| `TS_NOBREAKS` | `3` | TASKSTEP: its BRKs are its own again (NOTE_BRK) |
| `NI_FROM` | `0` | NSINFO's answer (NI_SIZE bytes): the mount point (PATH_MAX + 1 bytes, zero-terminated) |
| `NI_PATH` | `64` | NSINFO: the path in the device (zero-terminated, no leading /) |
| `NI_DEV` | `128` | NSINFO: the device letter |
| `NI_SPEC` | `129` | NSINFO: the spec (8 bytes, zero-padded, and a 0) |
| `NI_FLAGS` | `138` | NSINFO: the flags (MCREATE) |
| `NI_SEQ` | `139` | NSINFO: its place among the union's members (lowest first) |
| `NI_SIZE` | `140` |  |
| `KEY_UP` | `$80` | Keys: a raw console read gives the terminal's cursor and function keys as one code each |
| `KEY_DOWN` | `$81` |  |
| `KEY_RIGHT` | `$82` |  |
| `KEY_LEFT` | `$83` |  |
| `KEY_HOME` | `$84` |  |
| `KEY_END` | `$85` |  |
| `KEY_INS` | `$86` |  |
| `KEY_DEL` | `$87` |  |
| `KEY_PGUP` | `$88` |  |
| `KEY_PGDN` | `$89` |  |
| `KEY_F1` | `$8A` | Keys: F1-F12 are $8A-$95 |
| `KEY_F2` | `$8B` |  |
| `KEY_F3` | `$8C` |  |
| `KEY_F4` | `$8D` |  |
| `KEY_F5` | `$8E` |  |
| `KEY_F6` | `$8F` |  |
| `KEY_F7` | `$90` |  |
| `KEY_F8` | `$91` |  |
| `KEY_F9` | `$92` |  |
| `KEY_F10` | `$93` |  |
| `KEY_F11` | `$94` |  |
| `KEY_F12` | `$95` |  |
| `KEY_FOCUS` | `$97` | Keys: the focus moved to another window of this one's group; its number is the next key (a raw read's, keys hydra's) |
| `KEY_RESIZE` | `$96` | Keys: the window's size changed (a raw read's, keys hydra's: consctl reads with the new one) |
| `KEY_MOD` | `$98` | Keys: a key the terminal sent modified (keys mods's): the modifiers next (1 Shift, 2 Alt, 4 Ctrl), then the key |
| `O_READ` | `0` | Open modes |
| `O_WRITE` | `1` |  |
| `O_RDWR` | `2` |  |
| `O_RW_MASK` | `3` |  |
| `O_TRUNC` | `$10` | Open modes: empty the file first |
| `O_NONBLOCK` | `$40` | Open modes: E_AGAIN rather than wait |
| `FD_MAX` | `16` | Fds a task has (0 stdin, 1 stdout, 2 stderr) |
| `PATH_MAX` | `63` | A path's length at most |
| `IO_UNIT` | `512` | The most a READ or WRITE asks of a server in one request |
| `SR_NAME` | `0` | The stat record (SR_SIZE bytes): its name (32, zero-terminated) |
| `SR_QTYPE` | `32` | Stat: the qid's type (QT_*) |
| `SR_QVERS` | `33` | Stat: the qid's version |
| `SR_QPATH` | `34` | Stat: the qid's path (4: unique in its server) |
| `SR_MODE` | `38` | Stat: the mode (2: permissions; high byte DM_DIR) |
| `SR_LENGTH` | `40` | Stat: the length (4) |
| `SR_MTIME` | `44` | Stat: the time it changed (4: seconds since 2000) |
| `SR_DEV` | `48` | Stat: the device letter |
| `SR_INST` | `49` | Stat: the device's instance (its spec's first character) |
| `SR_SIZE` | `64` |  |
| `QT_DIR` | `$80` | Qid types: a directory |
| `QT_FILE` | `$00` |  |
| `DM_DIR` | `$80` | A mode's high byte: a directory |
| `DM_APPEND` | `$40` | A mode's high byte: append-only (writes go to the end) |
| `TASK_INBOX` | `$0200` | A server's copy of the request it's serving (RQ_*) |
| `TASK_PATH` | `$02C0` | A server's copy of the name a request names (64 bytes) |
| `RQ_TYPE` | `0` | The request block (RQ_SIZE bytes): the request (R_*) |
| `RQ_FID` | `1` | Request: the fid (out: the new fid, R_OPEN and R_CREATE) |
| `RQ_MODE` | `2` | Request: the open mode |
| `RQ_CLIENT` | `3` | Request: the client task |
| `RQ_U` | `4` | Request: the client's U |
| `RQ_FLAGS` | `5` | Request: RF_* |
| `RQ_OFFSET` | `6` | Request: the offset (4) |
| `RQ_COUNT` | `10` | Request: the count asked for (2) |
| `RQ_DONE` | `12` | Request: the count done (2, out) |
| `RQ_BUF` | `14` | Request: the buffer, in the client's view (2) |
| `RQ_PERM` | `16` | Request: R_CREATE's new mode (high byte); out (R_OPEN, R_CREATE): the qid type |
| `RQ_NAMELEN` | `17` | Request: the name's length |
| `RQ_GROUP` | `18` | Request: the client's note group |
| `RQ_DEV` | `19` | Request: the device letter |
| `RQ_SPEC` | `20` | Request: the mount's spec (8, zero-padded) |
| `RQ_EVENT` | `28` | Request: the server's event count as it took it (after E_AGAIN, the client waits for a change) |
| `RQ_SIZE` | `32` |  |
| `RF_NONBLOCK` | `$01` | Request flags: answer E_AGAIN rather than wait |
| `R_OPEN` | `1` | Requests |
| `R_CREATE` | `2` |  |
| `R_READ` | `3` |  |
| `R_WRITE` | `4` |  |
| `R_CLUNK` | `5` |  |
| `R_STAT` | `6` |  |
| `R_WSTAT` | `7` |  |
| `R_REMOVE` | `8` |  |
| `R_FLUSH` | `9` |  |
| `R_DUP` | `10` |  |
| `SND_CHANNELS` | `24` | Sound (#a): its channels: 0-7 the YM2151's, 8-23 the Vera X's PSG (its voices 0-15) |
| `SND_PSG` | `8` | Sound: the PSG's first channel (its voice 0) |
| `SND_PATCHES` | `163` | Sound: its patches (0-127: General MIDI's programs; 128-162: drums and percussion) |
| `SND_DRUMS` | `128` | Sound: General MIDI's drums (MIDI channel 10's notes) |
| `SND_R_CH` | `$02` | Sound: a command, at a register the chip doesn't have: the channel the next commands are for (0-23) |
| `SND_R_PATCH` | `$03` | Sound: load patch n into the channel (its speakers stay) |
| `SND_R_NOTE` | `$04` | Sound: key on MIDI note n (60: middle C; 69: A, 440 Hz), with the channel's bend |
| `SND_R_OFF` | `$05` | Sound: key off (the release) |
| `SND_R_VOL` | `$06` | Sound: the channel's volume (0-127, as General MIDI's) |
| `SND_R_PAN` | `$07` | Sound: the channel's speakers (SND_PAN_*) |
| `SND_R_BEND` | `$09` | Sound: the channel's bend (signed: 64ths of a semitone; the note playing too) |
| `SND_R_DRUM` | `$0A` | Sound: a General MIDI drum (its patch and pitch), keyed on |
| `SND_R_FREQ_LO` | `$0B` | Sound: a frequency's low byte (Hz), for SND_R_FREQ (only kept) |
| `SND_R_FREQ` | `$0C` | Sound: key on at a frequency, SND_R_FREQ_LO's byte and this its high one (Hz; 0: key off) |
| `SND_R_GLIDE` | `$0D` | Sound: the channel's pitch to note n without a new attack (legato) |
| `SND_R_WAVE` | `$0E` | Sound: a PSG channel's waveform: SND_WAVE_* << 6 \| its pulse width (0-63: 63 a square) |
| `SND_PAN_LEFT` | `1` | Sound: SND_R_PAN's speakers |
| `SND_PAN_RIGHT` | `2` |  |
| `SND_PAN_BOTH` | `3` |  |
| `SND_WAVE_PULSE` | `0` | Sound: SND_R_WAVE's waveforms (the PSG's) |
| `SND_WAVE_SAW` | `1` |  |
| `SND_WAVE_TRIANGLE` | `2` |  |
| `SND_WAVE_NOISE` | `3` |  |
| `r0` | `$02` | The call registers r0-r15: arguments and results |
| `r1` | `$04` |  |
| `r2` | `$06` |  |
| `r3` | `$08` |  |
| `r4` | `$0A` |  |
| `r5` | `$0C` |  |
| `r6` | `$0E` |  |
| `r7` | `$10` |  |
| `r8` | `$12` |  |
| `r9` | `$14` |  |
| `r10` | `$16` |  |
| `r11` | `$18` |  |
| `r12` | `$1A` |  |
| `r13` | `$1C` |  |
| `r14` | `$1E` |  |
| `r15` | `$20` |  |

<!-- calls:end -->
