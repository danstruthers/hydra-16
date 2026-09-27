## **Hydra 16**

Project to create a multi-tasking 6502-based computer and basic operating system.

The code is built with the **cc65** suite (https://cc65.github.io/).  The board schematics and PCB layouts are done in **KiCAD 9.0** (https://www.kicad.org).

### **Building**

The OS ROM is built by `os_rom/makeC02.bat` (`ca65` + `ld65` with `os_rom/os_rom_C02.cfg`), which produces two ROM images:

| Image | Chip | Contents |
| :---- | :--- | :------- |
| `os_rom/tmp/os_rom_C02.bin` | BIOS/OS ROM (`$E000-$FFFF`, 16 8K pages selected by `W`) | BIOS, OS, WOZMON (page 0); HyForth and the disassembler (page 1) |
| `os_rom/tmp/paged_rom_C02.bin` | Paged ROM (`$A000-$DFFF`, 16K banks selected by `$01`) | `COPYTORAM` and the HyForth RAM image (copied to `$0800` at startup) |

Most changes affect **both** images (HyForth's RAM image calls ROM addresses directly), so burn both.  The paged ROM image is written in chip order: the hardware swaps the 8K halves of each 16K bank (A13), so CPU `$A000` reads ROM offset `$2000` and CPU `$C000` reads ROM offset `$0000`.  Burn it at offset 0.

`sim/hydrasim.js` is a Hydra-16 emulator (Node.js) that boots these images without the hardware; see `sim/README.md`.  The plan for the memory manager and IO subsystem is in `os_rom/MMU_PLAN.md`.

### **Startup**

1. **POST** (power-on self test): checks the memory mapping and the paged RAM's address, data and bank lines, and prints two lines (see **POST** below).
2. IRQ tables and vectors, tasks, MMU (including detection of the installed RAM modules), message rings, VIA.
3. The sound and serial drivers start in their own tasks, then the welcome message is printed.
4. The shell (HyForth, then WOZMON on `bye`) starts in task 1, which receives the serial input.  The scheduler's tick starts, and task 0 becomes the idle task.

The scheduler is preemptive: the VIA timer 1 interrupt (about every 5 ms) switches between runnable tasks, round-robin.  A task can hold the CPU with `NO_PREEMPT`/`PREEMPT` (interrupts keep running), or with `sei`/`cli` for very short sections.  `TASK_RUN` starts a task in the background, `TASK_START` starts one and waits for it, and `TASK_WAIT`/`IO_WAKE` block and wake a task.  `$F869` (`F869R` in WOZMON) runs the scheduler self test.

### **POST**

The power-on self test runs first thing at every reset, in task 0 with IRQs off, using polled serial output (no drivers), so it works even when little else does.  The code is `POST` in `os_rom/os_main.s` (first line) and `os_rom/post_ram.s` on BIOS page 2 (second line).  A good board prints:

```
POST ZP:T ST:T LO:T 7D:T SH:S P1:4C
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000 20:0/00/0000
```

**First line: the memory mapping** the task system depends on.

| Field | Good | Checks |
| :---- | :--- | :----- |
| `ZP:` | `T` | `$0080` (zero page) is per-Task (`T`), not Common to all tasks (`C`) |
| `ST:` | `T` | `$0180` (stack page) is per-task |
| `LO:` | `T` | `$0280` (task RAM) is per-task |
| `7D:` | `T` | `$7D80` (top of task RAM, the task system page) is per-task |
| `SH:` | `S` | Shared bank `$F0` (U = 0) is Shared between tasks (`S`), or not (`X`) |
| `P1:` | `4C` | The byte at `$EA00` on BIOS ROM page 1 (the `jmp` at `forth_main`): page 1 is present and current |

**Second line: the paged RAM lines.**  Each value is a hex mask of **bad** lines (bit n set = line n bad), so all zeros is good.

* `U:x`: the U register lines U0-U3, tested on shared bank `$F0`.
* `bb:x/dd/aaaa`: bank `bb`, tested at `$8000-$9FFF`:
  * `x`: bank register lines 0-3 (the byte at `$8000` of bank `bb` XOR 1, 2, 4 and 8)
  * `dd`: data lines D0-D7 (walking one at `$8000`)
  * `aaaa`: address lines A0-A12 (`$8000 + 2^n` for each line n; also catches a write landing on `$8000`, i.e. a line stuck high)

The banks tested are the first bank of each shared RAM chip (U = 0), then the first bank of each installed task RAM module (`00`, `10`, `20`, ...).  A missing chip shows up as bad lines.  The tests are destructive, which is fine at reset: nothing is kept in paged RAM yet.

A chip that fails is left unused: the MMU reserves every bank ID on a bad shared RAM chip (in all 16 U macro-pages) and treats a bad task RAM module as not installed.  (A shared chip's bank lines 2-3 select between chips, so only its lines 0-1 count against it.)  The system banks (IDs `$00-$09`) can't be moved, so a fault on the `F0`, `F4` or `F8` chip still needs fixing.

Shared RAM chips (`board/SharedMem.kicad_sch`, HM628512).  On the V1 board, bank register bits 2 and 3 are swapped (as are bits 6 and 7, and the same bits of the ROM bank register), so bank IDs `$04-$07` are on U28 and `$08-$0B` on U27:

| POST | Bank IDs | Chip |
| :--- | :------- | :--- |
| `F0` | `$F0-$F3` | U25 |
| `F4` | `$F4-$F7` | U28 |
| `F8` | `$F8-$FB` | U27 |
| `FC` | `$FC-$FF` | U29 |

HM628512 pins, for tracking down a bad line: A0 12, A1 11, A2 10, A3 9, A4 8, A5 7, A6 6, A7 5, A8 27, A9 26, A10 23, A11 25, A12 4; U0-U3 on A13-A16 (pins 28, 3, 31, 2); bank lines 0 and 1 (RAMB_M0/M1) on A17 and A18 (pins 30, 1); D0-D7 on pins 13-15 and 17-21.  For example, `F0:0/00/0001` is A0 (pin 12) on U25, and `F8:2/00/0000` is bank line 1 (A18, pin 1) on U27.

The emulator can inject a stuck address line to check the test (`--ram-fault`, see `sim/README.md`).

### **Tasks**

There are 16 tasks (`T` = `$0-$F`), each with its own `$0000-$7FFF` (zero page, stack and task RAM) and its own RAM banks.

| Task | Use |
| :--- | :-- |
| `$0` | System task: boot, then the idle task (runs only when no other task can) |
| `$1` | Shell (HyForth / WOZMON); the foreground task (it gets the console input) to start with |
| `$D` | Pipe server (Resident) |
| `$E` | Sound driver (Resident) |
| `$F` | Serial driver (Resident) |

Drivers run in **Resident** tasks, which only run from IRQs and from calls into the driver (`TASK_CALL`).  Tasks send each other data through **message rings** in shared RAM: one 256-byte ring per receiver/sender pair (`MSG_SEND_BYTE`, `MSG_RECV_BYTE`, `MSG_PEEK`).

### **IO**

All IO goes through **file descriptors**, Plan 9 style (see `os_rom/IO_PLAN.md`): a task opens a name (`IO_OPEN "/dev/cons"`), gets an fd, and reads and writes it (`IO_READ`, `IO_WRITE`, `IO_GETC`, `IO_PUTC`, `IO_CTL`, `IO_CLOSE`).  Devices are **file servers**: a driver registers its names (`DEV_REGISTER`), and each request runs its serve routine in the driver's task.  A read with no data yet makes the task wait (it doesn't use the CPU) until the driver wakes it.  The IO layer is on BIOS ROM page 2.

| Name | Server | |
| :--- | :----- | :- |
| `/dev/cons` | Serial driver (task `$F`) | The console: reads get the keyboard input, but only for the **foreground task** (the shell to start with; `IO_CTL` code `SER_CTL_FOREGROUND` changes it); other readers wait |
| `/dev/ser` | Serial driver | The serial port, for any task |
| `/dev/snd` | Sound driver (task `$E`) | Writes are YM2151 register/value byte pairs; `IO_CTL` codes `SND_CTL_INIT` and `SND_CTL_TEST` |
| `/dev/pipe` | Pipe server (task `$D`) | `IO_PIPE` makes a pipe (a read fd and a write fd, 255 bytes buffered); readers get end of file once the writers are gone |
| `/dev/null`, `/dev/zero` | IO layer | The usual |

Each task has 12 fds.  The shell opens fds 0, 1 and 2 (stdin, stdout, stderr) on `/dev/cons`, and tasks it starts get copies of its open fds; a task's fds are closed when it ends.  `READ_CHAR` (a key, if there is one) and `WRITE_CHAR` read fd 0 and write fd 1; tasks without them (the system task and drivers) use the serial port directly.  `GET_CHAR` waits for a key on fd 0, sleeping (the task uses no CPU until one comes in); WOZMON and HyForth wait for input with it.  `IO_DUP2` makes one fd refer to another's file, e.g. to redirect stdout.  The serial driver buffers both ways (256-byte RX and TX rings in its task), and sends from its transmit interrupt, so output doesn't busy-wait.  `$F88A` (`F88AR` in WOZMON) runs the IO self test.

HyForth has the IO words `open ( sz mode -- fd )` (e.g. `q^/dev/zero^ 1 open`; mode 1 = read, 2 = write, 3 = both), `close ( fd -- )`, `read ( fd addr n -- n' )`, `write ( fd addr n -- n' )`, `ioctl ( fd code arg -- )`, `fdup2 ( fd newfd -- )`, `pipe ( -- rfd wfd )` and `ioerr ( -- n )` (a failed call gives `!IO ERR!`; `ioerr` is the error code).

### **Memory Map**
* PER-TASK memory map (each task has its own copy of this memory space, except for shared RAM pages, as discussed below)

| Start | End  | Description |
| :---- | :--- | :---------- |
| $00 | | RAM Page selection register (Pages `$00-$EF` are task-specific.  Pages `$F0-$FF` are shared between all tasks, and are further indexed using the U register, below) |
| $01 | | ROM Page selection register |
| $02 | | OS/BIOS zero page, growing up from `$02` (`os_rom/zero.s`); the same layout in every task |
| | $FF | Task zero page, growing down from `$FF` (`TASK_ZP` macros); each task's code has its own, e.g. HyForth `$C8-$FF`, sound driver `$F8-$FF` |
| $0100 | $01FF | Hardware Stack |
| $0200 | $06FF | Buffers: HyForth input buffer (`$0200`), data and return stacks (`$0300-$03FF`), memory-manager stack (`$0400-$05FF`); WOZMON input buffer (`$0600`) |
| $0700 | $07FF | Unused |
| $0800 | $7CFF | Task RAM.  The shell task's HyForth dictionary grows up from `$0800`; the MMU allocates 256-byte pages top-down from `$7C00` |
| $7D00 | $7DFF | Task system page: IRQ registration tables (`$7D00-$7D8F`, the same in every task) and unclaimed-IRQ counters (`$7D90-$7D9F`) |
| $7E00 | $7FFF | MMU area: page and bank allocation maps, handle table |
| $8000 | $9FFF | Paged RAM (8K pages; task-specific and shared pages all show up here).  Task pages exist only for installed RAM modules (16 pages per module) |
| $A000 | $DFFF | Paged ROM (16K pages; ROMs are shared between all tasks, but the page selection is per-task, see `$01` above) |

* SHARED memory map (all tasks see the following areas the same)

| Start | End  | Description |
| :---- | :--- | :---------- |
| $E000 | $FFFF | BIOS/OS ROM paged area (indexed by the W register; see below).  Page 0: BIOS and OS.  Page 1: HyForth and the disassembler.  Page 2: the IO layer, the serial driver's file server, the POST RAM line tests and SPI.  Pages 3-F: unused |
| $E000 | $E004 | RESET Vector entry point: sets W to zero.  This is replicated at the beginning of each BIOS page, so that an arbitrary W register value at startup/RESET continues on page 0, right after the page 0 copy. |
| $E005 | $FCFF | Effective BIOS paged area.  Compiler segments (pages) `BIOS_P1 - BIOS_PF` correspond to `W` register values of `$01 - $0F`, respectively.  Code on different pages calls each other through far-call gates. |
| $F800 | $F895 | BIOS thunks (`jmp` table of BIOS, MMU, shared memory, scheduler and IO entry points), on page 0 and page 1.  `$F833` (`F833R` in WOZMON, `mmtest` in HyForth) runs the MMU self test; `$F869` the scheduler self test; `$F88A` the IO self test; `$F88D` is `GET_CHAR` (wait for a key), `$F890` `IO_DUP2`, `$F893` `IO_PIPE` |
| $FD00 | $FDFF | COMMON block, the same on every page: IRQ entry stubs and exit, NMI entry, far-call trampolines |
| $FE00 | $FEFF | "WOZMON" monitor page (page 0) |

#### **I/O Ports**

There are 15 shared I/O ports on the Hydra, with 16 1-byte registers per port, located from $FF00-$FFEF.  Some ports are taken by the on-board devices.  Others are reserved for specific add-on cards (ports 2 & 3 for video, for example).  Still others are assigned to card slots, usually to correspond with the assigned IRQ numbers, with two I/O ports per slot.  I/O port assignment currently matches IRQ assignment for devices.  Though this arrangement is not a requirement, it does make things easier to track if followed.

The area from $FFF0 to $FFFF (that would have been reserved for I/O port 15) is the System port, where pseudo-registers T-W ($FFF0-$FFF3) and the interrupt vector addresses ($FFFA-$FFFF) live.  There are 6 unused bytes ($FFF4-$FFF9) that are reserved for future System expansion.

| Start | End  | Description |
| :---- | :--- | :---------- |
|  | | **I/O Ports** `$00-$0E` |
| $FF00 | $FF0F | Onboard VIA (65C22) |
| $FF10 | $FF13 | Onboard ACIA (65C51) Serial |
| $FF14 | $FF1F | Unused |
| $FF20 | $FF3F | Reserved for future Video |
| $FF40 | $FF41 | Onboard YM2151 Sound generator |
| $FF42 | $FF4F | Unused |
| $FF50 | $FFEF | Unused (future I/O ports, expansion cards) |
|  | | **Pseudo-registers** |
| $FFF0 | | `T` Register (current task selector) |
| $FFF1 | | `U` Register (current shared memory macro-page) |
| $FFF2 | | `V` Register (interrupt vector selector) |
| $FFF3 | | `W` Register (BIOS page selection register) |
| $FFF4 | $FFF9 | Unused (future pseudo-register expansion) |
| | | **Vectors** (replicated on each BIOS page) |
| $FFFA | $FFFB | NMI Interrupt handler vector (the COMMON block's NMI entry) |
| $FFFC | $FFFD | Reset Vector (Set to `$E000`) |
| $FFFE | $FFFF | Interrupt Vector (see below) |


### **Interrupts**

Interrupt priority is lowest number == highest priority, so the S/W interrupt vector (#15) is the lowest priority.  
The interrupt vector (`$FFFE & $FFFF`) is actually a 16-entry pseudo-register indexed by either a) `V` register bits 0-3 if no hardware interrupt is active OR when a `BRK` instruction is executed, or b) the lowest numbered active interrupt request line (via the IRQ priority decoder circuit) if one or more H/W IRQs is active.  It is also indexed on write by `V` register bits 0-3, which is how the interrupt vectors are set.  
**_All_** interrupts can be called via the S/W interrupt mechanism by setting `V` to the IRQ #, and then calling `BRK`.  Just remember that `V` is a shared, pseudo-register, so should be saved and restored by each task whenever used.

#### **IRQ dispatch**

At startup, `IRQ_INIT` points all 16 vectors at the IRQ entry stubs in the COMMON block, which switch to BIOS page 0 and run the IRQ dispatcher (`os_rom/irq.s`).  Drivers don't set vectors themselves; they register handlers:

* `IRQ_REGISTER` (`.X` = `IRQ_NUMBER(n)`, `.A.Y` = handler) adds a handler for hardware IRQ `n`, up to 2 per IRQ.  The handler runs **in the task that registered it** (its zero page, stack and RAM), whichever task was interrupted.
* A handler returns with `rts` and C = 1 if it claimed the interrupt (C = 0 passes it to the next handler).  It must not re-enable interrupts.
* An IRQ that none of its own handlers claims is counted in the unclaimed-IRQ counters (`$7D90-$7D9F`, by IRQ #) and offered to every registered handler, so it still gets cleared.
* `SWI_REGISTER` (`.X` = S/W interrupt number `$0-$F`, `.A.Y` = handler) registers a S/W interrupt handler; `IRQ_UNREGISTER` / `SWI_UNREGISTER` remove handlers.

| IRQ # | Description |
| ---: | :--- |
| 0 | On-board VIA |
| 1 | On-board ACIA (Serial) |
| 2 | Card Slot 0 (low) |
| 3 | Card Slot 0 (high) |
| 4 | On-board Sound (YM 2151) |
| 5 | Card Slot 1 (low) |
| 6 | Card Slot 2 (low) |
| 7 | Card Slot 3 (low) |
| 8 | Card Slot 4 (low) |
| 9 | Card Slot 5 (low) |
| 10 | Card Slot 1 (high) |
| 11 | Card Slot 2 (high) |
| 12 | Card Slot 3 (high) |
| 13 | Card Slot 4 (high) |
| 14 | Card Slot 5 (high) |
| 15 | S/W interrupt (Set Register `V[0..3]` = `IRQ_NUMBER(15)`, Set `V[4..7]` = S/W Interrupt number)\* |

To reference an IRQ, use the `IRQ_NUMBER(num)` macro, i.e.: `lda     #IRQ_NUMBER(0)`

or reference the defines for each IRQ, i.e. `IRQ_NUMBER_ONBOARD_VIA`.

This will ensure that the proper IRQ mapping occurs (see Errata for more information).

\* Call `jsr SW_INT` after loading S/W interrupt number ($0-F) into A.  `SW_INT` saves and restores `V`.

Have fun!
