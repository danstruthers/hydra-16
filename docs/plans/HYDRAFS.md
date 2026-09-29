## **HydraFS: the Hydra's SD card filesystem**

A small filesystem for SD cards, designed for the 65C02 and the Hydra's Plan 9-style IO layer (see `IO_PLAN.md`): directories, files with a Plan 9-style qid, text and binary directory listings.  It replaces FAT32 on the Hydra's cards.  **All five build steps are done**: the SD layer's cards and sizes, the PC tool, and the server reading, writing and checking, on ROM page 6 (`os_rom/io/hfs_srv.s`, `hfs_write.s`, `hfs_check.s`), with HyForth words.

### **Goals and trade-offs**
* Easy for a 65C02: fixed-size directory entries, contiguous runs of clusters (extents) instead of a FAT chain, 32-bit arithmetic at most.
* Plan 9-like: every file and directory is reached by walking a path; stat records; a qid (unique id + version) that survives renames; create, remove, and rename by wstat; no hard links.
* Simple and robust enough: writes go in a safe order (data, then the free map, then the directory entry), but there's no journal.
* **Not readable on a PC.**  Cards are made and read with a host tool (`sim/tools/hydrafs.js`), as disk images; a disk imager writes an image to a real card.  The same images work in the emulator (`--sd`).

### **Names**
| Name | What it is |
| :--- | :--------- |
| `/dev/sd/0/data` ... `/dev/sd/7/data` | Card 0-7 (SPI device 0-7) as raw bytes, at the fd's offset (the first 4 GB) |
| `/dev/sd/0/ctl` ... `/dev/sd/7/ctl` | Read: the card's details, as text (e.g. `sdhc 7580 MB hydrafs label=GAMES free=6100 MB`).  Write: text commands: `init` (start the card again, e.g. after changing it), `format [label]`, `label <text>` |
| `/sd/0/...` ... `/sd/7/...` | The files on card 0-7: the HydraFS server (device `hfs`), mounted at `/sd` in each task's namespace (the shell mounts it at startup; the tasks it starts inherit it).  `/sd/0` is card 0's root directory |

Both servers run in the storage task (`$C`), which owns the SPI bus and the block cache; the filesystem uses the block layer directly, not `/dev/sd/N/data`.  The whole card is the filesystem: **no partitions** in this version.

* Names: 1-31 characters, **case-sensitive**, any byte except `/` and 0.  `.` and `..` are understood when walking a path (the server keeps each open file's path), but aren't stored.

### **On the card**
Blocks are the card's 512-byte sectors.  Space is allocated in **clusters of 8 blocks (4 KB)**; cluster 0 is the first block of the data area.  Numbers are little-endian.

| Blocks | Contents |
| :----- | :------- |
| 0 | The superblock |
| 1 ... | The free map: 1 bit per cluster (1 = in use), 4096 clusters per block (16 MB of card per block; a 32 GB card has 2048 map blocks) |
| after the map | The data area: clusters of file and directory data, and extent blocks |

#### **The superblock** (block 0)
| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 8 | Magic: `HYDRAFS1` |
| 8 | 1 | Version: 1 |
| 9 | 1 | Cluster size, as a shift: 3 (8 blocks) |
| 10 | 2 | Reserved (0) |
| 12 | 4 | Clusters in the data area |
| 16 | 4 | The free map's first block (1) |
| 20 | 4 | Its size in blocks |
| 24 | 4 | The data area's first block |
| 28 | 4 | Free clusters (a hint: `check` recounts it) |
| 32 | 4 | Where to look for free clusters next (a hint) |
| 36 | 4 | The next qid id to give out |
| 40 | 4 | The next modification stamp (a counter until the Hydra has a clock) |
| 44 | 20 | Reserved (0) |
| 64 | 64 | The root directory's entry (below) |
| 128 | 32 | The volume label, zero-terminated (31 characters at most) |
| 160 | 352 | Reserved (0) |

#### **Directory entries** (64 bytes)
A directory is a file whose data is an array of 64-byte entries (8 per block).  The entry *is* the file's metadata: there are no separate inodes.  A free entry has a 0 first name byte; new entries fill free ones first, and a directory only grows when it's full.

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 32 | Name, zero-terminated (0 first byte: a free entry) |
| 32 | 1 | Mode: bit 7 = directory, bit 6 = append-only, bit 0 = read-only |
| 33 | 1 | Reserved (0) |
| 34 | 2 | Qid version: goes up by 1 at every change (write, truncate, rename) |
| 36 | 4 | Qid id: unique on the card, never reused, the same across renames |
| 40 | 4 | Size in bytes (up to 4 GB) |
| 44 | 4 | Modification stamp |
| 48 | 6 | Extent 1: first cluster (4), clusters (2) |
| 54 | 6 | Extent 2: first cluster (4), clusters (2) |
| 60 | 4 | The first extent block, for more extents (a block number; 0 = none) |

* An **extent** is a run of contiguous clusters: up to 65535 clusters (256 MB).  A file's data is its extents in order.  Most files need one or two; a fragmented file's further extents go in a chain of extent blocks.
* The allocator tries to grow a file's last extent (the next cluster free?) before starting a new one, so files stay contiguous.

#### **Extent blocks** (512 bytes)
| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 4 | The next extent block (0 = the last) |
| 4 | 2 | Extents used in this block (0-84) |
| 6 | 2 | Reserved (0) |
| 8 | 504 | 84 extents of 6 bytes (first cluster, clusters) |

### **Stat records** (48 bytes)
What `IO_STAT` returns, and what a binary directory read returns per entry: the directory entry without its extents.

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 32 | Name, zero-terminated |
| 32 | 1 | Mode (as the entry) |
| 33 | 1 | Card (0-7) |
| 34 | 2 | Qid version |
| 36 | 4 | Qid id |
| 40 | 4 | Size |
| 44 | 4 | Modification stamp |

(`IO_STAT`'s block grows from 16 to 48 bytes; `/dev/null` and friends return all zeros, as now.)

### **Reading a directory**
Open it for reading, then read:
* **Text** (the default): a line per entry, `name size` (a `/` after a directory's name, and no size), then CR LF.  E.g. `games/` and `star.frt 1234`.  Good from HyForth: `q^/sd/0^ 1 open` then `cat`.
* **Binary**: open with `IO_MODE_STAT` (new mode bit): each read returns whole 48-byte stat records.

### **Calls and requests**
Existing calls work as they do on devices: `IO_OPEN` walks the path (`IO_MODE_TRUNC`, new, empties a file as it's opened), `IO_READ`, `IO_WRITE` (writing past the end grows the file), `IO_SEEK`, `IO_CLOSE`, `IO_STAT`.  New:

| Call | H9P request | Does |
| :--- | :---------- | :--- |
| `IO_CREATE` (.A.Y = name, .X = open mode, `ZP_IO_BUF` = the new file's mode: directory, append-only, read-only) | `H9_CREATE` | Creates a file or directory in an existing directory, and opens it.  A file that's there already is emptied, as in Plan 9 |
| `IO_REMOVE` (.A.Y = name) | `H9_REMOVE` | Removes a file, or an empty directory |
| `IO_WSTAT` (.A = fd, `ZP_IO_BUF` = a stat record) | `H9_WSTAT` | Renames (within its directory) and sets the mode bits; other fields are left alone |

HyForth words: `create ( sz mode -- fd )`, `remove ( sz -- )`, `rename ( sz-old sz-new -- )`, `ls ( sz -- )` (a directory's text listing), `mkdir ( sz -- )`.

The filesystem's requests are numbered from `H9_CREATE` (8) on, and every other server refuses them (`ERR_IO_BAD_REQ`), so `IO_REMOVE` of `/dev/null`, say, is an error, not a no-op.  `IO_REMOVE` borrows a free fd for the request and gives it back.

**How writing behaves** (as built):
* **Mode bits are checked when a file is opened**, as in Plan 9: opening a read-only file for writing (or with `IO_MODE_TRUNC`) is `ERR_IO_MODE`, and so is opening a directory for writing.  The fd that creates a file can write it even if its mode is read-only, and a file made read-only while it's open for writing can still be written through that fd.
* **An append-only file** is written at its end, wherever the fd's offset is.
* **A write can't start past the end of a file** (`ERR_IO_BAD_REQ`): no holes.  Sparse files are for later.
* **Creating a name that's there**: a file, made again as a file, is emptied and opened; a directory, or a file where a directory was asked for, is `ERR_IO_EXISTS`.  A new directory is opened for reading, whatever the open mode says.
* **Removing** needs the file not to be open (`ERR_IO_BUSY`) and a directory to be empty (`ERR_IO_NOT_EMPTY`); a card's root can't be removed.
* **Renaming** is within the file's directory (the name is a name, not a path); a name that's taken is `ERR_IO_EXISTS`.  A 0 first name byte keeps the name, and a mode of `$FF` keeps the mode bits.
* **A full card** is `ERR_IO_FULL` (`$81`); what was written up to it stays.
* **Qid versions and stamps** change once per open for writing (at the first write), and at a truncate, create, remove (the directory's) or wstat.

**What reaches the card when**, in an order that's safe without a journal:
* A file's data goes to the card at once, a block at a time (a 256-byte write is a whole block write: about 1,000 CPU cycles a byte, 3.5 KB/s).  A block that's entirely past the end of the file isn't read first.
* When a file gets a cluster: the free map (the cluster marked in use), then the extent block if the new extent goes in one, then the entry (with the file's size so far).
* Its size otherwise changes in RAM, in every open fd's copy of the entry, and goes to the card when the last fd on it is closed.  So after a crash, a file being written can be shorter than what was written, but its clusters are never lost or in use twice.
* Emptying or removing a file writes its entry first (no extents), then frees its clusters: a crash between leaves lost clusters (`check` will find them), never a cluster in use twice.
* A new entry is written before its directory's entry (the directory's size, when the entry is at its end), so a crash never leaves a half-made entry in a directory.
* The superblock's counters (free clusters, the hint, the next qid and stamp) are kept in RAM and written at the end of each request that changes them; the next qid before the entry that uses it.

### **The storage task's state**
* The one-block cache (512 bytes, shared with `/dev/sd`), for files' data and for reading directories.  A second, the **metadata buffer** (512 bytes), for the free map, extent blocks, entries being written and the superblock, so a file's data and its allocation don't evict each other.  A block is changed there and written back when another block is wanted, and at the end of every request, which also forgets it: nothing is kept in it between requests, so a raw `/dev/sd` write or a changed card can't leave it stale.
* Per card (8): its state, and the superblock's numbers and counters (`HFS_V_*`, 33 bytes each, as parallel arrays).  `init` on a card's ctl file forgets them, and lets go of any HydraFS file open on it.
* Open files: 8, shared by every task.  Each keeps a 16-byte header (the card, the open mode, how many fds share it, where its directory entry is, and where its directory's entry is, for a rename) and a **copy of the entry itself** (64 bytes), so a read finds the file's extents without an extra block read.  Every copy of one entry is kept the same, and a walk takes an open file's copy over the card's (its size may not be on the card yet).  Reads walk the extent list from the start each time: that's arithmetic alone for the two extents in the entry, and costs a block read per chunk only for a file fragmented into three or more.
* The allocator takes the cluster after a file's last if it's free (so files stay in one piece), else scans the free map from the hint, a byte (8 clusters) at a time past full ones.
* A walk keeps the entry it's on (`HFS_ENT`, 64 bytes) and a stack of the directories above it (`HFS_STK`, 8 levels of 5 bytes), so `..` needs no stored path.  A path deeper than that gives `ERR_IO_NAME`.

### **Formatting and tools**
* `format [label]` on `/dev/sd/N/ctl` makes an empty HydraFS: from the card's size (its CSD register: the SD layer gets CMD9, and the emulator's card too) it clears block 0, writes the free map, all free, then the superblock (with an empty root directory: size 0, no clusters yet), so a format that's cut short leaves no half-made HydraFS.  The layout is the one `hydrafs.js mkfs` makes.  A 32 GB card's map is 2048 blocks: about a minute and a half at 3.58 MHz.  `label <text>` sets the label (31 characters at most).
* `sim/tools/hydrafs.js` (Node, on the PC): `mkfs <image> <MB> [label]`, `ls <image> [path]`, `put <image> <file> <path>`, `get <image> <path> <file>`, `mkdir`, `rm`, and `import <image> <folder>` (a whole folder tree), for making test cards and moving files to and from the Hydra.

### **Build order**
1. **(Done)** The SD layer: CMD9 (the card's size); per-card state for devices 0-7 (`/dev/sd/N/data`, `/dev/sd/N/ctl`).  The emulator: CMD9 (and `--sdsc N`).
2. **(Done)** `sim/tools/hydrafs.js` (mkfs, info, ls, put, get, mkdir, rm, import, check): test images first, from the PC side.  (A new directory entry goes in the first free one, else at the end; a file grows its last extent when the next cluster is free; extent blocks take a cluster each, their first block.)
3. **(Done)** Read-only HydraFS (`os_rom/io/hfs_srv.s`, ROM page 3, the `hfs` device, mounted at `/sd` by the shell): walk (with `.` and `..`), open, read, both directory formats, stat.  `IO_STAT`'s record grew from 16 to 48 bytes and `IO_MODE_STAT` was added; the `hydrafs` regression test covers it.  A card that holds no HydraFS gives the new `ERR_IO_NOT_FS` (`$80`).
4. **(Done)** Writing (`os_rom/io/hfs_write.s`): write and grow, `IO_CREATE`, `IO_REMOVE`, `IO_WSTAT`, `IO_MODE_TRUNC`; `format` and `label`.  The server moved to its own ROM page (6): page 3 was full.  New errors `ERR_IO_FULL` (`$81`), `ERR_IO_EXISTS` (`$82`), `ERR_IO_NOT_EMPTY` (`$83`), `ERR_IO_BUSY` (`$84`); new thunks `$F8C9`-`$F8CF`.  The `hydrafs-write` regression test covers it, and checks the cards afterwards with the PC tool.
5. **(Done)** HyForth words (`ls`, `create`, `mkdir`, `remove`, `rename`, written with step 4 to test it, and `ctl`: a command to a ctl file); `check` on the ctl file (`os_rom/io/hfs_check.s`), and the card's HydraFS details in its ctl file's text.  The `hydrafs-check` regression test covers it.

### **The check**
`check` on `/dev/sd/N/ctl` walks every directory from the root, marks in a bitmap every cluster that a file, a directory or an extent block uses, and compares that with the free map:
* **lost**: marked in use, but nothing uses them (harmless, but the space is wasted; a crash while a file was being emptied or removed leaves these);
* **unmarked**: in use, but marked free (dangerous: a new file could be given them);
* **twice**: used by two files, or twice by one (one of them is damaged).

It recounts the free clusters into the superblock.  `check fix` also makes the free map say what the files use: lost clusters are freed and unmarked ones marked.  A cluster used twice needs a person to decide which file keeps it, so it's only reported (the PC tool's `check` names the files).

Reading the ctl file then shows the results, after the card's label and free space:
```
sdhc 1 MB 2048 blocks
hydrafs label=GAMES
free 1012 KB of 1020 KB
check: lost 0, unmarked 0, twice 0
```
(`, fixed` after `check fix`; the check line after a check of that card, until another card is checked.)

How: the bitmap is 8 KB, for 65,536 clusters (256 MB of card) at a time, so a bigger card takes a pass for each 256 MB, each walking the directories again: a 32 GB card takes 128.  The walk keeps a copy of each directory's entry it's in, and where it is in it, after the bitmap, and follows directories 24 deep (deeper: `ERR_IO_NAME`, and no results).  The buffer (about 9.7 KB of the storage task's RAM) is taken from the MMU at the first check, and kept.  Clusters a file claims past the card's end aren't counted (the PC tool's `check` reports them).

### **Later**
Partitions (a HydraFS partition next to a small FAT one, for a PC), a real clock for the stamps, fsck-style repair, sparse files.
