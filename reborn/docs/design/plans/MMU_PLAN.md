## **Hydra MMU Plan**

### **Goals**
* Per-task heap allocations, using tables stored in that task's own RAM. A task switch (`T`) swaps them in automatically.
* Shared allocations for cross-task messaging and driver transfer buffers, using tables stored in shared RAM.
* HyForth (the future shell) gets its large memory blocks from the MMU.
* All access goes through **handles** via the MMU API, plus a lock call that returns a raw pointer for speed-critical code.

### **Tiers of allocation (per task)**

| Tier | Size | Source | Tracking |
| :--- | :--- | :----- | :------- |
| Small | 1-3 bytes | Stored inside the handle entry itself (`AI_SMALL`) | Handle table |
| Chunk | 4-64 bytes | Fixed-size chunk allocators (4, 8, 16, 32, 64) carved from 256-byte pages (no 128: with the header in the first chunk, a 128-byte chunk page would hold only one chunk) | Chunk page header + free list |
| Page | 129 bytes - ~16K | Contiguous 256-byte pages in `$0800-$7CFF`, allocated top-down | Page bitmap (16 bytes) |
| Bank | 8K units | Task banks `$00-$EF`, seen through `$8000-$9FFF` | Bank bitmap (30 bytes) |

Requests are rounded up to the next chunk size. Callers can force a tier with flags (e.g. `AI_PAGED` for bank memory).

### **Where the MMU area lives: top of Task RAM (`$7E00-$7FFF`)**
The low end of Task RAM is already full: `BUFFERS` packs `$0200-$07FF` (serial input, the Forth TIB and stacks, `MEMSTK`, the WOZMON `IN` buffer), and the Forth RAM dictionary starts at `$0800`. The top of Task RAM, directly under the paged window, is better for the MMU area:
* ROM code in every task can use the same fixed address, whatever the task is running and however the Forth/shell layout changes later.
* Page `$7D` is the **task system page** (IRQ/SWI registration tables, spurious counters, driver state; see the IO subsystem section). MMU pages are allocated **top-down** from `$7C`, and the Forth dictionary grows **up** from `$0800`. The MMU header keeps a **low-water mark** (the lowest allocated page), and the dictionary may grow up to it.
* Pages `$00-$07` and `$7D-$7F` are pre-marked in use in the page bitmap.

### **Per-task layout (Task RAM, starting at `$7E00`)**

| Offset | Size | Description |
| :----- | :--- | :---------- |
| $00 | 1 | MMU status / version (non-zero = initialized) |
| $01 | 16 | Page bitmap for pages `$00-$7F` (pages `$00-$07` and `$7D-$7F` pre-marked in use) |
| $11 | 30 | Bank bitmap for banks `$00-$EF` |
| $2F | 6 | Chunk allocator heads, one page number per size class (0 = none) |
| $35 | 16 | Page run-end bitmap (marks the last page of each page allocation) |
| $45 | 30 | Bank run-end bitmap (marks the last bank of each bank allocation) |
| $63 | 1 | Low-water mark: lowest allocated page (`$7D` when empty) |
| $64 | .. | Handle table (fills the rest of the MMU area) |

**MMU area size** = `MMU_TASK_PAGES` (build-time constant). **Suggested default: 2 pages (`$7E00-$7FFF`), about 100 handles.** 1 page gives about 36 handles, which is enough for small tools. 3 pages give about 160, for Forth or programs that allocate a lot. With the chunk allocators, most tasks should need far fewer than 100 live allocations.

**Chunk page header** (first bytes of each chunk page): `size_class`, `free_count`, `free_head` (byte offset into the page, 0 = full), `next_page` (next page of the same class). Free chunks are linked through their first byte. That's one byte per link, because every chunk sits inside a single page.

**Handle** = 1-byte index into the handle table (0 = invalid). Each entry is 4 bytes, the same as the current `SmallAllocInfo`: `PageAddress` (addr.w + bank), then `status`. Sizes aren't stored in the entry. The allocator gets them from the chunk page header (chunks) or the run-end bitmaps (pages and banks). For small allocations, the data bytes sit where the `PageAddress` would be, and the length (1-3) goes in status bits 0-1, which are unused when `AI_SMALL` is set. A 1-byte handle fits in a register, and because it's an index, the MMU can move blocks later without breaking handles.

### **Shared memory (256 banks: `$F0-$FF` × `U` 0-F)**

* **Shared bank ID** = `U << 4 | (bank & $0F)`. The existing `PageAddress::page` encoding already works this way.
* **Bank ID `$00` holds the system data** (tables and message rings), and no one allocates from it:
  * Shared bank bitmap (32 bytes) and an owner/refcount table (256 bytes: owner task in the high nibble, reference count in the low nibble).
  * Shared handle table (4-byte entries: `status`, `PageAddress`). Shared handles are 1 byte too, so they can be sent in messages.
  * Head/tail pointers for the 256 message rings (512 bytes).
* **Message rings live in shared bank IDs `$01-$08`**. There's one 256-byte ring for each receiver/sender pair (16 × 16 = 64K):
  * Bank ID = `1 + (receiver >> 1)`, address = `$8000 + (receiver & 1) * $1000 + sender * $100`.
  * Each ring has a single writer and a single reader, so senders never contend. 8-bit pointers wrap for free.
* Any task can call the shared API. A `php`/`sei`...`plp` critical section protects table updates, so the "system task only" limit can go away (it currently blocks drivers).
* Shared allocations are in banks (8K) or 256-byte pages inside a bank. Chunk allocators aren't needed here.

### **Messaging and buffered transfers**
*(The message rings were built (`msg.s`), then removed in the code cleanup: pipes and `/dev/cons` replaced them (see `IO_PLAN.md`), and their shared bank IDs `$01-$08` are free for `SH_ALLOC` now.  The notes below are the original design.)*

* **Message** = `type`, `len`, then up to a few payload bytes, written into the receiver's ring for that sender. The sender is implied by which ring it's in. `MSG_RECV` scans the 16 rings round-robin, or reads one sender's ring directly. Larger data goes in a shared buffer, and the message carries only its **shared handle**.
* When a task waits on an empty inbox, `TASK_STATUS_REG` bit 2 (Awaiting I/O) is set, and `MSG_SEND` clears it.

### **HyForth integration**
HyForth keeps its own allocator for small records and gets everything large from the MMU.

* **Small-record arena:** at `cold`, Forth gets its arena from the MMU (`FORTH_ARENA_PAGES` pages, e.g. 8 = 2K) and points `MEMTOP`/`MEMLAST` at the top of it, replacing the fixed `MEMTOP = $8000`. `MALLOC`, `MEMSTK`, `mlen`, `mktemp` and `purge0` keep working as they are inside the arena.
* **Large blocks:** `malloc` requests at or above `FORTH_LARGE_MIN` (e.g. 256 bytes) call `MM_ALLOC` (page tier) instead:
  * The block starts with the same 3-byte `type`/`len` header, so `mlen` and existing words that use records don't change.
  * Type bit `$40` marks the record as MMU-backed. Bit `$80` (temp) keeps its current meaning.
  * The `MEMSTK` slot holds the block's address, the same as for arena records. Page-tier blocks don't move, so the address stays valid.
  * When a record is freed (`purge0` or a new `mfree`), MMU-backed records call `MM_FIND` (address → handle) and then `MM_FREE`.
* **Bank memory (more than ~16K, or explicit):** new words `halloc ( bytes flags -- h )`, `hfree ( h -- )`, `hlock ( h -- addr )` and `hunlock ( h -- )`. Bank memory is only visible in the `$8000-$9FFF` window while it's locked.
* **`ALTBUF`:** allocated from the MMU (16 pages) at `cold`, or when first used, instead of the fixed `$6000-$6FFF`.
* **Dictionary limit:** `here` growth and the `free` word check against the MMU low-water mark instead of `MEMLAST`.
* **Task exit:** `MM_TASK_RESET` frees the arena, large blocks and `ALTBUF` with no Forth cleanup needed.
* **Later:** `MEMSTK` could become MMU handles too, which frees `$0500-$06FF`. That isn't needed for this phase.

### **Task completion (`MM_TASK_RESET`)**
Called from `TASK_START`'s `@task_complete`. It frees all of the task's memory without walking its handles:
1. Re-initialize the task's MMU area: clear the bitmaps, chunk heads and handle table. All task pages and banks become free at once.
2. Shared memory: scan the owner/refcount table. The task's references are dropped, and any bank whose count reaches 0 is freed. Banks still in use by other tasks survive until they're released.
3. Reset the head and tail of the 32 message rings the task is part of: its 16 inboxes, and its ring in each other task's inbox.

### **API (all: C = 0 success, C = 1 with error in .A)**

| Call | In | Out |
| :--- | :- | :-- |
| `MM_ALLOC` ($F821) | .A.Y = size (.A = low), .X = 0 or `AI_PAGED` | .A = handle |
| `MM_FREE` ($F824) | .A = handle | fails if locked |
| `MM_FIND` | .A.Y = block address | .A = handle |
| `MM_LOWWATER` | | .A = lowest allocated page |
| `MM_READ` / `MM_WRITE` ($F827 / $F82A) | .A = handle, .Y = offset 0-255 (.X = byte for write) | .A = byte |
| `MM_LOCK` ($F82D) | .A = handle | .A.Y = pointer, .X = previous RAM bank; selects the bank (`AI_PAGED`); pins the block |
| `MM_UNLOCK` ($F830) | .A = handle, .X = bank from `MM_LOCK` | restores the bank |
| `MMU_TEST` ($F833) | | runs the handle calls in the current task and prints `MMU test: ok` or `FAIL x ee` (WOZMON: `F833R`) |
| `SH_ALLOC` / `SH_FREE` / `SH_READ` / `SH_WRITE` / `SH_LOCK` | same shapes, shared handles | |
| `MSG_SEND_BYTE` | .A = byte, .X = receiving task (sender = current task) | C = 1 if the ring is full |
| `MSG_RECV_BYTE` | .X = sending task, or `$FF` for any | .A = byte, .X = sender; C = 1 if empty |
| `MSG_PEEK` | .X = sending task | .A = bytes waiting |

All calls go through `BIOS_THUNKS`, after `MEM_COPY`.

### **Zero-page convention**
Every task has its own zero page, so ZP only has to be divided up **within one task**, not across the system.
* **OS/BIOS ZP grows up from `$02`** (the `ZEROPAGE` segment, `zero.s`). It has the same layout in every task, because the same ROM code runs in every task. It currently ends at `$7A`.
* **Task ZP grows down from `$FF`**. Each task's code (shell, sound driver, serial driver, ...) reserves its ZP with a macro, instead of hard-coded addresses (`sound.s` `$A0`/`$B0`) or `.org` (HyForth `$C8`):
  * `TASK_ZP_BEGIN` resets a per-task counter to `$100`.
  * `TASK_ZP name, size` lowers the counter by `size` and defines `name` at the new value.
  * `TASK_ZP_END` adds a link-time `.assert` that the counter is still above the end of the `ZEROPAGE` segment. The linker checks this for every task image.
* Drivers and the shell run in different tasks, so their task ZP can overlap freely. For example, sound and Forth can both use `$C8-$FF`.
* A driver that shares a task with other code (not planned) would have to allocate its ZP after that code's, from the same counter.

### **Build order**
**Phase 1 - MMU basics (done, stage 1)**
1. `MMU_INIT` / `MM_TASK_INIT`: clear each task's MMU area and bitmaps (shared tables still a stub). `jsr MMU_INIT` is on at reset.
2. Page and bank bitmap allocators (`MM_PAGE_ALLOC`/`FREE`, `MM_BANK_ALLOC`/`FREE`, `BM_*`).

**Phase 2 - Drivers and shell in their own tasks (done; pulls work forward from the IO subsystem)**
3. **(Done) ZP convention:** add the `TASK_ZP` macros, then convert `sound.s` and HyForth's ZP to them.
4. **(Done) ROM layout:** the MMU and OS stay on page 0. HyForth and the disassembler moved to page 1 (`W = 1`), with far-call gates in both directions (`page0_gates.s`, `page1.s`) and a `COMMON` block at `$FD00` on every page (IRQ/NMI entry, far-call trampolines). Each ROM page is its own 8K linker memory area.
5. **(Done) IRQ dispatcher:** per-IRQ stubs on every BIOS page, registration tables in the task system page (`$7D00`), and `IRQ_REGISTER`/`SWI_REGISTER`. See the IO subsystem section.
6. **(Done for sound; serial in step 8) Resident driver tasks:** `TASK_STATUS_REG` Resident bit, and `DRV_START` (run a driver's `init` in its task). Boot (task 0) starts:
   * the **sound** task: `SOUND_INIT` and the YM IRQ handler.
   * the **serial** task: `SERIAL_INIT` and `SERIAL_IRQ_HANDLER`.
7. **(Done, msg.s; removed later: replaced by pipes) Message rings (byte streams):** fixed shared banks `$01-$08` with ring pointers in shared bank `$00`, and `MSG_SEND_BYTE`/`MSG_RECV_BYTE`/`MSG_PEEK`, plus `MSG_RESET_TASK` for task reset. Framed messages (type, length) can be layered on top later. This doesn't need the shared allocator.
8. **(Done) Serial through messages:** the serial driver runs in task `$F`; the serial task's RX handler writes to the capture task's ring, and `READ_CHAR` reads from the ring. TX stays polled/direct at first. Retire `INPUT_BUFFER`.
9. **(Done) Shell in its own task (task 1, via `TASK_PREPARE` + `SWITCH_TO`):** task 0 finishes boot, starts the shell task (Forth/WOZMON) as the serial-capture task, and hands over to it with `SWITCH_TO`. This needs no preemptive scheduler: the only running task is the shell, and the drivers run from IRQs.

**Phase 3 - Rest of the MMU**
10. **(Done, stage 2)** Handle table plus `MM_ALLOC`/`MM_FREE` for the small, page and bank tiers, then `MM_READ`/`WRITE`/`LOCK`/`UNLOCK`, the `$F821-$F835` thunks and `MMU_TEST`.  Until chunks exist (step 11), 4+ byte allocations take whole pages.
11. **(Done, stage 3)** Chunk allocators: 4, 8, 16, 32 and 64-byte chunks; 65+ bytes take whole pages.  A chunk page that becomes empty goes back to the page allocator.
12. **(Done, stage 4)** Shared bank allocator and shared handles (`shared.s`): whole 8K banks, tables in shared bank ID `$00` (`$8200` bitmaps, `$8400` handle table of 255 entries). Each entry keeps a mask of the tasks holding a reference instead of a count, so a task reset drops exactly that task's references. `SH_ALLOC`, `SH_ATTACH`, `SH_DETACH`, `SH_READ`, `SH_WRITE`, `SH_LOCK`, `SH_UNLOCK` (`$F836-$F848`). Bank IDs `$00-$08` and missing `U` macro-pages are reserved at boot. 256-byte pages inside a shared bank are left for the IO subsystem.
13. **(Done, stage 4)** `MM_TASK_RESET` (`$F84B`): resets the task's MMU area, drops its shared references, empties its message rings, and unregisters its IRQ/SWI handlers. Hooked into task completion; `TASK_START` now saves the parent's context so the parent resumes correctly. Driver `stop` is still to come (no driver task completes yet).
14. **(Done, stage 5)** HyForth: `cold` resets the task's MMU area and gets a 2K arena for small `malloc` records (`FORTH_ARENA_SIZE`); `MALLOC` now fails cleanly (out of memory) instead of overrunning. Records of 256+ bytes get their own MMU block (type bit `MEM_MMU` = `$40`), freed by `purge0` via `MM_FIND` + `MM_FREE`. The dictionary is protected by the MMU page floor (`MM_SET_FLOOR`): `DICTCHK` keeps it `FORTH_DICT_MARGIN` pages above `here`, checked before each compiled word and in `:`, `var`, `cons` and `bload` ("out of memory" instead of corruption). New words `halloc`, `hfree`, `hlock`, `hunlock`, `mmtest`; `free` reports the room between `here` and the lowest MMU page. `ALTBUF` was unused, so it's left as is.
15. Tests in the style of `MEM_TEST` that you can run from WOZMON or Forth.
16. **(Done)** Far pointers and references (`fp.s`, BIOS ROM page 5).  A far pointer (`FarPtr`: address, kind, selector) says what's mapped at its address (a task's RAM and RAM bank, a shared bank, a paged ROM bank, a BIOS ROM page), so it reads the same from any page and any task (except a task's own RAM, from another task): `FP_MAKE`, `FP_READ`, `FP_WRITE`, `FP_COPY` (a BIOS page is read through `FP_PEEK_PAGE` in the COMMON block).  References are handles for far pointers: `MM_REF` (an MMU handle entry with `AI_SMALL | AI_BLOCK` set, the kind in bits 0-1; `MM_READ`, `MM_WRITE`, `MM_LOCK` / `MM_UNLOCK` for task RAM and the paged ROM, `MM_FREE`) and `SH_REF` (a shared handle whose count is `SH_REF_MARK`, its far pointer in `SH_REF_TBL` at `$8900` of shared bank ID `$00`; `SH_READ`, `SH_WRITE`, `SH_LOCK` for shared RAM, `SH_ATTACH` / `SH_DETACH`); `MM_FP` / `SH_FP` give any handle's far pointer.  The IO layer reads callers' names through far pointers (`IO_OPEN`, `IO_MOUNT`, `IO_BIND`, `IO_UNMOUNT`, `DEV_REGISTER`), taking the caller's ROM page from the far call's stack frame.  Tested by the MMU test (steps 5-9) and the IO test (step k).  (To make room on page 0, POST moved to page 4: `tests/post.s`.)

**Later:** the preemptive scheduler (VIA T1 tick plus a reschedule flag), serial TX through messages, driver loading from the Forth shell (`drv-load`, `capture`, ...), and shared ring buffers for bulk transfers.

### **IO subsystem (IRQ dispatch and drivers: Phase 2; the rest later)**
The IRQ dispatcher, resident driver tasks and serial-through-messages are built in Phase 2 (see Build order). The rest comes after the MMU work.

#### **Current IRQ handling (what gets replaced)**
* The 16-entry vector RAM (`$FFFE`, indexed by `V[0..3]` on write) points **directly at handlers**, set with `IRQ_SET_VECTOR`. Handlers run in **whatever task is current**, using that task's ZP and stack. For example, `SERIAL_IRQ_HANDLER` writes into the interrupted task's `INPUT_BUFFER`/`ZP_WRITE_PTR`.
* The vectors are copied onto every BIOS page, but the handlers only exist on page 0. An IRQ while `W <> 0` jumps into whatever code is on that page.
* Known bugs:
  * `IRQ_VECTOR_INIT` defaults every unused IRQ to `SERIAL_IRQ_HANDLER`.
  * `IRQ_SET_VECTOR` and `SERIAL_INIT` always turn interrupts back on.
  * `SW_IRQ_HANDLER` does nothing.
  * `SW_INT` puts `rts` directly after `brk`, and `brk` returns past the following signature byte, so that `rts` is skipped.
  * `NMI_HANDLER` returns immediately, and the scheduler code after its `rti` never runs.

#### **New design: every IRQ runs a registered handler in a registered task**
1. **Per-IRQ entry stubs, on every BIOS page.** Each of the 16 vectors points to a stub in a new `IRQ_TRAMP` segment that sits at the **same offset on every BIOS page** (like `RESETVEC_Pn`). A stub is `pha` / `phx` / `ldx #irq` / `jmp IRQ_ENTRY`. `IRQ_ENTRY` is also on every page: it saves `W`, sets `W = 0` and continues into the dispatcher on page 0. The exit path mirrors this (restore `W`, then `rti` from the same offset on every page).
2. **Registration table, copied into every task.** The table goes in a new per-task **system page at `$7D00`**:
   * The layout is `IRQ# × IRQ_MAX_CHAIN (2)` entries of `{task, handler.w}` = 96 bytes. It's indexed by the logical IRQ number (the `IRQ_NUMBER()` errata mapping only applies when writing `V`).
   * `IRQ_REGISTER` writes the entry into all 16 tasks' copies (switching `T` in a loop, the way `TASKS_INIT` does). So the dispatcher can read it from whichever task was interrupted, with no bank switching. Registration is rare, and dispatching happens constantly.
   * Software interrupts get their own 16-entry `{task, handler.w}` table (48 bytes), indexed by `V[4..7]`.
3. **Dispatcher (`IRQ_DISPATCH`), with I set throughout:**
   1. `phy`, then save the current `T`. `tsx` / `stx STACK_SAVE_REG` stores the interrupted task's SP in its own ZP, which is the right value for the scheduler too.
   2. For each chain entry of this IRQ: if its task differs from the current task, set `T` = the handler's task, then `ldx STACK_SAVE_REG` / `txs`. The handler then runs on its own task's stack, **below** that task's saved frame, with its own ZP, `RAM_BANK_REG` and MMU area.
   3. `jsr` the handler. Handler convention: preserve `RAM_BANK_REG`/`U`, and return **C = 1 if the interrupt was claimed**. That stops the chain. Shared sources like the VIA (scheduler tick on T1, serial TX pacing on T2) can have one handler per sub-device.
   4. Switch back: `T` = the interrupted task, restore its SP from `STACK_SAVE_REG`, pull the registers, go through the `W` exit and `rti`.
   5. For logical IRQ 15 (and `BRK`), read `V[4..7]` and dispatch through the software table instead.
   6. Unregistered or unclaimed IRQs increment a spurious counter in the system page and return.
4. **Scheduler hook:** the system-task handler for VIA T1 can set a "reschedule" flag. At step 4, the dispatcher then switches to `NEXT_TASK` instead of the interrupted task (this replaces the dead code in `NMI_HANDLER`).
5. **Cost:** about 60-80 cycles of overhead per IRQ. At 19200 baud a byte arrives about every 1860 cycles, so there's plenty of margin. A driver task's stack needs about 32 bytes of headroom below its saved SP.

#### **API**

| Call | In | Out |
| :--- | :- | :-- |
| `IRQ_REGISTER` | .X = `IRQ_NUMBER(n)`, .A.Y = handler; the handler runs in the calling task | C = 1 if the chain is full |
| `IRQ_UNREGISTER` | .X = IRQ#, .A.Y = handler | |
| `SWI_REGISTER` / `SWI_UNREGISTER` | .X = SW#, .A.Y = handler; runs in the calling task | |
| `SW_INT` (fixed) | .A = SW# | saves/restores `V`, `brk` followed by a signature byte |

When a task completes, the cleanup that runs with `MM_TASK_RESET` also removes that task's handlers. It first calls each driver's `stop` entry, so the device's interrupt is turned off before its handler goes away.

#### **Drivers**
* **Driver descriptor** (in ROM, or loaded): `name`, `init`, `stop`, the IRQ#s it uses, and a minimum stack/RAM requirement.
* **Driver tasks** get a new `TASK_STATUS_REG` bit 3 = **Resident**: the task stays allocated but isn't scheduled as a main loop. Its code runs only from IRQs and when it receives messages.
* **Loading:** `DRV_START(descriptor, task)` reserves or uses the given task, runs the driver's `init` **in that task** (so its state lives in that task's ZP/RAM), and `init` calls `IRQ_REGISTER` with `T_REGISTER`.
* **Serial driver:**
  * RX: its IRQ handler (in the driver task) writes each byte to the **serial-capture task's** message ring, from sender = driver task. The capture task number is a driver variable, changed by a control message (the shell sets it).
  * TX: tasks send bytes by message to the driver task, and the TX-ready IRQ drains them.
  * `WRITE_CHAR`/`READ_CHAR` become message calls. A polled fallback remains for boot/WOZMON before the driver starts.
  * The BIOS `INPUT_BUFFER` at `$0200` and `ZP_READ_PTR`/`ZP_WRITE_PTR` are then retired.
* **Sound driver:** the YM2151 timer IRQ (`SOUND_IRQ_HANDLER`) is registered to the sound task the same way.
* **Forth shell:** words like `drv-load ( desc task -- )`, `drv-stop ( task -- )`, `capture ( task -- )` and `irqs` (list registrations and spurious counts).

#### **Other IO work**
* The shared-ring write path is used from IRQ context, so it saves and restores `RAM_BANK_REG`/`U` and doesn't depend on the MMU area.
* Bulk transfers from drivers (e.g. SD card) use shared buffers passed by shared handle. `SHARED_RING_PUT`/`GET` for streaming belong here.

### **Decisions**
* The MMU area lives at the top of Task RAM (`$7E00`), and pages are allocated top-down.
* HyForth gets its small-record arena and all large blocks from the MMU.
* Phase 2 runs on the hardware (Forth in task 1, sound and serial in driver tasks). The ACIA's IRQ arrives at `IRQ_NUMBER(1)` as documented (no spurious counts); the dispatcher's poll-all-handlers fallback for unclaimed IRQs stays as a safety net.
* The power-on self test (`POST` in `tests/post.s`, on BIOS page 4) is permanent and will be expanded over time. It runs first at reset, with polled serial output and no IRQs, so it works even when the task/IRQ system doesn't.
* `$7D00-$7DFF` is the task system page. All IRQs, not just software interrupts, run registered handlers in their registered task (IO subsystem phase).
* Handle table: 4-byte entries, with `MMU_TASK_PAGES` = 2 by default (about 100 handles). This can be changed after real use.
* All of a task's memory is reset when the task completes (see `MM_TASK_RESET`).
* Message boxes: one ring for each receiver/sender pair.
