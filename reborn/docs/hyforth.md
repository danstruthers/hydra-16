# HyForth: the old pieces back, and a shell

HyForth (`modules/forth`, `forthlib/`) is a Forth 2012 system, phase 6 of the plan: a small core in the paged ROM and
the other word sets as pre-compiled libraries (`/lib/forth/NAME.fl`).  In October 2026 the user asked for two more
things: some pieces of the old HyForth (`../os_rom/hyforth`, `../docs/using/hyforth.md`) back, and HyForth run as a
shell over rc, as hylang will be ([hylang.md](hylang.md), "The prompt").  This is the specification for both: what was
asked for, what each piece meets in the system as it is now and how that's settled, and the steps (6.6 on).

## What's asked for

* **Names in lower case**, as the old HyForth had them: every word the system defines (`dup`, `if`, `sys-open`,
  `o_read`).  Names are still found in either case, so programs written in upper case (the Forth 2012 test suite)
  run as they did.
* **WORDS as the old HyForth showed them**, a few to a line with each word's address in hex, and now its kind too:
  whether it's a literal, whether it's immediate, and whether it's assembly or Forth.
* From the old HyForth: `disasm` and `sys`; the sound words; `rand`, the bit words and colours; `libs`, `lib` and
  `-lib`; the shell's prompt (the current directory, from a format); tasks and exit statuses; `ctl` and `vols`.
* **HyForth as a shell**: at its prompt a line is Forth if its first word is a Forth word or a number, else it's an
  rc command line.  A window's shell is chosen by a setting (`/lib/shell`), rc unless it names another.

## The review: what each piece meets

The old HyForth was the shell itself, and much of what it built in is now done elsewhere: rc's grammar and built-ins,
the programs in `/bin`, Forth 2012's own words, the sys- words, and the C and hylang libraries' names for the same
things.  Each piece is checked against those, so that nothing is there twice and the same thing has the same name
and order in each language.

| Piece | The old HyForth's | What's there now | Settled |
| :--- | :--- | :--- | :--- |
| Lower case | All lower case | Upper case in the sources; found in either case | Every name lower case: the core, the libraries, `hydra.fs`'s constants (`o_read`, `e_noent`).  `see`, `words`, `order` and the errors show them so |
| WORDS | ` AAAA: name \|`, four to a line: the header's address | Names only | The format below |
| `disasm` | `( addr n -- addr' )`, the monitor's disassembler | `see` decompiles a definition (calls, literals, branches; other code as `$xx` bytes); `dump` | `disasm ( addr n -- )`: n instructions, every 65C02 opcode (the Rockwell bit ones too: the W65C02 has them), each line its address, bytes and instruction, a `jsr` or `jmp` to a word with the word's name (`jmp $AAA2  \ cr`).  The old monitor's tables.  A library of its own, `disasm.fl` (`lib disasm`: 1.6K, so not loaded as forth starts); with it loaded, `see` of an assembly word shows its instructions (`code dup` ... `end-code`), to an `rts`, `rti`, `jmp` or `bra` past every branch forward in it.  `see` finds it by name (`(see-code)`), as a library calls only the core |
| `sys` | `( addr a x y -- a x y p )`; `syscall ( addr a y -- x )` | A sys- word for each system call; `execute` (an xt, no registers) | `sys` as it was, in `hydra.fl` (8-bit BASIC's name for calling machine code).  `syscall` isn't needed: the sys- words call the system.  forth's data stack is .X and the program's zero page (`$22`-`$7F`): `sys` keeps .X, and the code it calls must leave that zero page alone |
| Sound | `sndinit`, `ywrite ( xxaa -- f )`, `patch ( p ch -- )`, `note ( n ch -- )`, `noteoff ( ch -- )`, `sndtest`, `sndstop`, `play` | `/dev/snd` and `/dev/sndctl`; C's `snd_patch (ch, p)` ...; hylang's `(snd-patch ch n)` ...; `play`, the song player | `sound.fl` with C's and hylang's names and order, the channel first: `snd-reset` (was `sndinit`), `snd-reg ( reg val -- )` (was `ywrite`), `snd-patch ( ch p -- )`, `snd-note ( ch n -- )`, `snd-off ( ch -- )`; and what the driver has that the old words didn't: `snd-vol ( ch v -- )`, `snd-pan ( ch pan -- )`, `snd-bend ( ch n -- )`, `snd-drum ( ch n -- )`, `snd-volume ( v -- )` (the master), `snd-claim ( mask -- )`, `snd-release ( mask -- )`.  Not as words: `note` (on the Hydra a note is Plan 9's: hylang's `note`, the kernel's `NOTE`, `/proc/N/note`), and `play`, `sndtest` and `sndstop`, which the player does at the shell: `play /rom/songs/test.zsm &`, then `kill $apid` |
| Random numbers | `rand ( -- u )`, `rand32 ( -- ud )`, `rseed ( ud -- )`: a Galois LFSR | hylang's `(random n)`: 0 to n-1, xorshift32 seeded from the ticks and the clock | `random.fl`: `rand`, `rand32` and `rseed` as they were, and `random ( u -- u' )` as hylang's; hylang's generator and seed |
| Bits | `tbit ( n b -- n f )`, `sbit ( n b -- n' )`, `cbit ( n b -- n' )`, `<<`, `>>`, `bool` | `lshift`, `rshift` (Core), `0<>` (Core Extension) | `bits.fl`: `tbit`, `sbit` and `cbit` as they were (bits 0-15; `tbit` keeps n).  `<<` and `>>` are `lshift` and `rshift`, and `bool` is `0<>`, so they aren't added |
| ANSI | `Acls`, `Ascr ( c r -- )`, `Acol ( c -- )` (a raw attribute) | Facility's `page` and `at-xy ( col row -- )`; C's conio; hylang's `screen.hl` | `page` and `at-xy` are `Acls` and `Ascr` already.  The rest is the set below, in `facility.fl`, and `sgr ( n -- )` is `Acol` |
| `libs`, `lib`, `-lib` | ROM libraries as bits of `LIBSET`, four file libraries | `require` (loads a file once), `marker` (takes everything since out), Search-Order's word lists and `library` | `lib name` is `require name.fl` (else `name.fs`), or the library searched again if it was hidden (its headers back where they were, so a `marker` still finds them in order); `-lib name` stops it being searched without unloading it (definitions that use it still run); `libs` lists the libraries loaded, the core (`forth`) first, `(name)` for a hidden one.  A file's library, that is: a word list (`library`, `wordlist`) is the Search-Order words' to show and hide (`order`, `also`, `previous`), not `-lib`'s.  `marker` still takes libraries out.  The core keeps a table of the libraries it loads (16: each one's name, image, first and last header, word list): what `words` uses to tell assembly from Forth too.  In `tools.fl`, which forth starts with; `-lib tools` is refused (-21), as it would take `lib` away |
| The prompt | `prompt ( sz -- )`: `%v` card, `%d` directory on it, `%p` path, `%l` label, `%t` task | rc's `$prompt`, a list (`'% '` and a tab), in the environment | forth's own format, not the environment's (an rc started from forth would read it as its prompt): `prompt ( c-addr u -- )`, `%v` (`0:` under `/sd/0`, else nothing), `%d` (the path on the card, else the whole path), `%p` (the whole path), `%t` (the task), `%w` (the window), `%%`.  The shell's default is the old one, `%v%d> ` (`0:/games> `, `/ram> `).  `%l` isn't kept (it read the card's ctl for each prompt).  With no prompt set, forth says ` ok` after each line, as now |
| Tasks and statuses | `status`, `exits`, `wait`, `&` and `$apid`, `send N line`, `ps`, `kill`, `sleep`, `fg`, `shell`, `forth` | rc: `$status`, `$apid`, `wait`, `&`, `exit`; programs `ps`, `kill`, `sleep`; windows in place of `fg` | `status ( -- n )`: the last command's code (an rc line's, `sh`'s, `run`'s).  `exits ( n -- )` ends forth with that code; `exit` typed at the prompt ends it too, as rc's (in a definition it's Forth's).  A trailing `&` on an rc line: forth starts it without waiting, in a note group of its own (rc's way), and sets `$apid` in its environment (so `kill $apid` works); `wait ( task -- )` waits for it.  A `&` inside a line is rc's.  `send N line`: window N gets the line as if typed, through a new file of the console's, `#cN/kbdin` (so rc can do it too: `echo ls >'#c2/kbdin'`).  `ps`, `kill`, `sleep` are the programs; `fg`, `shell` and `forth` are windows now (Ctrl-] c) |
| `ctl` and `vols` | `ctl ( sz-file sz-text -- )`; `vols`: each card, its label, its free space | File Access's `open-file`, `write-file`; rc's `echo -n text >file`; `df`: each disk's kind, size, free space and label | `ctl ( c-addr1 u1 c-addr2 u2 -- )`, the text written to the file in one write, in `hydra.fl`.  `vols` is `df`, which runs by its name at the shell, so it isn't added |

## WORDS

Each word in a column of its own: its code's address (its xt) in hex, three flags, and its name; as many to a line
as fit the screen's width (`$COLUMNS`, else 80), the newest first.

```
 3356 l-f five            3348 --f twice           333A -if x
 1E6D --a lib             1E10 --a libs            1C3A --a words
 A759 l-a bl              A74A --a >in             A73A --a state
```

| Flag | Means |
| :--- | :--- |
| `l` | A literal: the word pushes a number and does nothing else (a `constant`'s, a `2constant`'s, a definition that's only a number; the libraries' `bl`, `true`, `r/o` ... are made the same way, `CONSTCODE`) |
| `i` | Immediate: it runs while compiling |
| `a` / `f` | Assembly (the core's, or a library's, `NAME.fl`) or Forth (made by the compiler from source: `:`, `create`, `constant` ...) |

## The terminal's words

The Hydra's console is an ANSI terminal (`page` and `at-xy` already send its sequences), so these are its
sequences too, in `facility.fl`.  They match the other languages: Forth 2012 where it has a word (`page`, `at-xy`
from 0, the keys); hylang's `screen.hl` names (`clear-line`, `bold`, `plain`, `cursor-off`, `cursor-on`, `color`);
C's conio's colour numbers (0-7 the terminal's eight, 8-15 their bright ones) and screen size (`$LINES` and
`$COLUMNS`, else 24 by 80); and the console's raw keys (`KEY_UP` ...).

| Word | Stack | Sends, or does | Elsewhere |
| :--- | :--- | :--- | :--- |
| `page` | `( -- )` | `CSI 2J`, `CSI H`: the screen cleared, the cursor at the top left (there now) | hylang `cls`, C `clrscr` |
| `at-xy` | `( col row -- )` | `CSI row+1;col+1 H`, from 0 (there now) | hylang `at` (from 1), C `gotoxy` |
| `clear-line` | `( -- )` | `CSI K`: the rest of the line | hylang `clear-line` |
| `clear-below` | `( -- )` | `CSI J`: the rest of the screen | |
| `cursor-up`, `cursor-down`, `cursor-right`, `cursor-left` | `( n -- )` | `CSI n A`, `B`, `C`, `D` | |
| `cursor-save`, `cursor-restore` | `( -- )` | `ESC 7`, `ESC 8` | |
| `cursor-off`, `cursor-on` | `( -- )` | `CSI ?25l`, `CSI ?25h` | hylang, C `cursor` |
| `form` | `( -- rows cols )` | The screen's size: `$LINES` and `$COLUMNS`, else 24 and 80 | C `screensize` |
| `color` | `( c -- )` | The text's colour, 0-15: `CSI 30`-`37 m`, `90`-`97 m` | hylang `color`, C `textcolor` |
| `bgcolor` | `( c -- )` | The background's, 0-15: `CSI 40`-`47 m`, `100`-`107 m` | hylang `color`'s second, C `bgcolor` |
| `black` `red` `green` `yellow` `blue` `magenta` `cyan` `white` | `( -- c )` | 0-7 | C's `COLOR_*`, hylang's atoms |
| `bright` | `( c -- c' )` | Its bright one (8 more) | C `COLOR_GRAY` ... |
| `plain` | `( -- )` | `CSI 0m`: every attribute off, the terminal's own colours | hylang `plain` |
| `bold`, `dim`, `underline`, `blink`, `reverse` | `( -- )` | `CSI 1m`, `2m`, `4m`, `5m`, `7m` | hylang `bold`, C `revers` |
| `sgr` | `( n -- )` | `CSI n m`: any other attribute (the old `Acol`) | |
| `beep` | `( -- )` | A BEL: the console rings the bell | hylang `beep` |
| `ekey` | `( -- u )` | A key, raw: a character, or a cursor or function key (`k-up` ...), as `key` reads them | Facility Ext; hylang `(key)`, C `cgetc` |
| `ekey?` | `( -- flag )` | Whether one is waiting | hylang `(key?)`, C `kbhit` |
| `ekey>char` | `( u -- u false \| char true )` | | Facility Ext |
| `ekey>fkey` | `( u -- u false \| x true )` | | Facility Ext |
| `k-up` `k-down` `k-left` `k-right` `k-home` `k-end` `k-prior` `k-next` `k-insert` `k-delete` `k-f1` ... `k-f12` | `( -- x )` | The console's codes (`KEY_UP` ... `KEY_F12`) | Facility Ext; C's `CH_*` |
| `k-shift-mask`, `k-ctrl-mask`, `k-alt-mask` | `( -- x )` | Never set: the console decodes no modifiers | Facility Ext |
| `emit?` | `( -- flag )` | Always true | Facility Ext |

## The Hydra's words (hylang's layer 2)

What hylang has built in for the Hydra, as Forth names it (6.12): Gforth's names where Forth has some, a name that
isn't a program's (the shell's rule runs a word before a program), and the sys- words still there for the raw call.

| Word | Stack | Does | hylang |
| :--- | :--- | :--- | :--- |
| `get-dir` | `( c-addr1 u1 -- c-addr2 u2 )` | The current directory, in the buffer (Gforth's) | `cwd` |
| `set-dir` | `( c-addr u -- wior )` | The current directory changed (Gforth's; the shell's `cd` is its parsing form) | `cd` |
| `open-dir`, `read-dir`, `close-dir` | `( c-addr u -- wdirid wior )`, `( c-addr u1 wdirid -- u2 flag wior )`, `( wdirid -- wior )` | A directory's names, one at a time (Gforth's: a directory reads as stat records, which `read-dir` takes the names of) | `ls`, `dir` |
| `=mkdir` | `( c-addr u wmode -- wior )` | A directory made (Gforth's name: `mkdir` is the program's) | `mkdir` |
| `unsetenv` | `( c-addr u -- )` | A variable of the environment removed (beside `getenv` and `setenv`, in `shell.fl`) | `unsetenv` |
| `note`, `note-group` | `( task n -- )`, `( group n -- )` | A note sent (Plan 9's postnote) | `note`, `note-group` |
| `on-note` | `( xt -- )` | The notes that come (but Ctrl-C and kill) given to xt `( n -- flag )` at the next word interpreted or loop step: true, forth goes on; false, as Ctrl-C (THROW -28).  0: none | `on-note` |
| `pause` | `( -- )` | The other tasks' turn (Forth's name for it: `YIELD`) | `yield` |
| `ior>text` | `( ior -- c-addr u )` | A system error's text (`not found`) | `errstr` |

A note for a handler is taken where Ctrl-C is: the core's note handler marks it, and the next word interpreted, or a
loop's step, runs the handler (a wait it ends, KEY or a line's read, waits again first; MS ends early).  Preemption's
words aren't added: `hold` is Forth's (pictured output), and `sys-preempt-off` and `sys-preempt-on` say it.

## The shell

**The rule.**  A line typed at the shell's prompt is Forth if its first word is a Forth word (found in the search
order) or a number, and an rc command line otherwise, run whole by rc (`rc -c`), waited for, its code `status`.  So
pipes, redirections, globbing, quoting and `$x` are rc's, and Forth isn't given a shell grammar of its own (the plan's
I2).  A name in both is Forth's: `.` (rc's `. file` is `include file`) and `#` (Forth's comment is `\`).  Only a line
typed at the prompt goes by the rule: not a line while a definition is being compiled, a file being included, or an
`evaluate`.

**What can't be rc's.**  An rc line runs in a task of its own, which can't change forth's current directory or (a
copy made as it changes it) its namespace, so these are Forth words, parsing their arguments as rc's built-ins do
(`-a`, `-b`, `-c`; `'...'` quoting, `''` a quote in it; `$name`, the environment's variable, its first word): `cd
[dir]` (none: `$home`), `bind`, `mount`, `unmount`, `newns`; a usage that isn't right says the word's (`usage: bind
[-a|-b] [-c] new old`), a failure the system's text (`/none: not found`).  In a definition the sys- words do the same
(`sys-chdir`, `sys-bind` ...): no second form of each (the plan's I2).

**Statuses.**  `status ( -- n )` is the last rc line's code (or `wait`'s), and `$status` rc's: the program's exit
message (`interrupt`), or its code if it has none.  A line ending in `&` isn't waited for: its task is `$apid`, in a
note group of its own, as rc's `&` (rc says nothing either); `wait ( task -- )` waits for it.  Ctrl-C ends the
program running (the window's note group), and the shell goes on, on a new line, as rc does.  `exit` typed at the
prompt ends forth (its code `status`), as rc's; in a definition it's Forth's.  `exits ( n -- )` ends it with code n.
`getenv ( c-addr1 u1 -- c-addr2 u2 )` and `setenv ( c-addr1 u1 c-addr2 u2 -- )` (the name first, as C's and hylang's)
read and set the environment: a list's words with spaces between them; a value set is one word, as rc keeps one.

**`forth -l`**, a login shell, as `rc -l`: `newns` (forth's core, with the SDK's nslib, its buffers the dictionary's
top, free then: before it there's no `/lib` to load anything from), `startup.fs`, then `/lib/forth/profile.fs` (the
ROM disk's, or a card's or the RAM disk's in its place, through the `/lib` union), which loads `shell.fl` and does
what rc's profile does: the window at `/dev` (`$window`'s), its notes to forth's note group.

**Which shell.**  init reads `/lib/shell` each time it starts window 0's shell or the windows' starter (wstart): its
first line is the shell's program and arguments (`/bin/forth -l`); with none, `rc -l` as before.  wstart is given it
as its arguments, and runs in init's namespace (not an empty one of its own now), so the program is found as init
finds it; each window's shell still starts in an empty namespace of its own, which its profile builds.  Each shell's
`/ram` is its own area, so a `/lib/shell` for every window is a card's (`/sd/0/lib/shell`) or the shared RAM disk's
(`/sram/lib/shell`, till the next reset).  hylang will be chosen the same way.

**`send n line`** types the line, and Enter, in window n, whatever shell is there: it's written to the window's
`#cN/kbdin`, a file of the console's (rio's `kbdin`: a write's bytes are the window's keys, as typed; 63 at most, as
its keys' queue holds), so rc can do it too: `echo ls >'#c2/kbdin'` (its LF is a new line, as the console's Enter).

**`%`** sends the rest of a line to rc whatever its first word: `% free` runs the `free` program though a Forth word
`free` shadows it (as Memory-Allocation's will), and a script's line can be rc's the same way.  While a definition is
being compiled the prompt is the second one, rc's (a tab), which `prompt2 ( c-addr u -- )` sets as `prompt` does the
first.

**Programs as values**, with hylang's names (its `sh-out`, `output-of`, `sh` with input, `spawn`), beside `sh` and
`run` (`hydra.fl`'s): `sh-out ( c-addr u -- c-addr2 u2 )` (a command line's output, a string; its code `status`),
`output-of ( xt -- c-addr u )` (what a word writes: forth's own output, not a program's), `' words | wc -l` (`|` takes
the rest of the line as the command, and what the word writes is its input: the old HyForth's `words | wc`, without a
grammar of the shell's in Forth) and its stack form `piped ( xt c-addr u -- )` (as `include` and `included`), and
`spawn ( c-addr u -- task )` (a program, as `run`'s, not waited for: `$apid`, `wait`).  The strings are in the
dictionary's free space, 256 bytes past HERE: they last till something's added to the dictionary (the word given
`output-of` mustn't add any).  A program that stops reading early (`head`) leaves forth going on; Ctrl-C stops the word
and the program both.  The core has the code that starts programs and waits for them (`fprog.inc`, headerless: `sh`,
`run` and the shell's rc lines use it, as what two libraries use is the core's), and its output's flush takes a hook
(`out_hook`) that `output-of` and `piped` give it while they run.

**`shell.fl`** holds the shell's words.  The core finds two of them by name, as `see` finds `(see-code)`, so they
work only while the library's loaded and searched: `(shell-prompt)`, the prompt, shown before each line typed at the
console (in place of ` ok` after it; none while a definition's being compiled; on a new line if forth's output since
didn't end one), and `(shell-line) ( -- flag )`, which runs a line by the rule.  So `lib shell` makes any forth a
shell, and `-lib shell` (or a `marker` that takes it out) a plain Forth again.

## The steps

6.6 to 6.10 done (October 2026), then 6.11 and 6.12, from comparing the shell with hylang's;
[forth-status.md](forth-status.md) has each one's notes, the sizes and what's next.  The tests: `forth` (the Forth
2012 suite, still passing), `hyforth` (6.6-6.8), `fshell` (6.9 and 6.11), `lshell` (6.10) and `fhydra` (6.12).

| Step | | Tested |
| :--- | :--- | :--- |
| 6.6 | Lower case, `words`, the library table, `libs`, `lib`, `-lib` | The suite; `words`' flags (a constant, a definition, an immediate one, the core's and a library's); a library hidden and searched again, where it was; `-lib tools` refused; a `.fs` one; one a `marker` takes out |
| 6.7 | `disasm.fl` (and `see` with it), `sys`, `bits.fl`, `random.fl` | The modes and the Rockwell opcodes, a `jsr` to a word; `see` of a code word with `disasm.fl` and without; `sys` of code in RAM; the bit words; `random`'s numbers against xorshift32's |
| 6.8 | The terminal's words, `sound.fl`, `ctl` | The sequences sent; `form`; an arrow key and a character read raw; notes keyed on (the emulator's YM2151), a claim, the volume; `ctl` and its error |
| 6.9 | The shell: `shell.fl`, `forth -l`, `profile.fs` | The rule (a number, a word, a pipeline, a redirection), `cd` and the prompt (its format), a definition over lines, `status` and `$status`, `&` and `wait`, Ctrl-C, errors and a usage, `-lib shell` and `lib shell`, `exit` |
| 6.10 | `/lib/shell` for init and wstart; `send` and `#cN/kbdin` | A card's `/lib/shell`: forth in window 0 and in a window made (`$window`); a line sent to window 0, run there |
| 6.11 | The shell's next: `%`, the second prompt (`prompt2`); programs as values: `sh-out`, `output-of`, `\|` and `piped`, `spawn`; the programs' code in the core (`fprog.inc`), the output's hook | `% free` past a word `free`; a definition's second line (its tab); a line's output and a word's, strings; a word's output into `wc` (`\|`, `piped`), into `head`, which ends first, and stopped by Ctrl-C; `spawn` and `wait`; `sh` and `run` as before (the forth test) |
| 6.12 | The Hydra's words (hylang's layer 2): the directories' (Gforth's), `=mkdir`, `unsetenv`, `note`, `note-group`, `on-note` (the core's note handler and its polls), `pause`, `ior>text` | A directory read, one made; the directory set and got (the prompt follows); an error's text; a variable set, read, removed; a note to itself taken by a handler between words and in a loop, and one it says no to |
