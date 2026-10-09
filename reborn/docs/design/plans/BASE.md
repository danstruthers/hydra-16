## **The base: the kernel, a serial driver and a monitor, in `/base`**

A plan (October 2026) for the user's request: break out of HydraOS a **base system**, the BIOS ROM's kernel and its
system calls, the IRQ handling, the serial driver and a simple monitor (Woz's, as the V1.8C line had), as bare as it
can be, so that someone can write their own operating system on the Hydra-16's kernel.  It is the **same base
HydraOS 1.0 runs on**, not a copy: it moves to a new top-level folder, `/base`, and `/reborn` builds on it.  Task 0
(the kernel) and task F (the console's driver) stay as they are; task 1, init in HydraOS, is the monitor in the
base.

### **Contents**
1. [What's there now](#whats-there-now)
2. [The base](#the-base)
3. [Booting the base](#booting-the-base)
4. [Task F: the serial driver](#task-f-the-serial-driver)
5. [Task 1: the monitor](#task-1-the-monitor)
6. [The folders](#the-folders)
7. [The build](#the-build)
8. [The emulator and the tests](#the-emulator-and-the-tests)
9. [The documents](#the-documents)
10. [The order of work](#the-order-of-work)
11. [Risks](#risks)
12. [Questions](#questions)
13. [Answers](#answers-the-users-9-october-2026)
14. [As built](#as-built)

---

### **What's there now**

The kernel is already self-contained, and already the part the rest is built on:

| Part | Where | What it is |
| :--- | :---- | :--------- |
| **The kernel** | `reborn/kernel` (11,000 lines), BIOS ROM pages 0-4 | The reset and boot, POST, the IRQ entry and exit (the COMMON block), the scheduler and the tick, tasks, memory and banks, semaphores, shared segments, notes, the clock, files and the server protocol, namespaces, the loader (`.hyx`), the kernel's messages, the debugger's hooks; the polled bring-up console (`PUTC`, `GETC` with no fds open) |
| **The calls' spec** | `reborn/spec/api.def`, `errors.def` | The jump table (192 slots at `$F800`), the error codes: `tools/apigen.js` makes the kernel's table and the SDK's `hydra.inc` from them |
| **The drivers and init** | `reborn/modules`, listed in `modules/rom.txt` | The kernel starts each boot driver (`HF_BOOT`) from the paged ROM's module directory, from task F down (`cons` F, `storage` E, `kdev` D ...), then the directory's init as task 1.  Nothing in the kernel names a module |
| **The console** | `modules/cons` (17,000 lines, 35K: three banks) | Task F: the ACIA's interrupts and rings, and on them the windows, a VT100 for each, the chrome, the seats, the mouse, `/pc`'s frames, `/ser`, the snarf buffer |
| **The paged ROM** | `tools/romimg.js` | Bank 0 the module directory; bank 1 the old hardware test (`old/os_rom/bin/paged_rom_C02.bin`); then the modules; then the ROM disk |

So the kernel is already what the base wants: the only things to separate are the files, the build, a smaller
task F, and a monitor in task 1.  The boot (`kernel/reset.s`) needs no change: a different module list gives a
different system.

### **The base**

The base is:
* **The BIOS ROM, whole and unchanged**: all five kernel pages, all 192 system calls.  The files, namespaces and
  the loader are calls too, and an OS written on the base gets them (it can write its own file servers on the same
  protocol, or not use them).  `base/bin/bios.bin` *is* HydraOS's BIOS ROM, byte for byte: the build checks it.
* **Task F, the serial driver** (`ser`, below): the ACIA under interrupts, its rings, the console device `#c` with
  `cons` and `consctl`, Ctrl-C's note.
* **Task 1, the monitor** (`wozmon`, below).
* **The hardware test** in paged ROM bank 1, as now (POST starts it on a T at boot).
* **The asm SDK's core**: `hydra.inc` (the calls and errors), `hyx2.inc` and `hyx2.cfg` (a RAM program), `macros.inc`,
  `srvlib` (a driver's or server's library: what `ser` is built on).
* **The board's emulator and a test runner**, so the base builds and tests on its own (below).
* **Its documents**: the kernel's reference and how to build an OS on it.

Everything else is HydraOS's and stays in `/reborn`: the other drivers (storage, kdev, snd, gpio, vid, input), init,
the shells and languages, the tools, the ROM disk, the C SDK, the numbers and asm libraries, the window system.

### **Booting the base**

The same boot as HydraOS's, with `base/modules/rom.txt`:

```
init        wozmon
module      ser
module      wozmon
```

```
Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 ...
POST ok
RAM modules: 03
task F: ser
task 1: wozmon
\
```

One change to the kernel: its banner says the board and the kernel, not HydraOS (`HydraOS 1.0 for the Hydra-16`
today), since it's the base's too; HydraOS's init says `HydraOS 1.0` as it starts.  Tasks 2-E are free for the OS
written on it.

### **Task F: the serial driver**

This is the plan's one real design choice (question 1).  HydraOS's `cons` is task F now, but it is most of a window
system: three banks, the VT100s, the chrome, the seats, the mouse, `/pc`.  A bare base wants only the serial port.
Three ways:

| | Task F in the base | The same as HydraOS's? | Work |
| :- | :----------------- | :--------------------- | :--- |
| **A** | `cons`, as it is | Yes, the same module | Least: move nothing.  But the base isn't bare: 35K, windows, a bar at the top (it can be turned off) |
| **B** (recommended) | `ser`: a small driver, the serial layer of `cons` taken out into a file of its own (`base/modules/ser/serial.inc`) that **both** `ser` and `cons` include | The same code for the port (the ISR, the rings, the rates, the line, Ctrl-C), and the same interface (`#c/cons`, `consctl`'s `rawon` and `rawoff`, `/ser`, `/serctl`): a program written for the base runs on HydraOS unchanged | Most: `cons` changes (its serial layer becomes the shared file), its tests must stay green; `ser` is new, about 1,500 lines, one bank |
| **C** | None: the monitor on the kernel's polled console | No task F at all | None, but the input is polled, there's no Ctrl-C, and task F isn't "the same" |

`ser` (B) serves `#c` with one console: `cons` (a read a line, cooked: Backspace, Ctrl-U; or raw, each byte, with
`consctl`'s `rawon`), `consctl`, `ser` (raw bytes, as `cons`'s `/ser`) and `serctl` (the rate).  Ctrl-C and Ctrl-\
send their notes to the console's note group, as `cons` does.  No windows, no VT100 (the PC's terminal is the
screen), no `/pc`.  In HydraOS, `rom.txt` lists `cons` instead of `ser`, so task F is `cons`, built on the same
serial layer.  The base works with no task F too (C): the monitor falls back to the polled console, as init does
now, which helps bring up a new board.

### **Task 1: the monitor**

`wozmon`, the base's init: Woz's Apple 1 monitor, as the V1.8C line had it, but a program in task 1 on the kernel's
calls, not code in the BIOS ROM.  Its fds 0-2 are `#c/cons` (raw), or the polled console without a task F.

* **Woz's commands**: `XXXX` (a byte), `XXXX.YYYY` (a block), `XXXX: AA BB ...` (store), `XXXXR` (run: a `JSR`, and
  an `RTS` comes back to the prompt), several on a line; Backspace and Escape as V1.8C's had them; the prompt `\`.
* **What it sees**: task 1's own map, as any task's: its RAM `$0000-$7FFF` (the monitor's own below `$0400`, the rest
  free), its RAM bank at `$8000` (`$00` selects it), its paged ROM bank at `$A000` (`$01`), the BIOS ROM's page, the
  I/O.  So it can read the ROMs, poke the hardware, put a program in RAM and run it; a program that calls the kernel
  (`JSR` to the jump table) gets all of it, SPAWN of a module included.
* **Ctrl-C** (with `ser`): a program run with `R` that doesn't come back is stopped by its note, and the monitor's
  prompt comes back.

* **The disassembler** (V1.8C's `D` mode; question 3): `XXXXL` lists 20 instructions from `XXXX` (`L` again, or
  Enter on an empty line after one, the next 20), each `XXXX: AA BB CC  lda $1234,x`, as `as` writes them.  It is
  HydraOS's own disassembler, not a second one: `dis.inc` and `w65c02.inc` (the W65C02S's one table, 500 lines, no
  RAM of their own) move from `reborn/modules/asm` to `base/lib/asm`, and both `wozmon` and HydraOS's asm library
  (`as`, `dis`, `db`, HyForth's `disasm`) include them.

V1.8C's other extras (another task's memory, hex loads) aren't in it.

### **The folders**

```
base/
  README.md                the base: what it is, building it, running it, writing an OS on it
  build.js                 the BIOS ROM, the base's paged ROM, the SDK core (and, from Node, the steps reborn uses)
  bin/                     bios.bin, prom0.bin (the base's paged ROM: directory, hardware test, ser, wozmon)
  kernel/                  from reborn/kernel (git mv: its history follows)
  include/                 hw.inc, layout.inc
  spec/                    api.def, errors.def
  modules/                 rom.txt; ser/ (serial.inc, shared with reborn's cons), wozmon/
  lib/asm/                 dis.inc, w65c02.inc: the disassembler and the instruction table (from reborn/modules/asm;
                           wozmon's, and reborn's asm library's)
  sdk/asm/                 hydra.inc (made), hyx2.inc, hyx2.cfg, macros.inc, srvlib.*
  tools/                   apigen.js, romimg.js, check.js, budget.js
  hwtest/                  bank1.bin: the old hardware test's bank (16K, from old/os_rom's image)
  sim/                     the board's emulator (lib/, run.js, the test runner), and its PC tools that are the
                           board's (hydrafs.js stays HydraOS's)
  tests/                   the kernel's tests (t_irq, t_task, t_sem, t_mem, t_note, t_srv, t_scall ...), wozmon's
  docs/                    the kernel's guide and reference; writing a driver; writing an OS on it; the monitor
reborn/
  build.js                 the base's build first, then HydraOS's modules, the ROM disk, the C SDK ...
  modules/, romfs/, sdk/, programs/, forthlib/, tests/, docs/ ...   (HydraOS's, as now)
  sim/                     HydraOS's: test.js's tests, bench.js, web.js, the PC tool (hydrapc.js, pcfs.js: /pc
                           is cons's), hydrafs.js, hysong.js; the board's emulator from ../base/sim
```

`/reborn` *includes* the base the way it includes its own folders: `reborn/build.js` requires `../base/build.js`,
`ca65` gets `-I ../base/include -I ../base/sdk/asm`, `cons` includes `../../base/modules/ser/serial.inc`, the
tests and `run.js` require `../base/sim/lib`.  `reborn/bin/bios.bin` stays (the chips are programmed from one
folder), made by copying the base's, and the build fails if the two differ.

### **The build**

* `node base/build.js`: the kernel (`bin/bios.bin`), `ser` and `wozmon`, the base's paged ROM (`bin/prom0.bin`: one
  chip), the SDK core, the budgets.
* `node reborn/build.js`: the base's build (its steps, as a library), then everything HydraOS's as now, with
  `rom.txt` listing `cons` and the rest; its paged ROM is its own (`reborn/bin/prom0-3.bin`), its BIOS ROM the base's.
* The CI builds and tests both.

### **The emulator and the tests**

The emulator is the board's (the CPU, the T/U/V/W registers, the ACIA, the VIA, the YM2151, the DS1747, the Vera X,
the SD cards), so it moves to `base/sim` (question 2), and `reborn/sim` keeps what is HydraOS's.  `run.js` boots
either system's images (`--base`, or from the base's folder).

The tests split the same way.  The kernel's tests already run as init on their own (each a test module, `t_task`,
`t_sem` ...): they move to `base/tests` and run on the base's image (`ser`, not `cons`).  A test that needs a
HydraOS driver (`t_disk`, `t_spi`, `t_vid`, `t_rc` ...) stays in `reborn`.  New ones: the base boots (its lines, as
above); `ser` (cooked and raw reads, `/ser`, the rate, Ctrl-C); the monitor (examine, a block, store, run, a program
calling the kernel, Ctrl-C back to the prompt; with no task F, the polled console).  `node reborn/sim/test.js` runs
both sets (the base's first), so nothing is lost from today's 117.

### **The documents**

* `base/README.md` and `base/docs`: the kernel's part of the guide moves there (the boot, POST, the memory map, the
  IRQs, tasks, the calls: [the reimplementation document](../reimplementation-from-scratch.md)'s kernel sections),
  with two new ones: **writing an OS on the base** (what the kernel gives, the module directory and `rom.txt`, a boot
  driver, init, the ABI's promises) and **the monitor**.
* `reborn/docs/hydra-16.md`: a part on the base, and its parts 5-7 point there; the PDF has both.
* The repository's README: the two layers.

### **The order of work**

Each step leaves both builds, all the tests and the images as they were (but where the step says otherwise).

1. **The banner**: the kernel's says the board and the kernel; HydraOS's init says `HydraOS 1.0`.  (The BIOS ROM
   changes: the tests' expected lines too.)
2. **The move**: `git mv` the kernel, the includes, the spec, the kernel's tools and the SDK core into `base/`;
   `base/build.js` (the kernel's steps out of `reborn/build.js`), `reborn/build.js` on it.  The images unchanged,
   byte for byte.
3. **The emulator** to `base/sim` (if question 2 says so), and the test runner able to run a test set from either
   folder.
4. **The serial layer**: `cons`'s port code into `serial.inc`, included back; `cons` byte-different but its tests
   all passing.
5. **`ser`** on `serial.inc`, with its tests (on a test image: `ser` in task F, a test module as init).
6. **`wozmon`**, `base/modules/rom.txt`, `base/bin/prom0.bin`; its tests; the base boots on its own.  Then its
   disassembler: `dis.inc` and `w65c02.inc` to `base/lib/asm` (the asm library including them from there, its
   tests unchanged), and `L`.
7. **The kernel's tests** to `base/tests`, on the base's image.
8. **The documents**, the CI, the PDF.

Steps 2 and 3 move many files, so they come once the other sessions' outstanding work is merged (the answer to
question 4): nobody merges across the renames.

### **Risks**

* **The move breaks paths**: every `../` in the build, the tests, the docs' links, `sim/web.js`, `run.js`'s labels
  (`obj/kernel/bios.dbg`), the danlang emulator, the CI.  Step 2 ends with both images byte for byte the same and
  every test passing, so a broken path shows.
* **Branches in flight** (question 4): a session working in `reborn/kernel` or `reborn/sim` meanwhile has to merge
  across the move.
* **`cons`'s serial layer** isn't a separate part today: its ISR and rings are shared with `/pc`'s frames and the
  seats' keys.  Step 4 may find that the honest seam is lower (the ISR and rings only), which still makes `ser` and
  `cons` share the port's code.
* **The base's `#c`** must stay a subset of `cons`'s, so a program written for the base runs on HydraOS: the tests
  for `ser` run against `cons` too.

### **Questions**

1. **Task F in the base**: `ser`, a small driver sharing its serial layer with `cons` (B, recommended); `cons` as it
   is (A); or none, the polled console (C)?
2. **The emulator**: move the board's emulator to `base/sim`, so the base builds and tests on its own (recommended);
   or leave it in `reborn/sim` and test the base from there?
3. **The monitor's extras**: Woz's commands only (recommended, as bare as can be); or some of V1.8C's (a
   disassembler, examining another task's memory, loading Intel hex or S-records over the serial port)?
4. **When**: the move touches the kernel's and the emulator's paths for every branch; do it now (the Numbers session's
   `reborn-numspeed` and any others merged or told first), or after the branches in flight land?

### **Answers (the user's, 9 October 2026)**

1. **Task F**: `ser`, a small driver, its serial layer shared with `cons` (B).
2. **The emulator**: to `base/sim`; the base builds and tests on its own.
3. **The monitor**: Woz's commands and the disassembler (`L`, on HydraOS's own `dis.inc` and `w65c02.inc`, which
   move to `base/lib/asm`).  No hex loads, no other task's memory.
4. **When**: the other sessions' outstanding merges first, then the move.

### **As built**

Steps 1-8 (branch `reborn-base`):

1. **The banner** (9bf1bd6): the kernel's is `Hydra-16: kernel 0.1, ABI 1`; HydraOS's init says `HydraOS 1.0 for the Hydra-16`.
2. **The move** (a00c428): `git mv` of the kernel, `include/`, `spec/api.def` and `errors.def`, the SDK's core, the
   modules' links and `check.js`, `budget.js`, `romimg.js`; `apigen.js` split (the base's: the jump table, the
   errors, `hydra.inc`, `api.json`; HydraOS's: the languages' bindings, the reference, the libraries');
   `romimg.js` takes the ROM disk's builder as `romfsLib` (HydraOS's `tools/romimg.js` gives it).  Every image byte
   for byte as before.
3. **The emulator** (4dabbbe): `sim/lib`'s devices, `run.js` and `view.js` to `base/sim`; `run.js` any system's
   (`opt.root`, `main(argv, { root, createPcHost })`); HydraOS's `run.js` gives its folder and `/pc`'s host.  The
   danlang emulator stays HydraOS's.
4. **The serial layer** (2304686): `base/lib/serial.inc` (macros: the rates, timer 2's pacing, the send ring, `rx_get`,
   a rate set, the ACIA on); `cons` built on it, byte for byte the same.  (The receive side's irq entry is each
   driver's own: `cons`'s has the windows' prefix, `/ser` and `/pc` in it.)
5. **`ser`** (78da141): 3.2K, a bank; HydraOS's init, HyForth and rc run on it in `cons`'s place (the test `ser`).
6. **`wozmon`** (8e01c18): Woz's commands; `L` and `K` switch the examines to instructions and back, as V1.8C's
   monitor did (not `XXXXL`); `R` runs the last address shown; a note's handler sets the task's frame's PC to the
   monitor's restart, so Ctrl-C (or a `BRK`) brings back the prompt.  `dis.inc` and `w65c02.inc` are in
   `base/lib` (not `base/lib/asm`), with defaults for the flags HydraOS's `asmlib.inc` defines.  The base's
   `rom.txt` and `bin/prom0.bin`.
7. **The tests** (cba35cf, 9b10ba1): the runner in `base/sim/test.js` (`setup`: a system's image, PC folders, danlang
   run, build); the base's tests (`base`, `wozmon`, `wozpolled`).  **`kdev` is the base's** (the user's choice,
   9 October: without it the base's `SPAWN` of a module, pipes and `/proc` have no server), task E; with it, 12 of
   the kernel's tests run on the base's image (`note`, `file`, `ns`, `proc`, `env`, `kmesg`, `mem`, `sem`,
   `xcall`, `banks`, `banks3`, `kcopy`, their modules and `testlib.inc` in `base/tests/mod`).  125 tests: the
   base's 15, HydraOS's 110.
8. **The documents**: `base/README.md`, `base/docs/monitor.md`, `base/docs/os.md`; the guide's parts 2, 4, 17 and
   18; the moved paths in the documents in use; the repository's README; the CI builds and tests the base first.

Not done: the kernel's own parts of HydraOS's documents (the conventions, the programmer's guide) stay in
`reborn/docs`, the base's documents pointing there; the danlang emulator stays HydraOS's.
