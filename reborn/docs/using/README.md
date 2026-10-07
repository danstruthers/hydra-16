# Using the Hydra-16

The guides for using the rebuilt system (`reborn/`).  The first hour is [the tutorial](../tutorial.md); after it:

| Guide | What it covers |
| :---- | :------------- |
| [The tutorial](../tutorial.md) | Switching it on (in the emulator or on the board), the shell, files and disks, windows, the languages, a program of your own |
| [rc](rc.md) | The shell underneath everything: commands, quoting, variables, pipelines, redirection, control flow, scripts |
| [The tools](tools.md) | Every program in `/bin`: files, text, tasks, disks, and the rest |
| [HyForth](hyforth.md) | The Forth, and the login shell: its words, libraries, the shell rule, files, devices, sound |
| [hylang](hylang.md) | The lisp (danlang on the Hydra): its REPL and scripts, the system library, the Hydra's built-ins, devices |
| [BASIC](basic.md) | Microsoft's BASIC (EhyBASIC): programs and scripts, files, sound and `PLAY`, `SYS` and the system's calls, memory, the shell |

For writing programs: [../programming/README.md](../programming/README.md), the programmer's guide, and the SDKs'
own guides, [../../sdk/asm/README.md](../../sdk/asm/README.md) (assembly) and [../../sdk/c/README.md](../../sdk/c/README.md)
(C).  The system calls' reference is `/rom/doc/api.md` on the Hydra (made from `spec/api.def` at the build:
`obj/gen/api.md`).

## The system in a paragraph

The Hydra-16 runs 16 tasks, each with its own 32K of RAM and its own RAM banks.  The kernel is in the BIOS ROM;
everything else, the drivers and the programs, are modules in the paged ROM that run in place.  Almost everything is
a file, Plan 9's way: a device is a directory of files (the console's at `/dev`, the disks' at `/dev/sd`, the tasks'
at `/proc`), controlled by writing commands to its `ctl`.  Each task has a namespace, the tree of names it sees,
built by binds and mounts, so `/bin` is a union of the RAM disk's, the cards', the ROM disk's and the ROM's
programs.  The console is windows, rio's way on a serial terminal: each window is a whole console with its own shell,
shown one at a time (Ctrl-] and a digit).  The login shell is HyForth over rc (`forth -l`): a line is Forth if its
first word is a Forth word or a number, else an rc command line.
