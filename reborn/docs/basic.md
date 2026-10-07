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
| Where it runs | A ROM image at `$A000`, from WOZMON | A module run in place (`HYX2_PROGRAM "basic"`, one bank, 10K of 16K), started by rc; its data and BSS from `$0400` |
| Zero page | `$30`-`$FA`: its variables, the input line, and CHRGET (code that held the text pointer in its own `lda abs`) | The program's `$22`-`$7F` (91 bytes, `zeropage.inc`): what it reads through (`(zp),y`), names as zero-page addresses (`ldx #FAC`) or indexes as one block (REASON's `TEMP1`-`FAC`, the floating point's `TMPEXP`-`SERLEN`), in Microsoft's order; `TXTPTR`; the rest (flags, vectors, `CURLIN`, `OLDTEXT` ...) in the BSS |
| CHRGET | Copied to the zero page at the cold start | In ROM, reading through `TXTPTR` (`lda (TXTPTR)`: a cycle more a character) |
| The line buffer | In the zero page after `LINNUM` (50 bytes, while lines could be 71) | A page of its own (`$0400`, `basic.cfg`'s `LINEBUF`): 240 characters.  Microsoft's code for a buffer out of the zero page back (Applesoft's and CBM2's: direct mode by the page, the line's number before it, GET's terminator, INPUT's branch), and the new line's link made not to look like the program's end |
| Memory | Asked for (`MEM`), and tested a byte at a time | The task's RAM from the BSS's end to `$7F00` (`BREAK`): 30,666 bytes free |
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

## To come

File statements, sound, SYS and the system's calls, the task's RAM banks for more memory, and the shell: each
reviewed against what the system and the other languages have before it's added.

## The test

`basic` (tests/tests.js): at the console, the banner, PRINT, the operators and functions, either case, the short
forms and LIST's full names, a program (FOR, GOSUB, DATA, READ, INPUT, DIM, DEF FN), Ctrl-C and CONT, GET, errors,
BYE; a pipeline into it; in `/ram`, SAVE as text and tokenized, LOAD of each, RUN "name", a file not there, the
text's `cat`; scripts (`basic file`, `#!/bin/basic`: codes 0 and 1).
