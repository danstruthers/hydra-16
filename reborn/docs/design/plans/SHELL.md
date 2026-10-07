## **HyForth as a shell**

A plan for making HyForth a usable shell on HydraFS cards: the cards found and a volume selected at boot, a boot script, a current volume and directory shown in the prompt, the commands to move around and handle files, and running programs: HyForth scripts (`.hys`) and Hydra executables (`.hyx`).  The decisions are at the end.

**Status: done** (build steps 1-6 below).  How to use it is in [hyforth.md](../../../../old/docs/using/hyforth.md#the-shell-directories-files-and-programs), and writing programs in [programs.md](../../../../old/docs/programming/programs.md).  Where it differs from this plan:
* `vol n` and `ls -l` weren't made: `cd /sd/n` goes to card n, and `ls` shows names and sizes.
* `run` tells an executable from a script by its `HYX1` header, not its name.
* The shell waits for a program by making itself the program's parent and pausing (as `TASK_START` does), rather than polling.  The program gets the console only if the shell has it.
* A script run by `run` starts with a copy of the shell's stack (it's a `TASK_CLONE`, like a pipeline's stage), so `3 4 add` passes it arguments; executables get none.
* HyForth got `"..."` strings, valid wherever a `q^...^` string is (stack forms, `prompt`, `ctl`, `open` ...), and the parsing forms take a `"quoted name"` with spaces in it.
* The executable loader is on page 7 with the rest of the shell; the build tools are `programs/asm/hyx.cfg` and `hyx.inc` (the header made by ld65) and `sim/tools/mkhyx.js` (for raw binaries).

When it was planned: HydraFS was complete (reading, writing, `check`), with HyForth words for files (`ls`, `create`, `mkdir`, `remove`, `rename`) and for the cards themselves (`vols`, `mkfs`, `relabel`, `fsck`, `fsfix`).  Every file word took a full path as a `q^...^` string.

### **1. The current directory, in the IO layer**

A **current directory per task**, kept by the IO layer rather than by HyForth, so every program gets it, and the tasks a shell starts (pipeline stages, programs) inherit it:

* **Where:** in the task's IO transfer area, next to its namespace: the namespace goes from 7 entries to 5, which frees 64 bytes, a path of up to 63 characters.  It's copied to new tasks as the namespace is (`IO_INHERIT`).
* **`IO_OPEN` and friends:** a name that doesn't start with `/` is taken as relative to it (`notes.txt`, `games/star.frt`, `../x`).  The namespace applies after that, as now.
* **New calls:** `IO_CHDIR` (.A.Y = a path, relative or not: it's checked by opening it as a directory, and stored as a clean absolute path, with `.` and `..` worked out) and `IO_GETCWD` (the path into a buffer).
* **At startup:** the selected volume's root (below), else `/`.
* **Volumes:** a volume is a card, so "the current volume" is just the card in the current directory's path (`/sd/1/...`); `vol 1` is `cd /sd/1`.

(The alternative, keeping the directory in HyForth alone, is simpler, but other programs and pipeline stages wouldn't see it.)

### **2. Boot**

After the drivers start, the boot shell (task 1):
1. **Finds the volumes:** it opens `/sd/0` ... `/sd/7`, which starts each card and reads its superblock, and says which hold a HydraFS (`hydrafs 0 2`).
2. **Selects the lowest numbered one:** its root is the shell's current directory, which every task it starts inherits.
3. **Runs `boot.hys`** from that root, if it's there, as `include` does, before the first prompt.  Only the boot shell does: a `shell` started later doesn't.

### **3. The prompt**

Set with `prompt` (`"%v%d> " prompt` or `q^%v%d> ^ prompt`, the default: `0:/games> ` on card 0, `/> ` off the cards).  In the format:

| Field | Is |
| :---- | :- |
| `%v` | The volume: `0:` when the current directory is on card 0, nothing off the cards |
| `%d` | The directory: the path on the card (`/games`), or the whole path off the cards |
| `%p` | The whole path (`/sd/0/games`) |
| `%l` | The card's HydraFS label (`GAMES`; nothing off the cards).  Added after the plan: read from the card's ctl file, and kept until the prompt moves to another card, or `mkfs` or `relabel` runs |
| `%t` | The task number |
| `%%` | A `%` |

The prompt is printed only when input comes from the console, so a script being read doesn't print prompts.

### **4. Shell commands**

Each command has a **parsing form** for typing, which takes its arguments from the words after it on the line, like a shell (`cd games`), and a **stack form** for definitions and scripts' Forth code, in parentheses, which takes a string: `"..."` or `q^...^` (`"games" (cd)`, `q^games^ (cd)`).  A parsing form takes a name with spaces in it in quotes (`cd "my games"`).  The file words that take a `"..."` or `q^...^` path today (`ls`, `mkdir`) become `(ls)` and `(mkdir)`.

| Command | Does |
| :------ | :--- |
| `cd dir`, `cd ..`, `cd` | Change directory (`cd` alone: the card's root) |
| `pwd` | Show the current directory |
| `vol n` | Go to card n's root |
| `ls`, `ls dir` | List a directory (`-l`: with qid versions and modes, from stat records.  As built: `-l` gives each entry's date and time, from its stamp) |
| `cat file` | Show a file |
| `cp from to`, `mv from to` | Copy, move (a move within a card is a rename; across cards, a copy and remove) |
| `rm file`, `mkdir dir`, `rmdir dir` | Remove, make |
| `vols`, `mkfs`, `fsck`, `fsfix`, `relabel` | The cards (as now) |
| `include file` | Run a HyForth script (`.hys`) in this shell: its definitions stay |
| `run file` | Run a program in a task of its own: a script (`.hys`) or an executable (`.hyx`); the shell waits for it (Ctrl-C and `kill` work on it) |
| `name` | A word HyForth doesn't know: `name.hyx` or `name.hys`, from the current directory, then the selected volume's `/bin`, is run as `run` does |

### **5. Running programs**

* **Scripts** (`.hys`): `include` points the shell's stdin at the file, reads lines until the end of the file (no prompts, no echo), then puts stdin back; an error stops the script and says which line.  `run` does the same in a new shell task, so the script's definitions go away with it.
* **Executables** (`.hyx`): a file with a small header (below), loaded into a new task's RAM and started there, with the shell's fds and current directory.  A ROM loader does it: the shell opens the file on a spare fd and starts a task at the loader, which inherits the fd, reads the header and the code into its own RAM, closes the fd and jumps to the entry point.  The task ends when the program returns (or calls the exit call, or is killed), and the shell's prompt comes back.

**The executable header** (16 bytes, then the code; not relocatable: a program is linked for its load address):

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 4 | Magic: `HYX1` |
| 4 | 2 | Load address (in task RAM, `$0800-$7BFF`) |
| 6 | 2 | Length of the code |
| 8 | 2 | Entry point |
| 10 | 2 | Flags (0) |
| 12 | 4 | Reserved (0) |

Programs call the OS through the `$F8xx` thunks, as now.  A `sim/tools` script (or a ca65 config) makes the header from a linked binary.

### **Build order**

1. The current directory in the IO layer (`IO_CHDIR`, `IO_GETCWD`, relative names).
2. Boot: finding the volumes, selecting one, `boot.hys`.
3. The prompt, and the commands, with their parsing and stack forms.
4. `include` and `run` for scripts.
5. The executable loader, the header tool, a sample program, and running a program by its name.
6. Docs and regression tests throughout.

### **Decisions**

* Commands have parsing forms for typing and stack forms for definitions.
* The prompt's format can be set (`prompt`), with the volume and directory by default.
* Executables have the header above, and run in a task of their own.
* Scripts have both `include` (in the shell) and `run` (a task of their own).
* `.hys` is a HyForth script, `.hyx` a Hydra executable; the boot script is `boot.hys` in the selected volume's root.
