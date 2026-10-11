# The Hydra-16 book

The Hydra-16 is a multitasking computer built around the 65C02, and HydraOS is its operating system.  This book is the
whole of it, in the order you'd meet it: what the machine is, the board and how to build it, the kernel the board runs,
the drivers, the shell, the languages, the tools, the emulator that runs it all on a PC, and finally how to write
programs for it.  Each chapter builds on the ones before it, and each starts with the big picture before the detail, so
you can read as far as you need and stop.

If you're new, read [chapter 1](01-overview.md) first: it explains the ideas the rest of the book takes for granted, and
ends with an hour's tour of the running system, in the emulator, with no hardware needed.

## Contents

1. **[The Hydra-16 at a glance](01-overview.md)**: what it is, the ideas behind it (sixteen tasks in hardware, banked
   memory, paged ROM, prioritised interrupts), the software in layers, and a first hour with the system.
2. **[The hardware](02-hardware/README.md)**: the board, from the outside in.
   1. [The architecture](02-hardware/architecture.md): the buses, the memory map, the pseudo-registers, banking, the
      ROMs, I/O, interrupts, clocks, reset.
   2. [The devices](02-hardware/devices.md): the VIA (GPIO, I2C, SPI, the tick), the ACIA (the serial port), the YM2151
      and the audio path, the clock chip, the expansion slots.
   3. [The parts](02-hardware/parts.md): every part on every board, generated from the schematics, with what each does
      and what to buy.
   4. [Building it](02-hardware/building.md): assembly, power, jumpers, programming the ROMs, the first power-up, POST,
      the hardware test, and what to do when something's wrong.
   5. [The board layout](02-hardware/layout.md): where everything is, the connectors and headers and their pins.
   6. [The quirks](02-hardware/quirks.md): what the board does that you wouldn't expect, and what changed from 1.8C to
      1.8D.
   7. [The cards](02-hardware/cards.md): the memory daughter card, the bus breakout card, and the Vera X video card.
3. **[The base: the kernel and the system calls](03-base/README.md)**: what every system on the Hydra shares.
   1. [The boot](03-base/boot.md): reset, POST, the drivers, init.
   2. [The kernel](03-base/kernel.md): tasks and the scheduler, interrupts, memory and banks, shared segments,
      semaphores, notes, the clock.
   3. [Files, servers and namespaces](03-base/files.md): the file model every device is part of.
   4. [The system calls](03-base/calls.md): how a program calls the kernel, and what it can ask for.
   5. [The monitor](03-base/monitor.md): Woz's monitor, the base's own init.
4. **[The drivers](04-drivers/README.md)**: the devices as files.
   1. [The serial console](04-drivers/serial.md): `ser` and `cons`, the windows, the PC tool's `/pc`.
   2. [The kernel's devices](04-drivers/kdev.md): `kdev`: the modules, `/proc`, pipes, the environment.
   3. [Storage](04-drivers/storage.md): SD cards, the RAM disks, the ROM disk, HydraFS.
   4. [Sound](04-drivers/sound.md): the YM2151, the Vera X's PSG and PCM, `/dev/snd`.
   5. [GPIO, I2C, SPI and the clock](04-drivers/gpio.md).
   6. [Video](04-drivers/video.md): the Vera X: `vid`, its screen, graphics and sprites, and `input`, its keyboard and
      mouse.
5. **[The shell: rc](05-rc.md)**: commands, pipes, variables, scripts, namespaces and windows.
6. **[The languages](06-languages/README.md)**: [HyForth](06-languages/hyforth.md), [hylang](06-languages/hylang.md)
   and [HyBASIC](06-languages/hybasic.md), and the numbers they share.
7. **[The tools](07-tools.md)**: the core tools, `edit` and `ed`, `as` and `dis`, `db`, `play`, `xmodem`, `calc`.
8. **[The emulator and the PC tools](08-emulator.md)**: running the Hydra on a PC, in a terminal or a browser; the
   tests; the PC tool, card images and writing a real card.
9. **[The programming guide](09-programming/README.md)**: programs of your own.
   1. [Assembly](09-programming/assembly.md): the assembly SDK, modules and RAM programs.
   2. [C](09-programming/c.md): the C SDK, cc65, the library, how C reaches the system calls.
   3. [Servers and drivers](09-programming/servers.md): serving files, owning interrupts.
   4. [Libraries](09-programming/libraries.md): the number and asm libraries, `XCALL`.
   5. [The system call reference](09-programming/calls-reference.md): every call, generated from the specification.

The design notes, the plans each part was built from, and the project's status are in [design/](design/README.md):
they're history, and not needed to use the system.

## How to read it

* **Names in `code`** are what you type, a file's path, or a name in the source: `ls /rom`, `base/kernel/sched.s`,
  `SPAWN`.  Paths without a folder at the front are in the repository: `base/` is the base, `reborn/` HydraOS,
  `board/` the KiCad files, `docs/` this book.
* **Numbers with `$`** are hexadecimal, as the 6502's assemblers write them: `$FF00` is 65280.
* **References like U7 or J18** are the schematic's: a chip (U, IC), a connector or jumper (J), a resistor (R) or
  capacitor (C).  The [board layout](02-hardware/layout.md) shows where each one is.
* **Prompts** show where a line is typed: `/>` HyForth (the login shell), `%` rc, `hylang>` hylang, `>` HyBASIC,
  `T1 00>` the monitor, and `$` a shell on the PC.

## Printing it

[hydra-16.pdf](hydra-16.pdf) is the whole book as one PDF, with its contents as bookmarks: `node docs/build/docpdf.js`
makes it again (it needs Edge or Chrome).  The parts list is made from the schematics by `node docs/build/bom.js`, and
the call reference from the specification by `node docs/build/callref.js`: both run whenever the board or the calls
change, and the repository's checks fail if they haven't.
