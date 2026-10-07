# Writing a server or a driver

A server is a task that serves a device letter: its clients' `OPEN`, `READ`, `WRITE`, `STAT` ... of `#x/...` come to
it as requests.  A driver is a server that's a module of the paged ROM, started in a task of its own (the highest
free: task F down), at boot if it's flagged so.  Every server is built on **srvlib** (`sdk/asm/srvlib.inc` at the
top of its source, `srvlib.s` at the end), so a new one is mostly tables: its files, and a routine for each that
makes or takes its contents.

## A server, walked through

`sdk/asm/samples/counter/counter.s` serves `#k`: `count`, a text file (the count), and `ctl` (`add N`, `reset`).

```
.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"                                       ; (srvlib.s at the end)

            HYX2_DRIVER "counter", init, srv_serve, 0, 0, HF_BOOT
```

`HYX2_DRIVER`'s entries: `init` (run once as it starts), the serve entry (srvlib's `srv_serve`, which takes each
request and does the rest), the irq entry and the stop entry (none here), and its flags (`HF_BOOT`: started at
boot, which waits for its init).  `init` registers the letter:

```
init:
            stz         count
            stz         count + 1
            lda         #'k'
            jmp         SRV_REGISTER                        ; (C = 1, .A = an error: it ends)
```

The tree is a table: each entry its name, its parent (an entry's number), its kind, its handler, its mode and an aux
byte:

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

A text file's handler makes its text, each time it's read (srvlib slices it at the read's offset, so the server
keeps nothing between reads); a ctl command's gets its words after it, parsed (`srv_argn` of them, `srv_argp` their
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

## The kinds of entry

| Kind | What it is |
| :--- | :--------- |
| `SK_DIR` | A directory: the entries whose parent it is, read as stat records |
| `SK_TEXT` | A text file: its handler makes the text (`srv_tputs`, `srv_tputc`, `srv_tputdec`, `srv_tputhex`), 255 bytes at most |
| `SK_CTL` | A ctl file: a table of commands; a bad one is `E_INVAL` and changes nothing.  Its aux: the entry of a text file it reads as |
| `SK_DATA` | Anything else: the handler gets each request (`R_OPEN`, `R_READ`, `R_WRITE`, `R_CLUNK` ...) and moves the data itself (`CLIENT_READ`, `CLIENT_WRITE`), setting the count done |
| `SK_DYN` | A directory whose children a handler makes as it's read (the tasks in `/proc`, the modules in `#m`); its aux the entry each child is |
| `SK_RAW` | A device whose every request goes to one handler, its fids its own (a file system: `#f`) |

`SRV_TREES` gives a server several letters (a tree each), as `kdev` has; a request's `RQ_DEV` says which.

## Requests and waiting

A request is a block the client fills in its own memory (`RQ_*`: the request, its fid, mode, offset, count, buffer,
its task and note group); `SRV_TAKE` copies it into the server's `TASK_INBOX`, and `SRV_REPLY` sends back the count
done and a new fid.  srvlib does both.  Data moves by kcopy: `CLIENT_READ` and `CLIENT_WRITE` between the server's
memory and the client's buffer, as each task sees its own.

**A server never waits.**  When it can't answer yet (a pipe that's empty, a console with no line), it answers
`E_AGAIN`, and the kernel has the client wait till the server's event count changes from what it was as the server
took the request, then sends the request again.  So a server adds 1 to its event count (`inc TASK_EVENT`) whenever
something its clients may be waiting for has happened: a byte in, room to send.  A note ends a client's wait: the
server gets `R_FLUSH` (`SRV_FLUSH`, if it keeps anything for a client), the client `E_INTR`.  A server that must give
up on something that doesn't come times it itself: the kernel has no timed wait for a client.

## Interrupts

A driver owns an IRQ line with `IRQ_OWN` (`.A` the line); its irq entry runs in its own task with the line in `.A`,
IRQs off, and returns `.A` = 0, or `IRQ_RESCHED` for a task switch.  It has about 85 cycles: it wakes clients by
adding 1 to its event count (never `WAKE`), and sends a note to a group with `NOTE_QUEUE`.  No stretch of code may
hold IRQs off for more than 200 cycles (a character at 115200 is 320).  A module of more than one bank owns no line.

## Putting it in the ROM

A driver is a folder in `modules/` (`modules/NAME/*.s`), listed in `modules/rom.txt`; `node build.js` builds it with
the rest.  A test of it is a test module (`tests/mod`) that runs as init, with the driver in the ROM
(`tests/tests.js`: a test's `modules`).  `/proc` refuses a driver's memory and registers, and `NOTE` its notes: a
driver's task is the system's.
