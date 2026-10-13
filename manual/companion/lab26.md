# Lab 26. Taking Programs Apart

> **THIS LAB:** Manual: Chapter 28, sections 28.1 to 28.3; and Chapter 29, sections 29.1 to 29.7. About an hour.
> You need a Hydra, and `stars.s` from Lab 25, in `/ram`.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do: the disassembler

1. Assemble `stars.s` with a labels file, and take the program apart again:

```text
/ram> as -l stars.s
/ram> dis stars
/ram> dis -l stars.lbl stars
```

   ✎ What's the difference between the two listings? What has `dis` no way of getting back from the program?

<div class="write l3"></div>

2. Put it back together, and compare:

```text
/ram> dis stars >stars2.s
/ram> as stars2.s stars2
/ram> cmp stars stars2 && echo the same
```

   ✎ What did `cmp` say? What does that prove?

<div class="write l2"></div>

3. Now look inside a program you didn't write: `dis /rom/bin/echo | more`. ✎ Find one system call that `echo`
   makes. Which is it?

<div class="write"></div>

### Do: the debugger

4. Start `stars` in the debugger, and load the names:

```text
/ram> db ./stars
db> l /lib/as/hydra.inc
db> l stars.lbl
db> d main 9
```

   ✎ Copy the first line `db` printed when it started. What's in PC, and in A?

<div class="write l2"></div>

5. Step through the program an instruction at a time. After each `s`, write what changed.

| I typed | PC | A | The next instruction | What just happened |
|---|---|---|---|---|
| `s` | | | | |
| `s` | | | | |
| `s` | | | | |
| `s` | | | | |
| `s` | | | | |

6. Look at the program's variables: `m row 2`. ✎ What's in `row`? What's in `count`, and why?

<div class="write"></div>

7. Let it run to a **breakpoint**. Set one where the program is about to print a new line, and continue:

```text
db> b line+B
db> c
db> r
db> c
db> c
```

   ✎ What does the program print between each stop? What does `r` show, each time?

<div class="write l3"></div>

8. Change the program while it's stopped. Find the address of the `lda #$2A` (the star) with `d line 3`, and write
   a different character's code over the `2A`. For example, if the `A9 2A` is at $0844:

```text
db> w 0845 23
db> x
db> c
```

   ✎ What does the program print now? Is `stars.s` changed? Is the file `stars`?

<div class="write l2"></div>

9. If the program ran to its end, `db` has finished too. If it's still stopped, leave the debugger with `q`.

### Questions

1. ✎ What's a disassembler for, if you already have an assembler?

<div class="write l2"></div>

2. ✎ `dis` can't always tell code from data. Why not? What does it do about a byte that isn't an instruction?

<div class="write l2"></div>

3. ✎ What's the difference between `s` and `n` when the next instruction is a `jsr`?

<div class="write l2"></div>

4. ✎ What is a breakpoint? How does `db` make the program stop there? (Section 29.11.)

<div class="write l2"></div>

5. ✎ You find a bug with the debugger, and fix it by writing into memory. What must you still do?

<div class="write"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | disassemble a program, with and without its labels |
| ☐ | start a program in the debugger, and read the line of registers |
| ☐ | step, set a breakpoint, and continue |
| ☐ | look at memory, and change it |
| ☐ | say what each of `s`, `n`, `c`, `b`, `r`, `m`, `d` and `q` does |

> **CHALLENGE:** ★ Section 29.8 hunts a real bug in `show1`. Section 29.12 shows how to make `show1.s` from
> Chapter 27's `show.s`. Make it, run `./show1 nothing`, and then find the bug yourself with `db` *before* you
> read how the manual found it.
