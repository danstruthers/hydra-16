# The design: the plan and the notes

How HydraOS came to be as it is: the plan it was built to, and the design notes written along the way.  These are
the reasoning and the history; what the system is now is in [the guide](../hydra-16.md) and the documents it links to
([status.md](../status.md) says where each part stands).  Many notes were written for the old system (frozen in
[../../../old](../../../old/docs/README.md)), and parts of them are history, marked so.

* [reimplementation-from-scratch.md](reimplementation-from-scratch.md): the plan HydraOS was built to, its
  principles, its phases (each with a note on how it was built), and the questions it left the user (all answered).

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
* [SOUND.md](plans/SOUND.md): the YM2151: its library, a song player, a test song that uses the whole chip, and
  importing music from other machines.
* [PC.md](plans/PC.md): `/pc`, a folder on the PC served over the serial port by the PC tool, which is the terminal
  too: its protocol.
* [DISKS.md](plans/DISKS.md): RAM and ROM disks as HydraFS volumes, with program caches.
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
