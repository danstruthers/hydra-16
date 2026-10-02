## **WOZMON, the disassembler, POST and the self tests**

The machine-level tools: WOZMON (a monitor, after Steve Wozniak's Apple 1 monitor) with a built-in disassembler, the power-on self test, the ROM self tests, and the hardware test.  Sources: `os_rom/monitor/`, `os_rom/tests/`, `os_rom/hwtest/`.

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

What WOZMON shows is the task's own view: `0000-7FFF` is task 1's RAM, `8000-9FFF` the bank in the prompt, `A000-DFFF` the paged ROM bank, and `E000-FFFF` BIOS ROM page 0 (the kernel's: WOZMON itself runs on page 4, and reads ROM as the disassembler does, from page 0 unless that was set to another).  To look at another bank, store it in `0000` (RAM bank) or `0001` (ROM bank).

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
* The system's shared banks (IDs `$00` and `$09-$0C`) can't move elsewhere, so a fault on the `F0` or `F8` chip still needs fixing.

To find the chip and pin behind a report, see the [Hardware Reference](../hardware.md#the-paged-ram-window): the shared bank IDs per chip (with V1's crossed bits 2/3: `F4` is U28, `F8` is U27), and the HM628512 pinout.  For example:
* `F0:0/00/0001` is A0 (pin 12) on U25;
* `F8:2/00/0000` is bank line 1 (A18, pin 1) on U27.

The emulator can inject a stuck address line to see the report (`--ram-fault`, [emulator](../tools/emulator.md)).  For a closer look at a fault, run the [hardware test](#the-hardware-test).

### **The hardware test**

A test of as much of the board as software can reach, for bringing up a board or chasing a fault.  It's a program of its own in paged ROM bank 1 (`os_rom/hwtest/`).  It takes the machine over: interrupts off, its own polled serial I/O at 9600 8N1, and no calls into the OS.  Its memory tests overwrite everything, so it ends with a reset.

**Starting it:**
* From HyForth: `hwtest`.
* From POST: type `T` just after a reset, before POST's second line appears.  This works when the OS can't start, for example with a bad shared RAM chip or an IRQ line held active.

**The menu.**  Type a test's key to run it, or:

| Key | Does |
| :-- | :--- |
| `A` | All the tests, quick: every bank's marks, and every byte of a sample of the memory (about 15 seconds) |
| `F` | All the tests, full: every byte of every bank, in every task (about 3 minutes at 3.58 MHz with three RAM modules) |
| `L` | All the tests, quick, again and again until a key: a count of the passes and failures after each (for an intermittent fault) |
| `R` | Reset: the machine starts again, through POST |

Each test prints its name, what it found, then `ok` or `FAIL` and the fault.  A run ends with `hwtest: all passed` or `hwtest: failed: N`:

```
CPU ................ ok
RAM modules ........ 0 1 2 ok
shared RAM ......... FAIL bank 04 8000 bits 08
CPU clock .......... 3.58 MHz ok
SPI devices ........ SD cards 0 ok
hwtest: failed: 1
```

**The tests,** in the order `A` runs them.  The memory tests write a pattern that depends on the address, so a stuck or crossed address line shows up as well as a bad data line.  Bits are hex masks of the bad bits.

| Key | Test | Checks | A fault shows |
| :-- | :--- | :----- | :------------ |
| `1` | CPU | A sample of the W65C02S's instructions and flags: binary and decimal arithmetic, and the 65C02's own (`STZ`, `TSB`/`TRB`, `BRA`, `PHX`, `RMB`/`SMB`, `BBR`/`BBS`, `(zp)`) | `check N` |
| `2` | T U V W registers | Each stores and reads back all 8 bits | the register and its bad bits |
| `3` | shared RAM | Every one of the 256 banks (`U` 0-F, IDs `$F0-$FF`) has its own mark; every byte of the first bank of each chip (full: of every bank) | Address faults: each chip and the lines it doesn't see, with pins, e.g. `U29: A18 (pin 1)` (from the marks of all 256 banks: the bits a bank's ID and the mark it holds differ by).  The same A13-A16 line on all four chips is the `U` register's line itself (test 2).  `no mark`: a data fault or no chip.  Otherwise a byte fault: `bank UB` (U, then the bank), where, the bad bits |
| `4` | RAM bank registers | Each task's `$00` (74LS219s IC1 and IC2): task 0 marks the 16 shared banks; then each task set to each bank and read at once; then all 16 tasks set to their own banks first and read afterwards; then lines 4-7 | `at once: task F: D>F` (set to $FD, it read bank $FF's mark: a cell that doesn't hold its value); `all set, then read: task F bank FF read A8 (bank FD's mark, bits 02)` (right at once but wrong later: writing one register disturbs another).  `skipping banks FE FF (...)` (information, not a failure): task 0 alone reads those banks wrong, so the shared RAM is at fault (test 3, e.g. a bad solder joint on U29, which holds banks `$FC-$FF`); the registers are tested with the other banks.  `(task 0 too)` after a failure: task 0 read that bank wrong as well, so it's the RAM again (an intermittent fault) |
| `5` | task RAM | First, each task's RAM is its own: a mark in each (a bad task line to U7, A15-A18, makes two tasks share their RAM, which the patterns can't show).  Then every task's zero page (from `$02`), stack page and `$0200-$7FFF` (task F's to `$7FF7`: a DS1747's clock registers may be there) | the task, where, the bad bits; or `task F has task B's mark: U7 A17 (pin 30)` |
| `6` | RAM modules | Which modules there are (shown); each bank of each has its mark in each task; every byte of each module's first bank (full: of every bank, in every task) | the bank, the task, where, the bad bits |
| `7` | BIOS ROM | A CRC of each 8K page against the checksums the build stores | `page N`, and `(W line n)` if it reads as another page |
| `8` | paged ROM | A CRC of each 16K bank, and the bank lines | `bank NN`, or `bank line n` |
| `I` | interrupts | No IRQ line active with the devices quiet; the vector RAM (all 16 entries, two patterns); `BRK` through entry `V`; the VIA's, the ACIA's and the YM2151's interrupts on their lines, one at a time, then all at once in priority order | `an IRQ line is held active`; `vector entry N wrote .. read ..`; the device, and `no IRQ` or the line it came on; `unasked: IRQ on line N` (one came before a device was asked).  With line 4, the YM2151's status too (bit 0: timer A's flag, 1: timer B's) |
| `V` | VIA | Timer 1's latches, the shift register and IER read back; timer 1 counting, its flag (one-shot and free-running); timer 2's; the shift register's | the part |
| `Y` | sound chip (YM2151) | Its busy flag (set by a write, then clear), and its timers' flags | `always busy (no chip?)`, `never busy`, `timer A`, ... |
| `K` | CPU clock | The CPU's clock against the YM2151's (`SND_CLK`, 3.58 MHz): shows 0.89, 1.79, 3.58 or 7.16 MHz (jumpers J6-J7), which must be what the ROM is built for (`CPU_CLOCK_MULT`).  It shows its counts: `first`, from starting YM2151 timer A to its first overflow (judged: 16,384 at 3.58 MHz), and `next`, to the one after with the timer's flag reset by writing `$14` again (shown only: larger by the write's time if that restarts the timer) | the clock and what the ROM expects, or `not a clock the board has` and the counts |
| `S` | serial port (ACIA) | DCD and DSR active; the control and command registers; a programmed reset; a character's time at 9600 baud (Rockwell only) | the register, or the character's time in cycles |
| `P` | SPI devices | Information: each of devices 0-7 is sent an SD card's reset (CMD0); the SD cards that answer are shown | (another answer is shown as `(device: answer)`) |
| `C` | I2C bus | SCL and SDA high when released and each low alone when pulled low (a line shorted, held low or stuck high); then information: the devices that answer, at `$08-$77` | the lines' levels |
| `X` | slot cards | Information: the slot ports (`1A` = slot 1, select A) that don't read as empty | |
| `H` | hold a task (probe) | Not a test, and not in `A`, `F` or `L`: every task's RAM bank register set to `$F0` + the task, then the task typed (0-F) held in `T`, its bank at `$8000` read over and over, until a key.  Meanwhile, measure T0-T3 (IC1-IC4 pins 1, 15, 14, 13) and the bank register's outputs, `RAMB0-7` (IC1 and IC2 pins 5, 7, 9, 11) | `not a task` |

The ROM tests' checksums are made by the build (`os_rom/tools/romsum.js`, run by `build.js`) and kept at the end of paged ROM bank 1, so burn both images from the same build.  The CPU clock and serial tests time things with VIA timer 1, so a bad VIA shows up there too; run the tests in order when chasing a fault.  The emulator can inject faults to see the reports: `--ram-fault`, `--stuck-irq`, `--acia-line`, `--clock` ([emulator](../tools/emulator.md)).
