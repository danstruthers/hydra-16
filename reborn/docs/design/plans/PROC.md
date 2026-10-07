## **/proc: tasks as files, their memory included**

A plan to move `/dev/proc` to `/proc`, as Plan 9 has it, and to add a task's **memory as files**:
`/proc/N/mem` (its address space as it sees it) and `/proc/N/ram` (its banks on the RAM modules).  Then a
debugger, a memory dump or a core file is just a program reading files.  Who may read and write them is the same
rule as the RAM disks' areas ([DISKS.md](DISKS.md#who-can-use-which-area)), and **task 0, the system task, may use
everything**.  Built so far: `/proc` mounted, `pages`, `ns`, `cmd` and `ctl` checked by `TASK_MAY` (step 2 below,
and `ns` from step 4), and the memory files, `mem` and `ram` (step 3); `regs` and `fd` are next.

### **Contents**
1. [Where it stands](#where-it-stands)
2. [What Plan 9 does](#what-plan-9-does)
3. [The files](#the-files)
4. [/proc/N/mem](#procnmem)
5. [/proc/N/ram](#procnram)
6. [Who can use it](#who-can-use-it)
7. [How it works](#how-it-works)
8. [What it gives](#what-it-gives)
9. [Tests](#tests)
10. [Steps](#steps)

---

### **Where it stands**

`/proc` ([io.md](../../../../old/docs/programming/io.md#the-tasks-proc)), the device `proc` mounted by the boot shell (`/dev/proc` is
the same files), lists the busy tasks, and for each task N has `status`, `ctl` (`kill`, `break`, `fg`), `cwd`, `env`,
`pages` (a **summary**: `pages PP floor FF`; it was `mem`), `ns` (its namespace, as `ns` prints it), `cmd` (a line
for its shell to run: HyForth's `send`), and its memory, `mem` and `ram`.  It's served in its client's task, from BIOS
ROM page 9 (`servers/proc_srv.s`).  Any task can read the status files; `ctl`, `cmd`, `mem` and `ram` are for the
task's family and task 0.  Far pointers still refuse another task's RAM (`FP_READ`: the MMU test's step 9): the
files are the one way to it, and they check who asks.

---

### **What Plan 9 does**

`/proc/N/` has `mem` (the process's memory, at the offsets of its addresses), `regs` (its registers), `ctl`
(`stop`, `start`, `kill`, ...), `status`, `ns` (its namespace, as `bind` and `mount` lines), `fd` (its open files),
`note` (signals).  The debugger (`acid`) and `ps` are ordinary programs reading those files, and a process's
owner (or the host owner, Plan 9's system user) may read and write its `mem`.

---

### **The files**

| Name | Read | Write | Who |
| :--- | :--- | :---- | :-- |
| `/proc` | A line per busy task (as now) | | Every task |
| `/proc/N`, `/proc/N/status` | Task N's line (as now) | | Every task |
| `/proc/N/cwd`, `/proc/N/env` | As now | | Every task |
| `/proc/N/ns` | Its namespace, as the lines that would make it (`ns`'s) | | Every task |
| `/proc/N/fd` | Its open fds: a line each, the fd, its mode and its name | | Every task |
| `/proc/N/pages` | `pages PP floor FF`: today's `mem` summary, renamed | | Every task |
| `/proc/N/ctl` | | `kill`, `break`, `fg`; later `stop`, `start`, `step`, `break ADDR` (the debugger) | Its family, task 0 |
| `/proc/N/regs` | Its saved registers: `A X Y S P PC` and `W`, its RAM and ROM banks | Later: the debugger sets them | Its family, task 0 |
| `/proc/N/mem` | Its address space, as it sees it: offset = address ([below](#procnmem)) | Its RAM | Its family, task 0 |
| `/proc/N/ram` | Its banks on the RAM modules: offset = bank * 8K + offset in the bank ([below](#procnram)) | The same | Its family, task 0 |

**The name:** `/proc` is a mount of the proc device in the default namespace (as `/env` will be:
[NAMESPACES.md](NAMESPACES.md)); `/dev/proc` stays as a bind to it until the programs and docs have moved.

---

### **/proc/N/mem**

The 64K that task N's code sees, with its registers as they are while it's switched out:

| Offsets | What | Write |
| :------ | :--- | :---- |
| `$0000-$7FFF` | Its task RAM: zero page, stack, its program and data | Yes |
| `$8000-$9FFF` | The RAM bank it has selected (its `$00`), or the shared bank it has mapped | Yes |
| `$A000-$DFFF` | The paged ROM bank it has selected (its `$01`) | `ERR_IO_MODE` |
| `$E000-$FEFF` | The BIOS ROM page it's on (`W`) | `ERR_IO_MODE` |
| `$FF00-$FFFF` | The I/O space: reads give zeros (a read can have effects: the ACIA's data, say) | `ERR_IO_MODE` |

* **A read or write is a snapshot.**  The task may run between two reads; a debugger stops it first (`stop` in
  `ctl`), or reads a running one knowing that.
* **Offsets past `$FFFF`** are the end of the file.  `IO_STAT` gives a size of 64K.
* `cp /proc/3/mem core3` saves a task's whole address space; `cp` of a 32K slice back is how a debugger loads a
  patch.  HyForth's `dump` and WOZMON's `R` keep working on the reading task's own memory; this is for another
  task's.

---

### **/proc/N/ram**

Task N's banks on the RAM modules, by the bank numbers it uses (each task numbers its own banks, `$00` up, and
sees them at `$8000` through its `$00`): offset `b * $2000 + o` is byte `o` of its bank `b`.

* **Only the banks it has:** the memory manager's bank map says which (`MM_ALLOC` with `AI_PAGED`).  A read of a
  bank it hasn't got gives `ERR_IO_NOT_FOUND`; a listing (`IO_STAT`) gives the size up to its last bank.
* So a task's whole state is `/proc/N/mem` and `/proc/N/ram`; shared RAM is reached by its handles, as now.

**Raw RAM: `/dev/ram` (built).**  Neither file above reads memory as the hardware has it; `/dev/ram` does: every
task's 32K, the shared RAM by bank ID, every RAM module's banks for every task, read-only, and **for task 0
only**, the system's task (any other task's open is `ERR_IO_PERM`).  Its offsets and how it reads are in
[io.md](../../../../old/docs/programming/io.md#the-ram-itself-devram).

---

### **Who can use it**

The status files (`status`, `cwd`, `env`, `ns`, `fd`, `pages`) are for every task, as now: `ps` and `top` read
them.  The rest (`ctl`, `regs`, `mem`, `ram`) need the client (task A, from the request's `IO_BLK_CLIENT`) to be:
* **task 0,** the system task, which passes every check, here and on the RAM disks ([DISKS.md](DISKS.md#who-can-use-which-area));
* **task N itself;**
* **a task that started N,** directly or not: a shell and the programs it runs, a debugger and the program it
  started.

So two shells can't read or kill each other's programs, and a program can't change its shell.  This also closes a
gap: today any task can `kill` any other.

**The check is the kernel's `TASK_MAY`** (built: page 5), shared with the HydraFS server: "may A use N's things", walking N's owner
chain (`ZP_TASK_OWNER`) up for `/proc` (is A an ancestor of N?) and A's for the RAM disks (is N an ancestor of A?),
task 0 passing both.  Two things it needs first:
* **Owners kept right when a task ends** (done: `TASK_ORPHANS`): its children's owner becomes its own owner, as Unix
  gives orphans to `init`, so a new task in its slot isn't taken for their parent.
* **The whole chain:** `TASK_SIGNAL` looks 4 owners up; `TASK_MAY` follows the chain to its end (at most 16 steps).
  For `/proc` it's called with `.Y` = 1: the asker started the task (or is it).

---

### **How it works**

*(Built, as below.  The plan was a copy routine run in task N through `TASK_CALL`, with IRQs on; but that
call writes task N's zero page and stack while it runs, so the copy wouldn't read them as N has them.)*

The proc server runs in its client's task (page 9), as before.  Reading task N's memory has to happen with
`T` = N, since every task's `$0000-$7FFF` and its banks are only seen while `T` is that task:
* **`MEM_COPY`** (`servers/ram_srv.s`, shared with `/dev/ram`) sets `T` to N with IRQs off and no stack use, and
  N's `$00` to the client's IO transfer bank, so the client's data area and the copy's numbers (`MC_AREA`, after
  the system namespace in each transfer bank) are seen.  It copies up to 64 bytes (about 2,000 cycles: the
  `irqs-off` test's limit is 5,000), then puts N's `$00` and the 8 zero page bytes it borrowed back; a read of
  them gives N's values, and a write to them lands in them.
* **N's bank window** (`$8000-$9FFF`): the copy switches N's `$00` to its bank and back for each byte (and `U`
  for a shared one), since the transfer bank uses the window too.
* **ROM areas:** `$A000-$DFFF` is read with `T` = N, so it's N's paged ROM bank; the BIOS page (`$E000-$FEFF`) is
  N's `W`, read with IRQs on through `PEEK_PAGE`.  `U` and `W` come from N's frame on its stack, where the
  scheduler left them; a task in a `TASK_CALL`, a driver or the asker itself has none there (`U` 0, page 0).
* **`/proc/N/ram`** is the same copy with N's `$00` set to bank `b`, if N's MMU bank map has it.
* **A free task** gives `ERR_IO_NOT_FOUND`; a driver task's memory, only task 0 (`TASK_MAY`).

---

### **What it gives**

* **The debugger** ([NEXT_STEPS.md](NEXT_STEPS.md)) as a program: `regs`, `mem` and `ctl`'s `stop`, `step` and
  `break ADDR`, in C or HyForth.
* **Core dumps:** the shell can save `/proc/N/mem` when a program dies (`ERR_` exit statuses), for later.
* **`ps`, `top`, `which`** read the status files (as now), and `fd` and `ns` show what a task has open and sees.
* **Tools on files:** `cmp`, `od`, a hex editor work on another task's memory, as on a card's file.
* **Fewer special calls:** reading another task's RAM needs no new kernel call for programs, only the files.

---

### **Tests**

In the emulator (`sim/tests/devices.js`): `/proc` as `/dev/proc` was (the existing tests, renamed); a shell
reading a background program's `mem` (a known value at a known address) and writing it (the program prints the
change); `$8000` through the program's bank; a ROM area read and a write refused; `ram` for a program with two
banks; another family's task refused (`ERR_IO_PERM`), `kill` included; task 0's request allowed; the owner
chain after a task ends and its slot is reused; the `irqs-off` limit while copying.

---

### **Steps**

1. *Done:* the owner chain kept right at task end; `TASK_MAY` (task 0 passes).
2. **(Done)** `/proc` as a mount (`/dev/proc` stays: the same device); `pages` for the old `mem`; `ctl` checked by
   `TASK_MAY`; and `cmd` (`send`), a line for another running shell (the `send` and `proc` tests).
3. **(Done)** `/proc/N/mem` and `/proc/N/ram`, reads and writes: `MEM_COPY` with `T` = N and IRQs off (the
   `proc-mem` test: another shell's RAM and bank read and written, the ROM and I/O areas, refusals, a 64K copy).
4. `regs`, `fd` (`ns`: done in step 2).
5. The debugger's `ctl` commands (`stop`, `start`, `step`, `break ADDR`) with the debugger itself.
6. Docs: `io.md` (`/proc`), `tasks.md` (owners, task 0), `hyforth.md` (`ps`).
