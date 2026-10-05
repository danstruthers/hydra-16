# HyForth's status

Where HyForth (`modules/forth`, its core; `forthlib/`, its libraries) stands from step 6.6 on: the old HyForth's
pieces brought back, and HyForth as a shell, to the specification in [hyforth.md](hyforth.md).  Its first steps,
6.1 to 6.5 (the core, the word sets, the Hydra's words, libraries and scripts, the small core), are in
[status.md](status.md)'s table, and the plan's §16 ([../../docs/reimplementation-from-scratch.md](../../docs/reimplementation-from-scratch.md))
has each step as built.

## In short

HyForth is a Forth 2012 system (Core, Core Extension, Exception, Facility, File Access, Programming-Tools,
Search-Order, String, and a few Double words) that passes the Forth 2012 test suite's tests of them, its ROM core the
Core word set and the other word sets pre-compiled libraries it loads into its dictionary.  Since 6.5, at the user's
asking (October 2026), it has the old HyForth's pieces the user liked, each reviewed against the system as it is
(Forth 2012's words, rc and `/bin`, the C library, hylang) so that nothing is there twice and the same thing has the
same name in each language: names in lower case; `words` with each word's address and kind (a literal, immediate,
assembly or Forth); `libs`, `lib` and `-lib` over a table of the libraries loaded; a disassembler (and `see` of a
code word); `sys`; the bit words; random numbers; the terminal's words (hylang's names, conio's colours, the
standard's keys); sound (C's and hylang's names); `ctl`.  And it's a shell over rc, as hylang will be: `forth -l`,
where a line is Forth if its first word is a word or a number, and an rc command line otherwise; `cd`, `bind`,
`mount`, `unmount` and `newns` as words (an rc line can't change forth's own); the old prompt's format; statuses,
`&` and `wait`.  `/lib/shell` names each window's shell for init and wstart (rc if there's none), and `send` types a
line in another window, through the console's new `kbdin`.  Then (6.11), the first of what comparing it with
hylang's shell suggested: `%` (a line rc's, whatever its first word), the second prompt while a definition's compiled,
and programs as values with hylang's names: `sh-out`, `output-of`, `' words | wc -l` and `piped`, `spawn`.  And
(6.12) hylang's Hydra built-ins, as Forth names them: a directory's names (Gforth's `open-dir`, `read-dir` ...),
`=mkdir`, `get-dir`, `set-dir`, `unsetenv`, `note`, `note-group`, `on-note` (a program's note handler), `pause`,
`ior>text`.

```
PASS forth   HyForth (Forth 2012): the test suite (Core, Core Extension, Exception, Facility, File Access, Programming-Tools, Search-Order, String) in two sessions, its files INCLUDED from a card, the word sets' libraries REQUIREd from /lib/forth; scripts (forth file.fs, #!/bin/forth: arguments, REQUIRE from /lib/forth, a library, an error, a pipeline); at the console: startup.fs's Programming-Tools (.S), libraries REQUIREd (and again after a MARKER), a definition, KEY? and KEY, errors (a file's, the system's), SH, RUN, a sys- word, a bank, the constants library, Ctrl-C, BYE
PASS hyforth HyForth's additions (docs/hyforth.md): names in lower case; words (each word's xt, and whether it's a literal, immediate, assembly or Forth); the libraries loaded (libs), one not searched (-lib) and searched again (lib, where it was), the one with lib refused, a .fs one, one a MARKER takes out; disasm (the modes, the Rockwell opcodes, a jsr to a word), see of a code word (with disasm.fl, and without), sys, the bit words, random's numbers; the terminal's sequences, form, ekey and the keys (an arrow key, a character); the sound words (notes on the YM2151, a claim, the volume); ctl (and its error)
PASS fshell  HyForth as a shell (forth -l, shell.fl): its namespace and profile; a line Forth's or rc's by its first word (a number, a word, a pipeline, a redirection), or rc's by % (a program a word shadows); cd and the prompt (its format); a definition over lines (the second prompt); status and $status; & ($apid) and wait; programs as values: sh-out, output-of, a word's output a program's input (|, piped: one that ends first, Ctrl-C), spawn; Ctrl-C ending a program; errors, a usage; -lib shell and lib shell; exit
PASS fhydra  HyForth's Hydra words (hylang's layer 2): a directory read (open-dir, read-dir, close-dir), =mkdir, get-dir and set-dir (the prompt follows), ior>text; setenv, getenv, unsetenv; a note to itself taken by on-note's handler, between words and in a loop, and one it says no to (as Ctrl-C); pause
PASS lshell  the shell /lib/shell names (a card's: /bin/forth -l): init's in window 0, wstart's in a window made (Ctrl-] c: $window); send, a line typed in another window (#cN/kbdin), run there
```

The rest of the system's 44 tests pass with these changes (the console's, init's, wstart's and nslib's among them):
49 in all.

## Sizes

| What | Size |
| :--- | :--- |
| The core (`forth`, in place in its paged ROM bank) | 11,547 bytes, 70% of the bank (9,636 before 6.6: the library table, the shell's hooks, nslib for `forth -l`; 6.11's programs' code and output hook; 6.12's notes for a handler) |
| The dictionary free at the prompt | `forth`: 21,991 bytes (23,781 before 6.6: `tools.fl`'s new words, the core's table and its programs' arguments); `forth -l`: 18,594 (`shell.fl` too) |
| The libraries (`/lib/forth/NAME.fl`: their images and relocations) | `coreext` 1,609, `exception` 56, `file` 1,045, `tools` 3,567 (these four as forth starts); `facility` 2,186, `string` 1,204, `search` 690, `double` 118, `hydra` 3,085, `disasm` 1,642, `bits` 149, `random` 419, `sound` 683, `shell` 3,626 |

## Steps

| Step | | Notes |
|---|---|---|
| 6.6 The old HyForth's pieces: names, WORDS, libraries | Done | After 6.5, at the user's asking (October 2026), `docs/hyforth.md` the plan: the old HyForth's pieces the user liked, each reviewed against what the system has now (Forth 2012's words, rc and `/bin`, the C and hylang libraries) so that nothing is there twice and the same thing has the same name in each language; and HyForth as a shell (6.9).  Every name in lower case, as the old HyForth's were (the core's, the libraries', `hydra.fs`'s constants, `see`'s and `order`'s), still found in either case.  `words` as the old one showed them, with more: as many to a line as fit (`$COLUMNS`, else 80), each word's xt in hex and its kind, `l` a literal (a word whose code is a literal and `rts`: a `constant`'s, a `2constant`'s; the libraries' `bl`, `true`, `r/o` ... made the same way, `CONSTCODE`), `i` immediate, `a` or `f` assembly or Forth.  The core keeps a table of the libraries it loads (`libtab`: 16, each one's name, image, first and last header, and word list), which `words` uses to tell a library's words (assembly) from those the compiler made; and `libs` (the core, `forth`, then each library, `(name)` one not searched), `lib name` (`name.fl`, else `name.fs`, by REQUIRED; or a hidden one searched again, its headers back where they were in their word list, newest first, as a MARKER takes them) and `-lib name` (its headers out of their word list; its code stays, so what uses it still runs; `-lib tools` refused, -21: it has `lib`) in `tools.fl`.  A MARKER that takes a library out takes its record too (those at HERE or after, let go of as the table's read).  The `hyforth` test |
| 6.7 Machine code, bits, random numbers | Done | `disasm.fl` (`lib disasm`, 1.6K: not loaded as forth starts), the old monitor's tables (`os_rom/monitor/disasm.s`): `disasm ( addr n -- )`, n instructions, every 65C02 opcode (the Rockwell bit ones too), each line its address, bytes and instruction in lower case, a `jsr` or `jmp` to a word with its name (`jmp $AAA2  \ cr`); with it loaded, `see` of an assembly word shows its instructions (`code dup` ... `end-code`) to an `rts`, `rti`, `jmp`, `bra` or `stp` past every branch forward in it (or the next header).  `see` finds it by name, `(see-code)`, as a library calls only the core.  `sys ( addr a x y -- a x y p )` in `hydra.fl`: machine code called with its registers, its flags after.  `bits.fl` (`tbit`, `sbit`, `cbit`: `<<`, `>>` and `bool` aren't added, being `lshift`, `rshift` and `0<>`), `random.fl` (`rand`, `rand32`, `rseed`, and hylang's `random ( u -- u' )`: hylang's xorshift32, seeded from the clock and the ticks as it loads; checked against xorshift32's numbers) |
| 6.8 The terminal's words, sound, ctl | Done | In `facility.fl`, beside `page` and `at-xy`, the terminal's other sequences, named as hylang's `screen.hl` names them and coloured as C's conio numbers them (0-7 and their bright 8-15): `clear-line`, `clear-below`, `cursor-up` ... `cursor-left` (0: not moved, as the terminal's 0 would), `cursor-save`, `cursor-restore`, `cursor-off`, `cursor-on`, `form` (`$LINES` and `$COLUMNS`, else 24 by 80), `color`, `bgcolor`, the colours `black` ... `white` and `bright`, `plain`, `bold`, `dim`, `underline`, `blink`, `reverse`, `sgr` (the old `Acol`), `beep`; and the Facility Extension's keys: `ekey`, `ekey?`, `ekey>char`, `ekey>fkey`, `k-up` ... `k-f12` (the console's raw codes, `KEY_*`), the modifiers' masks (never set), `emit?`.  `sound.fl` with C's and hylang's names and order (the channel first): `snd-reset`, `snd-volume`, `snd-claim`, `snd-release`, `snd-reg` (the old `ywrite`), `snd-patch`, `snd-note`, `snd-off`, `snd-vol`, `snd-pan`, `snd-bend`, `snd-drum`; a channel's command written with the channel in one write; a failure the system's error, named (`/dev/snd: busy`).  `note`, `play`, `sndtest` aren't words (a note is Plan 9's; the player is `play`).  `ctl ( c-addr1 u1 c-addr2 u2 -- )` in `hydra.fl`.  `vols` isn't added: it's `df`.  The forth test's console session waits longer after `hydra.fs` (171 lines compiled, three searches of the dictionary each, a little slower with Facility's words: the window keeps only 64 keys typed ahead) |
| 6.9 HyForth as a shell | Done | `shell.fl`, and `forth -l`.  At the console's prompt a line is Forth if its first word is a word or a number, and an rc command line otherwise, run whole by `rc -c` and waited for (pipes, redirections, globs, quoting, `$x`: rc's); one ending in `&` isn't waited for (`$apid`, a note group of its own).  What an rc line can't do to forth (it's another task: forth's directory and namespace stay) are words that parse rc's way (quotes, `$name`): `cd`, `bind`, `mount`, `unmount`, `newns`; and `prompt` (the old format: `%v%d> `, `%p`, `%t`, `%w`, `%%`; forth's own, not rc's `$prompt`), `status` and `$status` (rc's: the message, or the code), `exits`, `wait`, `exit` (typed at the prompt it ends forth, as rc's; in a definition it's Forth's), `getenv`, `setenv`, `send` (6.10).  The core finds the shell by name (`(shell-prompt)` before each line typed at the console, in place of ` ok`, on a new line if forth's output since didn't end one: `lastc`, the last character out; `(shell-line)` for each), so `-lib shell` or a MARKER makes forth a plain Forth again.  `forth -l` builds its namespace first, with the SDK's nslib in the core (1K: the shell can't load a library before there's a `/lib`), its zero page forth's scratch and its buffers the dictionary's top (nslib's new `NS_ZP` and `NS_BSS`: init, rc and the tests use it as they did), then `startup.fs` and `/lib/forth/profile.fs`, which loads `shell.fl`, puts its window at `/dev` and takes its notes.  The `fshell` test |
| 6.10 Which shell; send | Done | init reads `/lib/shell` each time it starts window 0's shell or wstart: its first line is the shell's program and arguments (`/bin/forth -l`), else `rc -l`; wstart is given it as its arguments, and runs in init's namespace now (it had an empty one of its own), so it finds the program as init does.  A card's `/lib/shell` (or the shared RAM disk's: each shell's `/ram` is its own) names every window's.  The console has a file more in each window, `kbdin` (Plan 9's rio's): a write's bytes are the window's keys, as typed (63 at most, its keys' queue); `send n line` writes a line and its Enter there, so another window's shell runs it, whatever shell it is.  The `lshell` test |
| 6.11 The shell's next: `%`, the second prompt, programs as values | Done | The first of what comparing the shell with hylang's suggested (below).  `%` sends the rest of a line to rc, whatever its first word (`% free`, past a Forth word `free`); while a definition's compiled the prompt is the second one, rc's tab (`prompt2` sets it).  Programs as values, hylang's names: `sh-out ( c-addr u -- c-addr2 u2 )` (a command line's output; `status` its code), `output-of ( xt -- c-addr u )` (what a word writes), `' words \| wc -l` (`\|`: the rest of the line the command, the word's output its input, through a pipe; the old `words \| wc` without a shell's grammar in Forth) and `piped ( xt c-addr u -- )`, `spawn ( c-addr u -- task )` (a program not waited for: `$apid`).  The strings are in the dictionary's free space, 256 bytes past HERE (till something's added to it).  The code that starts and waits for programs is the core's now, headerless (`fprog.inc`: rc's arguments, a program's, `SPAWN` with an fd map, the wait and its status), as what two libraries use is: `hydra.fl`'s `sh` and `run` and `shell.fl` share it (`hydra.fl` 524 bytes smaller).  The output's flush takes a hook (`out_hook`), which `output-of` (into the string) and `piped` (into the pipe) give it while the word runs, using only r0 and r1 (it comes in the middle of any word); the console's last character is kept as it was, so the next prompt isn't after a blank line.  A program that stops reading first (`head`) doesn't stop forth (its writes are dropped); Ctrl-C stops the word and the program both.  The `fshell` test |
| 6.12 The Hydra's words (hylang's layer 2) | Done | In `hydra.fl`, hylang's Hydra built-ins as Forth names them (Gforth's where Forth has some; none a program's, as the shell runs a word before a program): `get-dir`, `set-dir` (the shell's `cd`'s stack form), `open-dir`, `read-dir`, `close-dir` (a directory's names, from its stat records), `=mkdir` (`mkdir` is the program), `note`, `note-group`, `on-note`, `pause` (`YIELD`), `ior>text` (`ERRSTR`); `unsetenv` in `shell.fl`, beside `getenv` and `setenv`.  `on-note ( xt -- )`: the core's note handler marks a note (but Ctrl-C's and kill's) for the program's handler (`intr` $80, `note_pend`; Ctrl-C's $C0), and the next word interpreted or loop step (`intr_check`, which the compiled loops now `jsr`) runs `xt ( n -- flag )` there: true, forth goes on; false, THROW -28 as for Ctrl-C (a handler no longer in the dictionary: the same).  A wait the note ends (KEY, a line's read) waits again, and MS ends early (`intr_wait`), the next word taking it; the libraries' own loops (`words`, `see`, `disasm`) stop for Ctrl-C alone.  Preemption's words aren't added (`hold` is Forth's; the sys- words say it).  The `fhydra` test |

## HyForth and hylang's shell

October 2026, after 6.10: HyForth against hylang's plan for its shell (`hylang.md`: the prompt, and the four layers
of the Hydra it reaches), so that the two shells can do the same things.

| Area | hylang's plan | HyForth | Gap |
| :--- | :--- | :--- | :--- |
| The line's rule | `(`, `{`, `[` or a prefix: hylang's; else rc's | A word or a number first: Forth's; else rc's; `%`, rc's | Alike (`%` since 6.11) |
| `cd`, `bind`, `mount`, `unmount`, `newns` typed | To `rc -c` (which can't change hylang's directory or namespace) | Words of the shell, parsing rc's way | hylang's |
| `&`, `$apid`, `$status` | `&` rc's: `$apid` and `$status` lost with it | Kept; `wait` | hylang's |
| The prompt | `hylang> `; more lines: the brackets open, ` <` | A format (`%v%d> `, `%p`, `%t`, `%w`); while compiling, the second (rc's tab: `prompt2`) | hylang's isn't a format |
| A login shell | Not said | `forth -l`: newns, `startup.fs`, `profile.fs` | hylang's (`/lib/shell` is ready for it) |
| Programs | `run`, `sh`, `sh-out` (the output, a string), `sh` with input, `spawn`, `wait`, `kill`, `pid` | `sh`, `run`, `sh-out`, `output-of`, `\|` and `piped` (a word's output a program's input), `spawn`, `&` typed, `wait`, `status` | Alike since 6.11 (`kill`, `pid`: the program, `sys-getpid`) |
| Files | `ls`, `dir`, `stat`, `exists?`, `dir?`, `mkdir`, `cwd`, `copy-file`, `glob` | File Access; `open-dir`, `read-dir`, `close-dir`, `=mkdir`, `get-dir`, `set-dir` (6.12); the sys- words | Glob, `copy-file` (`cp`) |
| The environment | `env`, `$name`, all of it, `setenv`, `unsetenv` | `getenv`, `setenv`, `unsetenv`; `$name` in the shell's words; `/env` read as a directory | Alike |
| The clock | `time`, `date`, `ticks`, `sleep` | `time&date`, `ms`, `sys-time`, `sys-ticks` | Small |
| Notes | `note`, `note-group`, `on-note` | `note`, `note-group`, `on-note` (6.12) | Alike |
| Tasks | `ps`, `task-info`, `yield`, `sleep-until`, `hold` | The programs; `pause` (6.12); the sys- words | Preemption's words (`hold` is Forth's) |
| Memory and banks | `peek`, `poke`, banks, segments | `c@` `@` ..., `bank!`, `bank-window`, `seg-bank!`, sys- words | Alike |
| The screen and keys | `screen.hl` (8 functions); `key`'s atoms | The terminal's words; `ekey`, `k-up` ... | hylang's: `screen.hl` to have HyForth's set |
| The devices | `cons`, `gpio`, `i2c`, `spi`, `snd`, `disk`, `proc`, `clock`, `pc`: libraries over their files | `sound.fl` | HyForth's, the most |
| Every call | `sys-` functions | `sys-` words | Alike |
| Libraries, start | `use`, `load` (`/lib/hylang`); `globals.hl`, `profile.hl` | `lib`, `require` (`/lib/forth`); `startup.fs`, `profile.fs` | Alike |

## Next

1. **The device libraries (hylang's layer 3)** as source (`.fs`) over the devices' files, hylang's names, each a
   worked example too: `cons` (`window`, `new-window`, `show-window`), `gpio`, `i2c`, `spi`, `disk`, `proc`, `clock`,
   `pc`; and `sound`'s `note-of` and `tune`.
2. **Housekeeping:** the dictionary's searches (slower with each library loaded: `hydra.fs`'s 171 lines; an index by
   length or first letter); the helpers each library has its own copy of; a guide for using HyForth (`docs/using/
   hyforth.md` is the old one's); the board (`forth -l` as a window's shell, at 115200, a real card's `/lib/shell`).
3. **The standard's other word sets:** Memory-Allocation (the plan's "later"; its `free` would shadow the `free`
   program at the prompt: `% free`), the rest of Double, Locals, Block.
4. **For hylang's prompt, so the two stay alike:** `cd`, `bind`, `mount`, `unmount` and `newns` the shell's own, an
   `&` at a line's end kept (`$apid`, `wait`), `$status`; `hylang -l` (newns by nslib, a profile for its window), so
   `/bin/hylang -l` in `/lib/shell` works; the prompt's format codes; `screen.hl` with HyForth's terminal set
   (danlang's first).

Not kept from the old HyForth: the prompt's `%l` (a card's label: its ctl read for each prompt), `rand32`'s Galois
generator (hylang's xorshift32 in its place), `syscall`, the training scripts, memory records, `halloc` and the rest
the system does another way now (`hyforth.md` has each one).  `send` reaches a window's shell only if its keys' queue
has room (63 keys): a longer line is cut.
