## **Writing drivers and file servers**

How to add a device: a file server that tasks open by name, and optionally a driver task with IRQ handlers behind it.  Sources to read alongside:

| Source | What it shows |
| :----- | :------------ |
| `os_rom/io/io.s` | `NULL_SERVE`: the smallest server |
| `os_rom/io/proc_srv.s` | Names, text files and a `ctl` file |
| `os_rom/io/pipe_srv.s` | Waiting and waking |
| `os_rom/io/sd_srv.s` | A server on its own ROM page, with a cache |
| `os_rom/drivers/serial.s` | A driver with IRQ handlers |

Part of the [Programmer's Guide](README.md).

### **The model**

```
  client task                    IO layer (BIOS page 2)                server (its task)
  IO_WRITE fd, buf, n  ------>   fd -> (device, fid, offset)
                                 copy <= 256 bytes of buf into
                                 the client's transfer area
                                 fill in the request block    ------>  serve routine (TASK_CALL)
                                                                       reads the transfer area,
                                                                       does the IO
                                 advance the offset, loop     <------  C = 0 and a count, or an error
```

* **The server** is a **serve routine** that runs in a task: the driver's task, or the calling task.  The IO layer calls it with `TASK_CALL`, so it runs with the server task's zero page, stack and RAM.
* **One request at a time:** a task that calls a busy server waits its turn.  The server can be preempted in the middle of a request like any code.
* **Data** passes through the client's **transfer area** in shared RAM, which the server maps while it works on the request.
* **Requests** follow a small subset of Plan 9's 9P ("H9P").

### **Registering a device**

`DEV_REGISTER` (`$F887`) adds a name to the device table:

| In | |
| :- | :- |
| `.A.Y` | The name: 1-8 characters, e.g. `"null"` for `/dev/null` |
| `.X` | The task the serve routine runs in: usually the driver's own (`T_REGISTER`), or `IO_DEV_CALLER_TASK` (`$FF`) to run it in whichever task makes the request (`/dev/proc`, `/dev/null`) |
| `ZP_TC_VEC` | The serve routine: a **page 0** address.  A server on another page gets a page 0 gate |

It returns the device's index, or `ERR_IO_NAME` (bad or duplicate name) / `ERR_IO_NO_DEVS` (16 devices already).

A driver registers from its `init`, which `DRV_START` runs in the driver's task ([tasks.md](tasks.md#drivers-resident-tasks)).  If the init fails, the task's devices are removed again.

```
; drivers/storage.s: the storage task's init (page 0), for a server on page 3
FAR_GATE_INLINE     SD_SERVE,       PAGE3::SD_SERVE,        3

STORAGE_INIT:
            jsr         STORAGE_INIT3                       ; (its own setup, on page 3)
            bcs         @done
            LOAD_ADDR   SD_SERVE, ZP_TC_VEC                 ; the page 0 gate is the serve routine
            lda         #<SD_NAME                           ; "sd"
            ldy         #>SD_NAME
            ldx         #STORAGE_TASK_NUM
            jmp         DEV_REGISTER
@done:
            rts
```

### **The serve routine**

```
; IN:  .A = request (H9_*), .X = client task, .Y = fid
; OUT: C = 0 (.A = the new fid, for H9_OPEN); or C = 1, .A = error
```

| Request | Value | The server should |
| :------ | :---- | :---------------- |
| `H9_OPEN` | 1 | Look at the name (the rest after the device name, in the data area) and the mode; return a **fid**: a byte of its own choosing that identifies the open file in later requests |
| `H9_READ` | 2 | Put up to *count* bytes from *offset* in the data area; set the count to what it gave (0 = end of file) |
| `H9_WRITE` | 3 | Take up to *count* bytes from the data area at *offset*; set the count to what it took, or leave it for all |
| `H9_CLUNK` | 4 | The fd is closed: release the fid |
| `H9_STAT` | 5 | Put a 16-byte stat block in the data area (`STAT_ZERO` for all zeros) |
| `H9_CTL` | 6 | Device-specific control: the code and argument are in the request block |
| `H9_DUP` | 7 | Another fd now refers to the fid (`IO_DUP2`, or a new task inherited it): count it if it keeps counts |

Return `ERR_IO_BAD_REQ` for requests it doesn't support.

**Fids** are the server's business.  Common patterns:
* one fid per kind of file (`/dev/cons` 0, `/dev/ser` 1, `/dev/ser/ctl` 2);
* a fid that encodes the file (`/dev/proc`: kind `| task`; `/dev/sd`: kind `| card`);
* an index into the server's own table (pipes).

A server that counts its users keeps counts per fid.  It gets an `H9_DUP` for every extra fd, and an `H9_CLUNK` for every close.

### **The request block and the transfer area**

Each task has a 512-byte **IO transfer area** in shared bank ID `$09`, at `$8000 + task * $200` when mapped.  Its request block is at the start, and the data at `+$100`:

| Offset | Field (`include/io.inc`) |
| :----- | :----------------------- |
| 0 | `IO_BLK_TYPE`: the request |
| 1 | `IO_BLK_FID`: the fid |
| 2 | `IO_BLK_MODE`: the fd's mode (on every request, so a server can tell a pipe's ends apart) |
| 3 | `IO_BLK_CLIENT`: the client task |
| 4-7 | `IO_BLK_OFS`: the 32-bit offset (read, write) |
| 8-9 | `IO_BLK_COUNT`: bytes requested (in), bytes done (out); 1-256 |
| 11 | `IO_BLK_CTL_CODE` (`H9_CTL`) |
| 12 | `IO_BLK_CTL_ARG` (`H9_CTL`) |
| `$20-$FF` | The task's namespace (not the server's business) |
| `$100-$1FF` | `IO_BLK_DATA`: the data, or the name for `H9_OPEN` (zero-terminated) |

**Mapping it:**
* `IO_SRV_MAP` (`.X` = the client) maps the client's area into `$8000-$9FFF` in the server's task, and points `ZP_IO_REQ` at its request block (the data is at `ZP_IO_REQ + $100`).
* `IO_SRV_UNMAP` undoes it; call it before returning.
* `IO_SRV_COUNT` sets the count to `.A` and unmaps.  It's on page 2 only; servers elsewhere write the count themselves.

While the area is mapped, the server's own bank at `$8000` isn't: copy through a buffer in task RAM if the server's data is in a bank.

**The smallest server** (`/dev/null`, `io/io.s`):

```
NULL_SERVE:
            cmp         #H9_READ
            beq         @read
            cmp         #H9_STAT
            beq         @stat
            cmp         #H9_CTL
            beq         @bad
@ok:                                                ; H9_OPEN (fid 0), H9_WRITE (all taken), H9_CLUNK, H9_DUP
            lda         #0
            clc
            rts
@read:                                              ; end of file: count = 0
            jsr         IO_SRV_MAP
            lda         #0
            ldy         #IO_BLK_COUNT
            sta         (ZP_IO_REQ),Y
            iny
            sta         (ZP_IO_REQ),Y
            jsr         IO_SRV_UNMAP
            bra         @ok
@stat:
            jsr         STAT_ZERO
            bra         @ok
@bad:
            lda         #ERR_IO_BAD_REQ
            sec
            rts
```

### **Waiting: when there's no data yet**

A server that can't complete a request now:

1. **Remembers the client.**  `.X` on entry is the client; the usual way is to set its bit in a 16-bit wait mask.
2. **Returns** C = 1 with `.A` = `ERR_IO_WOULD_BLOCK`.  The IO layer marks the client waiting, and it sleeps (no CPU).  A non-blocking fd gets the error instead.
3. **Wakes the client** when data comes in, usually in the driver's IRQ handler: it calls `IO_WAKE` (`.A` = task) for each task in the mask, and clears the mask.  The IO layer then sends the **same request again**.

Waking a task that has already been woken, or isn't waiting any more, is harmless: it just tries again.  The IO layer holds off task switches between marking the client waiting and yielding, so a wake can't be lost.

### **Names and ctl files**

**Names.**  For `H9_OPEN`, the data area holds the rest of the name after the device.  Opening `/dev/proc/3/ctl` sends `/3/ctl` to the `proc` server, and a mount sends what follows the mount point.  A server walks it however it likes: `proc_srv.s` and `sd_srv.s` parse `/N/name`.

**ctl files.**  A text control file is friendlier than `IO_CTL` codes, because anything that can write text can use it (HyForth, a script).  Examples:
* `/dev/proc/N/ctl` takes `kill`, `break`, `fg`;
* `/dev/sd/N/ctl` takes `init`, and reads back the card's details;
* `/dev/ser/ctl` takes `b9600 l8 pn s1`-style settings, and reads back the current ones (`io/serctl.s`).

The parsing in `PROC_WRITE` / `SD_CTL_WRITE` takes a command word followed by the end of the write, a space, CR, LF or 0.

**Text reads.**  A server that returns generated text (a status line) makes the text in the data area for each read, then hands over what's after the fd's offset, up to the count.  `PROC_READ` and `SD_CTL_READ` do this, so `cat` gets the text once and then end of file.

### **Checklist for a new device**

1. **Choose where it runs:**
   * a driver task, for a device with state or IRQs;
   * or the calling task (`IO_DEV_CALLER_TASK`), for a stateless one like `/dev/proc`.
2. **Write the serve routine**, on the page that suits it, with a page 0 gate if it isn't on page 0.
3. **Write the driver's `DriverInfo` and `init`:**
   * set up the hardware;
   * `IRQ_REGISTER` its handler ([interrupts.md](interrupts.md));
   * `DEV_REGISTER` its names.
4. **Start it at boot:** `DRV_BOOT` in `kernel/os_main.s`, in a free task.  Task numbers `$2-$B` are free; drivers so far use `$C-$F`, downwards.
5. **Add HyForth words** if it's useful from the shell (`hyforth/hywords.s`: `open`, `read`, `write` and `ioctl` already reach any device).
6. **Add a regression test** that opens, reads and writes it in the emulator ([emulator](../tools/emulator.md#regression-tests)), and model the device in `sim/hydrasim.js` if it's new hardware.

### **The storage layer (for the filesystem server)**

The storage task (`$C`) owns the SPI bus.  Its block layer (`drivers/sd.s`, page 3) works on card `SD_DEV` (0-7), in the storage task's zero page:

| Routine | Does |
| :------ | :--- |
| `SD_INIT` | Start the card: SDHC/SDXC (block addresses) or SDSC (byte addresses).  Reads its size from the CSD register into `SD_CARD_BLOCKS` |
| `SD_READ_BLOCK` | Read block `SD_LBA` (32 bits) into the 512 bytes at `SD_BUF` |
| `SD_WRITE_BLOCK` | Write the 512 bytes at `SD_BUF` to block `SD_LBA` |

**Errors:** `ERR_IO_DEVICE` (no card, or no answer), `ERR_IO_NOT_READY` (not started), `ERR_IO_MEDIA` (the card refused).

**The cache:** `sd_srv.s` keeps a one-block cache (`SD_CACHE_LOAD`: block `SD_LBA` of card `SD_DEV`).  The HydraFS server ([plans/HYDRAFS.md](../plans/HYDRAFS.md)) will be a second device in the same task, on the same block layer.
