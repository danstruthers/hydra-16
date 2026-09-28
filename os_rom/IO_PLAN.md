## **Hydra IO Plan**

A Plan 9-style IO subsystem: **all IO goes through file handles.**  Devices, drivers and (later) filesystems are *file servers*; a task opens a name, gets a file descriptor, and reads and writes it.  The console, the serial port and the sound chip are files; so, later, are an SD card filesystem, pipes between tasks, and per-task information.

This plan builds on the MMU work (`MMU_PLAN.md`): tasks, driver tasks, `TASK_CALL`, the IRQ dispatcher and shared memory.

### **Goals**
* One interface for all IO: `open`, `read`, `write`, `close` (plus `stat` and `seek`), the same for every device and file.
* Drivers are file servers running in their own (resident) tasks, so a driver's state and IRQ handlers stay in its task.
* A task blocks (waits) on IO instead of busy-waiting, once there is a scheduler.
* Per-task names (Plan 9 namespaces) in a later phase; a global `/dev` to start with.
* Small enough for a 65C02: fixed-size tables, 1-byte handles, no dynamic allocation in the IO path.

### **Plan 9 ideas, and what they become on the Hydra**

| Plan 9 | Hydra |
| :----- | :---- |
| Everything is a file; devices are file servers | Drivers register as file servers (`DEV_REGISTER`); `/dev/cons`, `/dev/ser`, `/dev/snd`, ... |
| Per-process file descriptors (0 = stdin, 1 = stdout, 2 = stderr) | Per-task fd table in the task system page; fds 0-2 inherited by spawned tasks |
| 9P protocol between the kernel and file servers | "H9P": a small request block (open, read, write, clunk, stat, ...) passed to the server's `serve` routine |
| The kernel moves data between the client and the server | Each task has a 256-byte **IO transfer area** in shared RAM; the server reads/writes it |
| `iounit`: the largest read/write per message | 256 bytes; `IO_READ`/`IO_WRITE` split bigger transfers |
| Per-process namespace, `bind` and `mount` | Phase 3: a per-task mount table (inherited on spawn) |
| A read with no data blocks the process | The task sets "Awaiting I/O" and yields; the server wakes it (needs the scheduler) |
| Notes (signals) | Later: `interrupt` / `kill` notes (the console keys' break and kill are a first step: `TASK_SET_BREAK`) |

### **Architecture**

```
  client task                    IO layer (BIOS page 2)                server (driver task)
  -----------                    ----------------------                --------------------
  IO_WRITE fd, buf, n  ------>   fd -> (server, fid, offset)
                                 copy <= 256 bytes of buf into
                                 the task's IO transfer area
                                 fill in the H9P request     ------>  TASK_CALL serve (in its task)
                                                                       reads the transfer area,
                                                                       does the IO
                                 advance offset, loop        <------  C = 0, count  (or error / "would block")
```

* **Transport:** requests go to the server with `TASK_CALL` (a procedure call into the server's task, like the IRQ dispatcher uses), not as messages through the rings.  It's synchronous, and the server still runs with its own ZP, stack and RAM.  The server can be preempted in the middle of a request like any task (so a long one, e.g. an SD card block, doesn't hold up the others), and serves one call at a time: another task that calls it meanwhile waits (see step 2b).  The request is a real H9P message in memory, so a later message-based transport (or a remote one) can reuse the servers unchanged.
* **Data:** tasks have separate RAM, so data goes through the client's IO transfer area in shared RAM, which both tasks can map.  The IO layer copies between the caller's buffer and the transfer area (`MM_*` / `SH_*` style bank switching, IRQ-safe).
* **Where the code lives:** the IO layer and the device table code on BIOS ROM **page 2** (`W = 2`), reached through gates and thunks; page 0 is nearly full (about 370 bytes free), so new code goes on other pages behind gates (page 1 about 2.4K free, page 2 about 2.3K, page 3 about 6.9K, page 4 about 4.3K, pages 5-F empty).  Server `serve` routines live with their drivers.

### **File descriptors**
The fd table lives in the task system page, after the IRQ tables: **`$7DA0-$7DFF`, 12 fds x 8 bytes.**

| Offset | Size | Field |
| :----- | :--- | :---- |
| 0 | 1 | Server (device table index; `$FF` = fd not open) |
| 1 | 1 | Fid: the server's own handle for the open file |
| 2 | 1 | Mode: read / write / read-write, flags (e.g. append) |
| 3 | 1 | Reserved |
| 4 | 4 | Offset (32-bit, for filesystems; devices ignore it) |

* **fd 0, 1, 2** are stdin, stdout and stderr.  The shell starts with all three on `/dev/cons`.  `SPAWN_TASK`/`TASK_PREPARE` copy the parent's open fds (and tell each server with a `dup`-style H9P request so its reference counts stay right).
* `MM_TASK_RESET` closes every fd of the task (a clunk to each server), so a finished task can't leak open files.

### **Names**
The layout below is Plan 9-style for now.  It may change later: once the base IO system works, the filesystem layout will be revisited (a separate design document will describe the ideas).

* **Phase 1: a global `/dev`.**  `DEV_REGISTER` (called from a driver's `init`, in its task) adds `name -> (task, serve routine)` to the device table in shared bank ID `$00` (`$8800-$88FF`, 16 devices).  `IO_OPEN "/dev/cons"` finds `cons` in the table.  Names are short (up to 8 characters) and case-sensitive.
* **Phase 3: per-task namespaces (done).**  A mount table per task, inherited on spawn: 7 entries of 32 bytes in the task's IO transfer area (`IO_BLK_NS`, next to the data area names are resolved in; layout in `include/io.inc`).  `IO_MOUNT` attaches a device's server at a path (e.g. the SD card's HydraFS server at `/sd`); `IO_BIND` makes one name refer to another.  `IO_OPEN` applies the longest matching prefix (whole path elements), rewriting binds (up to 4, `ERR_IO_NS_LOOP` beyond), and falls back to the global `/dev`.  The server that owns a mount gets the rest of the name (`/games/star.frt`) to walk.  (A mount names a device, not a fid within one: mounting a server's subdirectory can come with the filesystem servers.)

### **H9P: the request block**
The first 16 bytes of the task's IO transfer area; the data (up to 256 bytes) follows.

| Byte | Field |
| :--- | :---- |
| 0 | Type (below) |
| 1 | Fid |
| 2 | Mode (open) |
| 3 | Client task |
| 4-7 | Offset (read / write) |
| 8-9 | Count: bytes requested (in) / done (out) |
| 10 | Error (out) |
| 11-15 | Reserved |

| Type | Request | Plan 9 |
| :--- | :------ | :----- |
| `H9_OPEN` | name (in the data area), mode -> fid | walk + open |
| `H9_CREATE` | name, mode -> fid (filesystems) | create |
| `H9_READ` | fid, offset, count -> data, count | read |
| `H9_WRITE` | fid, offset, count, data -> count | write |
| `H9_CLUNK` | fid: close | clunk |
| `H9_STAT` | fid -> size, type, flags (in the data area) | stat |
| `H9_DUP` | fid -> another reference (fd inherited by a child) | (kernel) |
| `H9_FLUSH` | cancel a blocked request | flush |
| `H9_CTL` | device-specific control (baud rate, capture, ...) | writes to a `ctl` file |

Servers return C = 0 with the count (or fid), or C = 1 with an error: `ERR_IO_NOT_FOUND`, `ERR_IO_BAD_FD`, `ERR_IO_MODE`, `ERR_IO_WOULD_BLOCK`, `ERR_IO_EOF`, ...  Plan 9 uses `ctl` files for control; a Hydra device can offer both `H9_CTL` and a `/dev/<name>ctl` file.

### **IO transfer areas**
Shared bank ID **`$09`** (reserved at boot): **512 bytes per task** at `$8000 + task * $200` (16 tasks = 8K).  `$00-$0F` is the request block, `$100-$1FF` the data (256 bytes = the iounit).  Only the IO layer and the server handling the task's request touch a task's area, one request at a time per task.

### **Blocking and the scheduler**
Reads often have to wait (no key pressed yet).  So this plan starts with the scheduler:
* **Preemptive scheduler:** the VIA T1 timer IRQ (the system task's `VIA_IRQ_HANDLER`) sets a reschedule flag; at the end of `IRQ_DISPATCH`, if it's set, the dispatcher switches tasks (`SCHED_SWITCH`, which picks the next runnable one: `SCHED_PICK`) instead of returning to the interrupted task.  Task switches save and restore `W` per task (a task can be interrupted while on ROM page 1 or 2).  `YIELD` switches voluntarily.
* **Critical paths (holding the CPU):** two ways, depending on how long.
    * **`sei` ... `cli` (or `php`/`sei` ... `plp`):** no IRQs at all, so no task switch.  Only for short sections (tens of cycles): it also holds off serial RX (a byte arrives every ~0.5 ms at 19200 baud, and the ACIA holds only one), the timer tick and sound.  This is what the MMU, shared memory and message code already do.
    * **`NO_PREEMPT` ... `PREEMPT`:** a nestable per-task counter.  While it's non-zero, IRQs and their handlers run normally (drivers keep up), but the dispatcher doesn't switch tasks; it only notes that a switch is due.  `PREEMPT` brings the count back down, and switches right away if one was due.  For longer sections, like timing-sensitive IO or updating several related structures.  A task that blocks on IO (or calls `YIELD`) while holding it still gives up the CPU, since it can't make progress anyway.
* **Blocking IO:** a server that can't finish a request returns `ERR_IO_WOULD_BLOCK` and remembers the waiting task.  The IO layer marks the task "Awaiting I/O" (`TASK_STATUS_REG` bit 2) and yields; the scheduler skips waiting tasks.  When data arrives (usually in the server's IRQ handler), the server calls `IO_WAKE task`, and the request is retried.
* **Non-blocking mode** (`O_NONBLOCK` in the fd mode) returns `ERR_IO_WOULD_BLOCK` to the caller instead, which is how `READ_CHAR`'s "is there a key?" test keeps working.
* Until the scheduler exists, a blocked request just waits (`wai`) and retries.

### **Clock speed and wait states**
The CPU runs at 3.58 MHz; the board can also run it at 7.16 MHz (the W65C02S goes to 14 MHz).  Some devices can't keep up with a faster bus: the YM2151 runs on its own 3.58 MHz clock, and slow 65C51/65C22 grades and ROMs have similar limits.

* **Now (board V1): a build-time clock setting.**  `CPU_CLOCK_MULT` in `include/hw.inc` (1 = 3.58 MHz, 2 = 7.16 MHz); every timing constant is derived from it: the scheduler tick (`TIMER_TASK_INT` = 5 ms), `YM_TIMEOUT` and `YM_DELAY_64` (note lengths), and the self test delays.  There is no wait-state hardware, so above 3.58 MHz the sound chip mustn't be used (software can space out accesses, but can't stretch a bus cycle).
    * SPI is bit-banged, so its clock speeds up with the CPU.  At 7.16 MHz the SD card's initialisation clock would be about 600 kHz (the limit is 400 kHz), so the SPI code will need a clock-dependent delay before the SD card server (Phase 3).
* **Board V2: RDY wait states** (the hardware design, to be finalised with V2; possibly available earlier on an expansion card; see also `IDEAS.md`, which also records the alternative of switching the clock divider):
    * A **wait table per I/O port or memory area**, whose entry for the accessed port/area is copied into a **RDY hold counter**; RDY is held low while the counter counts down, and released at zero.
    * A new pseudo-register, **`$FFF9`**, is the interaction point: the value the RDY hold counter loads from.  It is **initialised to 15 at boot** (the maximum wait), so everything is slow and safe until the ROM sets the table up.
    * A clock jumper register to read the CPU clock from, replacing the build-time setting.
    * RDY has to be open-drain with a pull-up: the W65C02S drives it low itself during `WAI` (the ROM uses `WAI`).
* **OS support (with V2):** set the wait table first thing at reset, before the self test touches the ACIA; per-device defaults for the on-board devices; drivers declare their port's wait states (a `DriverInfo` field that `DRV_START` programs) and an `IO_WAIT_SET` call; the self test reports the clock and the table; timing constants read the clock register instead of `CPU_CLOCK_MULT`.

### **Servers**

| Name | Server | Notes |
| :--- | :----- | :---- |
| `/dev/cons` | Serial driver (task `$F`) | The console: read = keyboard, write = screen.  Replaces the "serial-capture task": whoever reads `cons` gets the input |
| `/dev/ser`, `/dev/serctl` | Serial driver | Raw serial port; control (baud rate, echo) |
| `/dev/snd` | Sound driver (task `$E`) | Write YM2151 register/value pairs; `IO_CTL` `SND_CTL_INIT` / `SND_CTL_TEST` (the test tune, played by a background player task) / `SND_CTL_STOP` (instead of a `/dev/sndctl` file); HyForth's sound words use it |
| `/dev/null`, `/dev/zero` | IO layer | The usual |
| `/dev/sd` | Storage task (`$C`) | The SD card as bytes (block cache, writes straight through) |
| Later: `/sd/...` | SD card filesystem server (HydraFS) | Uses `SPI` in the BIOS; first real use of offsets, `H9_CREATE` and directories |
| `/dev/pipe` | Pipe server (task `$D`) | `IO_PIPE`: two fds on a 255-byte ring in the pipe task's RAM (8 pipes); end of file when the writers are gone.  Later: `|` in the shell |
| Later: `/proc/<task>/...`, `/env/...` | System servers | Task status, memory use, notes; per-task environment variables |

**Serial driver changes:** it becomes a file server.  RX bytes go into a buffer in its own task RAM (instead of the capture task's message ring); `H9_READ` on `cons` takes from it, blocking when it's empty.  TX goes through a TX buffer drained by the ACIA's transmit IRQ, so `WRITE_CHAR` no longer busy-waits.  `READ_CHAR`/`WRITE_CHAR` (and the `$F800` thunks) become one-byte reads and writes on fd 0 / fd 1, so WOZMON and HyForth work unchanged, and follow redirection.

### **API**
All calls: C = 0 on success, C = 1 with the error in .A.  Thunks after `$F853`.

| Call | In | Out |
| :--- | :- | :-- |
| `IO_OPEN` | .A.Y = zero-terminated name, .X = mode | .A = fd |
| `IO_CLOSE` | .A = fd | |
| `IO_READ` | .A = fd, `ZP_IO_BUF` = buffer, `ZP_IO_CNT` = count | `ZP_IO_CNT` = bytes read (0 = end of file) |
| `IO_WRITE` | .A = fd, `ZP_IO_BUF` = buffer, `ZP_IO_CNT` = count | `ZP_IO_CNT` = bytes written |
| `IO_GETC` / `IO_PUTC` | .X = fd (.A = byte for put) | .A = byte (get) |
| `IO_SEEK` | .A = fd, `ZP_IO_OFS` = offset | |
| `IO_STAT` | .A = fd, `ZP_IO_BUF` = 16-byte buffer | stat block |
| `IO_DUP2` | .A = fd, .X = new fd | redirection (e.g. stdout to a file) |
| `IO_DUP` | .A = fd | .A = a new fd (the lowest free) for the same file |
| `IO_PIPE` | | .A = read fd, .X = write fd |
| `IO_MOUNT` | .A.Y = path, `ZP_IO_BUF` = device name | a mount in the task's namespace |
| `IO_BIND` | .A.Y = path, `ZP_IO_BUF` = target path | a bind in the task's namespace |
| `IO_UNMOUNT` | .A.Y = path | removes the path's entry |
| `IO_NS_LIST` | | prints the task's namespace |
| `TASK_CLONE` | .A.Y = entry, .X = its ROM page | .A = a new task: a copy of this one (fork) |
| `GET_CHAR` | | .A = a key from fd 0 (waits) |
| `IO_CTL` | .A = fd, .X = control code, .Y = argument | device-specific |
| `DEV_REGISTER` | .A.Y = name, `ZP_TC_VEC` = serve routine | registers the calling (driver) task |
| `IO_WAKE` | .A = task | server: a blocked request can be retried |
| `YIELD` | | give up the CPU (scheduler) |
| `TASK_SLEEP` | .A.Y = ticks (200 a second, up to 32767) | sleeps that long (the other tasks run, or the system idles) |
| `TASK_SLEEP_UNTIL` | .A.Y = a tick count (`TICKS_GET`) | sleeps until then |
| `NO_PREEMPT` / `PREEMPT` | | hold the CPU (nestable) while IRQs keep running; see Critical paths |

**HyForth words:** `open ( sz mode -- fd )` (the name is a HyForth `q^...^` string, e.g. `q^/dev/cons^ 3 open`), `close ( fd -- )`, `read ( fd addr n -- n' )`, `write ( fd addr n -- n' )`, `ioctl ( fd code arg -- )`, `ioerr ( -- n )` (a failed call gives `!IO ERR!`; `ioerr` is the IO layer's error code); `emit`/`key` on fds 1 and 0.  Later: `type`, and a `redirect ( fd -- )` for output to a file.

### **Build order**
**Phase 1 - Scheduler (done)**
1. **(Done)** One frame format for every switched-out task (the IRQ dispatcher's frame plus `U`, so `W` and `U` follow each task); `SCHED_SWITCH`/`SCHED_PICK` (round-robin over tasks 1-15, task 0 idle); `YIELD`; `NO_PREEMPT`/`PREEMPT`; the VIA T1 tick (the VIA handler returns a reschedule marker) and preemption at the end of `IRQ_DISPATCH`, never while a task runs a `TASK_CALL` routine for another task (`ZP_TC_GUEST`).  `TASK_START` (spawn and wait) and the new `TASK_RUN` (background) start tasks through `TASK_TRAMPOLINE`/`TASK_EXIT`.  `SWITCH_TO`/`NEXT_TASK` are gone.
2. **(Done)** `TASK_WAIT`/`IO_WAKE` (the "Awaiting I/O" bit), `TASK_STATUS`; `SCHED_TEST` (`$F869`): interleaved tasks, a `NO_PREEMPT` block, wait/wake.
2b. **(Done)** Preemptible servers and sleeping.  A `TASK_CALL` routine (a server's serve routine, a new task copying itself in for `TASK_CLONE`) can be switched out like any code: the caller is marked in a call (`TASK_STATUS_REG` bit 6, `TASK_CALLING_FLAG`) and isn't run until the call returns; the server, switched out mid-call, gets bit 7 (`TASK_GUEST_OUT_FLAG`), which makes it runnable (even resident or paused) until it's resumed, and a break or kill waits until it's back in its own code.  A server serves one call at a time: a task that calls it while it's busy waits in `TC_WAIT_FREE` (the server's `ZP_TC_WAITERS` mask, woken by `TASK_WAKE_MASK` when the call returns); the IRQ dispatcher's calls (`TASK_CALL_IRQ`) run at once, on the stack below the switched-out call.  A server can now also wait itself (`TASK_WAIT`, or an IO request of its own to another server) in the middle of a request.  `TASK_SLEEP` (ticks) / `TASK_SLEEP_UNTIL` (a tick count), HyForth `sleep`: the task waits, and the system task's tick handler wakes it (`SLEEP_CHECK`, the `ZP_SLEEPERS` mask); the test tune sleeps between notes instead of spinning on `YIELD`, so the system idles (`wai`) while it plays.

**Phase 2 - IO core and the console**
3. **(Done)** BIOS ROM page 2 (`page2.s`, `io.s`) with its gates and thunks (`$F86C-$F88A`); compact 6-byte gates (`FAR_GATE_INLINE`, via `FAR_INLINE` in the COMMON block); the IO transfer bank (`$09`); the fd tables (`$7DA0`, set up by `TASKS_INIT`); the device table (`$8800`) and `DEV_REGISTER`, `IO_SRV_MAP`/`IO_SRV_UNMAP`, `IO_INIT` (`io_p0.s`, page 0).  Page 0 is nearly full (about 55 bytes left): converting its older 15-byte gates to compact ones would free a few hundred bytes.
4. **(Done)** `IO_OPEN`/`CLOSE`/`READ`/`WRITE`/`GETC`/`PUTC`/`SEEK`/`STAT`/`CTL`, H9P dispatch with race-free blocking (the task marks itself waiting before calling the server), `/dev/null` and `/dev/zero`, and `IO_TEST` (`$F88A`).
5. **(Done)** The serial driver as the `cons`/`ser` server (`serial.s`: init, IRQ handler, `SER_TX_TRY`; `ser_srv.s` on page 2: the requests), with 256-byte RX and TX rings in its task and IRQ-driven TX (Rockwell TDRE interrupt); `cons` reads only for the foreground task (`SER_CTL_FOREGROUND`, `SER_CALL_SET_CAPTURE`), others wait; readers and writers wait (not spin) on an empty RX / full TX ring.  `READ_CHAR` (non-blocking, as before) / `WRITE_CHAR` on fds 0 and 1, or the rings directly for tasks without them; `IO_STD_OPEN` gives the shell fds 0-2; `TASK_BUILD_FRAME` copies the parent's open fds (`IO_INHERIT`); `MM_TASK_RESET` closes fds (`IO_CLOSE_ALL`).  `IO_WRITE` now offers a server the rest after a short write.  The serial-capture message ring is gone.  To make room on page 0, SPI moved to page 2.  Not done yet: `/dev/serctl`, `H9_DUP` (no server counts references yet), and making blocking reads the default for `key` (step 6).
6. **(Done)** HyForth words `open`, `close`, `read`, `write`, `ioctl`, `ioerr` (error `!IO ERR!`); `emit`/`key` through fds 1 and 0.  `GET_CHAR` (`$F88D`): wait for a key on fd 0 (echoed), sleeping instead of polling; HyForth's line input and `key`, and WOZMON, use it, so an idle shell doesn't use the CPU.

**Phase 3 - More servers and names**
7. **(Done)** `/dev/snd` (`snd_srv.s`: register/value pairs, `IO_CTL` init and test).  `H9_DUP`, sent by `IO_DUP2` and `IO_INHERIT`; every request now carries the fd's mode (`IO_BLK_MODE`).  `IO_DUP2` (`$F890`, HyForth `fdup2`) for redirection.  Pipes (`pipe_srv.s`, a Resident task `$D`): `IO_PIPE` (`$F893`, HyForth `pipe`) opens the read end and adds a write end (`IO_FD_COPY` with the write mode); the server counts each end's fds, readers and writers wait for each other, a reader gets end of file when no writers are left, a writer `ERR_IO_BROKEN` when no readers are.  `IO_TEST` checks a pipe between two tasks (a child writes 600 bytes).  Fixed: `IO_SERVE` holds `NO_PREEMPT` from marking the task waiting until it yields or is done; a task switch in between left it waiting with nobody to wake it.  (`|` in the shell: step 7b.)
7b. **(Done)** `|` in the shell: HyForth splits a line at each `|` (outside `q^...^` strings); each stage but the last runs in a copy of the shell's task (`TASK_CLONE`, like fork: task RAM, task ZP, namespace and fds; HyForth's `child_start`) with stdout into a pipe, and ends at the end of its part of the line; the shell runs the last stage with stdin from the pipe, and puts stdin back at the end of the line.  HyForth `cat` and `wc ( -- lines words chars )`; `key` gives -1 at end of file.  `/dev/cons` does the echo now (not `READ_CHAR`/`GET_CHAR`), so piped input isn't echoed; `WRITE_CHAR` drops output when stdout fails (a pipe nobody reads).  `IO_DUP`.
7c. **(Done)** Console control keys.  Ctrl-D / Ctrl-Z: end of input (`/dev/cons` returns end of file; `SER_KEY_EOF`, `SER_KEY_EOF2`).  Ctrl-C / Ctrl-\: break / kill, acted on by the serial IRQ handler as the key arrives (`SER_BREAK`): the foreground task gets `TASK_BREAK_FLAG` or `TASK_KILL_FLAG` and the tasks it started (`ZP_TASK_OWNER`, set by `TASK_BUILD_FRAME`) the kill flag, and they stop waiting; when the scheduler next resumes a flagged task (`SCHED_RESUME`), it rewrites its frame to continue at `BREAK_ENTRY`, which cleans up (waiting, `NO_PREEMPT`, the RAM bank) and goes to the task's break handler (`TASK_SET_BREAK`, `$F8A8`: HyForth's goes back to the prompt) or ends the task (the shell starts again).  A precursor to Plan 9 notes.  (Page 0 room: the sound test tune moved to page 2, `snd_test.s`.)
8. **(Done)** Per-task namespaces (`ns.s`): `IO_MOUNT`, `IO_BIND`, `IO_UNMOUNT`, `IO_NS_LIST` (HyForth `mount`, `bind`, `unmount`, `ns`), inherited by new tasks and cleared when a task ends; `IO_OPEN` resolves names through them.  Servers that walk longer paths come with the filesystem (step 9).
9. The SD card filesystem server (`/sd`): HydraFS (`HYDRAFS.md`).
    * **(Done)** The storage layer, on BIOS ROM page 3 (`page3.s`, `spi.s`, `sd.s`, `sd_srv.s`; `storage.s` on page 0), in a Resident storage task (`$C`) that owns the SPI bus.  SPI: bit-banged on the VIA's port B, mode 0, device select through the board's 74HC138 (device 0 = header J18; `SD_SPI_DEVICE`).  SD card: `SD_INIT` (CMD0, CMD8, ACMD41, CMD58: SDHC/SDXC block addresses or SDSC byte addresses), `SD_READ_BLOCK` / `SD_WRITE_BLOCK` (512-byte blocks, with timeouts).  `/dev/sd`: the card as bytes at the fd's offset (IO_SEEK; the first 4 GB), through a one-block cache, writes straight through; the card starts at the first open (`ERR_IO_DEVICE` without one), `SD_CTL_INIT` starts it again.  The emulator has an SD card on device 0 (`--sd image`).  HyForth `seek`.
    * **Next**: the HydraFS server (`HYDRAFS.md`), in the storage task on the block layer (so it reaches the whole card): directories, reading and writing files; mounted at `/sd`.  (HydraFS was chosen over FAT32: see `HYDRAFS.md`.)
10. `/proc` and `/env`; notes.
    * **(Done)** `/dev/proc` (`proc_srv.s`, page 2, served in the reading task): the task list, `/dev/proc/N/status`, and `/dev/proc/N/ctl` (`kill`, `break`, `fg`).  Notes, so far: `TASK_SIGNAL` (a break or a kill for a task, and a kill for the tasks it started), used by the console keys, the ctl files and HyForth's `kill`.  Task switching: `CONS_SET_FG` brings a task to the front (HyForth `fg`, the ctl files, Ctrl-] then the task); only the foreground task and the tasks it started write to `/dev/cons` (the others wait, like Unix job control); a foreground task that ends hands the console to the task that started it (`CONS_RELEASE`).  HyForth `shell` starts another shell, `ps` lists the tasks.
    * **(Done)** stdio buffering: `WRITE_CHAR` buffers stdout and `GET_CHAR` / `READ_CHAR` read stdin ahead (128 bytes each, page `$07` of each task) when the fd isn't the console (`STDOUT_PUT`, `STDIN_GET`, `IO_FLUSH` in `io.s`), with fast paths on pages 0 and 1 (`_M_STDOUT_FAST`, `_M_STDIN_FAST`) so a buffered byte takes no far call.  `words | wc` went from about 18M to 2M cycles.  (Found with the emulator's `--profile`.)
    * **Next**: `/env`; more notes (a note text, a handler per note, like Plan 9's `notify`).

### **Decisions**
* **Transport:** direct calls (`TASK_CALL` into the server's task), preemptible, one call at a time per server.  The H9P block stays a real message in memory, so a message-based transport (requests through the rings, servers as scheduled tasks) can be added later without changing the servers.
* **Scheduler:** preemptive, round-robin on the VIA T1 tick (`TIMER_TASK_INT_H/L` in `include/kernel.inc`: about 5 ms).  A task can hold the CPU with `sei`/`cli` for short sections, or `NO_PREEMPT`/`PREEMPT` for longer ones (IRQs keep running).
* **Sizes:** 12 fds per task, 16 devices, a 256-byte iounit, 8-character device names, 32-bit offsets.
* **Console ownership:** keyboard input from `/dev/cons` goes to the **foreground task** only, set by the shell (`IO_CTL` `SER_CTL_FOREGROUND` on a `cons` fd, or `SER_CALL_SET_CAPTURE`).  Other tasks reading `cons` block until they're brought to the foreground; any task may write to it.
