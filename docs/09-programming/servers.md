# Servers and drivers

A **server** is a task that serves a device letter: its clients' `OPEN`, `READ`, `WRITE`, `STAT` ... of `#x/...` come
to it as requests.  A **driver** is a server that's a module of the paged ROM, started in a task of its own (the highest
free: task F down), at boot if it's flagged so.  Every one of the system's drivers is built this way, and so can yours
be: a device of your own, a file system, a service other programs talk to through files.

[Files, servers and namespaces](../03-base/files.md#servers-and-requests) explained the model; this page is how to
write one.

## Contents

1. [srvlib](#srvlib)
2. [A server, walked through](#a-server-walked-through)
3. [The tree](#the-tree)
4. [The kinds of entry](#the-kinds-of-entry)
5. [Hooks](#hooks)
6. [Requests and waiting](#requests-and-waiting)
7. [Interrupts](#interrupts)
8. [Several devices, and raw devices](#several-devices-and-raw-devices)
9. [Putting it in the ROM, and testing it](#putting-it-in-the-rom-and-testing-it)

---

## srvlib

Every server is built on **srvlib** (`base/sdk/asm/srvlib.inc` at the top of its source, `srvlib.s` at the end), so a new
one is mostly tables: its files, and a routine for each that makes or takes its contents.  srvlib does the rest: it takes
each request, keeps the fids, walks the tree, makes stat records and directory listings, slices text files at a read's
offset, and parses `ctl` commands.  It answers what it doesn't know with `E_NOSYS`.

---

## A server, walked through

`sdk/asm/samples/counter/counter.s` serves `#k`: `count`, a text file (the count), and `ctl` (`add N`, `reset`).

```
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"                                       ; (srvlib.s at the end)

            HYX2_DRIVER "counter", init, srv_serve, 0, 0, HF_BOOT
```

`HYX2_DRIVER`'s entries: `init` (run once as it starts), the serve entry (srvlib's `srv_serve`, which takes each request
and does the rest), the irq entry and the stop entry (none here), and its flags (`HF_BOOT`: started at boot, which waits
for its init).  `init` registers the letter:

```
init:
            stz         count
            stz         count + 1
            lda         #'k'
            jmp         SRV_REGISTER                        ; (C = 1, .A = an error: it ends)
```

The tree is a table: each entry its name, its parent (an entry's number), its kind, its handler, its mode and an aux byte:

```
srv_tree:
            SRV_ENTRY   s_root,  $FF, SK_DIR,  0,         SM_READ,            0     ; 0
            SRV_ENTRY   s_count, 0,   SK_TEXT, gen_count, SM_READ,            0     ; 1
            SRV_ENTRY   s_ctl,   0,   SK_CTL,  ctl_cmds,  SM_READ | SM_WRITE, 1     ; 2 (reads as count)
            .word       0
ctl_cmds:                                                   ; Its commands: the word, the handler
            .word       s_add, c_add
            .word       s_reset, c_reset
            .word       0
```

A text file's handler makes its text each time it's read (srvlib slices it at the read's offset, so the server keeps
nothing between reads); a ctl command's handler gets the words after it, parsed (`srv_argn` of them, `srv_argp` their
places, `srv_arg` their numbers: decimal, or `$hex`):

```
gen_count:
            lda         count
            ldx         count + 1
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

c_add:
            lda         z:srv_argn                          ; (No number: E_INVAL)
            beq         @inval
            clc
            lda         count
            adc         srv_arg
            sta         count
            ...
```

```
% cat '#k/count'
0
% echo add 5 >'#k/ctl'; cat '#k/count'
5
```

---

## The tree

`srv_tree` is the server's files, an entry each (`SRV_ENTRY`, `SE_SIZE` bytes), entry 0 the root, ending with a 0 word:

| Field | What it is |
| :---- | :--------- |
| `SE_NAME` | The name's address |
| `SE_PARENT` | The parent's entry number (`$FF` for the root) |
| `SE_KIND` | `SK_DIR`, `SK_TEXT`, `SK_CTL`, `SK_DATA`, `SK_DYN`, `SK_RAW` |
| `SE_HANDLER` | The kind's routine or table |
| `SE_MODE` | Its permissions: `SM_READ`, `SM_WRITE` |
| `SE_AUX` | A byte of the kind's own |

---

## The kinds of entry

| Kind | What it is |
| :--- | :--------- |
| `SK_DIR` | A directory: the entries whose parent it is, read as stat records |
| `SK_TEXT` | A text file: its handler makes the text (`srv_tputs`, `srv_tputc`, `srv_tputdec`, `srv_tputhex`), 255 bytes at most |
| `SK_CTL` | A ctl file: a table of commands (`.word name, handler`, ending with 0); a write is split into words and the command's handler called; a bad command is `E_INVAL` and changes nothing.  Its aux: the entry of a text file it reads as (its state) |
| `SK_DATA` | Anything else: the handler gets each request (`.A` = `R_OPEN`, `R_READ`, `R_WRITE`, `R_CLUNK`, `R_DUP`; `.X` the fid), moves the data itself (`CLIENT_READ`, `CLIENT_WRITE`), sets the count done (`RQ_DONE`), and answers C = 0, or C = 1 and an error |
| `SK_DYN` | A directory whose children a handler makes as it's read (the tasks in `/proc`, the modules in `#m`): the handler names its children (`DYN_NAME`), finds one by name (`DYN_FIND`), and names one by its id (`DYN_IDNAME`); its aux is the entry each child is, so a child can be a directory of its own |
| `SK_RAW` | A device whose every request goes to one handler, its fids its own (a file system: `#f`) |

**Control is text**: a device is set by writing a command to its `ctl`, its words after it, and read back as text.  So a
device needs no special program: rc's `echo` and `cat`, and every language's file calls, drive it.

---

## Hooks

A server may define these before including `srvlib.s`, for what the tables don't cover:

| Hook | Called |
| :--- | :----- |
| `SRV_OPENED` | For each fid made (an open, a dup): it may keep a byte for it (`srv_fid_aux`), from the open's spec, or refuse |
| `SRV_CLUNKED` | For each fid forgotten (a close) |
| `SRV_FLUSH` | When a client's wait is ended by a note (`R_FLUSH`): forget what was kept for it |
| `SRV_PRE`, `SRV_POST` | Before every request, and after it |
| `SRV_STAT` | For each stat record made: fill in more (a data file's length) |
| `SRV_TREES` | Several devices ([below](#several-devices-and-raw-devices)) |

**What's a client's**: a task's claim on something (sound channels, the VERA) is that task's, given back as its last file
of the device closes: count the fids in `SRV_OPENED` and `SRV_CLUNKED`.

---

## Requests and waiting

A request is a block the client fills in its own memory (`RQ_*`: the request, its fid, mode, offset, count, buffer, its
task and note group); `SRV_TAKE` copies it into the server's `TASK_INBOX`, and `SRV_REPLY` sends back the count done and a
new fid.  srvlib does both.  Data moves by kcopy: `CLIENT_READ` and `CLIENT_WRITE` between the server's memory and the
client's buffer, as each task sees its own.

**A server never waits.**  When it can't answer yet (a pipe that's empty, a console with no line), it answers `E_AGAIN`,
and the kernel has the client wait until the server's **event count** changes from what it was as the server took the
request, then sends the request again.  So a server adds 1 to its event count (`inc TASK_EVENT`) whenever something its
clients may be waiting for has happened: a byte in, room to send.

* A note ends a client's wait: the server gets `R_FLUSH` (`SRV_FLUSH`, if it keeps anything for a client), the client
  `E_INTR`.
* A client whose fd is `O_NONBLOCK` gets `E_AGAIN` instead of waiting.
* **A server that must give up on something that doesn't come times it itself**: the kernel has no timed wait.  It makes
  its event count change now and then (from an interrupt it owns), and the client, asking again, looks at the time.
* A server serves one request at a time; a second client waits until it's free.

---

## Interrupts

A driver owns an interrupt line with **`IRQ_OWN`** (`.A` the line) in its `init`; its **irq entry** (the third of
`HYX2_DRIVER`'s) runs in its own task with the line in `.A`, interrupts off, and returns `.A` = 0, or `IRQ_RESCHED` for a
task switch.  The rules:

* **About 85 cycles**: no stretch of code may hold interrupts off for more than 200, and the dispatch takes 115.
* **Never wait**, and never call what waits.
* **Wake clients by adding 1 to the event count** (`inc TASK_EVENT`), never with `WAKE`.
* **Send a note** to a note group with `NOTE_QUEUE` (the console's Ctrl-C does).
* **Quiet the device**: the lines are level-triggered and can't be masked, so the handler must clear what interrupted,
  or it comes straight back.
* **Own the line before the device can interrupt**: a line nobody owns is counted and ignored, and comes straight back.
* A module of more than one bank owns a line only with its irq entry (and what that calls) in its RAM.

VIA timer 2 (line 16) and CA1 (line 17) are lines of their own; owning one is owning that part of the VIA.  The lines'
owners are in [the kernel](../03-base/kernel.md#interrupts).

---

## Several devices, and raw devices

* **`SRV_TREES`** gives a server several letters, a tree each: `.byte` the letter, `.word` its tree, ending with 0.  A
  request's `RQ_DEV` chooses the tree.  `kdev` serves nine devices this way.
* **A spec**: what follows a device's letter in a `#` name (`#c2`, `#fx`) reaches the server in the open request, so one
  tree can serve several instances (the console's windows).  `SRV_OPENED` reads it and keeps it in the fid's aux byte.
* **`SK_RAW`**: a tree of one entry, whose handler gets every request and keeps its own fids: for a server whose files
  aren't a fixed tree, such as a file system (HydraFS's `#f`).

---

## Putting it in the ROM, and testing it

A driver is a folder in `modules/` (`modules/NAME/*.s`), listed in `modules/rom.txt`; `node build.js` builds it with the
rest.  `HF_BOOT` starts it at boot, in the next free task down from F; without it, a program starts it with `SPAWN`.

A **test** of it is a test module (`tests/mod`) that runs as init, with the driver in the ROM (`tests/tests.js`: a test's
`modules`), and `node sim/test.js NAME -v` runs it ([the tests](../08-emulator.md#the-tests)).  `/proc` refuses a
driver's memory and registers, and `NOTE` its notes: a driver's task is the system's.
