## **Hydra 16: Ideas and future work**

Ideas worth coming back to, with the reasoning so far.  Plans that are being built live in `MMU_PLAN.md` and `IO_PLAN.md`.

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

What helps: sent back to back (as the ROM first did), one bad bit also threw the terminal off the character boundaries, and whole lines were garbled.  So at 115200 the ROM paces sending with VIA timer 2, with idle bits after each character (`SER_PACE_GAP` in `os_rom/include/hw.inc`, 2: `SER_PACED`, `io/serfast.s`): the terminal finds the next start bit, and an error stays one character.  It costs about a third of the rate (about 7,000 characters a second).

What's left: the MAX232's output stage (U5), or the cable's RS-232 receiver.  Next tests:
* The cable alone: join pins 2-3 (and 7-8) at its DE-9, send a big file from the terminal at 115200, and compare what comes back.
* The Hydra alone: a loopback plug on its DE-9 (2-3, 7-8) and a hardware test that sends a few thousand bytes at 115200 and reads them back through its own receiver (not written yet).
* A scope on DE-9 pin 2 (the edges' times and levels), or a MAX232A / MAX3232 in U5.
Once it's fixed, try `SER_PACE_GAP` at 0 (the full 11,000 characters a second).
