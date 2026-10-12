# Appendix K. System Calls

A **system call** asks HydraOS to do something for a program. This appendix lists all of them, group by group.

**From assembly language** (section 25.13), a call is a `jsr` to its address. Its inputs go in A, X, Y and the call
registers r0 to r15; its answers come back in the same places, a 16-bit one in A (its low byte) and X (its high
byte). C = 0 means it worked; C = 1 means it failed, with the error code (Appendix G) in A. A call may change A, X,
Y, r0 to r15 and the flags, but never the program's zero page, $22 to $7F. `hydra.inc` (in `/lib/as`) names every
call, constant and error code used here.

**From the languages**, the calls a program makes (all but those marked *Not in the languages*) have the call's name
in small letters, with `-` for `_`:

| Language | `OPEN` is | Section |
|---|---|---|
| HyForth (`lib hydra`) | `sys-open` ( *inputs* -- *outputs* *ior* ) | 19.9 |
| hylang | `(sys-open ...)`, or `(sys :open ...)` | 21.4 |
| HyBASIC | `SYS "OPEN"`, with `RREG` for the registers after | 23.8 |

*It can wait* means the call can make the program wait: for a key, a disk, another task, or time to pass.

<!-- generated: tools/calls.js -->

## K.1 The system

**`SYSINFO`** ($F800). What the Hydra has: the version of the system calls, the RAM modules, and the free tasks. *Out:* A = the ABI version (1); X = how many RAM modules are installed; r0 = the free tasks (bit = task).

**`ERRSTR`** ($F803). An error code's message, as text (Appendix G). *In:* A = an error code; r0 = a buffer (32 bytes). *Out:* the error's text in the buffer, zero-terminated ("error $xx" for an unknown code).

**`KMESG`** ($F806). The kernel's messages: what it printed as the Hydra started (also `/dev/kmesg`). *In:* r0 = a buffer; r1 = its size, the most it copies; r2 = an offset in the messages held, 0 the oldest. *Out:* A/X = the bytes held, KMESG_SIZE at most; the buffer has those from r2, r1 at most.

**`REBOOT`** ($F809). Start the Hydra again, as the RESET button does; or start the hardware test. *In:* A = 0, or REBOOT_HWTEST. *Out:* (doesn't return).

**`XCALL`** ($F80C). Call a routine in another bank of the paged ROM: a library's. Every register goes to it and comes back. *In:* r15 = a routine's address ($A000-$DFFF); r14 (its low byte) = the paged ROM bank it's in; A, X, Y, the flags and r0-r13: the routine's. *Out:* A, X, Y, the flags and r0-r15 as the routine leaves them. *(Not in the languages.)*


## K.2 Tasks

**`EXITS`** ($F830). End this program, with an exit code and a message (its status). Returning from `main` is `EXITS` with code 0. *In:* A = the exit code (0: success); r0 = a message (zero-terminated, 31 characters at most), or 0 for none. *Out:* (doesn't return).

**`WAIT`** ($F833). Wait for a program this one started to end, and get its exit code and message. *In:* A = a task this one started, or $FF for any; r0 = a buffer for the message (32 bytes), or 0. *Out:* A = the task that ended; X = its exit code; its message in r0's buffer ("" for none). *Errors:* E_CHILD, E_INTR. *It can wait.*

**`SPAWN`** ($F836). Start a program, in a task of its own. *In:* r0 = the program's path (through the namespace: /bin/NAME; or #m/NAME, a module in the paged ROM); r1 = its arguments (zero-terminated strings, an empty one after the last: 176 bytes at most), or 0; A = flags (SPAWN_NEWGROUP, SPAWN_NEWNS, SPAWN_FDMAP, SPAWN_NOENV, SPAWN_STOPPED); r2 = an fd map (with SPAWN_FDMAP). *Out:* A = the new task. *Errors:* E_NOENT, E_NOEXEC, E_NOTASK, E_TOOBIG, E_INVAL, E_MFILE, E_INTR. *It can wait.*

**`GETPID`** ($F839). This task's number (0 to 15). *Out:* A = this task's number.

**`YIELD`** ($F83C). Let the other tasks have a turn.

**`SLEEP`** ($F83F). Wait for a number of ticks (200 a second). *In:* A/X = ticks (TICK_HZ a second; 0-32767). *Errors:* E_INTR. *It can wait.*

**`SLEEP_UNTIL`** ($F842). Wait until the tick count reaches a number: for keeping steady time. *In:* A/X = a tick count (at most 32767 ticks ahead). *Errors:* E_INTR. *It can wait.*

**`TICKS`** ($F845). The tick count: 200 a second since the Hydra started. *Out:* A/X = the tick count (TICK_HZ a second; it wraps after about 5.5 minutes).

**`PREEMPT_OFF`** ($F848). Keep the processor: no other task runs until `PREEMPT_ON`. Use it only for a moment.

**`PREEMPT_ON`** ($F84B). Let the other tasks run again, after `PREEMPT_OFF`.

**`TASKINFO`** ($F84E). What the kernel knows about a task: its state, parent, time used, name (as `ps` shows). *In:* A = a task (0-15); r0 = a buffer (TI_SIZE bytes). *Out:* the buffer: TI_STATE, TI_FLAGS, TI_PARENT, TI_CPU (3 bytes: ticks), TI_BANK, TI_TYPE, TI_NAME (16). *Errors:* E_SRCH.

**`NOTIFY`** ($F851). Set this task's note handler (section 25.16), or take it away. *In:* r0 = the note handler, or 0 for none. *(Not in the languages.)*

**`NOTE`** ($F854). Send a note to a task, or to a note group. *In:* A = a task, or NOTE_GROUP | a note group; X = the note (1-31: NOTE_*). *Errors:* E_INVAL, E_SRCH, E_PERM.

**`MODINFO`** ($F857). An entry of the paged ROM's module directory: a module's bank, type and name. *In:* A = an entry of the module directory (0 on); r0 = a buffer (ME_SIZE bytes). *Out:* the buffer: ME_BANK, ME_BANKS, ME_TYPE (HT_*), ME_FLAGS (HF_*), ME_NAME (12, zero-padded). *Errors:* E_NOENT.

**`ENV_GET`** ($F85A). Read an environment variable, of this task or another. *In:* A = a task ($FF: this one); r0 = a variable's name; r1 = a buffer; r2 = its size; r3 = an offset in the value. *Out:* A/X = the value's length from the offset (0 at its end or past it); as much of it as fits, in the buffer. *Errors:* E_NOENT, E_SRCH, E_INVAL, E_NAMETOOLONG.

**`ENV_PUT`** ($F85D). Set an environment variable (or part of its value). *In:* A = a task ($FF: this one); r0 = a variable's name; r1 = bytes; r2 = their count; r3 = where in its value they go (the value cut there first: 0 for a new value; its length at most). *Errors:* E_NOMEM, E_INVAL, E_SRCH, E_NOENT, E_NAMETOOLONG.

**`ENV_DEL`** ($F860). Remove an environment variable. *In:* A = a task ($FF: this one); r0 = a variable's name. *Errors:* E_NOENT, E_SRCH, E_INVAL, E_NAMETOOLONG.

**`ENV_NAME`** ($F863). The name of an environment's variable number X: for listing them all. *In:* A = a task ($FF: this one); X = which variable (0 on); r0 = a buffer (ENV_NAME_MAX + 1 bytes). *Out:* its name in the buffer, zero-terminated; A/X = its value's length. *Errors:* E_NOENT, E_SRCH.

**`TASKREAD`** ($F866). Read another task's arguments, current directory, environment, registers or open files (as `/proc` does). *In:* A = a task ($FF: this one); X = what: TR_ARGS (its arguments: ARGS_MAX bytes), TR_CWD (its current directory: PATH_MAX + 1 bytes), TR_ENV (its environment's first variables, whole, and a 0: ENV_SIZE bytes at most), TR_FRAME (its state and the frame it left as it was switched out: TF_SIZE bytes, TF_*) or TR_ENVAT (part of its environment), or TR_FD (its fd r2: FI_SIZE bytes, FI_*); r0 = a buffer that big; r1 = TR_ENVAT's count (the most it copies); r2 = TR_ENVAT's offset, or TR_FD's fd. *Out:* the bytes in the buffer; A/X = TR_ENVAT's: the environment's bytes in use (its 0 too: it copied as many as there are from r2, r1 at most). *Errors:* E_SRCH, E_INVAL, E_BADF.

**`GETPPID`** ($F869). The task that started this one. *Out:* A = this task's parent ($FF: none).

**`SEM_NEW`** ($F86C). Make a **semaphore**: a counter that tasks take and give back, to share something without getting in each other's way. *In:* A = its count: how many can take it before a taker waits (0-255); X = 0, or SEM_MUTEX: a mutex (its count 1, and it has a holder: only the task that took it gives it back). *Out:* A = the semaphore (0 to SEM_MAX - 1). *Errors:* E_NOMEM.

**`SEM_ACQUIRE`** ($F86F). Take one from a semaphore; if it has none, wait until another task gives one back. *In:* A = a semaphore. *Errors:* E_INVAL, E_BUSY, E_INTR. *It can wait.*

**`SEM_TRY`** ($F872). Take one from a semaphore, but don't wait if it has none. *In:* A = a semaphore. *Errors:* E_INVAL, E_BUSY, E_AGAIN.

**`SEM_RELEASE`** ($F875). Give one back to a semaphore, waking a task that's waiting for it. *In:* A = a semaphore. *Errors:* E_INVAL, E_PERM, E_RANGE.

**`SEM_FREE`** ($F878). Free a semaphore. *In:* A = a semaphore. *Errors:* E_INVAL.

**`EXEC`** ($F87B). Run another program in this task, in this one's place. *In:* r0 = the program's path (as SPAWN's); r1 = its arguments (as SPAWN's: 176 bytes at most), or 0. *Out:* (doesn't return, once the program is found: it runs in this task's place). *Errors:* E_NOENT, E_NOEXEC, E_TOOBIG, E_PERM, E_INTR. *It can wait.*


## K.3 Memory

**`BREAK`** ($F890). Move the end of the program's memory (its **break**), or ask where it is. *In:* r0 = the new end of the program's data (the first byte past it), or 0 to ask. *Out:* r0 = the end. *Errors:* E_INVAL, E_NOMEM.

**`PAGES_ALLOC`** ($F893). Take some pages (256 bytes each) of the task's memory, above the break. *In:* A = pages (256 bytes each). *Out:* r0 = the first one's address; A = its page. *Errors:* E_INVAL, E_NOMEM.

**`PAGES_FREE`** ($F896). Give pages back. *In:* A = pages; r0 = the first one's address. *Errors:* E_INVAL.

**`BANKS`** ($F899). How many RAM banks the task has, and on which modules. *Out:* A = the banks the task has (16 for each good RAM module); r0 = the good modules (bit = module).

**`BANKS_ALLOC`** ($F89C). Take some of the task's RAM banks: A gives the first bank's number, to write to $00. *In:* A = banks. *Out:* A = the first (for $00). *Errors:* E_INVAL, E_NOMEM.

**`BANKS_FREE`** ($F89F). Give banks back. *In:* A = the first bank; X = how many. *Errors:* E_INVAL.

**`SEG_CREATE`** ($F8A2). Make a shared **segment**: shared RAM banks that other tasks can attach to. *In:* A = banks (1-128, 8K each). *Out:* A = the segment (0-15). *Errors:* E_INVAL, E_NOMEM.

**`SEG_ATTACH`** ($F8A5). Attach to a segment. *In:* A = a segment. *Errors:* E_INVAL.

**`SEG_DETACH`** ($F8A8). Detach from a segment (the last task to leave frees it). *In:* A = a segment. *Errors:* E_INVAL.

**`SEG_MAP`** ($F8AB). How to reach a bank of a segment: the values to write to U and to $00. *In:* A = a segment; X = one of its banks (0 on). *Out:* A = the value for U; X = the value for $00. *Errors:* E_INVAL, E_RANGE.

**`SEGINFO`** ($F8AE). The shared RAM: how much there is, and which segments are in use. *Out:* A = the shared banks segments can have (8K each: the good chips'); X = those segments have now; r0 = the segments in use (bit = segment).

**`BANKS_ALLOC_IN`** ($F8B1). As `BANKS_ALLOC`, from a range of bank numbers only. *In:* A = how many; X = the lowest bank it may take; Y = the highest (module m's are $m0-$mF). *Out:* A = the first. *Errors:* E_INVAL, E_NOMEM.

**`SEG_CREATE_IN`** ($F8B4). As `SEG_CREATE`, from a range of the shared RAM only. *In:* A = how many banks (1-128); X = the lowest shared bank ID it may take; Y = the highest (U << 4 | the bank's low nibble; segments have $80-$FF). *Out:* A = the segment. *Errors:* E_INVAL, E_NOMEM.


## K.4 Files

**`OPEN`** ($F8C0). Open a file, for reading or writing or both. *In:* r0 = a path; A = the mode (O_READ, O_WRITE, O_RDWR; | O_TRUNC, O_NONBLOCK). *Out:* A = an fd. *Errors:* E_NOENT, E_NODEV, E_NAMETOOLONG, E_MFILE, E_NFILE. *It can wait.*

**`CREATE`** ($F8C3). Make a new file, or a directory, and open it. *In:* r0 = a path; A = the mode; X = the new file's mode, high byte (DM_DIR: a directory). *Out:* A = an fd. *Errors:* E_NOENT, E_NODEV, E_NAMETOOLONG, E_MFILE, E_NFILE, E_EXIST. *It can wait.*

**`CLOSE`** ($F8C6). Close a file. *In:* A = an fd. *Errors:* E_BADF. *It can wait.*

**`READ`** ($F8C9). Read bytes from a file. *In:* A = an fd; r0 = a buffer; r1 = a count. *Out:* A/X = the count read (0: the end of the file). *Errors:* E_BADF, E_AGAIN, E_INTR. *It can wait.*

**`WRITE`** ($F8CC). Write bytes to a file. *In:* A = an fd; r0 = the data; r1 = its count. *Out:* A/X = the count written. *Errors:* E_BADF, E_AGAIN, E_INTR. *It can wait.*

**`SEEK`** ($F8CF). Move the place in a file where the next read or write happens. *In:* A = an fd; r0, r1 = an offset (low word, high word: signed); X = from: 0 the start, 1 the offset, 2 the end. *Out:* r0, r1 = the new offset. *Errors:* E_BADF, E_INVAL. *It can wait.*

**`STAT`** ($F8D2). A file's **stat record**: its name, type, length, time and more. *In:* r0 = a path; r1 = a buffer (SR_SIZE bytes). *Out:* the file's stat record in the buffer (SR_*). *Errors:* E_NOENT, E_NODEV, E_NAMETOOLONG, E_MFILE, E_NFILE. *It can wait.*

**`FSTAT`** ($F8D5). An open file's stat record. *In:* A = an fd; r0 = a buffer (SR_SIZE bytes). *Out:* the stat record in the buffer. *Errors:* E_BADF. *It can wait.*

**`WSTAT`** ($F8D8). Change a file: its name, its mode, or its length. *In:* r0 = a path; r1 = a stat record. *Errors:* E_NOENT, E_NODEV, E_NAMETOOLONG, E_MFILE, E_NFILE, E_NOSYS. *It can wait.*

**`FWSTAT`** ($F8DB). Change an open file, as `WSTAT` does. *In:* A = an fd; r0 = a stat record. *Errors:* E_BADF, E_NOSYS. *It can wait.*

**`REMOVE`** ($F8DE). Remove a file, or an empty directory. *In:* r0 = a path. *Errors:* E_NOENT, E_NODEV, E_NAMETOOLONG, E_NOTEMPTY, E_NOSYS. *It can wait.*

**`DUP`** ($F8E1). Another fd for an open file. *In:* A = an fd. *Out:* A = a new fd (the lowest free), on the same channel (the same offset). *Errors:* E_BADF, E_MFILE.

**`DUP2`** ($F8E4). Make fd X the same as fd A (closing what X was first): how the shell does `>` and `|`. *In:* A = an fd; X = another (0-15). *Errors:* E_BADF. *It can wait.*

**`CHDIR`** ($F8E7). Change the current directory. *In:* r0 = a directory's path. *Errors:* E_NOTDIR, E_NOENT, E_NODEV, E_NAMETOOLONG. *It can wait.*

**`GETCWD`** ($F8EA). The current directory. *In:* r0 = a buffer (PATH_MAX + 1 bytes). *Out:* A = the current directory's length; the directory in the buffer, zero-terminated.

**`PIPE`** ($F8ED). Make a **pipe**: two fds, and what's written to one is read from the other. *Out:* A = an fd to read from; X = an fd to write to. *Errors:* E_NODEV, E_MFILE, E_NFILE, E_NOMEM. *It can wait.*

**`FD2PATH`** ($F8F0). The name an open file was opened by. *In:* A = an fd; r0 = a buffer (PATH_MAX + 1 bytes). *Out:* the name in the buffer, zero-terminated. *Errors:* E_BADF.


## K.5 Namespaces

**`BIND`** ($F920). Bind a name to another place in the namespace (section 8.5). *In:* r0 = new (a path); r1 = old (a path: absolute); A = MREPL, MBEFORE or MAFTER, and MCREATE. *Errors:* E_NOENT, E_INVAL, E_NSFULL, E_NODEV, E_NAMETOOLONG. *It can wait.*

**`MOUNT`** ($F923). Mount a device's files in the namespace. *In:* X = a device letter; r0 = its spec (a string, 8 characters at most), or 0; r1 = old; A = BIND's flags. *Errors:* E_NODEV, E_NOENT, E_INVAL, E_NSFULL, E_NAMETOOLONG. *It can wait.*

**`UNMOUNT`** ($F926). Undo a bind or a mount. *In:* r0 = new, or 0; r1 = old. *Errors:* E_NOENT, E_INVAL, E_NSFULL, E_NAMETOOLONG. *It can wait.*

**`NSINFO`** ($F929). An entry of a task's namespace: what's bound or mounted where (as `ns` shows). *In:* A = a task ($FF: this one); X = which entry of its namespace (0 on); r0 = a buffer (NI_SIZE bytes). *Out:* the entry in the buffer (NI_*): its mount point, its device letter and spec, the path in that device, its flags (MCREATE) and its place among the union's members. *Errors:* E_RANGE, E_SRCH.


## K.6 The console

**`PUTC`** ($F950). Write a byte (a character) to the standard output. *In:* A = a byte. *It can wait.*

**`PUTS`** ($F953). Write a string to the standard output. *In:* r0 = a string, zero-terminated. *Errors:* E_INTR. *It can wait.*

**`GETC`** ($F956). Read a byte from the standard input, waiting for one. *Out:* A = a byte. *Errors:* E_EOF, E_INTR. *It can wait.*

**`PUTHEX`** ($F959). Write a byte to the standard output as two hex digits. *In:* A = a byte. *It can wait.*


## K.7 The clock

**`TIME`** ($F980). The date and time: seconds since the start of 2000. *Out:* r0/r1 = the clock: seconds since 2000-01-01 00:00:00.

**`TIME_SET`** ($F983). Set the date and time. *In:* r0/r1 = seconds since 2000-01-01 00:00:00.

**`RTC`** ($F986). Read or write the DS1747 clock chip's registers. *In:* A = 0, read, or 1, write; r0 = 8 bytes: the DS1747's registers (RTC_REGS, hw.inc), BCD. *Errors:* E_INVAL.


## K.8 Drivers

**`IRQ_OWN`** ($F9B0). A driver: take an interrupt line, so its interrupts run the driver's handler. *In:* A = an IRQ line (0-14), or LINE_VIA_T2 (VIA timer 2), or LINE_VIA_CA1 (VIA CA1). *Errors:* E_BUSY, E_RANGE, E_INVAL. *(Not in the languages.)*

**`IRQ_RELEASE`** ($F9B3). A driver: give an interrupt line back. *In:* A = an IRQ line. *Errors:* E_PERM, E_RANGE. *(Not in the languages.)*

**`WAKE`** ($F9B6). Wake a task that's waiting in `PAUSE`. *In:* A = a task. *(Not in the languages.)*

**`PAUSE`** ($F9B9). Wait until another task (or an interrupt handler) wakes this one. *It can wait.* *(Not in the languages.)*

**`NOTE_POST`** ($F9BC). A driver: send a note. *In:* A = a task, or NOTE_GROUP | a note group; X = the note. *Errors:* E_INVAL, E_SRCH, E_PERM. *(Not in the languages.)*

**`SRV_REGISTER`** ($F9BF). A driver: serve a device letter, so the requests for its files come to this task. *In:* A = a device letter (printable; not #). *Errors:* E_INVAL, E_EXIST, E_NOMEM. *(Not in the languages.)*

**`SRV_TAKE`** ($F9C2). A driver: take the request being served. *Out:* A = the request (R_*); Y = the client; the request block in TASK_INBOX, and (R_OPEN, R_CREATE, R_REMOVE) the name in TASK_PATH. *(Not in the languages.)*

**`SRV_REPLY`** ($F9C5). A driver: answer the request being served. *In:* TASK_INBOX's RQ_FID, RQ_DONE, RQ_PERM, RQ_EVENT: what the request returns; A, C: the answer. *Out:* A and C kept. *(Not in the languages.)*

**`CLIENT_READ`** ($F9C8). A driver: copy bytes from the program whose request it's serving. *In:* r0 = here; r1 = there, in the client's view; r2 = a count. *(Not in the languages.)*

**`CLIENT_WRITE`** ($F9CB). A driver: copy bytes to the program whose request it's serving. *In:* r0 = here (the data); r1 = there, in the client's view; r2 = a count. *(Not in the languages.)*

**`NOTE_QUEUE`** ($F9CE). An interrupt handler: send a note to a note group (as the console does for <kbd>CTRL</kbd>-<kbd>C</kbd>). *In:* IRQs off; X = a note group (0-15); A = the note's bit: 1 << (note - 1), for a note 1-8. *Out:* X kept. *(Not in the languages.)*

**`ROMREAD`** ($F9D1). Copy bytes from any bank of the paged ROM. *In:* A = a paged ROM bank; r0 = where in it ($A000-$DFFF); r1 = a buffer (not at $A000-$DFFF); r2 = a count. *Out:* the bytes in the buffer. *Errors:* E_PERM. *(Not in the languages.)*

**`TASKMEM`** ($F9D4). A driver: read or write another task's memory (as `/proc`'s `mem` does). *In:* A = a task; X = TM_READ or TM_WRITE, | TM_BANK; r0 = a buffer; r1 = an address in the task's view, $0000-$DFFF (with TM_BANK: $8000-$9FFF, in its RAM bank r3); r2 = a count (the range ends at $E000, or with TM_BANK at $A000, at most). *Errors:* E_PERM, E_SRCH, E_INVAL. *(Not in the languages.)*

**`TASKSTOP`** ($F9D7). A driver: stop a task where it is, or start it again. *In:* A = a task; X = 1 to stop it, 0 to start it again. *Errors:* E_PERM, E_SRCH. *(Not in the languages.)*

**`TASKSTEP`** ($F9DA). A driver: the debugger's step. *In:* A = a task; X = TS_STEP, TS_NEXT, TS_BREAKS or TS_NOBREAKS. *Errors:* E_PERM, E_SRCH, E_INVAL, E_BUSY. *(Not in the languages.)*


## K.9 Debugging HydraOS

**`DBG_SCALL`** ($FA10). For testing: call a driver directly. *In:* Y = a driver task; A, X = what its serve entry gets. *Out:* A, X and C from its serve entry. *Errors:* E_SRCH, E_NODEV. *It can wait.* *(Not in the languages.)*

**`DBG_KCOPY`** ($FA13). For testing: copy bytes between this task and another. *In:* A = another task; r0 = an address in this task; r1 = an address in that task; r2 = a byte count; C = 0 to copy from this task to that one, C = 1 the other way. *Errors:* E_SRCH. *(Not in the languages.)*

**`DBG_PS`** ($FA16). For testing: print a line for each task. *It can wait.* *(Not in the languages.)*

<!-- end generated: tools/calls.js -->
