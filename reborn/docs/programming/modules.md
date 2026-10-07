# Modules and programs

## HYX2

Every executable is a HYX2 module: a 48-byte header (`sdk/asm/hyx2.inc`), then its code and data.  The header says
what it is (`HT_PROGRAM`, `HT_DRIVER`, `HT_LIBRARY`), where it loads (`$A000` in place, `$0800` in RAM), its data and
BSS, its entries (main, or init, serve, irq and stop), its device letter, the banks it spans, and its name.

| Macro | For |
| :---- | :-- |
| `HYX2_PROGRAM "name", main` | A program: `main` gets its arguments at `r0`; returning is `EXITS` 0 |
| `HYX2_DRIVER "name", init, serve, irq, stop, flags` | A driver ([servers.md](servers.md)) |
| `HYX2_LIBRARY "name", "segment", "memory"` | A library module: code alone, called by other modules |

## In RAM, or in place

**A program in a file** (on a card, a RAM disk, `/pc`) is a RAM program: assembled with `-D HYX2_RAM` and linked by
`sdk/asm/hyx2.cfg` (its header, code and data from `$0800`, its BSS after them).  `SPAWN` gives the new task the file
as its fd 15, and the task reads it in itself as it starts, clears its BSS, and runs it.  `node build.js prog DIR`
builds one from a folder of `.s` (or `.c`) files.

**A module of the paged ROM** runs in place: the task that runs it has the module's bank at `$A000` (its `$01`), and
only its data is copied into the task's RAM (from `$0400`), its BSS cleared, as it starts.  It's built by
`modules/module.cfg` and listed in `modules/rom.txt`; `#m/NAME` is its file, and `#m/bin` lists the programs, bound at
`/bin`.  Running in place costs no load: `SPAWN` of a module takes about 50,000 cycles (14 ms).

## Modules of several banks

A module bigger than a bank (16K) spans two to eight, one after another: `HYX2_DRIVER ..., 2` (or `HYX2_PROGRAM
"name", main, 2`), its second bank's code in the segment `CODE2` (linked by `modules/module2.cfg`), the third's in
`CODE3` ...  It calls between its banks through trampolines in its RAM: `FAR2` (from the first into the second),
`FAR1` (back), and `FARN bank, routine` from any to any, the caller's bank set again after.  Such a module keeps its
note handler in its RAM, as the kernel calls a handler with whichever bank is at `$A000`, and owns no IRQ line.  rc
and `play` (two banks each), HydraFS (the storage driver's second) and hylang (eight) are built so.

## Libraries and XCALL

A library module is code other modules call, in their tasks, with their RAM: it has no data, BSS or entries of its
own.  A program finds a library's bank in the module directory (`MODINFO`: entry by entry, its `ME_NAME` and
`ME_BANK`), and calls it with `XCALL`, the X16's `jsrfar`: `r15` the routine's address, `r14` its bank; `.A`, `.X`,
`.Y`, the flags and `r0`-`r13` pass through both ways, and the caller's bank comes back after it.

```
            lda         lib_bank                            ; (MODINFO's ME_BANK, found as it started)
            sta         r14
            LDR         r15, LIB_ADD                        ; The library's routine ($A030: its jump table)
            lda         #3
            ldx         #4
            jsr         XCALL                               ; .A = 7
```

A library keeps a jump table at its start, after its header, so its routines' addresses don't move as it changes:
`tests/mod/t_lib/t_lib.s` is one, and `t_xcall` calls it.

## Debugging

The emulator (`sim/run.js`) traces a program's system calls by name (`--trace-calls`), stops at a label (`--break
K_OPEN`, `--break rc:main`), watches an address (`--watch ADDR[:T]`), and with `-i` has a monitor (Ctrl-A b: step,
registers, memory in any task's view, breaks and watches).  The top of `sim/run.js` lists its options.  On the
Hydra, `/proc/N` is a task's state (`regs`, `mem`, `fd`), and its `ctl` stops, starts and steps it; `db`, the
debugger, works through them: a program started stopped, stepped, run to breakpoints, with ld65's symbols (the build's
`.lbl` files, or `as -l`'s) ([../using/tools.md](../using/tools.md#the-debugger)).
