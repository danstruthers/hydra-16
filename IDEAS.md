## **Hydra 16: Ideas and future work**

Ideas worth coming back to, with the reasoning so far.  Plans that are being built live in `os_rom/MMU_PLAN.md` and `os_rom/IO_PLAN.md`.

### **Slow devices on a faster CPU clock (board V2)**
The CPU runs at 3.58 MHz; the board can also run it at 7.16 MHz, and the W65C02S goes to 14 MHz.  Some devices can't keep up with a faster bus: the YM2151 runs on its own 3.58 MHz clock, and slower 65C51/65C22 grades and ROMs have similar limits.  Until there's hardware for this, the CPU clock is a build-time setting (`CPU_CLOCK_MULT` in `os_rom/defines.s`), and above 3.58 MHz the sound chip mustn't be used.

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
