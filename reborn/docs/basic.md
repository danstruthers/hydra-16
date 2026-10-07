# BASIC: EhyBASIC on the system

`basic` (`modules/basic`) is EhyBASIC, the Hydra-16's Microsoft BASIC, as a program of the system: a paged ROM module
run in place, `/bin/basic`.  EhyBASIC is Microsoft BASIC 2A for the 6502, by way of Michael Steil's reconstruction
(mist64/msbasic), Ben Eater's port and EhyBASIC's for the Hydra-16 (burntcouch/ehybasic): a ROM image at `$A000`,
started from WOZMON with `A000R`, that talked to two old BIOS addresses.  In October 2026 the user asked for it to be
converted to run on the system as it is now, starting from `C:\source\ehybasic-2` (its working tree: the build in its
`temp/hydrabas.bin`), with these choices:

* It lives here, on the branch `reborn-basic`, and the ehybasic folder is left as it is.
* The keywords and error messages in full again (`GOSUB`, `RETURN`, `LEFT$`, `AND`, `?SYNTAX ERROR`), with EhyBASIC's
  short forms still accepted (`JSR`, `RTN`, `RSTR`, `CLR`, `ST$`, `CH$`, `LT$`, `RT$`, `MD$`, `&`, `|`, `!`).
* `SAVE` and `LOAD` with text listings by default, and a tokenized form too.
* In the first version also: file statements, sound, `SYS` and calls, and more memory (the task's RAM banks).
* Then a shell, as HyForth's and hylang's: `basic -l`, a BASIC line run as BASIC and any other as an rc command line.

## The conversion

EhyBASIC's sources are mist64's, with conditional assembly for ten machines.  The Hydra build's choices were settled
first (`.ifdef`s resolved as its build had them, the other machines' code taken out), and the result assembled to the
same bytes as its last build, so what follows starts from the code that ran.  Microsoft's code is otherwise as it
was, its labels and comments kept, but where the system wanted it changed:

| What | EhyBASIC's | Now |
| :--- | :--- | :--- |
| Where it runs | A ROM image at `$A000`, from WOZMON | A module run in place (`HYX2_PROGRAM "basic"`, one bank, 11K of 16K), started by rc; its data and BSS from `$0400` |
| Zero page | `$30`-`$FA`: its variables, the input line, and CHRGET (code that held the text pointer in its own `lda abs`) | The program's `$22`-`$7F` (91 bytes, `zeropage.inc`): what it reads through (`(zp),y`), names as zero-page addresses (`ldx #FAC`) or indexes as one block (REASON's `TEMP1`-`FAC`, the floating point's `TMPEXP`-`SERLEN`), in Microsoft's order; `TXTPTR`; the rest (flags, vectors, `CURLIN`, `OLDTEXT` ...) in the BSS |
| CHRGET | Copied to the zero page at the cold start | In ROM, reading through `TXTPTR` (`lda (TXTPTR)`: a cycle more a character) |
| The line buffer | In the zero page after `LINNUM` (50 bytes, while lines could be 71) | A page of its own (`$0400`, `basic.cfg`'s `LINEBUF`): 240 characters.  Microsoft's code for a buffer out of the zero page back (Applesoft's and CBM2's: direct mode by the page, the line's number before it, GET's terminator, INPUT's branch), and the new line's link made not to look like the program's end |
| Memory | Asked for (`MEM`), and tested a byte at a time | The task's RAM from the BSS's end, and a bank of its own after it, to `$9FFF` (below: "More memory"): 38,550 bytes free |
| Output | `MONCOUT`, a BIOS address | fd 1, buffered (a LF or a full buffer sends it); a new line is LF alone, and the column 0 after it (Microsoft's set it to 13) |
| Input | `MONRDKEY` a key at a time, BASIC editing the line | stdin a line at a time (`INLIN`: the console's cooked lines, edited and echoed by the console, or a file's or a pipe's: LF, CR or CR LF); its end ends BASIC in direct mode |
| Ctrl-C | The keyboard polled at each statement | A note (`NOTIFY`): the handler sets `intr`, which each statement checks (`ISCNTC`), and a wait for a line or WAIT's loop ends at; at the prompt it's a new line, in INPUT `BREAK IN n` |
| GET | `MONRDKEY` | The console's raw mode (`/dev/consctl`'s `rawon`) and a read of `/dev/cons` that doesn't wait, as HyForth's `key?`; stdin's next byte when it isn't the console |
| Keywords | Short forms in the table, to fit 8K | The full names, and an alias table after them (`ALIAS_MAP`): the tokenizer turns an alias's token into its keyword's, so `LIST` shows the full name.  The table is past 256 bytes now, so the tokenizer and `LIST` walk it with a pointer (`KWPTR`) rather than `.Y`.  Letters outside strings, `REM` and `DATA` are taken in either case (`UPCASE_X`) |
| Errors | Two letters (`?SN ERROR`) | Microsoft's messages (`?SYNTAX ERROR IN 10`), through a table of their addresses (`ERRTAB`) |
| The end | None | `BYE` (and stdin's end): `EXITS` with code 0 |
| Not at the console | - | No banner and no `OK`, and an error's line ended: `basic <prog.bas` and pipelines give the program's output alone |

Some of Microsoft's code knows where things are, and the conversion keeps it so (`token.inc` asserts it): FOR is token
`$81` and DATA `$83`; the operators `+` to `OR` come just before `>`, `=`, `<`; `LEFT$`, `RIGHT$` and `MID$` are the
last functions.  One trick that knew the segments' order (the tokenizer read the keyword table as `MATHTBL+29`) is
gone, and `NAMENOTFOUND` checks both bytes of its caller's address (Microsoft's `CONFIG_SAFE_NAMENOTFOUND`).

An alias reserves its name, as every keyword does: `ST$`, `LT$` and the like can't be variables.

## Programs in files: LOAD, SAVE, RUN "name", scripts

* `SAVE "name"` writes the program as text: its LIST, a line each, its number first (`10 PRINT "HI"`: LIST shows
  line numbers without the sign's space now, so the file reads as typed).  The file is made, or emptied first.
* `SAVE "name",B` writes it tokenized: a 10-byte header (`HYBAS`, a version, the keyword table's sum, the length),
  then the lines as they are in memory.  Faster to load, but only by a BASIC with the same keyword table (the sum
  says so: another's is `?BAD FILE ERROR`).
* `LOAD "name"` takes either (the header tells them apart) in place of the program.  A text's lines come through the
  main loop as if typed (INLIN reads the file through a buffer of its own, so stdin's lines read ahead wait), and
  a line without a number is passed over: a `#!/bin/basic` line, or a comment.  A tokenized program is read whole,
  and its lines linked again where they are.
* `RUN "name"` is LOAD, then RUN.
* `basic file [argument ...]` runs a script: the file LOADed and run, no banner, and its end is BASIC's: code 0, or 1
  after an error or a BREAK (its message, its line ended).  A file whose first line is `#!/bin/basic` runs so by its
  name.
* The system's errors are shown as BASIC's are, their text in capitals: `?NOT FOUND ERROR`.  An error or Ctrl-C
  while LOAD or SAVE has a file closes it (RESTART's `IO_RESET`).

## Files: OPEN, CLOSE, PRINT#, INPUT#, GET#, EOF

Reviewed first, against Microsoft's own BASICs and the system: Microsoft BASIC 2A is Commodore's line, whose files
are `OPEN lfn,device,sa,"name"`, `PRINT#`, `INPUT#`, `GET#`, `CLOSE` and the status `ST`; GW-BASIC's are `OPEN
"name" FOR INPUT AS #n`, `PRINT #n,`, `INPUT #n,`, `LINE INPUT #n,`, `CLOSE #n` and `EOF(n)`.  This tokenizer finds
a keyword anywhere, even in a name (Microsoft's: `SCORE` is `SC` `OR` `E`), so short new keywords (GW-BASIC's `AS`,
`OUTPUT`) would break programs; the Hydra has paths, not device numbers.  Settled: Commodore's form with a path, and
GW-BASIC's EOF, with two new statements and one function:

* `OPEN n,"name"[,"mode"]`: channel `n` (1 to 4) a file or a device (`/dev/cons`, `#n/kmesg` ...), read (`R`, as
  with no mode), written (`W`: made, or emptied first) or added to (`A`: written at its end).  A channel open already:
  `?FILE OPEN ERROR`; one not open: `?FILE NOT OPEN ERROR`; another number: `?ILLEGAL QUANTITY ERROR`.
* `CLOSE n`, or `CLOSE` alone for all.  `RUN`, `NEW`, `CLEAR` and a line entered close them all too (as GW-BASIC
  does), and an error or Ctrl-C leaves them open.
* `PRINT #n, ...`, `INPUT #n, ...` and `GET #n, ...`: the statements with the channel first (a `#` after the keyword,
  spaces as you like, rather than Commodore's `PRINT#` keywords).  PRINT# keeps the console's column; INPUT# has no
  prompt, takes a line's comma-separated values as INPUT does, its end is `?END OF FILE ERROR`, and a value that
  isn't a number is an error (no REDO from a file); GET# gives a byte at a time, `""` (or 0) at the end.
* `EOF(n)`: true (-1) when channel `n` has nothing more to read.

Each channel is an input source as stdin and LOAD's file are (an fd and a 128-byte buffer: `IN_BYTE`), so INPUT#
reads through the same INLIN.  Microsoft's INPUT took its flag from `.Y`, the buffer's high byte, 0 in the zero page:
with the buffer in RAM it gave `?SYNTAX ERROR` for a bad answer, and now it's `?REDO FROM START` again.

## Sound: SOUND, BEEP, SLEEP

Reviewed against C's `snd.h` (`snd_note`, `snd_patch`, `snd_vol`, `snd_off`, `snd_claim`, `snd_volume` ...),
HyForth's `sound.fl` and hylang's `snd-` functions (the channel first; MIDI notes, 60 middle C; patches 0-162),
and against BASICs' (Commodore's `SOUND voice,freq,duration`, GW-BASIC's `SOUND freq,duration`, the X16's `FMNOTE`
and the like).  Every new keyword is a name a program can't use, found even inside longer names (`PANEL` would be
`PAN` and `EL`), so sound is one statement, not one for each of `snd_*`; and the system's units: MIDI notes,
seconds.

* `SOUND ch, note [, patch [, vol]]`: on channel `ch` (0-7) MIDI note `note` (0-127), its patch (0-162) and its
  volume (0-127) first if they're given: one write of the driver's commands to `/dev/snd`, so no other program's
  comes between them.  `SOUND ch`: its note off (the release).  Drums are patches 128-162.
* `SOUND "word [n]"`: a line for `/dev/sndctl`, the driver's own words: `"claim 255"`, `"release 255"`, `"volume 150"`,
  `"reset"`.  Its errors are the driver's (`?INVALID ARGUMENT ERROR`; another program's channel, `?BUSY ERROR`).
* `BEEP`: the console's bell, sent at once.
* `SLEEP s`: `s` seconds, as rc's and hylang's `sleep` (to the tick, 5 ms; up to 163 s), the output sent first;
  Ctrl-C ends it (and the program).

Not here, for keywords' sake: pan, bend and General MIDI's drum numbers (`snd_pan`, `snd_bend`, `snd_drum`), and
songs (`snd_play`: `play` at rc's prompt, or from the shell later).

## Machine code and the system's calls: SYS, RREG, USR

Reviewed against Commodore's BASIC 7 (`SYS address[,a[,x[,y]]]` and `RREG`, which read the registers back), Microsoft's
USR, and HyForth's and hylang's `sys-` words and functions (a call by its name, every call a program makes, made from
`spec/api.def`).  Settled: BASIC 7's two statements, and SYS taking a call's name too:

* `SYS address [, a [, x [, y]]]`: the machine code at `address` called (`jsr`), with `.A`, `.X` and `.Y` (0 if
  they're not given); the output is sent first.
* `SYS "NAME" [, a [, x [, y]]]`: a system call by its name, in either case (`SYS "GETPID"`, `SYS "banks_alloc",1`):
  `tools/apigen.js` makes the table, `obj/gen/basicsys.inc` (each name and its address in the jump table), from the
  specification, as HyForth's and hylang's are made.  A name that isn't there: `?NO SUCH CALL ERROR`.  The call
  registers `r0`-`r15` are bytes 2-33: POKE them just before the SYS (BASIC's own I/O uses `r0` and `r1`; SYS keeps
  them while it sends the output), and PEEK the results there after it.
* `RREG [a] [, x] [, y] [, p]`: the registers after the last SYS into numeric variables, any left out (`RREG ,X`);
  for a system call, bit 0 of `p` (C) says it failed and `a` is then the error.  They're also at fixed places:
  `PEEK(1280)` to `PEEK(1283)` (`SYSREGS`, `$0500`).
* `USR(x)`: Microsoft's, its jump at 1284 (`$0504`): POKE its address at 1285 and 1286.  The code gets `x` in the
  floating point accumulator and leaves its value there.  Until it's set, `?ILLEGAL QUANTITY ERROR`.

Where machine code can go: above `HIMEM` (below), in BASIC's own memory: `HIMEM 40704` keeps `$9F00`-`$9FFF`.
SYS selects BASIC's bank at `$8000` again after the code returns; USR's code must leave it as it found it.

## More memory: a bank of its own, HIMEM

Microsoft's BASIC has one flat memory, the program, its variables, its arrays and its strings in one piece reached by
16-bit pointers everywhere; arrays or strings in banks would mean a bank switched at each of hundreds of places.
The window at `$8000` is just above the task's RAM, though, so BASIC takes its RAM to `$7FFF` (`BREAK`) and a bank of
its own (`BANKS_ALLOC`), selected there for good: one piece from the BSS's end to `$9FFF`, 38,550 bytes free (about
30K without), and no code of Microsoft's changed.  The strings, which grow down from the top, are in the bank, and a
program's arrays can be past 32K.  In task F (its RAM ends at `$7EFF`: the clock's registers) or with no bank to be
had, its memory is the RAM to `$7F00`, as before.

* `FRE(0)` is unsigned now (Microsoft's was negative past 32767).
* `HIMEM n`: BASIC's memory's top at `n`, what's above it (to `$9FFF`) kept for machine code; the variables cleared,
  as CLEAR does (Applesoft's `HIMEM:`).  Past the top, or less than a page past the program: `?ILLEGAL QUANTITY
  ERROR`.
* The window is BASIC's: a program mustn't select another bank (`POKE 0`) while it runs, and SYS selects BASIC's
  again after its code.

Checking memory past 32K turned up a bug of EhyBASIC's own: its sources had lost a `dex` that sizes a string array's
elements (4 bytes, not a descriptor's 3), so the garbage collector, which steps 3 at a time, read the elements wrong
once there were enough strings, and hung or broke the memory.  Microsoft's `dex` is back.  EhyBASIC's other changes
to Microsoft's code are fixes (RND's and an integer limit's 4-byte constants made 5, a page boundary in
`FRM_STACK2`, KBD BASIC's normalization limit) and stay; the same expressions give the same results in this BASIC
and in EhyBASIC's last good build (its sources' CONFIG_2A, the binary `hydrabas021126-0307-good-inline.bin`, run on
a bare 65C02).

## The shell: basic -l

HyForth's and hylang's rules ([hyforth.md](hyforth.md), "The shell"; [hylang.md](hylang.md), "The prompt"), with
BASIC's idea of its own lines (`hyshell.inc`):

* **The rule.**  At the prompt a line is BASIC's if it's a program line (a number first), `?`, a statement's keyword
  as its first word (or an alias of one: `JSR`), or an assignment (a name, `$` or `%` after it as it may be, then
  `=` or a subscript's `(`); any other is an rc command line, run whole by rc (`rc -c`) and waited for.  So pipes,
  redirections, globbing, quoting and `$x` are rc's, and BASIC isn't given a shell grammar.  The words are matched
  whole, in either case (`printf` is rc's, `print` BASIC's); a name in both is BASIC's (`sleep`, which means the same,
  but `wait`, `if`, `for` too), and `%` before a line makes it rc's whatever it is, at any BASIC's prompt.
* **Statuses.**  An rc line's code is `$status` (rc's: the program's exit message, or its code); one ending in `&`
  isn't waited for (its task `$apid`, in a note group of its own).  Ctrl-C while rc runs is rc's, and the shell goes
  on, on a new line.  `exit` ends BASIC with the last code; `bye` with 0.
* **What an rc line can't do.**  It runs in a task of its own, so BASIC's current directory and namespace are the
  shell's own commands, their arguments rc's way (`'...'` quoted, `''` a quote; `$name` the environment's variable,
  its first word): `cd [dir]` (none: `$home`), `bind [-a|-b] [-c] new old`, `mount [-a|-b] [-c] #x old [spec]`,
  `unmount [new] old`, `newns`.  A usage that isn't right says so (`usage: bind [-a|-b] [-c] new old`), a failure
  says the system's text (`/none: not found`), and either sets `$status`.
* **The prompt** is HyForth's and hylang's: the directory and `> ` (`/rom/lib> `; on a card `0:/games> `), on a line of
  its own, in place of `OK`.
* **`basic -l`**, a login shell: its namespace made (`newns`: the SDK's nslib, its zero page BASIC's temporaries,
  its 2.4K of buffers in a bank taken for the while), its window's console at `/dev` (not window 0's: `#c` taken off,
  `#c$window` put after), its notes its note group's (`/dev/consctl`'s `group`) — what rc's `/lib/profile` does, done
  in code, as a profile of BASIC's can't hold a shell command in an `IF` — then `/lib/basic/profile.bas` (the ROM
  disk's; through the `/lib` union a card's or the RAM disk's in its place) run as typed lines, and the prompt.  A
  card's `/lib/shell` with `/bin/basic -l` makes it a window's shell, init's and wstart's.
* **`ENV$(name$)`**, the environment's variable (its first word; `""` if it isn't set), as hylang's `env` and
  HyForth's `getenv`.

The module is 14.7K of its bank now (nslib 2K of it).

## The test

`basic` (tests/tests.js): at the console, the banner, PRINT, the operators and functions, either case, the short
forms and LIST's full names, a program (FOR, GOSUB, DATA, READ, INPUT, DIM, DEF FN), Ctrl-C and CONT, GET, errors,
BYE; a pipeline into it; in `/ram`, SAVE as text and tokenized, LOAD of each, RUN "name", a file not there, the
text's `cat`; scripts (`basic file`, `#!/bin/basic`: codes 0 and 1); files (OPEN's three modes, PRINT#, INPUT#, GET#
and EOF at the end, CLOSE, the file's `cat`; FILE OPEN, FILE NOT OPEN, a file not there); INPUT's REDO FROM START;
sound (SOUND's notes with a patch, a volume and off, SLEEP between two timed on the emulator's YM2151, BEEP's bell,
`/dev/sndctl`'s volume kept and its error, ILLEGAL QUANTITY); SYS (calls by name and RREG, one not there, machine code
above HIMEM called by SYS and by USR, registers in and out); memory (FRE past 32767, a 32K array, 301 strings and
the garbage collector, an integer array, HIMEM and its errors); the shell (`basic -l` at rc's prompt: BASIC's lines and
rc's by the rule, `cd` and the prompt, `$status`, `%`, a usage, ENV$, a program line, `exit`).  `bawin`: a card's
`/lib/shell` naming `/bin/basic -l`, init's in window 0 and wstart's in a window made (`$window`, ENV$).
