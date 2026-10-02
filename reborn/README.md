# Hydra-16 reborn

The Hydra-16's operating system, rebuilt from the ground up by the plan in
[docs/reimplementation-from-scratch.md](../docs/reimplementation-from-scratch.md): one kernel in the BIOS ROM,
everything else modules in the paged ROM run in place in tasks of their own, one way to call the system (a
jump table made from a specification), one interrupt path, and every performance-critical mechanism measured
in the emulator before anything depends on it.  The old system (`os_rom/`) is untouched beside it.

Where it stands, step by step against the plan, and the spikes' measurements: [docs/status.md](docs/status.md).
The rules every source here follows: [docs/conventions.md](docs/conventions.md).

## Build, run, test

Node.js 18 or later, and the cc65 tools (`ca65`, `ld65`): `$CC65_BIN`, `$CC65_HOME/bin`,
`C:\source\cc65\win64_snapshot\bin`, or the PATH.

```
node build.js                       the BIOS ROM (bin/bios.bin), the paged ROM (bin/prom.bin), the budget report
node sim/run.js                     boot them in the emulator for a while, then report (the output, the tasks)
node sim/run.js -i                  the Hydra's serial console, live (Ctrl-A x quits, Ctrl-A h helps)
node sim/test.js                    the regression tests, with their time budgets
node sim/test.js irq -v             one test, with its output
```

`build.js --clock 2` builds for a 7.16 MHz board (jumper J7); `--acia wdc` for a WDC W65C51N in the serial
port.  `bin/` and `obj/` are the build's: not in Git.

## The tree

| Folder | What's there |
|---|---|
| `spec/` | `api.def`, the system calls (each call's group, slot, registers, errors and words), and `errors.def`, the error codes: the one source for the jump table, `hydra.inc`, the reference and the emulator's names |
| `include/` | `hw.inc`, the board as the software sees it (facts only); `layout.inc`, where every piece of the kernel's state lives |
| `kernel/` | The kernel: the BIOS ROM's 16 pages (`bios.cfg`), the reset stub and vectors on each, the COMMON block, interrupts, the scheduler, calls between tasks and kcopy, tasks and modules, the bring-up console, the small calls |
| `modules/` | The paged ROM's modules, a folder each (`init`, `hello`); `module.cfg`, their link; `rom.txt`, which go in the ROM and which is init |
| `sdk/asm/` | For programs in assembly: `hyx2.inc` (the module header), `macros.inc`; with `obj/sdk/hydra.inc`, made from `spec/` |
| `tools/` | `apigen.js` (the specification's outputs), `romimg.js` (the paged ROM image), `budget.js` (sizes and room left) |
| `sim/` | `run.js` and `test.js` over `lib/`, the emulator (a copy of `../sim/lib`, with a receive-latency counter added to the ACIA) |
| `tests/` | `tests.js`, the tests and their budgets; `mod/`, the test modules (each runs as init) and `testlib.inc` |
| `docs/` | `conventions.md`, `status.md` |
