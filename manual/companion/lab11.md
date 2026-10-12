# Lab 11. Disks

> **THIS LAB:** Manual: Chapter 14. About 45 minutes. You need a Hydra with an SD card (the emulator has one).

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Type `ls /dev/sd`. Each name is a disk. ✎ Fill in the table: read each disk's `ctl` file (`cat /dev/sd/0/ctl`)
   for the details.

| Disk | What kind it is | Its size | Seen in the tree at |
|---|---|---|---|
| `0` | | | |
| `x` | | | |
| `r` | | | |
| `s` | | | |

2. Try to change the ROM disk:

```text
/> echo hello >/rom/test
```

   ✎ What did the Hydra say? Why can't it be done?

<div class="write l2"></div>

3. How much room does a file really take? Make a tiny file on the SD card, and watch the free space:

```text
/> df
/> echo x >/sd/0/tiny
/> ls -l /sd/0/tiny
/> df
/> rm /sd/0/tiny
```

   ✎ How many bytes long is the file? How much less free space was there after you made it? Why isn't that the
   same number? (Section 14.5.)

<div class="write l3"></div>

4. Check a disk for faults. `s` is the shared RAM disk:

```text
/> fsck s
```

   ✎ What did it report?

<div class="write l2"></div>

5. `du` adds up the space that files use. ✎ What does each of these say?

| I typed | It said |
|---|---|
| `du /sd/0/sample` | |
| `du /rom/bin` | |

6. A disk has a name of its own, its **label**. ✎ What's your SD card's label? (It's in the card's `ctl` file.)

<div class="write"></div>

### Questions

1. ✎ The Hydra has three kinds of disk. What are they, and what's each one good for?

<div class="write l3"></div>

2. ✎ What's a block? How many bytes is it? What's a cluster?

<div class="write l2"></div>

3. ✎ An SD card works at 3.3 volts, and the Hydra at 5. What sits between them?

<div class="write"></div>

4. ✎ Why does the Hydra need its own file system, HydraFS? What must be done to a new card before the Hydra can
   use it?

<div class="write l2"></div>

5. ✎ Give two ways to move a file from the Hydra to a PC (section 14.7).

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | list the disks, and say what each is |
| ☐ | find out how much space is free, and how much a directory uses |
| ☐ | check a disk for faults |
| ☐ | say what happens to each disk's files when the power goes off |

> **CHALLENGE:** ★ The storage driver can make another RAM disk while the Hydra is running. Read the `start` and
> `stop` commands in section 14.4, and work out how. How big a disk can you make? Where does its memory come
> from? (Watch `free` before and after.) Don't forget to stop it.
