# The kernel's devices

**kdev** (`base/modules/kdev/kdev.s`) serves the kernel's own devices: the root of the file tree, `null` and the
kernel's messages, the clock, the paged ROM's modules, the tasks (`/proc`), pipes, the environment, shared segments'
names, and raw RAM.  It's a boot driver like any other (task E in the base, task D in HydraOS), not the kernel: the
kernel keeps the tables, and kdev turns them into files.  HydraOS's kdev (`reborn/modules/kdev`: the base's source,
with its own added) also serves [GPIO and I2C](gpio.md) and reads [the Vera X's keyboard and
mouse](video.md#the-keyboard-and-the-mouse), so that those take no tasks of their own.  [Files, servers and
namespaces](../03-base/files.md#the-kernels-own-devices) lists its devices; this page is how to use them.

## Contents

1. [The root](#the-root)
2. [null, zero and the kernel's messages](#null-zero-and-the-kernels-messages)
3. [The clock](#the-clock)
4. [The modules](#the-modules)
5. [/proc: the tasks](#proc-the-tasks)
6. [Pipes](#pipes)
7. [The environment](#the-environment)
8. [Shared segments by name](#shared-segments-by-name)
9. [Raw RAM](#raw-ram)

---

## The root

`#/` is the root, bound at `/`: an empty directory for each mount point the default namespace uses, so `ls /` and
`ls /dev` show them even before anything's mounted there.  `/` holds `bin`, `dev`, `env`, `lib`, `mnt`, `pc`, `proc`,
`ram`, `rom`, `sd`, `sram` and `tmp`; `/dev` holds `gpio`, `i2c`, `mod`, `sd`, `seg`, `spi` and `vid`.

---

## null, zero and the kernel's messages

`#n`, bound at `/dev`:

* **`/dev/null`** reads as nothing and takes every write: `ls >/dev/null`.
* **`/dev/zero`** reads as zeros.
* **`/dev/kmesg`** is the kernel's messages: everything it printed on the bring-up console, the boot and POST
  included, its last 4K ([the boot](../03-base/boot.md#after-the-boot-the-kernels-messages)).

```
/> cat /dev/kmesg | head -4

Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000 20:0/00/0000
```

---

## The clock

`#t`, bound at `/dev`:

* **`/dev/ticks`**: the tick count (200 a second), in decimal.
* **`/dev/time`**: the date and time as text.  Write one to set the clock, and the DS1747 with it if there's one:
  `echo 2026-10-11 09:30:00 >/dev/time`.  `date` reads it.
* **`/dev/rtc`**: the DS1747's registers, as text (`none` without one).

```
/> cat /dev/ticks
1115
/> cat /dev/time
2000-01-01 00:00:10
```

Without a DS1747 the clock starts at 2000-01-01 each boot.

---

## The modules

`#m`, bound at `/dev/mod`: a file for each module of the paged ROM, its image (a module's header first, which `SPAWN`
reads).  `#m/bin` lists the programs alone, and is bound into `/bin`, so `rc` runs the ROM's `rc`.

```
/> ls /dev/mod | head -6
bin/
init
cons
storage
kdev
snd
```

`mods` lists the modules with their banks and types.

---

## /proc: the tasks

`#p`, mounted at `/proc`: a directory for each task in use, by its number.

```
/> ls /proc/1
status
ctl
args
cwd
fd
ns
env
regs
mem
ram
note
/> cat /proc/1/status
init wait - 37 1
```

`status` is the task's name, state, parent (`-`: none), CPU time in ticks, and note group.  The other files:

| File | What it is |
| :--- | :--------- |
| `args` | Its arguments |
| `cwd` | Its current directory |
| `fd` | Its open files, a line each: `0 rw #c 291 #c/cons` (the fd, its mode, the device, its qid, its name) |
| `ns` | Its namespace, as the `bind` and `mount` lines that would make it |
| `env` | Its environment |
| `regs` | Its saved registers (a stopped or waiting task's) |
| `mem` | Its 64K as it sees them: read and write it as a file, the offset the address |
| `ram` | Its own banks: bank b's byte o at `b * $2000 + o` |
| `note` | Write a note's name or number to send it: `echo interrupt >/proc/5/note` |
| `ctl` | Commands ([below](#ctl)) |

```
/> cat /proc/1/ns | head -4
bind '#/' /
bind '#/dev' /dev
bind -a '#c' /dev
bind -a '#n' /dev
```

### ctl

| Command | What it does |
| :------ | :----------- |
| `kill` | Sends the kill note: the task ends |
| `interrupt` | Sends the interrupt note (Ctrl-C's) |
| `note N` | Sends note N |
| `stop`, `start` | Stops the task where it is, or starts it again (`ps` shows `stopped`) |
| `step` | Runs one instruction of a stopped task |
| `next` | The same, but a `JSR`'s subroutine runs whole |
| `break`, `nobreak` | A `BRK` stops the task (a debugger's breakpoint), or is the program's own again |

`ps`, `top`, `kill`, `slay` and `db` are built on these files.  A task's memory, registers and notes are anyone's but
the kernel's and a driver's: their tasks are the system's.

---

## Pipes

`#|`: opening its `pipe` file makes a new pipe; the `PIPE` call makes both ends at once (`.A` the read end, `.X` the
write end).  A pipe holds 512 bytes; a reader waits for bytes, a writer for room, and the reader sees the end of the
file when every write end is closed.  Eight pipes can be open at once.  rc's `|` uses them.

---

## The environment

`#e`, mounted at `/env`: **the asking task's environment**, a file for each variable.  Every task has its own (a copy
of its parent's, made as it starts), so a variable a program sets is its own and its children's, not its parent's.

```
/> rc
% greeting=hello
% cat /env/greeting
hello% echo $greeting
hello
% ls /env
prompt
path
task
greeting
```

rc keeps its variables here (a list's words each followed by a zero byte, which is why `cat` showed no new line), and
its functions as `fn#NAME`.  `/proc/N/env` is any task's.  Note that at HyForth's prompt each rc line runs in an `rc` of
its own, so a variable set there ends with the line: start `rc` for a session that keeps them.

---

## Shared segments by name

`#s`, bound at `/dev/seg`: names for shared segments, so tasks that didn't start each other can find one.

* `echo name scores 3 >/dev/seg/ctl` names segment 3 `scores`.  The name holds a reference of its own, so the segment
  stays while it's named.
* `cat /dev/seg/scores` gives its number and banks (`3 2`: segment 3, of 2 banks); a task then attaches to it
  (`SEG_ATTACH`).
* `echo free scores >/dev/seg/ctl` lets the name go.

`free` shows the shared RAM: what the segments have, what's free.

---

## Raw RAM

`#r`, for init alone (it isn't in the default namespace): `task` is every task's RAM (task t's at `t * $8000`, 512K),
and `shared` the shared banks (shared bank ID i's at `i * $2000`, 2M), read only.  It's the system's raw view, for
init's own use.
