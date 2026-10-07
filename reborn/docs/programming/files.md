# Files and the namespace

## Files

A task has 16 fds (0 stdin, 1 stdout, 2 stderr).  Each names a channel, an open file in the kernel's table; `DUP`
and a child's inheritance share a channel, and so its offset.

| Call | What it does |
| :--- | :----------- |
| `OPEN` | `r0` a path, `.A` a mode (`O_READ`, `O_WRITE`, `O_RDWR`, with `O_TRUNC`, `O_NONBLOCK`): `.A` an fd |
| `CREATE` | A file made (or emptied), and opened; `.X` = `DM_DIR` makes a directory |
| `READ`, `WRITE` | `.A` an fd, `r0` a buffer, `r1` a count: `.A/.X` the count done (`READ`: 0 at the file's end) |
| `SEEK` | An fd's offset (`r0`, `r1`: 32 bits, signed), from the start, the offset or the end (`.X`) |
| `CLOSE`, `DUP`, `DUP2` | As Unix's |
| `STAT`, `FSTAT` | A file's stat record (64 bytes, `SR_*`: name, qid, mode, length, time, device) |
| `WSTAT`, `FWSTAT` | A file changed from a stat record: renamed, its mode, its length (fields left all `$FF` stay) |
| `REMOVE` | A file, or an empty directory |
| `PIPE` | Two fds, a pipe's ends: `.A` to read, `.X` to write (512 bytes held between them) |
| `FD2PATH` | The name an fd was opened by, whole and clean |
| `CHDIR`, `GETCWD` | The current directory |

A directory reads as stat records, one after another (`SR_SIZE` each): no text to take apart.  `READ` and `WRITE`
wait as their file's server needs (a pipe that's empty, a console with no line yet); a note ends the wait with
`E_INTR`, and an fd opened `O_NONBLOCK` answers `E_AGAIN` instead.  `PUTC`, `PUTS` and `GETC` are a byte or a string
to fd 1, and a byte from fd 0.

## Names

A name is made whole and clean before it's looked up: a relative one after the current directory, then `.`, `..` and
empty elements gone.  A name that starts with `#` is a device's own (`#c/cons`: the console's; `#c2/cons`: window
2's), in no namespace; any other is found in the task's namespace.

## The namespace

The namespace is a table of the task's binds and mounts, Plan 9's: a name's mount point is the longest one it starts
with, and the union there, its members in order, is where it's looked for.  `OPEN`, `STAT` and `REMOVE` try the
members in turn till one has the name; `CREATE` goes to the member bound with `MCREATE` (or the first).  A directory
opened at a union reads as every member's records.

| Call | What it does |
| :--- | :----------- |
| `BIND` | `r0` new at `r1` old: `MREPL` in place of what's there, `MBEFORE` or `MAFTER` in its union, `\| MCREATE` for creates |
| `MOUNT` | A device's tree (`.X` its letter, `r0` its spec: which of its trees) at old, the same flags |
| `UNMOUNT` | All of old, or its member new |
| `NSINFO` | An entry of a task's namespace (`/proc/N/ns` reads them as binds and mounts) |

A child shares its parent's namespace till one of them changes it (then it's copied); `SPAWN_NEWNS` gives it an empty
one, and `nslib.s` (the SDK's) builds the default from `/rom/lib/namespace` as rc's `newns` does.  The default
namespace (`/rom/lib/namespace`):

```
bind '#/' /
bind -a '#c' /dev           # cons consctl ser serctl
bind -a '#n' /dev           # null zero kmesg
bind '#d' /dev/sd
bind '#m' /dev/mod
bind '#s' /dev/seg
mount '#e' /env             # the environment
mount '#p' /proc
mount '#f' /sd              # the cards: /sd/0 ... /sd/f
mount '#f' /rom x           # the ROM disk
mount -c '#f' /ram r/$task  # this shell's own area of the RAM disk
bind -c /ram/bin /bin
bind -a /rom/bin /bin       # programs on the ROM disk
bind -a '#m/bin' /bin       # the ROM's program modules
...
```

## Devices

Each device is a file server, a driver's (or the kernel's own, `kdev`'s): a directory of files, controlled by writing
commands to its `ctl`, and read as text where that's what it is.

| Letter | Device | Files |
| :----- | :----- | :---- |
| `#c` | The console (a window each: `#cN`; its screen a VT100's, which answers DA, DSR's CPR, DECRQM, DECREQTPARM and xterm's `CSI 18 t` into a raw reader's keys, as they came, before its keys) | `cons`, `consctl` (`rawon`, `rawoff`, `keys vt` and `keys hydra`: a raw read's keys as a VT100 sends them, or one code each, `KEY_*`; `scroll jump` and `scroll smooth`: the window shown's writers going on as the serial port's terminal is painted as it can, or waiting for every byte to go out; `group`; `screen`, `serial`, `both`, or `terminal` and one of them; `terminal size C R`, the serial port's terminal's size, or `terminal size` alone to ask it (the console asks as it starts, and the PC tool tells it as its window changes); it reads with `size C R`, the window's: the smaller of the terminals it's shown on, and a raw reader with `keys hydra` gets `KEY_RESIZE` as it changes), `ser`, `serctl` (the rate), `wctl` (`new` a window in the writer's window's group, `new group` one in a group of its own, the fid's next read its number; `current N`; it reads as a line a window: its number, group, columns, rows, `*` the shown one; the chrome: `bar top\|bottom\|off`, `bar FORMAT`, `header FORMAT`, `footer FORMAT`, `header on\|off`, `chrome screen\|serial\|both on\|off [bar] [header] [footer]`, `status TEXT`, `monitor on\|off`, `default header\|footer FORMAT`, `default chrome ...`: init writes `/lib/windows`'s lines here as it starts), `wnew`, `kbdin`, `text` (the window's scrollback and screen, as text), `label` (the window's title: OSC 2 sets it too; an empty line, its program's name) |
| `#d` | The disks | `N/data`, `N/ctl`: `0`-`f` cards, `x` the ROM disk, `r` and `s` the RAM disks |
| `#f` | HydraFS | A disk's file system (the spec: its disk) |
| `#S` | SPI devices | `N/data` (a transaction), `N/ctl` (`mode 0`, `mode 3`) |
| `#g`, `#i` | GPIO, I2C | `0`-`7`, `port`, `ctl`, `ca1`; `ctl` and a file each device |
| `#a` | Sound | `snd`, `sndctl`, `bell`, `psg` (the Vera X's PSG) |
| `#v` | The Vera X | `ctl`, `term`, `vram`, `pal`, `sprites`, `font`, `frame`, `psg`, `pcm`, `pcmctl` |
| `#t`, `#n` | Time, null | `time`, `ticks`; `null`, `zero`, `kmesg` |
| `#m` | Modules | A file each (its image), `bin` |
| `#p` | Tasks | `N/status`, `args`, `cwd`, `env`, `ns`, `fd`, `regs`, `mem`, `ram`, `note`, `ctl` |
| `#e`, `#\|` | The environment, pipes | `/env/NAME`; `pipe` |
| `#s`, `#r` | Segments' names, raw RAM | `ctl` and a file each name; `task`, `shared` (init's) |
| `#P` | `/pc` | A folder on the PC, through the PC tool |

## The environment

Each task has its own environment (8K), a copy of its parent's: `ENV_GET`, `ENV_PUT`, `ENV_DEL` and `ENV_NAME` read
and change any task's by number (`$FF`: this one's), and `#e` serves the caller's as files at `/env`.  A value is any
bytes: rc keeps its variables there, a list's words each with a zero byte after it, and its functions as `fn#NAME`.
C's `getenv` and `setenv`, HyForth's `getenv`, and hylang's `$name` are these.
