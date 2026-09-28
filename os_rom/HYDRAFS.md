## **HydraFS: the Hydra's SD card filesystem**

A small filesystem for SD cards, designed for the 65C02 and the Hydra's Plan 9-style IO layer (see `IO_PLAN.md`): directories, files with a Plan 9-style qid, text and binary directory listings.  It replaces FAT32 on the Hydra's cards.  **Draft for review: nothing is built yet.**

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

### **The storage task's state**
* The one-block cache (512 bytes, as now), plus a second block buffer for the free map and extent blocks (so a file's data and its allocation don't evict each other).
* Per card (8): its type and state, and the superblock's numbers the server uses (about 24 bytes each).
* Open files: 8, about 32 bytes each: the card, where its directory entry is (block, index), its mode, size and qid, the extent the offset is in (its file position and first cluster), and the path depth (for `..`).

### **Formatting and tools**
* `format [label]` on `/dev/sd/N/ctl` makes an empty HydraFS: it reads the card's size (its CSD register: the SD layer gets CMD9, and the emulator's card too), writes the superblock (with an empty root directory: size 0, no clusters yet) and the free map, all free (the superblock and the map come before the data area, so they aren't in it).
* `sim/tools/hydrafs.js` (Node, on the PC): `mkfs <image> <MB> [label]`, `ls <image> [path]`, `put <image> <file> <path>`, `get <image> <path> <file>`, `mkdir`, `rm`, and `import <image> <folder>` (a whole folder tree), for making test cards and moving files to and from the Hydra.

### **Build order**
1. The SD layer: CMD9 (the card's size); per-card state for devices 0-7 (`/dev/sd/N/data`, `/dev/sd/N/ctl`).  The emulator: CMD9.
2. `sim/tools/hydrafs.js` (mkfs, put, ls, get): test images first, from the PC side.
3. Read-only HydraFS: mount at `/sd`, walk, open, read, both directory formats, stat.
4. Writing: write and grow, `IO_CREATE`, `IO_REMOVE`, `IO_WSTAT`, `IO_MODE_TRUNC`; `format`.
5. HyForth words; `check` on the ctl file (recount the free map, find lost clusters).

### **Later**
Partitions (a HydraFS partition next to a small FAT one, for a PC), a real clock for the stamps, fsck-style repair, sparse files.
