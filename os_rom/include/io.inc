.debuginfo

; ****************************************************************************
; IO (see docs/plans/IO_PLAN.md): file descriptors, the device table, the transfer areas, namespaces, H9P requests,
; and the drivers' and servers' constants

; ***  IO (see docs/plans/IO_PLAN.md)  ***

; File descriptors: per task, in the task system page after the IRQ tables (IRQ_SPURIOUS ends at $7D9F)
IO_MAX_FDS          = 12
IO_FD_SIZE          = 8
IO_FD_TABLE         = $7DA0     ; IO_MAX_FDS x IO_FD_SIZE
IO_FD_SERVER        = IO_FD_TABLE + 0   ; device table index ($FF = fd not open)
IO_FD_FID           = IO_FD_TABLE + 1   ; the server's handle for the open file
IO_FD_MODE          = IO_FD_TABLE + 2   ; IO_MODE_* bits
IO_FD_FLAGS         = IO_FD_TABLE + 3   ; IO_FDF_* bits (set by IO_OPEN; copied with the fd)
IO_FD_OFS           = IO_FD_TABLE + 4   ; 32-bit offset
IO_FD_CLOSED        = $FF
IO_FDF_CONS         = $01      ; The fd is /dev/cons: WRITE_CHAR puts bytes straight into the serial TX ring
                                ;   when the task is the foreground one (SER_CONS_TRY), with no IO request

IO_MODE_READ        = $01
IO_MODE_WRITE       = $02
IO_MODE_RDWR        = IO_MODE_READ | IO_MODE_WRITE
IO_MODE_STAT        = $04       ; reads of a directory return 48-byte stat records, not text (HydraFS)
IO_MODE_TRUNC       = $08       ; empty the file as it's opened (with IO_MODE_WRITE; HydraFS)
IO_MODE_NONBLOCK    = $80       ; reads return ERR_IO_WOULD_BLOCK instead of waiting

; Stat records (IO_STAT, and what a directory read gives per entry with IO_MODE_STAT).  A device that
; has nothing to say returns all zeros (STAT_ZERO); HydraFS fills them in from the directory entry, whose
; first 48 bytes have the same layout (except that byte 33 is the card, not a reserved byte).
IO_STAT_SIZE        = 48
IO_ST_NAME          = 0         ; 32 bytes, zero-terminated
IO_ST_MODE          = 32        ; HFS_M_* bits
IO_ST_CARD          = 33        ; the SD card (0-7)
IO_ST_QVER          = 34        ; the qid: its version (up by 1 at every change) ...
IO_ST_QID           = 36        ;   and its id (unique on the card, the same across renames)
IO_ST_SIZE          = 40        ; the size in bytes
IO_ST_STAMP         = 44        ; the modification stamp

; stdio buffers, per task (page $07 is free in every task): WRITE_CHAR collects output in STDOUT_BUF (for
; the console, a line at a time: then IO_FLUSH puts it straight into the serial TX ring), and when fd 0 isn't
; the console, GET_CHAR / READ_CHAR read ahead into STDIN_BUF (io.s: STDOUT_PUT, STDIN_GET), so a pipe
; carries a block per request instead of a byte
STDOUT_BUF          = $0700
STDOUT_BUF_SIZE     = 128
STDIN_BUF           = $0780
STDIN_BUF_SIZE      = 128

; WRITE_CHAR's fast path (on pages 0 and 1, so a buffered byte needs no far call): while the stdout buffer
; is in use (ZP_OUT_CNT > 0), .A goes into it, and this returns.  It's written out when it's full, and for
; the console (ZP_OUT_LINE: line-buffered, as a terminal is) at each LF.  Otherwise it goes on, to the slow path after it.  Preserves .A, .X, .Y
.macro _M_STDOUT_FAST
            phy
            ldy         ZP_OUT_CNT
            beq         :+++
            sta         STDOUT_BUF,Y
            iny
            sty         ZP_OUT_CNT
            bmi         :+                          ; (Full at 128: write it out)
            cmp         #ASCII_LF
            bne         :++
            bit         ZP_OUT_LINE                 ; An LF, and it's the console: write it out
            bpl         :++
:
            jsr         IO_FLUSH
:
            ply
            rts
:
            ply
.endmacro
.assert     STDOUT_BUF_SIZE = 128, error, "_M_STDOUT_FAST: full when the count reaches 128 (bpl)"

; GET_CHAR's and READ_CHAR's fast path: the next byte of the stdin read-ahead, if there is one: .A = it,
; C = 1, and this returns.  Otherwise it goes on, to the slow path after it.  Preserves .X, .Y
.macro _M_STDIN_FAST
            phx
            ldx         ZP_IN_POS
            cpx         ZP_IN_CNT
            bcs         :+
            lda         STDIN_BUF,X
            inx
            stx         ZP_IN_POS
            plx
            sec
            rts
:
            plx
.endmacro

; Environments (the env server: env_srv.s, page 9), in the system's shared bank (SYS_BANK): each task's is
; a block of entries "NAME=value", each zero-terminated, then a 0 (an empty one).  A new task gets a copy of
; its parent's (ENV_COPY, from IO_INHERIT).  An open variable is a slot: the task whose environment it's
; in, the fds using it, and its name (its fid: ENV_FID_VAR + the slot; the list's: 0).
ENV_BLOCKS          = $8D00     ; Task N's: ENV_BLOCKS + N * $100 (to $9CFF)
ENV_NAME_MAX        = 30        ; A name's characters (not '/', '=' or 0)
ENV_SLOTS           = $8000     ; ENV_SLOT_COUNT open variables, ENV_SLOT_SIZE bytes each
ENV_SLOT_SIZE       = 32
ENV_SLOT_COUNT      = 16
ENV_S_TASK          = 0         ;   the task whose environment it's in ($FF: a free slot)
ENV_S_REFS          = 1         ;   the fds using it
ENV_S_NAME          = 2         ;   its name, zero-terminated
ENV_NAMEBUF         = $8240     ; A request's name (ENV_NAME_MAX + 1)
ENV_VALBUF          = $8260     ; A value being made (256 bytes)
ENV_FID_VAR         = $80
.assert     ENV_SLOTS + ENV_SLOT_COUNT * ENV_SLOT_SIZE <= $8200, error, "ENV_SLOTS must end below SH_MAP"
.assert     ENV_VALBUF + 256 <= $8400, error, "ENV_VALBUF must end below SH_HANDLES"
.assert     ENV_BLOCKS + 16 * $100 <= $9FFF, error, "ENV_BLOCKS must end below the shared bank's probe byte"

; The clock chip (a DS1747: rtc.s, page 9): whether there's one (RTC_ST_*), in the system's shared bank too
RTC_STATE           = $9D00

; Exit statuses (kernel/exits.s, page 5): each task's code and message as it ended, in the system's shared bank
EXIT_TABLE          = $9D20     ; A record a task (16): the code, then the message (zero-terminated)
EXIT_SIZE           = 32
EXIT_MSG_MAX        = 30        ; A message's characters, at most
EXIT_BREAK          = 130       ; A break with no handler ended it: "interrupt"
EXIT_KILLED         = 137       ; A kill: "killed"
.assert     EXIT_TABLE > RTC_STATE .and EXIT_TABLE + 16 * EXIT_SIZE < $9FFF, error, "EXIT_TABLE: after RTC_STATE, below the probe byte"
.assert     RTC_STATE >= ENV_BLOCKS + 16 * $100 && RTC_STATE < $9FFF, error, "RTC_STATE: after the environments, below the probe byte"

; Device table: in shared bank ID $00 (see shared.s)
IO_MAX_DEVS         = 16
IO_DEV_SIZE         = 16
IO_DEV_TABLE        = $8800     ; IO_MAX_DEVS x IO_DEV_SIZE
IO_DEV_NAME         = 0         ; 8 bytes, zero-padded (free entry: first byte 0)
IO_DEV_NAME_LEN     = 8
IO_DEV_TASK         = 8         ; task the serve routine runs in ($FF = the calling task)
IO_DEV_SERVE        = 9         ; serve routine (ROM page 0 address)
IO_DEV_CALLER_TASK  = $FF

; IO transfer areas: 1.5K per task, four to a bank: at $8000 + (task & 3) * $600 in shared bank ID $09 + task / 4
; ($09-$0C; U = 0, banks $F9-$FC; IO_XFER_OF): the request block, the current directory, then (at + $100) up to
; IO_UNIT bytes of data, then the task's namespace (four pages).  Each bank's $9800-$9BFF holds a copy of the
; system namespace (NS_SYS)
IO_XFER_BANK_ID     = $09
IO_XFER_BANK        = $F0 | IO_XFER_BANK_ID     ; (Tasks 0-3's; IO_XFER_BANK + 1 tasks 4-7's, and so on)
IO_XFER_BANKS       = 4
IO_XFER_SIZE        = $600
IO_XFER_PER_BANK    = 4
.define     IO_XFER_PAGES   $80, $86, $8C, $92  ; (task & 3)'s area's first page: >PAGED_RAM_BASE + (task & 3) * 6
.assert     IO_XFER_PER_BANK * IO_XFER_SIZE <= $1800, error, "IO transfer areas: four to a bank, below the system namespace"
IO_UNIT             = 256
IO_BLK_TYPE         = 0         ; H9_* request
IO_BLK_FID          = 1
IO_BLK_MODE         = 2         ; open mode
IO_BLK_CLIENT       = 3         ; the task making the request
IO_BLK_OFS          = 4         ; 32-bit offset (read / write)
IO_BLK_COUNT        = 8         ; 16-bit byte count: requested (in), done (out)
IO_BLK_CTL_CODE     = 11        ; H9_CTL
IO_BLK_CTL_ARG      = 12
IO_BLK_PERM         = 12        ; H9_CREATE: the new file's mode (HFS_M_*)
IO_BLK_CALL         = 13        ; IO_OPEN and the calls that share its code: which call (H9_OPEN, H9_CREATE,
                                ;   H9_REMOVE, IO_CALL_CHDIR ...)
IO_BLK_SPEC         = 14        ; IO_OPEN and the calls that share its code: <> 0, the name came through a mount
                                ;   with a spec (mount hfs /rom x): the server may serve what only a spec names
IO_CALL_CHDIR       = $80       ; IO_BLK_CALL: IO_CHDIR (it sends H9_OPEN, to check the directory) ...
IO_CALL_CHDIR_SET   = $81       ;   the new directory has been set (the old one is at IO_BLK_DATA + $80)
IO_BLK_DATA         = $100
IO_BLK_NS           = $200      ; The task's namespace (below): NS_ENTRIES * NS_ENTRY_SIZE, to the area's end

; Per-task current directory (IO_CHDIR, IO_GETCWD): in the task's IO transfer area, after the namespace, so
; new tasks get a copy of it with the namespace.  An absolute path with no '/' at its end (empty: "/"),
; zero-terminated.  IO_OPEN (and IO_CREATE, IO_REMOVE) take a name that doesn't start with '/' as relative to
; it, and tidy every name first: "." and ".." worked out, "//" and a '/' at the end taken out (NS_ABS).
IO_BLK_CWD          = $C0
IO_CWD_MAX          = 64        ; (63 characters, and its 0)
.assert     IO_BLK_CWD + IO_CWD_MAX <= IO_BLK_DATA, error, "The current directory must fit below the data area"

; Per-task namespace (IO_MOUNT, IO_BIND): in the task's IO transfer area, after the data area.
; IO_OPEN applies the entry with the longest matching path prefix: a mount sends the rest of the name to
; a device's server, a bind rewrites the name (prefix -> target) and looks again.  No match: the global
; /dev.  New tasks get a copy of their parent's namespace.
NS_ENTRIES          = 32        ; ($200-$5FF of the area: four pages)
NS_ENTRY_SIZE       = 32
NS_PAGES            = NS_ENTRIES * NS_ENTRY_SIZE / 256
NS_TYPE             = 0         ; NS_FREE, NS_MOUNT, NS_BIND or NS_HIDE (NS_KIND), and NS_C: a create in its union goes to it
NS_DEV              = 1         ; Mount: the device (device table index)
NS_PREFIX           = 2         ; The path prefix, zero-terminated (13 characters at most)
NS_PREFIX_MAX       = 13
NS_TARGET           = 16        ; Bind: what the prefix stands for, zero-terminated (15 at most); mount: "/" and its
                                ;   spec, or "" (none): the server is sent the spec's path, then the rest of the name
NS_TARGET_MAX       = 15
NS_FREE             = 0
NS_MOUNT            = 1
NS_BIND             = 2
NS_HIDE             = 3         ; (Nothing under the path is found)
NS_KIND             = $03
NS_C                = $80
; IO_MOUNT, IO_BIND: .X = flags (0: the path's entries are replaced).  The entries with the same path are a
; union's members, looked in in order (docs/plans/NAMESPACES.md)
NS_AFTER            = $01       ; -a: after the path's members
NS_BEFORE           = $02       ; -b: before them
NS_CREATE           = $04       ; -c: a create in the union goes to this one
.assert     NS_AFTER = 1 .and NS_BEFORE = 2 .and NS_CREATE = 4, error, "NS_*: -a -b -c are bits 0-2 (HyForth's NSWORD)"
NS_HIDDEN           = $08       ; IO_BIND: hide the path (no target)
NS_SPEC             = $10       ; IO_MOUNT: ZP_IO_CNT = a spec (14 characters at most: hfs's disk and a path in it,
                                ;   "x", "r/1"), as Plan 9's mount's
NS_SYSTEM           = $20       ; IO_MOUNT, IO_BIND, IO_UNMOUNT: the system namespace's entry (for task 0 and the
                                ;   boot shell: ERR_IO_PERM for others)
NS_FRESH            = $40       ; IO_UNMOUNT: a fresh namespace (Plan 9's newns): all of the task's own entries go
                                ;   but its /ram mount, so it sees the system namespace and its area (the path: "/")
IO_BLK_NSTAB        = 15        ; The namespace's routines: the table they use now, 0 the task's, 1 the system's
.assert     IO_BLK_SPEC < $10 .and IO_BLK_SPEC > IO_BLK_CALL .and IO_BLK_NSTAB < $10 .and IO_BLK_NSTAB > IO_BLK_SPEC, error, "IO_BLK_SPEC, IO_BLK_NSTAB: the request block's spare bytes"
; The system namespace: entries every task sees, under its own (a task's entry with the same path, or a longer
; one, wins), written at boot (the boot shell's mounts, /rom/lib/namespace's lines with -s).  NS_ENTRIES entries
; at NS_SYS in each IO transfer bank, the same in each (NS_SYS_SYNC), so a name resolves with no bank switch
NS_SYS              = $9800
.assert     NS_SYS >= PAGED_RAM_BASE + IO_XFER_PER_BANK * IO_XFER_SIZE .and NS_SYS + NS_ENTRIES * NS_ENTRY_SIZE <= $9FFF .and <NS_SYS = 0, error, "NS_SYS: after the areas, below $9FFF"

; Another task's memory copied with IRQs off (ram_srv.s, MEM_COPY: /dev/ram, /proc/N/mem and ram): T is the other
; task's for the copy, with its $00 set to the client's transfer bank, so the copy's numbers are here, at the same
; place in each IO transfer bank (only one copy runs at a time: IRQs are off)
MC_AREA             = NS_SYS + NS_ENTRIES * NS_ENTRY_SIZE
MC_TASK             = MC_AREA + 0   ; The task whose memory it is (T for the copy)
MC_BANK             = MC_AREA + 1   ; Its $00 for the copy: a bank ID; or $FF: its address space as it is
MC_U                = MC_AREA + 2   ; U for a shared bank ID ($F0-$FF); or $FF
MC_ADDR             = MC_AREA + 3   ; Where, as the task sees it (2 bytes)
MC_N                = MC_AREA + 5   ; How many bytes (1-MC_MAX)
MC_WRITE            = MC_AREA + 6   ; 0: read (the memory to the data area); else write (the data area to it)
MC_XBANK            = MC_AREA + 7   ; The client's: its transfer bank,
MC_DATA             = MC_AREA + 8   ;   its data area (2 bytes),
MC_CLIENT           = MC_AREA + 10  ;   and its task
MC_T                = MC_AREA + 11  ; (Scratch: /dev/ram's offsets)
MC_SAVE             = MC_AREA + 12  ; The task's $00, then its zero page bytes the copy uses (MC_ZP_N): put back
MC_MAX              = 64            ; Bytes a copy at most (IRQs off: about 45 cycles each)
.assert     MC_SAVE + 9 <= $9FFF, error, "MC_AREA: after the system namespace, below $9FFF"
; IO_OPEN's own, in the transfer area's spare bytes (the request block page): the tidy name, for a union's next
; member (NS_RETRY), how many of the first union's members it has skipped, and how many are left after the one used
IO_BLK_ORIG         = $20
IO_ORIG_MAX         = IO_CWD_MAX
IO_BLK_SKIP         = $60
IO_BLK_ULEFT        = $61       ; ($FF: no union met yet)
.assert     IO_BLK_ORIG + IO_ORIG_MAX <= IO_BLK_SKIP .and IO_BLK_ULEFT < IO_BLK_CWD, error, "IO_BLK_ORIG ..."
; /proc/N/cmd (proc_srv.s: PROC_CMD): a line for the task's shell to run, and whether it's at its prompt.  Cleared
; when the task ends (NS_CLEAR_MAPPED)
IO_BLK_CMDST        = $62       ; IO_CMD_PROMPT: its shell is at its prompt; IO_CMD_WOKEN: woken (a break) for a line
IO_BLK_CMD          = $63       ; The line waiting, 0-terminated (none: 0)
IO_CMD_MAX          = 40        ; (With its 0)
IO_CMD_PROMPT       = $80
IO_CMD_WOKEN        = $40
; A union's directory, read through: the fd reading it (+ 1; 0: none), the member it reads (IO_BLK_SKIP's), and
; the tidy name, to open in the next member at the end of this one's (io.s: IO_UNION_KEEP, IO_UNION_NEXT).  One a
; task; cleared when the task ends
IO_BLK_UFD          = IO_BLK_CMD + IO_CMD_MAX
IO_BLK_USKIP        = IO_BLK_UFD + 1
IO_BLK_UNAME        = IO_BLK_USKIP + 1
IO_UNAME_MAX        = IO_BLK_CWD - IO_BLK_UNAME     ; (With its 0: a longer name, its first member's listing only)
.assert     IO_BLK_CMDST > IO_BLK_ULEFT .and IO_BLK_CMD = IO_BLK_CMDST + 1 .and IO_UNAME_MAX >= 40, error, "IO_BLK_CMD ..."
NS_MAX_REWRITES     = 4         ; Binds applied to one name, at most
.assert     IO_BLK_NS + NS_ENTRIES * NS_ENTRY_SIZE <= IO_XFER_SIZE .and IO_BLK_DATA + IO_UNIT <= IO_BLK_NS, error, "The namespace must fit after the data area"
.assert     <IO_BLK_NS = 0 .and (NS_ENTRY_SIZE * 8) = 256, error, "NS_FIRST, NS_NEXT: the namespace starts a page, 8 entries a page"

; H9P requests (serve routine: .A = request, .X = client task, .Y = fid; the request block is in the
; client's transfer area, see IO_SRV_MAP.  OUT: C = 0 (.A = fid for H9_OPEN), or C = 1 and .A = error)
H9_OPEN             = 1         ; name remainder (after the device name) in the data area -> fid
H9_READ             = 2         ; offset, count -> data, count done
H9_WRITE            = 3         ; offset, count, data -> count done
H9_CLUNK            = 4         ; close fid
H9_STAT             = 5         ; -> 16-byte stat block in the data area
H9_CTL              = 6         ; device-specific control: code, arg
H9_DUP              = 7         ; another fd refers to the fid now (IO_DUP2, or inherited by a new task)
; The filesystem's requests (HydraFS).  Other servers refuse them: requests H9_CREATE and up aren't theirs
H9_CREATE           = 8         ; name remainder in the data area, the new file's mode (IO_BLK_PERM), the open
                                ;   mode -> fid (a file that's there is emptied and opened)
H9_REMOVE           = 9         ; name remainder in the data area (no fid: the IO layer lends it an fd)
H9_WSTAT            = 10        ; a stat record in the data area: a new name (none: 0 first byte) and mode
                                ;   bits ($FF: as they are)
; Every request also carries the fd's mode (IO_BLK_MODE), so a server can tell the ends of a pipe apart.

; Sound driver: /dev/snd (drivers/sound.s; sound/: BIOS ROM page B).  Writes are YM2151 register/value byte pairs;
; the register numbers the chip doesn't have are the library's commands (SND_R_*).  Reads: the registers as
; they were written (the carriers' TL before the volume), a 256-byte file.
SND_CTL_INIT        = 1         ; IO_CTL code: clear the YM2151 (and the library's settings)
SND_CTL_TEST        = 2         ; IO_CTL code: play the test song (/rom/songs/test.zsm), in the background (the song
                                ;   player in a task of its own, SND_PLAYER); ERR_TASK_BUSY if it's playing
SND_CTL_STOP        = 3         ; IO_CTL code: stop the player (and key off all channels)
SND_CTL_CLAIM       = 4         ; IO_CTL code: .Y = a mask of channels, this fd's alone while it's open (other
                                ;   fds' writes to them are dropped); ERR_IO_BUSY if another fd has one
SND_CTL_RELEASE     = 5         ; IO_CTL code: .Y = a mask of channels this fd gives back
SND_CTL_VOLUME      = 6         ; IO_CTL code: .Y = the master volume (0-127)
SND_CTL_CLOCK       = 7         ; IO_CTL code: the sound clock (the YM2151's timer B, its interrupt counting its
                                ;   ticks: SND_CLK): .Y = K, a period of K timer B units (1024 of the chip's clocks,
                                ;   286 us) or K + 1, as often as SND_R_CLOCK_F's fraction says; 0: stop it.  This
                                ;   fd's while it runs (ERR_IO_BUSY: another's); its last close stops it
SND_R_CH            = $02       ; Command: the channel the next commands are for (0-7; one for all fds)
SND_R_PATCH         = $03       ;   load patch n (0-162: SND_PATCHES) into it (its speakers stay)
SND_R_NOTE          = $04       ;   key on MIDI note n (60: middle C; 69: A, 440 Hz), with its bend
SND_R_OFF           = $05       ;   key off (the release)
SND_R_VOL           = $06       ;   its volume (0-127: the carriers' levels, as General MIDI's volume)
SND_R_PAN           = $07       ;   its speakers: SND_PAN_* bits
SND_R_BEND          = $09       ;   its pitch bend (signed: 1/64 of a semitone; the note playing too)
SND_R_DRUM          = $0A       ;   a General MIDI drum (a note of MIDI channel 10): its patch and pitch, keyed on
SND_R_CLOCK_F       = $0C       ; The sound clock's fraction (65536ths of a unit, the low byte, then $0D the high),
                                ;   for SND_CTL_CLOCK
SND_PAN_LEFT        = 1
SND_PAN_RIGHT       = 2
SND_PAN_BOTH        = 3
SND_FIDS            = 8         ; /dev/snd open at once (fids 1-8)
; In the sound task's RAM
SND_SHADOW          = $0200     ; Every register as last written (256 bytes; the carriers' TL before the volume)
SND_VOL             = $0300     ; Each channel's volume (8)
SND_ATTEN           = $0308     ;   and its carriers' attenuation (TL steps: the volume's and the master's)
SND_BEND            = $0310     ; Each channel's bend (8)
SND_NOTE            = $0318     ; Each channel's note ($FF: none)
SND_OWNER           = $0320     ; The fid each channel is claimed by (0: none)
SND_REFS            = $0328     ; Each fid's fds (SND_FIDS: fid N at SND_REFS + N - 1; 0: free)
SND_MASTER          = $0330     ; The master volume (0-127)
SND_RAM_END         = $0331

; Storage: /dev/sd/N/data and /dev/sd/N/ctl (storage.s, spi.s, sd.s, sd_srv.s; ROM page 3), a disk N (docs/plans/
; DISKS.md), one character: an SD card by its SPI device's number as a hex digit (0-7 now; the board's SD socket,
; header J18, is device 0; 8-f are slot cards'), any other disk by a letter that isn't one: x the ROM disk, r the
; RAM disk, s the shared RAM disk.  A disk's number, the tables' index, isn't its name: cards 0-7 are disks 0-7,
; then x, r and s are 8, 9 and 10.  A fid is SD_FID_* | the disk.
SD_MAX_CARDS        = 8         ; SD cards: disks 0-7
DISK_ROM            = 8         ; The ROM disk: the paged ROM (sd.s: SD_ROM_READ; sim/tools/mkromdisk.js), at /rom
DISK_NAME_ROM       = 'x'       ;   its name: /dev/sd/x; hfs's x (mount hfs /rom x)
DISK_RAM            = 9         ; The RAM disk: the storage task's banks on the RAM modules (the tasks' areas, r/0
DISK_NAME_RAM       = 'r'       ;   to r/f: a shell's at /ram, mount hfs /ram r/N)
DISK_SRAM           = 10        ; The shared RAM disk: shared RAM banks, at /sram (mount hfs /sram s)
DISK_NAME_SRAM      = 's'
DISK_MAX            = 16        ; Disks the tables are for (a fid's low 4 bits)
.assert     DISK_SRAM = DISK_RAM + 1 .and DISK_NAME_SRAM = DISK_NAME_RAM + 1 .and DISK_NAME_RAM > DISK_NAME_ROM - 8, error, "DISK_FROM_NAME: r and s, 9 and 10"

; A disk's name (/dev/sd/N, /sd/N) to its number.  IN: .A = the name's character.  OUT: C = 0, .A = the disk; or
; C = 1: no such disk.  (Anonymous labels: the caller's cheap local labels go on past it)
.macro DISK_FROM_NAME
            cmp         #DISK_NAME_ROM
            bne         :+
            lda         #DISK_ROM                           ; x: the ROM disk
            clc
            bra         :+++
:
            cmp         #DISK_NAME_RAM
            bcc         :+                                  ; (Below r: a card's digit?)
            sbc         #DISK_NAME_RAM - DISK_RAM           ; r, s: the RAM disks (C = 1 from the cmp; past s,
            cmp         #DISK_SRAM + 1                      ;   C = 1: no disk)
            bra         :++
:
            sec
            sbc         #'0'
            cmp         #SD_MAX_CARDS                       ; A card (characters below '0' wrap: not one)
:
.endmacro

; The RAM disks (docs/plans/DISKS.md): 8K banks, block n in bank n / 16 of the disk's run (from its first, in
; RAMD_FIRST), at $8000 + (n % 16) * 512.  The RAM disk's banks are the storage task's own (MM_BANK_ALLOC_IN), the
; shared one's shared banks (SH_BANK_ALLOC).  Started at boot with these sizes, and quick-formatted (they lose
; their files at a reset); "start SIZE [FROM-TO]" and "stop" on /dev/sd/r/ctl and /dev/sd/s/ctl change them
RAMD_BANKS          = 32        ; The RAM disk at boot: 256K, from any module
SRAMD_BANKS         = 64        ; The shared RAM disk at boot: 512K, from free shared banks ...
RAMD_SHARED_TOP     = $7F       ;   in the lower half (shared bank IDs $01-$7F)
RAMD_FIRST          = $09E0     ; Each disk's first bank (or first shared bank ID): DISK_MAX bytes
RAMD_AREAS          = $09F0     ; The areas there may be on the RAM disk: bit N (of 16) for /r/N, set as one's
                                ;   used, cleared when it's removed (TASK_AREA_END, page 5, looks as a task ends)
RAMD_AREA_TASK      = $09F2     ; HFS_AREA_END: the area's task ...
RAMD_AREA_LEN       = $09F3     ;   the path's length so far ...
RAMD_AREA_PATH      = $09F4     ;   and where it is (256 bytes from the MMU: STORAGE_INIT3)
PAGED_ROM_BASE      = $A000     ; (Where the CPU sees a paged ROM bank: $A000-$DFFF)
DISK_ROM_BLOCKS     = 256 * 32  ;   the ROM disk's blocks: the whole paged ROM (block n: bank n / 32, $A000 + n % 32 * 512)
SD_FID_DATA         = $00       ; /dev/sd/N/data: the card as bytes
SD_FID_CTL          = $10       ; /dev/sd/N/ctl: its details (read) and commands (write)
SD_STATE_SDSC       = 1         ; A card's state: 0 = not started; byte addresses
SD_STATE_SDHC       = 2         ;   block addresses (SDHC, SDXC)
SD_CTL_INIT         = 1         ; IO_CTL code: start the card again (e.g. after changing it)
SD_STATE_ROM        = 3         ;   the ROM disk (block addresses; read only)
SD_STATE_RAM        = 4         ;   the RAM disk (the storage task's banks)
SD_STATE_SRAM       = 5         ;   the shared RAM disk (shared banks)
SD_CSD              = $0228     ; The last CSD register read (16 bytes)
; Each disk's state and size, and HydraFS's numbers for it (below), in the storage task's RAM above its scratch
; ($0800-$08FF): DISK_MAX bytes, or 4 bytes at the disk * 4 (arrays 64 bytes apart), the storage task's MMU floor
; above them (HFS_SCRATCH_FLOOR)
SD_CARD_STATE       = $0900     ; Each disk's state: 0 = not started, SD_STATE_*
SD_CARD_BLOCKS      = $0920     ;   and size in blocks (4 bytes each: a card's from its CSD register)

; HydraFS: the files on the cards, /sd/N/... (fs/hfs_srv.s, ROM page 3, in the storage task, on the SD
; driver's block cache).  The device is "hfs"; the shell mounts it at /sd, so /sd/0 is card 0's root
; directory, and the tasks it starts inherit the mount.  The format is in docs/plans/HYDRAFS.md, and
; sim/tools/hydrafs.js makes and reads card images on the PC.
HFS_BLOCK           = 512       ; A block: the card's sector
HFS_CLUSTER_BLOCKS  = 8         ; A cluster (what space is allocated in): 8 blocks, 4 KB
HFS_ENTRY_SIZE      = 64        ; A directory entry (a directory is a file of them)
HFS_NAME_MAX        = 31        ; A name's characters (any byte but '/' and 0; case-sensitive)
HFS_MAX_OPEN        = 8         ; Files open at once, all cards together (a fid is the slot: 0-7)
HFS_DEPTH_MAX       = 8         ; Path elements in one walk ("..": back up through them)
HFS_VERSION         = 2         ; The newest format this reads (and writes, with a quick format): 2 has a
HFS_VERSION_FULL    = 1         ;   free map that's written as it's used (HFS_SB_MAPINIT); 1's is all written
HFS_CSHIFT          = 3         ;   and its cluster size, as a shift (HFS_CLUSTER_BLOCKS)

; Partitions: a card with a partition table (an MBR, in block 0) has its HydraFS in its partition of type
; HFS_PART_TYPE, and HydraFS counts its blocks from the partition's first (HFS_V_BASE), so another partition
; (a FAT one, for a PC) can be next to it.  A card without one is all HydraFS, from block 0
HFS_PART_TYPE       = $7F       ; The partition type (for an OS in development, by convention)
HFS_PART_ALIGN      = 2048      ; A new partition starts on a 1 MB boundary (block 2048 on an empty card)
MBR_TABLE           = $1BE      ; Block 0: the table, 4 entries of MBR_ENTRY_SIZE bytes ...
MBR_ENTRY_SIZE      = 16
MBR_P_STATUS        = 0         ;   an entry: $00, or $80 (active)
MBR_P_CHS1          = 1         ;   its first block as cylinder, head, sector (3: $FE $FF $FF, LBA only)
MBR_P_TYPE          = 4         ;   its type (0: an unused entry)
MBR_P_CHS2          = 5         ;   its last block as cylinder, head, sector (3)
MBR_P_START         = 8         ;   its first block (4)
MBR_P_BLOCKS        = 12        ;   its size in blocks (4)
MBR_SIG             = $1FE      ;   and $55 $AA after the table

; The superblock (block 0)
HFS_SB_MAGIC        = 0         ; "HYDRAFS1"
HFS_SB_VERSION      = 8
HFS_SB_CSHIFT       = 9         ; The cluster size, as a shift
HFS_SB_CLUSTERS     = 12        ; Clusters in the data area
HFS_SB_MAP          = 16        ; The free map's first block
HFS_SB_MAPSZ        = 20        ;   its size in blocks (1 bit per cluster, 1 = in use)
HFS_SB_DATA         = 24        ; The data area's first block
HFS_SB_FREE         = 28        ; Free clusters (a hint: "check" recounts it)
HFS_SB_HINT         = 32        ; Where to look for free clusters next (a hint)
HFS_SB_NEXT_QID     = 36        ; The next qid id to give out
HFS_SB_STAMP        = 40        ; The latest modification stamp given (a stamp is the clock's time: seconds
                                ;   since 2000-01-01; before the clock, a counter's next)
HFS_SB_MAPINIT      = 44        ; Version 2: the free map's blocks written so far (from its first); the rest
                                ;   haven't been, and read as all free (version 1: ignored, all written)
HFS_SB_ROOT         = 64        ; The root directory's entry
HFS_SB_LABEL        = 128       ; The volume label, zero-terminated
.assert     HFS_SB_ROOT / HFS_ENTRY_SIZE * HFS_ENTRY_SIZE = HFS_SB_ROOT, error, "The root's entry must be entry-aligned in block 0 (HFS_ENT_READ)"

; A directory entry (HFS_ENTRY_SIZE bytes): the file's metadata, extents and all, so there are no inodes
HFS_E_NAME          = 0         ; 32 bytes, zero-terminated (a 0 first byte: a free entry)
HFS_E_MODE          = 32        ; HFS_M_*
HFS_E_QVER          = 34        ; The qid's version, then its id
HFS_E_QID           = 36
HFS_E_SIZE          = 40        ; The size in bytes (up to 4 GB)
HFS_E_STAMP         = 44        ; The last change: the clock's time (seconds since 2000-01-01)
HFS_E_EXT1          = 48        ; Extent 1: its first cluster (4), its clusters (2)
HFS_E_EXT2          = 54        ; Extent 2 (0 clusters: unused)
HFS_E_EXTBLK        = 60        ; The first extent block, for the rest (0 = none)
HFS_EXT_SIZE        = 6
HFS_M_DIR           = $80       ; Mode: a directory
HFS_M_APPEND        = $40       ;   append-only
HFS_M_RO            = $01       ;   read-only
.assert     IO_ST_STAMP + 4 = HFS_E_EXT1, error, "A stat record is the entry without its extents (HFS_REC)"

; An extent block (a whole block): more extents for a fragmented file
HFS_X_NEXT          = 0         ; The next extent block (0 = the last)
HFS_X_COUNT         = 4         ; Extents used in this one (0-84)
HFS_X_FIRST         = 8         ; 84 extents, HFS_EXT_SIZE bytes each
HFS_X_MAX           = 84
.assert     HFS_X_FIRST + HFS_X_MAX * HFS_EXT_SIZE = HFS_BLOCK, error, "An extent block's extents fill it"

; In the storage task's RAM, beside the SD driver's (SD_CARD_STATE ...).  Per disk: whether it holds a HydraFS,
; and the superblock's numbers the server works from (4 bytes each, at the disk * 4, in arrays 64 bytes apart in
; the superblock's order: four to a page, so a loop with .X copies a page's four: HFS_VOLUME, HFS_SB_SAVE).  The
; counters (free, hint, qid, stamp) change in RAM and go back to the superblock at the end of the request
; (HFS_FINISH)
HFS_V_STATE         = $0910     ; 0 = not looked at yet, 1 = HydraFS, $FF = not one (DISK_MAX bytes)
HFS_V_MINIT         = $09A0     ; The free map's blocks written (HFS_SB_MAPINIT; version 1: HFS_V_MAPSZ)
HFS_V_CLUSTERS      = $0A00     ; Clusters in the data area             (the superblock's numbers: a page ...)
HFS_V_MAP           = $0A40     ; The free map's first block
HFS_V_MAPSZ         = $0A80     ;   its size in blocks
HFS_V_DATA          = $0AC0     ; The data area's first block
HFS_V_FREE          = $0B00     ; Free clusters                         (... and the counters: the next page)
HFS_V_HINT          = $0B40     ; Where to look for a free cluster next
HFS_V_QID           = $0B80     ; The next qid id
HFS_V_STAMP         = $0BC0     ; The latest modification stamp
HFS_V_STRIDE        = HFS_V_MAP - HFS_V_CLUSTERS
.assert     HFS_V_STRIDE = DISK_MAX * 4, error, "HFS_V_*: an array is 4 bytes a disk"
.assert     HFS_V_DATA - HFS_V_CLUSTERS = (HFS_SB_DATA - HFS_SB_CLUSTERS) * HFS_V_STRIDE / 4 .and HFS_V_STAMP - HFS_V_FREE = (HFS_SB_STAMP - HFS_SB_FREE) * HFS_V_STRIDE / 4, error, "HFS_V_*: in the superblock's order"
.assert     HFS_V_FREE - HFS_V_CLUSTERS = $100 .and <HFS_V_CLUSTERS = 0, error, "HFS_V_*: the numbers a page, the counters the next"
.assert     HFS_SB_FREE = HFS_SB_DATA + 4 .and HFS_SB_MAPINIT = HFS_SB_STAMP + 4, error, "The superblock's numbers, counters and HFS_SB_MAPINIT in a row"
; The server's scratch (RAM: only pointers need to be in its ZP), in the page the storage task keeps from its
; MMU (SD_INIT raises its page floor above it: HFS_SCRATCH_FLOOR)
HFS_META            = $0800     ; The metadata buffer (512 bytes from the MMU): the free map, extent blocks,
                                ;   entries being written, the superblock; written back when another block
                                ;   is wanted, and at the end of the request
HFS_MBLK            = $0802     ;   the block in it (4)
HFS_MSTATE          = $0806     ;   HFS_MS_* bits
HFS_MS_VALID        = $01       ;     it holds HFS_MBLK
HFS_MS_FRESH        = $40       ;     it's a free map block that isn't on the card yet: all free (HFS_MAP_BIT)
HFS_MS_DIRTY        = $80       ;     it's been changed, and not written yet
HFS_SBDIRTY         = $0807     ; <> 0: the card's counters (HFS_V_FREE ...) changed: HFS_FINISH writes them
HFS_C               = $0808     ; A cluster being allocated or freed (4)
HFS_N4              = $080C     ; Clusters left to look at; a count (4)
HFS_D               = $0810     ; A data cluster while its extent block is allocated; the wanted cluster (4)
HFS_T4              = $0814     ; A qid or stamp taken; a temporary (4)
HFS_LASTB           = $0818     ; A file's last extent: the extent block it's in (0: the entry) (4) ...
HFS_LASTO           = $081C     ;   and its offset there (0: the file has none) (2)
HFS_DLOC            = $081E     ; Create: the directory's place (block, index: 5) ...
HFS_NLOC            = $0823     ;   the new entry's (and where a directory scan found one) (5)
HFS_PLOC            = $0828     ; The place of the directory an entry being opened is in (5)
HFS_SCAN            = $082D     ; HFS_DIR_SCAN: what it looks for (HFS_SCAN_*)
HFS_GREW            = $082E     ; Create: <> 0: the directory got a new entry at its end
HFS_NAMEAT          = $082F     ; Create: where the new name starts in the request's name
HFS_PERM            = $0830     ; Create: the new file's mode
HFS_RUN_VEC         = $0831     ; HFS_EACH_RUN: the routine it calls for each run of a file's clusters (2)
HFS_CK_DEPTH        = $0833     ; Check: how many directories deep its walk is
HFS_CK_MASK         = $0834     ;   a cluster's bit (marking), or the bits of a map byte that are clusters
HFS_CK_N            = $0835     ;   clusters of a run left to mark (3)
HFS_ZBUF            = $0838     ; A block of zeros, and the superblock, as a map block goes on the card
                                ;   (HFS_MAP_WRITTEN: 512 bytes from the MMU) (2)
HFS_MW              = $083A     ;   the map block being written (4)
HFS_PG_STEP         = $083E     ; Progress (HFS_PG_*): a tenth of the steps (4) ...
HFS_PG_LEFT         = $0842     ;   steps left to the next tenth (4) ...
HFS_PG_TENS         = $0846     ;   tenths done ($FF: not shown)
HFS_FMT_OPT         = $0847     ; Format: HFS_FMT_FULL, or 0 (a quick format) ...
HFS_FMT_FULL        = $01       ;   the whole free map written
HFS_FMT_PART        = $02       ;   a partition made for it, if the card hasn't one (-p)
HFS_FMT_BLKS        = $0848     ;   the size asked for, in blocks (0: the whole card) (4)
HFS_CK_MARKS        = $084C     ; Check: <> 0: the pass's bitmap has marks in it (0: it's all clear)
HFS_V_BASE          = $0960     ; Per disk, at the disk * 4: its HydraFS's first block on the disk (4: its
                                ;   partition's first, or 0), set by HFS_VOLUME or a format
HFS_LOC             = $0870     ; Where an entry is: its block (4), then its index in the block (0-7)
; Sparse files (hfs_sparse.s): holes, and writes past a file's end
HFS_XBC             = $0875     ; HFS_FILE_BLOCK: the extent block it's in (0: the entry's extents) (4) ...
HFS_HOLEP           = $0879     ;   and a hole's extent it found (HFS_IN_HOLE) (2)
HFS_POSB            = $087B     ; A place in a file's extent list: the extent block (0: the entry) (4) ...
HFS_POSO            = $087F     ;   and the extent's offset there (2)
HFS_POS_SIZE        = 6
HFS_POSK            = $0881     ; A place, kept (HFS_POS_SIZE)
HFS_XNEW            = $0887     ; An extent being put in a file's list (HFS_EXT_SIZE)
HFS_XREP            = $088D     ; Another, kept for later (HFS_EXT_SIZE)
HFS_HK              = $0893     ; A hole being filled: the cluster's place in it (2) ...
HFS_HN              = $0895     ;   and its clusters (2)
HFS_ENTCHG          = $0897     ; <> 0: the entry's extents changed (HFS_FILL writes it)
HFS_WZERO           = $0898     ; $80: HFS_W_RANGE writes zeros, not the request's bytes
HFS_WPOS            = $0899     ; A write past the end (HFS_EXTEND): its SD_POS (4), SD_LEFT (2), SD_DONE,
HFS_WLEFT           = $089D     ;   kept while the gap is filled
HFS_WDONE           = $089F
HFS_WK              = $08A0     ;   and the gap's whole clusters (a hole) (4)
HFS_HOLEF           = $08A4     ; A read: <> 0: the block is in a hole (zeros)
HFS_ROMB            = $08A5     ; A read: the paged ROM bank the block is seen in (the ROM disk's), or this task's
HFS_BLK             = $08A6     ;   and where the block is: in the paged ROM, or the cache (2) (HFS_BLOCK_AT)
HFS_HOLE            = $FF       ; A hole's extent: its first cluster is $FFFFFFFF
HFS_IN_HOLE         = $01       ; HFS_FILE_BLOCK's C = 1 .A for a block in a hole (not an error)
HFS_SCRATCH_FLOOR   = $0C       ; The storage task's MMU page floor: the page above the scratch and the disks' tables
.assert     HFS_V_STAMP + HFS_V_STRIDE <= HFS_SCRATCH_FLOOR * 256 .and HFS_V_MINIT + HFS_V_STRIDE <= HFS_V_CLUSTERS, error, "The disks' tables must end below HFS_SCRATCH_FLOOR"
.assert     SD_CARD_BLOCKS + DISK_MAX * 4 <= HFS_V_BASE .and HFS_V_BASE + DISK_MAX * 4 <= HFS_V_MINIT .and HFS_V_STATE + DISK_MAX <= SD_CARD_BLOCKS, error, "The disks' tables overlap"
.assert     HFS_V_MINIT + DISK_MAX * 4 <= RAMD_FIRST .and RAMD_FIRST + DISK_MAX <= RAMD_AREAS .and RAMD_AREA_PATH + 2 <= HFS_V_CLUSTERS, error, "RAMD_FIRST ... overlap the disks' tables"
.assert     HFS_BLK + 2 <= SD_CARD_STATE .and SD_CARD_STATE < HFS_SCRATCH_FLOOR * 256, error, "The HydraFS scratch must end below the disks' tables"
.assert     HFS_POSO = HFS_POSB + 4, error, "A place is HFS_POSB then HFS_POSO (HFS_POS_SIZE bytes)"
HFS_FHDR            = $0380     ; An open file: HFS_MAX_OPEN headers of HFS_FHDR_SIZE bytes (at the fid * 16)
HFS_FHDR_SIZE       = 16
HFS_H_CARD          = 0         ;   its card, or $FF: a free slot
HFS_H_OMODE         = 1         ;   the open mode (IO_MODE_*)
HFS_H_REFS          = 2         ;   the fds using it (H9_DUP, H9_CLUNK)
HFS_H_EIDX          = 3         ;   where its directory entry is: the entry's index in its block ...
HFS_H_EBLK          = 4         ;     and the block (4 bytes)
HFS_H_FLAGS         = 8         ;   HFS_HF_* bits
HFS_HF_WRITTEN      = $01       ;     written since it was opened (the qid version and stamp are new)
HFS_HF_DIRTY        = $80       ;     the entry (its size) has changed since it was last written
HFS_H_PIDX          = 9         ;   where its directory's entry is (for a rename; the root: its own place)
HFS_H_PBLK          = 10        ;     and the block (4 bytes)
HFS_FILES           = $0400     ; And a copy of its directory entry, HFS_ENTRY_SIZE bytes at the fid * 64,
                                ;   so a read needs no extra block read to find the file's extents.  Every
                                ;   copy of one entry is kept the same (HFS_SYNC), and a walk takes an open
                                ;   file's copy over the card's (its size may not be on the card yet)
HFS_ENT             = $0600     ; The entry a walk is at (HFS_ENTRY_SIZE bytes)
HFS_STAT            = $0640     ; A stat record, or a listing's line being made (IO_STAT_SIZE bytes)
HFS_STK             = $0680     ; A walk's directories, for "..": HFS_DEPTH_MAX places of 5 bytes
HFS_NEWE            = $06A8     ; A new entry being made; an entry's old extents, while they're freed
HFS_CK_CARD         = $06E8     ; The last check ("check" on /dev/sd/N/ctl): its card ($FF: none yet) ...
HFS_CK_FIXED        = $06E9     ;   <> 0: it fixed the free map ("check fix")
HFS_CK_LOST         = $06EA     ;   clusters marked in use that nothing uses (4) ...
HFS_CK_UNMARKED     = $06EE     ;   clusters in use, but marked free (4)
HFS_CK_TWICE        = $06F2     ;   clusters in use twice (4)
HFS_CK_FREE         = $06F6     ;   free clusters, as it counted them (4)
HFS_CK_BUF          = $06FA     ; The check's buffer (from the MMU at the first check, then kept): a bitmap of
                                ;   HFS_CK_WINDOW clusters, then the walk's directories
HFS_CK_SP           = $06FC     ;   the walk's deepest directory, in it
HFS_CK_WINDOW       = 65536     ; Clusters one check pass covers (8 KB of bitmap, 256 MB of card)
HFS_CK_DEPTH_MAX    = 24        ; Directories deep a check can walk (deeper: ERR_IO_NAME)
HFS_CK_LEVEL        = 68        ; A directory in the walk: its entry, then where the walk is in it (4)
HFS_CK_BUF_SIZE     = HFS_CK_WINDOW / 8 + (HFS_CK_DEPTH_MAX + 1) * HFS_CK_LEVEL
.assert     HFS_FILES & $FF = 0, error, "HFS_FILES must start a page: HFS_FILE_PTR"
.assert     HFS_FHDR + HFS_MAX_OPEN * HFS_FHDR_SIZE <= HFS_FILES, error, "The open file headers must end below HFS_FILES"
.assert     HFS_STK + HFS_DEPTH_MAX * 5 <= HFS_NEWE, error, "HFS_STK must end below HFS_NEWE"
.assert     HFS_NEWE + HFS_ENTRY_SIZE <= HFS_CK_CARD, error, "HFS_NEWE must end below the check's results"
.assert     HFS_CK_SP + 2 <= $0700, error, "The HydraFS tables must fit in the BUFFERS pages"
.assert     HFS_CK_BUF - HFS_CK_LOST = 16, error, "The check's four counts: 4 bytes each, in a row (HFS_CK_ADD)"
.assert     HFS_NAME_MAX + 1 + 10 + 2 <= IO_STAT_SIZE, error, "A listing's longest line must fit in HFS_STAT"

; Pipe server: /dev/pipe, in its own Resident task (io_p0.s, pipe_srv.s).  Each pipe has a 256-byte ring
; (255 bytes of data) and counts the fds on each end; the fid is the pipe's index.
PIPE_MAX            = 8
PIPE_TABLE          = $0200     ; In the pipe task's RAM: PIPE_MAX x PIPE_ENTRY_SIZE
PIPE_ENTRY_SIZE     = 8
PIPE_READERS        = PIPE_TABLE + 0    ; fds on the read end
PIPE_WRITERS        = PIPE_TABLE + 1    ; fds on the write end (none left: readers get end of file)
PIPE_HEAD           = PIPE_TABLE + 2    ; next byte in
PIPE_TAIL           = PIPE_TABLE + 3    ; next byte out (empty: head = tail)
PIPE_RD_WAIT        = 4         ; (Entry offsets, 2 bytes each) tasks waiting to read (bit = task)
PIPE_WR_WAIT        = 6         ;   and to write
PIPE_BUF_PAGE       = PIPE_TABLE + PIPE_MAX * PIPE_ENTRY_SIZE   ; The rings' first page: PIPE_MAX pages
                                ;   from the pipe task's MMU (PIPE_INIT), one per pipe

; Serial driver: the /dev/cons and /dev/ser server (drivers/serial.s, servers/ser_srv.s)
SER_RX_BUF          = $0200     ; 256-byte RX ring, in the serial task's RAM
SER_TX_BUF          = $0300     ; 256-byte TX ring, in the serial task's RAM
SER_FID_CONS        = 0         ; /dev/cons: reads only for the foreground task
SER_FID_CONSCTL     = 3         ; /dev/cons/ctl: rawon, rawoff (Plan 9's consctl)
SER_FID_SER         = 1         ; /dev/ser: the raw port, for any task
SER_PEND_WAKE_RD    = $01      ; SER_PEND (bit 0): wake the tasks waiting to read
SER_PEND_WAKE_WR    = $02      ;   (bit 1) wake the tasks waiting to write
SER_PEND_BREAK      = $04      ;   (bit 2) a break key (Ctrl-C)
SER_PEND_KILL       = $08      ;   (bit 3) a kill key (Ctrl-\)
SER_PEND_CMD        = $10      ;   (bit 4) a console command: the key after the prefix key (SER_PEND_KEY)
SER_PEND_BEEP       = $80      ;   (bit 7) a BEL was sent: the YM2151 beeps
SER_FID_CTL         = 2         ; /dev/ser/ctl: the port's settings, as text: "b9600 l8 pn s1" (read), and
                                ;   commands to change them (write): bN (rate), lN (data bits 5-8), pX (parity:
                                ;   n none, o odd, e even, m mark, s space), sN (stop bits 1-2)
SER_CTL_FOREGROUND  = 1         ; IO_CTL code: .Y = the new foreground task (it gets the console input)
SER_CTL_RATE        = 2         ; IO_CTL code: .Y = the baud rate (SER_RATE_*)
SER_CTL_FORMAT      = 3         ; IO_CTL code: .Y = the character format (SER_FMT_*)
; (A change waits until what's queued has been sent, then takes effect for the next byte both ways.)
SER_FMT_BITS        = $03       ; Format: data bits - 5 (0-3: 5-8 bits)
SER_FMT_PARITY      = $1C       ;   parity, shifted left 2: SER_PAR_*
SER_FMT_STOP2       = $20       ;   2 stop bits (not with 8 data bits and parity; 1.5 with 5 bits, no parity)
SER_FMT_8N1         = 3         ;   8 data bits, no parity, 1 stop bit (the boot format)
SER_PAR_NONE        = 0
SER_PAR_ODD         = 1
SER_PAR_EVEN        = 2
SER_PAR_MARK        = 3
SER_PAR_SPACE       = 4
SER_RATE_300        = 0         ; Baud rates (index in the serial driver's rate table, ser_srv.s)
SER_RATE_600        = 1
SER_RATE_1200       = 2
SER_RATE_1800       = 3
SER_RATE_2400       = 4
SER_RATE_3600       = 5
SER_RATE_4800       = 6
SER_RATE_7200       = 7
SER_RATE_9600       = 8
SER_RATE_19200      = 9
SER_RATE_115200     = 10
SER_RATES           = 11
; Console control keys (the serial IRQ handler acts on the break, kill and prefix keys; /dev/cons on the EOF keys)
SER_KEY_BREAK       = $03       ; Ctrl-C: break the foreground task (back to its prompt: TASK_SET_BREAK)
SER_KEY_KILL        = $1C       ; Ctrl-\: kill the foreground task (the shell starts again)
SER_KEY_EOF         = $04       ; Ctrl-D: end of input (a /dev/cons read returns end of file)
SER_KEY_EOF2        = $1A       ; Ctrl-Z: the same
SER_KEY_PREFIX      = $1D       ; Ctrl-]: the next key is a console command: a task (0-F) to bring to the
                                ;   front, l to list them; Ctrl-] again for the key itself (SER_COMMAND)

; /dev/proc (servers/proc_srv.s): the tasks.  A fid is PROC_FID_* | task
PROC_FID_LIST       = $00       ; /dev/proc: a line per task (read)
PROC_FID_STATUS     = $10       ; /dev/proc/N, /dev/proc/N/status: task N's line (read)
PROC_FID_CTL        = $20       ; /dev/proc/N/ctl: kill, break or fg (write)
PROC_FID_CWD        = $30       ; /dev/proc/N/cwd: task N's current directory (read)
PROC_FID_ENV        = $40       ; /dev/proc/N/env: its environment, as /env's list (read)
PROC_FID_PAGES      = $50       ; /dev/proc/N/pages: its memory: the MMU pages it has, its page floor (read)
PROC_FID_NS         = $60       ; /dev/proc/N/ns: its namespace, as ns lists it (read)
PROC_FID_CMD        = $70       ; /dev/proc/N/cmd: a line for its shell to run (write)
PROC_FID_MEM        = $80       ; /dev/proc/N/mem: its address space, as it sees it (read; write its RAM)
PROC_FID_RAM        = $90       ; /dev/proc/N/ram: its banks on the RAM modules, bank * 8K + offset (read, write)
