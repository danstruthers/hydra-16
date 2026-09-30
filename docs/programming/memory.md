## **Memory: the MMU, shared memory, far pointers**

How a task allocates memory, shares it with other tasks, and refers to memory anywhere in the machine.  Sources: `os_rom/kernel/mmu.s`, `os_rom/kernel/shared.s`, `os_rom/kernel/fp.s`.  Part of the [Programmer's Guide](README.md).

### **The MMU: a task's own memory**

Each task has its own memory manager state, the **MMU area** at `$7E00-$7FFF` of its RAM.  So a task switch swaps it in with the rest of the task's RAM.  All memory is reached through **handles**: 1-byte numbers (1-101) that stay valid until freed.  (The MMU area stops at `$7FF7`: the last 8 bytes of task RAM are left alone in every task, as task F's are a DS1747's clock registers when one is in U7.)

**What an allocation gets**, by size (`MM_ALLOC`):

| Size | Gets | Where |
| :--- | :--- | :---- |
| 1-3 bytes | Stored in the handle entry itself | The handle table |
| 4-64 bytes | A 4, 8, 16, 32 or 64-byte chunk | Chunk pages (256-byte pages split into chunks of one size) |
| 65 bytes and up | Whole 256-byte pages, contiguous | Task RAM `$0800-$7CFF`, from the top down |
| any, with `.X` = `AI_PAGED` (`$01`) | Whole 8K RAM banks | The task's banks on the RAM modules, seen at `$8000-$9FFF` |

Allocations don't move: a pointer from `MM_LOCK` stays valid until `MM_UNLOCK`.  The calls hold `NO_PREEMPT` while they work (the task isn't switched out halfway), not interrupts off: only the task's own MMU area is involved, and IRQ handlers never allocate.

**The calls** (all: C = 0 on success; C = 1 and an error in `.A` on failure):

| Call | In | Out |
| :--- | :- | :-- |
| `MM_ALLOC` (`$F821`) | `.A.Y` = size (1-`$FFFF`), `.X` = 0 or `AI_PAGED` | `.A` = handle |
| `MM_FREE` (`$F824`) | `.A` = handle | Fails with `ERR_MEM_LOCKED` if it's locked |
| `MM_READ` (`$F827`) | `.A` = handle, `.Y` = offset (0-255) | `.A` = the byte |
| `MM_WRITE` (`$F82A`) | `.A` = handle, `.Y` = offset, `.X` = the byte | |
| `MM_LOCK` (`$F82D`) | `.A` = handle | `.A.Y` = pointer, `.X` = the previous RAM bank.  For a bank allocation, its first bank is selected at `$8000` |
| `MM_UNLOCK` (`$F830`) | `.A` = handle, `.X` = the bank from `MM_LOCK` | Restores the RAM bank |
| `MM_FIND` (`$F84E`) | `.A.Y` = a chunk or page allocation's address | `.A` = its handle |
| `MM_SET_FLOOR` (`$F851`) | `.A` = page | Pages below it aren't handed out (so a program can use RAM from `$0800` up directly) |
| `MM_TASK_RESET` (`$F84B`) | `.A` = task | Frees everything the task has (done for you when a task ends) |

`MM_READ` and `MM_WRITE` check the offset against the size of small allocations and chunks.  For offsets above 255, lock the allocation and use the pointer.

**Example**: a 300-byte buffer.

```
            lda     #<300
            ldy     #>300
            ldx     #0
            jsr     MM_ALLOC            ; .A = handle (two pages)
            bcs     @error
            sta     BUF_HANDLE
            jsr     MM_LOCK             ; .A.Y = its address, .X = RAM bank to restore
            sta     BUF_PTR
            sty     BUF_PTR + 1
            stx     BUF_BANK
            ...                         ; use (BUF_PTR),Y
            lda     BUF_HANDLE
            ldx     BUF_BANK
            jsr     MM_UNLOCK
            lda     BUF_HANDLE
            jsr     MM_FREE
```

**Bank memory** (`AI_PAGED`) is only visible while locked: `MM_LOCK` selects the bank at `$8000-$9FFF`.  A task has 16 banks per installed RAM module (bank IDs `$00-$EF`, as far as modules are fitted).  An allocation of several banks gets consecutive IDs; select the next ones by writing `$00` yourself.

**The page floor.**  The MMU allocates task RAM pages from `$7C00` down.  A program that uses RAM from `$0800` up directly sets the floor above its data (`MM_SET_FLOOR`), so the MMU never hands those pages out.  HyForth keeps its floor 2 pages above its dictionary.  `TASK_CLONE` skips the free pages between the floor and the lowest allocation.

**Layout of the MMU area** (for debugging; `MmuHeader` in `mmu.s`): a version byte, the page and bank maps with their run-end maps, the chunk list heads, the low-water mark and page floor, then the handle table (4 bytes an entry: address, bank, status).

### **Shared memory**

Shared memory lives in the shared RAM banks, which every task can map.
* **Units:** it's allocated in whole 8K banks.
* **Handles:** a **shared handle** is 1 byte, valid in every task, so a task can send it to another (in a pipe, say).
* **References:** each task that uses the memory holds a reference.  The banks are freed when the last reference is dropped, so a task that ends can't leak them.

| Call | In | Out |
| :--- | :- | :-- |
| `SH_ALLOC` (`$F836`) | `.A.Y` = size | `.A` = shared handle; the caller holds the first reference |
| `SH_ATTACH` (`$F839`) | `.A` = shared handle | Take a reference (e.g. to a handle received from another task) |
| `SH_DETACH` (`$F83C`) | `.A` = shared handle | Drop this task's reference |
| `SH_READ` (`$F83F`) | `.A` = shared handle, `.Y` = offset (first bank) | `.A` = the byte |
| `SH_WRITE` (`$F842`) | `.A` = shared handle, `.Y` = offset, `.X` = the byte | |
| `SH_LOCK` (`$F845`) | `.A` = shared handle | Its first bank is mapped at `$8000-$9FFF`; `.X` = previous RAM bank, `.Y` = previous `U` |
| `SH_UNLOCK` (`$F848`) | `.X`, `.Y` from `SH_LOCK` | Restores the RAM bank and `U` |

The calling task must hold a reference for all but `SH_ALLOC` and `SH_ATTACH`.

**How shared banks are numbered.**  The 2 MB of shared RAM is 256 banks, the **shared bank ID** = `U << 4 | (bank & $0F)`.  Bank ID `$37`, for example, is `U` = 3 with `$00` = `$F7`.  `U` is global (not per task), so code that sets it must put it back; `SH_LOCK`/`SH_UNLOCK` do.

**Reserved shared banks:**

| Shared bank ID | Use |
| :------------- | :-- |
| `$00` | System tables: the shared bank map (`$8200`), the shared handle table (`$8400`), the device table (`$8800`), the shared reference table (`$8900`) |
| `$09` | The IO transfer areas: 512 bytes per task ([servers.md](servers.md#the-request-block-and-the-transfer-area)) |
| any on a bad chip | Reserved at boot if POST found the chip bad; missing `U` macro-pages too |

### **Far pointers**

A 16-bit address alone is ambiguous on the Hydra: what's at `$8000` depends on `$00` and `U`, at `$A000` on `$01`, at `$E000` on `W`, and below `$8000` on `T`.  A **far pointer** carries that context, so it means the same thing from any task and any ROM page.

`FarPtr` (4 bytes, `include/kernel.inc`):

| Byte | Field |
| :--- | :---- |
| 0-1 | `addr`: the address |
| 2 | `space`: the kind in bits 0-1: `FP_TASK` (0), `FP_PROM` (1), `FP_SHARED` (2), `FP_BIOS` (3).  Bit 2 is `FP_RO` (read-only).  For `FP_TASK`, bits 4-7 hold the task whose RAM it is |
| 3 | `sel`: `FP_TASK`: the RAM bank (for `$8000-$9FFF`); `FP_PROM`: the paged ROM bank; `FP_SHARED`: the shared bank ID; `FP_BIOS`: the BIOS ROM page |

A task's RAM can only be read through a far pointer by that task.  The others get `ERR_MEM_NOT_VALID`: tasks can't see each other's RAM.  ROM and shared RAM work from anywhere.

The calls work on the far pointer register **`ZP_FP`**, in the calling task's zero page:

| Call | In | Out |
| :--- | :- | :-- |
| `FP_MAKE` (`$F8B1`) | `.A.Y` = an address as the caller sees it, `.X` = the caller's ROM page | `ZP_FP` describes it |
| `FP_READ` (`$F8B4`) | `.Y` = offset | `.A` = the byte at `addr + .Y` |
| `FP_WRITE` (`$F8B7`) | `.Y` = offset, `.X` = the byte | Refused (`ERR_MEM_NOT_SUPPORTED`) for ROM or `FP_RO` |
| `FP_COPY` (`$F8BA`) | `.A.Y` = destination (task RAM, or the caller's bank at `$8000`), `.X` = count (0 = 256); C = 1 to copy a string up to its 0 | `.X` = bytes copied |

```
            lda     #<MY_ROM_STRING     ; a string in this page's ROM
            ldy     #>MY_ROM_STRING
            ldx     W_REGISTER          ; our ROM page
            jsr     FP_MAKE             ; ZP_FP = it, in a form any task can use
```

The IO layer reads names through far pointers.  So `IO_OPEN` and friends take a name in RAM, in the paged ROM, or on the caller's own BIOS page.

### **References: far pointers as handles**

A **reference** wraps a far pointer in a handle, so code that takes handles can use static data (a ROM table, a string) the same way as allocated memory:

| Call | In | Out |
| :--- | :- | :-- |
| `MM_REF` (`$F8BD`) | `ZP_FP` | `.A` = an MMU handle for it (this task).  `MM_READ`, `MM_WRITE` (not ROM), `MM_LOCK`/`MM_UNLOCK` (task RAM, and the paged ROM: its bank is selected), and `MM_FREE` (drops the handle) |
| `SH_REF` (`$F8C3`) | `ZP_FP` (ROM or shared RAM) | `.A` = a shared handle, usable by any task: `SH_ATTACH`, `SH_READ`, `SH_WRITE`, `SH_LOCK` (shared RAM), `SH_DETACH` |
| `MM_FP` (`$F8C0`) | `.A` = MMU handle | `ZP_FP` = where it is (allocation or reference) |
| `SH_FP` (`$F8C6`) | `.A` = shared handle | `ZP_FP` = where it is |

A reference to shared RAM or a BIOS page can't be locked (`ERR_MEM_NOT_SUPPORTED`): the BIOS page can't be mapped while ROM code runs.  Use `MM_READ`, or `MM_FP` then `FP_COPY`.

### **Copying**

`MEM_COPY` (`$F81E`) copies `.A.Y` bytes from `ZP_TEMP_VEC` to `ZP_TEMP_VEC2`, within what the caller can see.  Between tasks, use shared memory, a pipe, or the IO layer.
