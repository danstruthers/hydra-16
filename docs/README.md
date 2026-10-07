## **The Hydra-16's design documents**

HydraOS's own documents are in [../reborn/docs](../reborn/docs/status.md); the old system's are in [../old/docs](../old/docs/README.md).  Here:

* [hardware.md](hardware.md): the board (V1) as its schematic describes it, with the two companion cards.
* [reimplementation-from-scratch.md](reimplementation-from-scratch.md): the plan HydraOS was built to, its phases, and the questions it left the user (all answered now).

### **Plans and design notes**

The reasoning behind the design, and what's planned.  Many were written for the old system, and parts of them are history (marked done).
* [NEXT_STEPS.md](plans/NEXT_STEPS.md): what's missing to make the Hydra fun and useful for hobbyists and programmers, and the milestones to get there.
* [VIDEO.md](plans/VIDEO.md): the Vera X video card in slot 0 (the VERA: VGA, sprites, PSG and PCM), its keyboard controller, and the software for them.
* [SOUND_PARITY.md](plans/SOUND_PARITY.md): the same sound in every language and at rc: sndctl's text, the score language on the Hydra, the PSG and PCM.
* [WINDOWS.md](plans/WINDOWS.md): text windows: screens in the console's RAM banks, a whole VT100, window groups, headers, footers and a bar.
* [SOUND.md](plans/SOUND.md): the YM2151: its library, a song player, a test song that uses the whole chip, and importing music from other machines.
* [PC.md](plans/PC.md): `/pc`, a folder on the PC served over the serial port by the PC tool, which is the terminal too.
* [DISKS.md](plans/DISKS.md): RAM and ROM disks as HydraFS volumes, with program caches.
* [NAMESPACES.md](plans/NAMESPACES.md): the Plan 9 way: union directories put together with binds and mounts, a default namespace, and `/bin` and `/lib` in place of search paths.
* [PROC.md](plans/PROC.md): `/proc`, the tasks as files.
* [HYDRAFS.md](plans/HYDRAFS.md): the SD card filesystem (spec).
* [SHELL.md](plans/SHELL.md), [IO_PLAN.md](plans/IO_PLAN.md), [MMU_PLAN.md](plans/MMU_PLAN.md), [REORG_PLAN.md](plans/REORG_PLAN.md): the old system's shell, IO, memory manager and ROM reorganisation.
* [CODE_REVIEW.md](plans/CODE_REVIEW.md): a review of the old system's software, with bugs, budgets and recommendations.
* [IDEAS.md](plans/IDEAS.md): ideas for later (wait states for board V2, ...).
