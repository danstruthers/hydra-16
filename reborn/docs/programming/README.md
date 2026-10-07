# The programmer's guide

How to write programs for the rebuilt Hydra-16 (`reborn/`): what the system gives a program, how it's called, and how
the parts fit.  It's for someone who knows 65C02 assembly or C and wants to write for this system; the SDKs' guides
([../../sdk/asm/README.md](../../sdk/asm/README.md), [../../sdk/c/README.md](../../sdk/c/README.md)) cover building
and running, and `/rom/doc/api.md` on the Hydra (the build's `obj/gen/api.md`, made from `spec/api.def`) is every
call's reference.  The rules every part of the system keeps are [../conventions.md](../conventions.md); this guide
is the gentler way in.

| Chapter | What it covers |
| :------ | :------------- |
| [The system in one page](#the-system-in-one-page) | Below: the parts, and where a program fits |
| [calls.md](calls.md) | Calling the system: the jump table, registers, errors, the SDK's macros |
| [memory.md](memory.md) | A task's 64K, its break and pages, its banks, shared segments, semaphores |
| [tasks.md](tasks.md) | Starting and ending programs, waiting, notes (signals), time, `/proc` |
| [files.md](files.md) | Files and directories, the namespace, devices, the environment |
| [servers.md](servers.md) | Writing a server or a driver: srvlib, requests, waiting, interrupts |
| [modules.md](modules.md) | The HYX2 format, programs in RAM and in the ROM, modules of several banks, libraries and `XCALL` |
| [video.md](video.md) | The screen, the Vera X: `/dev/vid`'s files, the console's terminal, claiming the chip, VRAM, the registers |

## The system in one page

**The hardware a program sees.**  A 65C02 at 3.58 MHz (7.16 with a jumper).  Sixteen tasks, each with its own
`$0000-$7FFF` (zero page, stack and 32K of RAM) and its own bank registers: `$00` picks one of its RAM banks for
`$8000-$9FFF`, `$01` one of the paged ROM's banks for `$A000-$DFFF`.  `$E000-$FFFF` is the BIOS ROM, where the kernel
and the jump table live; the I/O area is `$FF00-$FFEF`, and belongs to the drivers.

**The kernel** is in the BIOS ROM, a task of its own (task 0): the scheduler (round robin, preemptive, 200 ticks a
second), the IRQ path, calls between tasks, memory, notes, files' channels and namespaces.  Its state is in task 0's
RAM, out of other tasks' reach.

**Modules** are everything else, in the paged ROM, each running in place in a task of its own: the drivers (`cons`
the console and `/pc`, `storage` the SPI bus and every disk, `kdev` the kernel's devices, `snd` the YM2151, `gpio`
the VIA's port A and I2C, `vid` the Vera X's screen), init, the shells and the tools.  A program in a file (on a
card, a RAM disk, `/pc`) is read into a task's RAM at `$0800` and runs there.

**Files**, Plan 9's way: every device is a file server, a tree of files; a program opens, reads and writes them with
the same calls whatever's behind them, and controls a device by writing commands to its `ctl` file.  Each task has a
namespace, the tree it sees, built from binds and mounts (`/rom/lib/namespace`), shared with its children until one
changes it.

**The console** is windows on the serial terminal, and on the Vera X's screen if there's one: each a whole console
with its own `cons`, line editor and shell.
The login shell is HyForth over rc; rc is the shell of scripts and `system()`.

```
 task 0  kernel      the scheduler, IRQs, calls, memory, notes, channels, namespaces
 task 1  init        the namespace, the RAM disks, window 0's shell, wstart
 task 2+ programs    the shells, the tools, yours: in place from the paged ROM, or in RAM from $0800
 task A  vid         #v  /dev/vid: the Vera X (with no card it ends as it starts)
 task B  gpio        #g  /dev/gpio, #i  /dev/i2c
 task C  snd         #a  /dev/snd, sndctl, bell
 task D  kdev        #/ #n #t #m #p #| #e #s #r: the root, null, ticks, modules, /proc, pipes, /env, segments
 task E  storage     #S  /dev/spi, #d  /dev/sd, #f  HydraFS: /rom, /ram, /sram, /sd/N
 task F  cons        #c  /dev/cons ..., #P  /pc
```

## A first program

`sdk/asm/samples/hi/hi.s` greets its arguments and says where it runs: its arguments come at `r0`, it prints with
`PUTS` and `PUTC` (fd 1), asks the kernel its task (`GETPID`) and its directory (`GETCWD`), and reads its environment
(`ENV_GET`); returning from `main` ends it with code 0.  [calls.md](calls.md) walks through it.  Build it with `node
build.js prog DIR` (the folder of its `.s` files), and run it from a card, a RAM disk or `/pc`: [../tutorial.md](../tutorial.md),
step 8.
