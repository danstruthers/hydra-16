# The C SDK

Programs for the Hydra-16 in C, with cc65 (`cc65`, `ca65`, `ld65`).  A program is a HYX2 file, as an assembly
one is (`sdk/asm/README.md`): a 48-byte header, then its code and data, which `SPAWN` reads into a task's RAM at
`$0800` and starts.  cc65's own target `none` does the compiling; the Hydra's part is the start-up, the link and
the library under the standard one.

The SDK is this folder; `node build.js` builds the library and also copies the SDK, with the generated
`hydracalls.h`, the library and the samples, to `bin/sdk/c`, to take elsewhere.

| File | What it is |
|---|---|
| `include/snd.h` | The YM2151, and the Vera X's PSG (channels 8-23), through the sound driver: channels claimed, patches, notes, volumes, bends, drums, waveforms, raw registers |
| `include/vera.h` | The Vera X's screen: the bitmap and drawing on it (`vid_pen`, `vid_line`, `vid_circle`, `vid_text` ...), VRAM, the palette, sprites, frames, the mouse; the chip claimed and its registers (`VERA`, `vpoke`, `vpeek`); `hydra_tgi`, cc65's TGI's driver |
| `include/hydra.h` | The Hydra's own calls (tasks and exit statuses, the namespace, the tick, RAM banks, any call by `hy_call`), and what cc65's headers leave to a target: `setenv`, `fstat`, `isatty`, conio's colours and keys |
| `include/num.h` | The Hydra's numbers, the number libraries' (hylang's, HyForth's and BASIC's): exact integers of any size, fixed decimals, rationals and complex numbers; their arithmetic, conversions, text in every base, bits, and the math functions (below) |
| `numdefs.h` | The number libraries' constants (`NUM_MAX`, the errors `NE_`, the kinds `NK_` ...).  Made from `spec/numbers.def` by the build, and kept in Git; `num.h` includes it |
| `include/asm.h` | The W65C02S's instructions: `dis_insn`, one disassembled as `as` writes it (the asm library's, `db`'s and `dis`'s), and symbol files (`lbl_load`: ld65's and `as -l`'s labels, `hydra.inc`'s calls; `lbl_at`, `lbl_label`, `lbl_near`, `lbl_addr`) |
| `include/dice.h` | Dice for games, danlang's `lib/dice.dl` in C: `die`, `die_plus`, `d6` ... `d100`, `dice`, `dice_plus`, `sumrolls`, `special_die`, the open-ended dice (`oe_die` ...), `roll`, `best_of_rolls`, `worst_of_rolls`, the D&D attribute rolls; and the numbers they're rolled with, hylang's and HyForth's (`rand16`, `rand32`, `rseed`, `random`: xorshift32, the same numbers for the same seed); a test's hook, `dice_hook` (every die rolled by a function of the program's: the faces fixed, the dice seen) |
| `include/harn.h` | HârnMaster (third edition, 3.5) characters and their rules, the C version of danlang's `lib/harn.dl`: a character made as the book's Character article makes one (`harn_create`: birth, sunsign, culture and class, family, appearance, the attributes with their modifiers, medical traits; a template fixes any part), the Skill Data table (`harn_skills`: 91 skills, a program's own after them) and a skill's SB, OML and development, the penalties (UP, EP, PP), tests (`harn_test`'s CF, MF, MS, CS; EMLs; attribute, shock, stumble and fumble rolls), the combat tables (melee, missile, strike location, injury), armour, weapons, a wound taken (`harn_wound`: kill and shock rolls, bleeding, amputation), healing, and a character in a file (`harn_write`, `harn_read`: text, a line a thing, which games/hmchar writes) |
| `asmdefs.h` | The asm library's constants (`DIS_MAX`, `DF_PAD`, the kinds `DK_`).  Made from `spec/asm.def` by the build, and kept in Git; `asm.h` includes it |
| `hydracalls.h` | Every system call's address, error code and constant, each with `HY_` before its name.  Made from `base/spec/` by the build, and kept in Git; never edit it.  `hydra.h` includes it |
| `hydra.cfg` | The link: the header, code and data from `$0800`, the BSS after them, the heap, and the C stack (2K) down from `$7F00`; a program whose BSS ends within `__STACKMIN__` (768 bytes) of `$7F00` fails to link, rather than its stack running over its BSS |
| `lib/hydra.lib` | The library: cc65's `none.lib`, with the modules of `lib/` in place of cc65's that a target gives (the build: `obj/sdk/c/hydra.lib`) |
| `lib/` | Its sources: `crt0.s` (the header, the start, `exit`), the files and stdio's buffers, the environment, `system`, `signal`, `time` and `clock`, conio, errors; `num.h`'s functions (`num.s`, and `numcall.s`, which finds the libraries); `asm.h`'s (`dis.s`, `lbl.c`); `dice.h`'s (`xorshift.s`, `diceseed.c` its first seed, `dice.c`, `dicemore.c`, `dicebest.c`) and `harn.h`'s (`harn.c` the skills and penalties, `harnname.c` the names, `harnkey.c` the skills' keys, `harnsp.c` their specialties, `harnfile.c` a character's file, `harngen.c` a character made, `harntest.c` the tests, `harncmbt.c` the combat tables and wounds, `harnarm.c` armour, `harnwpn.c` weapons, `harnheal.c` healing, `harnfall.c` falling: each part a file, so a program links only what it uses); `printf`'s and `scanf`'s cores, cc65's `_printf.s` and `_scanf.c` with the Hydra's numbers added |
| `samples/` | `hello` (arguments), `upper` (a filter), `code` (exit statuses), `keys` (conio: the screen and raw keys), `tones` (sound: `snd.h`), `jukebox` (a song in the background: `snd_play`), `sketch` (the screen and the mouse: `vera.h`), `shapes` (cc65's TGI), `ctest` (the library's test), `harn` (a HârnMaster character made and shown as a Character Profile has it, a blow struck at it, and some dice: `harn.h`, `dice.h`); the multitasking demos (below): `race`, `chorus`, `philo`, `prodcons`, `round`, `ntest` (`num.h`'s test: numbers, and `printf`'s and `scanf`'s) |

## A program

```
#include <stdio.h>

int main (int argc, char* argv[])
{
    printf ("hello, %s\n", argc > 1 ? argv[1] : "world");
    return 0;
}
```

* `argv[0]` is its name: its file's (`/sd/0/sample/c/hello`'s is `hello`); its arguments are rc's words.
* `main`'s value, or `exit`'s, is its exit code (0 is success); `hy_exits ("why")` ends it with a message and
  code 1, as Plan 9's `exits` does.  rc's `$status` is the message if there is one, else the code.
* Its fds 0, 1 and 2 are stdin, stdout and stderr: its window's console, or what rc gave it (`<`, `>`, `|`).
  stdio is buffered: a file's bytes are read and written 255 at a time, a console's output a line at a time
  (and before any read), stderr's at once; it all goes out at `fflush`, `fclose` and the program's end.
* Files: `fopen`, `open`, `read`, `write`, `lseek`, `stat`, `fstat`, `remove`, `rename`, `mkdir`, `rmdir`,
  `chdir`, `getcwd`, and `opendir`/`readdir` (`hy_dirstat`: an entry's whole stat record).  Names go through its
  namespace, as rc's do; `hy_bind`, `hy_mount` and `hy_unmount` change it.
* A failed call returns -1 (or NULL) and sets `errno` (cc65's: `ENOENT` ...) and `_oserror` (the kernel's own
  code, `HY_E_NOENT` ...; `_stroserror` and `hy_errstr` give its text, the kernel's).
* Its environment is a copy of rc's variables: `getenv`, `setenv`, `putenv`, `unsetenv`.
* `system ("cmd")` runs a command line with rc (`rc -c`): its value is the command's exit code.  `hy_spawn`
  starts a program in a task of its own, `hy_wait` waits for it and takes its code and message; `hy_exec` runs a
  program in this one's place, in its task (Plan 9's `exec`: back only if it fails); `hy_parent` is
  the task that started this one.
* Semaphores, for tasks that share something: `hy_sem_new` (a count, or `HY_SEM_MUTEX`), `hy_sem_acquire` (it
  waits), `hy_sem_try`, `hy_sem_release`, `hy_sem_free`.  Every task's, by number; the program's end frees those
  it made and gives back the mutexes it holds.
* Shared segments, memory several tasks see: `hy_seg_create` (banks of 8K: its number, this task attached),
  `hy_seg_attach` (another task, given the number), `hy_seg_map` (a bank of it at `HY_BANK_WINDOW`, `$8000`),
  `hy_seg_detach`.  The last task attached to go frees it.
* `signal (SIGINT, f)`: Ctrl-C at its window (the interrupt note) runs `f`; `SIG_IGN` ignores it; the default
  ends the program.
* conio (`conio.h`) works the console as an ANSI terminal: `clrscr`, `gotoxy`, `textcolor`, `revers`, `cursor`;
  `cgetc` reads keys raw (no echo, each as it's typed, the cursor and function keys as one code each: `CH_*` in
  `hydra.h`) until the program ends.  `screensize` is the window's size (its `consctl`'s `size` line); `cgetc`
  gives `CH_RESIZE` when it changes, and `screensize` then has the new one.  With `keys mods` written to `consctl`, a
  key the terminal sent modified comes as `CH_MOD`, its modifiers (`HY_MOD_SHIFT`, `HY_MOD_ALT`, `HY_MOD_CTRL`), then
  the key.
* The window's chrome (`hydra.h`): `hy_wlabel` its title, `hy_wstatus` its status line (the footer's `%s`),
  `hy_wctl` any line of its `wctl` (`chrome screen off` ...); `hy_wnew (flags)` a window made, not shown (`HY_WGROUP`:
  in a group of its own), its number: its files `#cN` (`#cN/cons` ...), the window gone with its last cons closed.
* Sound (`snd.h`): claim the channels it uses (`snd_claim`), then patches, notes, volumes, bends and drums on
  them; they're given back as it ends.  `snd_play` plays a song in the background (`play`, in a task of its own).
* `time` is the system's clock (`/dev/time`; `hy_time`, in seconds since 2000), `clock` the ticks since the
  program's start; `sleep` and `hy_sleep_ticks` let the other tasks run, and `hy_sleep_until` sleeps till a tick
  count (for steady timing: a time already passed returns at once).
* Its RAM: `$0800` to `$7F00`, for the program, its BSS, the heap (`malloc`) and the C stack (2K); its own RAM
  banks at `$8000`-`$9FFF` (`hy_banks_alloc`, `hy_bank`).  The 6502's stack is 256 bytes: deep recursion runs
  out of it.  cc65's runtime has the zero page from `$22` (26 bytes); the program's own goes after it, to `$7F`.

## Dice and HârnMaster

`dice.h` is danlang's `lib/dice.dl`, function for function, by the same names where C allows (`die+` is
`die_plus`, `oe-dice` is `oe_dice`).  `harn.h` is HârnMaster, third edition (3.5): the C version of danlang's
`lib/harn.dl`, both following the book, by harn.dl's names (`harn.get-sb` is `harn_get_sb`) and the book's.

```
#include <stdio.h>
#include <harn.h>

int main (void)
{
    static harn_char h;

    harn_create (&h, NULL);
    printf ("%s, %s; endurance %u, dodge %u\n", harn_species_name (h.att[HARN_SP]),
        harn_sunsign_name (h.att[HARN_SS]), harn_end (&h), h.ml[HARN_DGE]);
    return 0;
}
```

* The numbers are hylang's and HyForth's (`lib random`): `rseed (42)` gives the same `rand16` and `random` as
  `42 0 rseed` gives HyForth's `rand` and `random`.  With no `rseed` the first number seeds the generator from
  the clock and the ticks.  `random (n)` is from 0 to n - 1; `die (6)` from 1 to 6.
* A character's attributes are `att[]`, by `HARN_SP` ... (in the book's order of generation); its birthday is
  `(month << 8) | day`; its skills' masteries `ml[]`, by `HARN_ACR` ... (the Skill Data table's order;
  Dodge, the book's quasi-skill, is `HARN_DGE`, Agility times 5).  `harn_create` opens the automatic skills at
  their OML, and Ritual for a deity the template gives.
* `harn_template` makes a template that rolls everything; set what it should fix (an `att[]`, `culture`, a
  medical trait ...) and pass it to `harn_create`.
* EML: `harn_eml` is ML less five times the physical penalty (physical and combat skills) or the universal penalty
  (the rest); `harn_test` rolls d100 against an EML (kept from 5 to 95) and gives `HARN_CF` ... `HARN_CS`.
* Combat: `harn_melee_result` (or `harn_missile_result`) of the two parties' success levels gives a strike
  (`HARN_R_AH` and its dice ...), a block, a fumble, a stumble or a tactical advantage; `harn_strike_location` a
  location; the impact less `harn_armour_at` there is the effective impact, and `harn_wound` makes it an injury
  and does what follows (the kill roll, shock, bleeding, amputation).
* A program's own skills go after the book's (`harn_own_skills`: skill numbers `HARN_NSKILLS` on, up to
  `HARN_MAXSKILLS`), and every skill function works on them.
* Left out: occupations, contacts, aging, psionics, magic, religion's rituals and mounted combat.  The tables are
  big: `harn.h` whole, with `printf`, leaves a program little room (the sample writes with `fputs`).

## Numbers

`num.h` gives C the numbers hylang, HyForth and BASIC have: integers of any size (to 255 bytes), fixed decimals
(`1.25`), rationals (`2/3`) and complex numbers (`1+2i`), all exact, worked by the number libraries in the paged
ROM.  A number is an array of bytes (`num_t`) in their stored format: `NUM_MAX` (1,040) at most, most of them a few.

```
#include <stdio.h>
#include <num.h>

int main (void)
{
    num_t a[32], b[32], c[64];

    num_parse (a, sizeof a, "2/3", NULL, NULL);
    num_parse (b, sizeof b, "0.5", NULL, NULL);
    num_add (c, sizeof c, a, b);
    printf ("%N, in binary %{b}N\n", c, c);        /* 7/6, in binary 111/110 */
    return 0;
}
```

* A function that makes a number takes its place and its room first, then its operands, and gives back the
  result's length; or -1, and `num_error` says why (`NE_DIV0` ...; `num_strerror` its text).  A result may go
  over one of its operands.  `num_size` is a number's length, from its bytes.
* The first call readies the libraries (`num_init`): it takes two of the program's RAM banks.
* Numbers are read and written in the base, decimal at the start; `num_set_base` changes it, with hylang's base
  strings (`"x"` writes `FF`, `"#x"` `#xFF`; `"b"`, `"o"`, `"c"` balanced ternary, `"16r"`, `"[01]"` digits of
  its own ...).  `num_parse` and `num_display` take a base of their own too (`NULL`: the base); `num_format` is
  hylang's `format` (`"{} is {x}"`).
* `num_digits` sets the math functions' precision, 12 significant digits at the start: `num_sqrt` of 2 is
  `1.41421356237`, of `9/4` exactly `3/2`.
* `printf` (`fprintf`, `sprintf`, `snprintf` and their `v` forms) writes a number with `%N` (a `num_t*`), in the
  base; and with a base in braces right after the `%`, for `%N` and C's integers alike: `%{x}N`, `%{#b}d`
  (`#b101`), `%{c}ld`, `%{16r}u`; `%{}N` the base, and `%{*}N` the base from the arguments, a string (before a `*`
  width's).  The width, `-` and `0` pad the whole text, a `+` or a space goes before a number not below 0, and a
  precision is passed over: a number is written whole, however long (2^1000's 302 digits; its text 8K at most).
  Without braces, `%d` and `%x` are C's own.
* `scanf` (`fscanf`, `sscanf`) reads them the same way: `%N` a number into a place and its room, two arguments
  (`scanf ("%N", n, sizeof n)`), and `%{x}d` an `int` in base x.  Such a conversion reads a word: up to white
  space, its width, or the character the format has next (`"%N,%N"` reads `1/2,3`), all of which must be a number.
* `calc` (`programs/calc`) is a program on it; the sample `ntest` is its test.  Assembly has the same calls:
  `sdk/asm`'s `numbers.inc` and `numlib.s`.

## Building it

In this repository, a program in a folder of its own (its `.c` files, and `.s` files if it has any):

```
node build.js prog path/to/hello          # path/to/hello/hello.hyx
```

With cc65 and `bin/sdk/c` alone:

```
cc65 -t none --cpu 65C02 -O -I sdk/c/include hello.c
ca65 --cpu 65C02 hello.s
ld65 -C sdk/c/hydra.cfg -o hello.hyx hello.o sdk/c/lib/hydra.lib
```

## Bank programs

A bank program (a child) is a subroutine kept in a RAM bank of its caller's own, not a task: it's called like any
function, on the caller's C stack, and works on the caller's RAM.  A program has some 30K of main RAM; its children
each have a bank's 8K more, loaded from a file the first time they're called, and kept loaded till a bank is wanted
(`bankprog.h`):

```c
#include <bankprog.h>

struct bank_fn* make;
int n;

bank_setup ("/lib/me.exp", 0x60, 16);           /* Once: my exports, and my children's zero page */
make = bank_fn ("/lib/maker.bp", "make");      /* maker.bp's make (NULL: bank_err says why) */
n = bank_call (make, int (*)(char*, int), buf, 10);
```

A child's folder (its `.c` and `.s` files) builds into `NAME.bp`: an o65 module linked at $8000
(`sdk/c/child.cfg`), what it uses of its caller (cc65's runtime's helpers, the caller's functions and data) its
imports, its globals its entries.  In this repository a child is `bank/NAME` (its `parents` file names the
programs that call it: the build fails if one of them hasn't something the child uses), and every C program's
exports, `NAME.exp`, are written beside its `NAME.hyx`.  The kernel loads a child (`BANK_LOAD`) into the code
cache it keeps a task's bank programs in (the kernel's "Bank programs", `docs/03-base/kernel.md`): its imports
found in its caller's exports by name, its own zero page put where `bank_setup` says, its text and data read into
the first gap they fit in a bank of the cache's (several children to a bank) and relocated there, its BSS cleared.
A call maps the child's bank at $8000 (the bank register alone: the hub's shared bank comes back as it was), checks
it's still the load it was, marks it used, and costs some 160 cycles more than a JSR; when the cache has no room
(`BANK_BANKS` banks, fewer if `bank_limit` says, or the kernel's banks all taken) the child used least lately is
given up, and loaded again as it's next called.

What a child can't do: it has the $8000 window while it runs, so it can't map another bank there, nor call another
child; it can call only its caller's main RAM's code (not its overlays nor its swapped parts); its code, data and BSS
fit one bank.  `bankrun [-n N] CHILD [ARG ...]` runs one from the shell (its `run (int argc, char** argv)`), lending
it cc65's whole runtime and the C library's usual functions: `bankrun /rom/lib/bank/hello.bp one two`.

## Running it

As an assembly program is run (`sdk/asm/README.md`): copy it to a card or the RAM disk, and type its path, or its
name if it's in `.` or a `bin`.  The samples are on the SD card (`bin/sdcard.img`): `/sd/0/sample/c/hello you`,
`echo hi | /sd/0/sample/c/upper`, `/sd/0/sample/c/keys`, `/sd/0/sample/c/tones`; `cd /ram; /sd/0/sample/c/ctest a 'b c'` runs the
library's test.

## The multitasking demos

Five samples show tasks working together.  Each is one program that starts copies of itself as its workers (each in
a task of its own, given what it needs as arguments: a shared segment's number, semaphores' numbers), draws what
they do as they do it, and says how it went.  They find themselves as `/bin/NAME`, `./NAME` or
`/sd/0/sample/c/NAME`, so after `bind -a /sd/0/sample/c /bin` (at rc, or HyForth's `bind`) they run by name.  Each
needs four or five tasks free besides its own; Ctrl-C ends them all, and the program says how far it got.

| Sample | What it shows |
| :----- | :------------ |
| `race [TASKS [ADDS]]` | Why shared memory needs a lock: four tasks add 1 to a counter in a shared segment, 500 times each (read it, work a little, write it back).  With nothing to keep them apart, a task stopped between its read and its write undoes the others' adds, and many are lost; with a mutex around each read and write, none are |
| `chorus` | The console as a shared resource: four tasks print a line each a letter at a time.  With nothing between them the letters tangle; with a mutex held for a whole line each line is whole; with a baton (a semaphore each, passed round: wait for yours, print, give the next one's) the lines come in turn |
| `philo [-d] [-n MEALS] [N]` | The dining philosophers: five tasks, a fork (a mutex) between each two, a live table of who's thinking, hungry or eating and who holds which fork.  Taking the lower-numbered fork first, they never deadlock; with `-d` each takes their left fork first, and they soon do: the program sees it and ends them |
| `prodcons [-n ITEMS]` | Producers and consumers: two tasks make items and two use them, through a ring of 8 slots in a shared segment, kept in step by two counting semaphores (free slots, filled ones) and a mutex.  The ring fills while the producers are quicker and empties while the consumers are, each side waiting (using no CPU) when it must |
| `round [TIMES [TICKS]]` | "Row, Row, Row Your Boat" as a round: four tasks a voice each, on YM2151 channels 0-3, two bars apart.  They meet at a barrier (semaphores), then each keeps its own time by the tick (`hy_sleep_until`), and the words light up as they're sung.  In the emulator, `sim/run.js -i --sound` plays it in a browser |

Each runs as a test too (`tests/tests.js`: race, chorus, philo, prodcons, round): no adds lost with the mutex, the
lines whole and in turn, no fork ever in two hands and the deadlock seen, every item used once, every note in time.
