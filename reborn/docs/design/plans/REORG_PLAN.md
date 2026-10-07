## **OS ROM reorganisation plan**

A plan for tidying the OS ROM: less dead code, clearer ROM page roles, a cleaner source tree and build output.  Steps 1-7 are done.  (Sizes as of the storage work: page 0 has about 320 bytes free before the thunks, page 1 about 2.3K in gaps and at the end, page 2 about 1.2K, page 3 about 6.9K, pages 4-F nothing used.)

### **1. Remove dead code** (done)
*Done: the message rings (`msg.s`; the system-bank macros are `_M_SYS_ENTER` / `_M_SYS_LEAVE` and `SYS_BANK` in `defines.s` now, and shared bank IDs `$01-$08` are free for `SH_ALLOC`), `math.s` and `WRITE_DEC` / `WRITE_BYTE_MIN`, the unused VIA helpers, `COPYTORAM`'s dots and address printing, `DO_WELCOME`'s vector dump (and the shell's second clear screen, so boot messages stay), the I2C block, the old WDC ACIA alternatives (the WDC 65C51 is back as a build option for the new serial driver: `SER_ACIA`, TX paced by VIA timer 2), `ALTBUF` and HyForth's stale address comments.  Page 0: about 560 bytes back before the thunks and 260 in `BIOS`; the OS ZP is 10 bytes smaller.*

* **Message rings** (`msg.s`, 422 bytes of page 0): nothing sends or receives since pipes and `/dev/cons` replaced them; only `MSG_INIT` and `MSG_RESET_TASK` are still called.  Also frees shared bank IDs `$01-$08` (64K of shared RAM), `$8000-$81FF` of bank `$00` and 3 ZP bytes.  Keep the bank 0 map macro (renamed).
* **`math.s`** (about 300 bytes): only `WRITE_DEC` calls it, and nothing calls `WRITE_DEC`.  `ZP_MATH_TEMP` goes too.
* **Unused VIA helpers**: the T2 routines, `VIA_IS_*`.
* **Boot noise**: `COPYTORAM`'s progress dots (thousands of characters: about 2 seconds at every boot and shell restart), and `DO_WELCOME`'s IRQ vector dump.
* **Source clutter**: the I2C `.if 0` block in `bios.s`; the WDC ACIA / software serial timing alternatives in `defines.s`; stale hardcoded ROM addresses in `hyforth.s` comments.

### **2. Page 1's gates** (done)
* *Done: the 36 old 15-byte `FAR_GATE`s in `page1.s` are `FAR_GATE_INLINE`s (6 bytes), and the `FAR_GATE` macro is gone: `GATES_P1` went from 776 to 380 bytes.*

### **3. Page roles and fixed offsets** (done)
*Done: the self tests are on page 4 (`.scope PAGE4`, gates in `page4.s`); page 1 has no fixed offsets (about 2.4K free in one piece) and page 0's `BIOS` follows the thunks; POST checks `PAGE1::forth_main`.  `tools/check_pages.js` lists calls to another page that miss a gate (it can't see pointers to ROM data handed across pages: the IO test copies its path names to RAM for that).*

| Page | Role |
| :--- | :--- |
| 0 | Kernel: reset and POST core, IRQ dispatch, scheduler, `TASK_CALL`, MMU and shared memory cores, IRQ handlers, quick-switch code, gates, thunks, COMMON, WOZMON |
| 1 | HyForth and the disassembler |
| 2 | IO: the IO layer, namespaces, the file servers (serial, sound, pipes, null, zero) |
| 3 | Storage: SPI, the SD card, `/dev/sd`, the HydraFS server (to come: `HYDRAFS.md`) |
| 4 | Diagnostics: POST RAM tests, `MMU_TEST`, `SCHED_TEST`, `IO_TEST` (from pages 1 and 2: about 1.5K) |
| 5+ | Later subsystems |
| Paged ROM | Read-only data: HyForth training scripts, bload libraries, sound patches and tunes, help text, the disassembler's opcode tables |

* Only the thunk table (`$F800`), COMMON (`$FD00`), WOZMON (`$FE00`) and the vectors need fixed addresses.  Remove page 1's `DISASM` / `DISASM_CODE` / `FORTH_ROM` offsets (about 500 bytes of gaps) and page 0's `BIOS` offset (it splits page 0's free space in two).  POST's `P1:4C` check reads a fixed `$EA00`: use the symbol.

### **4. Source tree and build output** (done)
*Done: `bin/` and `obj/`; folders `include/`, `kernel/`, `io/`, `drivers/`, `tests/`, `monitor/`, `tools/`; `bios.s` split into `kernel/print.s`, `drivers/serial.s`, `drivers/via.s`, `kernel/vectors.s`; `defines.s` into `include/hw.inc`, `kernel.inc`, `io.inc`, `ascii.inc`, `macros.inc`; `sound.s` has its own segment; the shell is in `os_main.s`.  The old 6502 build (`make.bat`, `os_rom.cfg`) is gone.  (`fs/` comes with the filesystem server.)  Later (the code review, [CODE_REVIEW.md](CODE_REVIEW.md)): `io/` is the IO layer only, the devices' servers are in `servers/`, HydraFS is in `fs/`, and the clock chip's driver in `drivers/rtc.s`.*

* **Build output**: the ROM images (`os_rom_C02.bin`, `paged_rom_C02.bin`) go in **`os_rom/bin/`** (in source control); everything else the build makes (`.o`, listing, labels, map) in **`os_rom/obj/`** (not in source control: `.gitignore`).  **`os_rom/tmp/` goes** (removed from source control).  `makeC02.bat` and the linker config name the new places; the sim's default `--rom` follows.
* **Source folders**, by role, for example:
  * `kernel/`: boot and POST, tasks and scheduler, IRQs, MMU, shared memory, thunks, gates, COMMON
  * `io/`: the IO layer, namespaces, the file servers
  * `drivers/`: serial, sound, VIA, SPI, SD card
  * `fs/`: the HydraFS server
  * `tests/`: POST RAM, MMU, scheduler and IO tests
  * `monitor/`: WOZMON, the disassembler
  * `hyforth/` (as now)
  * `include/`: the split `defines.s` (below), `zero.s`
* **Split big files**: `bios.s` (print routines, serial driver, VIA, dead I2C) into `print.s`, `serial.s`, `via.s`; `defines.s` (1,000 lines) into `hw.inc` (VIA, ACIA, YM registers, IO ports), `kernel.inc` (tasks, IRQs, MMU, errors), `io.inc` (IO, H9P, namespaces, servers), `macros.inc`.  Give `sound.s` its own segment (it has none and lands in `SHELL`); merge the 8-line `shell.s` into `os_main.s`.

### **5. Shared helpers** (done)
*Done: `_M_BANK_ENTER` / `_M_BANK_LEAVE` (`_M_SYS_ENTER` and `_M_IO_MAP_XFER` are one-line wrappers), `IO_FD_ENTRY` (fd * 8) and `IO_SRV_COUNT`.  Not done: the quick switches (2-3 instructions each, in different registers: a macro saves nothing), and the bank flips in `IO_DEV_FIND` and `IO_SRV_MAP` (not enter / leave pairs).*

* One pair of bank mapping macros for the four hand-written variants (`_M_SYS_ENTER`, `_M_IO_MAP_XFER`, and inline in `NS_PUT_DEV`, `IO_DEV_FIND`, `IO_SRV_MAP`).
* Macros for the "peek / poke another task's ZP" quick switch (about 8 copies).
* An `fd * 8` helper in `io.s` (about 10 copies), and an `IO_SRV_COUNT` helper for the servers' "map, set the count, unmap".

### **6. RAM** (done)
*Done: the pipe rings come from the pipe task's MMU (`PIPE_BUF_PAGE`); they used to overlap the MMU's pages from `$0800`.  Kept: the serial rings (`$0200-$03FF`, below the MMU's pages; the IRQ handler's absolute addressing is the fast path), `MMU_PAGE_BOTTOM` for all tasks, and `ZP_D_*` in the OS ZP (the disassembler runs in HyForth's task, so it would need its own fixed task ZP anyway).*

* The resident tasks' fixed buffers (serial rings `$0200-$03FF`, the pipe table and rings `$0200-$0AFF`) come from the MMU instead (the pipe task's reach above the MMU's bottom page).  `MMU_PAGE_BOTTOM` (`$08`: HyForth's buffers) could be per task.
* Move single-purpose OS ZP (the disassembler's `ZP_D_*`) to task ZP.

### **7. HyForth's core from ROM** (done)
*Done: HyForth's interpreter and built-in words (headers and code) run from ROM page 1; only its variables (segment `FORTH_DATA`, about 90 bytes) are copied to RAM at `$0800`, and the dictionary starts at `$0B00`.  To make room, the code of the bulkier words (the shell's and IO words, tasks, sound, memory records, the stack printers, multiply and divide ...), the error messages, `MALLOC` and the disassembler moved to page A: those words are far words (`def_far`: `FARWORD` runs their code there; see [rom-layout.md](../../../../old/docs/programming/rom-layout.md#calling-across-rom-pages)).  The debug dump code sits after page 1's thunks.  `SYSCALL` jumps through `TEMP1` instead of modifying itself.  A pipeline's line takes about 37% fewer cycles (`TASK_CLONE` copies about 7K less), and the shell starts sooner.*

* Running HyForth's core from ROM page 1 instead of a 6K RAM copy frees shell RAM and roughly halves `TASK_CLONE`'s time.  Needs a RAM trampoline for `SYSCALL` (self-modifying) and changes to the dictionary layout: a project of its own.
