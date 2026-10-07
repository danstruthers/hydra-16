# Tasks, notes and time

## Starting a program

`SPAWN` starts a program in a task of its own: `r0` its path (through the namespace: `/bin/ls`, or `#m/ls`, a module
in the paged ROM), `r1` its arguments (zero-terminated strings, an empty one after the last; 176 bytes at most), `.A`
flags; `.A` gives the new task.  A module of the paged ROM runs in place; any other program is read into the new
task's RAM at its load address (`$0800`) by the task itself as it starts.

| Flag | Meaning |
| :--- | :------ |
| `SPAWN_NEWGROUP` | A note group of its own (else the caller's: Ctrl-C reaches both) |
| `SPAWN_NEWNS` | An empty namespace (else it shares the caller's) |
| `SPAWN_FDMAP` | Its fds from the map at `r2`: a count, then for each of its fds from 0 the caller's fd that becomes it (`$FF`: closed) |
| `SPAWN_NOENV` | An empty environment (else a copy of the caller's) |
| `SPAWN_STOPPED` | Stopped at its entry point, for a debugger (`TF_STOPPED` in its `TI_FLAGS` once it's there) |

The child gets the caller's fds 0, 1 and 2 (sharing their channels and offsets), its current directory, and a copy of
its environment.  Its parent is the caller (`GETPPID` says so).

## Ending, and waiting

`EXITS` ends the calling task with a status: `.A` a code (0 success), `r0` a message (31 characters) or 0.  Returning
from a program's entry point is `EXITS` with 0.  Everything the task had (its fds, pages, banks, segments' references,
semaphores, IRQ lines) is given back.

`WAIT` waits for a child to end (`.A` the task, or `$FF` any) and takes its status: `.A` the task, `.X` its code,
its message in `r0`'s buffer.  A child that ended first keeps its status till it's waited for; a parent that ends
first leaves its children to init.  rc shows a status as `$status`: the message, or the code.

## Notes

Notes are Plan 9's signals: numbered (1 Ctrl-C's `interrupt`, 2 `kill`, 3 `hangup`, 4 `alarm`, 5 `brk` a BRK
instruction; 16-31 a program's own), sent by `NOTE` (`.A` a task, or `NOTE_GROUP | g` a note group; `.X` the note)
or by writing to `/proc/N/note` or `ctl`.  Ctrl-C at a window sends `interrupt` to the group that has the window.

A task takes its notes in its own code, never inside the kernel: at a switch into it, or at the end of a system call
that waits (which then gives `E_INTR`).  With no handler, the default ends the task (128 + the Unix number: 130 for
Ctrl-C, 137 for a kill).  `NOTIFY` (`r0` a routine) sets a handler: it's called with `.A` the note, and returns C = 0
to go on where the task was, or C = 1 for the default.  A kill is never the handler's.

`sdk/asm/samples/tick/tick.s`:

```
main:
            LDR         r0, handler                         ; Its note handler
            jsr         NOTIFY
@second:
            lda         #<TICK_HZ                           ; A second (TICK_HZ ticks): a note ends it sooner
            ldx         #>TICK_HZ
            jsr         SLEEP
            lda         noted
            bne         @end
            ...

; The note handler: .A = the note.  C = 0: the task goes on where it was (here, SLEEP ends early); C = 1 would be the
; default
handler:
            sta         noted
            clc
            rts
```

A program that runs code in more than one bank (a module of several banks, or one that makes `XCALL`s) keeps its
handler in its RAM (its `DATA` segment): the kernel calls it with whichever bank is at `$A000` when the note comes.
In C, `signal (SIGINT, f)` is a handler for Ctrl-C's note.

## Time

`TICKS` is the tick count (`TICK_HZ`, 200 a second; 16 bits, so it wraps after about 5.5 minutes).  `SLEEP` waits
`.A/.X` ticks, `SLEEP_UNTIL` till a tick count (for steady timing), `YIELD` lets the others run.  `PREEMPT_OFF` and
`PREEMPT_ON` hold the CPU (no task switch; interrupts go on) for a few ticks' worth of timing-critical work.  `TIME`
is the clock, seconds since 2000-01-01 in `r0:r1`, set from the DS1747 at boot if there's one (`TIME_SET` sets it;
`/dev/time` is it as text).

## Tasks working together

A task shares nothing with another but what it's given: fds (pipes, files, devices), shared segments, semaphores, notes,
and its exit status.  The C SDK's multitasking demos
([../../sdk/c/README.md](../../sdk/c/README.md#the-multitasking-demos)) put them to work: each starts copies of itself
(`SPAWN`), passes them a segment's and semaphores' numbers as arguments, meets them at a barrier, draws them as they go,
and waits for their ends (`WAIT`).  `round` keeps four tasks in time with `SLEEP_UNTIL` alone, from a start they share.

## Seeing tasks

`TASKINFO` says what the kernel knows of a task (its state, flags, parent, CPU time, name, group: `TI_*`);
`TASKREAD` reads its arguments, current directory, environment, registers, or an open fd (`TR_*`).  `/proc/N` has
them as files (`status`, `args`, `cwd`, `env`, `regs`, `fd`, `ns`, `mem`, `ram`), with `note` and `ctl` (`kill`,
`interrupt`, `note N`, `stop`, `start`, `step`, `next`, `break`, `nobreak`) to act on it.  `ps` and `top` read them.

## Debugging

A debugger is a program over `/proc` (`db` is one: [../using/tools.md](../using/tools.md)).  It starts its program
with `SPAWN_STOPPED`, or stops a task with `ctl`'s `stop`; reads its registers with `TASKREAD`'s `TR_FRAME` and its
memory through `mem`; and steps it with `ctl`'s `step` (one instruction) or `next` (a `JSR`'s subroutine run whole),
waiting for `TF_STOPPED` again (`TASKINFO`).  Behind `ctl` is `TASKSTEP`, a driver's call (kdev's).  The kernel runs
the instruction out of line: a copy of it in the task's own zero page with a `BRK` after it, which stops the task again
with its PC where the instruction went (a branch has two `BRK`s, taken and not).  `JMP`, `JSR`, `RTS` and `RTI` it does
itself, on the task's frame; a `JSR` into the jump table always runs whole.  So a step is the instruction as the task
would have run it, in its own banks, and takes no breakpoint in a ROM.  A task in a call (its PC in the kernel, or
waiting) can't be stepped: `E_BUSY` till it's run on and stopped in its own code.

Breakpoints are the debugger's: a `BRK` written into the program's RAM through `mem`.  After `ctl`'s `break`, any
`BRK` stops the task, its PC back on the `BRK`, rather than giving it the note `sys: brk`; `nobreak` gives a `BRK`
back to the program.  A subroutine that reads the bytes after its `JSR` (its arguments, as some libraries' do) can't
be stepped over: its return address is in the kernel's copy.
