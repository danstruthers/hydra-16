# The monitor

When the base runs on its own, its init (task 1) is **wozmon**: Steve Wozniak's monitor from the Apple 1, as the
Hydra-16's first software had it, with a disassembler added.  It's a way to look at memory, change it, and run what's
there, from the serial console: enough to bring up a new board, try an instruction, or poke a device's registers.

It's a program on the kernel's calls (`base/modules/wozmon/wozmon.s`), not code in the BIOS ROM, so it sees the machine
as any task does, and a program it runs can call the kernel.

## Contents

1. [The prompt and the commands](#the-prompt-and-the-commands)
2. [A session](#a-session)
3. [What it sees](#what-it-sees)
4. [Writing a program with it](#writing-a-program-with-it)
5. [Without a console driver](#without-a-console-driver)

---

## The prompt and the commands

```
T1 00>
```

The prompt is the task (`T1`), the RAM bank at `$8000` (`00`; a shared bank, `$F0` up, shows its `U` too: `T1 F2(3)>`),
and `>`.  A line holds any number of commands, as Woz's did, separated by spaces; hex digits in upper or lower case:

| Command | What it does |
| :------ | :----------- |
| `XXXX` | **Examine**: the byte at `XXXX`, on a line of its own (`XXXX: BB`) |
| `XXXX.YYYY` | **A block**: `XXXX` to `YYYY`, 8 bytes a line.  `.YYYY` alone goes on from where the last stopped |
| `XXXX: BB BB ...` | **Store**: the bytes from `XXXX` on (it shows `XXXX`'s old byte first).  `: BB` alone goes on after the last stored |
| `XXXXR` | **Run**: a `JSR` to `XXXX` (`R` alone: the last address shown).  The program's `RTS` comes back to the prompt |
| `L` | From now on, an examine shows **instructions**: one a line, its bytes and its text as the assembler `as` writes it (`lda #$41`) |
| `K` | Back to bytes |

**Keys:** Backspace (or Delete) takes back a character; Escape gives up the line (Woz's `\`).  **Ctrl-C** stops whatever
is running, a program started with `R` too, and brings the prompt back; so does any other note (a `BRK` in a program is
the note `brk`).

---

## A session

Store a six-byte program at `$1000`, list it as instructions, run it, then look at the VIA's registers and the start
of the jump table (captured from the emulator):

```
T1 00> 1000: A9 48 20 50 F9 60
1000: 2E
T1 00> L 1000.1005
1000: A9 48     lda #$48
1002: 20 50 F9  jsr $F950
1005: 60        rts
T1 00> 1000R
1000: A9 48     lda #$48H
T1 00> K FF00.FF0F
FF00: FF FF 00 00 04 21 E8 45
FF08: 0F 03 00 40 00 00 E0 FF
T1 00> F800.F80F
F800: 4C 77 EB 4C B0 EB 4C FD
F808: ED 4C 2B EB 4C 4C EB 4C
```

* The store showed `$1000`'s old byte (`2E`: RAM powers up random).
* `$F950` is `PUTC`, so the program prints `H` (`$48`) and returns.  `R` showed the address it ran first, as an
  instruction since `L` was on.
* `FF00.FF0F` is the VIA, port 0: its timers counting.
* `F800` is the jump table: a `jmp` (`4C`) for each call.

---

## What it sees

Task 1's view of the machine, as any task's:

* **RAM**, `$0000-$7FFF`: the task's own.
  * `$02-$21` are the call registers `r0`-`r15`, and `$22-$7F` the monitor's own zero page.
  * `$80-$FF` and `$0200-$03FF` are the kernel's areas for the task: don't write them.
  * `$0100-$01FF` is the stack, and the monitor's RAM starts at `$0400` (a few hundred bytes).
  * **`$1000` up is free** for programs.
* **The RAM bank** at `$8000-$9FFF`: the one `$00` selects.  `0: 05` puts bank 5 there; `$F0-$FF` are shared banks
  (with `U` at `$FFF1`).
* **The paged ROM's bank** at `$A000-$DFFF`: the one `$01` selects, which is the monitor's own module, since it runs
  from there.  Changing `$01` pulls the monitor out from under itself; a program run with `R` may change it if it puts
  it back before it returns.
* **The BIOS ROM's page 0** at `$E000-$FFFF`, and the I/O at `$FF00-$FFEF`.  Examining a device's register reads it,
  with whatever that does (reading the ACIA's data register takes a byte).

`T`, `V` and `W` belong to the kernel: changing them from the monitor will stop the system.

---

## Writing a program with it

A program put in RAM with `:` and run with `R` is an ordinary subroutine in task 1: it can call any system call with
`jsr` (the addresses are in `base/sdk/asm/hydra.inc`, and in [the call reference](../09-programming/calls-reference.md)),
and ends with `RTS`.  For anything longer than a few bytes, write it with the assembler and load it as a program
([assembly](../09-programming/assembly.md)); on HydraOS, `as` assembles on the Hydra itself.

`L` shows any code as instructions, the kernel's and the ROM modules' too: `E000.E00F` with `L` on is the reset stub.

---

## Without a console driver

With no task F (a paged ROM without `ser`), the monitor uses the kernel's bring-up console: the serial port, polled.
Everything works but Ctrl-C (nothing catches it), which is useful when bringing up a new board or a new console driver.
