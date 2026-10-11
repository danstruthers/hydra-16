# An operating system of your own

The base gives an operating system on the Hydra-16 its hardest parts: the kernel, its calls, the interrupt path, a
console driver and the kernel's devices.  So a new system is mostly modules: what runs at boot, and what the user sees.
HydraOS is one such system (`reborn/`: its drivers, shells, languages and tools are all modules on this kernel); this
page is how to make another.

## Contents

1. [What the kernel gives](#what-the-kernel-gives)
2. [What boots](#what-boots)
3. [A module](#a-module)
4. [Building it](#building-it)
5. [What else the base has](#what-else-the-base-has)

---

## What the kernel gives

The kernel is the BIOS ROM (`base/bin/bios.bin`), the same for every system, with its 89 calls ([the system
calls](../03-base/calls.md)): tasks and the scheduler, memory and banks, shared segments, semaphores, notes, the clock,
files and the server protocol, namespaces, the loader.  Your system uses them as HydraOS does; it never needs to change
the kernel.

---

## What boots

The kernel boots, runs POST, then starts what the paged ROM's **module directory** says ([the
boot](../03-base/boot.md)):

1. **The boot drivers**: each module of type driver with the flag `HF_BOOT`, in the directory's order, the first in task
   F, the next in E, and down.  Each runs its `init` (it registers its device letters, `SRV_REGISTER`, and owns its
   interrupt lines, `IRQ_OWN`); the boot waits for them all.
2. **Init**: the directory's init entry, a program, in task 1, with an empty namespace of its own and no fds: it builds
   them.  **Init is the system.**

The base's `rom.txt` is `ser` (task F), `kdev` (task E) and `wozmon` (init).  Yours keeps `ser` and `kdev` (or your own
console driver in their place), and names your init:

```
init        myinit
module      ser
module      kdev
module      myinit
module      ...         ; your programs and drivers
```

---

## A module

A module is a program, a driver or a library in the paged ROM, run in place in its own banks (one to eight), with a
48-byte header first (`base/sdk/asm/hyx2.inc`):

```
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "myinit", main            ; a program: main gets r0 = its arguments; rts ends it

.code
main:
            LDR         r0, s_cons                 ; fds 0-2: the console (ser's #c)
            lda         #O_RDWR
            jsr         OPEN
            ...
```

* **A program** (`HYX2_PROGRAM name, main`) runs in a task of its own, its RAM from `$0400` (its data and BSS) up to its
  break, its zero page `$22-$7F`.  `SPAWN "#m/NAME"` starts one (kdev's `#m` serves the directory's modules as files;
  bound at `/bin`, `SPAWN "/bin/NAME"`).
* **A driver** (`HYX2_DRIVER name, init, serve, irq, stop, flags`) serves a device: its files, by the kernel's server
  protocol, through srvlib ([servers and drivers](servers.md)).  Its `irq` entry gets the lines it owns.
* **A library** (`HYX2_LIBRARY`) is code another task calls in its own task with `XCALL` ([libraries](libraries.md)).

Init's first jobs are the console (open `#c/cons` three times, for fds 0, 1 and 2, and write `group` to `#c/consctl` so
Ctrl-C reaches it), and the namespace: bind what your programs will look for (`/dev`, `/bin`, `/env`, `/proc`).  The
SDK's `nslib.s` builds a namespace from a file of `bind` and `mount` lines, as HydraOS's init does.

---

## Building it

The quickest way is the base's own build: put a module in a folder, `base/modules/NAME/NAME.s`, and `node build.js` in
`base/` assembles it with the base's includes (`sdk/asm`, `include`, `obj/gen`, `lib`) and links it with
`modules/module.cfg` (`moduleN.cfg` for N banks); then name it in `rom.txt`.

For a system of your own beside the base, do what HydraOS's `reborn/build.js` does:

* `require('../base/build.js')`: its `build()` for the kernel and the base's modules, its `buildModule()` for yours;
* its `tools/romimg.js` for your paged ROM (the module directory, the hardware test in bank 1, your modules);
* `sim/run.js`'s `main(argv, { root })` and `sim/test.js`'s `setup({ root, tests, image })` run your tree's images and
  tests, as HydraOS's `sim/run.js` and `sim/test.js` do.

---

## What else the base has

* **The kernel's devices** (`kdev`): the modules, `null` and the kernel's messages, the tasks, pipes, the environment,
  shared segments' names, the clock, raw RAM ([the kernel's devices](../04-drivers/kdev.md)).
* **The console** (`ser`): `#c/cons` (a line at a time, cooked; or raw with `#c/consctl`'s `rawon`), `#c/ser` and
  `#c/serctl` (the serial port raw, its rate).  HydraOS's console (`cons`) serves the same files and more, on the same
  serial layer (`base/lib/serial.inc`): write your own on it, or keep `ser` ([the serial console](../04-drivers/serial.md)).
* **The monitor** (`wozmon`): to poke at the machine while you bring your system up ([the monitor](../03-base/monitor.md)).
* **The emulator**: the board, cycle by cycle, with a monitor of its own, the system calls traced by name, breaks on a
  module's labels ([the emulator](../08-emulator.md)).
