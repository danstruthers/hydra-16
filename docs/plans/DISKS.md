## **RAM and ROM disks: /ram and /rom as HydraFS volumes**

Disks in memory, and what they let the rest of the system drop:
* **The ROM disk:** the whole paged ROM (4 MB) as one read-only disk, so everything in ROM that isn't code is a
  file.  **Built** (the disk `x`, mounted at `/rom`).
* **RAM disks:** a program cache and scratch files, fast, with no card: an area for each task, and a shared one.
  **Built** (the disks `r` and `s`: each shell's own area mounted at `/ram`, and `/sram`).

All of them are block devices under the HydraFS server, as the cards are, and [io.md](../programming/io.md#the-ram-disks-ram)
describes them as they are.  The names put together from them (`/bin` over the caches, the cards and the ROM)
are the [namespace plan](NAMESPACES.md)'s, and a task's memory as files is the [/proc plan](PROC.md)'s.

### **Contents**
1. [What it looks like](#what-it-looks-like)
2. [Disks in memory](#disks-in-memory)
3. [The ROM disk](#the-rom-disk) (built)
4. [Pointers and banks: what can't go wrong](#pointers-and-banks-what-cant-go-wrong)
5. [RAM disks](#ram-disks)
6. [Who can use which area](#who-can-use-which-area)
7. [How the RAM disks work](#how-the-ram-disks-work)
8. [Booting: finding the disks](#booting-finding-the-disks)
9. [What the disks simplify](#what-the-disks-simplify)
10. [The program caches](#the-program-caches)
11. [Tests](#tests)
12. [Steps](#steps)

---

### **What it looks like**

| Name | What | On | Lasts |
| :--- | :--- | :- | :---- |
| `/rom` | The ROM's files: programs, songs, libraries, help (`mount hfs /rom x`) | The ROM disk, `x` | Read-only |
| `/ram` | A shell's own area on the RAM disk (`mount hfs /ram r/N`, N its task): a directory for it and the tasks it starts | The RAM disk, `r` | Until task N ends (removed then), or reset (or `stop`) |
| `/ram/bin`, `/ram/lib` | The shell's own program and library cache | The RAM disk | |
| `/sram` | The shared RAM disk, every task's (`mount hfs /sram s`) | The shared RAM disk, `s` | Until reset (or `stop`) |
| `/sram/bin`, `/sram/lib` | The shared program and library cache | The shared RAM disk | |

```
/> ls /rom/bin                            the programs in ROM
/> cp game.hyx /ram/bin/game.hyx          into this shell's cache (/ram/bin) ...
/> cp game.hyx /sram/bin/game.hyx         ... or the shared one, for every task
/> game                                   found in a cache: loaded from RAM, not the card
```

**RAM disks lose their contents on reset** (and at power-off): they're formatted each time they're started.

---

### **Disks in memory**

**A memory disk is a block device, like a card.**  Its memory is a run of banks, 8K RAM banks or 16K paged ROM
banks, shown as one run of 512-byte blocks, 0 to N-1: block n is at bank `n / 16` (RAM) or `n / 32` (ROM) of the
run, at that offset in the bank.  The banks don't show above that: the file system sees **one large partition**,
so 1.5 MB of shared RAM is one 3,072-block disk, not 192 pieces of 8K.

**HydraFS goes on it,** as on a card: the same server, so a memory disk has everything a card has (directories,
`ls -l`, stamps, `fsck`; sparse files on RAM disks), and there's no second file system.  The storage driver's
disks are named the same way in `/dev/sd` and `/sd`: an SPI device by its number as one hex digit, any other disk
by a letter that isn't one:

| Name | What | Memory | Also seen as | Disk number |
| :--- | :--- | :----- | :----------- | :---------- |
| `0`-`7` | SD cards on the board's SPI devices 0-7 | | `/sd/0` ... `/sd/7` | 0-7 |
| `8`-`f` | SD cards on slot cards' SPI devices `$8`-`$F` (planned) | | `/sd/8` ... `/sd/f` | 11-15, handed out |
| `x` | **The ROM disk** (built) | The paged ROM, all 256 banks | `/rom` | 8 |
| `r` | The RAM disk (built) | The storage task's banks on the RAM modules | `/ram` (a shell's own area, `r/N`) | 9 |
| `s` | The shared RAM disk (built) | Shared RAM banks (the board's 2 MB) | `/sram` | 10 |

The disks in memory are never under `/sd`, which has the cards alone: they're mounts of the HydraFS server with a
spec, as Plan 9's `mount` takes one (`mount hfs /rom x`), and the server serves them only through such a mount (the
request says so: `IO_BLK_SPEC`), or for task 0.  Each is also a block device, `/dev/sd/x`, `/dev/sd/r`,
`/dev/sd/s`, as a card is.

* **A hex digit is the SPI device's, always:** the card on SPI device `$A` is `/sd/a`.  A slot card can decode SPI
  devices `$8`-`$F` (`SPI_A3`: [hardware.md](../hardware.md#spi-bus-via-port-b)), so `/sd/8` ... `/sd/f` are theirs,
  and nothing else may take those names.  The other disks are letters that aren't hex digits (`x`, `r`, `s`), so
  the two can never meet, and every name is one character.
* **A name isn't a number.**  The tables are indexed by a disk number, 0-15 (`DISK_MAX`: a fid's low 4 bits), and
  `DISK_FROM_NAME` (`include/io.inc`) turns a name into it, in `/dev/sd`'s server and in HydraFS's.  Cards 0-7
  are disks 0-7, the ROM disk is disk 8.
* **Cards on slot cards** (planned) need a slot map: disks 11-15 handed out to SPI devices `$8`-`$F` at their first open
  (five at once), and `SPI_SELECT` given the device from the map, not the disk number.  The map is the driver's
  own: names, listings (`vols`, the prompt's volume) and errors always show the SPI device's hex digit, and
  `DISK_FROM_NAME` reads `8`-`f` (either case) too.

So the card tools work on them: `vols` lists them, `check` on `/dev/sd/s/ctl` checks the shared cache,
`/dev/sd/s/data` is the raw disk.

---

### **The ROM disk**

**Built.**  The paged ROM is the disk `x` (`/dev/sd/x`; disk 8 in the tables): block n is bank n / 32,
at `$A000 + (n % 32) * 512`, as the CPU sees it (8,192 blocks).  It starts with a partition table, as a card can:

| Blocks | Banks | What |
| :----- | :---- | :--- |
| 0 | 0, `$A000-$A1FF` | The partition table (an MBR), and a line saying what the disk is |
| 1-63 | 0-1 | Partition 1, type `$DA` (not a file system): the system's banks: HyForth's variables and `COPYTORAM` (bank 0, from `$A200`), the hardware test (bank 1) |
| 64-8191 | 2-255 | Partition 2, type `$7F`: the HydraFS volume, label `ROM`, read-only |

* **The storage driver** (`drivers/sd.s`, page 3) reads a block by selecting its bank in the storage task's own
  ROM bank register, copying 512 bytes into the block cache, and putting the register back (`SD_ROM_READ`).
  Writes are refused (`ERR_IO_MODE`) in the driver, in `/dev/sd`'s data and ctl files (`format`, `label`), and in
  the HydraFS server (create, write, remove, wstat), each before anything changes, the block cache included.
* **The HydraFS server** finds the `$7F` partition as on a card, so nothing in it is special to the ROM but
  `HFS_RO_DISK`.
* **`/rom`** is a mount of the HydraFS server with the spec `x` (`mount -s hfs /rom x`), in the system
  namespace, which the boot shell makes; every task sees it.  The IO layer's built-in
  `/rom` prefix, the `/rom` server (`servers/rom_srv.s`, BIOS ROM page D), its gate and its image format
  (`HYROM1`, `mkromfs.js`) are gone.
* **The build** (`sim/tools/mkromdisk.js`, after the link): the volume made with the HydraFS PC tool from the
  manifest (`os_rom/romfs.txt`), stamped 2000-01-01 so each build is the same; the table and the volume's blocks
  written into the image in the chips' order (the A13 half-swap, the V1 board's bank-bit swaps); then the image
  read back as the CPU sees it and every file checked ([below](#pointers-and-banks-what-cant-go-wrong)).
* **One disk, not a disk per chip.**  The table is in bank 0 for all 4 MB.  A chip can't be burned on its own
  with a volume of its own (a cartridge), but the server needs one partition per disk, there's no union to
  build, and a file can be any size.  A slot card with a ROM of its own could be another disk, with a letter of its own.

---

### **Pointers and banks: what can't go wrong**

A file on the ROM disk can lie across banks (`jukebox.hyx` is in banks 4 and 5 now).  None of these can break it:

1. **A block never crosses a bank, or the board's 8K halves.**  512 divides `$2000`, and a block's bank and
   offset come from its number alone (asserted in `sd.s`).  So a read selects one bank and copies from one place.
2. **No CPU pointer walks through a file.**  Every read goes through HydraFS, a block at a time into the block
   cache, so a file's blocks can be in any banks, in any order, and its extents decide where.  Nothing assumes
   bank b + 1 follows bank b.
3. **The CPU always sees logical banks.**  The V1 board's swaps (bank bits 2 and 3, 6 and 7, and the A13 halves)
   are in the hardware; only the image writer (`mkromdisk.js`, `romsum.js`) and the emulator (`romBank` in
   `sim/lib/machine.js`) know about them.  So code that selects bank n gets the build's bank n.
4. **The build checks it.**  `mkromdisk.js` rebuilds the disk from the finished image through the emulator's own
   bank mapping, opens it as a HydraFS volume, and compares every file with its source byte for byte: a mismatch
   fails the build.  It prints how many files lie across banks (`--list`: which).
5. **The machine checks it.**  The `rom-copy` test copies every file in `/rom` to a card on the emulated machine
   (through `SD_ROM_READ`) and compares the card's copies with the sources.
6. **Code and data used by address stay in the system banks,** linked by ld65 into a memory area per bank half,
   so a segment can't spill into the next bank (the link fails).  The one place that walked a pointer from bank
   to bank was the song player reading the ROM's test song (`ZSM_BYTE` incremented the ROM bank past `$DFFF`);
   it's gone: `sndtest` plays `/rom/songs/test.zsm` as a file ([below](#what-the-disks-simplify)).
7. **A task's banks are its own.**  The bank registers are in each task's zero page; the storage task saves and
   restores its own around a read.  A new task now starts with banks 0 (`RESERVE_TASK`): before, it kept its
   slot's last banks, and a shell started where the song player had been ran bank 2's song data as HyForth's
   `COPYTORAM` (found by the `irqs-off` test when bank 0's layout moved).
8. **The hardware test's bank lines** compare bank 0's first page with bank 2^n's: the table's block 0 starts with
   a line of text, so bank 0's first page can't be all zeros like an empty block elsewhere.

---

### **RAM disks**

**The ROM starts both** at boot (`RAMD_BOOT`, `servers/ramdisk.s`), with the sizes in `os_rom/include/io.inc`,
and a write to a ctl file stops one and starts it again with another size, from other memory (`boot.hys` can do it):

```
echo start 256K 1-2 > /dev/sd/r/ctl        the RAM disk: 256K, from RAM modules 1 and 2 (the areas: /ram)
echo start 1M $20-$9F > /dev/sd/s/ctl      the shared RAM disk: 1 MB, from shared bank IDs $20-$9F (/sram)
echo stop > /dev/sd/s/ctl                  stop one (its files go; not while one is open)
cat /dev/sd/r/ctl                          its size, its banks, its HydraFS: "ram 256 KB 512 blocks", "banks $10-$2F"
```

* **The defaults:** the RAM disk 256K (`RAMD_BANKS`) of the storage task's banks, from any module; the shared
  one 512K (`SRAMD_BANKS`) from the lower half of the shared banks (`$01-$7F`).  On a machine with less memory,
  half, and half again, rather than none.
* **`start SIZE FROM-TO` on `r`:** the modules the RAM disk's banks come from.  A task's banks on the memory
  cards can only be seen while `T` is that task (each task has its own bank numbers), and the HydraFS server runs
  in the storage task, so this disk is in the storage task's own banks: its slice of each module, 128K of a
  2 MB module, taken with `MM_BANK_ALLOC_IN` (a run of banks from a range, with no handle).  The areas share it:
  there's no limit for one area but the disk's size.
* **`start SIZE FROM-TO` on `s`:** the shared bank IDs it comes from (`U << 4 | bank`, as `memory.md` numbers
  them: `$20-$9F` is macro-pages 2-9, 1 MB), taken with `SH_BANK_ALLOC` (a run, with no handle: nothing else
  hands them out, and `stop` gives them back).  The system's own banks (`$00`, the IO transfer banks `$09-$0C`) and
  banks in use are never taken.  Why the lower half by default: `SH_ALLOC` takes from the top down with IRQs
  off, looking past every bank in use above a free one, and 64 of them there made a shared allocation hold IRQs
  off too long (the `irqs-off` test).
* **Sizes** take `K` and `M`; without one, they're 8K banks (1-255).  Numbers are decimal, or hex after a `$`.

---

### **Who can use which area**

The Hydra has no users, so the identity a server can check is the task asking: every request carries it
(`IO_BLK_CLIENT`), and the kernel knows which task started which (the owner chain, `ZP_TASK_OWNER`).  The HydraFS
server checks it on the RAM disk (`HFS_AREA_CHECK`), before a walk and before a create cuts its name:

| Area | Who | What |
| :--- | :-- | :--- |
| Area `r/N` | Task N, and the tasks it started (and theirs: its family) | Read and write |
| Area `r/N` | Any other task | Nothing: `ERR_IO_PERM` (`$88`, "not allowed") |
| The RAM disk's other names (`r/zz`) | Every task but 0 | Nothing: not an area |
| The RAM disk's root (its listing), `/sram` and all in it | Every task | Read and write |
| `/rom/...` | Every task | Read only (`ERR_IO_MODE` for the rest) |
| Everything | Task 0, the system task | Read and write (the ROM disk still read-only) |

* **A child has its parent's area:** a program started from the shell reads and writes the shell's area (and its
  cache), and so do the tasks it starts.  Two shells' families can't see into each other's areas.
* **Task 0 passes every check.**  It's the system's own task (the boot, the kernel's work for other tasks), so a
  request from it is never refused for permission, here or (planned) in `/proc` ([PROC.md](PROC.md)).  Only code
  the system runs in task 0 has that: a program never runs there.
* **An area itself** can't be renamed, or its mode changed, but by task 0 (`HFS_AREA_WSTAT`): renaming `r/1` to
  `r/2` would take task 2's.
* **Owners kept right:** when a task ends, the tasks it started get its owner instead (`TASK_ORPHANS`, page 5, from
  `TASK_EXIT_NOTED`), as Unix gives orphans to `init`, so a new task in its slot isn't taken for their parent.
* **One check for both:** "may task A use task N's things?" is the kernel's `TASK_MAY` (page 5: `.A` = A, `.X` = N),
  which the HydraFS server calls through a gate.  `.Y` = 0 is the areas' way (A is N, or a task N started);
  `.Y` = 1 the other way, for the `/proc` server (A started N: a debugger and its program,
  [PROC.md](PROC.md#who-can-use-it)).  Task 0 is always yes.
* **An open file stays usable** by whoever has the fd, as in Unix and Plan 9.
* **Any mount of the disk** (`mount hfs /a r`, then `/a/2`) is checked the same: the server checks the area.
* **Permissions are the server's,** not the namespace's (Plan 9's way): a bind or a mount can rename an area, but
  the server still checks who's asking.  The namespace can add hiding, for any device ([NAMESPACES.md](NAMESPACES.md)).

---

### **How the RAM disks work**

**In the storage driver** (BIOS ROM page 3, with `/dev/sd` and the block cache), as the ROM disk is:
* `SD_READ_BLOCK` and `SD_WRITE_BLOCK` for `r` and `s` (`SD_STATE_RAM`, `SD_STATE_SRAM`) map the block's bank at
  `$8000` (`SD_RAM_MAP`: the storage task's own `$00` for `r`; `U` and `$00` for a shared bank ID), copy 512
  bytes between it and the block cache (in task RAM), and put the bank and `U` back.  Block n is bank n / 16 of
  the disk's run, from its first (`RAMD_FIRST`), at `$8000 + (n % 16) * 512`.
* **Starting one** (`RAMD_START`) takes the banks, writes zeros to block 0 (so RAM from before, or from power-up,
  can't look like a partition table), then makes a HydraFS on it as `format` does (a quick one, labelled `RAM` or
  `SRAM`).  **Stopping one** (`RAMD_STOP`) gives the banks back, and is refused while a file on it is open.
* `DISK_FROM_NAME` (`include/io.inc`) knows `r` and `s` (disks 9 and 10).

**In the HydraFS server** (page 6):
* (`/r/s` was the shared disk too, so that one bind gave both; with each shell's own area at `/ram`, the shared
  disk has a bind of its own, `/sram`, and that's gone.)
* the areas' check (`HFS_AREA_CHECK`, `HFS_AREA_WSTAT`: `ERR_IO_PERM`);
* **an area removed as its task ends:** the task-end path (`TASK_EXIT_NOTED`, after `MM_TASK_RESET` has closed the
  task's fds) calls `TASK_ORPHANS` (page 5), which gives the task's children its owner, then looks at the storage
  task's `RAMD_AREAS` (a bit for each area that may be there, set by `HFS_AREA_CHECK` when one is used) and, if
  the task's bit is set, runs `HFS_AREA_END` in the storage task with `TASK_CALL` (no fd, no request: the storage
  task is between requests then).  That walks a path of its own (`RAMD_AREA_PATH`, 256 bytes) from the area down
  to the first file or empty directory, removes it (`HFS_REMOVE_AT`, the remove request's own), and starts again,
  until the area itself is gone and its bit cleared.  A file still open (a task it started may have one) stops
  it, and the bit stays for the next time; a parent waiting for the task (`TASK_JOIN`) goes on only after it.

**Names:** the boot shell mounts `/sram` (`mount -s hfs /sram s`: `SH_MOUNTS`) and makes the shared caches'
directories, `/sram/bin` and `/sram/lib` (`SH_RAM_DIRS`).  Each shell mounts its own area at `/ram` (`mount hfs
/ram r/N`, in its own namespace) and makes it, with `bin` and `lib` (`SH_OWN_AREA`: the boot shell's from `SH_BOOT`,
one started with `shell` from `SHELL_MAIN`); the programs, scripts and pipelines it runs inherit that.
With no memory modules there's no RAM disk (its banks are the modules'), so each shell's area is on the shared
one instead: `s/ram/N`, in `/sram/ram` (`mount hfs /ram s/ram/N`), the same at `/ram`, but kept when the shell
ends, and open to every task, as `/sram` is.

**Speed.**  A block from RAM or ROM is a 512-byte copy, about 7,000 cycles; from a card it's the bit-banged SPI,
about 150,000 (the `sd-speed` test's budget).  The rest of a load is the same either way, and it's most of the
RAM disk's time.  Profiled (`sim/hydrasim.js --profile N-M`), a 16K program from the shared RAM disk takes 1.34
million cycles, against 4.69 million from a card (3.5 times as fast: `ram-speed`):
* **Three copies of every byte, 55%:** from the disk's bank to the block cache (`SD_RAM_READ`), to the client's
  transfer area (`HFS_FILE_READ`), and to the program (`IO_COPY_OUT`), each a page four bytes a turn of the loop
  (`_M_COPY_PAGE`: 13.75 cycles a byte).  The RAM disk's bank and the transfer area are both seen at `$8000`, so
  the cache's copy can't be skipped without switching banks for every byte, which costs about as much.
* **Each request, 31%:** the file is read 256 bytes at a time (`IO_UNIT`), 64 requests, each with its task calls,
  gates and scheduling, HydraFS's place in the file (`HFS_EXT_TRY`, `HFS_FILE_BLOCK`) and the IO layer's
  bookkeeping: about 6,000 cycles a request.  Bigger requests would halve it.
* **Starting the program, 14%:** the shell's word lookup and name search, the new task (its namespace: only the
  entries in use are copied), the MMU.

The ROM disk's blocks are read where they are: the paged ROM is at `$A000`, apart from the transfer area, so
HydraFS copies a file's bytes from it straight into the transfer area (`HFS_BLOCK_AT`), with no copy into the
cache (a ROM block never changes), about 16 cycles a byte less.  A program found by name in a cache pays for the
places looked in first (the current directory, on the card).

---

### **Booting: finding the disks**

1. **The storage driver starts the disks it can:** the ROM disk needs no start (`SD_ROM_INIT`: its state and
   size); the RAM disks are started with the defaults and formatted (done); the cards are started at their first
   open, as now.
2. **The boot shell finds the volumes:** `/sd` mounted, then each disk's partition table read (HydraFS finds its
   partition, as on a card).  `/rom` is bound if the ROM disk has a volume (an image without one, or a blank
   paged ROM, has no table: no `/rom`, and everything else works).  *Done:* the boot shell binds a name only if
   what it stands for opens (`SH_BINDS`; the `rom-none` test).
3. **(Done)** **The namespace from a file:** the boot shell reads `/rom/lib/namespace`, the default list of binds and mounts
   (`/ram`, `/bin` and `/lib` unions), then a card's `/lib/namespace` ([NAMESPACES.md](NAMESPACES.md)).  The list
   is a file in ROM, not code: changing it is a rebuild of the ROM disk, not of a BIOS page.
4. **The current directory:** the first card with a HydraFS, as now; with no card, the shell's own area,
   `/ram`, so files can be saved (until reset) on a machine with no card at all.  *Done* (`SH_VOLUMES`).
5. **The boot script:** a card's `boot.hys`; with no card, `/rom/boot.hys` (now a line saying so; later it can
   copy the ROM's programs into `/sram/bin`, set `PATH`, print help).  *Done:* `SH_VOLUMES` says which
   (`BOOTFLAG` 1 or 2).

---

### **What the disks simplify**

Every piece of the ROM that isn't code can be a file, and every place that searches can be a bind:

| Now (or before) | With the disks | Status |
| :-------------- | :------------- | :----- |
| `/rom`: its own server (page D), image format and PC tool, an IO-layer prefix, a gate | A HydraFS volume read by the storage driver; a bind | **Done.**  Page D is free (7.5K) |
| The test song in bank 2, and the player's ROM mode (`ZSM_PLAY_ROM`, `SND_SONG_BANK`, the bank walk in `ZSM_BYTE`, `songs/test_rom.s`) | `sndtest` plays `/rom/songs/test.zsm` (already on the disk) as `play` does any file | **Done:** bank 2 is the volume's, and the only cross-bank pointer is gone |
| HyForth's training scripts and sample binary words in bank 0 (`ftrain`, words compiled at `COPYSTART` offsets) | Files in `/rom/forth`, read with `include` | Planned: bank 0 keeps only `COPYTORAM` and the variables |
| New HyForth words: assembly on page 1 (196 bytes free) or page A (530) | Forth source libraries in `/rom/lib` (`lib name`, which reads files already) | From now on, where speed allows |
| The shell's search: the current directory, `$PATH`, the card's `/bin`, then `/rom/bin` (and `$LIBPATH`, `/lib`, `/rom/lib`) | `.` then `/bin`, a union of the caches, the card and the ROM ([NAMESPACES.md](NAMESPACES.md)) | **Done** |
| A built-in default namespace (code in the boot shell) | `/rom/lib/namespace`, a file | **Done** (the mounts and the card's binds stay the boot shell's) |
| No card: nowhere to save | `/ram` (the shell's area) as the current directory | **Done** |
| Slow program loads from a card, and no resident programs | The caches: a program copied to RAM once | **Done** |
| A task's memory: summaries in `/proc/N/pages`; far pointers refuse other tasks' RAM | `/proc/N/mem`, the bytes, for the family and task 0 ([PROC.md](PROC.md)) | Planned |
| ROM programs on BIOS pages (the editor, page 8; the song player, page C) | Could be `.hyx` files in `/rom/bin`, loaded into task RAM | An option: BIOS pages aren't short, but files can change without a BIOS rebuild |
| Two PC tools for images (`hydrafs.js`, `mkromfs.js`) | One, `hydrafs.js`, for cards and the ROM | **Done** |

What stays in system banks: what has to run without the OS (the hardware test, bank 1), and what HyForth copies by
address before any IO is up (`COPYTORAM` and its variables, bank 0).

---

### **The program caches**

Two caches: the boot shell's own (`/ram/bin`, its area's, and its children's, since they share its namespace)
and the shared one (`/sram/bin`).  For a program typed with no `/`, the shell looks in the current directory
(Plan 9's `.` first: `path=(. /bin)`), then `/bin` (`SH_FIND`, `shell/run.s`), a union in the namespace
([NAMESPACES.md](NAMESPACES.md)) of:
1. the shell's cache, `/ram/bin` (`-c`: a copy into `/bin` goes there);
2. the shared cache, `/sram/bin`;
3. the boot card's `/bin`;
4. `/rom/bin`;

then `$PATH`'s directories.  Libraries the same, in `/lib`, then `$LIBPATH`.  A name with a `/` is used as
given, so `/sd/0/bin/game` always loads the card's copy.

* **Caching is copying:** `cp game.hyx /sram/bin/game.hyx`, and `rm` takes it out.  (The `cache` and `uncache`
  commands this plan had are left out: a copy says the same, and HyForth's page 1 has under 200 bytes.)
* **Not checked against the card:** after rebuilding a program, copy it again (or remove it), or the old copy
  keeps running.  (A program in the current directory runs before a cached copy, so the new one runs from the
  build's folder.)
* `boot.hys` can copy the programs used most into `/sram/bin`, for every task.

---

### **Tests**

In the emulator:
* **The ROM disk (done):** `/rom` listed, read, a program run by name from `/rom/bin`, read-only everywhere
  (`rom`); every file copied to a card and compared with its source (`rom-copy`); the build's read-back check;
  `/dev/sd/x/ctl` (`rom 4 MB 8192 blocks`), the ROM disk not under `/sd`, `ns` showing the mount, `/sd/8` no disk
  (`rom`).
* **The RAM disks (done):** the defaults at boot, their ctl files, `/ram` bound, the caches' directories, a file
  bigger than a bank through each and back to a card, compared (`ram-disks`); `stop` (busy with a file open) and
  `start` with sizes and ranges, busy, no room, bad text, the ROM disk refused (`ram-ctl`); less on a small machine
  (`ram-small`).
* **The areas and caches (done):** the shell's area, a pipeline's stage in it, another task's area and a name
  that isn't one refused, programs by name from both caches and from a stage, the current directory first
  (`ram-areas`).
* **An area removed as its task ends (done):** a script, a task of its own, makes an area with directories in
  directories and files and leaves one open; when it ends, the area is gone and the space back (`ram-area-end`).
* **Task 0's requests (done):** an area's removal runs as task 0 (`SD_CLIENT` = 0) and walks into another task's
  area, so `ram-area-end` passes only if task 0 passes the check.
* **An area kept while a task its task started has a file in it open (done):** a script starts `upper` in the
  background, writing into the script's area; the area stays when the script ends, and goes at the next end of a
  task in that slot (`ram-area-busy`).
* **The speed (done):** a 16K program by its full path from the card, then from `/sram/bin`: at least 2.5 times
  as fast (`ram-speed`).
* **The boot (done):** no `/rom` bind, and no `/rom/boot.hys`, with a paged ROM image that has no volume
  (`rom-none`); `/rom/boot.hys` run with no card (`rom`), not with one (`ram-disks`); `/ram` as the current
  directory with no card (every test without a card: the prompt `/ram> `).
* **The test song as a file (done):** `sndtest` plays it from the ROM disk (`sound`).

---

### **Steps**

1. **The ROM disk.**  *Done:* the disk `x` (disk 8) in the storage driver, read-only in the driver, `/dev/sd` and
   HydraFS; `/rom` as a bind; `mkromdisk.js` with its read-back check; the `rom-copy` test; the old server and
   format gone; new tasks starting on bank 0.
2. **The RAM disks.**  *Done:* the memory manager's `MM_BANK_ALLOC_IN` and `SH_BANK_ALLOC`/`SH_BANK_FREE`; `r` and
   `s` (disks 9 and 10) in the storage driver, `start` and `stop`, the defaults at boot; `/ram` (and `/sram`
   through HydraFS); the areas' check and `ERR_IO_PERM`; `TASK_ORPHANS`; the caches in the shell's search and their
   directories; the tests; the docs.
3. **An area removed when its task ends.**  *Done:* `TASK_AREA_END`, `HFS_AREA_END`, `RAMD_AREAS`.
4. **The test song as a file.**  *Done:* `sndtest` plays `/rom/songs/test.zsm` (`ZSM_PLAY_TEST`); the player's ROM
   mode and bank 2 are gone, and the volume starts at bank 2.
5. **The boot.**  *Done:* `/rom` (and `/ram`) bound only when there; `/rom/boot.hys`; with no card, `/ram` as the
   current directory.
6. **`TASK_MAY` as a kernel routine.**  *Done* (page 5), with the way `/proc` will need.
7. Cards on SPI devices `$8`-`$F`, with the slot map, whenever a slot card has one.

The [namespace plan](NAMESPACES.md) then turns the search and the default names into binds, and the
[/proc plan](PROC.md) uses `TASK_MAY` for task memory.
