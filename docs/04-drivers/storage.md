# Storage

**storage** (`reborn/modules/storage/`, task E) owns the SPI bus and every disk, and the file system on them, HydraFS.
One driver for all of it, so an SD card's transfer never meets another device's on the bus.  It's HydraOS's; the base
has no storage.

## Contents

1. [The disks](#the-disks)
2. [Using them](#using-them): the cards, the ROM disk, the RAM disks
3. [The disks' files](#the-disks-files): `data` and `ctl`
4. [Getting a card ready](#getting-a-card-ready)
5. [Writes, and taking a card out](#writes-and-taking-a-card-out)
6. [The tools](#the-tools): `df`, `mkfs`, `fsck`, `label`
7. [HydraFS](#hydrafs): what's on a disk
8. [SPI devices as files](#spi-devices-as-files)
9. [How it works](#how-it-works)

---

## The disks

| Disk | What it is | Its files at | Speed |
| :--- | :--------- | :----------- | :---- |
| `0`-`7` | An SD card on SPI device 0-7, the board's headers J18-J25 | `/sd/0` ... `/sd/7` | About 12K a second |
| `8`-`f` | An SD card on SPI device 8-15, a card in a slot | `/sd/8` ... | |
| `v` | An SD card on the Vera X's own slot (the VERA's SPI controller) | `/sd/v` | Nearly twice as fast |
| `x` | **The ROM disk**: the paged ROM's banks after the modules, read only | `/rom` | |
| `r` | **The RAM disk**: banks of the storage driver's own, 256K as it starts.  Each shell has its own area of it | `/ram` | At once |
| `s` | **The shared RAM disk**: a shared segment, 512K as it starts | `/sram` | At once |

```
/> df
disk  kind   size        free        label
0     sdhc   16380 KB    15956 KB    HYDRA-16
x     rom    684 KB      0 KB        ROM
r     ram    255 KB      252 KB      RAM
s     sram   511 KB      510 KB      SRAM
```

A disk is listed once it's started: a card when something first opens it, the RAM disks as init starts them.

---

## Using them

The disks' file systems are mounted by the default namespace ([namespaces](../03-base/files.md#namespaces)):

* **`/sd/N`** is card N's, `/sd/0` the usual one.  A card's `bin` and `lib`, if it has them, join `/bin` and `/lib`, so a
  card can carry programs and libraries of your own.
* **`/rom`** is the ROM disk: the tools, which run from RAM (`/rom/bin`: `ls`, `cat`, `cp`, `ed`, `as`, `mkfs`, `grep`,
  `db`, `edit`, `calc` ...: [the tools](../07-tools.md)), the languages' libraries (`/rom/lib/forth`, `/rom/lib/hylang`, `/rom/lib/basic`), the
  assembler's include files (`/rom/lib/as`), the system's files (`/rom/lib/namespace`, `profile`, `shell`, `windows`),
  a song, and the call reference (`/rom/doc/api.md`).
* **`/ram`** is the shell's own area of the RAM disk (`r/N`, N its task): fast and private, gone when the power goes.
  `/ram/bin` comes first in `/bin`, so a program put there runs before the ROM's of the same name.
* **`/sram`** is the shared RAM disk: every shell sees the same one.

```
/> ls /rom
README
bin/
doc/
lib/
songs/
/> ls /sd/0
bench/
sample/
songs/
```

HydraOS's own card (`reborn/bin/sdcard.img`) has the SDK's samples (`/sd/0/sample`, the C ones in `/sd/0/sample/c`, the
assembler's sources in `/sd/0/sample/as`), the sample songs (`/sd/0/songs`) and the benchmarks (`/sd/0/bench`).

---

## The disks' files

`#d`, bound at `/dev/sd`: a directory for each disk started, each with two files.

* **`data`**: the disk as a file of bytes (its first 4 GB), at the fd's offset.  `cat /dev/sd/0/data` reads the raw card.
* **`ctl`**: reads as the disk and its file system; takes commands.

```
/> cat /dev/sd/0/ctl
sdhc 17 MB 34816 blocks
hydrafs label=HYDRA-16
partition at block 2048
free 15956 KB of 16380 KB
/> cat /dev/sd/r/ctl
ram 256 KB 512 blocks
hydrafs label=RAM
free 252 KB of 255 KB
banks $00-$1f
```

| Command | What it does |
| :------ | :----------- |
| `init` | Start the card again (after it's been changed) |
| `sync` | Write the block kept back ([below](#writes-and-taking-a-card-out)) now |
| `format [-f] [-p] [-s SIZE] [LABEL]` | A new, empty HydraFS (`-f` a full format; `-p` in a partition of its own after the card's others) |
| `label TEXT` | Set the label |
| `check [fix]` | Check the file system (and fix what's wrong) |
| `start SIZE [FROM-TO]` | A RAM disk of SIZE (8K banks, or `256K`, `1M`), from a range of memory cards; an empty HydraFS on it |
| `stop` | A RAM disk stopped, its banks given back (not while it's open) |

`echo start 128K 1-1 >>'#d/r/ctl'` starts a RAM disk on memory card 1's banks (`>>` because a stopped disk's directory
isn't listed).

---

## Getting a card ready

Any SD card (SDSC or SDHC) works, on an SD card module with its own 3.3 V regulator and level shifter
([the devices](../02-hardware/devices.md#the-spi-bus)).  HydraFS isn't a PC's file system, so a card is made one of two
ways:

* **On the PC**: write HydraOS's card image to it, `node sim/tools/sdwrite.js bin/sdcard.img DISK` from `reborn/`
  (`--list` shows the PC's disks first: it refuses the system's disk, and one that isn't removable without
  `--force`).  `sim/tools/hydrafs.js` makes images of your own from a folder ([the PC tools](../08-emulator.md#the-pc-tools)).
* **On the Hydra**: `mkfs 0 MYCARD` makes an empty HydraFS on card 0.  With `-p`, it goes in a partition of its own
  after the card's others, so a card can keep a PC partition too.

---

## Writes, and taking a card out

A card's writes are **kept back a block at a time**: the block a file's last write changed stays in the storage
driver's buffer until another block's wanted, the file's closed, or `sync`.  So a program writing a few bytes at a
time isn't slowed by the card.  But a block still kept back is lost if the card is taken out or the power goes:

* **Close the file** (end the program writing it), or **`echo sync >/dev/sd/0/ctl`**, before taking a card out or
  switching off.
* After changing a card, **`echo init >/dev/sd/0/ctl`** (or just open it: a card's first open starts it).  `init` drops
  a block kept back, since the card may be another.

The RAM disks' writes go at once.  HydraFS writes in a safe order (a file's data, then the free map, then its directory
entry), so a card pulled out loses at most what was being written, not the file system.

---

## The tools

| Tool | What it does |
| :--- | :----------- |
| `df` | Each disk started: its kind, its file system's size and what's free, its label |
| `mkfs [-fp] disk [label ...]` | A new, empty HydraFS on the disk (`0`-`f`, `r`, `s`): `-f` a full format; `-p` in a partition of its own |
| `fsck [-f] disk` | The disk's HydraFS checked (`-f`: and its faults fixed): what was lost, unmarked, or used twice |
| `label disk [text ...]` | The disk's label set, or said |

`mkfs`, `fsck` and `label` are RAM programs on the ROM disk; each writes the disk's `ctl`.

---

## HydraFS

HydraFS is the Hydra's own file system, designed for the 65C02 and Plan 9's file model.  Its full format is in
`docs/design/plans/HYDRAFS.md`; in short:

* **Blocks** are the card's 512-byte sectors; space is given out in **clusters of 8 blocks** (4K).
* **Block 0** is the superblock: the magic `HYDRAFS1`, the sizes, the free map's place, the root directory's entry, the
  label.  Then the **free map**, a bit a cluster, then the **data area**.
* **A directory** is a file of 64-byte entries, and **the entry is the file's metadata**: its name (31 bytes, any byte but
  `/` and 0, case-sensitive), its mode, its qid (an id unique on the disk, kept across renames, and a version that goes
  up at every change), its size (to 4 GB), its time, and where its data is.
* **A file's data** is a list of **extents**: runs of contiguous clusters.  Two fit in the entry; more go in a chain of
  extent blocks.  The allocator grows a file's last extent when it can, so files stay contiguous and a read is a run of
  blocks.  A hole (a sparse file's zeros) takes no space.
* **Partitions**: a card can have a partition table (a PC's MBR), HydraFS in its partition of type `$7F`, so a card can
  hold a HydraFS next to a FAT partition a PC reads.
* No journal: writes go in a safe order instead, and `fsck` finds and fixes what an interrupted write leaves.

---

## SPI devices as files

`#S`, bound at `/dev/spi`: a directory for each SPI device, `0`-`f`.  Any SPI part on a header or a card can be used
from a program or the shell, without a driver of its own:

* **`N/data`**: a write sends its bytes with device N selected, one transaction a request (256 bytes at most), and keeps
  the bytes the device sent back meanwhile; a read gives those bytes.  With none kept, a read clocks in as many as it
  asks for (sending `$FF`).
* **`N/ctl`**: `mode 0` (the clock idles low) or `mode 3` (it idles high); both send and sample on the clock's rising
  edge, most significant bit first.

A device is one opener's at a time, and not while it's a card's (`E_BUSY`).  HyForth's `lib spi` and hylang's `spi`
library are words over these files ([GPIO, I2C and SPI](gpio.md)).

---

## How it works

* **Two banks**: the first is srvlib, the SPI bus, the SD card layer, the disks, `#S` and `#d`; the second is HydraFS
  (`#f`).
* **SPI** is bit-banged on the VIA's port B, which only this task touches: 18 cycles a bit in, 33 out (the loops are the
  first Hydra software's, proven on the board).  The Vera X's card is on the VERA's SPI controller (12.5 MHz), which
  this task touches too; the rest of the VERA is `vid`'s.
* **A cache**: a card's blocks are cached, besides the block buffer, so a file read again doesn't go to the card.
* **The ROM disk** is read through the kernel's `ROMREAD`: block n is in bank n / 32 of the paged ROM's disk area.
  **A RAM disk**'s block n is in bank n / 16 of its banks.
