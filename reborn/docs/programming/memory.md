# Memory

## A task's 64K

| Where | What |
| :---- | :--- |
| `$00`, `$01` | Its bank registers: the RAM bank at `$8000`, and the paged ROM bank at `$A000` |
| `$02`-`$21` | `r0`-`r15`, the call registers |
| `$22`-`$7F` | The program's own zero page |
| `$80`-`$FF` | The system's (the task's state; one byte a program writes: `TASK_EVENT`, a server's event count) |
| `$0100`-`$01FF` | The stack |
| `$0200`-`$03FF` | The system's: the task's fds, name, arguments (`TASK_ARGS`, `$0350`), current directory ... |
| `$0400`-`$7FFF` | The program's: a RAM program from `$0800` (its header, code, data, then BSS); then its break; pages from the top down |
| `$8000`-`$9FFF` | The RAM bank `$00` picks: one of the task's own, or a shared one |
| `$A000`-`$DFFF` | The paged ROM bank `$01` picks: a module running in place has its own there |
| `$E000`-`$FFFF` | The BIOS ROM (page 0: the jump table at `$F800`); the I/O area `$FF00`-`$FFEF` |

Every task has its own copy of `$0000`-`$7FFF`, so a stray write lands in its own RAM, not another's.

## The break and pages

`BREAK` moves the end of the program's data (its break: `r0` the new end, or 0 to ask), as Unix's `brk`.  Above it,
`PAGES_ALLOC` gives a run of 256-byte pages from the top down (`.A` pages; `r0` the first's address), and
`PAGES_FREE` gives them back.  The two never overlap: the break can't rise over a page given out.  C's `malloc` and
HyForth's `allot` sit on these; fine-grained allocation is the language's.

## Banks

A task has 16 banks of 8K for each RAM module installed (the board's default is two or three modules: 256K or 384K a
task).  `BANKS` says how many and which modules are good (`r0`, a bit each; module m's banks are `$m0`-`$mF`).
`BANKS_ALLOC` takes a run from the lowest free (`.A` banks; `.A` the first), `BANKS_ALLOC_IN` from a range (`.X` the
lowest, `.Y` the highest: one module's), and `BANKS_FREE` gives them back.  The bitmap is only so libraries in one
program don't collide: a program selects a bank by writing `$00`, as on the X16, and reads and writes it at
`$8000`-`$9FFF`.

```
            lda         #4
            jsr         BANKS_ALLOC                         ; Four banks: .A = the first
            bcs         @nomem
            sta         $00                                 ; The first at $8000
            lda         #$5A
            sta         $8000
```

In C: `hy_banks_alloc (n)` and `hy_bank (b)`, the window at `HY_BANK_WINDOW`.

## Shared segments

A segment is a run of shared banks that several tasks attach to: `SEG_CREATE` makes one of `.A` banks (attached to
the caller; `.A` the segment), `SEG_ATTACH` and `SEG_DETACH` count a task in and out (the last out frees it, and a
task's end detaches it), and `SEG_MAP` says how to reach bank `.X` of it: `.A` the value for the U register (`$FFF1`)
and `.X` the value for `$00`.

```
            lda         seg
            ldx         #0                                  ; Its first bank
            jsr         SEG_MAP
            sta         $FFF1                               ; U: the shared macro-page
            stx         $00                                 ; Then the bank: at $8000
```

To find a segment by name rather than by passing its number, name it in `/dev/seg` (`#s`): write `name NAME N` to
`/dev/seg/ctl`, and any task reads `/dev/seg/NAME` for its number (`3 2`: segment 3, of 2 banks), then attaches.  The
name holds a reference of its own, so the segment stays while it's named; `free NAME` lets go.  `SEG_CREATE_IN` makes
one from a range of the shared bank IDs (`$80`-`$FF` are segments'); `SEGINFO` and `free` say what's used.

In C: `hy_seg_create`, `hy_seg_attach`, `hy_seg_detach`, and `hy_seg_map` (a bank of it at `HY_BANK_WINDOW`: U, then
`$00`).  The race, philo and prodcons samples share one between their tasks
([../../sdk/c/README.md](../../sdk/c/README.md#the-multitasking-demos)).

## Semaphores

For tasks that share something (a segment, a device, a file) or wait for each other, the kernel keeps 16 semaphores,
every task's by number:

| Call | What it does |
| :--- | :----------- |
| `SEM_NEW` | A semaphore: `.A` its count, `.X` 0 or `SEM_MUTEX` (a mutex: a count of 1, given back only by the task that took it); `.A` its number |
| `SEM_ACQUIRE` | Take one of its count, waiting (using no CPU) till there's one; a note ends the wait (`E_INTR`) |
| `SEM_TRY` | The same without the wait: `E_AGAIN` if there's none |
| `SEM_RELEASE` | Give one back; the tasks waiting wake, and the first to run takes it |
| `SEM_FREE` | Free it; its waiters wake with `E_INVAL` |

A task's end frees the semaphores it made and gives back the mutexes it holds.  A mutex's holder that asks again
gets `E_BUSY`, not a wait for ever.

```
            lda         #0
            ldx         #SEM_MUTEX
            jsr         SEM_NEW                             ; A mutex
            sta         lock
            ...
            lda         lock
            jsr         SEM_ACQUIRE                         ; Ours, till we give it back
            bcs         @interrupted
            ...                                             ; (The shared thing, alone)
            lda         lock
            jsr         SEM_RELEASE
```

In C: `hy_sem_new`, `hy_sem_acquire`, `hy_sem_try`, `hy_sem_release`, `hy_sem_free`.  The C SDK's multitasking demos
use them each way: a mutex around shared memory (race) and around the console (chorus), a semaphore for each task
passed round as a baton (chorus), forks as mutexes and a deadlock (philo), counting semaphores for a ring's free and
filled slots (prodcons), and barriers, each task giving "ready" one and taking one of "go" (all of them).

## Another task's memory

Only through `/proc`: `/proc/N/mem` is task N's 64K as it sees it, and `/proc/N/ram` its banks (bank b's byte o at
`b * $2000 + o`), read and written as files, any task's but the kernel task's and a driver's.  init alone may read
`#r` (`#r/task`: every task's RAM; `#r/shared`: the shared banks), the system's raw view.
