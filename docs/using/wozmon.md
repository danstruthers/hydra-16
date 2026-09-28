## **WOZMON, the disassembler, POST and the self tests**

The machine-level tools: WOZMON (a monitor, after Steve Wozniak's Apple 1 monitor) with a built-in disassembler, the power-on self test, and the ROM self tests.  Sources: `os_rom/monitor/`, `os_rom/tests/`.

### **Getting to WOZMON**

Type `bye` at HyForth's prompt.  WOZMON runs in the shell task (task 1), so it has the console's fds.  To go back to HyForth, press **Ctrl-\\**: the shell starts again from scratch.

The prompt shows the task, the RAM bank at `$8000` (with the shared macro-page `U` in brackets for a shared bank) and the paged ROM bank:

```
T1 00:00>          task 1, RAM bank $00, ROM bank $00
T1 F0(0):00>       shared bank $F0, U = 0
```

### **Commands**

A line holds one or more items.  Addresses and data are hex; leading zeros can be left out.

| Type | Does |
| :--- | :--- |
| `E000` | Show the byte at `E000` |
| `E000.E00F` | Show `E000` to `E00F` (8 bytes a line) |
| `.E0FF` | Show from the last address shown to `E0FF` |
| `1000: 41 42` | Store `41`, `42` at `1000`, `1001` (it shows the old byte at `1000`) |
| `: 43` | Store at the next address |
| `1000R` | Run the code at `1000` (a `jsr`: `rts` comes back to WOZMON) |
| `1000S` | Start the code at `1000` in a new task and wait for it to end (`TASK_START`) |
| `L` | List mode: show addresses as disassembled instructions from now on |
| `K` | Back to byte mode |
| `T`, `U`, `V`, `W` | Stand for the pseudo-register addresses `FFF0-FFF3`: `T` shows `T`, `W: 1` stores 1 in `W` (careful: that switches the ROM under WOZMON's feet) |
| Backspace, Esc | Erase a character; cancel the line |

**List mode** disassembles as it shows:

```
T1 00:00>L E000.E008
E000: A9 00    LDA  #$00
E002: 8D F3 FF STA  $FFF3
E005: D8       CLD
```

What WOZMON shows is the task's own view: `0000-7FFF` is task 1's RAM, `8000-9FFF` the bank in the prompt, `A000-DFFF` the paged ROM bank, and `E000-FFFF` BIOS ROM page 0.  To look at another bank, store it in `0000` (RAM bank) or `0001` (ROM bank).

The disassembler is also a call: `DISASM_AY` (`$F81B`) disassembles at `.A.Y` (C = 0: one instruction; C = 1: `.X` of them).  HyForth has it as `disasm ( addr n -- )`.

### **Self tests**

The ROM has three self tests, runnable from WOZMON (or HyForth's `syscall`).  Each prints its name, then `ok`, or `FAIL` with a step letter and a value to look up in its source.

| Run | Test | Checks |
| :-- | :--- | :----- |
| `F833R` | MMU (`tests/mmu_test.s`; HyForth: `mmtest`) | Small, chunk, page and bank allocations; reads, writes, locks; far pointers and references; the maps come back clean |
| `F869R` | Scheduler (`tests/sched_test.s`) | Three tasks interleaving; a `NO_PREEMPT` section staying together; a wait and wake |
| `F88AR` | IO (`tests/io_test.s`) | Opening, reading and writing devices; pipes between tasks; `IO_DUP2`; namespaces; names in ROM |

The scheduler test prints the tasks' letters as they run, e.g.:

```
Sched test:
mbbaambammbambbaambammbambbaambammba
m[cccccccccc]mmmmmmmmmmm
wW
done
```

### **POST: the power-on self test**

POST runs first at every reset, before anything else: in task 0, with interrupts off and polled serial output.  So it works even when little else does.  A good board prints:

```
POST ZP:T ST:T LO:T 7D:T SH:S P1:4C
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000 20:0/00/0000
```

**The first line: the memory mapping** the task system depends on.

| Field | Good | Checks |
| :---- | :--- | :----- |
| `ZP:` | `T` | `$0080` (zero page) is per-Task (`T`), not Common to all tasks (`C`) |
| `ST:` | `T` | `$0180` (stack page) is per task |
| `LO:` | `T` | `$0280` (task RAM) is per task |
| `7D:` | `T` | `$7D80` (the task system page) is per task |
| `SH:` | `S` | Shared bank `$F0` (U = 0) is Shared between tasks (`S`), or not (`X`) |
| `P1:` | `4C` | The byte at `forth_main` on BIOS page 1 (a `jmp`): page 1 is there and current |

**The second line: the paged RAM's lines.**  Each value is a hex mask of **bad** lines (bit n set = line n bad), so all zeros is good.

* `U:x`: the `U` register's lines U0-U3, tested on shared bank `$F0`.
* `bb:x/dd/aaaa`: bank `bb`, tested at `$8000-$9FFF`:
  * `x`: bank register lines 0-3 (writes to bank `bb` XOR 1, 2, 4 and 8 mustn't land in `bb`)
  * `dd`: data lines D0-D7 (a walking one)
  * `aaaa`: address lines A0-A12 (`$8000 + 2^n` for each line n; also catches a write landing on `$8000`)

The banks tested are the first bank of each shared RAM chip (`F0`, `F4`, `F8`, `FC`, with U = 0), then the first bank of each installed task RAM module (`00`, `10`, `20`, ...).  A missing chip shows as bad lines.  The tests are destructive, which is fine at reset.

**A chip that fails is left unused:**
* A bad shared RAM chip has every bank ID on it reserved, in all 16 macro-pages.
* A bad task RAM module is treated as not installed.
* The system's shared banks (IDs `$00` and `$09`) can't move elsewhere, so a fault on the `F0` or `F8` chip still needs fixing.

To find the chip and pin behind a report, see the [Hardware Reference](../hardware.md#the-paged-ram-window): the shared bank IDs per chip (with V1's crossed bits 2/3: `F4` is U28, `F8` is U27), and the HM628512 pinout.  For example:
* `F0:0/00/0001` is A0 (pin 12) on U25;
* `F8:2/00/0000` is bank line 1 (A18, pin 1) on U27.

The emulator can inject a stuck address line to see the report (`--ram-fault`, [emulator](../tools/emulator.md)).
