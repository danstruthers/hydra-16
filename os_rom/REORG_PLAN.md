## **OS ROM reorganisation plan**

A plan for tidying the OS ROM: less dead code, clearer ROM page roles, a cleaner source tree and build output.  Not started yet; the order below is the agreed one.  (Sizes as of the storage work: page 0 has about 320 bytes free before the thunks, page 1 about 2.3K in gaps and at the end, page 2 about 1.2K, page 3 about 6.9K, pages 4-F nothing used.)

### **1. Remove dead code** (done)
*Done: the message rings (`msg.s`; the system-bank macros are `_M_SYS_ENTER` / `_M_SYS_LEAVE` and `SYS_BANK` in `defines.s` now, and shared bank IDs `$01-$08` are free for `SH_ALLOC`), `math.s` and `WRITE_DEC` / `WRITE_BYTE_MIN`, the unused VIA helpers, `COPYTORAM`'s dots and address printing, `DO_WELCOME`'s vector dump (and the shell's second clear screen, so boot messages stay), the I2C block, the old WDC ACIA alternatives (the WDC 65C51 is back as a build option for the new serial driver: `SER_ACIA`, TX paced by VIA timer 2), `ALTBUF` and HyForth's stale address comments.  Page 0: about 560 bytes back before the thunks and 260 in `BIOS`; the OS ZP is 10 bytes smaller.*

* **Message rings** (`msg.s`, 422 bytes of page 0): nothing sends or receives since pipes and `/dev/cons` replaced them; only `MSG_INIT` and `MSG_RESET_TASK` are still called.  Also frees shared bank IDs `$01-$08` (64K of shared RAM), `$8000-$81FF` of bank `$00` and 3 ZP bytes.  Keep the bank 0 map macro (renamed).
* **`math.s`** (about 300 bytes): only `WRITE_DEC` calls it, and nothing calls `WRITE_DEC`.  `ZP_MATH_TEMP` goes too.
* **Unused VIA helpers**: the T2 routines, `VIA_IS_*`.
* **Boot noise**: `COPYTORAM`'s progress dots (thousands of characters: about 2 seconds at every boot and shell restart), and `DO_WELCOME`'s IRQ vector dump.
* **Source clutter**: the I2C `.if 0` block in `bios.s`; the WDC ACIA / software serial timing alternatives in `defines.s`; stale hardcoded ROM addresses in `hyforth.s` comments.

### **2. Page 1's gates** (done)
* *Done: the 36 old 15-byte `FAR_GATE`s in `page1.s` are `FAR_GATE_INLINE`s (6 bytes), and the `FAR_GATE` macro is gone: `GATES_P1` went from 776 to 380 bytes.*

### **3. Page roles and fixed offsets**
| Page | Role |
| :--- | :--- |
| 0 | Kernel: reset and POST core, IRQ dispatch, scheduler, `TASK_CALL`, MMU and shared memory cores, IRQ handlers, quick-switch code, gates, thunks, COMMON, WOZMON |
| 1 | HyForth and the disassembler |
| 2 | IO: the IO layer, namespaces, the file servers (serial, sound, pipes, null, zero) |
| 3 | Storage: SPI, the SD card, `/dev/sd`, the FAT32 server (being written) |
| 4 | Diagnostics: POST RAM tests, `MMU_TEST`, `SCHED_TEST`, `IO_TEST` (from pages 1 and 2: about 1.5K) |
| 5+ | Later subsystems |
| Paged ROM | Read-only data: HyForth training scripts, bload libraries, sound patches and tunes, help text, the disassembler's opcode tables |

* Only the thunk table (`$F800`), COMMON (`$FD00`), WOZMON (`$FE00`) and the vectors need fixed addresses.  Remove page 1's `DISASM` / `DISASM_CODE` / `FORTH_ROM` offsets (about 500 bytes of gaps) and page 0's `BIOS` offset (it splits page 0's free space in two).  POST's `P1:4C` check reads a fixed `$EA00`: use the symbol.

### **4. Source tree and build output**
* **Build output**: the ROM images (`os_rom_C02.bin`, `paged_rom_C02.bin`) go in **`os_rom/bin/`** (in source control); everything else the build makes (`.o`, listing, labels, map) in **`os_rom/obj/`** (not in source control: `.gitignore`).  **`os_rom/tmp/` goes** (removed from source control).  `makeC02.bat` and the linker config name the new places; the sim's default `--rom` follows.
* **Source folders**, by role, for example:
  * `kernel/`: boot and POST, tasks and scheduler, IRQs, MMU, shared memory, thunks, gates, COMMON
  * `io/`: the IO layer, namespaces, the file servers
  * `drivers/`: serial, sound, VIA, SPI, SD card
  * `fs/`: the FAT32 server
  * `tests/`: POST RAM, MMU, scheduler and IO tests
  * `monitor/`: WOZMON, the disassembler
  * `hyforth/` (as now)
  * `include/`: the split `defines.s` (below), `zero.s`
* **Split big files**: `bios.s` (print routines, serial driver, VIA, dead I2C) into `print.s`, `serial.s`, `via.s`; `defines.s` (1,000 lines) into `hw.inc` (VIA, ACIA, YM registers, IO ports), `kernel.inc` (tasks, IRQs, MMU, errors), `io.inc` (IO, H9P, namespaces, servers), `macros.inc`.  Give `sound.s` its own segment (it has none and lands in `SHELL`); merge the 8-line `shell.s` into `os_main.s`.

### **5. Shared helpers**
* One pair of bank mapping macros for the four hand-written variants (`_M_SYS_ENTER`, `_M_IO_MAP_XFER`, and inline in `NS_PUT_DEV`, `IO_DEV_FIND`, `IO_SRV_MAP`).
* Macros for the "peek / poke another task's ZP" quick switch (about 8 copies).
* An `fd * 8` helper in `io.s` (about 10 copies), and an `IO_SRV_COUNT` helper for the servers' "map, set the count, unmap".

### **6. RAM**
* The resident tasks' fixed buffers (serial rings `$0200-$03FF`, the pipe table and rings `$0200-$0AFF`) come from the MMU instead (the pipe task's reach above the MMU's bottom page).  `MMU_PAGE_BOTTOM` (`$08`: HyForth's buffers) could be per task.
* Move single-purpose OS ZP (the disassembler's `ZP_D_*`) to task ZP.

### **7. Later: HyForth's core from ROM**
* Running HyForth's core from ROM page 1 instead of a 6K RAM copy frees shell RAM and roughly halves `TASK_CLONE`'s time.  Needs a RAM trampoline for `SYSCALL` (self-modifying) and changes to the dictionary layout: a project of its own.
