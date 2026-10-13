# Chapter 29. The Debugger

A **bug** is a mistake in a program, and **debugging** is finding it. The hardest part is usually seeing what the
program is really doing, as opposed to what you meant it to do. A **debugger** shows you: it runs a program an
instruction at a time, or up to a place you choose, and lets you look at its registers and memory, and change them,
on the way. The Hydra's debugger is `db`.

> **IN THIS CHAPTER:** You'll run a program an instruction at a time, set breakpoints, look at its registers and
> memory and change them, and hunt down a real bug. **You need:** Chapters 26 and 27: it uses their programs `stars`
> and `show`.

```text
db program [arg ...]
db -p task
```

`db program` starts the program (with its arguments), stopped at its first instruction, in a task of its own. A name
with no `/` in it is looked for in `/bin`, so a program in this directory is `./name`. `db -p task` stops a task that's
already running, wherever it is, and debugs that.

## 29.1 The commands

`db` shows the registers, then waits at its prompt, `db>`, for a command:

| Command | What it does |
|---|---|
| `r` | Show the registers, and the next instruction |
| `s` [*n*] | **Step**: run one instruction (or *n*); a `jsr` steps into the subroutine |
| `n` [*n*] | **Next**: the same, but a `jsr` runs the whole subroutine, as one step |
| `c` | **Continue**: run until a breakpoint, a `BRK`, the program's end, or <kbd>CTRL</kbd>-<kbd>C</kbd> |
| `u` *addr* | **Until**: step (as `n` does) until the program reaches *addr* |
| `b` [*addr*] | Set a **breakpoint** at *addr*; with none, list them |
| `x` [*addr*] | Clear the breakpoint at *addr*; with none, clear them all |
| `d` [*addr*] [*n*] | **Disassemble** *n* instructions (12) from *addr* (the program counter, or where the last `d` ended) |
| `m` [*addr*] [*n*] | Show *n* bytes of **memory** (64) from *addr*, in hex and as characters |
| `w` *addr* *byte* ... | **Write** bytes (in hex) into memory, from *addr* |
| `l` *file* | Load names from a labels file, or from `hydra.inc` |
| `q` | **Quit**: the program `db` started is ended; a task it stopped with `-p` runs on |

Any other line shows a short list of the commands.

An **address** is a number in hex (with a `$` in front, or not), or a name from a labels file, with `+` or `-` and a
distance in hex after it if you like: `0842`, `$0842`, `line`, `main+9`. A count, *n*, is in decimal.

## 29.2 Starting

This chapter debugs Chapter 27's `stars`. Assemble it with `-l`, so there's a labels file, and start it in `db`:

<!-- session
cd /ram
!file samples/stars.s
-->

```session
/ram> as -l stars.s
/ram> db ./stars
task 3
PC=0830 A=30 X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0830  A9 01     lda #$01
db> l /lib/as/hydra.inc
108 symbols
db> l stars.lbl
4 symbols
```

`stars` is running in task 3, stopped before its first instruction. The line of registers shows:

| Part | What it is |
|---|---|
| `PC=0830` | The program counter: the next instruction is at $0830 |
| `A=30 X=FF Y=00` | The registers A, X and Y |
| `S=FD` | The stack pointer |
| `P=nv--dizc` | The flags: a capital letter for a flag that's 1, a small one for 0 (section 26.3) |
| `W=0 U=0` | The Hydra's W and U registers, as the task sees them |
| `RAM=00 ROM=00` | The task's RAM bank and ROM bank registers ($00 and $01) |

Under it is the next instruction: its address, its bytes, and what it is.

`l` loads names. `/lib/as/hydra.inc` has the system calls' and the call registers' names, and `stars.lbl` the
program's own labels, so `db` can write `jsr PUTC` and `sta row` instead of `jsr $F950` and `sta $22`.

## 29.3 Looking at the program

`d` disassembles. Here is all of `stars`:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
db ./stars
l /lib/as/hydra.inc
l stars.lbl
-->

```session
db> d main 19
0830  A9 01     lda #$01                 ; main
0832  85 22     sta row                  ; main+2
0834  A5 22     lda row                  ; main+4
0836  20 42 08  jsr line                 ; main+6
0839  E6 22     inc row                  ; main+9
083B  A5 22     lda row                  ; main+B
083D  C9 06     cmp #$06                 ; main+D
083F  D0 F3     bne main+4               ; main+F
0841  60        rts                      ; main+11
0842  85 23     sta count                ; line
0844  A9 2A     lda #$2A                 ; line+2
0846  20 50 F9  jsr PUTC                 ; line+4
0849  C6 23     dec count                ; line+7
084B  D0 F7     bne line+2               ; line+9
084D  A9 0A     lda #$0A                 ; line+B
084F  20 50 F9  jsr PUTC                 ; line+D
0852  60        rts                      ; line+10
0853  07 AF     rmb0 count+8C            ; line+11
0855  85 F6     sta count+D3             ; line+13
```

After each instruction, `db` says where it is, by the nearest label before it. The last two lines aren't code: `d`
doesn't know where the program ends, and the bytes after the last `rts` make nonsense instructions.

## 29.4 Stepping

`s` runs one instruction, and shows the registers after it:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
db ./stars
l /lib/as/hydra.inc
l stars.lbl
-->

```session
db> s
PC=0832 A=01 X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0832  85 22     sta row                  ; main+2
db> s
PC=0834 A=01 X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0834  A5 22     lda row                  ; main+4
db> m row 2
0022  01 F6                    ..
db> s 3
0836  20 42 08  jsr line                 ; main+6
0842  85 23     sta count                ; line
PC=0844 A=01 X=FF Y=00 S=FB P=nv--dizc W=0 U=0 RAM=00 ROM=00
0844  A9 2A     lda #$2A                 ; line+2
```

* The first `s` ran `lda #$01`: A is 1. The second ran `sta row`.
* `m row 2` shows 2 bytes of memory from `row`: `row` is 1, as it should be. `count` is $51: whatever was left in
  that byte, since the program hasn't set it yet.
* `s 3` ran three instructions, showing each one as it went: `lda row`, the `jsr`, and, inside `line`, `sta count`.
  S went from $FD to $FB: the `jsr` pushed its return address, two bytes, on the stack.

## 29.5 Breakpoints

Stepping through a whole program would take forever. A **breakpoint** stops the program when it gets to a place:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
db ./stars
l /lib/as/hydra.inc
l stars.lbl
s
s
s 3
-->

```session
db> b line
db> b
1 0842  85 23     sta count                ; line
db> c
*
breakpoint 1
PC=0842 A=02 X=FF Y=00 S=FB P=nv--dizc W=0 U=0 RAM=00 ROM=00
0842  85 23     sta count                ; line
```

`b line` set a breakpoint at `line`, and `b` alone listed it. `c` let the program run: it finished the first row,
printing its star, and stopped when it called `line` again, for the second row. A is 2, the number of stars for this
row. You can have 16 breakpoints, but only in RAM: `db` makes one by putting a `BRK` instruction in the program while it
runs, and taking it out again when it stops, so it can't make one in ROM.

## 29.6 Until, and next

`u` runs (a step at a time) until the program reaches an address. `u main+9` runs to the instruction after
`jsr line`, which finishes this call of `line`:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
db ./stars
l /lib/as/hydra.inc
l stars.lbl
s
s
s 3
b line
c
-->

```session
db> u main+9
**
PC=0839 A=0A X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0839  E6 22     inc row                  ; main+9
db> x
db> n 4
083B  A5 22     lda row                  ; main+B
083D  C9 06     cmp #$06                 ; main+D
083F  D0 F3     bne main+4               ; main+F
PC=0834 A=03 X=FF Y=00 S=FD P=Nv--dizc W=0 U=0 RAM=00 ROM=00
0834  A5 22     lda row                  ; main+4
db> n 2
0836  20 42 08  jsr line                 ; main+6
***
PC=0839 A=0A X=FF Y=00 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0839  E6 22     inc row                  ; main+9
```

* `u main+9` printed the second row's two stars, and stopped back in `main`. (A is $0A, the code of the new line
  `line` printed last.)
* `x` cleared the breakpoint.
* `n 4` stepped four times: `inc row`, `lda row`, `cmp #$06` and `bne main+4`, which went back to `main+4`. Look at
  the flags: `cmp` compared 3 with 6, and 3 - 6 is negative, so N is 1, and C is 0 (3 is less than 6).
* `n 2` stepped over `lda row` and then the whole `jsr line`, as one step: it printed the third row, and stopped
  after it. `s` would have gone into `line`.

`s` and `n` both run a system call (a `jsr` to HydraOS's jump table) whole: HydraOS's own code can't be stepped. And
`u` steps as `n` does, so it only stops at an address in the same subroutine, or one it returns to: `u line` from
`main` would run straight past, because `n` runs `line` whole.

## 29.7 Changing the program

`w` writes bytes into the program's memory, while it's stopped. The star is the operand of `lda #$2A`, the byte at
$0845. Write $23, the code of `#`, there, and let it run on:

<!-- session
cd /ram
!file samples/stars.s
as -l stars.s
db ./stars
l /lib/as/hydra.inc
l stars.lbl
s
s
s 3
b line
c
u main+9
x
n 4
n 2
-->

```session
db> w 0845 23
db> c
####
#####
task 3 ended: code 0
```

The last two rows are made of `#`. The program ended, and so did `db`. The change was only in memory: the file
`stars` is just as it was. So when you've found a fix this way, put it in the source, and assemble it again. (`db`
can change memory, but not the registers.)

## 29.8 Finding a bug

Here's a real bug. Chapter 27's `show` was first written without its `pha` and `pla`. That first version is
`show1.s` (section 29.12 shows how to make it). It worked, except when the file wasn't there:

<!-- session
cd /ram
!file samples/show1.s
-->

```session
/ram> as -l show1.s
/ram> ./show1 nothing
show: bad address
```

That's wrong: the file isn't there, so it should say `not found`. The message comes from `ERRSTR`, at `@fail`. So
stop the program there, and look at what it gives `ERRSTR`:

<!-- session
cd /ram
!file samples/show1.s
as -l show1.s
-->

```session
/ram> db ./show1 nothing
task 3
PC=0830 A=30 X=FF Y=20 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0830  B2 02     lda ($02)
db> l /lib/as/hydra.inc
108 symbols
db> l show1.lbl
4 symbols
db> d main+29 3
0859  A9 00     lda #$00                 ; main+29
085B  20 C0 F8  jsr OPEN                 ; main+2B
085E  B0 38     bcs main+68              ; main+2E
db> b main+68
db> c
breakpoint 1
PC=0898 A=20 X=2C Y=03 S=FD P=nv--dizC W=0 U=0 RAM=00 ROM=00
0898  A9 D1     lda #$D1                 ; main+68
db> s 4
089A  85 02     sta r0                   ; main+6A
089C  A9 09     lda #$09                 ; main+6C
089E  85 03     sta r0+1                 ; main+6E
PC=08A0 A=09 X=2C Y=03 S=FD P=nv--dizC W=0 U=0 RAM=00 ROM=00
08A0  20 03 F8  jsr ERRSTR               ; main+70
db> q
```

* `d main+29 3` found the `OPEN`. If it fails, `bcs main+68` goes to `@fail`. (A cheap local label isn't in the
  labels file, so `db` calls it by how far it is from `main`.)
* `b main+68` and `c` stopped there. C is 1 (`OPEN` failed), and A is $20: Appendix G says that's `not found`.
  Right so far.
* `s 4` stepped through `LDR r0, msg`'s four instructions, and now, just before `jsr ERRSTR`, A is $09: `LDR`'s
  `lda #>msg` put the high byte of `msg`'s address in it. And $09 is `bad address`. Found it!

The fix keeps A safe on the stack while `LDR` works: `pha` before it, and `pla` after, as `show.s` has. Most bugs
are like this: something changed that you didn't expect to change. The debugger shows you exactly where.

## 29.9 Stopping a running program

`c` runs until something stops the program. If it never stops, <kbd>CTRL</kbd>-<kbd>C</kbd> does: it goes to `db`
(the program runs in a note group of its own, so the note doesn't reach it), and `db` stops the program wherever it
is. The `tick` sample prints a dot a second, for ever:

<!-- session-script
!line db /sd/0/sample/tick
!line c
!wait 30
!raw ^C
!quiet
!line q
-->

```session
/> db /sd/0/sample/tick
task 3
PC=0830 A=30 X=FF Y=02 S=FD P=nv--dizc W=0 U=0 RAM=00 ROM=00
0830  9C 95 08  stz $0895
db> c
........PC=EB31 A=06 X=0E Y=03 S=FA P=nv--dIzc W=0 U=0 RAM=00 ROM=00 (in a call: its state 6)
EB31  A5 A2     lda $A2
db> q

/>
```

After eight dots, <kbd>CTRL</kbd>-<kbd>C</kbd> stopped it. `(in a call: its state 6)` means it was inside a system
call, `SLEEP` (state 6 is sleeping): the PC, $EB20, is in HydraOS's kernel. You can't step there, but `c` runs it on,
and `q` ends it.

<kbd>CTRL</kbd>-<kbd>C</kbd> stops an `n` or a `u` that's taking too long, too, and so does a breakpoint inside a
subroutine that `n` is running. If the program stops in a subroutine of its own, carry on from there as you like:
`s`, `n` and `c` all work. If it stops inside a system call that you were stepping over, `s` or `n` lets the call
finish and stops after it, and `c` runs on.

## 29.10 Debugging a running task

`db -p` stops a task that's already running, to look at it. Here a `sleep` runs in the background (it's task 2, as
`ps` would say):

<!-- session
-->

```session
/> sleep 100 &
/> db -p 2
task 2
PC=EB31 A=06 X=51 Y=02 S=F9 P=nv--dIzc W=0 U=0 RAM=00 ROM=00 (in a call: its state 6)
EB31  A5 A2     lda $A2
db> q
```

`q` lets a task `db` stopped run on, as if nothing had happened.

## 29.11 How db works

`db` uses the files of `/proc` (section 17.8). `/proc/3/ctl` takes the commands `stop`, `start`, `step`, `next`,
`break` and `nobreak` (and `kill`, which `q` uses), and `/proc/3/mem` is task 3's memory, which `db` reads and writes
like a file. While it's debugged, a task's `BRK` instructions stop it, instead of ending it, which is how breakpoints
work. And HydraOS keeps a stopped task's registers where `db` can read them.

So `db` is an ordinary program, with no special powers: it only uses files that any program can. That's the Plan 9
way of doing things, which HydraOS follows (Chapter 9).

## 29.12 Making show1.s

`show1.s` is `show.s` (section 27.14) with three small changes, so you don't need to type it all again. Copy
`show.s` to `show1.s`, and open the copy in `edit`:

1. In the first line's comment, and in the `HYX2_PROGRAM` line, change `show` to `show1`:

```text
; show1 file - show.s, as it was first written (it has a mistake)
```

```text
            HYX2_PROGRAM "show1", main
```

2. Near the end, find the three lines that start at `@fail`:

```text
@fail:      pha                 ; The error's text (LDR changes .A: kept)
            LDR r0, msg
            pla
```

and make them one, without the `pha` and the `pla`:

```text
@fail:      LDR r0, msg         ; The error's text
```

That missing pair is the bug that section 29.8 hunts down.
