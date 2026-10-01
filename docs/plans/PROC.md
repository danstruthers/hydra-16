## **/proc: tasks as files, their memory included**

A plan to move `/dev/proc` to `/proc`, as Plan 9 has it, and to add a task's **memory as files**:
`/proc/N/mem` (its address space as it sees it) and `/proc/N/ram` (its banks on the RAM modules).  Then a
debugger, a memory dump or a core file is just a program reading files.  Who may read and write them is the same
rule as the RAM disks' areas ([DISKS.md](DISKS.md#who-can-use-which-area)), and **task 0, the system task, may use
everything**.  Nothing here is built yet.

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

`/dev/proc` ([io.md](../programming/io.md#devproc)) lists the busy tasks, and for each task N has `status`, `ctl`
(`kill`, `break`, `fg`), `cwd`, `env`, and `mem`, which is a **summary** (`pages PP floor FF`), not the memory.
It's served in its client's task, from BIOS ROM page 9 (`servers/proc_srv.s`).  Any task can read any of it, and
any task can `kill` any other.  A task's RAM can't be reached from another task at all: far pointers refuse it
(`FP_READ` on another task's RAM: the MMU test's step 9).

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

**Raw memory (an option): `/dev/mem`.**  Neither file above reads memory as the hardware has it: every RAM module's
banks (whichever task's they are), the shared RAM by bank ID, the paged ROM by chip, the BIOS ROM by page.  A
`/dev/mem` could, at offsets laid out as `docs/hardware.md`'s memory map, for a memory tester or a whole-machine
dump.  Who may use it is a choice still to make: task 0 only (the system's own), or any task, as WOZMON already
reads any address it's given.  Its writes (RAM only) would be task 0's, either way.

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

The proc server runs in its client's task (page 9), as now.  Reading task N's memory has to happen with `T` = N,
since every task's `$0000-$7FFF` and its banks are only seen while `T` is that task:
* **`TASK_CALL`** runs a copy routine in task N's context (its zero page, its banks), as `mem`'s summary is
  counted now.  The routine copies up to 256 bytes between task N's address and the client's IO transfer area,
  mapping the transfer area at `$8000` only while it touches it, and putting task N's own `$00`, `$01` and `U`
  back after each byte when the address is in task N's bank window (`$8000-$9FFF`), since both use that window.
  It runs from BIOS ROM (page 9 or 5), so its own code is never in the window.
* **ROM areas** (`$A000-$FEFF`): the copy selects task N's ROM bank or reads its BIOS page (`PEEK_PAGE`), as far
  pointers do.
* **IRQs stay on** for the copy: a byte at a time with the banks switched in the task's own registers is what
  far pointers already do, and the `irqs-off` test's limit holds.
* **`/proc/N/ram`** is the same routine with task N's `$00` set to bank `b` for the copy.
* **A free task** gives `ERR_IO_NOT_FOUND`; a driver task's memory, only task 0.

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
2. `/proc` as a mount, `/dev/proc` a bind to it; `pages` for the old `mem`; `ctl` checked by `TASK_MAY`.
3. `/proc/N/mem` and `/proc/N/ram`: the copy routine through `TASK_CALL`, reads then writes.
4. `regs`, `fd`, `ns`.
5. The debugger's `ctl` commands (`stop`, `start`, `step`, `break ADDR`) with the debugger itself.
6. Docs: `io.md` (`/proc`), `tasks.md` (owners, task 0), `hyforth.md` (`ps`).
