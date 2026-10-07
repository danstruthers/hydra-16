## **/pc: a folder on the PC, over the serial port**

*Built*, in the old system and in reborn (version 2 of the protocol: [below](#version-2-reborns)).  A folder on the PC is a file tree on the Hydra, at `/pc`, served over the serial port the console already uses.  A program built with cc65 on the PC runs on the Hydra at once (`/pc/bin/game`), with no card to carry across; a file the Hydra writes there is on the PC.  The PC side is a Node.js program that is also the terminal, so one cable and one window do both.  Part of [NEXT_STEPS.md](NEXT_STEPS.md) (storage and names, step 6).

### **How it's used**

On the PC (once, in `sim/`: `npm install`, for the `serialport` package):

```
node sim/tools/hydrapc.js COM3 C:\hydra             the terminal, with C:\hydra served as /pc
node sim/tools/hydrapc.js COM3 C:\hydra --read-only  the Hydra can read it, not change it
node sim/tools/hydrapc.js --list                    the serial ports
```

It's a terminal like any other: what the Hydra prints shows, the keys typed go to it.  Ctrl-A is the tool's own prefix key: Ctrl-A x quits, Ctrl-A l lists the requests as they're served, Ctrl-A Ctrl-A sends a Ctrl-A.  The line runs at 9600 baud (the Hydra's at boot; `--baud` for another).

On the Hydra, `/pc` is there from boot (`/rom/lib/namespace` mounts it in the system namespace: `mount -s pc /pc`), and works as a card's directory does:

```
ls /pc                      the folder
cat /pc/notes.txt
cp /pc/game.hyx /ram/bin    a program into this shell's cache
/pc/game                    or run it where it is
bind -a /pc/bin /bin        the folder's bin in /bin's union: its programs by name
echo hi > /pc/out.txt       a file on the PC
cd /pc/src
```

`ls -l` shows the PC's sizes and times; `mkdir`, `rm`, `rmdir` and `mv` work; a `.hys` script runs from it; a pipeline's stages can use it at once.  With `--read-only`, a write, create, remove or rename is refused (`not allowed`).  With no PC tool on the line, a request on `/pc` fails after a second (`no answer`), and the attach frame's 7 bytes show on the terminal.

The emulator plays the PC tool's part: `node sim/hydrasim.js -i --pc-dir C:\hydra` ([emulator](../../../../old/docs/tools/emulator.md)).

### **The design**

**Plan 9's way.**  `/pc` is a file server like any other: the device `pc` (`/dev/pc`), mounted at a path.  The IO layer already speaks a small 9P (H9) to its servers; `/pc`'s server sends each request on to the PC as it is, and gives back the PC's answer.  So the PC does the work (`sim/tools/pcfs.js`, which answers the way HydraFS does), and the Hydra's side stays small: about 900 bytes on BIOS ROM page D, 250 on page 2.

**One line, two streams.**  The console's bytes and `/pc`'s frames share the serial line.  A frame starts with a mark byte, `$1E`, and inside it `$1E` and `$1F` are escaped (`$1F`, then the byte `^ $20`), so a `$1E` always starts a frame, and a frame cut short is simply dropped at the next one.  The PC escapes Ctrl-C, Ctrl-\ and Ctrl-] too (`$03`, `$1C`, `$1D`: the keys reborn's console acts on as they come in), so its frames never hold one; every reader takes any byte after `$1F` as that byte `^ $20`, so the old system takes them as well.  The PC tool picks the Hydra's frames out of what it prints; the Hydra's fast serial handler picks the PC's out of what it receives, before the console sees them.  A key the user types that is `$1E` (Ctrl-^) goes as `$1E $1F`, which the Hydra takes as the key.

| Frame | Bytes |
| :---- | :---- |
| The mark | `$1E` |
| Type | `A` attach, `Q` request (the Hydra's); `R` reply, `N` resend (the PC's) |
| Tag | The request's number: its reply carries it back |
| Length | The payload's, 2 bytes (low first) |
| Payload | Up to 288 bytes |
| CRC | CRC-16 (CCITT: `$1021`, from `$FFFF`; low byte first) of the type to the payload's end |

| Payload | Holds |
| :------ | :---- |
| `Q` | The request block's first 16 bytes (the request, fid, mode, client, offset, count, the ctl and create bytes; [servers.md](../../../../old/docs/programming/servers.md#the-request-block-and-the-transfer-area)), then its data: an open's, create's or remove's name, a write's bytes, a wstat's record |
| `R` | Status (0, or the Hydra's error code), a value (an open's fid), the count (2 bytes: a read's or a write's), then data (a read's bytes, a stat record) |
| `A` | The protocol's version: 1, the old system's (these payloads); 2, reborn's ([below](#version-2-reborns)).  The PC closes the files it had open: a new session |

**The Hydra's side** (`os_rom/servers/pc_srv.s`, page D; `servers/serfast.s`, page 2):
* **Where it runs.**  The device is served in the serial task, which holds both frame buffers (`PC_TXBUF`, `PC_RXBUF`, in its RAM) and the port's state.  Its serve routine never waits: it sends a frame and returns `ERR_IO_WOULD_BLOCK`, so the console's own requests keep going while the PC answers.
* **Sending.**  The serve routine copies the request into a frame and starts it; the fast handler (`SER_TX_STEP`) sends the whole frame, escaping as it goes, ahead of the TX ring's bytes, so the console's output never lands inside a frame.
* **Receiving.**  The fast handler (`SER_IRQ_FAST`) takes a frame's bytes into `PC_RXBUF` instead of the RX ring, and when it's whole, wakes the client whose request is out.  A frame that comes while the last one hasn't been taken is skipped (its length is still read, so its bytes don't reach the console).
* **Waiting.**  The client sleeps (`ERR_IO_WOULD_BLOCK`, and a sleeper of the tick until its reply is due), and the IO layer offers the request again when it's woken: then the reply is checked (its CRC, its tag) and put in the client's request block as a server's answer would be.
* **A frame going out** is never written over: a new request waits a tick while the last one's frame is still on the line (a request given up).
* **One request at a time.**  A second client finds the first's request out and tries again two ticks on.  If the first's client was killed, its request is taken over once it's a second past its time.  A client that gave its request up (Ctrl-C) and makes another is told apart by comparing the request with the one out (its mode's `IO_MODE_NONBLOCK` aside, so a request asked again waiting, after one that didn't, is the same).
* **Errors.**  A reply that comes damaged, or the PC's `N` (the request came damaged), or no reply in 2 seconds: the request again, 3 tries in all.  The PC answers a repeated tag with the reply it gave, without doing the request twice.  After that, or when the attach goes unanswered for a second, `ERR_IO_DEVICE` ("no answer"), and the next request attaches again.
* **Not the console.**  The IO layer treats the serial task's devices as the console (a line at a time out, a key at a time in: `IO_DEV_IS_SERIAL`); `/pc` is excepted, so it's read and written a block at a time.

**The PC's side** (`sim/lib/pcproto.js`: the frames and their reader; `sim/tools/pcfs.js`: the file server; `sim/tools/hydrapc.js`: the terminal):
* **Files.**  The folder is served as HydraFS serves a card: a directory reads as `name size` lines (or `name/`), or as 48-byte stat records with `IO_MODE_STAT`; a create of a file that's there empties it; a remove takes a file or an empty directory; a wstat renames in its directory and sets the read-only bit.  Stat times are the PC's local time, as the Hydra's clock keeps.
* **Safety.**  Nothing outside the folder is reached: a name's elements can't be `.` or `..`, or hold `\` or `:`.  `--read-only` refuses every change.
* **The terminal.**  Bytes that aren't a frame's are shown as they come.  A frame that stops for 100 ms isn't one, and its bytes are shown; one whose CRC is wrong is dropped (and the Hydra asked for it again).

**Speed.**  At 9600 baud the line moves about 900 bytes a second each way, so a 2K program loads in about 2.5 seconds, and a 256-byte read costs about 0.3 s on the line.  A song plays from `/pc` in time: the player reads 256 bytes ahead without waiting for them (`IO_MODE_NONBLOCK`, asking again each tick: the server knows the request it has out, whether it's asked again waiting or not), 512 at its start, and on into its loop before it gets there.  But a song that needs more than the line carries (a dense one needs about 2 KB a second), or a tick with more than about 500 bytes of writes (the test song's last, 449 bytes, comes 0.3 s late), waits for it: copy it to `/ram` or `/sram` first.  The protocol doesn't care about the rate: a faster line (`b115200` on `/dev/ser/ctl`, and `--baud 115200`) is the next step when it's wanted.

**Tests.**  `pc` (the commands above, and what the PC's folder holds after), `pc-two` (two tasks at once), `pc-song` (a song with a loop count, its notes' timing), `pc-read-only`, `pc-none` (no PC tool), `pc-damage` (frames damaged on the line, both ways: `--pc-damage`).

### **Version 2: reborn's**

Reborn ([the plan](../reimplementation-from-scratch.md), phase 5.5) keeps the frames, the tags, the resends and the PC tool, and changes what a frame carries to its own request block: the attach's payload is 2, and the PC tool (`pcfs.js`) answers each session in the version its attach named, so one PC tool serves either system.

| Payload | Holds |
| :------ | :---- |
| `Q` | The whole request block (32 bytes: appendix B of the plan), then its data: a name (`R_OPEN`, `R_CREATE`, `R_REMOVE`; with its 0), a write's bytes, a stat record (`R_WSTAT`) |
| `R` | Status (0, or reborn's error code: `spec/errors.def`), the fid (an open's, a create's, a dup's: the Hydra takes it from no other reply; the request's for the rest), the count done (2 bytes), the qid type; then data (a read's bytes, a 64-byte stat record).  The attach's: 0, 1 if the folder is read-only, 0, 0, and the version, 2 |

The PC answers as reborn's HydraFS (`#f`) does: a directory reads as 64-byte stat records, whole ones; a create with `DM_DIR` makes a directory; a wstat renames (a name whose first byte isn't 0) and sets the mode (no `w` bits: read-only); a dup is the same fid again; a read-only folder refuses changes with `E_ROFS` ("read-only").  Its records' device is `P`.

**Reborn's side** is the console driver (`reborn/modules/cons`, task F), which owns the line: the device `#P`, mounted at `/pc` by `/rom/lib/namespace` (`mount '#P' /pc`), a raw srvlib tree whose handler sends every request on.
* **Frames in the rings.**  Reborn's irq entries are kept to a few dozen cycles, so the PC's frames go into the receive ring with the keys, and are taken out of it as the keys are handed to the windows (before each request the driver serves); the frame going out goes into the send ring, whole, ahead of the windows' text, after each request.  A frame carries 128 bytes of data at most (a read's or a write's: the frame's block says how many, and the read or write comes short), so a reply fits the receive ring.
* **Waiting.**  The client's request answers `E_AGAIN` while its reply is out, and the client waits for the driver's event count to change: each byte in changes it, and while a reply is awaited with nothing to send, timer 2 (the driver's, for its paced sending) runs on and changes it every 8 rounds of about 65,000 cycles, so a client whose reply doesn't come looks at the time.  Asked again, the request is answered from the reply.
* **One request at a time.**  Another client's request waits for the event count to change (it does as the one out ends); a request still out a second after its reply was due (its client stopped asking: a non-blocking read given up) is given up.  A client that gave its request up and makes another is told apart by its block and its name.
* **Errors.**  As version 1's: a damaged reply, the PC's `N`, or no reply in 2 seconds, 3 tries; an attach unanswered for a second; then `E_IO` ("i/o error"), and the next request attaches again.  An attach answered without version 2 (an old PC tool) is `E_IO` too.

**The emulator** plays the PC tool's part with the same file server: `node reborn/sim/run.js -i --pc-dir C:\hydra` (`--pc-read-only`, `--pc-log`, `--pc-damage`), and the tests' `pc` field (`reborn/sim/lib/pchost.js`).  **Tests**: `pc`, `pc-two`, `pc-song` (in time with its blocking reads: 128 bytes at a time fit between its ticks), `pc-ro`, `pc-none`, `pc-damage`.

### **Not yet**
* **XMODEM**, for a terminal program other than this one ([NEXT_STEPS.md](NEXT_STEPS.md)): built in reborn (`xmodem`, phase 5.6).
* **A faster line**, with the PC tool and the Hydra changing rate together.
* **More than one request out at a time** (the tags allow it): the line, not the round trips, is what limits it at 9600.
