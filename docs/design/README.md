# The design: the plan and the notes

How HydraOS came to be as it is: the plan it was built to, the design notes written along the way, and where each part
stands.  These are the reasoning and the history, kept as they were written; what the system is now is [the
book](../README.md).  Many notes were written for the old system (frozen in [old/](../../old/docs/README.md)), and parts
of them are history, marked so.  Their links to documents that have since become the book's chapters lead to those
chapters.

* [reimplementation-from-scratch.md](reimplementation-from-scratch.md): the plan HydraOS was built to, its
  principles, its phases (each with a note on how it was built), and the questions it left the user (all answered).
* [status.md](status.md): where everything stands, step by step against the plan, with what the spikes and budgets
  measured; [forth-status.md](forth-status.md), HyForth's own steps.
* [conventions.md](conventions.md): the rules every source follows, in full: calling the system, memory, tasks,
  reaching other tasks, the kernel's pages, notes, interrupts, modules, files and servers, names, the source style.
* The languages' designs: [hyforth.md](hyforth.md) (HyForth's additions: the old HyForth's pieces, the shell, the
  index, code in banks), [hylang.md](hylang.md) (hylang: the language, where it differs from danlang, its design and its
  benchmarks), [basic.md](basic.md) (HyBASIC: its compiler, its stack machine, its banks, its benchmarks).
* [base-os.md](base-os.md): writing an OS on the base, as it was first written (now [a chapter of the
  book](../09-programming/your-own-os.md)).

## Plans and design notes

* [NEXT_STEPS.md](plans/NEXT_STEPS.md): what's missing to make the Hydra fun and useful for hobbyists and
  programmers, and the milestones to get there.
* [VIDEO.md](plans/VIDEO.md): the Vera X video card in slot 0 (the VERA: VGA, sprites, PSG and PCM), its carrier card
  and keyboard controller, and the software for them.
* [SOUND_PARITY.md](plans/SOUND_PARITY.md): the same sound in every language and at rc: sndctl's text, the score
  language on the Hydra, the PSG and PCM.
* [WINDOWS.md](plans/WINDOWS.md): text windows: screens in the console's RAM banks, a whole VT100, window groups,
  headers, footers and a bar.
* [NUMBERS.md](plans/NUMBERS.md): one number system for every language: danlang's and hylang's (integers of any size,
  fixed decimals, rationals, complex numbers, every base), a compact format, shared `numbers` and `math` libraries,
  and BASIC rewritten on them.
* [BASIC.md](plans/BASIC.md): a new BASIC for the Hydra, QuickBASIC's kind: line numbers optional, labels, blocks,
  `SUB` and `FUNCTION`, on those numbers.
* [SOUND.md](plans/SOUND.md): the YM2151: its library, a song player, a test song that uses the whole chip, and
  importing music from other machines.
* [PC.md](plans/PC.md): `/pc`, a folder on the PC served over the serial port by the PC tool, which is the terminal
  too: its protocol.
* [DISKS.md](plans/DISKS.md): RAM and ROM disks as HydraFS volumes, with program caches.
* [TASKS.md](plans/TASKS.md): fewer standing tasks: init becomes the shell, the kernel starts init for each window,
  a driver's own work between its calls (`SRV_NAP`), and GPIO, I2C and the input controller in `kdev`.
* [NAMESPACES.md](plans/NAMESPACES.md): the Plan 9 way: union directories put together with binds and mounts, a
  default namespace, and `/bin` and `/lib` in place of search paths.
* [PROC.md](plans/PROC.md): `/proc`, the tasks as files.
* [HYDRAFS.md](plans/HYDRAFS.md): HydraFS, the file system on the cards, the RAM disks and the ROM disk: its format
  (HydraOS's storage driver keeps it, unchanged).
* [SHELL.md](plans/SHELL.md), [IO_PLAN.md](plans/IO_PLAN.md), [MMU_PLAN.md](plans/MMU_PLAN.md),
  [REORG_PLAN.md](plans/REORG_PLAN.md): the old system's shell, IO, memory manager and ROM reorganisation.
* [CODE_REVIEW.md](plans/CODE_REVIEW.md): a review of the old system's software, with bugs, budgets and
  recommendations.
* [IDEAS.md](plans/IDEAS.md): ideas for later (wait states for board V2, ...).
