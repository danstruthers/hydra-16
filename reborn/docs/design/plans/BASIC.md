## **Hydra BASIC: a structured BASIC for the Hydra, on the shared numbers**

A plan (October 2026) for the user's direction: BASIC doesn't need to keep Microsoft's syntax; it should be a BASIC for the Hydra, more like the later QuickBASIC: **line numbers optional**, **labels** to branch to instead of line numbers, and the structured statements that came with them.  Its numbers are the shared ones ([NUMBERS.md](NUMBERS.md): danlang's and hylang's number system, every format, a current format to show and read numbers in, every number stored in the compact format).  It replaces today's BASIC (EhyBASIC, Microsoft BASIC 2A: [basic.md](../../basic.md)), which is retired when this one is done.  The user's answers to the plan's questions are in it (and listed at its end): today's BASIC retired; QuickBASIC's types, with one number type; parameters by reference; a buffer at the prompt with an editing mode; `RETURN` in a function; every number shown exactly; graphics in the first version.

### **Contents**
1. [What it keeps, and what changes](#what-it-keeps-and-what-changes)
2. [Programs, lines and labels](#programs-lines-and-labels)
3. [Values and names](#values-and-names)
4. [Control](#control)
5. [Procedures](#procedures)
6. [Errors](#errors)
7. [Input and output](#input-and-output)
8. [The Hydra](#the-hydra)
9. [At the prompt, and as a shell](#at-the-prompt-and-as-a-shell)
10. [How it runs](#how-it-runs)
11. [Tests and documents](#tests-and-documents)
12. [The order of work](#the-order-of-work)
13. [Questions](#questions)
14. [Answers](#answers-the-users-7-october-2026)

---

### **What it keeps, and what changes**

**Kept**: BASIC's statements and functions where QuickBASIC kept them (`PRINT`, `INPUT`, `LET`, `IF`, `FOR`, `GOTO`, `GOSUB`, `DATA`, `READ`, `DIM`, `LEFT$` ...), and everything the Hydra's BASIC added: files, `SOUND`, `PLAY`, `BEEP`, `SLEEP`, `SYS` and `RREG`, `ENV$`, `ARG$`, scripts (`basic prog.bas a b`, `#!/bin/basic`), the shell (`basic -l`), Ctrl-C.

**Changed**, as QuickBASIC changed it:
* Line numbers optional; labels (`loop:`) for `GOTO`, `GOSUB`, `RESTORE` and `ON ERROR`; a program a text file, or the buffer at the prompt, written in its editing mode, or line by line with numbers as before.
* QuickBASIC's types, with one number type: numbers (the tower's, exact), strings (`$`), fixed-length strings and records (`TYPE`); QuickBASIC's number suffixes and type names (`%`, `&`, `!`, `#`, `AS INTEGER`, `DEFDBL` ...) accepted, every one the same number.
* Block statements: `IF` ... `ELSEIF` ... `ELSE` ... `END IF`, `SELECT CASE`, `DO` ... `LOOP`, `WHILE` ... `WEND`, `EXIT`.
* `SUB` and `FUNCTION` with parameters and local variables, recursion.
* Names of any length, keywords only as whole words (`SCORE` is a name, not `SC OR E`), keywords and names in either case.
* `ON ERROR GOTO`, `RESUME`, `ERR`.
* Numbers: the shared tower (exact integers of any size, fixed decimals, fractions, complex numbers), not Microsoft's 40-bit floating point.

Microsoft BASIC's quirks go: two significant characters in a name, keywords found inside names, `DEF FN` (a `FUNCTION` instead), `USR`, a program's text tokenized as it's typed.  An old program with line numbers mostly runs, with spaces between its words where it ran them together.

### **Programs, lines and labels**

```
' count.bas: the primes below a number
INPUT "Below"; limit
FOR n = 2 TO limit - 1
    IF isPrime(n) THEN PRINT n;
NEXT
PRINT
END

FUNCTION isPrime (n)
    FOR d = 2 TO SQR(n)
        IF n MOD d = 0 THEN isPrime = 0: EXIT FUNCTION
    NEXT
    isPrime = -1
END FUNCTION
```

* A program is a text file, `.bas`, of lines; a line is statements separated by `:`.  `'` and `REM` start a comment.
* **A label** is a name and a colon at a line's start (`again:`), on a line of its own or before a statement.  **A line number** at a line's start is a label too (`100 PRINT "hi"`, then `GOTO 100`), so programs with line numbers run; numbers need not be in order, nor on every line.
* `GOTO`, `GOSUB`, `RESTORE`, `ON ... GOTO`, `ON ... GOSUB`, `ON ERROR GOTO` and `RESUME` take a label or a line number.
* A long line goes on after a `_` at its end.
* `DECLARE` is optional (the program is read whole before it runs, so a `SUB` or `FUNCTION` is known wherever it's written); `INCLUDE "file.bas"` (QuickBASIC's `'$INCLUDE`) adds another file's procedures, from the current directory or `/lib/basic`, through the `/lib` union.

### **Values and names**

* **Numbers**: the shared ones ([NUMBERS.md](NUMBERS.md)): `10 / 4` is `5/2`, `0.1 + 0.2 = 0.3` is true, `2 ^ 100` is exact; written in any format (`&HFF`, `&O17` and `&B101` as QuickBASIC wrote them, and danlang's `#xFF`, `#b101`, `#c+-0`, `#16r1F` ...); shown and read in the current format (`NBASE "x"`); the math functions at `DIGITS` digits.  Truth is -1 and 0, as BASIC's.
* **Strings**: `$` names (`name$`), of any length to 64K (Microsoft's stopped at 255), in a heap with a compacting collector.
* **Names**: letters, digits, `_` and `.` (QuickBASIC's), any length, all significant; a keyword can't be a name.  A name is a string's with `$` (or `AS STRING`, or `DEFSTR`'s letters), else a number's.  QuickBASIC's number suffixes are accepted and dropped: `x%`, `x&`, `x!`, `x#` and `x` are one variable, as there's one number type.
* **Arrays**: `DIM a(10)`, `DIM grid%(1 TO 8, 1 TO 8)`, `DIM names$(100)`, `DIM v(5) AS DOUBLE`; any number of dimensions; `REDIM`, `ERASE`, `LBOUND`, `UBOUND`; an array used before `DIM` has 0 to 10, as BASIC's always had.
* **Constants**: `CONST pi2 = 2 * PI`.

**Types**: one number type, strings, and QuickBASIC's records:

| Type | Written | Holds |
| :--- | :------ | :---- |
| A number | No suffix, or QuickBASIC's (`%`, `&`, `!`, `#`), all the same; `AS NUMBER`, and `AS INTEGER`, `AS LONG`, `AS SINGLE`, `AS DOUBLE` the same; `DEFINT`, `DEFLNG`, `DEFSNG`, `DEFDBL` accepted, changing nothing | Any number of the tower, exact |
| A string | `$`, `AS STRING`, `DEFSTR` | A string of any length to 64K |
| A fixed-length string | `AS STRING * n` | Exactly n characters (padded with spaces, or cut, as it's stored) |
| A record | `TYPE point` ... `END TYPE`, then `AS point` | Fields, each a number, a string or a record (`p.x`); records in records; arrays of records (`pts(i).x`) |

* **One number type**: every number is the tower's, exact, and stored in the compact format ([NUMBERS.md](NUMBERS.md)); nothing rounds a number as it's stored.  `a / b` is a fraction when it isn't whole; `\` (integer division) and `MOD` are QuickBASIC's; `CINT` and `CLNG` round to an integer (a half to the even one, QuickBASIC's), `CSNG` and `CDBL` leave a number as it is.
* **Shown exactly**: `PRINT`, `STR$` and the rest show every number as it is, never rounded: `1/3` as `1/3`, `2 ^ 100` whole, in the current format.  They write it with the numbers library's `num_display`, and `VAL` and `INPUT` read with its `num_parse`, in the format `NBASE` selects (the library's: [NUMBERS.md](NUMBERS.md), "The current format").
* **A suffix, and a number's text**: `x#` is a number's name, `#xFF` a number in base x, `#1` a file (after `PRINT`, `INPUT`, `OPEN ... AS` and the rest); `x&` a name, `&HFF` a number.

### **Control**

| Statement | |
| :-------- | :- |
| `IF c THEN s [ELSE s]` | On one line, as BASIC's |
| `IF c THEN` / `ELSEIF c THEN` / `ELSE` / `END IF` | Blocks, nested to any depth |
| `SELECT CASE x` / `CASE 1, 3, 5` / `CASE 10 TO 20` / `CASE IS > 100` / `CASE ELSE` / `END SELECT` | Numbers or strings |
| `FOR i = a TO b [STEP s]` / `NEXT [i]` | Exact steps (`STEP 0.1` reaches 1 exactly) |
| `DO [WHILE c \| UNTIL c]` / `LOOP [WHILE c \| UNTIL c]` | The test at either end, or neither (`EXIT DO` ends it) |
| `WHILE c` / `WEND` | |
| `EXIT FOR`, `EXIT DO`, `EXIT SUB`, `EXIT FUNCTION` | |
| `GOTO label`, `GOSUB label` / `RETURN`, `ON n GOTO a, b, c`, `ON n GOSUB a, b, c` | Labels or line numbers |
| `END`, `STOP` | `STOP` keeps the program's state for `CONT` |

A block is checked as the program is read: an `IF` without its `END IF`, a `NEXT` without its `FOR`, a `GOTO` to no label are errors before it runs, with their file and line.

### **Procedures**

* `SUB name (a, b$, c(), d AS INTEGER, p AS point)` ... `END SUB`, called as `name 1, "x", list(), 5, pt` or `CALL name (1, "x", list(), 5, pt)`.
* `FUNCTION name (a, b)` ... `END FUNCTION`: its value set by assigning to its name (`name = a * b`, QuickBASIC's), or given by `RETURN a * b`, which leaves it too; used in expressions (`y = name(2, 3)`); a `$` function gives a string.  (`RETURN` alone in a function leaves it with the value its name has; outside a function, it's `GOSUB`'s.)
* **Local variables**: a procedure's names are its own; `SHARED x, y()` reaches the program's; `STATIC` keeps a procedure's between calls.  Recursion to the depth memory allows.
* **Parameters by reference**, as QuickBASIC's: a variable, an array's element, a field or a whole array passed (`swap a, b`, `sort list()`) is the procedure's to change; an expression, a constant or a variable in brackets (`(x)`) passes a copy.  A variable passed must be of the parameter's kind (a number, a string, a fixed-length string, a record of the same `TYPE`), else `Parameter type mismatch`, as the program is read.
* The program's main part is the lines outside procedures.

### **Errors**

* `ON ERROR GOTO label` (`0` to turn it off); in the handler `ERR` (the code), `ERL` (the line), `ERR$` (its message, the system's text for a system error); `RESUME`, `RESUME NEXT`, `RESUME label`; `ERROR n` raises one.
* The error codes: BASIC's own, and the system's (`?NOT FOUND`: its `E_*` code), one list.
* Without a handler, an error stops the program with its message and the file and line: `count.bas:12: division by zero`.

### **Input and output**

* **The console**: `PRINT` (`;`, `,`; each number written by the numbers library's `num_display`, exactly, in its current format, with QuickBASIC's spaces around it), `PRINT USING "##.##"` (numbers in a picture, strings `&`), `INPUT ["prompt";] a, b$`, `LINE INPUT a$`, `INKEY$` (a key, or `""`), `CLS`, `LOCATE row, col`, `COLOR fg[, bg]` (conio's colours, as HyForth's and hylang's terminal words), `WIDTH`, `BEEP`.
* **Files**, QuickBASIC's: `OPEN "path" FOR INPUT | OUTPUT | APPEND | BINARY AS #n`, `PRINT #n`, `WRITE #n` (quoted, comma-separated), `INPUT #n`, `LINE INPUT #n`, `GET #n` and `PUT #n` (bytes, in `BINARY`), `SEEK`, `EOF(n)`, `LOF(n)`, `CLOSE`, `FREEFILE`; any number of files open, the system's limit; paths through the namespace (`/sd/0/data.txt`, `/pc/x`).
* **Files as the system has them**: `KILL` (remove), `NAME a AS b` (rename), `MKDIR`, `RMDIR`, `CHDIR`, `FILES` (a directory's names), `DIR$`.
* **`DATA`, `READ`, `RESTORE [label]`**, as BASIC's.

### **The Hydra**

* **Sound**: `SOUND`, `PLAY`, `BEEP`, as today's BASIC has them (sndctl's text, the score language).
* **The system**: `SHELL "rc line"` (QuickBASIC's name: rc runs it, waited for, its status then `STATUS`, as HyForth's and hylang's `status`), `SHELL$("line")` (its output, a string: hylang's `sh-out`), `SYS "NAME"` and `RREG` (any system call by its name), `ENV$("name")` (QuickBASIC's `ENVIRON$` too) and `ENVIRON "name=value"`, `ARG$(n)`, `TIMER` (seconds since midnight, as QuickBASIC's, from the clock, to the tick), `DATE$`, `TIME$`, `SLEEP`.
* **Memory**: `PEEK`, `POKE`, `BANK` (the task's RAM bank at `$8000`), `CALL ABSOLUTE` or `SYS addr` (machine code).
* **Graphics**, in the first version, on the Vera X: QuickBASIC's `SCREEN` (a mode: text, or a bitmap of the Vera's), `PSET`, `PRESET`, `LINE` (`B`, `BF`), `CIRCLE`, `PAINT`, `DRAW`, `PALETTE`, `COLOR`, `POINT`, `VIEW` and `WINDOW`, and the Vera's sprites; built with the languages' graphics words (VIDEO.md's step: HyForth's, hylang's and C's, which go ahead now), the same operations under each language's names.
* Names checked against C's, HyForth's and hylang's for the same things, as the system's rule is.

### **At the prompt, and as a shell**

**The buffer**: at the prompt BASIC keeps a program in a buffer, as QuickBASIC's environment did.  `NEW` empties it; `LOAD "f.bas"` reads a file into it; `SAVE ["f.bas"]` writes it (with no name, to the file it came from); `RUN` runs it (read and checked first), `RUN "f.bas"` loads and runs one; `LIST [from-to]` shows it (lines, or from a label to a label); `CONT` goes on after `STOP` or Ctrl-C.  After a run, its variables and procedures are still there for what's typed at the prompt.

**Two modes**:
* **Immediate mode**, the prompt: a statement typed runs at once (`PRINT 2 ^ 64`, `x = 5`, `plot 1, 2` with the buffer's `SUB plot`).  A line typed with a number first goes into the buffer at its number, as BASIC's always did (`DELETE 100-200` takes lines out), so a program can still be typed line by line.
* **Editing mode**: `EDIT [label]` enters it, the buffer in the system's screen editor (`edit`, nano's keys), at the label, or at the line of the last error; leaving the editor (`^X`) is leaving the mode, back at the prompt with the buffer as it was edited, read and checked again (its first error said with its line, for `EDIT` to go to).  Lines without numbers are written here.  (The buffer goes to `edit` as a file in the shell's `/ram`, and comes back from it.)

* `basic prog.bas args`, and `#!/bin/basic`, run a file as a script.
* `basic -l`, a shell, as now: a line is BASIC's when it starts with a keyword or an assignment, else rc's; `%` makes it rc's; `cd`, `bind`, `mount`, `unmount`, `newns` the shell's own; `&`; `/lib/basic/profile.bas`.

### **How it runs**

* **Read whole, then run**: a program is read into the task's memory (and its banks, past 32K) and turned into a compact form: keywords as tokens, every name a variable's slot (resolved once, not looked up as it runs), every label and line number an address, every number constant in the stored format, each block's ends linked (an `IF`'s `ELSE` and `END IF`, a `DO`'s `LOOP`).  `LIST` and `SAVE` write the text as it was read; `EDIT` works on the text.
* **An interpreter** over that form: a value stack of its own (not the 6502's: no limit of nesting), a frame for each procedure's call (its locals and parameters), the heap for strings and long numbers with a compacting collector that knows every reference.  Numbers that fit a value's 5 bytes are worked there; small integers add, subtract and compare without a library call; the rest go to the `numbers` and `math` libraries.
* **Speed**: names resolved and numbers read once make it much quicker than Microsoft's (which searches its variables and reads a constant's digits each time).  `bench.bas`, written again in the new BASIC, measures it against hylang and HyForth: the target, nearer hylang's than Microsoft's 12 times.
* **Size**: a module of three or four banks, in assembly, as the system's languages are; today's BASIC is one bank (93% full).

### **Tests and documents**

* A new suite in `tests/basic` (programs that check themselves, scripts against their output, as today's), for every statement and function; the shell's tests and `bench.bas` in the new BASIC; the numbers' cross-check from NUMBERS.md.
* `docs/basic.md` (the design) and `docs/using/basic.md` (the guide) written again, the guide a reference of every statement, as QuickBASIC's help was; the guide and its PDF.

### **The order of work**

| Step | Work | Size |
| :--- | :--- | :--- |
| 1 | **The language**: this plan's questions answered, and the reference written first (`docs/using/basic.md`), every statement and function | M |
| 2 | **Reading a program**: lines, labels, line numbers, tokens, names to slots (numbers, strings, records: `TYPE`), blocks linked and checked, errors with file and line | L |
| 3 | **The core**: expressions, numbers (NUMBERS.md's libraries, steps 1-4 there first), strings and the heap, variables, arrays, records, `IF`, `FOR`, `DO`, `WHILE`, `SELECT`, `GOTO`, `GOSUB`, `PRINT`, `INPUT`, `DATA` | L |
| 4 | **Procedures**: `SUB`, `FUNCTION`, locals, `SHARED`, `STATIC`, parameters by reference, recursion, `EXIT` | M |
| 5 | **Errors, files, the console's screen**: `ON ERROR`, `RESUME`, `OPEN` and the rest, `PRINT USING`, `LOCATE`, `COLOR`, `INKEY$` | M |
| 6 | **The Hydra and the prompt**: sound, `SHELL`, `SYS`, memory; the buffer, immediate and editing modes; scripts, `basic -l` | M |
| 6a | **Graphics**: QuickBASIC's statements on the Vera X, on the languages' graphics words (VIDEO.md's step, under way beside this) | M |
| 7 | **Tests, `bench.bas`, documents**; today's BASIC retired (`modules/basic` replaced, its tests and documents with it) | M |

Steps 1 and 2 can start at once, beside NUMBERS.md's first steps; step 3 needs its `numbers` and `math` libraries.

### **Questions**

None open: the user answered them all (below).  The numbers' own questions are NUMBERS.md's.

### **Answers** (the user's, 7 October 2026)

1. **Today's BASIC is retired**: no `msbasic`; the new BASIC is `basic`.
2. **QuickBASIC's types, with one number type**: its suffixes, `AS` and `DEFtype` accepted; every number the tower's, exact (no `INTEGER` or `SINGLE` rounding); strings, fixed-length strings and records (`TYPE`) as QuickBASIC's.
3. **Parameters by reference**.
4. **The prompt keeps a buffer**, with a way into and out of an editing mode: immediate mode and editing mode (`EDIT`, the screen editor), above; numbered lines typed at the prompt still go into it.
5. **`RETURN value` in a function**, beside assigning to its name.
6. **Every number shown exactly**: a fraction as a fraction (`1/3`), never rounded for display.
7. **Graphics in the first version**, with the languages' graphics words, which go ahead now as anticipated.
