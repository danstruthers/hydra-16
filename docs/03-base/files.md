# Files, servers and namespaces

On the Hydra, as in Plan 9, **everything is a file**: a disk's files, the console, the serial port, the sound chip,
another task's memory, a pipe, the environment.  A program uses them all with the same few calls (`OPEN`, `READ`,
`WRITE`, `CLOSE`), and controls a device by writing text commands to its `ctl` file.  This page is the model: what a
file is to the kernel, who serves it, how names find it, and the kernel's own devices.  The code is
`base/kernel/file.s`, `ns.s` and `env.s`, and `base/modules/kdev`.

## Contents

1. [Fds and channels](#fds-and-channels)
2. [Servers and requests](#servers-and-requests)
3. [Waiting](#waiting)
4. [Names](#names)
5. [Namespaces](#namespaces): binds, mounts and unions
6. [The environment](#the-environment)
7. [The kernel's own devices](#the-kernels-own-devices): kdev
8. [Every device letter](#every-device-letter)

---

## Fds and channels

A task has **16 fds**: 0 is its standard input, 1 its standard output, 2 its standard error.  Each fd names a
**channel**, an open file in the kernel task's table (48 of them): its server, the server's handle for the file (its
**fid**), the mode it was opened with, and its **offset**.

* `DUP`, `DUP2`, and a child's inheritance give another fd for the same channel, so they share the offset (as Unix's
  do).
* A child gets its parent's fds 0, 1 and 2, or with `SPAWN_FDMAP` whichever the parent chooses.
* `CLOSE` closes an fd; the channel closes when no fd names it.

| Call | What it does |
| :--- | :----------- |
| `OPEN` | `r0` a path, `.A` a mode (`O_READ`, `O_WRITE`, `O_RDWR`, with `O_TRUNC`, `O_NONBLOCK`): `.A` an fd |
| `CREATE` | A file made (or emptied) and opened; `.X` = `DM_DIR` makes a directory |
| `READ`, `WRITE` | `.A` an fd, `r0` a buffer, `r1` a count: `.A/.X` the count done.  `READ` gives 0 at the end of the file |
| `SEEK` | An fd's offset (`r0/r1`: 32 bits), from the start, the offset, or the end |
| `CLOSE`, `DUP`, `DUP2` | As Unix's |
| `STAT`, `FSTAT` | A file's **stat record**: 64 bytes (`SR_*`): its name, qid, mode, length, time, device |
| `WSTAT`, `FWSTAT` | Change a file from a stat record: rename it, change its mode or length (fields left all `$FF` stay) |
| `REMOVE` | A file, or an empty directory |
| `PIPE` | Two fds, a pipe's ends: `.A` to read, `.X` to write (512 bytes held between them) |
| `FD2PATH` | The name an fd was opened by, whole and clean |
| `CHDIR`, `GETCWD` | The current directory |

**A directory reads as stat records**, one after another (`SR_SIZE` each): there's no text to take apart.  `ls` reads
a directory and prints each record.

`PUTC`, `PUTS` and `GETC` are a byte or a string written to fd 1, and a byte read from fd 0.  A task with no fds (the
kernel, a driver) gets the bring-up console instead: the serial port, polled.

---

## Servers and requests

Every file is served by a **server**: a task that has registered a **device letter** (`SRV_REGISTER`) and answers
requests for it.  A **driver** is a server that's a module of the paged ROM, started in a task of its own.

When a program calls `OPEN`, `READ` or any file call, the kernel:

1. fills a **request block** (`RQ_*`, 32 bytes) in the program's own OS area: the request (`R_OPEN`, `R_READ`,
   `R_WRITE`, `R_STAT`, `R_CLUNK` ...), the fid, the mode, the offset, the count, the buffer's address, the client's
   task and note group;
2. **SCALLs** the server: the server's serve entry runs in the server's own task, while the client waits in its call;
3. the server takes the request into its `TASK_INBOX` (`SRV_TAKE`), moves the data between its memory and the
   client's buffer with `CLIENT_READ` and `CLIENT_WRITE` (kcopy: each side as its own task sees it), and answers
   (`SRV_REPLY`): C and `.A`, the count done, a new fid.

`READ` and `WRITE` ask for 512 bytes (`IO_UNIT`) at most a request.  A `READ` goes on until its count is done, the
file ends, or a request comes back short; a `WRITE` until its count is done.

**srvlib** (`base/sdk/asm/srvlib.inc` and `srvlib.s`) does nearly all of a server's work: it keeps the fids, makes stat
records, walks a tree of files described as a table, makes text files' contents on each read, and parses commands
written to `ctl` files.  A server is mostly that table and a routine for each file ([servers and
drivers](../09-programming/servers.md)).

**Control is text.**  A device is set by writing a command to its `ctl` file, its words after it (`echo b115200
>/dev/serctl`), and read back as text (`cat /dev/serctl`).  A bad command is `E_INVAL` and changes nothing.

---

## Waiting

**A server never waits.**  If it can't answer yet (a pipe is empty, a console has no line), it answers `E_AGAIN`, and
the kernel has the client wait:

* each server has an **event count**, a byte in its zero page (`TASK_EVENT`);
* the client waits (`EVENT` state, using no CPU) until the server's event count changes from what it was when the
  server took the request, or a `WAKE`; then the kernel sends the request again;
* the server adds 1 to its event count whenever something its clients may be waiting for happens: a byte came in,
  there's room to send.  Its interrupt entry can do this in a single instruction (`inc TASK_EVENT`).

**A note ends a client's wait**: the server gets `R_FLUSH`, and the client's call returns `E_INTR`.  An fd opened
with `O_NONBLOCK` gets `E_AGAIN` instead of waiting.

So a server stays simple (one request at a time, never blocked), a slow device never holds up the system, and Ctrl-C
always works.

---

## Names

A name is made **whole and clean** before it's looked up: a relative name is put after the current directory, then
`.`, `..` and empty elements are taken out.  `cd /sd/0/src; cat ../readme` reads `/sd/0/readme`.

A name that starts with **`#`** is a device's own: `#c/cons` is the console server's `cons`, found in no namespace.
What follows the letter, up to the `/`, is its **spec**, which picks one of the device's trees: `#c2/cons` is the
console's window 2, `#f` with spec `x` the ROM disk's file system.

Any other name is found in the task's **namespace**.

---

## Namespaces

A namespace is a table of **mount entries** (`ns.s`): each says that a device's tree (a letter, a spec and a path in
it) appears at a name, a **mount point**.  Several entries at one mount point make a **union**, in order.

**Looking a name up:** its mount point is the longest one it starts with, in whole elements.  The union's members are
its **candidates**, each with the rest of the name: `OPEN`, `STAT` and `REMOVE` try them in turn until one has the name.
`CREATE` goes to the member marked for creating (`MCREATE`), or the first.

A **directory at a union** reads as every member's records, one member after another: `ls /bin` lists the programs from
every place bound there.

| Call | What it does |
| :--- | :----------- |
| `BIND` | `r0` new at `r1` old: `MREPL` in place of what's there, `MBEFORE` or `MAFTER` in its union; `\| MCREATE` to take creates |
| `MOUNT` | A device's tree (`.X` its letter, `r0` its spec) at old, with the same flags |
| `UNMOUNT` | All of old, or its member new |
| `NSINFO` | One entry of a task's namespace (`/proc/N/ns` reads them as `bind` and `mount` lines) |

A bind is resolved when it's made, as in Plan 9: `bind /rom/bin /bin` binds whatever `/rom/bin` is at that moment.

**Who shares a namespace:** a child shares its parent's until either changes it, which gives the changer a copy.
`SPAWN_NEWNS` gives a child an empty one.  So a shell can change its own view of the files (bind a directory over
`/bin`, mount a card) without changing anyone else's.

In the base, the monitor needs no namespace: it opens `#c/cons` by its device name.  HydraOS's init builds the
system's namespace from a file, `/rom/lib/namespace`, and so does each new shell ([rc](../05-rc.md#namespaces)):

```
bind '#/' /
bind -a '#c' /dev           # cons consctl ser serctl
bind -a '#n' /dev           # null zero kmesg
bind '#d' /dev/sd
mount '#e' /env             # the environment
mount '#p' /proc
mount '#f' /sd              # the cards: /sd/0 ... /sd/f
mount '#f' /rom x           # the ROM disk
mount '#f' /sram s          # the shared RAM disk
mount -c '#f' /ram r/$task  # this shell's own area of the RAM disk
bind -c /ram/bin /bin
bind -a /sram/bin /bin
bind -a /sd/0/bin /bin      # (added only if the card has one)
bind -a /rom/bin /bin       # programs on the ROM disk
bind -a '#m/bin' /bin       # the ROM's program modules
...
```

---

## The environment
*`env.s`*

Each task has its own **environment**: 8K of variables, each a name and a value of any bytes, kept in a RAM bank of
the kernel task's.  `SPAWN` gives a child a copy of its parent's (`SPAWN_NOENV`: an empty one).

| Call | What it does |
| :--- | :----------- |
| `ENV_GET` | A variable's value, or part of it, into a buffer |
| `ENV_PUT` | Set a variable, or part of its value |
| `ENV_DEL` | Remove one |
| `ENV_NAME` | The variables' names, one at a time |

Each takes a task's number (`$FF`: this task's), so a program can read another's.  kdev serves the caller's environment
as files, `#e`, mounted at **`/env`** (Plan 9's): `cat /env/home`, `echo hi >/env/greeting`.  rc keeps its variables
there (a list's words each ending with a zero byte) and its functions as `fn#NAME`; C's `getenv`, HyForth's `getenv`
and hylang's `$name` read them.  With no memory card, there are no environments.

---

## The kernel's own devices

The kernel's devices are served by **kdev**, a boot driver like any other (task E in the base, D in HydraOS), not by the
kernel task: the kernel keeps only tables.

| Letter | Bound at | Files |
| :----- | :------- | :---- |
| `#/` | `/` | The root: an empty directory for each mount point the default namespace uses (`bin`, `dev`, `env`, `lib`, `mnt`, `pc`, `proc`, `ram`, `rom`, `sd`, `sram`, `tmp`; and in `dev`: `gpio`, `i2c`, `mod`, `sd`, `seg`, `spi`, `vid`), so `ls /` shows them |
| `#n` | `/dev` | `null` (reads as nothing, takes every write), `zero` (reads as zeros), `kmesg` (the kernel's messages: [the boot](boot.md#after-the-boot-the-kernels-messages)) |
| `#t` | `/dev` | `ticks` (the tick count, in decimal), `time` (the date and time, as text: write one to set the clock, and the DS1747 if there's one), `rtc` (the DS1747's registers) |
| `#m` | `/dev/mod`, and `#m/bin` at `/bin` | A file for each module of the paged ROM, its image (`SPAWN` reads its header); `bin`, the programs alone |
| `#p` | `/proc` | A directory for each task ([below](#proc)) |
| `#\|` | | Pipes: opening `pipe` makes one; `PIPE` makes both ends.  512 bytes each, 8 at once |
| `#e` | `/env` | The asking task's environment, a file a variable |
| `#s` | `/dev/seg` | Shared segments by name: `ctl` (`name NAME N`, `free NAME`), and a file each name, read as its segment's number and banks |
| `#r` | (init's alone) | Raw RAM, read only: `task` (every task's RAM, task t's at `t * $8000`) and `shared` (the shared banks) |

### /proc

`/proc/N` is task N, as files:

| File | What it is |
| :--- | :--------- |
| `status` | Its name, state, parent, CPU time in ticks, and note group |
| `args` | Its arguments |
| `cwd` | Its current directory |
| `env` | Its environment |
| `fd` | Its open files |
| `ns` | Its namespace, as `bind` and `mount` lines |
| `regs` | Its saved registers |
| `mem` | Its 64K as it sees it, read and written as a file (any task's but the kernel's and a driver's) |
| `ram` | Its banks: bank b's byte o at `b * $2000 + o` |
| `note` | Write a note's name or number to send it |
| `ctl` | Commands: `kill`, `interrupt`, `note N`, `stop`, `start`, `step`, `next`, `break`, `nobreak` |

`ps` and `top` read `status`; `kill` and `slay` write `ctl`; `db`, the debugger, uses `mem`, `regs` and `ctl`.  A
task's memory is reachable **only** this way: the kernel's `TASKMEM` serves only kdev.

---

## Every device letter

The base's devices are kdev's and the console's.  HydraOS adds the rest ([chapter 4](../04-drivers/README.md)):

| Letter | Device | Server | Bound at |
| :----- | :----- | :----- | :------- |
| `#/` | The root | kdev | `/` |
| `#c` | The console (`#cN`: window N) | `ser` (base), `cons` (HydraOS) | `/dev` |
| `#P` | `/pc`: a folder on the PC | `cons` | `/pc` |
| `#n`, `#t` | null, zero, kmesg; ticks, time, rtc | kdev | `/dev` |
| `#m` | The paged ROM's modules | kdev | `/dev/mod`, `/bin` |
| `#p` | The tasks | kdev | `/proc` |
| `#\|` | Pipes | kdev | |
| `#e` | The environment | kdev | `/env` |
| `#s` | Shared segments' names | kdev | `/dev/seg` |
| `#r` | Raw RAM | kdev | (init's) |
| `#d` | The disks | `storage` | `/dev/sd` |
| `#f` | HydraFS, a disk's file system | `storage` | `/sd`, `/rom`, `/ram`, `/sram` |
| `#S` | The SPI devices | `storage` | `/dev/spi` |
| `#g`, `#i` | GPIO, I2C | `gpio` | `/dev/gpio`, `/dev/i2c` |
| `#a` | Sound | `snd` | `/dev` |
| `#v` | The Vera X | `vid` | `/dev/vid` |
