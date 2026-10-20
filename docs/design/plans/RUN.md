## **Programs in a bank: RUN**

A program the shell runs needs a task of its own today: `SPAWN` makes one, reads the program into its RAM, and the
shell `WAIT`s for it.  Dingus Quest Zero found a cheaper way for its own pieces (Phase 3B, its decision G1, the
user's design): a *bank program*, linked as a relocatable o65 module, loaded into a RAM bank of the caller's own at
`$8000` and called there like a subroutine, kept loaded till its bank is wanted.  That was a library in the C SDK
(`bankprog.s`).  This plan (October 2026, at the user's asking) makes it the kernel's, and uses it for the shell's
commands: a command runs in the shell's own task, from a bank where it stays for the next time, and several share a
bank.

### **What there was**

* **dq0's library**, `sdk/c/lib/bankprog.s` (1,080 lines of assembly, in `hydra.lib`): a child's o65 file read into
  a bank the task allocates, its BSS cleared, its imports found by name in the caller's exports (`NAME.exp`, the
  build's), its zero page put in bytes the caller gives, its relocations applied; six children at most, one to a
  bank, the one called least lately given up for a new one.  A call (`bank_call`) maps the bank, `jsr`s the entry on
  the caller's C stack and maps the caller's bank back: some 60 cycles more than a `jsr`.  `dq0d` (popgen) and
  `themegen` use it.  It can serve only C callers, and it knows nothing of exits, notes or fds.
* **The shell's commands.**  rc starts every program the same way (`exec.s`: `spawn_r`, `SPAWN` with an fd map, then
  `WAIT`); HyForth, the login shell, hands every rc line to a new `rc -c`, whose last command `EXEC`s the program in
  rc's task.  So each command costs a task's set-up and the program's whole load from its file, every time.
* **The tools.**  37 of the 44 programs in `/rom/bin` are assembly, 2,200 to 2,900 bytes each (`toollib`, linked into
  every one, is some 2,000 of them; its BSS 700 more).  The C ones (calc, db, dis, edit, grep, sort) are 13K to 26K.
  The ROM disk is nearly full: 24 of its 1,408 blocks free.

### **The kernel's mechanism**

**Bank programs.**  A bank program's file is a HYX2 header with `HF_BANK` set, then an o65 module (ld65's, linked at
`$8000`: its text, data and BSS, its zero page from 0, its relocations, its imports and exports).  The build makes
the eligible tools this way *instead of* as RAM programs, so the ROM disk doesn't grow.  A child (a dq0 piece) is the
same o65 module with no HYX2 header.

**The task's code cache.**  The kernel keeps a task's bank programs in RAM banks of the task's own (BANKS_ALLOC's,
eight at most to start with, freed as the task ends or `EXEC`s), several to a bank, one above the other.  Each bank's
last 256 bytes are its directory: an entry for each module (where it is, its file's path and identity, its load's
number, when it was last used, whether it's running).  A module is placed in the first gap it fits; with none, a new
bank; with no bank to have, the module used least lately is given up (its bytes are free again), and so on till it
fits.  A module is looked for by its path and its file's identity (`STAT`'s qid, version, length and time), so a
changed file, or a different file under the same name after a `bind`, is loaded again.

**Loading.**  The kernel reads the module into its place, clears its BSS, applies its relocations (its text, data
and BSS by where it was put; its zero page by where that went; an import by what it names), and keeps its exports'
table after its BSS (a child's entries, by name).  An import is found in the exports file the caller names (dq0's
`NAME.exp`).

**`RUN`** (a new call in the task group): run a bank program in this task, as a subroutine, and wait for it.  The
program gets what `SPAWN` would give it (its arguments at `r0`; its fds 0, 1 and 2, or the caller's map's), and runs
on the caller's stack, its code, data and BSS in its bank, its zero page in the program's ($22 up: the caller's bytes
there kept, and put back).  While it runs, Ctrl-C ends it and `EXITS` ends it; the task keeps its name (a window's
header, `ps`, show the shell's).  When it ends (returning from `main`, or `EXITS`, or a note's default) the kernel
puts back what was the caller's: its fds (the program's closed), its note handler, its zero page, its banks at
`$8000` and `$A000`, its break (the program may use the caller's RAM past it, if the caller says it may:
`RUN_LEND`); and `RUN` returns the program's exit code and message.  What a program changes that's the task's and
not in that list (the environment, the namespace, the directory, banks or pages it allocates) stays changed: such a
program is `HF_TASK`, run only in a task of its own.  A note that ended it (Ctrl-C) is posted again, so the caller's own handler sees
it as it would have.  A program that RUNs another works the same way (each run's frame is in its own bank).

**`SPAWN_RUN`**, a new flag of `SPAWN`'s: if the program is a bank program that may run in place, `SPAWN` runs it at
once (`RUN`) and returns the caller's own task, whose `WAIT` gives the program's status; else it's an ordinary
`SPAWN`.  So a shell that starts a command and waits for it needs only the flag.

**A bank program in a task of its own** (a pipeline's stage, a command run with `&`, `rc -c`'s last command `EXEC`ed,
a program marked `HF_TASK`): the new task loads it into a bank of its own and runs it there, as a RAM program runs.

**Children** (`BANK_LOAD`, `BANK_FLUSH`, in the memory group): a child loaded into the cache with its imports from an
exports file and its zero page where the caller says; the caller finds its entries in its exports' table and calls
them itself (the bank mapped, a `jsr`, the bank put back), as `bankprog.h`'s `bank_call` does now.  `bankprog.h`
keeps its functions; `bankprog.s` becomes a small layer over the calls, and the loader, the cache and the given-up
children are the kernel's.

### **The tools**

| | Programs |
| :-- | :-- |
| In place (`RUN`), a bank program | cat, cmp, cp, date, df, du, echo, free, fsck, head, hello, hwtest, kill, label, ls, mkdir, mkfs, mods, more, mv, ns, ps, pwd, rm, rmdir, slay, sleep, tee, top, touch, uniq, wc, xd, xmodem |
| A bank program, but in a task of its own (`HF_TASK`) | as (its library borrows the task's RAM), newwin (it changes the environment and starts a shell), tail (its window on the file, 16K, is more than a shell can lend) |
| Still RAM programs | ed, edit and dis (they map banks at `$8000` themselves); calc, db, grep, sort (13K and more) |

`toollib` is most of each tool: two fit in a bank as they are.  Shared, one copy in a bank for the tools there
(a library module, its exports their imports), ten or so would.

### **Steps**

| Step | What | Status |
| :--- | :--- | :----- |
| 1 | The kernel: bank programs' loader, the code cache, `RUN`, `EXITS` and notes in a run, `SPAWN_RUN`, a bank program's own task; the base's tests | Done (`run.s`: page 5, its calls' entries on page 0) |
| 2 | The build: the tools as bank programs (`HF_BANK`, `HF_TASK`), the RAM programs left as they are | Done (a `bank` file in a program's folder: `run` or `task`) |
| 3 | rc: `SPAWN_RUN` for a simple command it waits for | Done |
| 4 | The other shells: HyForth's rc lines that are a simple command run in its own task; hylang's and BASIC's | HyForth's done; hylang's and BASIC's not yet |
| 5 | Children: `BANK_LOAD`, `BANK_FLUSH`; `bankprog.s` over them, its tests | Done (`bankprog.h` as it was, but `bank_limit`; `t_bank`'s children two big ones, to fill a bank) |
| 6 | Libraries: `toollib` shared in a bank | Not yet |
| 7 | The books: the calls, the tools, the shells | Done (the kernel, the calls, rc, HyForth, the tools, the programmer's guide) |

### **As built**

Measured on the emulator at HyForth's prompt (a line's Enter to the next prompt, before and after):

| Line | Before | After |
| :-- | --: | --: |
| `echo hi`, the first time | 168 ms | 105 ms |
| `echo hi` again | 133 ms | 58 ms |
| `ls /rom`, the first time | 268 ms | 286 ms |
| `ls /rom` again | 254 ms | 141 ms |
| `pwd` | 147 ms | 113 ms |
| `echo a \| wc` (a pipeline: each stage a task), again | 282 ms | 286 ms |
| `ls /rom \| wc`, again | 369 ms | 376 ms |

* **A command run again** costs no load and no task: the shell's own task runs it from its cache.  The first time a
  program goes above another in a bank it's relocated there, which can cost more than the old load (`ls` the first
  time); at a bank's start it isn't.
* **A pipeline's stages** are tasks as before.  A bank program's own task reads its o65 header, text and data in one
  read (the build puts their length in its file's HYX2 header, `HX_LENGTH`), so a stage costs one read more than a
  RAM program's only if that fails; the 1-2% left is the task's cache bank (`BANKS_ALLOC`) and placing it.
* **The ROM disk's name cache** (64 lookups, the storage driver's) held "no `bin/rc`" while every HyForth line started
  rc; with plain lines run in HyForth's task it ages out, and the next line that wants rc pays one scan of
  `/rom/bin`'s directory first (about 30 ms).
* **A child's call** (`bank_call`) costs some 160 cycles more than a `jsr`, from 60 in dq0's library: the kernel's
  cache can give a child up for another task's program, so each call checks its entry is still the same load, and
  stamps it used for the cache's choice of what to give up.
* **ld65's options** (its name and the time) are left out of a bank program's file, and a child's date option made
  fixed, so the build's images are the same each time.
* **Children too big for the first layout.**  A trial merge into `reborn-game` (October 2026) found popgen, histgen
  and linegen (images of 7,401, 6,808 and 6,755 bytes) fitting a bank's 7,680 but not with their import tables (436 to
  1,066 bytes) after them.  The kernel now borrows a **loader bank** for a child's load: its imports' names and
  addresses, 64 hash chains to find each export's imports (not every export against every import), and the child's
  and the exports' files read 2K at a time (64 bytes and 176 before); only the child's exports' table stays past its
  BSS.  dq0's tests all pass on the trial (`dq0layers` counts its loads at the kernel's `K_OPEN` now, a `STAT`'s open
  not counted).
* **HyForth's environment.**  A program HyForth runs in its own task sees HyForth's environment, so HyForth's shell
  puts rc's `task`, `path` and `prompt` in it as it starts (rc's defaults, if they aren't set), and `prog` before
  each run, as rc does: `cat /env/path` sees the same whether HyForth or rc runs it, and a bank program or a RAM
  program.
