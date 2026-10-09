## **Writing an OS on the base**

The base gives an operating system on the Hydra-16 its hardest parts, so a new one is mostly modules: what runs at
boot, and what the user sees.  HydraOS is one such system (`../../reborn`: its drivers, shells, languages and tools are
all modules on this kernel); this page is how to make another.

### **What the kernel gives**

The kernel is the BIOS ROM (`bin/bios.bin`), the same for every system.  A program calls it with `jsr` to a slot of the
jump table at `$F800` (the addresses are `hydra.inc`'s, made from `spec/api.def` by the build: `sdk/asm/hydra.inc`, in Git):
arguments in `.A`, `.X`, `.Y` and the call registers `r0`-`r15` (`$02-$21`), `C = 0` for success, `C = 1` with an error
code in `.A`.  The groups:

| Group | What's there |
| :---- | :----------- |
| system | `SYSINFO` (the kernel's version, the machine), `ERRSTR` (an error's text), `KMESG` (the kernel's messages), `REBOOT`, `XCALL` (a library in another bank) |
| task | `SPAWN`, `EXITS`, `WAIT`, `GETPID`, `GETPPID`, `SLEEP`, `SLEEP_UNTIL`, `YIELD`, `PAUSE`, notes (`NOTE`, `NOTIFY`), semaphores (`SEM_NEW`, `SEM_ACQUIRE` ...), the ticks, task information |
| memory | The break, RAM banks, shared segments (`SEG_CREATE` ...), copying between tasks |
| file | `OPEN`, `CREATE`, `READ`, `WRITE`, `CLOSE`, `SEEK`, `STAT`, `DUP`, `PIPE`, `REMOVE` ... on any server's files |
| name | Namespaces: `BIND`, `MOUNT`, `UNMOUNT`, `CHDIR`, the environment |
| cons | `PUTC`, `PUTS`, `GETC`: fd 1 and fd 0, or the serial port polled when they aren't open |
| time | The clock |
| server | `SRV_REGISTER`, `SRV_TAKE`, `SRV_REPLY`, `CLIENT_READ`, `CLIENT_WRITE`, `IRQ_OWN`, `NOTE_QUEUE`, `WAKE`: what a driver is made of |
| dbg | The debugger's calls (unstable) |

Every call, its registers and its errors: [calling the system](../../reborn/docs/programming/calls.md), and the
reference the build makes (`../../reborn/obj/gen/api.md`, on HydraOS's ROM disk as `/rom/doc/api.md`).  The
conventions (zero page, banks, the pages of the BIOS ROM): [conventions](../../reborn/docs/conventions.md).

### **What boots**

The kernel boots, runs POST, then starts what the paged ROM's **module directory** says (bank 0, written by
`tools/romimg.js` from `modules/rom.txt`):

1. **The boot drivers**: each module of type driver with the flag `HF_BOOT`, in the directory's order, the first in task
   F, the next in E, and down.  Each runs its init (it registers its device letters, `SRV_REGISTER`, and owns its IRQ
   lines, `IRQ_OWN`); the boot waits for them all.
2. **Init**: the directory's init entry (`rom.txt`'s `init NAME`), a program, in task 1, with an empty namespace of its
   own and no fds: it builds them.  Init is the system.

The base's `rom.txt` is `ser` (task F), `kdev` (task E) and `wozmon` (init).  Yours keeps `ser` and `kdev` (or your own
console driver in their place), and names your init:

```
init        myinit
module      ser
module      kdev
module      myinit
module      ...         ; your programs and drivers
```

### **A module**

A module is a program, a driver or a library in the paged ROM, run in place in its own banks (one to eight, 16K each),
with a 48-byte header first (`sdk/asm/hyx2.inc`):

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
  protocol, through `srvlib` (`sdk/asm/srvlib.inc` and `srvlib.s`: a tree of files and handlers; `kdev` and `ser` are
  built on it).  Its `irq` entry gets the lines it owns.
* **A library** (`HYX2_LIBRARY`) is code another task calls in its own task with `XCALL`.

Build one with the base's: put it in a folder, `modules/NAME/NAME.s`, and `node build.js` assembles it with the base's
includes (`sdk/asm`, `include`, `obj/gen`, `lib`) and links it with `modules/module.cfg` (`moduleN.cfg` for N
banks: a `.segment "CODE2"` ... in its source); then name it in `rom.txt`.  Or, in a tree of your own beside this one, do
what HydraOS's `build.js` does: `require('../base/build.js')`, its `build()` for the kernel and the base's modules, its
`buildModule()` for yours, its `tools/romimg.js` for your paged ROM.

### **What else the base has**

* **The kernel's devices** (`kdev`, task E): `#m` (the modules), `#n` (`null`, `zero`, `kmesg`: the kernel's messages),
  `#p` (the tasks: status, ctl, args, fds, memory ...), `#|` (pipes: `PIPE`), `#e` (the environment), `#s` (named shared
  segments), `#t` (the ticks), `#r` (raw RAM, for init).
* **The console** (`ser`, task F): `#c/cons` (a line at a time, cooked; or raw with `#c/consctl`'s `rawon`), `#c/ser`
  and `#c/serctl` (the serial port raw, its rate).  HydraOS's console (`cons`, its windows) serves the same files and
  more, on the same serial layer (`lib/serial.inc`): write your own on it, or keep `ser`.
* **The monitor** (`wozmon`): to poke at the machine while you bring your system up; [the monitor](monitor.md).
* **The emulator** (`sim/`): the board, cycle by cycle, with a monitor of its own (`sim/run.js -i`, Ctrl-A b), the
  system calls traced by name (`--trace-calls`), breaks on a module's labels.  `sim/run.js`'s `main(argv, { root })` and
  `sim/test.js`'s `setup({ root, tests, image })` run your tree's images and tests, as HydraOS's `sim/run.js` and
  `sim/test.js` do.
