# BASIC: its design

BASIC is the Hydra-16's BASIC: a structured BASIC in QuickBASIC's way, on the Hydra's numbers.
[using/basic.md](using/basic.md) is its reference (every statement and function), [design/plans/BASIC.md](design/plans/BASIC.md)
the plan it was built to and why, [design/plans/NUMBERS.md](design/plans/NUMBERS.md) the numbers it shares with
hylang, HyForth and C.  This is how it's made: `modules/basic`, a module of the paged ROM run in place (`/bin/basic`).

Contents: [The module](#the-module) · [Memory](#memory) · [Values](#values) · [Compiling](#compiling) ·
[Running](#running) · [Numbers](#numbers) · [The heap](#the-heap) · [Input and output](#input-and-output) ·
[The prompt and the shell](#the-prompt-and-the-shell) · [The Hydra's](#the-hydras) · [The tests](#the-tests) ·
[Against hylang and HyForth](#against-hylang-and-hyforth)

## The module

A program is compiled whole, its text read once for its procedures and types and again for its code, into the code
of a stack machine, which the interpreter runs.  A line typed at the prompt is compiled the same way, into a region
of its own, and run at once.

BASIC is a module of eight banks (`HYX2_PROGRAM "basic", main, 8`; `modules/module8.cfg`), each 16K at `$A000` in
turn.  A call between banks goes through the module's trampolines in its RAM (`FARN bank, routine`; the third bank
calls the second's with `C2`, which passes `.A`, `.X`, `.Y` and C).

| Bank | Files | What | Used |
| :--- | :--- | :--- | :--- |
| 1 | `run.inc`, `main.inc`, `fn.inc`, nslib | The interpreter: its ops, calls and frames, errors' way to a handler; the top (the prompt's loop, a script, `RUN`, `CONT`); the statements' and functions' groups by bank; the SDK's `newns` for `basic -l` | 44% |
| 2 | `lex.inc`, `comp.inc`, `expr.inc`, `stmt.inc`, `procs.inc` | The compiler: tokens, symbols, labels and their fixups, the line table, `INCLUDE`; expressions; statements; `SUB` and `FUNCTION`, calls and their arguments | 84% |
| 3 | `stmt3.inc`, `rec.inc` | The compiler's other statements (`DIM`, `DATA`, `INPUT`, files, the console, `SYSTEM` ...); `TYPE`, fields and records' copies | 43% |
| 4 | `num.inc`, `fns4.inc` | The numbers: the libraries' calls, the operators, the number functions, `VAL` (E notation), `STR$` | 32% |
| 5 | `heap.inc`, `gc.inc`, `fns5.inc` | The heap: strings, numbers past 32 bits, arrays and records; the collector; the string functions; `DIM`, `REDIM`, `ERASE`, `MID$ =` | 34% |
| 6 | `io.inc` | `PRINT` (zones, `TAB`, `PRINT USING`), `INPUT`, `READ`, the console's sequences and keys, files | 36% |
| 7 | `sys.inc`, `prog.inc` | The errors' messages; the system's functions (`TIMER`, `ENV$`, `SHELL`, `PEEK` ...); the program's text: `LOAD`, `SAVE`, `LIST`, `EDIT`, `DELETE`, numbered lines typed | 44% |
| 8 | `shell.inc`, `machine.inc`, `gfx.inc` | `basic -l`'s rule and its own commands; `SOUND`, `PLAY`, `SYS`, `RREG`; the graphics, as `/dev/vid/draw`'s commands | 55% |

What every bank calls is in the task's RAM (`ram.inc`, the `DATA` segment): the far memory's cursors, the output's
buffer, `b_error` (an error from any bank to the first's `err_entry`), `run_exit`.

## Memory

The task's RAM (`$0400` on) holds the interpreter's state (`BSS`, 7.7K), the globals (5 bytes each) and the value
stack (`stk_base` on: frames, `GOSUB`'s returns, `FOR`'s state and the expressions' values, 5 bytes each).
Everything else is in the task's RAM banks, seen at `$8000`-`$9FFF` one at a time through a table of logical banks
(`ltab`: a logical bank's physical one, taken from the system when it's first used) in regions:

| Logical banks | Region |
| :--- | :--- |
| 0-6 | The program's code; 7, the line typed at the prompt's |
| 8-15 | The program's text: the buffer the prompt keeps (a line: its length, its bytes; `$FF` the end), and `INCLUDE`'s files after it while it's compiled |
| 16-19 | The symbols: names (hashed, by scope), labels and their fixups |
| 20-21 | The line table: each statement's code address and line (an error's line, `ERL`, `Break in`) |
| 22-23 | `DATA`'s items |
| 24-25 | The numbers' scratch (their operands and results), a number's text |
| 26-63 | The heap |

A far address is a logical bank and an address in the window (3 bytes); in the code, text and symbol regions a
16-bit region address (its bank in the region, then 13 bits) says the same.  `BANK n` names a bank of the program's
own for `PEEK`, `POKE` and `SYS` at `$8000`-`$9FFF`.

## Values

A value is 5 bytes: a tag and 4 more (`basic.inc`).

| Tag | |
| :--- | :--- |
| `VT_INT` | An integer of 32 bits, in the value itself: most numbers a program uses |
| `VT_NUM` | Any other number, in the stored format of the numbers library (spec/numbers.def), in the heap |
| `VT_STR` | A string in the heap (bank `$FF`: `""`) |
| `VT_ARR`, `VT_REC` | An array or a record: its heap block (bank `$FF`: not made yet) |
| `VT_REF` | A slot given by reference (a parameter's) |
| `VT_LOC` | An element or a field given by reference: its array's or record's slot and the value's offset in its block |
| `VT_FRM`, `VT_GOS`, `VT_ERR` | A procedure's frame (its return, the caller's frame), `GOSUB`'s return, an error handler's mark |

## Compiling

`compile_prog` reads the text twice.  Pass 1 (`c_scan`) finds each `SUB` and `FUNCTION` (its parameters' kinds, its
entry), each `TYPE` (its fields, flattened: a record of another type's fields among them) and `DEFtype`'s letters,
so a procedure is known wherever it's written.  Pass 2 (`c_line`) compiles each line's statements.

* **The lexer** (`lex.inc`): keywords (a table by first letter), names (letters, digits, `_` and `.`; a suffix `$
  % & ! #` taken), strings, and numbers read by the numbers library in the program's base (`BASE`: a bare number
  starts with a digit), with QuickBASIC's `&H`, `&O`, `&B`, E notation in decimal (`e_conv` writes `1.5E-2` again
  as `0.015`, so it's exact), `.5`, and the Hydra's `#` forms (`#xFF`, `#b0.1`: the longest start that's one).  In a
  file's statements (`PRINT #`, `CLOSE #` ...) a `#` number form is taken back (`is_hash`), so `#x1` is the
  variable `x1`.
* **Symbols** (`comp.inc`): hashed by name and class (a number, a string, an array of each, a record, a label, a
  constant, a procedure) and scope (the main program, or a procedure); a global is an address in the RAM, a local a
  slot in its procedure's frame.
* **Labels** (a name and a colon, or a line's number) are addresses; one used before it's defined leaves a fixup,
  done at the end (`fix_all`: one never defined is `Label not defined`, at the line that named it).  At the prompt a
  label must be the program's already.
* **Blocks** (`IF`, `FOR`, `DO`, `WHILE`, `SELECT`, `SUB` ...) are a stack while they're compiled, each entry its
  jumps' chains and its line, so one left open is said at its start (`FOR without NEXT` at its `FOR`).
* **Calls** (`procs.inc`): an argument that's a variable, an array's element or a record's field goes by
  reference: a variable as `VT_REF`; an element or a field by its place (`AREF`, `FREF`: `VT_LOC`), its value
  loaded (`LDLOC`) and written back after the call (`WBK`).  Anything else is a copy.  A call keeps the state of the
  call or procedure it's compiled in (`call_save`, `call_load`): a `FUNCTION`'s call among a call's arguments, a
  `SUB`'s call in a procedure's body.
* **`INCLUDE "f"`** (or `'$INCLUDE: 'f'`), a line of its own: `line_next` reads f's lines there.  Pass 1 loads f
  into the text region after the program's (`inc_load`: the program's directory, else `/lib/basic`), pass 2 finds it
  by the `INCLUDE`s' order.  Its lines are numbered `$4000 + n * $800` on, so an error in it says its file and line.
* **Errors** at compile time go to `err_entry` with the line (`file:line: message`; a typed program's by its line's
  number, `line 20: ...`).

## Running

The interpreter (`run.inc`) runs the code from `ip`, an op a byte and its operands after it, by a table of ops:
jumps (`JMP`, `JF`, `JT`; Ctrl-C looked for after each jump taken, so `CONT` goes on at its target), `GOSUB` and
`RETURN`, `CALL` and `RET`/`RETF`, `FORT` and `FORN` (`FOR`'s test and `NEXT`, its variable, limit and step in
slots), loads and stores (`LDV`, `STV`, `REFV`), arrays' and records' elements and fields (`AGET`, `APUT`, `AREF`,
`FGET`, `FPUT`, `AGETF`, `APUTF`, `FREF`), the operators, and `OP_FN` and `OP_ST`: a function or a statement by its
number, in the bank its group is in (`fn.inc`'s `F_B4` ... and `ST_B4` ...).

* **A procedure's frame** (`CALL p`): its arguments (its parameters, first), the frame's mark (`VT_FRM`: the
  return, the caller's frame), then its locals, each its kind's default (`""`, 0, an array or record not made yet).
* **Errors** (`err_entry`): its code and line (the line table's, from the statement's code address, while code
  runs); to the program's `ON ERROR` handler if it has one (the value stack as the failing statement began, `RESUME`'s
  three ways from there), else said and back to the prompt (CONT's state kept), or a script's end (status 1).
  QuickBASIC's codes, and the system's errors (256 + its code, those QuickBASIC has a code for as QuickBASIC's).

## Numbers

Every number is the numbers library's (`modules/numbers`, the stored format of spec/numbers.def), called through
its bank of RAM (`num.inc`): an integer that fits 32 bits stays a `VT_INT` (the operators try that first), any other
lives in the heap.  The math library (`modules/math`) gives `SQR` ... `ATN` and `^` of a power that isn't whole,
exact when the answer is, else `DIGITS` significant digits.  `BASE` sets the library's base for `PRINT`, `STR$`,
`VAL`, `INPUT` and `READ`, and the compiler's (`cbase`) for the program's text after it; `RUN` starts in decimal.

## The heap

Strings, numbers past 32 bits, arrays and records are blocks in the heap (logical banks 26 on): a kind, a size, a
forwarding address, then its contents.  An array's block holds its dimensions (each its lowest index and count),
each element's values (a record's fields), then the values; a record's is an array's with no dimensions.  The
collector (`gc.inc`) marks and slides (Lisp 2's way): what the globals and the value stack reach is live, each live
block is given its new place, every value pointing at one is made to point there, and the blocks slide down.

## Input and output

Output goes through a buffer to stdout (`ofd`), its column followed for `PRINT`'s zones, `TAB`, `POS` and wrapping;
an error's message goes to stderr on a line of its own.  The console's sequences (`CLS`, `LOCATE`, `COLOR`) are
VT100's, keys come raw for `INKEY$` and `INPUT$`.  Files are fds of the system's, eight open at once (`#1` to
`#255`), each with its mode, its column and a read buffer.  `PRINT USING` reads its picture as QuickBASIC's (`#`,
`.`, `,`, `+`, `-`, `$$`, `**`, `!`, `\ \`, `&`, `_`) and the Hydra's `{}` fields.

## The prompt and the shell

The prompt keeps the program's text (the text region): a line typed with a number first goes in at its number,
any other is compiled and run at once, with the program's procedures and variables there after a `RUN`.  `LIST`,
`SAVE`, `LOAD`, `DELETE` and `RUN "f"` work on the text; `EDIT` writes it to `/ram/basicNN.bas`, runs `edit +N`
on it, and reads it again.  `basic -l` (`shell.inc`) is a login shell: its namespace made (`newns`),
`/lib/basic/profile.bas` run, then a line is BASIC's if it's a program line, `?`, a statement's keyword first, an
assignment or a call of the program's `SUB`, else rc's (`rc -c`, waited for); `cd`, `bind`, `mount`, `unmount`
and `newns` are its own.

## The Hydra's

* **Sound** (`machine.inc`): `SOUND` writes the sound driver's lines (`/dev/sndctl`); `PLAY` runs `play` with the
  line or the file, and waits for it.
* **The system**: `SYS "NAME"` finds a call in a table the build makes from the system's specification
  (`obj/gen/basicsys.inc`), `SYS addr` and `CALL ABSOLUTE` call machine code (at `$8000`-`$9FFF` in `BANK`'s bank),
  `RREG` reads the registers after.  `SHELL` and `SHELL$` run rc (`SHELL$` through a pipe, its last new lines
  dropped).
* **Graphics** (`gfx.inc`): `SCREEN`, `PSET`, `LINE`, `CIRCLE`, `PAINT`, `DRAW`, `GPRINT`, `PALETTE`, `SPRITE`,
  `WINDOW` and `VIEW` are the Vera X driver's commands (`/dev/vid/draw`); `PAINT` and `POINT` read the bitmap in VRAM.

## The tests

`tests/basic` is BASIC's suite (the bsuite test): twelve programs that check themselves (495 checks: arithmetic,
the number functions, logic, strings, arrays, control, procedures, records, data, errors, files, the Hydra's) and
four scripts piped into it, checked against their output (the errors' messages, `PRINT`'s layout, the prompt,
`INPUT`).  The basic test is BASIC at the console and as a shell; bplay `PLAY`; bawin `basic -l` in the windows;
bench `romfs/bench/bench.bas` against hylang's and HyForth's.  Writing the suite found bugs of BASIC's, among them:
`GOTO` a label never defined ran on (`ca_rd`'s flags), a `SUB` that called another `SUB` took its entry, a
`FUNCTION`'s call among a call's arguments, an exit's status always 0, an array's lowest index below 0.

## Against hylang and HyForth

`romfs/bench/bench.bas` has all twenty of the benchmarks (`sim/bench.js`), the same algorithms, sizes and results as
hylang's (`bench.hl`) and HyForth's (`bench.fs`), each in BASIC's own way: a `FUNCTION` each, its parameters (by
reference) and locals its own, recursion where they recurse, arrays where hylang has lists and HyForth memory, a
`FUNCTION`'s call where they call a function given (`map`, `filter`, `foldl`; `EXECUTE`).  `basic bench.bas
[reps [q|f [name...]]]` runs them (q: the bench test's quick sizes).  At 3.58 MHz, one run of each (`node
sim/bench.js`, October 2026: hylang 1 rep each in a hylang of its own, HyForth 5, BASIC 1):

| Kind | Benchmark | Result | HyForth ms | hylang ms | BASIC ms | BASIC/HyForth | BASIC/hylang |
| :--- | :-------- | -----: | ---------: | --------: | -------: | ------------: | -----------: |
| calls | `calls` | 2000 | 65 | 535 | 1,960 | 30.2x | 3.7x |
| calls | `fib` | 987 | 182 | 520 | 2,875 | 15.8x | 5.5x |
| calls | `tak` | 36 | 200 | 305 | 1,670 | 8.3x | 5.5x |
| calls | `ack` | 168 | 116 | 260 | 1,990 | 17.2x | 7.7x |
| loops | `loop` | 4000 | 77 | 400 | 1,805 | 23.4x | 4.5x |
| loops | `while` | 6000 | 424 | 725 | 3,505 | 8.3x | 4.8x |
| loops | `dotimes` | 6000 | 215 | 750 | 2,335 | 10.9x | 3.1x |
| loops | `nested` | 1800 | 311 | 825 | 3,005 | 9.7x | 3.6x |
| arith | `gcd` | 880 | 352 | 475 | 2,370 | 6.7x | 5.0x |
| arith | `collatz` | 1457 | 290 | 425 | 3,230 | 11.1x | 7.6x |
| arith | `hash` | 4072 | 673 | 715 | 2,170 | 3.2x | 3.0x |
| bytes | `sieve` | 172 | 334 | 1,115 | 3,690 | 11.0x | 3.3x |
| bytes | `sort` | 407 | 483 | 1,820 | 6,380 | 13.2x | 3.5x |
| bytes | `matrix` | 1375 | 1,088 | 970 | 2,950 | 2.7x | 3.0x |
| bytes | `queens` | 40 | 1,308 | 2,520 | 13,970 | 10.7x | 5.5x |
| lists | `mapf` | 9880 | 208 | 1,020 | 1,310 | 6.3x | 1.3x |
| lists | `fold` | 964 | 716 | 1,270 | 2,995 | 4.2x | 2.4x |
| lists | `each` | 700 | 210 | 450 | 2,410 | 11.5x | 5.4x |
| text | `chars` | 7 | 204 | 780 | 3,895 | 19.1x | 5.0x |
| text | `digits` | 2890 | 2,143 | 1,795 | 4,830 | 2.3x | 2.7x |
| All | | | 9,599 | 17,675 | 69,345 | 9.2x | 4.0x |

The last row's ratios are the geometric means; by kind, BASIC/HyForth: calls 16.2x, loops 11.9x, arithmetic 6.2x,
arrays (bytes) 8.1x, lists 6.7x, text 6.6x; BASIC/hylang 5.4x, 4.0x, 4.9x, 3.7x, 2.5x and 3.7x.  Over the six
benchmarks the first BASIC had (calls, fib, loop, gcd, sieve, sort), 14.9 times HyForth's time and 4.2 times hylang's,
where the first BASIC (EhyBASIC, Microsoft's 2A, retired for this one) was 44 and 12.3 times: three times as fast.

BASIC is nearest where a statement does much or a call is hylang's own cost too: `mapf` (1.3 times hylang's: its
`map` and `filter` make lists, BASIC calls a `FUNCTION` on integers), `fold` (2.4), and next to HyForth `digits`
(2.3: a number's text is the library's in both), `matrix` (2.7) and `hash` (3.2).  It's farthest where a step is
small: a `FOR` loop's step with a statement is some 1,600 cycles (`loop`, 23 times HyForth's `DO LOOP`), a
`FUNCTION`'s call some 1,900 more (`calls`, 30 times a `JSR`), a recursion's call deeper (`ack` 7.7 times hylang's,
`collatz` 7.6), and `MID$`'s new string for each character (`chars`, 19 times `C@`).  The interpreter's own time is
the next work: `FORN` and a call's frame (its arguments by reference, its mark, its locals' defaults) are the most of
it.
