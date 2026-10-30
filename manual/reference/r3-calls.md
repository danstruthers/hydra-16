# Chapter 3. System Calls

A system call is a `jsr` to a fixed address in the jump table at $F800. Its arguments go in A, X, Y and the call
registers r0 to r15 (two bytes each, in the zero page from $02); its results come back in the same places, a
16-bit result in A (low byte) and X (high byte). On return, C = 0 means success. C = 1 means failure, and A
holds an error code (Appendix G). A call may change A, X, Y, the flags and r0 to r15, including registers it
doesn't use for a result; it never changes the program's own zero page, $22 to $7F.

A call that *can wait* may suspend the task until something happens elsewhere: a key, a disk, another task. A
note delivered to the task ends such a wait, and the call returns `E_INTR`.

Each call's name is defined in `hydra.inc` (`/lib/as`), with the constants and error codes used here. The
constants and the error codes are listed at the end of the chapter.

**How the entries are made.** The kernel is built from a specification that gives, for every call, its
registers in and out, the errors it can return and why, whether it can wait, and what it does. The entries of
this chapter are made from that same specification, so they describe the calls the kernel has, in the kernel's
own terms. Names in `THIS FORM` are constants or other calls. `OPEN` and `READ`, which most programs begin
with, are written out at more length.

**From the languages.** A call that a program can use, as opposed to one meant for drivers or the debugger,
can be made from each language:

* **hylang** has a function for it, `sys-` and the call's name in small letters. Each entry gives its
  arguments. A call that fails is an error carrying the system's message.
* **HyForth**, after `lib hydra`, has a word of the same name. It takes the call's inputs from the stack in the
  order the entry lists them, the first deepest, and leaves the results, with an *ior* on top if the call can
  fail. A path is passed as the address of a zero-terminated string, which `>z` makes (section 20.9).
* **HyBASIC** has `SYS "NAME" [, a [, x [, y]]]`, which sets A, X and Y. Arguments that go in the call
  registers must be put there with `POKE` first (r0 is at addresses 2 and 3, r1 at 4 and 5, and so on), and the
  results are read with `RREG` and `PEEK`.

## The calls by group

| Group | From | Calls |
|---|---|---|
| The system | $F800 | `SYSINFO` `ERRSTR` `KMESG` `REBOOT` `XCALL` |
| Tasks | $F830 | `EXITS` `WAIT` `SPAWN` `GETPID` `YIELD` `SLEEP` `SLEEP_UNTIL` `TICKS` `PREEMPT_OFF` `PREEMPT_ON` `TASKINFO` `NOTIFY` `NOTE` `MODINFO` `ENV_GET` `ENV_PUT` `ENV_DEL` `ENV_NAME` `TASKREAD` `GETPPID` `SEM_NEW` `SEM_ACQUIRE` `SEM_TRY` `SEM_RELEASE` `SEM_FREE` `EXEC` `SEM_WAIT` `RUN` `TASKFORE` |
| Memory | $F890 | `BREAK` `PAGES_ALLOC` `PAGES_FREE` `BANKS` `BANKS_ALLOC` `BANKS_FREE` `SEG_CREATE` `SEG_ATTACH` `SEG_DETACH` `SEG_MAP` `SEGINFO` `BANKS_ALLOC_IN` `SEG_CREATE_IN` `BANK_LOAD` `BANK_FLUSH` |
| Files | $F8C0 | `OPEN` `CREATE` `CLOSE` `READ` `WRITE` `SEEK` `STAT` `FSTAT` `WSTAT` `FWSTAT` `REMOVE` `DUP` `DUP2` `CHDIR` `GETCWD` `PIPE` `FD2PATH` |
| Namespaces | $F920 | `BIND` `MOUNT` `UNMOUNT` `NSINFO` |
| The console | $F950 | `PUTC` `PUTS` `GETC` `PUTHEX` |
| The clock | $F980 | `TIME` `TIME_SET` `RTC` |
| Drivers | $F9B0 | `IRQ_OWN` `IRQ_RELEASE` `WAKE` `PAUSE` `NOTE_POST` `SRV_REGISTER` `SRV_TAKE` `SRV_REPLY` `CLIENT_READ` `CLIENT_WRITE` `NOTE_QUEUE` `ROMREAD` `TASKMEM` `TASKSTOP` `TASKSTEP` `SRV_NAP` `INIT_RUN` |
| Debugging | $FA10 | `DBG_SCALL` `DBG_KCOPY` `DBG_PS` |

## BANKS

System call $F899, one of those for memory. It never waits.

#### Arguments

None.

#### Result

A
: The banks the task has (16 for each good RAM module)

r0
: The good modules (bit = module)

#### Description

The task's own RAM banks: module m's are $m0-$mF, selected by writing $00 (the X16's way). Modules POST found bad aren't counted.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-banks)`. It returns a list: A, r0.

HyForth: `sys-banks`. HyBASIC: `SYS "BANKS"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## BANKS_ALLOC

System call $F89C, one of those for memory. It never waits.

#### Arguments

A
: Banks.

#### Result

A
: The first (for $00)

#### Description

A run of the task's banks, from the lowest. Only bookkeeping, so that the libraries in one program don't use the same banks: a program may use its banks without it.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | 0 |
| $06 | `E_NOMEM` | out of memory |  |

#### From the languages

hylang: `(sys-banks-alloc A)`, where *A* is a number. It returns A.

HyForth: `sys-banks-alloc`. HyBASIC: `SYS "BANKS_ALLOC"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## BANKS_ALLOC_IN

System call $F8B1, one of those for memory. It never waits.

#### Arguments

A
: How many.

X
: The lowest bank it may take.

Y
: The highest (module m's are $m0-$mF)

#### Result

A
: The first.

#### Description

`BANKS_ALLOC`, from a range of the task's banks (the RAM disk's start FROM-TO: the modules its banks are on).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | 0 |
| $06 | `E_NOMEM` | out of memory | No run that long there |

#### From the languages

hylang: `(sys-banks-alloc-in A X Y)`, where *A* is a number; *X* is a number; *Y* is a number. It returns A.

HyForth: `sys-banks-alloc-in`. HyBASIC: `SYS "BANKS_ALLOC_IN"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## BANKS_FREE

System call $F89F, one of those for memory. It never waits.

#### Arguments

A
: The first bank.

X
: How many.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Give banks back.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not banks `BANKS_ALLOC` gave this task |

#### From the languages

hylang: `(sys-banks-free A X)`, where *A* is a number; *X* is a number. It returns `NIL`.

HyForth: `sys-banks-free`. HyBASIC: `SYS "BANKS_FREE"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## BANK_FLUSH

System call $F8BA, one of those for memory. It can wait.

#### Arguments

A
: The banks the code cache may have from now on (`RC_BANKS` to start with), or 0 (as it was)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Give up every program and child in this task's code cache (but a program running), and free the banks left empty.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-bank-flush A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-bank-flush`. HyBASIC: `SYS "BANK_FLUSH"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`.

## BANK_LOAD

System call $F8B7, one of those for memory. It can wait.

#### Arguments

r0
: A child's path (an o65 module, linked at $8000: sdk/c/child.cfg)

r1
: Its caller's exports (NAME.exp), or 0.

A
: Where its zero page goes.

X
: The bytes there it may have. None of them in the window.

#### Result

A
: Its bank.

X
: Its directory entry (`RB_DIR` + 16 x X in its bank: `RE_GEN` its load, `RE_TABLE` its table)

#### Description

A child (a bank program of a caller's, bankprog.h's: its functions called by the caller itself) in this task's code cache, as `RUN` keeps programs there (several to a bank, the one used least lately given up when there's no room), loaded if it isn't: its imports found by name in the caller's exports (its NAME.exp, the build's: "BX", 1, then each export its name's length, its name and its address, a 0 after the last), its zero page where the caller says. Its exports' table (`RE_TABLE`: each its name, a 0, its address; a 0 after the last) gives its entries. A call: its bank in the window ($00 alone), the entry's jsr, the bank put back; `RE_GEN`, as it was when it was found, says it's still there (it may have been given up for another meanwhile: then `BANK_LOAD` again), and the caller sets `RE_STAMP` from `TK_RCLOCK` (one more) as it calls. On a failure X says why (`BL_*`), and a missing import's name is in `TA_SCRATCH`.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found | Its file; or an import the exports haven't |
| $2D | `E_NOEXEC` | not a program | Not a child |
| $08 | `E_RANGE` | out of range | Its zero page |
| $2A | `E_IO` | i/o error | The exports |
| $06 | `E_NOMEM` | out of memory |  |
| $0B | `E_TOOBIG` | too big | No room |
| $09 | `E_FAULT` | bad address | A pointer in the window |

#### From the languages

hylang: `(sys-bank-load r0 r1 A X)`, where *r0* is a string; *r1* is a string, or `NIL`; *A* is a number; *X* is a number. It returns a list: A, X.

HyForth: `sys-bank-load`. HyBASIC: `SYS "BANK_LOAD"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_FLUSH`.

## BIND

System call $F920, one of those for namespaces. It can wait.

#### Arguments

r0
: New (a path)

r1
: Old (a path: absolute)

A
: `MREPL`, `MBEFORE` or `MAFTER`, and `MCREATE`.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Bind new at old: in old's place (`MREPL`), or a union with what's there, new first (`MBEFORE`) or last (`MAFTER`); with `MCREATE`, a `CREATE` in the union comes to new. New is what it is now: a mount point itself is all of its union's members, anything else the first of its candidates that's there. A union's first bind at a directory that's there (and isn't a mount point itself) keeps it as a member.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found |  |
| $02 | `E_INVAL` | invalid argument |  |
| $50 | `E_NSFULL` | namespace full |  |
| $2B | `E_NODEV` | no such device |  |
| $0A | `E_NAMETOOLONG` | name too long |  |

#### From the languages

hylang: `(sys-bind r0 r1 A)`, where *r0* is a string; *r1* is a string; *A* is a number, or left out. It returns `NIL`.

HyForth: `sys-bind`. HyBASIC: `SYS "BIND"`.

#### See also

The other calls for namespaces: `MOUNT`, `UNMOUNT`, `NSINFO`.

## BREAK

System call $F890, one of those for memory. It never waits.

#### Arguments

r0
: The new end of the program's data (the first byte past it), or 0 to ask.

#### Result

r0
: The end.

#### Description

Move the end of the program's data area: Unix's brk (BRK is the 65C02's instruction, so the call is `BREAK`). It starts at the top of the module's data and BSS. The top of a task's RAM is $7FFF ($7EFF in task F).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Below the program's own data |
| $06 | `E_NOMEM` | out of memory | Over pages `PAGES_ALLOC` gave, or past the top of RAM |

#### From the languages

hylang: `(sys-break r0)`, where *r0* is a number, or left out. It returns r0.

HyForth: `sys-break`. HyBASIC: `SYS "BREAK"`.

#### See also

The other calls for memory: `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## CHDIR

System call $F8E7, one of those for files. It can wait.

#### Arguments

r0
: A directory's path.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Change the current directory: the name a relative path starts from. It's kept whole and clean (`GETCWD`).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $22 | `E_NOTDIR` | not a directory |  |
| $20 | `E_NOENT` | not found |  |
| $2B | `E_NODEV` | no such device |  |
| $0A | `E_NAMETOOLONG` | name too long |  |

#### From the languages

hylang: `(sys-chdir r0)`, where *r0* is a string. It returns `NIL`.

HyForth: `sys-chdir`. HyBASIC: `SYS "CHDIR"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `GETCWD`, `PIPE`, `FD2PATH`.

## CLIENT_READ

System call $F9C8, one of those for drivers. It never waits.

#### Arguments

r0
: Here.

r1
: There, in the client's view.

r2
: A count.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Bytes from the client of the request being served (kcopy).

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## CLIENT_WRITE

System call $F9CB, one of those for drivers. It never waits.

#### Arguments

r0
: Here (the data)

r1
: There, in the client's view.

r2
: A count.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Bytes to the client of the request being served (kcopy).

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## CLOSE

System call $F8C6, one of those for files. It can wait.

#### Arguments

A
: An fd.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Close an fd. The last fd naming a channel (`DUP`, `SPAWN` share them) tells its server.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $25 | `E_BADF` | bad file descriptor | Not open |

#### From the languages

hylang: `(sys-close A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-close`. HyBASIC: `SYS "CLOSE"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## CREATE

System call $F8C3, one of those for files. It can wait.

#### Arguments

r0
: A path.

A
: The mode.

X
: The new file's mode, high byte (`DM_DIR`: a directory)

#### Result

A
: An fd.

#### Description

Make a file or a directory, and open it.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found |  |
| $2B | `E_NODEV` | no such device |  |
| $0A | `E_NAMETOOLONG` | name too long |  |
| $26 | `E_MFILE` | no free file descriptor |  |
| $27 | `E_NFILE` | too many open files |  |
| $21 | `E_EXIST` | already exists |  |

#### From the languages

hylang: `(sys-create r0 A X)`, where *r0* is a string; *A* is a number, or left out; *X* is a number, or left out. It returns A.

HyForth: `sys-create`. HyBASIC: `SYS "CREATE"`.

#### See also

The other calls for files: `OPEN`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## DBG_KCOPY

System call $FA13, one of those for debugging. It never waits.

#### Arguments

A
: Another task.

r0
: An address in this task.

r1
: An address in that task.

r2
: A byte count.

C
: 0 to copy from this task to that one, C = 1 the other way.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Copy bytes between this task's memory and another's (the kernel's kcopy).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $41 | `E_SRCH` | no such task | Not a task |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for debugging: `DBG_SCALL`, `DBG_PS`.

## DBG_PS

System call $FA16, one of those for debugging. It can wait.

#### Arguments

None.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Print a line for each task in use: its number, state, flags, parent, CPU time and name.

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for debugging: `DBG_SCALL`, `DBG_KCOPY`.

## DBG_SCALL

System call $FA10, one of those for debugging. It can wait.

#### Arguments

Y
: A driver task.

A, X
: What its serve entry gets.

#### Result

A, X and C from its serve entry.

#### Description

Call a driver's serve entry directly, in the driver's task (the file layer will make these calls in phase 2). A driver serves one call at a time: a second caller waits.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $41 | `E_SRCH` | no such task | Not a task |
| $2B | `E_NODEV` | no such device | It's free or ended, or has no serve entry: a program |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for debugging: `DBG_KCOPY`, `DBG_PS`.

## DUP

System call $F8E1, one of those for files. It never waits.

#### Arguments

A
: An fd.

#### Result

A
: A new fd (the lowest free), on the same channel (the same offset)

#### Description

Another fd for an open file.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $25 | `E_BADF` | bad file descriptor |  |
| $26 | `E_MFILE` | no free file descriptor |  |

#### From the languages

hylang: `(sys-dup A)`, where *A* is a number. It returns A.

HyForth: `sys-dup`. HyBASIC: `SYS "DUP"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## DUP2

System call $F8E4, one of those for files. It can wait.

#### Arguments

A
: An fd.

X
: Another (0-15)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Make fd X name the same channel as fd A, closing what it named first.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $25 | `E_BADF` | bad file descriptor |  |

#### From the languages

hylang: `(sys-dup2 A X)`, where *A* is a number; *X* is a number. It returns `NIL`.

HyForth: `sys-dup2`. HyBASIC: `SYS "DUP2"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## ENV_DEL

System call $F860, one of those for tasks. It never waits.

#### Arguments

A
: A task ($FF: this one)

r0
: A variable's name.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Remove a variable.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found |  |
| $41 | `E_SRCH` | no such task |  |
| $02 | `E_INVAL` | invalid argument |  |
| $0A | `E_NAMETOOLONG` | name too long |  |

#### From the languages

hylang: `(sys-env-del A r0)`, where *A* is a number; *r0* is a string. It returns `NIL`.

HyForth: `sys-env-del`. HyBASIC: `SYS "ENV_DEL"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## ENV_GET

System call $F85A, one of those for tasks. It never waits.

#### Arguments

A
: A task ($FF: this one)

r0
: A variable's name.

r1
: A buffer.

r2
: Its size.

r3
: An offset in the value.

#### Result

A/X
: The value's length from the offset (0 at its end or past it)

As much of it as fits, in the buffer.

#### Description

A variable of a task's environment (each task has its own: `ENV_MAX` bytes; `SPAWN` copies its parent's). A value is any bytes (rc's lists: each word and a 0). `#e` serves the caller's as files, `/env/NAME.`.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found | No such variable |
| $41 | `E_SRCH` | no such task | No such task |
| $02 | `E_INVAL` | invalid argument | An empty name, or one with a / |
| $0A | `E_NAMETOOLONG` | name too long |  |

#### From the languages

hylang: `(sys-env-get A r0 size r3)`, where *A* is a number; *r0* is a string; *size* is the most to take: r2; *r3* is a number, or left out. It returns a list: A/X, the first A/X bytes of the buffer, as a string. (Here r1 is a buffer the function makes itself.)

HyForth: `sys-env-get`. HyBASIC: `SYS "ENV_GET"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## ENV_NAME

System call $F863, one of those for tasks. It never waits.

#### Arguments

A
: A task ($FF: this one)

X
: Which variable (0 on)

r0
: A buffer (`ENV_NAME_MAX` + 1 bytes)

#### Result

A/X
: Its value's length.

Its name in the buffer, zero-terminated.

#### Description

The variables of an environment, one at a time, in the order they were made.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found | Past the last |
| $41 | `E_SRCH` | no such task |  |

#### From the languages

hylang: `(sys-env-name A X)`, where *A* is a number; *X* is a number. It returns a list: the buffer's text, A/X. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-env-name`. HyBASIC: `SYS "ENV_NAME"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## ENV_PUT

System call $F85D, one of those for tasks. It never waits.

#### Arguments

A
: A task ($FF: this one)

r0
: A variable's name.

r1
: Bytes.

r2
: Their count.

r3
: Where in its value they go (the value cut there first: 0 for a new value; its length at most)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Set a variable, or make it (offset 0). An offset is for writing a long value in parts.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $06 | `E_NOMEM` | out of memory | No room in the environment: `ENV_MAX` bytes; or no RAM module for one |
| $02 | `E_INVAL` | invalid argument | Past the value's end; a bad name |
| $41 | `E_SRCH` | no such task |  |
| $20 | `E_NOENT` | not found | An offset given, and no such variable |
| $0A | `E_NAMETOOLONG` | name too long |  |

#### From the languages

hylang: `(sys-env-put A r0 r1 r3)`, where *A* is a number; *r0* is a string; *r1* is a string or a list of bytes (its length goes in r2); *r3* is a number, or left out. It returns `NIL`.

HyForth: `sys-env-put`. HyBASIC: `SYS "ENV_PUT"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## ERRSTR

System call $F803, one of those for the system. It never waits.

#### Arguments

A
: An error code.

r0
: A buffer (32 bytes)

#### Result

The error's text in the buffer, zero-terminated ("error $xx" for an unknown code)

#### Description

An error code as text, for messages.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-errstr A)`, where *A* is a number. It returns the buffer's text. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-errstr`. HyBASIC: `SYS "ERRSTR"`.

#### See also

The other calls for the system: `SYSINFO`, `KMESG`, `REBOOT`, `XCALL`.

## EXEC

System call $F87B, one of those for tasks. It can wait.

#### Arguments

r0
: The program's path (as `SPAWN`'s)

r1
: Its arguments (as `SPAWN`'s: 176 bytes at most), or 0.

#### Result

(doesn't return, once the program is found: it runs in this task's place)

#### Description

Run a program in this task, in place of the one that calls it (Plan 9's exec): the program is found as `SPAWN` finds it (a module of the paged ROM runs in place; any other is read into this task's RAM at its load address), and starts at its entry point with r0 pointing to its arguments, its name the header's. The task keeps what isn't the old program's: its number, its parent (whose `WAIT` gets the new program's exit status), its note group, every fd (a RAM program's file takes fd 15 as it loads), its current directory, its namespace and its environment, and its CPU time. What was the old program's goes as at its end: its IRQ lines, shared segments, semaphores (and the mutexes it holds), and the device letters it served; its note handler; its pages and banks. A note that comes meanwhile waits for the new program. If the program can't be found or isn't one, `EXEC` returns the error and the caller goes on; if it's found but can't be read (a RAM program's file cut short), the task ends with the error as its code. rc runs the last command of rc -c this way, and its exec built-in.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found | No such file |
| $2D | `E_NOEXEC` | not a program | Not a program this kernel runs |
| $0B | `E_TOOBIG` | too big | The arguments |
| $01 | `E_PERM` | not allowed | A driver |
| $05 | `E_INTR` | interrupted |  |

Also what else `OPEN` and `READ` give.

#### From the languages

hylang: `(sys-exec r0 r1)`, where *r0* is a string; *r1* is a list of strings, or `NIL`. It returns `NIL`.

HyForth: `sys-exec`. HyBASIC: `SYS "EXEC"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## EXITS

System call $F830, one of those for tasks. It never waits.

#### Arguments

A
: The exit code (0: success)

r0
: A message (zero-terminated, 31 characters at most), or 0 for none.

#### Result

(doesn't return)

#### Description

End the calling task, with an exit status (Plan 9's exits). Returning from a program's entry point is `EXITS` with code 0 and no message. Everything the task had is released; its parent can `WAIT` for the status.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-exits A r0)`, where *A* is a number, or left out; *r0* is a string, or `NIL`. It returns `NIL`.

HyForth: `sys-exits`. HyBASIC: `SYS "EXITS"`.

#### See also

The other calls for tasks: `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## FD2PATH

System call $F8F0, one of those for files. It never waits.

#### Arguments

A
: An fd.

r0
: A buffer (`PATH_MAX` + 1 bytes)

#### Result

The name in the buffer, zero-terminated.

#### Description

The name an fd's file was opened by (Plan 9's fd2path), whole and clean, as `OPEN` or `CREATE` made it (an fd `SPAWN` or `DUP` gave shares its channel, and its name): the namespace's, or a # name; a pipe's `#|/pipe`.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $25 | `E_BADF` | bad file descriptor | Not an open fd |

#### From the languages

hylang: `(sys-fd2path A)`, where *A* is a number. It returns the buffer's text. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-fd2path`. HyBASIC: `SYS "FD2PATH"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`.

## FSTAT

System call $F8D5, one of those for files. It can wait.

#### Arguments

A
: An fd.

r0
: A buffer (`SR_SIZE` bytes)

#### Result

The stat record in the buffer.

#### Description

An open file's stat record.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $25 | `E_BADF` | bad file descriptor |  |

#### From the languages

hylang: `(sys-fstat A)`, where *A* is a number. It returns the stat record, as a hash. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-fstat`. HyBASIC: `SYS "FSTAT"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## FWSTAT

System call $F8DB, one of those for files. It can wait.

#### Arguments

A
: An fd.

r0
: A stat record.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Change an open file from a stat record.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $25 | `E_BADF` | bad file descriptor |  |
| $03 | `E_NOSYS` | no such call |  |

#### From the languages

hylang: `(sys-fwstat A r0)`, where *A* is a number; *r0* is a hash, as `stat` gives. It returns `NIL`.

HyForth: `sys-fwstat`. HyBASIC: `SYS "FWSTAT"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## GETC

System call $F956, one of those for the console. It can wait.

#### Arguments

None.

#### Result

A
: A byte.

#### Description

Read a byte from stdin, waiting for one (the X16's CHRIN, unechoed).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $2E | `E_EOF` | end of file | The end of stdin |
| $05 | `E_INTR` | interrupted | A note came: taken on the way out |

#### From the languages

hylang: `(sys-getc)`. It returns A.

HyForth: `sys-getc`. HyBASIC: `SYS "GETC"`.

#### See also

The other calls for the console: `PUTC`, `PUTS`, `PUTHEX`.

## GETCWD

System call $F8EA, one of those for files. It never waits.

#### Arguments

r0
: A buffer (`PATH_MAX` + 1 bytes)

#### Result

A
: The current directory's length.

The directory in the buffer, zero-terminated.

#### Description

The current directory.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-getcwd)`. It returns the buffer's text. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-getcwd`. HyBASIC: `SYS "GETCWD"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `PIPE`, `FD2PATH`.

## GETPID

System call $F839, one of those for tasks. It never waits.

#### Arguments

None.

#### Result

A
: This task's number.

#### Description

The calling task's number (0-15).

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-getpid)`. It returns A.

HyForth: `sys-getpid`. HyBASIC: `SYS "GETPID"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## GETPPID

System call $F869, one of those for tasks. It never waits.

#### Arguments

None.

#### Result

A
: This task's parent ($FF: none)

#### Description

The task that started this one (Plan 9's getppid): init once that one has ended; none for init, the drivers and the kernel task.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-getppid)`. It returns A.

HyForth: `sys-getppid`. HyBASIC: `SYS "GETPPID"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## INIT_RUN

System call $F9E0, one of those for drivers. It never waits.

#### Arguments

A
: A number, 0-15.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Start init (the module the directory names) again, in a task of its own, with the number in decimal as its argument: as the boot starts it (no parent, no fds, an empty namespace and environment, a note group of its own), but the boot's has no argument. The kernel task starts it as it next runs; with no task free it isn't started (but 0's, which waits for one). The kernel itself asks for 0's when the task init was last started in for 0 (or by the boot) has EXECed and then ends: so an init that EXECs the login shell is started again when that shell ends (an init that ends itself is left ended). HydraOS's console asks for n's when the user makes window n. From a task or an irq entry.

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`.

## IRQ_OWN

System call $F9B0, one of those for drivers. It never waits.

#### Arguments

A
: An IRQ line (0-14), or `LINE_VIA_T2` (VIA timer 2), or `LINE_VIA_CA1` (VIA CA1)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Own an IRQ line: its interrupts run the calling task's irq entry, in the calling task. The entry gets the line in A, and returns A = 0, or `IRQ_RESCHED` to ask for a task switch (it woke a task). It runs with interrupts off and must not make calls that wait, and with the dispatch's own cycles it must stay under the 200-cycle budget: about 70 cycles of its own (`TASK_EVENT`, not `WAKE`, to wake clients; `NOTE_QUEUE` for a group). A task's lines are released when it ends. `LINE_VIA_T2` is VIA timer 2: owning it sets the timer one-shot with its interrupt on; the owner starts it (T2CL, then T2CH), and its entry clears the interrupt (reading T2CL, or starting it again). `LINE_VIA_CA1` is CA1: owning it turns CA1's interrupt on (off as it's given back), and the entry clears CA1's flag (IFR).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $07 | `E_BUSY` | busy | Another task owns it |
| $08 | `E_RANGE` | out of range | Not a line |
| $02 | `E_INVAL` | invalid argument | The task has no IRQ entry |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## IRQ_RELEASE

System call $F9B3, one of those for drivers. It never waits.

#### Arguments

A
: An IRQ line.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Give an IRQ line back.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $01 | `E_PERM` | not allowed | Not the caller's |
| $08 | `E_RANGE` | out of range |  |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## KMESG

System call $F806, one of those for the system. It never waits.

#### Arguments

r0
: A buffer.

r1
: Its size, the most it copies.

r2
: An offset in the messages held, 0 the oldest.

#### Result

A/X
: The bytes held, `KMESG_SIZE` at most.

The buffer has those from r2, r1 at most.

#### Description

The kernel's messages: what it printed on the bring-up console (the banner, POST, the boot's tasks, a driver's own lines), its last `KMESG_SIZE` bytes. `/dev/kmesg` (kdev's `#n/kmesg`) is them as a file.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-kmesg count r2)`, where *count* is the most to take: r1; *r2* is a number, or left out. It returns a list: the buffer, as a string, A/X. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-kmesg`. HyBASIC: `SYS "KMESG"`.

#### See also

The other calls for the system: `SYSINFO`, `ERRSTR`, `REBOOT`, `XCALL`.

## MODINFO

System call $F857, one of those for tasks. It never waits.

#### Arguments

A
: An entry of the module directory (0 on)

r0
: A buffer (`ME_SIZE` bytes)

#### Result

The buffer: `ME_BANK`, `ME_BANKS`, `ME_TYPE` (`HT_*`), `ME_FLAGS` (`HF_*`), `ME_NAME` (11, zero-padded), `ME_PAGE`.

#### Description

A module in the paged ROM (`SPAWN`'s `#m/NAME`), as the module directory has it. A module of a bank may share it with others: its header is at $A000 + 256 * `ME_PAGE` there (0 for a module of banks).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found | Past the last |

#### From the languages

hylang: `(sys-modinfo A)`, where *A* is a number. It returns the buffer, as a string. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-modinfo`. HyBASIC: `SYS "MODINFO"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## MOUNT

System call $F923, one of those for namespaces. It can wait.

#### Arguments

X
: A device letter.

r0
: Its spec (a string, 8 characters at most), or 0.

r1
: Old.

A
: `BIND`'s flags.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Mount a device's server, with a spec (which of its file trees: '#f' with x is the ROM disk), at old, as `BIND` does.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $2B | `E_NODEV` | no such device |  |
| $20 | `E_NOENT` | not found |  |
| $02 | `E_INVAL` | invalid argument |  |
| $50 | `E_NSFULL` | namespace full |  |
| $0A | `E_NAMETOOLONG` | name too long |  |

#### From the languages

hylang: `(sys-mount X r0 r1 A)`, where *X* is a number; *r0* is a string, or `NIL`; *r1* is a string; *A* is a number, or left out. It returns `NIL`.

HyForth: `sys-mount`. HyBASIC: `SYS "MOUNT"`.

#### See also

The other calls for namespaces: `BIND`, `UNMOUNT`, `NSINFO`.

## NOTE

System call $F854, one of those for tasks. It never waits.

#### Arguments

A
: A task, or `NOTE_GROUP` | a note group.

X
: The note (1-31: `NOTE_*`)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Send a note (Plan 9's postnote). It wakes the task if it waits: `WAIT`, `PAUSE`, `SLEEP` and `GETC` end with `E_INTR`. With no handler, and for `NOTE_KILL` always, the task ends with 128 + the note's Unix number (130 for `NOTE_INTERRUPT`, 137 for `NOTE_KILL`) and its name as the message.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | No such note |
| $41 | `E_SRCH` | no such task | No task, or nobody in the group |
| $01 | `E_PERM` | not allowed | The kernel task, a driver |

#### From the languages

hylang: `(sys-note A X)`, where *A* is a number; *X* is a number. It returns `NIL`.

HyForth: `sys-note`. HyBASIC: `SYS "NOTE"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## NOTE_POST

System call $F9BC, one of those for drivers. It never waits.

#### Arguments

A
: A task, or `NOTE_GROUP` | a note group.

X
: The note.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

`NOTE` without taking the caller's own notes: the note posted, the tasks woken. From a task, or an irq entry for a task (a group is too many tasks to look at with interrupts off: `NOTE_QUEUE`).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument |  |
| $41 | `E_SRCH` | no such task |  |
| $01 | `E_PERM` | not allowed |  |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## NOTE_QUEUE

System call $F9CE, one of those for drivers. It never waits.

#### Arguments

X
: A note group (0-15)

A
: The note's bit: 1 << (note - 1), for a note 1-8.

IRQs off.

#### Result

X kept.

#### Description

A note to a note group, from an irq entry (a console's Ctrl-C): queued, and posted by the kernel task as soon as it runs. Short enough for an irq entry: nothing is checked.

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## NOTIFY

System call $F851, one of those for tasks. It never waits.

#### Arguments

r0
: The note handler, or 0 for none.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Set the task's note handler (Plan 9's notify). A note runs it in the task, in its own code (never in the middle of a system call), with A = the note: it returns C = 0 to go on where the task was, or C = 1 for the default; it may call `EXITS`. A note that comes meanwhile waits for it to return.

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## NSINFO

System call $F929, one of those for namespaces. It never waits.

#### Arguments

A
: A task ($FF: this one)

X
: Which entry of its namespace (0 on)

r0
: A buffer (`NI_SIZE` bytes)

#### Result

The entry in the buffer (`NI_*`): its mount point, its device letter and spec, the path in that device, its flags (`MCREATE`) and its place among the union's members.

#### Description

A namespace's mount entries, one at a time, in the order of the kernel's table: each a member of the union at its mount point, the members in `NI_SEQ`'s order (lowest first). `/proc`'s ns, and ns.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $08 | `E_RANGE` | out of range | Past the last |
| $41 | `E_SRCH` | no such task |  |

#### From the languages

hylang: `(sys-nsinfo A X)`, where *A* is a number; *X* is a number. It returns the buffer, as a string. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-nsinfo`. HyBASIC: `SYS "NSINFO"`.

#### See also

The other calls for namespaces: `BIND`, `MOUNT`, `UNMOUNT`.

## OPEN

System call $F8C0: opens a file that exists, and returns a file descriptor for it. It can wait.

#### Syntax

```text
            LDR r0, path        ; r0 = the address of the path
            lda #mode
            jsr OPEN
            bcs failed          ; A = the error code
            sta fd              ; A = the file descriptor
```

#### Arguments

r0
: The address of the path: its bytes, then a zero byte.

A
: The mode: one of `O_READ` (0), `O_WRITE` (1) or `O_RDWR` (2), to which can be added `O_TRUNC` ($10), to empty
  the file as it is opened, and `O_NONBLOCK` ($40), to make reads and writes of this descriptor return `E_AGAIN`
  where they would otherwise wait.

#### Result

C = 0, and A = the file descriptor: a number from 0 to 15, the lowest one the task has free.

#### Description

1. The path is made absolute and clean. One that doesn't start with `/` (or with `#`, a device's own name) has
   the task's current directory put before it; then `.` elements and doubled slashes are removed, and each `..`
   removes the element before it. The result may be at most 63 bytes (`PATH_MAX`).
2. The path is looked up in the task's namespace (section 9.5). Where a directory is a union of several (section 9.7), each
   member is tried in turn, and the first that has the name is the one opened. A member whose device has gone is
   passed over as if it didn't have the name.
3. The device's driver is asked to open the file in the mode given. If it refuses, its error is the call's.
4. The descriptor is set to the file, at offset 0.

A descriptor has an offset of its own, which `READ`, `WRITE` and `SEEK` move. Descriptors made from it by `DUP`,
and those a child task is given by `SPAWN`, share that offset; a second `OPEN` of the same file does not.

Opening a directory with `O_READ` succeeds: it reads as a sequence of 64-byte stat records, one for each file in
it. When the directory is a union, the records of all its members follow one another.

`OPEN` never makes a file. `CREATE` does, and opens it.

#### Errors

| Code | Name | Cause |
|---|---|---|
| $20 | `E_NOENT` | No such file, in any member of a union |
| $2B | `E_NODEV` | The path names a device that isn't there |
| $0A | `E_NAMETOOLONG` | The path, made absolute, is longer than 63 bytes |
| $26 | `E_MFILE` | The task has all 16 of its descriptors in use |
| $27 | `E_NFILE` | All 48 of the system's channels are in use (every open file in every task takes one; `DUP`s share) |
| $05 | `E_INTR` | A note arrived while the call was waiting for the driver |

A driver adds its own: `E_ROFS` ($29) for a write mode on a read-only disk, and `E_ISDIR` ($23) for a write mode on
a directory.

#### Notes

Descriptors 0, 1 and 2 are standard input, output and error, already open when a program starts. A task that
closes one of them will get that number from its next `OPEN`.

A descriptor that isn't closed is closed when the task ends.

#### From the languages

| Language | Form |
|---|---|
| HyForth (`lib hydra`) | `sys-open` ( *z-addr mode* -- *fd ior* ), the path zero-terminated (`>z` makes one) |
| hylang | `(sys-open path [mode])`: the descriptor. A failure is an error, its text the path and the system's message |
| HyBASIC | `SYS "OPEN"`; or, for most purposes, the `OPEN` statement (Chapter 1) |

#### See also

`CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `DUP`, `FSTAT`.

#### Example

From hylang. Descriptors 0 to 2 are in use, so the first file opened is 3:

```session
hylang> (sys-open "/dev/time" 0)
=> 3
hylang> (sys-open "/dev/nothing" 0)
=> Error: /dev/nothing: not found
hylang> (sys-open "/rom/bin/ls" 1)
=> Error: /rom/bin/ls: read-only
hylang> (sys-open "/ram" 1)
=> Error: /ram: is a directory
hylang> (sys-open "#q/x" 0)
=> Error: #q/x: no such device
```

## PAGES_ALLOC

System call $F893, one of those for memory. It never waits.

#### Arguments

A
: Pages (256 bytes each)

#### Result

r0
: The first one's address.

A
: Its page.

#### Description

A run of pages of the task's RAM, above the break, taken from the top down: for a language runtime's heap, or a buffer.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | 0 |
| $06 | `E_NOMEM` | out of memory |  |

#### From the languages

hylang: `(sys-pages-alloc A)`, where *A* is a number. It returns a list: r0, A.

HyForth: `sys-pages-alloc`. HyBASIC: `SYS "PAGES_ALLOC"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## PAGES_FREE

System call $F896, one of those for memory. It never waits.

#### Arguments

A
: Pages.

r0
: The first one's address.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Give pages back.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not pages `PAGES_ALLOC` gave this task |

#### From the languages

hylang: `(sys-pages-free A r0)`, where *A* is a number; *r0* is a number. It returns `NIL`.

HyForth: `sys-pages-free`. HyBASIC: `SYS "PAGES_FREE"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## PAUSE

System call $F9B9, one of those for drivers. It can wait.

#### Arguments

None.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Wait until another task or an irq entry WAKEs this one, or a note comes (taken on the way out). (Wake-ups can come early: look again after.)

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## PIPE

System call $F8ED, one of those for files. It can wait.

#### Arguments

None.

#### Result

A
: An fd to read from.

X
: An fd to write to.

#### Description

A pipe (`#|`): what's written to one end is read from the other, 512 bytes held between them. A read waits for something to read, and is the end (0) once no writer is left; a write waits for room, and is `E_PIPE` once no reader is left.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $2B | `E_NODEV` | no such device | No pipe server |
| $26 | `E_MFILE` | no free file descriptor |  |
| $27 | `E_NFILE` | too many open files |  |
| $06 | `E_NOMEM` | out of memory | No pipe free |

#### From the languages

hylang: `(sys-pipe)`. It returns a list: A, X.

HyForth: `sys-pipe`. HyBASIC: `SYS "PIPE"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `FD2PATH`.

## PREEMPT_OFF

System call $F848, one of those for tasks. It never waits.

#### Arguments

None.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Hold the CPU: no task switch until the matching `PREEMPT_ON` (they nest). Interrupts keep running.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-preempt-off)`. It returns `NIL`.

HyForth: `sys-preempt-off`. HyBASIC: `SYS "PREEMPT_OFF"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## PREEMPT_ON

System call $F84B, one of those for tasks. It never waits.

#### Arguments

None.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Undo `PREEMPT_OFF`; a task switch that came due meanwhile happens now.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-preempt-on)`. It returns `NIL`.

HyForth: `sys-preempt-on`. HyBASIC: `SYS "PREEMPT_ON"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## PUTC

System call $F950, one of those for the console. It can wait.

#### Arguments

A
: A byte.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Write a byte to stdout (the X16's CHROUT).

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-putc A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-putc`. HyBASIC: `SYS "PUTC"`.

#### See also

The other calls for the console: `PUTS`, `GETC`, `PUTHEX`.

## PUTHEX

System call $F959, one of those for the console. It can wait.

#### Arguments

A
: A byte.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Write a byte to stdout as two hex digits (for debugging).

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-puthex A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-puthex`. HyBASIC: `SYS "PUTHEX"`.

#### See also

The other calls for the console: `PUTC`, `PUTS`, `GETC`.

## PUTS

System call $F953, one of those for the console. It can wait.

#### Arguments

r0
: A string, zero-terminated.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Write a string to stdout.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $05 | `E_INTR` | interrupted | A note came while it waited: taken on the way out |

#### From the languages

hylang: `(sys-puts r0)`, where *r0* is a string. It returns `NIL`.

HyForth: `sys-puts`. HyBASIC: `SYS "PUTS"`.

#### See also

The other calls for the console: `PUTC`, `GETC`, `PUTHEX`.

## READ

System call $F8C9: reads bytes from an open file into memory. It can wait.

#### Syntax

```text
            LDR r0, buf         ; r0 = where the bytes go
            LDR r1, count       ; r1 = how many, at most
            lda fd
            jsr READ
            bcs failed          ; A = the error code
                                ; A/X = how many were read; 0 is the end of the file
```

#### Arguments

A
: A file descriptor, open with `O_READ` or `O_RDWR`.

r0
: The address of a buffer in the task's memory.

r1
: The most bytes to read: 0 to 65,535. The buffer must have room for them.

#### Result

C = 0, and A/X = the number of bytes read, which were put in the buffer from its start. The descriptor's offset
has moved on by that many.

#### Description

The kernel asks the file's driver for the bytes at the descriptor's offset, at most 512 at a time (`IO_UNIT`),
and repeats until one of three things happens:

* **The count is reached.** The result equals r1.
* **The file ends.** The result is what there was before the end; a read that starts at the end returns 0. A
  result of 0 is how the end of a file is recognised, and it is not an error.
* **The driver returns fewer bytes than it was asked for.** The kernel stops there and returns them. This is how a
  device says "that is all for now": the console returns one line, however large the buffer.

A result smaller than r1 therefore does not by itself mean the end of the file. A program that needs exactly *n*
bytes must call `READ` again for the remainder until it has them or gets 0.

When the driver has nothing yet (no key typed, an empty pipe), the task waits. With `O_NONBLOCK` set on the
descriptor, the call returns `E_AGAIN` instead.

A directory reads as 64-byte stat records (`SR_SIZE`), one for each file in it: ask for a multiple of 64.

#### Errors

| Code | Name | Cause |
|---|---|---|
| $25 | `E_BADF` | A isn't an open descriptor, or it was opened with `O_WRITE` |
| $04 | `E_AGAIN` | Nothing to read yet, and the descriptor is non-blocking |
| $05 | `E_INTR` | A note arrived while the call was waiting |

A driver adds its own: a disk that fails answers `E_IO` ($2A).

#### From the languages

| Language | Form |
|---|---|
| HyForth (`lib hydra`) | `sys-read` ( *fd c-addr u* -- *u2 ior* ): *u2* bytes were read |
| hylang | `(sys-read fd count)`: the bytes read, as a string |
| HyBASIC | `SYS "READ"`; or `INPUT #`, `LINE INPUT #`, `INPUT$` and `GET` |

#### See also

`OPEN`, `WRITE`, `SEEK`, `CLOSE`, `PIPE`.

#### Example

The offset moving on, a count cut short by the end of the file, and 0 at the end:

<!-- session
echo hello world >/ram/t
hylang
-->

```session
hylang> (set {fd} (sys-open "/ram/t" 0))
=> NIL
hylang> (sys-read fd 5)
=> "hello"
hylang> (sys-read fd 100)
=> " world\n"
hylang> (sys-read fd 100)
=> ""
hylang> (sys-close fd)
=> NIL
```

## REBOOT

System call $F809, one of those for the system. It never waits.

#### Arguments

A
: 0, or `REBOOT_HWTEST`.

#### Result

(doesn't return)

#### Description

Start the system again, as the reset button does (the VIA's and ACIA's interrupts quieted first: only the button resets the chips); with `REBOOT_HWTEST`, into the hardware test, as a T typed during POST does. Every task ends where it is: files written are on their disks already (the disks write through).

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-reboot A)`, where *A* is a number, or left out. It returns `NIL`.

HyForth: `sys-reboot`. HyBASIC: `SYS "REBOOT"`.

#### See also

The other calls for the system: `SYSINFO`, `ERRSTR`, `KMESG`, `XCALL`.

## REMOVE

System call $F8DE, one of those for files. It can wait.

#### Arguments

r0
: A path.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Remove a file, or an empty directory.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found |  |
| $2B | `E_NODEV` | no such device |  |
| $0A | `E_NAMETOOLONG` | name too long |  |
| $24 | `E_NOTEMPTY` | directory not empty |  |
| $03 | `E_NOSYS` | no such call |  |

#### From the languages

hylang: `(sys-remove r0)`, where *r0* is a string. It returns `NIL`.

HyForth: `sys-remove`. HyBASIC: `SYS "REMOVE"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## ROMREAD

System call $F9D1, one of those for drivers. It never waits.

#### Arguments

A
: A paged ROM bank.

r0
: Where in it ($A000-$DFFF)

r1
: A buffer (not at $A000-$DFFF)

r2
: A count.

#### Result

The bytes in the buffer.

#### Description

Copy from any bank of the paged ROM, for a module run in place (the storage driver's ROM disk): the bank is the task's own meanwhile, and its module's is put back.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $01 | `E_PERM` | not allowed | The task owns an IRQ line: its irq entry would find the wrong bank at $A000 |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## RTC

System call $F986, one of those for the clock. It never waits.

#### Arguments

A
: 0, read, or 1, write.

r0
: 8 bytes: the DS1747's registers (RTC_REGS, hw.inc), BCD.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

The DS1747 in U7, its registers a byte at a time (task F's $7FF8-$7FFF): read with its updates halted (R) for the copy; written with W set, the control (its century) last with W clear, which starts it from them. No chip: the RAM there, as it is. A read must check the fields hold a date and time.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | A neither |

#### From the languages

hylang: `(sys-rtc A r0)`, where *A* is a number; *r0* is a string of 8 bytes, or `NIL` for zeros. It returns the buffer, as a string.

HyForth: `sys-rtc`. HyBASIC: `SYS "RTC"`.

#### See also

The other calls for the clock: `TIME`, `TIME_SET`.

## RUN

System call $F881, one of those for tasks. It can wait.

#### Arguments

r0
: A bank program's path (as `SPAWN`'s)

r1
: Its arguments (as `SPAWN`'s), or 0.

A
: Flags (`SPAWN_FDMAP`, `RUN_LEND`)

r2
: An fd map (with `SPAWN_FDMAP`, as `SPAWN`'s). None of them in the window ($8000-$9FFF)

#### Result

A
: Its exit code.

r0
: Its exit message (31 characters at most: `TA_SCRATCH`, "" for none)

#### Description

Run a bank program in this task, as a subroutine, and wait for it. A bank program (`HF_BANK`, not `HF_TASK`) is kept in this task's code cache: RAM banks of its own (eight at most, `BANKS_ALLOC`'s), several programs to a bank, one above the other, each loaded the first time it's run (its file's o65 module, relocated where it's put) and kept for the next; when there's no room the one run least lately is given up. The program is known by its file (`STAT`'s), so a changed file is loaded again. It runs with its bank at $8000 and its zero page in the program's ($22 up), on this task's stack: its arguments at r0, its fds this task's 0, 1 and 2 (`SPAWN_FDMAP`: the map's), no note handler (the task keeps its name). It ends as a program does (returning from main, `EXITS`, a note's default: Ctrl-C), and then the task is the caller's again: its fds (the program's closed), note handler, zero page, banks at $8000 and $A000 and break; a note that ended the program (but a kill) is posted again, for the caller's own handler. `RUN_LEND` lets the program move the break past the caller's (`BREAK`: memory the caller doesn't use; it's put back after); without it, `BREAK` fails. The banks and pages the program takes (and not gives back) are the task's till it ends. A program running in place can `RUN` another.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $2D | `E_NOEXEC` | not a program | Not a bank program that runs in place: `SPAWN` it |
| $20 | `E_NOENT` | not found |  |
| $07 | `E_BUSY` | busy | Running already |
| $06 | `E_NOMEM` | out of memory | No bank for it |
| $0B | `E_TOOBIG` | too big |  |
| $09 | `E_FAULT` | bad address | A pointer in the window |
| $05 | `E_INTR` | interrupted |  |

Also what `STAT` and `READ` give.

#### From the languages

hylang: `(sys-run r0 r1 A r2)`, where *r0* is a string; *r1* is a list of strings, or `NIL`; *A* is a number, or left out; *r2* is a list of bytes, or `NIL`. It returns A.

HyForth: `sys-run`. HyBASIC: `SYS "RUN"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `TASKFORE`.

## SEEK

System call $F8CF, one of those for files. It can wait.

#### Arguments

A
: An fd.

r0, r1
: An offset (low word, high word: signed)

X
: From: 0 the start, 1 the offset, 2 the end.

#### Result

r0, r1
: The new offset.

#### Description

Move an fd's offset (shared by the fds that `DUP` or `SPAWN` made of it).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $25 | `E_BADF` | bad file descriptor |  |
| $02 | `E_INVAL` | invalid argument | Before the start, or no such from |

#### From the languages

hylang: `(sys-seek A r0/r1 X)`, where *A* is a number; *r0/r1* is a number of 32 bits; *X* is a number, or left out. It returns r0/r1.

HyForth: `sys-seek`. HyBASIC: `SYS "SEEK"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## SEGINFO

System call $F8AE, one of those for memory. It never waits.

#### Arguments

None.

#### Result

A
: The shared banks segments can have (8K each: the good chips')

X
: Those segments have now.

r0
: The segments in use (bit = segment)

#### Description

The shared RAM, as free shows it.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-seginfo)`. It returns a list: A, X, r0.

HyForth: `sys-seginfo`. HyBASIC: `SYS "SEGINFO"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## SEG_ATTACH

System call $F8A5, one of those for memory. It never waits.

#### Arguments

A
: A segment.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Attach the calling task to a segment (attaching twice is once).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | No such segment |

#### From the languages

hylang: `(sys-seg-attach A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-seg-attach`. HyBASIC: `SYS "SEG_ATTACH"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## SEG_CREATE

System call $F8A2, one of those for memory. It never waits.

#### Arguments

A
: Banks (1-128, 8K each)

#### Result

A
: The segment (0-15)

#### Description

A shared segment: shared RAM banks any task can attach to, with the calling task attached. It stays while a task is attached (`SEG_DETACH`, or the task's end).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | 0 |
| $06 | `E_NOMEM` | out of memory |  |

#### From the languages

hylang: `(sys-seg-create A)`, where *A* is a number. It returns A.

HyForth: `sys-seg-create`. HyBASIC: `SYS "SEG_CREATE"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## SEG_CREATE_IN

System call $F8B4, one of those for memory. It never waits.

#### Arguments

A
: How many banks (1-128)

X
: The lowest shared bank ID it may take.

Y
: The highest (U << 4 | the bank's low nibble; segments have $80-$FF)

#### Result

A
: The segment.

#### Description

`SEG_CREATE`, from a range of the shared RAM (the shared RAM disk's start FROM-TO).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | 0 |
| $06 | `E_NOMEM` | out of memory | No run that long there |

#### From the languages

hylang: `(sys-seg-create-in A X Y)`, where *A* is a number; *X* is a number; *Y* is a number. It returns A.

HyForth: `sys-seg-create-in`. HyBASIC: `SYS "SEG_CREATE_IN"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## SEG_DETACH

System call $F8A8, one of those for memory. It never waits.

#### Arguments

A
: A segment.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Detach the calling task from a segment; the last task to go frees it.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not attached |

#### From the languages

hylang: `(sys-seg-detach A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-seg-detach`. HyBASIC: `SYS "SEG_DETACH"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_MAP`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## SEG_MAP

System call $F8AB, one of those for memory. It never waits.

#### Arguments

A
: A segment.

X
: One of its banks (0 on)

#### Result

A
: The value for U.

X
: The value for $00.

#### Description

How to reach a bank of a segment: write U, then $00, and it's at $8000-$9FFF.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not attached |
| $08 | `E_RANGE` | out of range | No such bank in it |

#### From the languages

hylang: `(sys-seg-map A X)`, where *A* is a number; *X* is a number. It returns a list: A, X.

HyForth: `sys-seg-map`. HyBASIC: `SYS "SEG_MAP"`.

#### See also

The other calls for memory: `BREAK`, `PAGES_ALLOC`, `PAGES_FREE`, `BANKS`, `BANKS_ALLOC`, `BANKS_FREE`, `SEG_CREATE`, `SEG_ATTACH`, `SEG_DETACH`, `SEGINFO`, `BANKS_ALLOC_IN`, `SEG_CREATE_IN`, `BANK_LOAD`, `BANK_FLUSH`.

## SEM_ACQUIRE

System call $F86F, one of those for tasks. It can wait.

#### Arguments

A
: A semaphore.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Take one of a semaphore's count; if there's none, wait (using no CPU) till `SEM_RELEASE` gives one back. A mutex's holder is then this task.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not a semaphore, or freed while this task waited |
| $07 | `E_BUSY` | busy | A mutex this task holds already |
| $05 | `E_INTR` | interrupted | A note came: taken on the way out |

#### From the languages

hylang: `(sys-sem-acquire A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-sem-acquire`. HyBASIC: `SYS "SEM_ACQUIRE"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## SEM_FREE

System call $F878, one of those for tasks. It never waits.

#### Arguments

A
: A semaphore.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Free a semaphore (any task may): its number can be made again, and the tasks waiting for it wake with `E_INVAL`. The end of the task that made it frees it too, and a task's end gives back the mutexes it holds.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not a semaphore |

#### From the languages

hylang: `(sys-sem-free A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-sem-free`. HyBASIC: `SYS "SEM_FREE"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## SEM_NEW

System call $F86C, one of those for tasks. It never waits.

#### Arguments

A
: Its count: how many can take it before a taker waits (0-255)

X
: 0, or `SEM_MUTEX`: a mutex (its count 1, and it has a holder: only the task that took it gives it back)

#### Result

A
: The semaphore (0 to `SEM_MAX` - 1)

#### Description

Make a semaphore, for tasks that share something (a shared segment, a device) or wait for each other. Every task sees the same ones, by number. It lasts till `SEM_FREE`, or the end of the task that made it.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $06 | `E_NOMEM` | out of memory | None free: `SEM_MAX` in all |

#### From the languages

hylang: `(sys-sem-new A X)`, where *A* is a number; *X* is a number, or left out. It returns A.

HyForth: `sys-sem-new`. HyBASIC: `SYS "SEM_NEW"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## SEM_RELEASE

System call $F875, one of those for tasks. It never waits.

#### Arguments

A
: A semaphore.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Give one back to a semaphore's count, and wake the tasks waiting for it: the first to run takes it.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not a semaphore |
| $01 | `E_PERM` | not allowed | A mutex this task doesn't hold |
| $08 | `E_RANGE` | out of range | Its count is 255 |

#### From the languages

hylang: `(sys-sem-release A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-sem-release`. HyBASIC: `SYS "SEM_RELEASE"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## SEM_TRY

System call $F872, one of those for tasks. It never waits.

#### Arguments

A
: A semaphore.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

`SEM_ACQUIRE` without the wait.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not a semaphore |
| $07 | `E_BUSY` | busy | A mutex this task holds already |
| $04 | `E_AGAIN` | not yet: try again | None to take |

#### From the languages

hylang: `(sys-sem-try A)`, where *A* is a number. It returns `NIL`.

HyForth: `sys-sem-try`. HyBASIC: `SYS "SEM_TRY"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## SEM_WAIT

System call $F87E, one of those for tasks. It can wait.

#### Arguments

A
: A semaphore.

X
: The most ticks to wait (`TICK_HZ` a second; 0: none, as `SEM_TRY`)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

`SEM_ACQUIRE` with a time limit: take one of a semaphore's count; if there's none, wait (using no CPU) till `SEM_RELEASE` gives one back or the ticks have passed, whichever comes first. A doorbell with a timeout: a task with work of its own to do every so often (a clock to keep) sleeps on its semaphore, and is woken at once when it's rung.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $04 | `E_AGAIN` | not yet: try again | The time came and none to take |
| $02 | `E_INVAL` | invalid argument | Not a semaphore, or freed while this task waited |
| $07 | `E_BUSY` | busy | A mutex this task holds already |
| $05 | `E_INTR` | interrupted | A note came: taken on the way out |

#### From the languages

hylang: `(sys-sem-wait A X)`, where *A* is a number; *X* is a number. It returns `NIL`.

HyForth: `sys-sem-wait`. HyBASIC: `SYS "SEM_WAIT"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `RUN`, `TASKFORE`.

## SLEEP

System call $F83F, one of those for tasks. It can wait.

#### Arguments

A/X
: Ticks (`TICK_HZ` a second; 0-32767)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Sleep for a number of ticks. The other tasks run meanwhile, or the system idles.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $05 | `E_INTR` | interrupted | A note woke it early: taken on the way out |

#### From the languages

hylang: `(sys-sleep A/X)`, where *A/X* is a number. It returns `NIL`.

HyForth: `sys-sleep`. HyBASIC: `SYS "SLEEP"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## SLEEP_UNTIL

System call $F842, one of those for tasks. It can wait.

#### Arguments

A/X
: A tick count (at most 32767 ticks ahead)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Sleep until the tick count reaches a value: for steady timing (a time that's passed returns at once).

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $05 | `E_INTR` | interrupted | A note woke it early: taken on the way out |

#### From the languages

hylang: `(sys-sleep-until A/X)`, where *A/X* is a number. It returns `NIL`.

HyForth: `sys-sleep-until`. HyBASIC: `SYS "SLEEP_UNTIL"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## SPAWN

System call $F836, one of those for tasks. It can wait.

#### Arguments

r0
: The program's path (through the namespace: `/bin/NAME`; or `#m/NAME`, a module in the paged ROM)

r1
: Its arguments (zero-terminated strings, an empty one after the last: 176 bytes at most), or 0.

A
: Flags (`SPAWN_NEWGROUP`, `SPAWN_NEWNS`, `SPAWN_FDMAP`, `SPAWN_NOENV`, `SPAWN_STOPPED`, `SPAWN_RUN`, `RUN_LEND`)

r2
: An fd map (with `SPAWN_FDMAP`)

#### Result

A
: The new task (`SPAWN_RUN`: this one, if the program ran in it)

#### Description

Start a program in a task of its own. `SPAWN` opens the file and reads its HYX2 header (`HX_*`): a module of the paged ROM (`HF_INPLACE`: `#m`'s files) runs in place, in its bank (the module directory's of its name); any other program is read into the new task's RAM at its load address ($0800), by the task itself as it starts, and its BSS cleared (one that can't be read ends at once, with the error as its exit code). It starts at its entry point with r0 pointing to its arguments (at `TASK_ARGS`: the list), its name the header's. Its fds: the caller's 0, 1 and 2; or with `SPAWN_FDMAP`, the map's: a count (`SPAWN_FDS` at most), then for each of the child's fds from 0 the caller's fd that becomes it ($FF, or a closed one: closed). Each shares the caller's channel and offset; the rest are closed. Its parent is the caller, which can `WAIT` for it. It's in the caller's note group unless `SPAWN_NEWGROUP` starts one (its number: the child's), and it shares the caller's namespace unless `SPAWN_NEWNS` gives it an empty one. Its environment is a copy of the caller's (`SPAWN_NOENV`: empty). `SPAWN_STOPPED`: it's stopped at its entry point, for the debugger (`TASKSTEP`; `/proc/N/ctl`'s step, or start). A bank program (`HF_BANK`) is loaded into a RAM bank of its task's own and runs there. `SPAWN_RUN`, for a caller that waits for the program at once: a bank program that may run in place (not `HF_TASK`) runs in this task (`RUN`, with the map and `RUN_LEND` as given) before `SPAWN` returns, and the task given back is this one, whose `WAIT` gives the program's exit status; any other program, or with `SPAWN_NEWGROUP`, `SPAWN_NEWNS`, `SPAWN_NOENV` or `SPAWN_STOPPED`, starts in a task of its own as ever. rc runs a simple command this way.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found | No such file |
| $2D | `E_NOEXEC` | not a program | Not a program this kernel runs |
| $40 | `E_NOTASK` | no free task | No free task |
| $0B | `E_TOOBIG` | too big | The arguments |
| $02 | `E_INVAL` | invalid argument | The map |
| $26 | `E_MFILE` | no free file descriptor |  |
| $05 | `E_INTR` | interrupted |  |

Also what else `OPEN` and `READ` give.

#### From the languages

hylang: `(sys-spawn r0 r1 A r2)`, where *r0* is a string; *r1* is a list of strings, or `NIL`; *A* is a number, or left out; *r2* is a list of bytes, or `NIL`. It returns A.

HyForth: `sys-spawn`. HyBASIC: `SYS "SPAWN"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## SRV_NAP

System call $F9DD, one of those for drivers. It can wait.

#### Arguments

A/X
: Ticks (1-32767)

Or 0, and Y = the driver's event count (`TASK_EVENT`) as it last saw it.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

For a driver with work of its own between its calls (a device to look at now and then): its init doesn't return, but waits here between its turns. While it waits it serves its calls, as a driver that has started does (the boot goes on); when the time has come it runs again, busy (a call to it waits) till its next `SRV_NAP`, or till its init returns (it's an ordinary driver from then). With 0 ticks it waits till its own event count isn't Y any more (its irq entry adds to it, or one of its calls): for work that follows what happens, not the clock. ps shows it asleep, or waiting for an event.

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `INIT_RUN`.

## SRV_REGISTER

System call $F9BF, one of those for drivers. It never waits.

#### Arguments

A
: A device letter (printable; not #)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Serve a device: its requests ("#x...") come to the calling task's serve entry. A task may serve several letters (the request block names it: `RQ_DEV`). Freed when the task ends.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $02 | `E_INVAL` | invalid argument | Not a letter |
| $21 | `E_EXIST` | already exists | Another task's |
| $06 | `E_NOMEM` | out of memory | No room |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## SRV_REPLY

System call $F9C5, one of those for drivers. It never waits.

#### Arguments

`TASK_INBOX`'s `RQ_FID`, `RQ_DONE`, `RQ_PERM`, `RQ_EVENT`: what the request returns. A, C: the answer.

#### Result

A and C kept.

#### Description

At the end of the serve entry: what the request returns, back to the client (srvlib does it).

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## SRV_TAKE

System call $F9C2, one of those for drivers. It never waits.

#### Arguments

None.

#### Result

A
: The request (`R_*`)

Y
: The client.

The request block in `TASK_INBOX`, and (`R_OPEN`, `R_CREATE`, `R_REMOVE`) the name in `TASK_PATH`.

#### Description

In the serve entry: take the request being served from the client (srvlib does it). It notes the server's event count (`TASK_EVENT`) in `RQ_EVENT`: a client answered `E_AGAIN` waits for it to change.

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## STAT

System call $F8D2, one of those for files. It can wait.

#### Arguments

r0
: A path.

r1
: A buffer (`SR_SIZE` bytes)

#### Result

The file's stat record in the buffer (`SR_*`)

#### Description

A file's stat record: its name, qid, mode, length, time, device letter.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found |  |
| $2B | `E_NODEV` | no such device |  |
| $0A | `E_NAMETOOLONG` | name too long |  |
| $26 | `E_MFILE` | no free file descriptor |  |
| $27 | `E_NFILE` | too many open files |  |

#### From the languages

hylang: `(sys-stat r0)`, where *r0* is a string. It returns the stat record, as a hash. (Here r1 is a buffer the function makes itself.)

HyForth: `sys-stat`. HyBASIC: `SYS "STAT"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## SYSINFO

System call $F800, one of those for the system. It never waits.

#### Arguments

None.

#### Result

A
: The ABI version (1)

X
: How many RAM modules are installed.

r0
: The free tasks (bit = task)

#### Description

What the system has.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-sysinfo)`. It returns a list: A, X, r0.

HyForth: `sys-sysinfo`. HyBASIC: `SYS "SYSINFO"`.

#### See also

The other calls for the system: `ERRSTR`, `KMESG`, `REBOOT`, `XCALL`.

## TASKFORE

System call $F884, one of those for tasks. It never waits.

#### Arguments

A
: A task: this one, or one it started.

X
: 1 to put it in the foreground, 0 to take it out.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Put a task in the foreground, or take it out. The scheduler picks a task in the foreground before the others whenever it can run: at the next switch (a tick at most), not after a round of everything else that's busy; and a driver serving its call goes first with it. For a program whose answer to a key runs through tasks of its own while others work in the background (a game's: its UI, parser and world first; its monsters' minds after). It can't starve the rest: the foreground has `FORE_RUN` ticks at most, then the background the rest of a tick (its tasks in turn), so every task gets its turn. A task waits in the foreground as any task waits (sleeping, on a semaphore: no CPU). `TASKINFO`'s `TI_FLAGS` has `TF_FORE` while it's in. It goes with the task: gone when the task ends, or EXECs another program.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $01 | `E_PERM` | not allowed | The kernel task, a driver, or another program's task |
| $41 | `E_SRCH` | no such task | Not a task, or not started |

#### From the languages

hylang: `(sys-taskfore A X)`, where *A* is a number; *X* is a number. It returns `NIL`.

HyForth: `sys-taskfore`. HyBASIC: `SYS "TASKFORE"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`.

## TASKINFO

System call $F84E, one of those for tasks. It never waits.

#### Arguments

A
: A task (0-15)

r0
: A buffer (`TI_SIZE` bytes)

#### Result

The buffer: `TI_STATE`, `TI_FLAGS`, `TI_PARENT`, `TI_CPU` (3 bytes: ticks), `TI_BANK`, `TI_TYPE`, `TI_NAME` (16)

#### Description

What the kernel knows of a task, for ps (and `/proc`'s source). A task's CPU time is the ticks that interrupted it. `TI_STATE` is 0 for a free task.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $41 | `E_SRCH` | no such task | Not a task |

#### From the languages

hylang: `(sys-taskinfo A)`, where *A* is a number. It returns the buffer, as a string. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-taskinfo`. HyBASIC: `SYS "TASKINFO"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## TASKMEM

System call $F9D4, one of those for drivers. It never waits.

#### Arguments

A
: A task.

X
: `TM_READ` or `TM_WRITE`, | `TM_BANK`.

r0
: A buffer.

r1
: An address in the task's view, $0000-$DFFF (with `TM_BANK`: $8000-$9FFF, in its RAM bank r3)

r2
: A count (the range ends at $E000, or with `TM_BANK` at $A000, at most)

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Bytes from a task's memory into the buffer, or from the buffer into it: `/proc`'s mem and ram (kdev, which decides who may: a program reaches another task's memory only through those files). Its task RAM, and its RAM bank and paged ROM bank as it has them selected; or with `TM_BANK` any of its RAM banks (its bank register points at it meanwhile: not for a driver, whose irq entry might run). A byte at a time, IRQs off for each.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $01 | `E_PERM` | not allowed | The caller isn't a driver |
| $41 | `E_SRCH` | no such task | Not a task |
| $02 | `E_INVAL` | invalid argument | Past the range |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## TASKREAD

System call $F866, one of those for tasks. It never waits.

#### Arguments

A
: A task ($FF: this one)

X
: What: `TR_ARGS` (its arguments: `ARGS_MAX` bytes), `TR_CWD` (its current directory: `PATH_MAX` + 1 bytes), `TR_ENV` (its environment's first variables, whole, and a 0: `ENV_SIZE` bytes at most), `TR_FRAME` (its state and the frame it left as it was switched out: `TF_SIZE` bytes, `TF_*`) or `TR_ENVAT` (part of its environment), or `TR_FD` (its fd r2: `FI_SIZE` bytes, `FI_*`)

r0
: A buffer that big.

r1
: `TR_ENVAT`'s count (the most it copies)

r2
: `TR_ENVAT`'s offset, or `TR_FD`'s fd.

#### Result

A/X
: `TR_ENVAT`'s: the environment's bytes in use (its 0 too: it copied as many as there are from r2, r1 at most)

The bytes in the buffer.

#### Description

Another task's arguments, current directory, environment, registers or open files, as they are now: `/proc`'s args, cwd, env, regs and fd, ps. The kernel task copies them a byte at a time, with IRQs off for each. An environment may be `ENV_MAX` bytes: `TR_ENVAT` reads one whole, a part at a time.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $41 | `E_SRCH` | no such task |  |
| $02 | `E_INVAL` | invalid argument | Another what |
| $25 | `E_BADF` | bad file descriptor | `TR_FD`: not an open fd |

#### From the languages

hylang: `(sys-taskread A X count r2)`, where *A* is a number; *X* is a number; *count* is the most to take: r1; *r2* is a number, or left out. It returns a list: the buffer, as a string, A/X. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-taskread`. HyBASIC: `SYS "TASKREAD"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## TASKSTEP

System call $F9DA, one of those for drivers. It never waits.

#### Arguments

A
: A task.

X
: `TS_STEP`, `TS_NEXT`, `TS_BREAKS` or `TS_NOBREAKS`.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

The debugger's step (`/proc`'s ctl: kdev). `TS_STEP`: a stopped task runs its next instruction and stops again (`TI_FLAGS`: `TF_STOPPED` once it has). JMP, JSR, RTS and RTI are done at once, on its frame; others run out of line, in its zero page, a BRK after them. `TS_NEXT`: the same, but a JSR runs its subroutine whole, as a JSR to the kernel's jump table always does (a subroutine that reads bytes after its JSR can't be stepped over). `TS_BREAKS`: from now any BRK stops the task, its PC back on the BRK (the debugger's breakpoints, BRKs it writes into the program); `TS_NOBREAKS`: a BRK is the program's own again (`NOTE_BRK`). `SPAWN_STOPPED` starts a program stopped at its entry point.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $01 | `E_PERM` | not allowed | The caller isn't a driver; the kernel task, a driver |
| $41 | `E_SRCH` | no such task | Not a task, or not started |
| $02 | `E_INVAL` | invalid argument | Another X; to step: a BRK or STP |
| $07 | `E_BUSY` | busy | To step: not stopped, or a step still out |

Or in the kernel: in a call, its PC at $E000 or above, or W not 0.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `SRV_NAP`, `INIT_RUN`.

## TASKSTOP

System call $F9D7, one of those for drivers. It never waits.

#### Arguments

A
: A task.

X
: 1 to stop it, 0 to start it again.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Stop a task where it is (the scheduler passes it by: its waits go on, and end, but it doesn't run), or start it again: `/proc`'s ctl (kdev, which decides who may), for the debugger. `TASKINFO`'s `TI_FLAGS` has `TF_STOPPED` while it's stopped. A kill note starts it, to end it.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $01 | `E_PERM` | not allowed | The caller isn't a driver; the kernel task, a driver |
| $41 | `E_SRCH` | no such task | Not a task, or not started |

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `WAKE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## TICKS

System call $F845, one of those for tasks. It never waits.

#### Arguments

None.

#### Result

A/X
: The tick count (`TICK_HZ` a second; it wraps after about 5.5 minutes)

#### Description

The scheduler's tick count.

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-ticks)`. It returns A/X.

HyForth: `sys-ticks`. HyBASIC: `SYS "TICKS"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## TIME

System call $F980, one of those for the clock. It never waits.

#### Arguments

None.

#### Result

r0/r1
: The clock: seconds since 2000-01-01 00:00:00.

#### Description

The clock: the boot's time and the ticks since. It starts at 0 (2000-01-01) unless kdev finds a DS1747 with a date and time as it starts; a write of `/dev/time` sets it (and the chip).

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-time)`. It returns r0/r1.

HyForth: `sys-time`. HyBASIC: `SYS "TIME"`.

#### See also

The other calls for the clock: `TIME_SET`, `RTC`.

## TIME_SET

System call $F983, one of those for the clock. It never waits.

#### Arguments

r0/r1
: Seconds since 2000-01-01 00:00:00.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Set the clock (from now: the boot's time becomes the clock less the ticks since). The DS1747 is the caller's to set (`RTC`).

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-time-set r0/r1)`, where *r0/r1* is a number. It returns `NIL`.

HyForth: `sys-time-set`. HyBASIC: `SYS "TIME_SET"`.

#### See also

The other calls for the clock: `TIME`, `RTC`.

## UNMOUNT

System call $F926, one of those for namespaces. It can wait.

#### Arguments

r0
: New, or 0.

r1
: Old, or 0.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Undo a bind or a mount at old: all of what's there (r0 = 0), or new's member (as `BIND` finds new). With no old either (r1 = 0), everything: the caller has an empty namespace of its own (the tasks it shared one with keep theirs): what Plan 9's newns starts from, and nslib's ns_default does first.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found |  |
| $02 | `E_INVAL` | invalid argument |  |
| $50 | `E_NSFULL` | namespace full |  |
| $0A | `E_NAMETOOLONG` | name too long |  |

#### From the languages

hylang: `(sys-unmount r0 r1)`, where *r0* is a string, or `NIL`; *r1* is a string, or `NIL`. It returns `NIL`.

HyForth: `sys-unmount`. HyBASIC: `SYS "UNMOUNT"`.

#### See also

The other calls for namespaces: `BIND`, `MOUNT`, `NSINFO`.

## WAIT

System call $F833, one of those for tasks. It can wait.

#### Arguments

A
: A task this one started, or $FF for any.

r0
: A buffer for the message (32 bytes), or 0.

#### Result

A
: The task that ended.

X
: Its exit code.

Its message in r0's buffer ("" for none)

#### Description

Wait for a child to end, and take its exit status. A child that has ended already isn't waited for.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $42 | `E_CHILD` | no such child | No such child, or none at all |
| $05 | `E_INTR` | interrupted | A note came: taken on the way out |

#### From the languages

hylang: `(sys-wait A)`, where *A* is a number. It returns a list: A, X, the buffer's text. (Here r0 is a buffer the function makes itself.)

HyForth: `sys-wait`. HyBASIC: `SYS "WAIT"`.

#### See also

The other calls for tasks: `EXITS`, `SPAWN`, `GETPID`, `YIELD`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## WAKE

System call $F9B6, one of those for drivers. It never waits.

#### Arguments

A
: A task.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Make a waiting task (`PAUSE`) runnable. From a task or an irq entry. Waking a task that isn't waiting does nothing.

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for drivers: `IRQ_OWN`, `IRQ_RELEASE`, `PAUSE`, `NOTE_POST`, `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `NOTE_QUEUE`, `ROMREAD`, `TASKMEM`, `TASKSTOP`, `TASKSTEP`, `SRV_NAP`, `INIT_RUN`.

## WRITE

System call $F8CC, one of those for files. It can wait.

#### Arguments

A
: An fd.

r0
: The data.

r1
: Its count.

#### Result

A/X
: The count written.

#### Description

Write at the fd's offset, and move it on.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $25 | `E_BADF` | bad file descriptor | Not open for writing |
| $04 | `E_AGAIN` | not yet: try again |  |
| $05 | `E_INTR` | interrupted |  |

#### From the languages

hylang: `(sys-write A r0)`, where *A* is a number; *r0* is a string or a list of bytes (its length goes in r1). It returns A/X.

HyForth: `sys-write`. HyBASIC: `SYS "WRITE"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `SEEK`, `STAT`, `FSTAT`, `WSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## WSTAT

System call $F8D8, one of those for files. It can wait.

#### Arguments

r0
: A path.

r1
: A stat record.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Change a file from a stat record: its name, its mode, its length (what its server allows: HydraFS a file's length, longer with zeros or shorter). What's all $FF in the record (an empty name) is left as it is.

#### Errors

| Code | Name | Message | Cause |
|---|---|---|---|
| $20 | `E_NOENT` | not found |  |
| $2B | `E_NODEV` | no such device |  |
| $0A | `E_NAMETOOLONG` | name too long |  |
| $26 | `E_MFILE` | no free file descriptor |  |
| $27 | `E_NFILE` | too many open files |  |
| $03 | `E_NOSYS` | no such call | Its server changes nothing |

#### From the languages

hylang: `(sys-wstat r0 r1)`, where *r0* is a string; *r1* is a hash, as `stat` gives. It returns `NIL`.

HyForth: `sys-wstat`. HyBASIC: `SYS "WSTAT"`.

#### See also

The other calls for files: `OPEN`, `CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `STAT`, `FSTAT`, `FWSTAT`, `REMOVE`, `DUP`, `DUP2`, `CHDIR`, `GETCWD`, `PIPE`, `FD2PATH`.

## XCALL

System call $F80C, one of those for the system. It never waits.

#### Arguments

r15
: A routine's address ($A000-$DFFF)

r14 (its low byte)
: The paged ROM bank it's in.

A, X, Y, the flags and r0-r13: the routine's.

#### Result

A, X, Y, the flags and r0-r15 as the routine leaves them.

#### Description

Call a routine in another bank of the paged ROM (a library module's: `HT_LIBRARY`, its bank from `MODINFO`), as the X16's jsrfar: the caller's bank register ($01) is set to the routine's for the call and back after it, and everything else passes through both ways. The routine runs in the caller's task, with its RAM and zero page. A note may come while it runs: a program that makes far calls keeps its note handler in RAM, as a module of several banks does.

#### Errors

None: the call always succeeds.

#### From the languages

Not in the languages: it is a call for drivers, or for the debugger.

#### See also

The other calls for the system: `SYSINFO`, `ERRSTR`, `KMESG`, `REBOOT`.

## YIELD

System call $F83C, one of those for tasks. It never waits.

#### Arguments

None.

#### Result

Nothing, but the carry: C = 0 for success.

#### Description

Let the other tasks run; come back when it's this one's turn again (at once, if nothing else can run).

#### Errors

None: the call always succeeds.

#### From the languages

hylang: `(sys-yield)`. It returns `NIL`.

HyForth: `sys-yield`. HyBASIC: `SYS "YIELD"`.

#### See also

The other calls for tasks: `EXITS`, `WAIT`, `SPAWN`, `GETPID`, `SLEEP`, `SLEEP_UNTIL`, `TICKS`, `PREEMPT_OFF`, `PREEMPT_ON`, `TASKINFO`, `NOTIFY`, `NOTE`, `MODINFO`, `ENV_GET`, `ENV_PUT`, `ENV_DEL`, `ENV_NAME`, `TASKREAD`, `GETPPID`, `SEM_NEW`, `SEM_ACQUIRE`, `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`, `EXEC`, `SEM_WAIT`, `RUN`, `TASKFORE`.

## Constants

Every constant that `hydra.inc` defines for programs, by family. A value with a `$` is hexadecimal.

| Name | Value | Meaning |
|---|---|---|
| `ABI_VERSION` | 1 | The ABI version this include describes |
| `TICK_HZ` | 200 | Ticks a second |
| `INIT_TASK` | 1 | init's task (and its note group: the console's foreground at first) |
| `PROG_ZP` | $22 | A program's own zero page: $22-$7F |
| `PROG_ZP_END` | $80 |  |
| `RB_DIR` | $9F00 | A code cache bank's directory (`BANK_LOAD`): its entries at `RB_DIR` + 16 x n, n 1-15 |
| `RE_START` | 0 | A cache directory entry: where its module is (2) |
| `RE_END` | 2 | A cache directory entry: the first byte past it (2) |
| `RE_TABLE` | 4 | A cache directory entry: a child's exports' table (2) |
| `RE_GEN` | 6 | A cache directory entry: its load's number (0: free) |
| `RE_FLAGS` | 7 | A cache directory entry: `RF_*` |
| `RE_STAMP` | 8 | A cache directory entry: when it was last used (`TK_RCLOCK`) (2) |
| `TK_RCLOCK` | $EC | A task's code cache's uses counted (2): its zero page's |
| `RC_BANKS` | 8 | A task's code cache's banks at most, to start with (`BANK_FLUSH`) |
| `BL_FILE` | 2 | `BANK_LOAD`'s why (X): its file not there, or not read |
| `BL_FORMAT` | 3 | `BANK_LOAD`'s why (X): not a child (o65, linked at $8000, in a bank) |
| `BL_ZP` | 4 | `BANK_LOAD`'s why (X): its zero page more than the caller gives |
| `BL_EXPORTS` | 5 | `BANK_LOAD`'s why (X): the caller's exports' file not there, or not of its form |
| `BL_IMPORT` | 6 | `BANK_LOAD`'s why (X): an import the caller hasn't (its name in `TA_SCRATCH`) |
| `BL_BANK` | 7 | `BANK_LOAD`'s why (X): no bank, or no room in one |
| `TA_SCRATCH` | $0280 | A task's call scratch (32 bytes): an exit message, `BANK_LOAD`'s missing import |
| `TASK_ARGS` | $0350 | A task's arguments: zero-terminated strings, an empty one after the last (176 bytes at most) |
| `TASK_NAME` | $0230 | A task's name, zero-terminated |
| `TASK_EVENT` | $BD | A task's event count: a server adds 1 (inc `TASK_EVENT`) when what its clients wait for has happened |
| `TASK_INBOX` | $0200 | A server's copy of the request it's serving (`RQ_*`) |
| `TASK_PATH` | $02C0 | A server's copy of the name a request names (64 bytes) |
| `ARGS_MAX` | 176 | `SPAWN`'s arguments: their bytes at most (each string's 0, and the empty one that ends them) |
| `IRQ_RESCHED` | $80 | An irq entry's answer: a task switch, please |
| `LINE_VIA_T2` | 16 | `IRQ_OWN`'s line for VIA timer 2 (the hardware's lines: hw.inc) |
| `LINE_VIA_CA1` | 17 | `IRQ_OWN`'s line for VIA CA1 (J27's pin 11): owning it turns CA1's interrupt on; the owner clears its flag |
| `HT_PROGRAM` | 1 | HYX2 module types |
| `HT_DRIVER` | 2 |  |
| `HT_LIBRARY` | 3 |  |
| `HF_INPLACE` | $01 | HYX2 flags: runs in place in the paged ROM |
| `HF_BOOT` | $02 | HYX2 flags: a driver the kernel starts at boot |
| `HF_BANK` | $04 | HYX2 flags: a bank program: an o65 module after the header, run in a RAM bank (`RUN`) |
| `HF_TASK` | $08 | HYX2 flags: a bank program that runs only in a task of its own (not in place: `RUN`) |
| `HX_MAGIC` | 0 | The HYX2 header (`HX_SIZE` bytes, a program's first): HYX2 |
| `HX_HSIZE` | 4 | HYX2: the header's size (48) |
| `HX_TYPE` | 5 | HYX2: the type (`HT_*`) |
| `HX_FLAGS` | 6 | HYX2: the flags (`HF_*`) |
| `HX_ABI` | 7 | HYX2: the ABI version it was built for |
| `HX_LOAD` | 8 | HYX2: its load address (2: $A000 in place, $0800 a RAM program) |
| `HX_LENGTH` | 10 | HYX2: its image's length (2: in its last bank, in place) |
| `HX_DATA_LOAD` | 12 | HYX2: its data, where it is in the image (2) ... |
| `HX_DATA_RUN` | 14 | HYX2: ... where it runs (2) ... |
| `HX_DATA_LEN` | 16 | HYX2: ... and its length (2) |
| `HX_BSS` | 18 | HYX2: its BSS (2) ... |
| `HX_BSS_LEN` | 20 | HYX2: ... and its length (2: cleared as it starts) |
| `HX_TOP` | 22 | HYX2: the top of the RAM it uses (2: its break starts there) |
| `HX_MAIN` | 24 | HYX2: entry: main (a program), init (a driver) |
| `HX_SERVE` | 26 | HYX2: entry: serve (a driver) |
| `HX_IRQ` | 28 | HYX2: entry: irq (a driver) |
| `HX_STOP` | 30 | HYX2: entry: stop (a driver) |
| `HX_DEVICE` | 32 | HYX2: its device letter (a driver) |
| `HX_BANKS` | 33 | HYX2: the banks it spans (in place) |
| `HX_VERSION` | 34 | HYX2: its version (2) |
| `HX_NAME` | 36 | HYX2: its name (12: zero-terminated) |
| `HX_SIZE` | 48 |  |
| `ME_BANK` | 0 | `MODINFO`'s answer (`ME_SIZE` bytes): the module's first paged ROM bank |
| `ME_BANKS` | 1 | `MODINFO`: the banks it spans |
| `ME_TYPE` | 2 | `MODINFO`: its type (`HT_*`) |
| `ME_FLAGS` | 3 | `MODINFO`: its flags (`HF_*`) |
| `ME_NAME` | 4 | `MODINFO`: its name (11 bytes, zero-padded: 10 characters at most) |
| `ME_PAGE` | 15 | `MODINFO`: its header's page in its first bank, 0-63 ($A000 + 256 * it: a module of a bank, packed with others) |
| `ME_SIZE` | 16 |  |
| `TI_STATE` | 0 | `TASKINFO`'s answer: the task's state (0: free, 1 ready, 2 wait, 3 call, 4 idle, 5 new, 6 sleep, 7 blocked, 8 event) |
| `TI_FLAGS` | 1 | `TASKINFO`: its flags (`TF_DRIVER`, `TF_FORE`, `TF_STOPPED`) |
| `TI_PARENT` | 2 | `TASKINFO`: its parent ($FF: none) |
| `TI_CPU` | 3 | `TASKINFO`: its CPU time, in ticks (3 bytes) |
| `TI_BANK` | 6 | `TASKINFO`: its module's paged ROM bank ($FF: none) |
| `TI_TYPE` | 7 | `TASKINFO`: its module's type (`HT_*`; 0: none) |
| `TI_NAME` | 8 | `TASKINFO`: its name, zero-terminated (16 bytes) |
| `TI_GROUP` | 24 | `TASKINFO`: its note group |
| `TI_SIZE` | 25 | `TASKINFO`: the answer's size |
| `TF_DRIVER` | $01 | `TI_FLAGS`: a driver |
| `TF_STOPPED` | $80 | `TI_FLAGS`: stopped (`/proc`'s ctl: `TASKSTOP`) |
| `TF_FORE` | $02 | `TI_FLAGS`: in the foreground (`TASKFORE`): the scheduler picks it first |
| `TF_STATE` | 0 | `TR_FRAME`'s: the task's state (0 free, 1 ready, 2 waiting, 3 calling, 4 idle, 5 new, 6 sleeping, 7 blocked, 8 waiting for an event) |
| `TF_S` | 1 | `TR_FRAME`'s: its stack pointer, as it was before the frame |
| `TF_U` | 2 | `TR_FRAME`'s: the frame: U |
| `TF_Y` | 3 |  |
| `TF_W` | 4 |  |
| `TF_X` | 5 |  |
| `TF_A` | 6 |  |
| `TF_P` | 7 |  |
| `TF_PC` | 8 | `TR_FRAME`'s: PC (2) |
| `TF_RAM` | 10 | `TR_FRAME`'s: its RAM bank register ($00) |
| `TF_ROM` | 11 | `TR_FRAME`'s: its paged ROM bank register ($01) |
| `TF_SIZE` | 12 |  |
| `FORE_RUN` | 8 | The foreground's turn at most, in ticks (`TASKFORE`): then the background has the rest of a tick |
| `NOTE_INTERRUPT` | 1 | Notes: Ctrl-C (default: the end, 130) |
| `NOTE_KILL` | 2 | Notes: kill, never caught (137) |
| `NOTE_HANGUP` | 3 | Notes: hangup (129) |
| `NOTE_ALARM` | 4 | Notes: alarm (142) |
| `NOTE_BRK` | 5 | Notes: sys: brk, a BRK instruction (133) |
| `NOTE_USER` | 16 | Notes: 16-31 are the programs' own (128 + the note) |
| `NOTE_GROUP` | $80 | `NOTE`'s A: a note group, not a task |
| `SEM_MUTEX` | $01 | `SEM_NEW`'s X: a mutex (its count 1, given back only by its holder) |
| `SEM_MAX` | 16 | Semaphores: as many as there can be at once |
| `REBOOT_HWTEST` | $01 | `REBOOT`'s A: into the hardware test |
| `SPAWN_NEWGROUP` | $01 | `SPAWN`'s flags: the child starts a note group of its own |
| `SPAWN_NEWNS` | $02 | `SPAWN`'s flags: the child starts with an empty namespace of its own (else it shares its parent's) |
| `SPAWN_FDMAP` | $04 | `SPAWN`'s flags: the child's fds are r2's map (else the caller's 0, 1 and 2) |
| `SPAWN_NOENV` | $08 | `SPAWN`'s flags: the child starts with an empty environment (else a copy of its parent's) |
| `SPAWN_STOPPED` | $10 | `SPAWN`'s flags: the child stops at its entry point, for the debugger (`/proc/N/ctl`'s step) |
| `SPAWN_RUN` | $20 | `SPAWN`'s flags: a bank program that runs in place does, in this task (`RUN`), and `SPAWN` gives this task back |
| `SPAWN_FDS` | 15 | `SPAWN`'s fd map: the child's fds it can give, at most (its fd 15 is its loader's as it starts) |
| `RUN_LEND` | $40 | `RUN`'s flags (and `SPAWN_RUN`'s): the program may move the break past the caller's |
| `ENV_SIZE` | 1024 | `TASKREAD`'s `TR_ENV`: its buffer, an environment's first variables, whole, and a 0 |
| `ENV_MAX` | 8192 | An environment's bytes at most: each variable its name's length, its name, its value's length (2), its value; a 0 |
| `ENV_NAME_MAX` | 31 | A variable's name's length at most |
| `MREPL` | 0 | `BIND`'s and `MOUNT`'s flags: in place of what's at old |
| `MBEFORE` | 1 | `BIND`'s and `MOUNT`'s flags: a union, new first |
| `MAFTER` | 2 | `BIND`'s and `MOUNT`'s flags: a union, new last |
| `MCREATE` | 4 | `BIND`'s and `MOUNT`'s flags: a `CREATE` in the union comes to new |
| `EXIT_MSG_MAX` | 31 | An exit message's length at most (`WAIT`'s buffer: 32 bytes) |
| `KMESG_SIZE` | 4096 | `KMESG`: the kernel's messages held at most (then the oldest go) |
| `TR_ARGS` | 0 | `TASKREAD`: a task's arguments (`ARGS_MAX` bytes) |
| `TR_CWD` | 1 | `TASKREAD`: a task's current directory (`PATH_MAX` + 1 bytes) |
| `TR_ENV` | 2 | `TASKREAD`: a task's environment's first variables, whole, and a 0 (`ENV_SIZE` bytes at most) |
| `TR_FRAME` | 3 | `TASKREAD`: a task's state and frame (`TF_SIZE` bytes) |
| `TR_ENVAT` | 4 | `TASKREAD`: part of a task's environment (r1 bytes at most, from offset r2; A/X its bytes in use) |
| `TR_FD` | 5 | `TASKREAD`: a task's fd r2 (`FI_SIZE` bytes) |
| `FI_NAME` | 0 | `TR_FD`'s: the name its file was opened by (`PATH_MAX` + 1 bytes, zero-terminated) ... |
| `FI_MODE` | 64 | `TR_FD`'s: its open mode (`O_*`) ... |
| `FI_DEV` | 65 | `TR_FD`'s: its server's device letter ... |
| `FI_QTYPE` | 66 | `TR_FD`'s: its qid type (`QT_DIR` ...) ... |
| `FI_OFFSET` | 67 | `TR_FD`'s: and its offset (4 bytes, low first) |
| `FI_SIZE` | 71 |  |
| `TM_READ` | 0 | `TASKMEM`: from the task's memory |
| `TM_WRITE` | $01 | `TASKMEM`: to the task's memory |
| `TM_BANK` | $80 | `TASKMEM`'s flag: its RAM bank r3, at $8000-$9FFF |
| `TS_STEP` | 0 | `TASKSTEP`: one instruction |
| `TS_NEXT` | 1 | `TASKSTEP`: one instruction, a JSR's subroutine whole |
| `TS_BREAKS` | 2 | `TASKSTEP`: from now, the task's BRKs stop it (the debugger's breakpoints) |
| `TS_NOBREAKS` | 3 | `TASKSTEP`: its BRKs are its own again (`NOTE_BRK`) |
| `NI_FROM` | 0 | `NSINFO`'s answer (`NI_SIZE` bytes): the mount point (`PATH_MAX` + 1 bytes, zero-terminated) |
| `NI_PATH` | 64 | `NSINFO`: the path in the device (zero-terminated, no leading /) |
| `NI_DEV` | 128 | `NSINFO`: the device letter |
| `NI_SPEC` | 129 | `NSINFO`: the spec (8 bytes, zero-padded, and a 0) |
| `NI_FLAGS` | 138 | `NSINFO`: the flags (`MCREATE`) |
| `NI_SEQ` | 139 | `NSINFO`: its place among the union's members (lowest first) |
| `NI_SIZE` | 140 |  |
| `KEY_UP` | $80 | Keys: a raw console read gives the terminal's cursor and function keys as one code each |
| `KEY_DOWN` | $81 |  |
| `KEY_RIGHT` | $82 |  |
| `KEY_LEFT` | $83 |  |
| `KEY_HOME` | $84 |  |
| `KEY_END` | $85 |  |
| `KEY_INS` | $86 |  |
| `KEY_DEL` | $87 |  |
| `KEY_PGUP` | $88 |  |
| `KEY_PGDN` | $89 |  |
| `KEY_F1` | $8A | Keys: F1-F12 are $8A-$95 |
| `KEY_F2` | $8B |  |
| `KEY_F3` | $8C |  |
| `KEY_F4` | $8D |  |
| `KEY_F5` | $8E |  |
| `KEY_F6` | $8F |  |
| `KEY_F7` | $90 |  |
| `KEY_F8` | $91 |  |
| `KEY_F9` | $92 |  |
| `KEY_F10` | $93 |  |
| `KEY_F11` | $94 |  |
| `KEY_F12` | $95 |  |
| `KEY_FOCUS` | $97 | Keys: the focus moved to another window of this one's group; its number is the next key (a raw read's, keys hydra's) |
| `KEY_RESIZE` | $96 | Keys: the window's size changed (a raw read's, keys hydra's: consctl reads with the new one) |
| `KEY_MOD` | $98 | Keys: a key the terminal sent modified (keys mods's): the modifiers next (1 Shift, 2 Alt, 4 Ctrl), then the key |
| `O_READ` | 0 | Open modes |
| `O_WRITE` | 1 |  |
| `O_RDWR` | 2 |  |
| `O_RW_MASK` | 3 |  |
| `O_TRUNC` | $10 | Open modes: empty the file first |
| `O_NONBLOCK` | $40 | Open modes: `E_AGAIN` rather than wait |
| `FD_MAX` | 16 | Fds a task has (0 stdin, 1 stdout, 2 stderr) |
| `PATH_MAX` | 63 | A path's length at most |
| `IO_UNIT` | 512 | The most a `READ` or `WRITE` asks of a server in one request |
| `SR_NAME` | 0 | The stat record (`SR_SIZE` bytes): its name (32, zero-terminated) |
| `SR_QTYPE` | 32 | Stat: the qid's type (`QT_*`) |
| `SR_QVERS` | 33 | Stat: the qid's version |
| `SR_QPATH` | 34 | Stat: the qid's path (4: unique in its server) |
| `SR_MODE` | 38 | Stat: the mode (2: permissions; high byte `DM_DIR`) |
| `SR_LENGTH` | 40 | Stat: the length (4) |
| `SR_MTIME` | 44 | Stat: the time it changed (4: seconds since 2000) |
| `SR_DEV` | 48 | Stat: the device letter |
| `SR_INST` | 49 | Stat: the device's instance (its spec's first character) |
| `SR_SIZE` | 64 |  |
| `QT_DIR` | $80 | Qid types: a directory |
| `QT_FILE` | $00 |  |
| `DM_DIR` | $80 | A mode's high byte: a directory |
| `DM_APPEND` | $40 | A mode's high byte: append-only (writes go to the end) |
| `RQ_TYPE` | 0 | The request block (`RQ_SIZE` bytes): the request (`R_*`) |
| `RQ_FID` | 1 | Request: the fid (out: the new fid, `R_OPEN` and `R_CREATE`) |
| `RQ_MODE` | 2 | Request: the open mode |
| `RQ_CLIENT` | 3 | Request: the client task |
| `RQ_U` | 4 | Request: the client's U |
| `RQ_FLAGS` | 5 | Request: `RF_*` |
| `RQ_OFFSET` | 6 | Request: the offset (4) |
| `RQ_COUNT` | 10 | Request: the count asked for (2) |
| `RQ_DONE` | 12 | Request: the count done (2, out) |
| `RQ_BUF` | 14 | Request: the buffer, in the client's view (2) |
| `RQ_PERM` | 16 | Request: `R_CREATE`'s new mode (high byte); out (`R_OPEN`, `R_CREATE`): the qid type |
| `RQ_NAMELEN` | 17 | Request: the name's length |
| `RQ_GROUP` | 18 | Request: the client's note group |
| `RQ_DEV` | 19 | Request: the device letter |
| `RQ_SPEC` | 20 | Request: the mount's spec (8, zero-padded) |
| `RQ_EVENT` | 28 | Request: the server's event count as it took it (after `E_AGAIN`, the client waits for a change) |
| `RQ_SIZE` | 32 |  |
| `RF_NONBLOCK` | $01 | Request flags: answer `E_AGAIN` rather than wait |
| `R_OPEN` | 1 | Requests |
| `R_CREATE` | 2 |  |
| `R_READ` | 3 |  |
| `R_WRITE` | 4 |  |
| `R_CLUNK` | 5 |  |
| `R_STAT` | 6 |  |
| `R_WSTAT` | 7 |  |
| `R_REMOVE` | 8 |  |
| `R_FLUSH` | 9 |  |
| `R_DUP` | 10 |  |
| `SND_CHANNELS` | 24 | Sound (`#a`): its channels: 0-7 the YM2151's, 8-23 the Vera X's PSG (its voices 0-15) |
| `SND_PSG` | 8 | Sound: the PSG's first channel (its voice 0) |
| `SND_PATCHES` | 163 | Sound: its patches (0-127: General MIDI's programs; 128-162: drums and percussion) |
| `SND_DRUMS` | 128 | Sound: General MIDI's drums (MIDI channel 10's notes) |
| `SND_R_CH` | $02 | Sound: a command, at a register the chip doesn't have: the channel the next commands are for (0-23) |
| `SND_R_PATCH` | $03 | Sound: load patch n into the channel (its speakers stay) |
| `SND_R_NOTE` | $04 | Sound: key on MIDI note n (60: middle C; 69: A, 440 Hz), with the channel's bend |
| `SND_R_OFF` | $05 | Sound: key off (the release) |
| `SND_R_VOL` | $06 | Sound: the channel's volume (0-127, as General MIDI's) |
| `SND_R_PAN` | $07 | Sound: the channel's speakers (`SND_PAN_*`) |
| `SND_R_BEND` | $09 | Sound: the channel's bend (signed: 64ths of a semitone; the note playing too) |
| `SND_R_DRUM` | $0A | Sound: a General MIDI drum (its patch and pitch), keyed on |
| `SND_R_FREQ_LO` | $0B | Sound: a frequency's low byte (Hz), for `SND_R_FREQ` (only kept) |
| `SND_R_FREQ` | $0C | Sound: key on at a frequency, `SND_R_FREQ_LO`'s byte and this its high one (Hz; 0: key off) |
| `SND_R_GLIDE` | $0D | Sound: the channel's pitch to note n without a new attack (legato) |
| `SND_R_WAVE` | $0E | Sound: a PSG channel's waveform: `SND_WAVE_*` << 6 \| its pulse width (0-63: 63 a square) |
| `SND_PAN_LEFT` | 1 | Sound: `SND_R_PAN`'s speakers |
| `SND_PAN_RIGHT` | 2 |  |
| `SND_PAN_BOTH` | 3 |  |
| `SND_WAVE_PULSE` | 0 | Sound: `SND_R_WAVE`'s waveforms (the PSG's) |
| `SND_WAVE_SAW` | 1 |  |
| `SND_WAVE_TRIANGLE` | 2 |  |
| `SND_WAVE_NOISE` | 3 |  |
| `r0` | $02 | The call registers r0-r15: arguments and results |
| `r1` | $04 |  |
| `r2` | $06 |  |
| `r3` | $08 |  |
| `r4` | $0A |  |
| `r5` | $0C |  |
| `r6` | $0E |  |
| `r7` | $10 |  |
| `r8` | $12 |  |
| `r9` | $14 |  |
| `r10` | $16 |  |
| `r11` | $18 |  |
| `r12` | $1A |  |
| `r13` | $1C |  |
| `r14` | $1E |  |
| `r15` | $20 |  |

## Error codes

A call that fails returns with C = 1 and one of these in A. `ERRSTR` gives the message.

| Code | Name | Message |
|---|---|---|
| $01 | `E_PERM` | not allowed |
| $02 | `E_INVAL` | invalid argument |
| $03 | `E_NOSYS` | no such call |
| $04 | `E_AGAIN` | not yet: try again |
| $05 | `E_INTR` | interrupted |
| $06 | `E_NOMEM` | out of memory |
| $07 | `E_BUSY` | busy |
| $08 | `E_RANGE` | out of range |
| $09 | `E_FAULT` | bad address |
| $0A | `E_NAMETOOLONG` | name too long |
| $0B | `E_TOOBIG` | too big |
| $20 | `E_NOENT` | not found |
| $21 | `E_EXIST` | already exists |
| $22 | `E_NOTDIR` | not a directory |
| $23 | `E_ISDIR` | is a directory |
| $24 | `E_NOTEMPTY` | directory not empty |
| $25 | `E_BADF` | bad file descriptor |
| $26 | `E_MFILE` | no free file descriptor |
| $27 | `E_NFILE` | too many open files |
| $28 | `E_NOSPC` | disk full |
| $29 | `E_ROFS` | read-only |
| $2A | `E_IO` | i/o error |
| $2B | `E_NODEV` | no such device |
| $2C | `E_PIPE` | broken pipe |
| $2D | `E_NOEXEC` | not a program |
| $2E | `E_EOF` | end of file |
| $2F | `E_NOTFS` | no file system |
| $30 | `E_MEDIA` | medium error |
| $40 | `E_NOTASK` | no free task |
| $41 | `E_SRCH` | no such task |
| $42 | `E_CHILD` | no such child |
| $50 | `E_NSFULL` | namespace full |
| $51 | `E_NSLOOP` | too many binds |
