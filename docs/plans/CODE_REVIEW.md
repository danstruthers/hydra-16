## **Code review: the Hydra-16 software**

A review of the whole software tree as of branch `1.8C_0.5` (September 2026): what's good, what's wrong, and what would make it easier to grow.  It covers:
* the OS ROM (`os_rom/`, about 32,000 lines of 65C02 assembly);
* the emulator, tests and tools (`sim/`);
* the C library and samples (`programs/`);
* the documentation (`docs/`).

The bugs below were reproduced in the emulator.  The other findings come from reading the code, the link map and the build.

**Status:** fixed on the same branch, except where the table at the end says otherwise ([what was done](#what-was-done)).  The text below is the review as written; its numbers are from before the fixes.

### **Contents**
1. [Overall](#overall)
2. [Bugs](#bugs)
3. [ROM space](#rom-space)
4. [Zero page and the namespace](#zero-page-and-the-namespace)
5. [The kernel and the IO layer](#the-kernel-and-the-io-layer)
6. [The source tree](#the-source-tree)
7. [The build](#the-build)
8. [The emulator](#the-emulator)
9. [The tests](#the-tests)
10. [HyForth](#hyforth)
11. [The C library](#the-c-library)
12. [The documentation](#the-documentation)
13. [The repository](#the-repository)
14. [Recommendations, in order](#recommendations-in-order)

---

### **Overall**

This is careful, consistent work, and unusually well tested for a hobby OS.  What it does well:
* **Every routine says what it takes, gives and preserves** (`IN:`, `OUT:`, `Preserves`, `Modifies`), and the code follows it.  Register and flag conventions across gates (`FAR_INLINE`: `.A .X .Y C V` both ways) are stated once and kept.
* **The build checks itself.**  Layout `.assert`s (COMMON's copies lining up, the thunk table's place, structure offsets, ZP blocks not overlapping), `check_pages.js` (calls across ROM pages without a gate), `romsum.js` (CRCs for the hardware test), and `sim/regress.js` comparing the C library's addresses with the ROM's.
* **The design is coherent.**  Plan 9's ideas (devices as file servers, per-task namespaces, `/env`, exit statuses as notes) are applied consistently.  New features (`/rom`, `/dev/snd`, `/dev/time`) slot in as servers without special cases.
* **The tests are real.**  51 regression tests boot the actual ROM images.  They cover:
  * seeded or random power-up states, to catch uninitialised RAM;
  * fault injection: stuck IRQs, RAM address lines, a missing chip;
  * both ACIA variants, and SD card images checked by an independent tool.
* **The documentation is thorough**, kept up to date with the code, and has the design history in `docs/plans/`.

The main risks are about growth, not quality:
* **The fixed resources are used up.**  Page 0, COMMON, the OS zero page and the per-task namespace are all close to full.
* **The toolchain is Windows-only and manual.**
* **The emulator is one file.**
* **HyForth has a few traps** that will hurt newcomers more than they hurt the author.

---

### **Bugs**

**1. `syscall` can't reach the newer thunks.**  HyForth's `syscall` (`hyforth/hywords.s`, `SYSCALL`) does `jmp (TEMP1)` while running on BIOS ROM page 1.  Page 1's copy of the thunk table (`hyforth/page1.s`) stops at `IO_GETCWD` (`$F8D5`).  From `$F8D8` page 1 has its own gates (`GATES_P1`).  So calling any thunk added since then runs unrelated page 1 code:
* the semaphores, `TASK_SLEEP`, `TICKS_GET`, `CLOCK_GET`, `TASK_EXITS`, `TASK_JOIN` and `SHELL_CMD`;
* and machine code loaded with `bload` that calls them.

```
/> $F806 $41 0 syscall .         \ WRITE_BYTE: fine
41 00E8
/> $F8EA 0 0 syscall .           \ TICKS_GET, as the API index lists it
 !DS PTR ERROR!
```

The docs invite exactly this (`hyforth.md`: "`syscall` ... e.g. a thunk").  Two fixes:
* Recommended: make `syscall` call through page 0 (a far call with `ZP_FAR_PAGE` = 0, which is harmless for RAM addresses).  Then every thunk works from HyForth, now and later.
* Or add the missing entries to page 1's copy.  This needs room after the table, so `GATES_P1` would have to move.

Also, `syscall` treats `.X` = 0 on return as an error (`!SYS ERR!`), and many calls legitimately return `.X` = 0.  Returning C (the OS's error convention) would be better.

**2. The IO self test assumed free namespace entries.**  With `/rom` mounted at boot, the IO test (`F88AR`) failed at step `z` (`ERR_IO_NS_FULL`): it binds 4 names and a task has only 5 entries.  On this branch `/rom` no longer takes an entry: the IO layer resolves it like `/env`.  The test is still fragile.  It should unmount between steps, or check how many entries are free first.

**3. A mangled line in `sim/regress.js`** (around line 67): the closing brace of `zsmSong()` has two old comments fused onto it (`}  // programs/c/bin's (make.bat builds them)  // Wait n * ~2M cycles before the next key`).  Those comments belonged to the `C_SAMPLES` and `W` definitions above.  It's harmless, but put them back.

---

### **ROM space**

Free space per BIOS ROM page, from the link map (`$E000-$FEFF`; `$FF00-$FFF9` is the I/O space on every page):

| Page | Role | Free | Where |
| :--- | :--- | :--- | :---- |
| 0 | Kernel | **40 bytes** | 31 + 1 + 8 |
| COMMON | (every page) | **1 byte** | |
| 1 | HyForth | 284 | 23 + 5, and **256 unused at `$FE00-$FEFF`** |
| 2 | IO | 971 | 715 + 256 |
| 3 | Storage | 2,045 | |
| 4 | Tests, POST | 3,332 | |
| 5 | Far pointers, semaphores, exits | 5,985 | |
| 6 | HydraFS | 760 | |
| 7 | Shell | 3,800 | |
| 8 | Editor | 5,221 | |
| 9 | Client-task servers | 4,174 | |
| A | HyForth's far words | 1,594 | |
| B | Sound | 1,277 | |
| C | Player | 6,541 | |
| D | `/rom` | 6,642 | |
| E, F | Empty | 7,675 each | |

**Findings:**
* **Every page but 0 has 256 bytes it doesn't use**, at `$FE00-$FEFF`, between COMMON and the I/O space.  Page 0 has WOZMON there, and on the others no segment claims it.  For page 1 that's ten times its free space: give pages 1-F a `HIGH_Pn` segment at `$FE00` in `os_rom_C02.cfg` (HyForth's `FORTH_HIGH` could spill into it).  `rom-layout.md`'s "page 1 has about 40 bytes left" undercounts for this reason.
* **Page 0 and COMMON are the hard limits.**
  * **Move WOZMON off page 0.**  It's 248 bytes at `$FE00`, used only after `bye` or a crash.  It could run from page 4 (diagnostics) behind a gate.  The NMI and reset vectors stay where they are.
  * **Merge the peek routines in COMMON.**  `PEEK_D_XAM` and `FP_PEEK_PAGE` are the same routine (switch `W`, read `(zp),Y`, switch back) with different zero page pointers.  One `PEEK_PAGE` (`.X` = page, a shared pointer) saves about 15 bytes of COMMON on every page.  That's enough for another fast interrupt stub (the VERA's: [VIDEO.md](VIDEO.md)).
* **Make the budget visible.**  Have the build print this table (it's a few lines of JavaScript over the map, like `check_pages.js`).  Fail, or warn loudly, when page 0 or COMMON drops below a threshold.  The free-space numbers written in the docs go stale; one printed by every build doesn't.
* **The paged ROM** (4 MB, 192K used) is the place for everything that isn't code: fonts, help text, more `/rom` programs and libraries, song banks.  The [`/rom` work](../programming/io.md#the-roms-files-rom) makes that easy now.

---

### **Zero page and the namespace**

* **The OS zero page is full**, and the newest servers share scratch by aliasing:
  * the `/rom` server's `ROM_*` are other servers' `ZP_ENV_*`, `ZP_TIME_*` and `ZP_PROC_*`;
  * the HydraFS server borrows the SD server's bytes.

  The aliasing is correct, because these servers never run at once in one task, but nothing checks that.  Make it a declared union: one `CLIENT_SCRATCH` block of N bytes in `zero.s`, with each client-task server naming its own layout over it (`.struct` or `TASK_ZP`-style macros) and an `.assert` that each fits.  A new server then can't silently use a byte another one is holding across a call.
* **Candidates to move out of the OS zero page:** the disassembler's `ZP_D_*` (7 bytes) and WOZMON's `ZP_WM_*` (5).  Both run only in HyForth's task (or after `bye`), so they could live in that task's ZP block.  `REORG_PLAN.md` step 6 kept `ZP_D_*` because the disassembler needs a fixed address.  A fixed address in HyForth's block works just as well.
* **5 namespace entries a task is few.**  `/sd` takes one, and a user's binds and mounts get 4.  `/rom` and `/env` now cost none, and `/dev/vid` won't either.  Entries are 32 bytes (`NS_PREFIX_MAX` 13 and `NS_TARGET_MAX` 15 characters).  If more are wanted, the per-task IO block's `$20-$BF` could be relaid: for example, 6 entries of 26 bytes with shorter prefixes, or the current directory moved to the data area's end.

---

### **The kernel and the IO layer**

* **The scheduler and `TASK_CALL` are well reasoned:** preemptible servers (`TASK_GUEST_OUT_FLAG`), IRQ calls into a busy server, `NO_PREEMPT` around the waiting-flag race in `IO_SERVE`.  The comments explain the why, which is what matters most in code like this.  Keep it that way.
* **IRQ handlers run through `TASK_CALL_IRQ`** in their driver's task.  That's clean, but slow for anything at a high rate.  The VIA, ACIA and YM2151 have fast handlers in COMMON for this reason.  Write down a rule for drivers: up to a few hundred interrupts a second through the dispatcher; above that, a fast stub (and COMMON room: see above).
* **IO copies every byte twice** (caller ↔ transfer area ↔ server), 256 bytes a request, each request a `TASK_CALL`.  That's fine for files and the console.  Bulk transfers to devices (graphics, PCM) should let the client own the device directly after a claim, as `/dev/snd` already does for channels and as [VIDEO.md](VIDEO.md) plans for the VERA.
* **`IO_OPEN`'s special prefixes** (`/dev/`, `/env`, `/rom`) are now a small table in `io/io.s` (`S_OWN_PREFIXES`).  If more come, make it a list of (prefix, device) pairs in one place, documented in `io.md`, rather than more special cases.
* **Errors:** the codes are consistent (`kernel.inc`), but the user sees `!IO ERR!` and has to type `ioerr`.  A table of short messages (`not found`, `no space`, `busy` ...) in the paged ROM would let HyForth and C's `strerror` say what happened.

---

### **The source tree**

* **`os_rom/io/` holds two things:** the IO layer (`io.s`, `io_p0.s`, `ns.s`, the pipes) and nine file servers (`hfs_*`, `sd_srv`, `ser_srv`, `env_srv`, `proc_srv`, `time_srv`, `rom_srv` ...).  Split it into:
  * `io/`: the layer;
  * `servers/`: the devices;
  * `fs/`: HydraFS, its server, writing, check and format, as `REORG_PLAN.md` step 4 planned.
* **The thunk table is written twice:** `kernel/thunks.s` (page 0) and `hyforth/page1.s` (page 1, a shorter copy).  Only the start and end are `.assert`ed.  Two entries swapped in one copy would assemble and link, and call the wrong routine.  Generate both from one list in an include file (a macro per entry: `THUNK READ_CHAR`), and give page 1 the whole list (see bug 1).
* **Generated files are kept as sources:**
  * `os_rom/songs/test_rom.s` is made by `hysong.js` at every build;
  * `programs/c/bin/*.hyx` and `os_rom/bin/*.bin` are build outputs.

  Keeping the ROM images in Git was a deliberate choice (anyone can burn them without a toolchain).  But it means every build shows them changed, and branches conflict on them.  Better:
  * generate into `obj/` and keep only the sources;
  * attach the images to tagged releases (or commit them only at a release);
  * stamp the version and commit into the ROM (below).
* **The `FAR_JMP_GATE` macro** is 15 bytes at each use, against `FAR_GATE_INLINE`'s 6.  A `FAR_JUMP_INLINE` like the call's would save space wherever it's used.
* **Program sources** live in two places: `programs/hello.s` with `hyx.inc` and `make.bat` (assembly), and `programs/c/` (C).  Give assembly its own folder (`programs/asm/`) with the same layout as C's (`samples/`, `bin/`), and one build for both.

---

### **The build**

* **It's Windows batch files**, and the two builds find cc65 differently:
  * `os_rom/makeC02.bat` expects `ca65` and `ld65` on the `PATH`;
  * `programs/c/make.bat` uses `CC65_HOME`, defaulting to `C:\source\cc65\win64_snapshot`.

  Node.js is already required (`hysong.js`, `mkromfs.js`, `romsum.js`, `check_pages.js`, the tests).  So one `build.js` could do the whole sequence on any OS:
  1. the song;
  2. assemble and link;
  3. `/rom`, checksums and the page check;
  4. the budget report.

  It would find cc65 through `CC65_HOME` or the `PATH` in both places, and leave the `.bat` files as one-line wrappers.  Then Linux and macOS users can build too.
* **`check_pages.js` doesn't stop the build:** `makeC02.bat` runs it last, without checking its exit code.  A missing gate is a crash on the hardware, so make it fail the build.
* **Continuous integration:** the whole suite takes about a minute.  A GitHub Actions job (build cc65 from source or fetch a snapshot, build, run `regress.js`) on every push would catch breakage before it reaches a branch.
* **A version stamp:** `HyForth 0.91 05-07-2026` is a hand-edited string (`hyforth/upper.s`).  Have the build write the version, date and Git commit into an include file.  Then the boot banner, a `ver` word and `/dev/sysname` (Plan 9's) can report them, and a bug report says which ROM it came from.

---

### **The emulator**

`sim/hydrasim.js` is one file of 829 lines.  It holds the CPU, the memory system, every device (ACIA, VIA, SPI and SD, YM2151, DS1747), the command line, the report and the interactive terminal.  It's dense but readable, and it's the most valuable tool in the project.  For what comes next:
* **Split it into modules:** `cpu65c02.js`, `memory.js` (tasks, banks, the paged ROM's bit swaps), one file per device, `cli.js`.  Give each device the same small interface: `read`, `write`, `tick` (cycles), `irq`, `reset`.  Then cards plug into slots by number (`--card 0:vera`) as they do on the board.
* **Keep the core free of Node.**  No `fs`, `process` or `readline` below the CLI layer, so the same core runs in a browser: a web page with a terminal (xterm.js), later the VERA's canvas.  That's the easiest way to let people try the Hydra ([NEXT_STEPS.md](NEXT_STEPS.md)).
* **The 6502 core**'s cycle counts are tested (`cpu-cycles`); its instructions could also be checked against Klaus Dormann's 65C02 functional tests, the standard, which run in seconds.
* **A debugger:**
  * the emulator already has the build's debug info (`--profile` reads `os_rom_C02.dbg`), and `--pc`/`--watch`;
  * an interactive mode adds breakpoints by label, single steps, and source lines from the listing;
  * that makes ROM work much faster than adding prints.

---

### **The tests**

* **`regress.js` is one file of 1,100 lines.**  Move each area's tests into its own file (`sim/tests/io.js`, `hfs.js`, `sound.js` ...) with the runner separate.  It's easier to find a test and to add one.
* **Typing waits by cycle count** (`W(n)`: 2 million cycles before the next key).  That's slow (most waits are much longer than needed), and brittle (a slower code path makes a test fail by typing too soon).  Let the input wait for text instead, like `expect`: type the next line when the prompt (or a given string) appears.  `--mark` already finds text in the output, so the emulator is halfway there.  The suite would likely get several times faster.
* **Count and list the tests in one place.**  `getting-started.md` says how many there are, and it goes stale with every new test.  Have `regress.js --list` produce the documentation's list, or don't put a number in the docs.
* **Gaps:** the tests run one build of the ROM, so its build options go untested:
  * the 7.16 MHz build (`CPU_CLOCK_MULT` 2; `hwtest-faults` only runs the normal build at the wrong clock);
  * the WDC ACIA build (`SER_ACIA`), whose sending is paced by VIA timer 2: the emulator models the chip (`--acia wdc`), but no test builds the ROM for it.

  A test step that builds each variant into its own directory (`--rom`) and boots it would cover them.

---

### **HyForth**

The shell works well, and its use of the OS (pipes, redirection, programs by name, background tasks) is a highlight.  The language has traps for newcomers:
* **A number inside a definition is used while compiling, silently.**  `: x 65 . ;` seems to work the first time (`x` prints `0041`: the 65 was left on the stack by the compiler).  It fails the second time (`!DS PTR ERROR!`).  The docs explain the `lit [ 65 , ]` workaround, but almost every Forth tutorial in the world writes `: x 65 . ;`.  Make the interpreter compile a literal when it's compiling and the word is a number.  This is the most important usability fix in the review.
* **Output is always 4 hex digits.**  `11 .` prints `000B`.  Add decimal output (`base`, or `u.` and `.d`), at least as a choice.  A beginner's first program prints a number.
* **No line history or editing:** only Backspace.  Up/Down for the last lines and Left/Right to edit are what every user will reach for.  The console already decodes the arrow keys for conio.
* **Errors are terse** (`!IO ERR!`, then `ioerr` for a number).  See the error-message table above.
* **`syscall`:** see bug 1.

---

### **The C library**

Good: it covers stdio, files, directories, the environment, processes, sound and the console.  It's checked against the ROM by the tests, and documented in a guide of its own.  Suggestions:
* **One build** with the ROM's (above), finding cc65 the same way.
* **`FILENAME_MAX` is 17** in cc65's headers for this target, while HydraFS names are up to 31 characters and paths 64.  `hydra.h` notes it, but a program that sizes buffers with `FILENAME_MAX` will truncate.  Provide a `stdio.h` wrapper, or at least say so prominently in `c.md`.
* **`MM_ALLOC` from C** (paged memory beyond the 32K a task has) is still to do.  Larger programs (games, editors) will want it.
* **Next headers:** `vera.h` and a TGI driver ([VIDEO.md](VIDEO.md)), `i2c.h` and `spi.h` when those devices exist.

---

### **The documentation**

Thorough and accurate on the whole.  Small fixes:
* **The README's repository table** lists `os_rom/`, `sim/`, `board/` and `docs/`, but not `programs/`.
* **Hand-written numbers go stale:** page free space in `rom-layout.md` (and the 256 bytes at `$FE00` it misses), the test count in `getting-started.md`.  Generate them, or point to the build's report.
* **For newcomers**, add tutorials before the reference:
  * your first HyForth session;
  * your first C program on a card;
  * your first assembly program;
  * your first driver.

  The guides are complete references; a reader who doesn't know what a gate or a task call is needs a gentler road in.
* **The hardware reference's V1 errata** now say the bank bits 6/7 swap is confirmed.  The tools depend on it.

---

### **The repository**

* **No `.gitignore` at the top.**  KiCad's backups (`*-backups/`), lock files (`~*.lck`) and `fp-info-cache` show as untracked under `board/`.  A root `.gitignore` with those patterns quiets them without touching anything in `board/`.
* **Untracked `os/`** at the top level: if it's someone's local work, ignore it locally (`.git/info/exclude`).
* **Branch naming** (`1.8C_0.3` ...) works for one person.  With collaborators, a `main` that always builds and passes, and short-lived feature branches, are easier to follow.

---

### **Recommendations, in order**

| # | What | Why | Size |
| :- | :--- | :-- | :--- |
| 1 | `syscall` through page 0; one thunk list for both pages | A crash on a documented use | Small |
| 2 | HyForth compiles numbers in definitions | The biggest trap for every newcomer | Small to medium |
| 3 | A ROM budget report in the build; `check_pages` fails the build | Page 0 and COMMON are the limits; see them on every build | Small |
| 4 | Use the 256 bytes at `$FE00` on pages 1-F; merge the COMMON peeks; WOZMON off page 0 | Room for the next features (video, input) | Small to medium |
| 5 | `build.js` for both builds, any OS; CI on GitHub | Lets collaborators build and keeps `main` green | Medium |
| 6 | Split the emulator into modules with a device interface; core without Node | Needed for the VERA and the web emulator | Medium |
| 7 | Tests wait for text, not cycles; tests in files by area | Faster, steadier tests | Medium |
| 8 | Decimal output, line history and error messages in HyForth | Everyday comfort | Medium |
| 9 | A declared client-task ZP scratch union | Protects the servers' shared scratch | Small |
| 10 | Split `os_rom/io/` into the layer, servers and the filesystem | Easier to find things | Small (moves) |
| 11 | Generated files out of Git; release images; a version stamp | Fewer conflicts; traceable ROMs | Small |
| 12 | Tutorials | A way in for newcomers | Ongoing |

---

### **What was done**

| # | Done | Left |
| :- | :--- | :--- |
| 1 | `syscall` is a far word that calls through page 0, and pushes `.X` (no more silent abort on `.X` = 0); a new word `sys ( addr a x y -- a x y p )` passes every register and the flags.  The thunk table is one list (`include/thunks.inc`) for both pages, with every address checked; page 1's entries for the calls it has no gate for go on to page 0's (`THUNK_TO_P0`), so `bload`ed code can call them.  Test: `syscall` | |
| 2 | A number in a definition compiles as `lit` and the number.  Decimal input's range is checked (`-32768` was refused; `70000` wrapped silently).  Test: `forth-numbers` | |
| 3 | `tools/rom_space.js` prints the free space on every page at each build, and warns for page 0 and COMMON; `check_pages.js` fails the build | |
| 4 | Pages 0 and 2-F have a `HIGH_Pn` segment at `$FE00` (`kernel/high.s`), page 1 `FORTH_TOP`; WOZMON is on page 4 (`R` runs on page 0, reads go through `PEEK_D_XAM`); COMMON's two peeks are one, `PEEK_PAGE`; `FAR_JMP_GATE` is a 6-byte far call.  Page 0: 274 bytes free (from 40), COMMON 21 (from 1) | |
| 5 | `build.js` (any OS; cc65 by `CC65_HOME`, the `PATH` or the Windows snapshot); the `.bat` files run it; `.github/workflows/build.yml` builds cc65 and runs `node build.js test` | Pushing it to GitHub turns the workflow on |
| 6 | `sim/lib/`: `machine.js`, `cpu65c02.js`, a module per device, no Node.js in them; `hydrasim.js` is the command line.  Its output is byte for byte the old emulator's (reports and profiles compared) | The web page itself ([NEXT_STEPS.md](NEXT_STEPS.md)) |
| 7 | The tests are in `sim/tests/` by area; `\p` in the input waits for a (settled) prompt, and a run ends 3M cycles after its last command's prompt (`--stop-after-input`; `fullRun` to opt out).  The suite takes 36 s (from 52).  The two ROM build options are tested too (`build.js variants`: the CPU at 7.16 MHz, the WDC ACIA), which found that the WDC build no longer linked (page 0 was full: fixed) | 14 tests still wait by cycles where a prompt can't be waited for (inside a program, or where time has to pass) |
| 8 | Decimal output (`decimal`, `hex`, `u.`); a line editor at the prompt (arrows, Home/End, Delete, history, Ctrl-U); `!IO ERR!` says why (`not found`); C's `_stroserror` too.  Tests: `line-edit`, `rom` | |
| 9 | `ZP_CS`: the client-task servers' 13 bytes, declared once; `CS_FITS` checks each server's names for them | |
| 10 | `io/` (the layer), `servers/`, `fs/` (HydraFS), `drivers/rtc.s`; `programs/asm/` beside `programs/c/` | |
| 11 | `os_rom/VERSION` (shown at boot: no date or commit, so a build's images stay the same); the generated `test_rom.s` is in `obj/` | Taking the ROM images out of Git, and branch naming: your call (both work as they are) |
| 12 | [docs/tutorial.md](../tutorial.md): the first hour, in the emulator | More (a first driver; the VERA, once it's there) |

Also: the IO self test needs 2 free namespace entries (not 4); `/rom` takes none (as `/env`); the README lists `programs/` and `build.js`; a root `.gitignore` for KiCad's backups and locks; `FILENAME_MAX`, the IRQ-rate rule for drivers and the names that need no mount are in the docs.  Not done: `MM_ALLOC` from C, and a note when a background task ends ([IDEAS.md](IDEAS.md)).

Found on the way, not fixed:
* A key typed while the Hydra boots (before the first prompt) is lost.  Piped input to `hydrasim.js -i` loses its first character for it.
* The WDC ACIA build sent slowly at 115200: `words` took about 7.3M cycles (the Rockwell build: 2.5M).  Each character's timer 2 interrupt went through the dispatcher and a `TASK_CALL` (about 650 cycles) instead of the fast path the Rockwell build uses at 115200.  *(Fixed: `SER_T2_FAST` sends for both chips; `words` now takes 2.3M cycles on the WDC build, with no byte written while one is still going, and the `fast-output` and `sound` tests run on it too.)*
