## **Namespaces, the Plan 9 way: one tree put together from many**

A plan to make the namespace the way names are found, as Plan 9 does, in place of search paths.  Directories are
mapped onto other paths (bound, mounted) and stacked into **unions**, so `/bin` is one directory that holds the
program caches, the cards' programs and the ROM's, and the shell just looks in `/bin`.  Nothing here is built
yet; it goes with the [RAM and ROM disks](DISKS.md), which give it most of what it puts together.

### **Contents**
1. [Where it stands](#where-it-stands)
2. [What Plan 9 does](#what-plan-9-does)
3. [Unions](#unions)
4. [The namespace a task starts with](#the-namespace-a-task-starts-with)
5. [Finding programs and libraries](#finding-programs-and-libraries)
6. [Room: a bigger namespace](#room-a-bigger-namespace)
7. [Hiding](#hiding)
8. [Calls and words](#calls-and-words)
9. [Costs](#costs)
10. [Tests](#tests)
11. [Steps](#steps)

---

### **Where it stands**

Each task has a namespace of up to 5 entries, copied to the tasks it starts ([io.md](../programming/io.md#namespaces)):
a **mount** sends names under a path to a device's server, and a **bind** makes a path stand for another; the
entry with the longest matching prefix applies.  Names nothing matches must be under `/dev` or `/env`, which the
IO layer knows itself; `/rom` and `/ram` are plain binds to the ROM and RAM disks (`/rom = /sd/x`, `/ram = /sd/r`,
made by the boot shell: [DISKS.md](DISKS.md)).  Programs and libraries are found by **search paths**: the shell
tries the current directory, then the RAM disks' caches (`/ram/N/bin` up the owner chain, `/ram/s/bin`), then
`$PATH`'s directories (or the card's `/bin`), then `/rom/bin` (`$LIBPATH` and `/lib` the same).

That works, but the search is the shell's alone (a C program that opens `/bin/x` sees none of it), each new place
(the RAM caches, a second ROM disk, `/pc`) is another case in the shell's code, and `ls /bin` shows one of the
places, not what runs.

---

### **What Plan 9 does**

There are no search paths.  Each process has a namespace, built from **binds** (`bind new old`: the directory
`new` is also seen at `old`) and **mounts** (a file server's tree at `old`), and a bind or mount can **add to**
what's at `old` instead of replacing it: `bind -a /usr/me/bin /bin` puts my programs after the system's in `/bin`,
`bind -b` before.  So `/bin` is a **union directory**: a lookup tries each member in order and takes the first
that has the name; a listing shows them all.  The shell has `path=(. /bin)` and nothing else: every program,
from anywhere, is in `/bin`.  A process starts with its parent's namespace (a copy, or shared), and a user's
namespace is built at login from a file of `bind` and `mount` lines (`/lib/namespace`, then the user's own).
Permissions stay with the file servers: the namespace only says what a process can *name*.

---

### **Unions**

An entry gets **flags**, as Plan 9's:

| Flag | `bind` / `mount` | Meaning |
| :--- | :--------------- | :------ |
| (none) | `bind new old` | `new` replaces whatever was at `old` (as now) |
| `-b` | `bind -b new old` | `new` goes **before** what's at `old`: looked in first |
| `-a` | `bind -a new old` | `new` goes **after** what's at `old`: looked in last |
| `-c` | with `-b` or `-a` | a file **created** in the union goes into this member (the first such) |

* **Looking up** `old/name`: each member in order (a bind's target, or a mount's server), the first that has
  `name` wins.  So `/bin/game` is the first `game` in the stack.
* **Listing** `old`: each member's entries, in order (an fd on a union directory reads one member, then the
  next).  A name in two members shows twice, as in Plan 9; `ls` can leave out the later ones.
* **Creating** `old/name`: in the first member bound with `-c` (none: `ERR_IO_MODE`).
* **Removing, renaming:** in the member that has the name.
* **`unmount new old`** takes one member out; `unmount old` takes them all.

---

### **The namespace a task starts with**

The boot shell builds its namespace, every task's at first, from `/rom/lib/namespace`, a file of binds and mounts
on the ROM disk (Plan 9's `/lib/namespace`), then runs the card's `/lib/namespace` if there is one, then
`boot.hys`.  Only the lines that reach that file are built in (`/sd` and `/rom`, as the boot shell makes them
now).  The built-in names become plain entries (no more special cases in the IO layer):

```
mount  /dev    ...                      the devices (as now)
mount  /env    env
mount  /proc   proc                     the tasks (PROC.md)
mount  /sd     hfs                      the disks' volumes: the cards /sd/0-f, the ROM disk /sd/x, the RAM disks
bind   /sd/x   /rom                     the ROM disk (as now: the boot shell's, before the file is read)
bind   /sd/r   /ram                     the RAM disk: /ram/0-f, the tasks' areas; /ram/s, the shared one (as now)
bind -bc /ram/<the shell>/bin /bin      /bin: this shell's cache first (created in: cp) ...
bind -a /ram/s/bin /bin                 ... the shared cache ...
bind -a /sd/<the boot volume>/bin /bin  ... the card's programs ...
bind -a /rom/bin /bin                   ... the ROM's
(and /lib the same, with lib)
```

* **A new task gets a copy** of its parent's, as now.  So a program started from the shell has the shell's `/bin`,
  its cache included, and the tasks it starts the same.
* **Two shells** start with the same list, each with its own area's cache first (`/ram/1/bin`, `/ram/3/bin`).
* **A task's area** is bound by number when its namespace is made (`/ram/1/bin` for the boot shell).
* **Adding a tool directory** is a line in `boot.hys` or the card's `/lib/namespace`: `bind -a /sd/0/tools /bin`.

---

### **Finding programs and libraries**

* **The shell looks in two places:** the current directory, then `/bin` (the union: every cache, card and ROM in
  their order), as Plan 9's `path=(. /bin)`.  So a program in the folder you're in runs before a cached copy:
  after a rebuild, the new one runs from its folder without removing the cached copy.
* **Libraries:** `lib name` reads `/lib/name.hyl`, the union `/lib`.
* **`$PATH` and `$LIBPATH`** are still read, after `/bin` and `/lib`, while scripts move to binds; then they go.
* **C programs** see the same `/bin` (`system`, `hy_spawn` and `fopen("/bin/x")` all through the namespace).
* **Caching** a program is a copy into the first member with `-c`, the task's own cache (`cp game.hyx /bin/game.hyx`).

---

### **Room: a bigger namespace**

The default namespace above is about 12 entries, and a union member is an entry, so 5 isn't enough.  Each task's
namespace is now 5 entries of 32 bytes in its IO transfer area (`$20-$BF`, with the current directory after it).
It moves to a namespace bank of its own:
* **A shared bank**, 512 bytes a task (16 tasks: one 8K bank): **16 entries of 32 bytes** each; or 8 entries of
  64 bytes for longer names (a prefix and a target of 31 characters each, against 13 and 15 now).  Which is
  better depends on the default list's names: `/ram/s/bin` fits in 15, `/sd/0/tools/bin` doesn't.
* **An entry:** the type (mount, bind, hide), the flags (`-b`, `-a`, `-c`), the device or the target, the prefix.
  The union's members are entries with the same prefix, in order.
* **Copying** a task's namespace for a new task is a 512-byte copy in the bank (`IO_INHERIT`).
* The transfer area's freed bytes go to a longer current directory (63 characters now).

---

### **Hiding**

A **hide** entry makes a path not exist for the task and the tasks it starts: `hide /sd`, then `/sd/0/...` is
`ERR_IO_NOT_FOUND`, however it's named.  It's the namespace's part in fencing a program in: start an untrusted
program with `/sd` hidden, and only its own area (`/ram/N`) and `/rom` to work in.  (Plan 9 does it with the namespace a process
is given, and `RFNOMNT` stops it mounting more.)  Permissions themselves stay in the servers: a hide can't grant
anything, only take a name away.

---

### **Calls and words**

* **`IO_BIND`, `IO_MOUNT`** take the flags in `.X` (0: replace, as now, so programs keep working):
  `NS_BEFORE`, `NS_AFTER`, `NS_CREATE`.
* **`IO_UNMOUNT`** takes the member too (`ZP_IO_BUF`: the target or device; 0: all of `old`).
* **`IO_HIDE`:** a new call (`.A.Y` = the path).
* **`IO_NS_LIST`** prints the entries as the lines that would make them (`bind -a /ram/s/bin /bin`), as Plan 9's
  `ns` does, so a namespace can be saved and read back.
* **HyForth:** `bind` and `mount` take the flags (`"-a" "/sd/0/tools" "/bin" bind`); `unmount`, `hide`, `ns`;
  `newns` reads a file of `bind` and `mount` lines (the card's `/lib/namespace`).
* **C:** `hy_bind(new, old, flags)`, `hy_mount`, `hy_unmount`, `hy_hide`.

---

### **Costs**

* **Lookups:** a name in a union is tried member by member: `/bin/game` with 4 members is up to 4 opens, and an
  unknown word (HyForth tries it as a program) tries `.hyx`, `.hys` and `.zsm` in each.  The caches come first
  and are fast; a miss on a card costs a directory search there.  If unknown words get slow, HyForth can stop
  trying the card for a name that isn't one (a word it couldn't parse), or the IO layer can remember misses.
* **ROM:** the union reads and the flags are in the IO layer (page 2: about 970 bytes free); the namespace bank
  needs `NS_RESOLVE` and `IO_INHERIT` changed, not much else.
* **Programs** that bind or mount keep working: no flags is the old behaviour.

---

### **Tests**

In the emulator (`sim/tests/devices.js`): unions (`-b`, `-a`, the order a name is found in, a listing of all the
members, a create into the `-c` member, unmounting one member); the default namespace (`ns`), a new task's copy;
programs found in `/bin` from a cache, a card and the ROM, in that order; `$PATH` still read; `hide`; 16 entries
(and the 17th refused); the existing namespace tests unchanged.

---

### **Steps**

1. The namespace bank: 16 entries a task, `NS_RESOLVE` and `IO_INHERIT` on it, longer names; every test passing.
2. Unions: the flags, lookups through the members, union directory reads, creates, `unmount` of one member.
3. The default namespace from `/rom/lib/namespace`: the built-in names as entries (`/env`, `/proc`, `/ram`), `/bin` and `/lib`
   unions, `newns` and the card's `/lib/namespace`; the IO layer's special prefixes go.
4. The shell: programs in the current directory then `/bin`, libraries in `/lib`, `$PATH` and `$LIBPATH` after
   them; a copy into `/bin` goes to the `-c` member.
5. `hide`; `ns` printing `bind` lines; C's calls.
6. Docs: `io.md` (namespaces), `hyforth.md` (the words, finding programs), `c.md`, the tutorial.
