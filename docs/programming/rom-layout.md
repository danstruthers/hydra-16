## **ROM layout, calling conventions and the API index**

How the OS ROM is organised, how code calls between ROM pages, and the fixed entry points (thunks) programs use.  Part of the [Programmer's Guide](README.md).

### **The two ROM images**

| Image | Chip | Contents |
| :---- | :--- | :------- |
| `os_rom/bin/os_rom_C02.bin` (128K) | BIOS ROM, 16 pages of 8K at `$E000-$FFFF`, selected by `W` | The BIOS, kernel, drivers, IO layer, HyForth, WOZMON, self tests |
| `os_rom/bin/paged_rom_C02.bin` (192K) | Paged ROM banks 0-11 (as many as the ROM disk's files need), at `$A000-$DFFF` | The whole paged ROM is one disk, the ROM disk (`/sd/x`, bound at `/rom`: [io.md](io.md#the-roms-files-rom)), with a partition table in its first 512 bytes (bank 0, `$A000-$A1FF`).  Bank 0, after the table: `COPYTORAM` (`$A200`) and HyForth's variables (`$A300`, copied to `$0800` when the shell starts), HyForth's training scripts and sample binary words.  Bank 1: the [hardware test](../using/wozmon.md#the-hardware-test) (`hwtest/`, scope `HWTEST`), and the ROMs' checksums at its end (`$DFC0`).  Banks 0-1 are the disk's partition 1, so the volume leaves them alone.  Banks 2 on: the ROM disk's HydraFS volume, `/rom`'s files (the test song among them: `songs/test.zsm`, which the build makes from `songs/test.mml` with `sim/tools/hysong.js`), which `sim/tools/mkromdisk.js` adds to the image after the link, from `romfs.txt`.  A bank past the image's end reads as erased (`$FF`): `romsum.js` checks it that way |

Both come from one build (`os_rom/all.s`, linked by `os_rom/os_rom_C02.cfg`); after the link, `sim/tools/mkromdisk.js` writes the ROM disk's table and volume (and reads them back to check every file), and `tools/romsum.js` writes a CRC of each BIOS ROM page and paged ROM bank into bank 1, for the hardware test.  On the V1 board a bank number's bits 2 and 3, and 6 and 7, trade places on their way to the chips ([hardware.md](../hardware.md#the-paged-rom)), so the image holds the bank the CPU selects as `b` at bank `swap(b)`'s place (`mkromdisk.js` and `romsum.js` do this; banks 0-3 are where they seem).  The hardware test runs on its own: BIOS ROM code starts it with `_M_HWT_ENTER` (`include/hwtest.inc`), which gives every task paged ROM bank 1 and jumps to it, and it never calls back.  HyForth's code in the BIOS ROM uses its variables where the paged ROM's copy puts them, and the sample binary words call BIOS ROM addresses, so most changes affect both images: burn both.

### **BIOS ROM pages**

| Page (`W`) | Scope | Contents | Sources |
| :--------- | :---- | :------- | :------ |
| 0 | (global) | Reset, POST gate, the kernel (tasks, scheduler, IRQ dispatch, MMU, shared memory), serial and sound drivers, the IO layer's page 0 part, printing, thunks | `kernel/`, `drivers/serial.s`, `drivers/sound.s`, `io/io_p0.s` |
| 1 | `PAGE1` | HyForth: its interpreter, and its built-in words' headers and code, all run from ROM; a copy of the thunks | `hyforth/` |
| 2 | `PAGE2` | The IO layer: fds, namespaces, pipes, `/dev/cons` and `/dev/ser` (its settings, and the fast serial and tick interrupt handlers: `serfast.s`) | `io/`, `servers/ser_srv.s`, `servers/serctl.s`, `servers/serfast.s` |
| 3 | `PAGE3` | Storage: SPI, the disks' block layer (SD cards; the ROM disk: the paged ROM, read-only; the RAM disks, and starting and stopping them), `/dev/sd`, and HydraFS's format, label, partitions and check | `drivers/spi.s`, `drivers/sd.s`, `servers/sd_srv.s`, `servers/ramdisk.s`, `fs/hfs_format.s`, `fs/hfs_check.s` |
| 4 | `PAGE4` | POST and the self tests (MMU, scheduler, IO); WOZMON (`MON_START`, through gates: it reads `$E000-$FDFF` from BIOS ROM page `ZP_D_PAGE`, 0 unless the disassembler was set otherwise, and `R` runs code on page 0) | `tests/`, `monitor/wozmon.s` |
| 5 | `PAGE5` | Far pointers and references; semaphores; exit statuses; the owner chain at a task's end (`TASK_ORPHANS`: its children, its RAM disk area) and `TASK_MAY` | `kernel/fp.s`, `kernel/sem.s`, `kernel/exits.s` |
| 6 | `PAGE6` | The HydraFS server (`/sd/N/...`), in the storage task, on page 3's block layer: reading, writing, sparse files | `fs/page6.s`, `fs/hfs_srv.s`, `fs/hfs_write.s`, `fs/hfs_sparse.s` |
| 7 | `PAGE7` | The shell: the boot shell's start (the volumes found, one selected), the prompt, the file and card commands HyForth's shell words call (`SH_CMD`), running programs (`run`, the `.hyx` loader, arguments), redirection | `shell/page7.s`, `shell.s`, `files.s`, `run.s`, `redir.s` |
| 8 | `PAGE8` | The text editor (`edit`): a ROM program, run in a task of its own | `shell/page8.s`, `shell/edit.s` |
| 9 | `PAGE9` | The system's servers that run in their client's task: `/dev/proc`, `/env` (each task's environment) and `/dev/time` (the clock: `CLOCK_GET`, `CLOCK_SET`; a DS1747 clock chip, `RTC_BOOT`) | `servers/page9.s`, `servers/proc_srv.s`, `servers/env_srv.s`, `servers/time_srv.s`, `drivers/rtc.s` |
| A | `PAGE1::FAR` | HyForth's far words (their code: the shell's and IO words, tasks, sound, memory records, multiply and divide ...; their headers are on page 1), its error messages and `MALLOC`, and the disassembler | `hyforth/pagea.s`, `hyforth/farwords.s`, `monitor/disasm.s` |
| B | `PAGEB` | Sound: the YM2151's library (the registers' shadow, volumes, notes, patches, claims), `/dev/snd`, the patches (the X16's General MIDI set), the console bell (`YM_BEEP`) | `sound/` |
| C | `PAGEC` | The song player (ZSM): a ROM program the shell starts in a task of its own (`play`), a client of `/dev/snd` | `sound/pagec.s`, `sound/player.s` |
| D-F | | Empty (`/rom` was page D's server; it's now the ROM disk, which page 3's storage driver reads: `SD_ROM_READ`) | |

Every build prints the space left on each page (`tools/rom_space.js`; `node tools/rom_space.js --table` in `os_rom` shows where), and warns when page 0 or COMMON is nearly full.  Page 0 (the kernel) and page 1 (HyForth) have a few hundred bytes each, so new code goes on another page behind gates, and a new HyForth word's code on page A (a far word: see below), with only its header on page 1.  Each page's room above COMMON, `$FE00-$FEFF`, has a segment of its own (`HIGH_Pn`, declared in `kernel/high.s`; page 1's is `FORTH_TOP`) for code that fits nowhere else.  The link map (`os_rom/obj/os_rom_C02.map`) shows each page's segments.

**Fixed addresses on every page:**

| Address | What |
| :------ | :--- |
| `$E000` | Reset entry: sets `W` = 0 and continues on page 0.  Every page starts with it, because `W` isn't reset by hardware |
| `$F800-$F8F8` | The thunk table (pages 0 and 1): `jmp`s to the public calls, below |
| `$FD00-$FDFF` | The COMMON block: IRQ entry stubs and exit, the fast handlers' stubs (VIA, ACIA, YM2151), NMI entry, far-call trampolines, `PEEK_PAGE` (a byte from another page).  Identical on every page (the link checks it) |
| `$FE00-$FEFF` | Each page's room above COMMON (`HIGH_Pn`): page 0's boot welcome, page 1's HyForth words |
| `$FFFA-$FFFD` | NMI vector (the COMMON block's `NMI_ENTRY`) and RESET vector (`$E000`) |

### **Calling across ROM pages**

Writing `W` switches the BIOS ROM page, and the very next instruction is fetched from the new page.  So page switching happens only in the COMMON block, which is the same code at the same address on every page.

**Gates.**  Code calls a routine on another page through a *gate*, a small local stub made by a macro:

```
FAR_GATE_INLINE  IO_OPEN,  PAGE2::IO_OPEN,  2      ; a label IO_OPEN on this page
    ...
    jsr  IO_OPEN                                  ; calls PAGE2::IO_OPEN with W = 2, then comes back
```

* The gate is `jsr FAR_INLINE` followed by the target's address and page (6 bytes).
* `FAR_INLINE` (COMMON) reads them, pushes the caller's `W`, switches page, calls the routine, and switches back.
* **Registers:** `.A`, `.X`, `.Y`, C and V pass through both ways, and N/Z on return reflect `.A`.
* The stack holds the caller's page under the return address, so a routine entered through a gate can find its caller's page (`$0103,X` after `tsx` at its entry; `IO_CALLER_PAGE` does this).
* Not for IRQ handlers.

**Scopes.**  Each page's sources are included inside `.scope PAGEn` (see `all.s`), with the page's gate file first.  A page's gate labels therefore shadow the page 0 routines of the same name for that page's code: `jsr WRITE_CHAR` on page 2 goes through page 2's gate.
* Refer to page 0's own label with `::NAME`, and to another page's with `PAGEn::NAME`.
* The gate files are `hyforth/page1.s`, `io/page2.s`, `drivers/page3.s`, `tests/page4.s`, `kernel/page5.s`, `fs/page6.s`, `shell/page7.s`, `shell/page8.s`, `servers/page9.s` and `hyforth/pagea.s`.
* Page A's code is the scope `FAR` inside `PAGE1` (included from `hyforth/hyforth.s`), so it sees HyForth's names (its zero page, variables and constants), with its own gates first.  Page 1 reaches it through the global aliases after `PAGE1` in `all.s` (`FW_ENTRY_PA` ...).
* The gates from page 0 outward are in `kernel/page0_gates.s`, `io/io_p0.s` and `drivers/storage.s`.

**The page checker.**  The build ends by running `os_rom/tools/check_pages.js` on the debug info.  It lists any `jsr`/`jmp` from one page to a routine on another that doesn't go through a gate ("No cross-page references" when there's none).  It can't see pointers (a routine's address passed in registers), so:
* **Callbacks** (IRQ handlers, serve routines, `TASK_RUN` entry points, `TASK_CALL` routines) must be page 0 addresses, or come with their page where the call takes one (`TASK_RUN`, `TASK_SET_BREAK`).
* **Data pointers** (strings, buffers) are read as the reader sees them.  The IO layer reads names through far pointers ([memory.md](memory.md#far-pointers)), so they can be on the caller's page.  Other buffers must be in RAM.

**Gates into a task.**  `TASK_GATE name, target, task` makes a gate that runs `target` in another task (with `TASK_CALL`); e.g. HyForth's sound words run the sound code in the sound task.

**HyForth's far words.**  HyForth's inner interpreter jumps straight to a word's code, so a built-in word's code has to be on page 1.  A word whose code is on page A has a header made by `def_far` instead of `def_word`: its code on page 1 is `jsr FARWORD` and the address of its code on page A.  `FARWORD` calls it through the gate `FW_CALL`, and it runs as it would on page 1: page A has its own `next`, `this`, `keeps`, `errrtn` and stack routines (`spush_0`, `spull_1` ...) that end the word, back on page 1 (`next`, or `errrtn` with the error in `ERRFLAG`).  They put the stack back as it was when the word started, so a subroutine can use them too.

**HyForth's libraries.**  A built-in word is in the base language or in a library ([HyForth](../using/hyforth.md#the-base-and-its-libraries)).  Each library's headers are a chain of their own: the headers between `lib_begin LIBN_x` and `lib_end` (in `hywords.s` and `primitives.s`) go on library x's chain, and a library can have several such sections.  `LIB_HEADS` holds each chain's head.  The loaded libraries are the bits of `LIBSET`, in HyForth's zero page: `RESFIND` and `words` search the words in RAM, then the libraries loaded from files, then the base, then each loaded library's chain (`LIB_NEXT`).  `cold` loads `LIB_BOOT`, or nothing for a bare Forth (`forth`: `forth_bare_main`, `BAREFLAG`).  A new library needs a `LIBN_` number and bit (`hyforth.s`; up to 8) and its name and needs on page A (`LIBNAMES`, `LIBNEEDS` in `farwords.s`).

The interpreter's line reader calls three hooks on page A (`farwords.s`): `LINE_START` (the last line's redirection and pipeline undone; `boot.hys`), `LINE_PROMPT` and `LINE_READ` (pipelines and redirection set up).  Each does the shell's part (page 7) only when `LIB_SHELL` is loaded, and `RUNNAME` runs a program for an unknown word only then.  So the base itself calls no shell code.

**Libraries from files** (`lib name`: `RLIBLOAD` in `farwords.s`) have `RLIB_MAX` (4) slots in `FORTH_DATA`: a name (`RLIBNAME`), a chain head (`LIB_HEADS2`) and a bit in `LIBSET2`, a second zero-page set that `LIB_NEXT` searches before `LIBSET`.  `LIBSET2`'s bit 7 (`LIB2_BASE`) is the base: its chain is `LIB_HEADS2`'s entry 7.  The file is found by `SH_CMD`'s `SHC_LIBOPEN` (`name.hyl`: the current directory, then `$LIBPATH` or `/lib`), and read by the include engine (`INCOPENFD`).  While it's read, `LASTHEAP` starts from 0, so its definitions make a chain of their own, and the words in RAM's chain (kept in `RLIBSAVE`) aren't searched.  The user's words in RAM are a chain that ends in 0 (`warm`), not in the base.  At the file's end, `INCEND` calls `RLIBEND`: the slot whose `RLIBDEPTH` is that include depth gets the chain (`RLIBHAVE`, `LIBSET2`), and `LASTHEAP` is back.  `INCABORT` (an error while it's read) sets `RLIBFAIL`, so `RLIBEND` frees the slot instead.  `-lib` only clears the bit, so the memory isn't freed.

### **Calling conventions**

* **Success and failure:** the kernel, MMU, scheduler and IO calls return **C = 0** on success, and **C = 1** with an error code in `.A` on failure.
* **Exceptions** (WOZMON's convention): `READ_CHAR` returns C = 1 with a key in `.A` (C = 0: no key), and `GET_CHAR` returns C = 1 with a key (C = 0: an error in `.A`, e.g. `ERR_IO_EOF` at the end of a pipe).
* **Registers:** each routine's header comment in the source says which registers it keeps ("Preserves .X, .Y").  Most calls keep `.X` and `.Y` and the caller's I flag.
* **16-bit values** go in `.A` (low) and `.Y` (high).
* **Zero-page parameters:** some calls take them in the caller's zero page (e.g. `IO_READ` takes the buffer in `ZP_IO_BUF`).  The addresses are in the [API index](#api-index-the-thunks).
* **Interrupts:** calls may enable interrupts only if they were enabled on entry; they use `php`/`sei` ... `plp` for critical sections.  Don't call blocking calls (IO, `GET_CHAR`, `YIELD`, `TASK_SLEEP`) with interrupts off, or from an IRQ handler.

### **Zero page**

Every task has its own zero page, so zero page is only divided up within a task:
* **The OS zero page** grows up from `$02` (the `ZEROPAGE` segment, `os_rom/include/zero.s`), with the same layout in every task because the same ROM code runs in every task.  `$00` and `$01` are the bank registers.
* **A task's own zero page** grows down from `$FF`, reserved with macros:

```
TASK_ZP_BEGIN
TASK_ZP     MY_PTR, 2          ; MY_PTR = $FE
TASK_ZP     MY_COUNT, 1        ; MY_COUNT = $FD
TASK_ZP_END                    ; link error if it runs into the OS zero page
```

Each task's code (the shell, each driver) has its own block; blocks for different tasks may overlap.  For example, HyForth uses `$C8-$FF` in the shell task, and the serial driver its own bytes in the serial task.

OS zero-page variables that calls take parameters in.  `ZP_IO_BUF`, `ZP_IO_CNT` and `ZP_IO_OFS` are at fixed addresses (programs are built against them: `zero.s` asserts them); the others are from the current build's `os_rom/obj/os_rom_C02.lbl`, and can move between builds.  A program's own zero page is `$E0-$FF` (`PROGRAM_ZP`: the OS's stays below it).

| Variable | Address | Used by |
| :------- | :------ | :------ |
| `RAM_BANK_REG` / `ROM_BANK_REG` | `$00` / `$01` | The bank registers |
| `TASK_STATUS_REG` | `$03` | The task's status bits ([tasks.md](tasks.md#task-status)) |
| `ZP_TEMP_VEC`, `ZP_TEMP_VEC2` | `$0A`, `$0C` | `MEM_COPY`, `TASK_START` |
| `ZP_TC_VEC`, `ZP_TC_TASK` | `$1F`, `$21` | `TASK_CALL`, `DEV_REGISTER` (serve routine) |
| `ZP_FP` (4 bytes) | `$42` | Far pointer calls |
| `ZP_IO_BUF`, `ZP_IO_CNT`, `ZP_IO_OFS` | `$06`, `$08`, `$0A` (fixed) | `IO_READ`, `IO_WRITE`, `IO_SEEK`, `IO_STAT`, `IO_MOUNT`, `IO_BIND` |

### **Error codes**
(`os_rom/include/kernel.inc`)

| Code | Name | Meaning |
| :--- | :--- | :------ |
| `$02` | `ERR_OUT_OF_MEMORY` | No memory for the allocation |
| `$40` | `ERR_MEM_NOT_ALLOC` | Not allocated |
| `$41` | `ERR_MEM_NOT_VALID` | Not a valid handle (or another task's RAM) |
| `$42` | `ERR_MEM_NOT_SUPPORTED` | Not possible for this kind of memory (e.g. writing ROM) |
| `$43` | `ERR_MEM_BAD_ARG` | A bad size or offset |
| `$44` | `ERR_MEM_NO_HANDLES` | The handle table is full |
| `$45` | `ERR_MEM_LOCKED` | The allocation is locked |
| `$50` | `ERR_IRQ_CHAIN_FULL` | Already 2 handlers for this IRQ (or a SWI handler) |
| `$51` | `ERR_IRQ_NOT_FOUND` | No such handler registered |
| `$70` | `ERR_IO_NOT_FOUND` | No such file or device |
| `$71` | `ERR_IO_BAD_FD` | fd not open, or out of range |
| `$72` | `ERR_IO_MODE` | Not opened for that (e.g. writing a read-only fd) |
| `$73` | `ERR_IO_WOULD_BLOCK` | No data yet (a non-blocking fd) |
| `$74` | `ERR_IO_EOF` | End of file (`IO_GETC`, `GET_CHAR`) |
| `$75` | `ERR_IO_NO_FDS` | All 12 of the task's fds are open |
| `$76` | `ERR_IO_NO_DEVS` | The device table is full |
| `$77` | `ERR_IO_NAME` | A bad, too long or duplicate name |
| `$78` | `ERR_IO_BAD_REQ` | The server doesn't support that request |
| `$79` | `ERR_IO_DEVICE` | The device didn't respond (e.g. no SD card) |
| `$7A` | `ERR_IO_BROKEN` | Write to a pipe nobody reads |
| `$7B` | `ERR_IO_NO_PIPES` | All pipes are in use |
| `$7C` | `ERR_IO_NS_FULL` | The task's namespace is full |
| `$7D` | `ERR_IO_NS_LOOP` | Too many binds in a row |
| `$7E` | `ERR_IO_NOT_READY` | The device isn't started |
| `$7F` | `ERR_IO_MEDIA` | The medium refused the command or data |
| `$80` | `ERR_IO_NOT_FS` | The card holds no HydraFS (no superblock, or a version this can't read) |
| `$81` | `ERR_IO_FULL` | The card is full |
| `$82` | `ERR_IO_EXISTS` | There's a file or directory by that name already |
| `$83` | `ERR_IO_NOT_EMPTY` | The directory has files in it |
| `$84` | `ERR_IO_BUSY` | The file is open |
| `$85` | `ERR_IO_NOT_DIR` | Not a directory (`IO_CHDIR`, `rmdir`) |
| `$86` | `ERR_IO_IS_DIR` | A directory, where a file was wanted (`rm`, `cp`) |
| `$87` | `ERR_IO_NOT_EXEC` | A Hydra executable whose header doesn't fit task RAM (`run`) |
| `$88` | `ERR_IO_PERM` | Not allowed: another task's area on the RAM disk (`/ram/N`) |
| `$F1` | `ERR_NO_TASKS_AVAILABLE` | All 16 tasks are busy |
| `$F2` | `ERR_TASK_BUSY` | The task (or player) is busy |
| `$F3` | `ERR_BAD_TASK` | Not a task that can be used that way |

### **API index: the thunks**

The thunk table at `$F800` (on BIOS pages 0 and 1) gives every public call a fixed address, so programs in RAM (and HyForth's `syscall`) can call them whatever page they're on.  Each is a `jmp`; call it with `jsr`.  Details are in the guide chapter for each area, and in the routine's header comment in the source.

| Address | Call | In / out (summary) | Guide |
| :------ | :--- | :----------------- | :---- |
| `$F800` | `READ_CHAR` | A key from stdin if there is one: C = 1, `.A` = key | [io](io.md#stdio) |
| `$F803` | `WRITE_CHAR` | Write `.A` to stdout | [io](io.md#stdio) |
| `$F806` | `WRITE_BYTE` | Print `.A` as 2 hex digits | |
| `$F809` | `WRITE_HEX` | Print `.A` (0-F) as a hex digit | |
| `$F80C` | `WRITE_HEX_MASK` | Print `.A`'s low nibble as a hex digit | |
| `$F80F` | `WRITE_HSTRING` | Print the HString (length byte, then text) at `.A.Y` | |
| `$F812` | `WRITE_CRLF` | Print CR LF | |
| `$F815` | `CLEAR_SCR` | Clear the terminal (ANSI) | |
| `$F818` | `DISASM` | Disassemble at `ZP_D_XAM` | [WOZMON](../using/wozmon.md) |
| `$F81B` | `DISASM_AY` | Disassemble at `.A.Y`: C = 0 one instruction, C = 1 `.X` instructions | |
| `$F81E` | `MEM_COPY` | Copy `.A.Y` bytes from `ZP_TEMP_VEC` to `ZP_TEMP_VEC2` | |
| `$F821` | `MM_ALLOC` | `.A.Y` = size, `.X` = 0 or `AI_PAGED` → `.A` = handle | [memory](memory.md) |
| `$F824` | `MM_FREE` | `.A` = handle | |
| `$F827` | `MM_READ` | `.A` = handle, `.Y` = offset → `.A` = byte | |
| `$F82A` | `MM_WRITE` | `.A` = handle, `.Y` = offset, `.X` = byte | |
| `$F82D` | `MM_LOCK` | `.A` = handle → `.A.Y` = pointer, `.X` = previous RAM bank | |
| `$F830` | `MM_UNLOCK` | `.A` = handle, `.X` = RAM bank from `MM_LOCK` | |
| `$F833` | `MMU_TEST` | Run the MMU self test | [WOZMON](../using/wozmon.md#self-tests) |
| `$F836` | `SH_ALLOC` | `.A.Y` = size → `.A` = shared handle | [memory](memory.md#shared-memory) |
| `$F839` | `SH_ATTACH` | `.A` = shared handle: take a reference | |
| `$F83C` | `SH_DETACH` | `.A` = shared handle: drop the reference | |
| `$F83F` | `SH_READ` | `.A` = shared handle, `.Y` = offset → `.A` = byte | |
| `$F842` | `SH_WRITE` | `.A` = shared handle, `.Y` = offset, `.X` = byte | |
| `$F845` | `SH_LOCK` | `.A` = shared handle → mapped at `$8000`; `.X`, `.Y` = what to restore | |
| `$F848` | `SH_UNLOCK` | `.X`, `.Y` from `SH_LOCK` | |
| `$F84B` | `MM_TASK_RESET` | `.A` = task: free everything it has | |
| `$F84E` | `MM_FIND` | `.A.Y` = address → `.A` = handle | |
| `$F851` | `MM_SET_FLOOR` | `.A` = lowest page the MMU may hand out | |
| `$F854` | `YIELD` | Let the other tasks run | [tasks](tasks.md) |
| `$F857` | `NO_PREEMPT` | Hold the CPU (nestable) | |
| `$F85A` | `PREEMPT` | Undo `NO_PREEMPT` | |
| `$F85D` | `TASK_WAIT` | Wait until `IO_WAKE` | |
| `$F860` | `IO_WAKE` | `.A` = task: wake it | |
| `$F863` | `TASK_RUN` | `.A.Y` = entry, `.X` = ROM page → `.A` = new task | |
| `$F866` | `TASK_STATUS` | `.A` = task → `.A` = its status | |
| `$F869` | `SCHED_TEST` | Run the scheduler self test | |
| `$F86C` | `IO_OPEN` | `.A.Y` = name, `.X` = mode → `.A` = fd | [io](io.md) |
| `$F86F` | `IO_CLOSE` | `.A` = fd | |
| `$F872` | `IO_READ` | `.A` = fd, `ZP_IO_BUF`, `ZP_IO_CNT` → `ZP_IO_CNT` = bytes read | |
| `$F875` | `IO_WRITE` | `.A` = fd, `ZP_IO_BUF`, `ZP_IO_CNT` → `ZP_IO_CNT` = bytes written | |
| `$F878` | `IO_GETC` | `.X` = fd → `.A` = byte | |
| `$F87B` | `IO_PUTC` | `.X` = fd, `.A` = byte | |
| `$F87E` | `IO_SEEK` | `.A` = fd, `ZP_IO_OFS` = 32-bit offset | |
| `$F881` | `IO_STAT` | `.A` = fd, `ZP_IO_BUF` = 48-byte buffer | [io](io.md#stat) |
| `$F884` | `IO_CTL` | `.A` = fd, `.X` = code, `.Y` = argument | |
| `$F887` | `DEV_REGISTER` | `.A.Y` = name, `.X` = task, `ZP_TC_VEC` = serve routine | [servers](servers.md) |
| `$F88A` | `IO_TEST` | Run the IO self test | |
| `$F88D` | `GET_CHAR` | Wait for a key from stdin: C = 1, `.A` = key | [io](io.md#stdio) |
| `$F890` | `IO_DUP2` | `.A` = fd, `.X` = new fd: make `.X` a copy of `.A` | [io](io.md) |
| `$F893` | `IO_PIPE` | → `.A` = read fd, `.X` = write fd | |
| `$F896` | `IO_DUP` | `.A` = fd → `.A` = a new fd for the same file | |
| `$F899` | `TASK_CLONE` | `.A.Y` = entry, `.X` = page → `.A` = new task, a copy of this one | [tasks](tasks.md#starting-tasks) |
| `$F89C` | `IO_MOUNT` | `.A.Y` = path, `ZP_IO_BUF` = device name | [io](io.md#namespaces) |
| `$F89F` | `IO_BIND` | `.A.Y` = path, `ZP_IO_BUF` = target | |
| `$F8A2` | `IO_UNMOUNT` | `.A.Y` = path | |
| `$F8A5` | `IO_NS_LIST` | Print the namespace | |
| `$F8A8` | `TASK_SET_BREAK` | `.A.Y` = break handler, `.X` = its page | [tasks](tasks.md#signals-break-and-kill) |
| `$F8AB` | `TASK_SIGNAL` | `.A` = break or kill flag, `.X` = task | |
| `$F8AE` | `CONS_SET_FG` | `.A` = task: bring it to the front | [io](io.md#the-console) |
| `$F8B1` | `FP_MAKE` | `.A.Y` = address, `.X` = ROM page → `ZP_FP` | [memory](memory.md#far-pointers) |
| `$F8B4` | `FP_READ` | `.Y` = offset → `.A` = byte at `ZP_FP` + `.Y` | |
| `$F8B7` | `FP_WRITE` | `.Y` = offset, `.X` = byte | |
| `$F8BA` | `FP_COPY` | `.A.Y` = destination, `.X` = count; C = 1 for a string | |
| `$F8BD` | `MM_REF` | `ZP_FP` → `.A` = MMU handle for it | |
| `$F8C0` | `MM_FP` | `.A` = handle → `ZP_FP` | |
| `$F8C3` | `SH_REF` | `ZP_FP` → `.A` = shared handle for it | |
| `$F8C6` | `SH_FP` | `.A` = shared handle → `ZP_FP` | |
| `$F8C9` | `IO_CREATE` | `.A.Y` = name, `.X` = mode, `ZP_IO_BUF` = new file's mode bits → `.A` = fd | [io](io.md#the-files-on-a-card) |
| `$F8CC` | `IO_REMOVE` | `.A.Y` = name | [io](io.md#the-files-on-a-card) |
| `$F8CF` | `IO_WSTAT` | `.A` = fd, `ZP_IO_BUF` = stat record | [io](io.md#the-files-on-a-card) |
| `$F8D2` | `IO_CHDIR` | `.A.Y` = a directory's path: the current directory | [io](io.md#the-current-directory) |
| `$F8D5` | `IO_GETCWD` | `ZP_IO_BUF` = 64-byte buffer ← the current directory | [io](io.md#the-current-directory) |
| `$F8D8` | `SEM_NEW` | `.A` = count, `.Y` = 0 or `SEM_MUTEX` → `.A` = semaphore | [tasks](tasks.md#semaphores) |
| `$F8DB` | `SEM_ACQUIRE` | `.A` = semaphore: take one (waits) | [tasks](tasks.md#semaphores) |
| `$F8DE` | `SEM_TRY` | `.A` = semaphore: take one, or `ERR_SEM_BUSY` | [tasks](tasks.md#semaphores) |
| `$F8E1` | `SEM_RELEASE` | `.A` = semaphore: give one back | [tasks](tasks.md#semaphores) |
| `$F8E4` | `SEM_FREE` | `.A` = semaphore: free it | [tasks](tasks.md#semaphores) |
| `$F8E7` | `TASK_SLEEP` | `.A.Y` = ticks (200 a second, up to 32767) | [tasks](tasks.md#waiting-and-sleeping) |
| `$F8EA` | `TICKS_GET` | → `.A.Y` = the tick count | [tasks](tasks.md#waiting-and-sleeping) |
| `$F8ED` | `CLOCK_GET` | `.X` = a zero page address ← 4 bytes: seconds since 2000-01-01 | [io](io.md#the-clock-devtime) |
| `$F8F0` | `TASK_EXITS` | `.A` = exit code, `ZP_IO_BUF` = message (high byte 0: none): end this task | [tasks](tasks.md#exit-statuses) |
| `$F8F3` | `TASK_JOIN` | `.A` = task: wait for it to end → `.A` = its code, its message at `ZP_IO_BUF` (high byte 0: not wanted) | [tasks](tasks.md#exit-statuses) |
| `$F8F6` | `SHELL_CMD` | An entry point for `TASK_RUN` (`.X` = 0): a command shell running the lines on its stdin (`rc -c`) | [tasks](tasks.md#starting-tasks) |

Calls without a thunk (for ROM code; reached with a gate from other pages): `TASK_SLEEP_UNTIL`, `TASK_START`, `TASK_CALL`, `IRQ_REGISTER`, `IRQ_UNREGISTER`, `SWI_REGISTER`, `SWI_UNREGISTER`, `SW_INT`, `DRV_START`, `IO_FLUSH`, `YM_BEEP`, and the server helpers `IO_SRV_MAP`, `IO_SRV_UNMAP`, `IO_SRV_COUNT`.

**Adding a thunk:** add it at the end of the list in `include/thunks.inc`, with its address: both copies of the table are made from it, page 0's (`kernel/thunks.s`) and page 1's (`hyforth/page1.s`), and the build checks every address.  Give it as `THUNK_P0`: page 1's entry then goes on to page 0's (`THUNK_TO_P0`), so code running on page 1 (HyForth's binary words) can call it too; `THUNK` is for a call page 1 has a gate of its own for, which its entry jumps to.  A call on another page gets its gate in `GATES_P0` (`kernel/page0_gates.s`), or, as `GATES_P0` is full, in `BIOS_THUNKS` after the thunks' `jmp`s (`FAR_GATE_INLINE`, at the end of `kernel/thunks.s`): the thunks are at a fixed `$F800`, and room below them doesn't help what's after them.  Never move existing entries: programs rely on the addresses.

### **Adding code**

* **Where:** pick the page by role (table above).  If page 0 code needs it, add a gate on page 0 (`page0_gates.s`).  If code on another page calls into it, add a gate in that page's gate file.
* **Build:** see [Getting started](../getting-started.md#building).  The build stops at the first error, then checks for cross-page calls.
* **Test:** run `node build.js rom test` (or `makeC02 test`: build, then the regression tests).  For a new feature, add a test to `sim/regress.js` ([emulator](../tools/emulator.md#regression-tests)).
* **Style:** match the file's formatting: mnemonics and operands in fixed columns, `;` comments aligned, CRLF line endings, a header comment on each routine (what it does, IN, OUT, what it preserves).
