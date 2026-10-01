## **Hydra 16: Ideas and future work**

Ideas worth coming back to, with the reasoning so far.  Plans that are being built live in `MMU_PLAN.md` and `IO_PLAN.md`.

### **Next features, in order**
1. **A battery-backed clock: a DS1747 in U7.**  *(Done: the reserved bytes, the boot's probe and report, `/dev/time` with it, the emulator's `--rtc`, and tests.  Still to do: a hardware test of the chip, and a regular reload while the system runs, not only on a `/dev/time` read.)*  The DS1747 (the 5 V part; the DS1747W is 3.3 V) is a 512K RAM with a clock in its top 8 bytes, pin compatible with the HM628512 task RAM.  In U7 its registers (`$7FFF8-$7FFFF`) are **task F's `$7FF8-$7FFF`**: `T0-T3` come from U48 (74F573) through U21 (74F541, `T_M0-T_M3`) to U7's A15-A18 in order, with nothing inverted or swapped.  What it needs:
   * Keep the ROM off those 8 bytes, in every task: the MMU area's clear (`MM_TASK_INIT`) and its handle table (103 handles become 101), and the hardware test's task RAM test.  Any stray write there could stop the oscillator or change the time.
   * At boot: if its seconds count along with the tick (a plain HM628512 there doesn't), load `ZP_CLOCK` from it.  Setting the time (`/dev/time`) writes it too (the century, the day of the week, and OSC cleared: parts often come with the oscillator stopped).  Load it again now and then, so the tick clock doesn't drift.  Warn at boot when its battery flag says the battery is low.
   * Each access with interrupts off, like `CLOCK_GET` (`T` = F, the R or W bit, the registers, `T` back), so it needs no lock.
   * The emulator: a DS1747 in task F's RAM (`--rtc`), and regression tests.  The hardware test: is it counting, and is its battery good.
2. **Semaphores.**  *(Done: `kernel/sem.s`, thunks `$F8D8-$F8E4`, HyForth's `sem`, `mutex`, `acquire`, `acquire?`, `release`, `-sem`; [tasks.md](../programming/tasks.md#semaphores).  The kernel's waits already share one mechanism, a wait mask; semaphores use it too.)*  Nothing lets programs share memory safely today (`NO_PREEMPT` stops every task).  Kernel calls on a small table (16): `SEM_NEW` (a count; 1 = a mutex), `SEM_ACQUIRE` (waits, using no CPU), `SEM_TRY`, `SEM_RELEASE`, `SEM_FREE`.  Each has a count, a mask of the tasks waiting (woken with `TASK_WAKE_MASK`) and its holder.  The check and the wait happen with interrupts off, so no wakeup is lost.  A break or kill ends a wait with an error, and a task's end releases what it holds and frees what it made.  HyForth words for them.  Later, maybe named ones as files (`/dev/sem/NAME`).
3. **A sound library for the YM2151**, and a test song that uses all of the chip: its 8 voices and 4 operators each, the algorithms and feedback, LFO (vibrato and tremolo), noise, stereo, and the timers.  A semaphore (or one per voice) for sharing the chip between tasks: the bell, `/dev/snd` and a tune can't cut into each other's register writes.  *(The library is done: `os_rom/sound/` on page B, `/dev/snd`'s commands, volumes, the X16's General MIDI patches, and channels claimed by an fd instead of semaphores; HyForth's `patch`, `note`, `noteoff`; C's `snd.h`.  The song player too: ZSM files, `play`, a song by its name, C's `snd_play`; and the test song in the ROM (`sndtest`), written in a score language (`sim/tools/hysong.js`).  The chip's timer B times the player (the sound clock).  Still to do: importing music from other machines: [SOUND.md](SOUND.md).)*
4. **C programs:** a cc65 target (start-up code, and a library over the `$F8xx` calls and `MM_ALLOC`).  *(Done: `programs/c/`, [programs.md](../programming/programs.md#c-programs): stdio and the file calls, `stat`/`fstat`, `dirent.h`, the environment (`getenv`, `setenv`, `putenv`, `unsetenv` on `/env`), `conio` (ANSI, raw keys through `/dev/cons/ctl`), `system`/`hy_spawn`/`hy_wait` (a command shell, `SHELL_CMD`), exit statuses, `argv[0]` (the loader's `HYX_NAME`), the header's BSS and top, `clock`, `isatty`, semaphores.  Still to do: `rename` across directories; `MM_ALLOC` from C.)*
5. **`/rom`: programs and libraries in the paged ROM.**  A read-only file server over the paged ROM banks (4 MB, mostly free), made from a directory by a PC tool; `PATH` and `LIBPATH` fall back to `/rom/bin` and `/rom/lib`.  So programs, scripts and libraries run without a card.  *(Done, and since redone as the ROM disk: the paged ROM is the disk `/sd/x`, a read-only HydraFS volume bound at `/rom` ([DISKS.md](DISKS.md#the-rom-disk)), made by `sim/tools/mkromdisk.js` from `os_rom/romfs.txt`, the C samples and a song in it; [io.md](../programming/io.md#the-roms-files-rom).  Still to do: libraries for `/rom/lib`.)*
6. **XMODEM** send and receive, to move files over the serial port without taking the card out.  Its checksums and retries also get past the bad characters at 115200.
7. **Exit status** for programs and scripts (through `TASK_EXIT`), and a word to read it, so a script can check a step.  *(Done, as Plan 9's `exits`: a code and a message, `TASK_EXITS` and `TASK_JOIN` (`kernel/exits.s`), HyForth's `status` and `exits`, `/env/status`; [tasks.md](../programming/tasks.md#exit-statuses).)*
8. **Running a program in the background** (`&`).  *(Done: `[B]`, `/env/apid`, HyForth's `wait`; [hyforth.md](../using/hyforth.md#background-tasks-and-exit-statuses).  Still to do: a note when a background task ends, and `wait` with no number for all of them.)*
9. **Notes with handlers** (Plan 9's `notify`), as `IO_PLAN.md` step 10 has it.
10. **Tools as programs:** paging output, `head`, `grep`, a hex dump of a file, copying a directory.

### **Slow devices on a faster CPU clock (board V2)**
The CPU runs at 3.58 MHz; the board can also run it at 7.16 MHz, and the W65C02S goes to 14 MHz.  Some devices can't keep up with a faster bus: the YM2151 runs on its own 3.58 MHz clock, and slower 65C51/65C22 grades and ROMs have similar limits.  Until there's hardware for this, the CPU clock is a build-time setting (`CPU_CLOCK_MULT` in `os_rom/include/hw.inc`), and above 3.58 MHz the sound chip mustn't be used.

#### **Planned: RDY wait states**
The more versatile option, and the plan for board V2 (possibly earlier on an expansion card).
* A **wait table** with an entry per I/O port or memory area.  On an access, the entry for the port/area is copied into a **RDY hold counter**; RDY is held low while it counts down, and released at zero.
* A new pseudo-register, **`$FFF9`**, is the interaction point: the value the RDY hold counter loads from.  It's **initialised to 15 at boot** (the maximum wait), so everything is slow and safe until the ROM sets the table up.
* A clock jumper register to read the CPU clock from, replacing the build-time setting.
* RDY has to be open-drain with a pull-up: the W65C02S drives it low itself during `WAI` (the ROM uses `WAI`).
* Transparent to software: every access to a slow port or memory area gets its waits, including instruction fetches from a slow ROM, and existing code needs no changes.
* OS support: set the table first thing at reset (before the self test touches the ACIA); per-device defaults; drivers declare their port's wait states (a `DriverInfo` field that `DRV_START` programs) and an `IO_WAIT_SET` call; the self test reports the clock and the table.

#### **Alternative: switch the clock divider**
Set the CPU clock divider through `$FFF9`: a driver writes a slower divider before touching its device, and clears it (back to the system default) when done.
* Reuses the board's existing divider chain (74F191s): no wait table or RDY counter.
* Needs **glitch-free switching** (only on an edge where the fast and slow clocks line up), so the change takes effect a cycle or two after the write: a driver needs a `nop` or a read-back before touching the device.  `$FFF9` should be readable (to save/restore it, and to report the jumper setting), with "the jumper speed" as the reset default.
* Slows **everything** while set: other tasks, IRQ handlers, and the VIA timer (which counts PHI2), so the scheduler tick stretches.  A slow section can't be preempted (the next task would inherit the slow clock): run it with IRQs off, and/or save the divider in each task's frame (like `W` and `U`).
* Only protects code that switches explicitly: the ROM (and anything executed) must be fast enough for the full clock.

### **Serial output errors at 115200 (open)**
Long output at 115200 (a WOZMON dump of the whole BIOS ROM, about 32,000 characters) gets a few bad characters on the board: about 20 in 160,000.  Nothing is lost or reordered, and in each bad character bit 7, the last data bit, arrives as 1: the receiver reads the start of the stop bit, as if the character's end came early.  19200 is clean.

What's been ruled out:
* **The ROM's data:** the emulator's output at 115200 matches the ROM exactly, and so do all the other characters on the board.
* **The rate:** "115200" is really 111,861 baud (the ACIA's 1.79 MHz clock / 16), but a terminal set to exactly 111861 (an FTDI cable, which can do it) gets the same errors.
* **The ACIA:** a logic analyzer on its TxD (U3 pin 10) shows every bit 8.94-8.96 us, including the last data bit before the stop bit.
* **The MAX232's supplies and wiring:** V+ about +9 V, V- about -8.4 V; U5 pin 14 to DE-9 pin 2 and DE-9 pin 5 to ground about 1 ohm.

What helps: sent back to back (as the ROM first did), one bad bit also threw the terminal off the character boundaries, and whole lines were garbled.  So at 115200 the ROM paces sending with VIA timer 2, with idle bits after each character (`SER_PACE_GAP` in `os_rom/include/hw.inc`, 2: `SER_PACED`, `servers/serfast.s`): the terminal finds the next start bit, and an error stays one character.  It costs about a third of the rate (about 7,000 characters a second).

What's left: the MAX232's output stage (U5), or the cable's RS-232 receiver.  Next tests:
* The cable alone: join pins 2-3 (and 7-8) at its DE-9, send a big file from the terminal at 115200, and compare what comes back.
* The Hydra alone: a loopback plug on its DE-9 (2-3, 7-8) and a hardware test that sends a few thousand bytes at 115200 and reads them back through its own receiver (not written yet).
* A scope on DE-9 pin 2 (the edges' times and levels), or a MAX232A / MAX3232 in U5.
Once it's fixed, try `SER_PACE_GAP` at 0 (the full 11,000 characters a second).
