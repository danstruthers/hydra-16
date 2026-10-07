# HyForth: the old pieces back, and a shell

HyForth (`modules/forth`, `forthlib/`) is a Forth 2012 system, phase 6 of the plan: a small core in the paged ROM and
the other word sets as pre-compiled libraries (`/lib/forth/NAME.fl`).  In October 2026 the user asked for two more
things: some pieces of the old HyForth (`../old/os_rom/hyforth`, `../old/docs/using/hyforth.md`) back, and HyForth run as a
shell over rc, as hylang is too ([hylang.md](hylang.md), "The prompt").  This is the specification for both: what was
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
| Sound | `sndinit`, `ywrite ( xxaa -- f )`, `patch ( p ch -- )`, `note ( n ch -- )`, `noteoff ( ch -- )`, `sndtest`, `sndstop`, `play` | `/dev/snd` and `/dev/sndctl`; C's `snd_patch (ch, p)` ...; hylang's `(snd-patch ch n)` ...; `play`, the song player | `sound.fl` with C's and hylang's names and order, the channel first: `snd-reset` (was `sndinit`), `snd-reg ( reg val -- )` (was `ywrite`), `snd-patch ( ch p -- )`, `snd-note ( ch n -- )`, `snd-off ( ch -- )`; and what the driver has that the old words didn't: `snd-level ( ch v -- )` (`snd-vol`, its first name),
`snd-regs ( c-addr -- )`, `snd-pan ( ch pan -- )`, `snd-bend ( ch n -- )`, `snd-drum ( ch n -- )`, `snd-volume ( v -- )` (the master), `snd-claim ( mask -- )`, `snd-release ( mask -- )`.  Not as words: `note` (on the Hydra a note is Plan 9's: hylang's `note`, the kernel's `NOTE`, `/proc/N/note`), and `play`, `sndtest` and `sndstop`, which the player does at the shell: `play /rom/songs/test.zsm &`, then `kill $apid` |
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

## Compile-only words

Forth 2012 leaves some words' interpretation semantics undefined, and interpreting one is an ambiguous condition
whose THROW code is -14 ("interpreting a compile-only word").  Interpreted, HyForth's broke it: `1 >r` typed at the
prompt pushed onto the interpreter's own return stack and ended forth (a login shell with it), and `1 then` stored
HERE at address 1.  So they're compile-only (6.14): outside a definition the text interpreter THROWs -14, said with
the word's name (`>r: compile only`), and forth goes on.  In a definition they're compiled as before.

| Compile-only | Words |
| :--- | :--- |
| The return stack's | `>r`, `r>`, `r@`, `2>r`, `2r>`, `2r@`, `unloop`, `i`, `j`, `exit` (typed at the shell's prompt, `exit` is the shell's, which ends forth) |
| The control structures' | `if`, `else`, `then`, `begin`, `until`, `while`, `repeat`, `again`, `ahead`, `do`, `?do`, `loop`, `+loop`, `leave`, `case`, `of`, `endof`, `endcase` |
| The other compiling words | `;`, `recurse`, `does>`, `literal`, `2literal`, `sliteral`, `[']`, `[char]`, `postpone`, `[compile]`, `."` (`.(` is its interpreted form), `abort"`, `c"` |

Not compile-only: the words another word set gives interpretation semantics (`s"` and `s\"`, File Access's; `to`,
`is`, `action-of`), `[`, and `cs-pick` and `cs-roll` (the control-flow stack is the data stack, so they're only
stack words then).  The mark is in the byte an inline word has after its name (its code's length): bit 7,
`F_COMPILE`; a compile-only word that's called (`i`, or an immediate one) has that byte with a length of 0.  A
`synonym` of one is one too.  `'` and `find` still give its xt, as the standard has them.

## Finding words: the index

A word list is a chain of headers, newest first, the core's (in ROM) at the end of FORTH's, so finding a core word,
or finding that a number isn't a word, read every header: about 290 at `forth -l`'s prompt, 460 with `hydra.fs`,
6 to 10 ms each, so a line of 8 words took 50 ms and a 200-line file seconds.  So (6.15) forth keeps an index of
the word lists it searches, in a RAM bank of the task's (16 for each memory module, its own: the last one, taken
with `BANKS_ALLOC` as forth starts), where it costs the dictionary nothing and no write past HERE can reach it.

- **Its form.**  A record for each word list searched (8 at most, as the search order has): the word list, its
  last header as it was indexed, and 64 chains of nodes (a header, the next node), newest first, one for each hash
  of a name: its length * 8 plus its first and last characters in upper case, the low 6 bits (over the 561 names
  of the core, the libraries and `hydra.fs`: 9 a chain, 16 at most).  A search reads one chain, comparing a node's
  header's length, then its name.  The nodes (1,018) are shared, taken as they're needed; the bank's last 2.4K are
  nslib's buffers, for `newns` (6.16: they were the dictionary's top), and the 512 bytes before them the files'
  read-ahead (6.20).
- **Kept current.**  A word made goes into its word list's record as it's made (at its chain's start), if the
  record was current.  Whatever else changes a word list (a library loaded, a MARKER) leaves its record behind its
  last header, and the next search of it makes its chains again; `-lib` and `lib` (which change FORTH's in the
  middle) start the whole index again, as does a bank full of nodes, or more word lists than records.  The bank's
  first two bytes, `ix`, say it's the index: a program that wrote there starts it again.
- **No index:** no bank, more words than it holds, or a name in the bank's window ($8000-$9FFF: an EVALUATE of a
  bank's text, which the index's bank would hide): each header is read, as before.

Searching is 15 times faster (500 searches for a name that isn't there: 620 ticks, now 37; 978, now 60, with
`hydra.fs`; `dup`: 641, now 62), loading `hydra.fs` 2.5 times (952 ticks, now 383) and the eight device
libraries 3.3 times (2,260, now 690); the Forth 2012 suite's run takes 385M cycles, not 678M.  What's left of a
file's loading is mostly its reading (the storage driver, the line's scan) and its numbers' conversion.

## Loading files

A profile of `require hydra.fs` after the index (the emulator's, 6.8M cycles) showed where a file's loading went:
the storage driver's task 34% and the system calls 12%, as each line was a READ of 130 bytes and a SEEK back past
it; the line's scan 10%; the numbers' conversion 7% (two 16 x 16 multiplies a digit); the parser 9% (two calls a
character); the index's fixed cost a search.  So (6.20):

- **A read-ahead buffer** for the file being included, 512 bytes in the index's bank (below `newns`'s buffers:
  the index keeps 1,018 nodes): a line is copied from it as it's scanned (CR or LF looked for only among the
  control characters), and the file read and seeked once for 512 bytes, not twice a line.  The buffer is the
  innermost file's: a file read, or one nested in it ended, reads it again from the line's place (`src_pos`), so
  SAVE-INPUT and RESTORE-INPUT keep working by a line's place, as they did.  A line that runs past the buffer's end
  reads it again from the line's start; a short read, more after it.  No bank: as before.
- **Numbers**: a digit is the double times BASE, shifted and added for each of BASE's bits (any BASE under 256: the
  standard's 2-36), not two 16 x 16 multiplies.
- **The parser**: PARSE-NAME (the interpreter's) has loops of its own, one over a line's characters by an index
  register (a source with 256 or more left: 255 at a time).
- **The index**: a name's bytes compared as they are first, in either case only if they differ; the hash takes bit
  5 off its two characters rather than upper-casing them; the two word lists searched last have their records
  remembered (`hydra.fs`'s and FORTH alternate); a search keeps the bank and `tmp` in variables, not on the stack.

`require hydra.fs` takes 3.7M cycles now (6.8M; 203 ticks, not 383), the eight device libraries 9.5M (12.7M; 508
ticks, not 690), and the Forth 2012 suite's run 491M (639M).  The rest is mostly the storage driver's: reading the
ROM disk, and finding names in `/lib`, a union of four directories, where a name that isn't there cost 65 ms (13
ticks; 2 in a single directory), and `lib` looks for `NAME.fl` before `NAME.fs`.  So (6.21) the storage driver keeps
the names HydraFS has looked up, there or not, and the directories' entries on the way (its walk cache, in the
check's buffer): such a name costs 22 ms now, the rest of it the kernel's requests to each directory, and every
command line found through `/bin` gains too.

## Code in banks

The dictionary is the task's RAM from forth's BSS to `$7F00`, about 21K at the prompt, and a program's colon
definitions fill it: 100 definitions of 227 bytes' code each don't fit.  The task's RAM banks (16 of 8K for each
memory module) held only the index, the read-ahead buffer and (6.22) Block's blocks.  So (6.23) a colon
definition's code (`:noname`'s too) goes to a bank, and the dictionary keeps its header and a 6-byte stub, its xt:
`jsr far_enter`, the bank, the code's address.  100 such definitions take 1,192 bytes of the dictionary.

- **Calls.**  `far_enter` selects the stub's bank at `$8000` and jumps to the code, with the bank before and
  `fe_back`'s address under it, so the code's `rts` selects that bank again: 93 cycles more than a `jsr`.  A
  definition calling a word in its own bank calls its code (`comp_jsr` knows the stub), as before: a loop calling a
  word in the same bank takes 84 cycles a step, as with code banks off, and one in another bank 177.
- **The banks**, taken with `BANKS_ALLOC` as they're wanted (`code_begin`), 32 at most (`CB_MAX`).  A definition
  starts in the next one when this one has less than 2K left (`CB_ROOM`), so it has 2K at least and 8K at most
  (more: THROW -8).  With no bank to be had the code goes in the dictionary, as before.  A MARKER keeps the code's
  next byte and the banks taken, and gives back those taken since.
- **Compiling.**  While a definition is compiled HERE is its code's (`cmode`'s `CM_CODE`; the dictionary's HERE in
  `dhere`); `;` (`code_end`), a THROW to a CATCH from before the definition, and QUIT give the dictionary's back.
  Each byte of code is written with its bank selected meanwhile (`ccomma_a`, and `code_put` and `code_get` for the
  origs and the LEAVEs' chain), as an immediate word in an older bank may be running: `: my-if postpone if ;
  immediate`, defined in the first bank, compiles into the third.
- **What stays in the dictionary.**  A string a definition hands out (S", C", ABORT"'s message) would be in its
  bank, which a word in another bank hides, so it's in the dictionary, and the code has a `jsr` to `xsquote_p`,
  `xcquote_p` or `xabortq_p` and its address; `."` stays in the code (it's typed while its bank is selected).
  DOES> leaves a stub in the dictionary (`jsr far_does`, the bank, the children's code) and compiles `jsr
  do_does_far` and its address: a child's `jsr` goes to the stub, and `far_does` pushes the child's body and runs
  the code as `far_enter` does.
- **CATCH** keeps the bank selected and `cmode` in its frame, and THROW selects that bank again: a THROW from a word
  in another bank, or from a library's code with a bank of its own selected, comes back to code that's there.
- **The window.**  A definition's code is at `$8000`, so `bank!` or `seg-bank!` in one would take it from under
  itself: called from a code bank, they THROW -21, and `false code-banks` (the Hydra library's) before such a
  definition compiles it into the dictionary.  `see` follows a stub to its bank, and names a call to a word in the
  same bank by its stub.

The core is 628 bytes bigger, the dictionary at the prompt 242 bytes smaller (the BSS, and `see`'s part in
`tools.fl`), and the suite's run takes what it did (494M cycles).

## The standard's other word sets

Forth 2012's word sets HyForth hadn't, each a library (6.16 to 6.19), each passing the Forth 2012 test suite's
file for it, which the `forth` test now runs too (in three sessions: the dictionary hasn't room for it all at once).

| Library | Words | How |
| :--- | :--- | :--- |
| `memory.fl` | Memory-Allocation: `allocate`, `free`, `resize` | A heap at the top of the dictionary's space, from `heap_lo` (the core's) to `DICT_END`: it grows down as it's wanted, and gives its lowest pages back to the dictionary as they're freed, so the dictionary ends below `heap_lo`'s page (`allot`, `,`, a library's load, the shell's strings, `unused`).  A block: its size in the cell before it (bit 0, in use), first fit, free blocks joined as a search passes them, one split when 4 bytes or more are left over; `free` and `resize` take only an address `allocate` gave (else their ior and the heap as it was).  The iors are Forth 2012's codes, -59, -60, -61.  At the shell's prompt `free` shadows the `free` program: `% free` runs it.  So the heap can have the top, nslib's buffers for `newns` (2.4K) moved to the end of the index's bank |
| `double.fl` | Double-Number: `d+`, `d-`, `d.`, `d.r`, `d0<`, `d0=`, `d2*`, `d2/`, `d<`, `d=`, `d>s`, `dmax`, `dmin`, `m+`, `m*/` (with 6.5's `2constant`, `2variable`, `2literal`, `dnegate`, `dabs`), and the extension's `2rot`, `2value`, `du<` | `m*/` multiplies to three cells and divides a cell at a time, symmetrically, as the core's `/` does.  A `2value`'s code is a call to `do2value`, the core's, by which Core Extension's `to` knows it to store two cells |
| `locals.fl` | Locals: `{: args \| vals -- comment :}`, `(local)`, and the extension's `locals\|` (16 a definition: ENVIRONMENT? `#LOCALS`) | A frame on the 6502's stack, made where the locals are declared and let go at `;`, EXIT and DOES>; the zero page's `lp` (the core's) is its first cell, so DO's loop and `>r` above it don't move it, the frame before is kept under it, and CATCH keeps `lp` for THROW.  A local compiles as code that reads its cell through `lp` (12 bytes; after `to`, sets it).  The core asks the library (its `loc_vec`) about each name it compiles, before the search order and numbers, so a local's name hides a word's or a number's (`dup`, `bead`, `i` in a DO loop) till its definition's end; and at `;`, EXIT (Core's, and the shell's), DOES>, and ENVIRONMENT? |
| `block.fl` | Block: `block`, `buffer`, `update`, `save-buffers`, `flush`, `load`, `blk`, and the extension's `empty-buffers`, `list`, `scr`, `thru`; `open-blocks` (Gforth's) | Block u is the 1024 bytes at u * 1024 of the block file: `blocks.fb` in the directory current as it's first wanted (made if it isn't there), or the one `open-blocks` names.  Two buffers in the library: a block read into the one not given last (written first if it was UPDATEd), past the file's end spaces.  Behind them, the task's RAM banks (16 at most, taken as they're wanted, 8 blocks a bank) keep the blocks read and written: a block wanted again is copied from its bank, not read from the file; an UPDATEd buffer let go goes to its bank, changed there, and `save-buffers`, `flush` and the program's end (the core's `blk_end`: `blk_vec`'s call 1, at BYE and a script's end) write the changed ones to the file; with the banks all used the oldest goes (written first if it's changed), and with none to be had the file is the buffers' alone.  `load` makes a block the source: the source record has BLK (`src_blk`), which EVALUATE's and a file's set to 0; when a source nested in a block's ends (or THROW unwinds to it) the core asks the library (`blk_vec`) for its buffer again, as a LOAD since may have taken it.  `\` in a block skips to its 64-character line's end, REFILL goes on to the next block, SAVE-INPUT and RESTORE-INPUT keep the block (Core Extension's, now 6 cells) |

The record of files INCLUDED (REQUIRED's) holds 512 bytes of names now, not 256: the suite's first session filled
it, and a file past it is INCLUDED again.

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

## The devices (hylang's layer 3)

hylang's device libraries, as Forth source (6.13): `/lib/forth/NAME.fs`, `lib NAME` (`lib` finds a `.fs` when
there's no `.fl`), each over its device's files (the driver's own words, nothing new in the system), hylang's names
and its order (the device, pin or task first).  Each uses only the words forth starts with (File Access's, mostly),
so it's a worked example of reaching that device by hand too, and none needs another (a library a library loads
isn't one `lib` can take out on its own).  A failure THROWs the system's error (`ior>text` has its text); a text read
is in the library's buffer till its next.  Where hylang gives a list or a hash, Forth gives the driver's text.

| Library | Device | Words | hylang's, not here |
| :--- | :--- | :--- | :--- |
| `cons` | `#c` (`/dev`) | `window ( -- n )` (`$window`; none: 0), `windows ( -- c-addr u )` (`wctl`'s lines, `*` the one shown), `new-window`, `show-window ( n -- )` | `raw-on`, `raw-off` (`key` and `ekey` set the raw mode, a line read ends it), `beep` (Facility's) |
| `gpio` | `#g` (`/dev/gpio`) | `gpio ( pin -- level )`, `gpio! ( pin level -- )` (an output, set), `gpio-in`, `gpio-out ( pin -- )`, `gpio-port ( -- byte )`, `gpio-port! ( byte -- )`, `gpio-ddr! ( byte -- )`, `gpio-ca1! ( rise? -- )`, `gpio-ca2! ( n -- )` (0, 1, -1 an input), `gpio-wait ( -- count )` (CA1's next edge), `gpio-state ( -- c-addr u )` (`ctl`'s lines) | |
| `i2c` | `#i` (`/dev/i2c`) | `i2c-read`, `i2c-write ( addr reg c-addr u -- )` (at the device's register, `i2c-reg-size` bytes of it: 0, none), `i2c-speed ( khz -- )`, `i2c-reg-size ( n -- )`, `i2c-devices ( -- )` (the addresses that answer, typed), `i2c? ( addr -- flag )` | |
| `spi` | `#S` (`/dev/spi`) | `spi ( dev c-addr u -- )` (a transaction: the bytes that came back in the bytes' place), `spi-read ( dev c-addr u -- )` (u clocked in), `spi-mode ( dev mode -- )` (0 or 3) | |
| `proc` | `#p` (`/proc`) | `task-args`, `task-cwd`, `task-env`, `task-ns`, `task-regs ( task -- c-addr u )`, `task-mem ( task addr c-addr u -- )`, `task-ram ( task bank offset c-addr u -- )` | |
| `clock` | `#t` (`/dev`) | `set-date ( c-addr u -- )` (`2026-10-04 12:00:00`: the clock and the DS1747), `rtc ( -- c-addr u )` (`running`, `stopped` or `none`, and `battery low`) | |
| `disk` | `#d` (`/dev/sd`) | `disk-ctl ( disk -- c-addr u )` (its ctl's text; a disk by its letter: `[char] x`), `disk-start`, `disk-stop ( disk -- )`, `cards ( -- mask )` (bit n: a card on SPI device n) | `disks` (`disk-ctl` of each), `df` (the program) |
| `pc` | `#P` (`/pc`) | `pc? ( -- flag )` (the PC tool answers; none: a second, then false) | |
| `sound` | `#a` (`/dev`) | (in `sound.fl`, beside 6.8's words) `note-of ( c-addr u -- n )` (`C#4`, `Db4`, `B-1`: a MIDI number, 60 middle C; not a note: THROW -24), `tune ( c-addr u ch tempo -- )` (`C4 1 E4 1 - 1 G4 2`: notes and their beats, `-` a rest; tempo beats a minute; Ctrl-C ends it, the note off) | `play` (the program) |

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
first line is the shell's program and arguments (`/bin/forth -l`); with none, `rc -l` as before.  (Since October
2026 the ROM disk has one, `/rom/lib/shell`, naming `/bin/forth -l`: HyForth is the login shell, the user's
choice; a card's or the shared RAM disk's `/lib/shell` comes first in `/lib`'s union.)  wstart is given it
as its arguments, and runs in init's namespace (not an empty one of its own now), so the program is found as init
finds it; each window's shell still starts in an empty namespace of its own, which its profile builds.  Each shell's
`/ram` is its own area, so a `/lib/shell` for every window is a card's (`/sd/0/lib/shell`) or the shared RAM disk's
(`/sram/lib/shell`, till the next reset).  hylang will be chosen the same way.

**`send n line`** types the line, and Enter, in window n, whatever shell is there: it's written to the window's
`#cN/kbdin`, a file of the console's (rio's `kbdin`: a write's bytes are the window's keys, as typed, all of them: its
keys' queue holds 63, and the write waits while the window's shell takes them), so rc can do it too:
`echo ls >'#c2/kbdin'` (its LF is a new line, as the console's Enter).  A line is 126 characters at most, as one typed.

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

6.6 to 6.10 done (October 2026), then 6.11 to 6.13, from comparing the shell with hylang's, then 6.14 to 6.23;
[forth-status.md](forth-status.md) has each one's notes, the sizes and what's next, and
[using/hyforth.md](using/hyforth.md) is the guide for using it.  The tests: `forth` (the Forth 2012 suite, still
passing, its files for 6.16-6.19's word sets, and 6.22's and 6.23's scripts), `hyforth` (6.6-6.8 and 6.14), `fshell`
(6.9 and 6.11), `lshell` (6.10), `fhydra` (6.12), `fdev` (6.13), `findex` (6.15), `fload` (6.20) and the storage
driver's `wcache` (6.21).

| Step | | Tested |
| :--- | :--- | :--- |
| 6.6 | Lower case, `words`, the library table, `libs`, `lib`, `-lib` | The suite; `words`' flags (a constant, a definition, an immediate one, the core's and a library's); a library hidden and searched again, where it was; `-lib tools` refused; a `.fs` one; one a `marker` takes out |
| 6.7 | `disasm.fl` (and `see` with it), `sys`, `bits.fl`, `random.fl` | The modes and the Rockwell opcodes, a `jsr` to a word; `see` of a code word with `disasm.fl` and without; `sys` of code in RAM; the bit words; `random`'s numbers against xorshift32's |
| 6.8 | The terminal's words, `sound.fl`, `ctl` | The sequences sent; `form`; an arrow key and a character read raw; notes keyed on (the emulator's YM2151), a claim, the volume; `ctl` and its error |
| 6.9 | The shell: `shell.fl`, `forth -l`, `profile.fs` | The rule (a number, a word, a pipeline, a redirection), `cd` and the prompt (its format), a definition over lines, `status` and `$status`, `&` and `wait`, Ctrl-C, errors and a usage, `-lib shell` and `lib shell`, `exit` |
| 6.10 | `/lib/shell` for init and wstart; `send` and `#cN/kbdin` | A card's `/lib/shell`: forth in window 0 and in a window made (`$window`); a line sent to window 0, run there, then one of 100 characters, more than its keys' queue holds (the write waiting for room) |
| 6.11 | The shell's next: `%`, the second prompt (`prompt2`); programs as values: `sh-out`, `output-of`, `\|` and `piped`, `spawn`; the programs' code in the core (`fprog.inc`), the output's hook | `% free` past a word `free`; a definition's second line (its tab); a line's output and a word's, strings; a word's output into `wc` (`\|`, `piped`), into `head`, which ends first, and stopped by Ctrl-C; `spawn` and `wait`; `sh` and `run` as before (the forth test) |
| 6.12 | The Hydra's words (hylang's layer 2): the directories' (Gforth's), `=mkdir`, `unsetenv`, `note`, `note-group`, `on-note` (the core's note handler and its polls), `pause`, `ior>text` | A directory read, one made; the directory set and got (the prompt follows); an error's text; a variable set, read, removed; a note to itself taken by a handler between words and in a loop, and one it says no to |
| 6.13 | The device libraries (hylang's layer 3): `gpio`, `i2c`, `spi`, `cons`, `proc`, `clock`, `disk`, `pc` as source; `sound.fl`'s `note-of` and `tune` | Pins read and set, the port, `ctl`'s lines, CA1's edge; a memory written and read at a register, the devices; an echo device's transactions, mode 3; the window; a task's args, cwd, regs and memory; the chip, the time set; the ROM disk's ctl, a card; the PC tool; notes' numbers, a tune's notes on the YM2151 in time, a bad note |
| 6.14 | Compile-only words: THROW -14 interpreted (`F_COMPILE`) | `>r`, `if`, `."`, a `synonym` of `>r` and a library's `2>r` typed (each `name: compile only`, forth going on); `>r`, `i`, `r>` and the synonym compiled and run; the Forth 2012 suite |
| 6.15 | Housekeeping: the word lists' index (in a bank); `hex2` and `hdr_out` the core's (`tools.fl`'s and `disasm.fl`'s copies gone); `argc` 0 under `forth -l`; the guide, [using/hyforth.md](using/hyforth.md) | A word redefined, a definition hidden till `;`, a MARKER's words gone, MARKERs till the bank's full, more word lists than records, EVALUATE of a bank's text, the index's bank overwritten; 500 searches under 150 ticks; the Forth 2012 suite (Search-Order's word lists among it); `argc` at `forth -l`'s prompt |
| 6.16 | Memory-Allocation (`memory.fl`): the heap at the dictionary's top (`heap_lo`); `newns`'s buffers in the index's bank | `memorytest.fth`; `newns` at `forth -l`'s start (the shells' tests) |
| 6.17 | Double-Number (`double.fl`), and its extension's `2rot`, `2value` (`do2value`, the core's; `to`), `du<` | `doubletest.fth` (its numbers read with prefixes and signs; `d.` and `d.r`'s lines as they should be) |
| 6.18 | Locals (`locals.fl`): `{:`, `(local)`, `locals\|`; the core's `lp`, `loc_vec` and its calls; CATCH keeps `lp` | `localstest.fth` (its Search-Order part too) |
| 6.19 | Block (`block.fl`) and its extension; the source record's BLK, `blk_vec`; `\`, REFILL, SAVE-INPUT and RESTORE-INPUT in a block; 512 bytes of names INCLUDED | `blocktest.fth` (its blocks 20-29 in `blocks.fb` on the card; 64 characters a line, as it works out) |
| 6.20 | Loading files faster: the read-ahead buffer (in the index's bank), numbers by BASE's bits, PARSE-NAME's own loops, the index's fixed cost | A file made for the read-ahead (a CR LF across its buffers, CR LF and CR ends, a line cut at 128, a last line ended by a CR and the file's end), names between tabs, numbers with each prefix and in base 36, a double; `hydra.fs` in under 300 ticks; the suite (its files, SAVE-INPUT and RESTORE-INPUT in them) |
| 6.21 | The storage driver's walk cache: HydraFS's names looked up, there or not, and the directories' entries on the way, in the check's buffer; a disk's forgotten as it changes | rc lines (`wcache`): a name not there, then made, renamed, removed; directories made, removed and renamed under names looked up; a name made through `/lib`'s union after it wasn't there; the file system's, disks', loader's and tools' tests (a check, a format, the budgets) |
| 6.22 | Block's banks: the blocks read and written kept in the task's RAM banks behind the two buffers, the changed ones written to the file by `save-buffers`, `flush` and the program's end (`blk_vec`'s call 1, the core's `blk_end` at BYE and a script's end) | `blocktest.fth`; `blk1.fs` and `blk2.fs` (a block changed and let go to its bank, no flush: there in the file for the next forth); 32 blocks read again, 0.62 M cycles from banks, 2.18 M from a RAM disk's file |
| 6.23 | Code banks ("Code in banks"): a colon definition's code in the task's RAM banks, its xt a stub (`far_enter`); its strings and DOES>'s stub in the dictionary; CATCH keeps the bank; `code-banks`, and `bank!` from a code bank THROW -21 | The suite, every definition in a bank; `cbank.fs` (13K of definitions EVALUATEd from the first bank into the next ones; an immediate word, a DOES> child, a string and a THROW from the first bank; SEE; `bank!` refused; `false code-banks`; a MARKER giving the banks back); 10,000 calls, 84 cycles a step in the same bank, 177 in another |
