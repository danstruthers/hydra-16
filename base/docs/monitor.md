## **The monitor: wozmon**

The base's task 1 (its init) is Steve Wozniak's Apple 1 monitor, as the Hydra-16's V1.8C software had it: a way to look
at memory, change it, and run what's there, from the serial console.  It is a program on the kernel's calls
(`../modules/wozmon/wozmon.s`), not code in the BIOS ROM, so it sees what any task sees, and a program it runs can call
the kernel.

### **The prompt and the commands**

```
T1 00>
```

The prompt is the task (`T1`), the RAM bank at `$8000` (`00`; a shared bank, `$F0` up, shows its `U` too: `T1 F2(3)`)
and `>`.  A line holds any number of items, as Woz's did; hex digits in upper or lower case:

| Item | What it does |
| :--- | :----------- |
| `XXXX` | Examine: the byte at `XXXX`, on a line of its own (`XXXX: BB`) |
| `XXXX.YYYY` | A block: `XXXX` to `YYYY`, 8 bytes a line.  `.YYYY` alone goes on from where the last stopped |
| `XXXX: BB BB ...` | Store: the bytes from `XXXX` on (the examine shows `XXXX`'s old byte first).  `: BB` alone goes on after the last stored |
| `XXXXR` | Run: a `JSR` to `XXXX` (`R` alone: the last address shown).  The program's `RTS` comes back to the prompt |
| `L` | From now on, an examine shows instructions: a line each, its bytes and its text as the assembler `as` writes it (`lda #$41`) |
| `K` | Back to bytes |

Backspace (or Delete) takes back a character; Escape gives up the line (Woz's `\`).  **Ctrl-C** stops whatever is
running, a program started with `R` too, and brings back the prompt; so does any other note (a `BRK` in a program is
the note `NOTE_BRK`).

```
T1 00> 1000: A9 48 20 50 F9 60
1000: 00
T1 00> L 1000.1005
1000: A9 48     lda #$48
1002: 20 50 F9  jsr $F950
1005: 60        rts
T1 00> 1000R
1000: A9H
T1 00>
```

(`$F950` is `PUTC`: the program prints `H`.  The calls' addresses are in `hydra.inc`, made by the build in
`../obj/sdk/hydra.inc`.)

### **What it sees**

Task 1's view of the machine, as any task's:

* **RAM**, `$0000-$7FFF`: the task's own.  The monitor's zero page is `$22-$7F`, the kernel's call registers `r0`-`r15` are
  `$02-$21`; the task's OS areas, `$80-$FF` and `$0200-$03FF`, are the kernel's (don't write them); the stack is
  `$0100-$01FF`, the monitor's RAM from `$0400` (a few hundred bytes).  **`$1000` up is free** for programs.
* **The RAM bank** at `$8000-$9FFF`: the one `$00` selects (`00` to the installed RAM's top; `$F0`-`$FF` shared).
* **The paged ROM's bank** at `$A000-$DFFF`: the one `$01` selects, which is the monitor's own module, as it runs from
  there.  Changing `$01` pulls the monitor out from under itself; a program run with `R` may change it, if it puts it
  back before it returns.
* **The BIOS ROM's page** at `$E000-$FFFF`, and the I/O at `$FF00-$FFEF` (examining a device's register reads it, with
  whatever that does).

### **Without a console driver**

With no task F (a paged ROM without `ser`), the monitor uses the kernel's bring-up console: the serial port, polled.
Everything works but Ctrl-C (nothing catches it), which is useful when bringing up a new board.
