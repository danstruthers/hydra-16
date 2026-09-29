## **Programs: Hydra executables (`.hyx`)**

A Hydra executable is a program on a card that the shell loads and runs in a task of its own: `run hello.hyx`, or just `hello` ([HyForth](../using/hyforth.md#the-shell-directories-files-and-programs)).  It's machine code linked for a fixed address in task RAM, with a 16-byte header in front.  A sample is in `programs/`.

### **The file**

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 4 | Magic: `HYX1` |
| 4 | 2 | Load address |
| 6 | 2 | Length of the code (the bytes after the header) |
| 8 | 2 | Entry point |
| 10 | 2 | Flags: 0 |
| 12 | 4 | Reserved: 0 |

Then the code, which is loaded at the load address as it is: a program isn't relocatable.  It must fit in `$0800-$7BFF` (`ERR_IO_NOT_EXEC`, `$87`, otherwise).  The constants are in `os_rom/include/shell.inc` (`HYX_*`).

`run` tells a program from a script by the header, not by the name: a file without `HYX1` at its start is run as a HyForth script.  Name them `.hyx` anyway, so the shell finds them by name.

### **What a program gets**

* **A task of its own**, started with copies of the shell's fds (stdin, stdout and stderr: fds 0-2), namespace and current directory.  So a program can read and write files by relative names, and be a stage of a pipeline (`run hello.hyx | wc`).
* **Its RAM:** task RAM `$0800-$7BFF`.  Its code is loaded at its load address, and the MMU's page floor is set just above it, so the MMU's allocations come from the pages above its end.  For RAM beyond its end, a program allocates it (`MM_ALLOC`), or raises the floor (`MM_SET_FLOOR`) to use it directly ([memory.md](memory.md#the-mmu-a-tasks-own-memory)).
* **Zero page** `$A9-$FF` (the OS's is `$00-$A8` in this build: the `ZEROPAGE` segment in the link map), and the stack page: the loader has used a few bytes of the stack, and `$0100-$010F` held the header.
* **The OS calls** through the `$F8xx` thunks ([the API index](rom-layout.md#api-index-the-thunks)): the entry point is called on ROM page 0, where they are.
* **The console** while it runs, if the shell had it: its keys come to the program, and **Ctrl-C ends it** (a task without a break handler ends on a break; `TASK_SET_BREAK` sets one: [tasks.md](tasks.md#signals-break-and-kill)).  The shell's prompt comes back when it ends.

**Ending:** the program returns (`rts` from its entry point), or it's killed (Ctrl-C, Ctrl-\\, `kill`).  Either way its task ends: its fds are closed (stdout's buffered output goes out first), and its memory is freed.

### **Building one**

`programs/` has a sample, `hello.s`, and what it's built with:

| File | What |
| :--- | :--- |
| `hyx.inc` | `HYX_HEADER entry`: the header, in its own segment, with the load address and length filled in by the linker |
| `hyx.cfg` | The ld65 config: the header, then `CODE`, `RODATA` and `DATA` at `$0800`.  There's no BSS segment: everything is in the file (variables go in `DATA`) |
| `make.bat` | ca65, then ld65 with `hyx.cfg`: `bin\hello.hyx` |

```
.include "hyx.inc"
WRITE_CHAR      = $F803
            HYX_HEADER  start
.code
start:      lda         #'!'
            jmp         WRITE_CHAR              ; its rts ends the program
```

From another assembler, make a raw binary for a fixed address, then put the header on it with `sim/tools/mkhyx.js` (Node.js):

```
node sim/tools/mkhyx.js prog.bin prog.hyx $0800 [ENTRY]     the header on a binary (ENTRY: default the load address)
node sim/tools/mkhyx.js --info prog.hyx                      show a .hyx file's header
```

### **Putting it on a card**

* **In the emulator, or on a real card's image:** `node sim/tools/hydrafs.js mkdir card.img bin`, then `node sim/tools/hydrafs.js put card.img programs/bin/hello.hyx bin` ([the card tool](../tools/emulator.md#hydrafs-card-images)).
* **On the Hydra:** copy it from one card to another with `cp`.

A program in the current directory, or in `/bin` on the current directory's card, runs by its name: `hello`.

### **How `run` does it** (`os_rom/shell/run.s`, BIOS ROM page 7)

1. The shell opens the file, and reads its start: an `HYX1` header is checked (it has to fit in task RAM); anything else is a script.
2. The file goes on fd 11 (`SH_RUN_FD`), back at its start, and the shell starts a task (`TASK_RUN`) at the loader, `SH_LOAD`, which inherits it.
3. The shell waits: it hands the console to the new task if it has it (`CONS_SET_FG`), makes itself the task's parent, and pauses, as `TASK_START` does.  `TASK_EXIT` wakes it, and `CONS_RELEASE` gives the console back.
4. The loader, in the new task, reads the header and the code into place, sets the MMU's page floor above it, closes fd 11, and calls the entry point on ROM page 0.  When that returns, the loader returns, and the task ends.

A script (`.hys`) is run by HyForth itself: a copy of the shell's task (`TASK_CLONE`) reads it from fd 11 as `include` would, and ends at its end; the shell waits for it the same way.
