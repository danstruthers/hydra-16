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
IO_MODE_NONBLOCK    = $80       ; reads return ERR_IO_WOULD_BLOCK instead of waiting

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

; Device table: in shared bank ID $00 (see shared.s)
IO_MAX_DEVS         = 16
IO_DEV_SIZE         = 16
IO_DEV_TABLE        = $8800     ; IO_MAX_DEVS x IO_DEV_SIZE
IO_DEV_NAME         = 0         ; 8 bytes, zero-padded (free entry: first byte 0)
IO_DEV_NAME_LEN     = 8
IO_DEV_TASK         = 8         ; task the serve routine runs in ($FF = the calling task)
IO_DEV_SERVE        = 9         ; serve routine (ROM page 0 address)
IO_DEV_CALLER_TASK  = $FF

; IO transfer areas: shared bank ID $09 (U = 0, bank $F9), 512 bytes per task at $8000 + task * $200:
; the request block, then (at + $100) up to IO_UNIT bytes of data
IO_XFER_BANK_ID     = $09
IO_XFER_BANK        = $F0 | IO_XFER_BANK_ID
IO_UNIT             = 256
IO_BLK_TYPE         = 0         ; H9_* request
IO_BLK_FID          = 1
IO_BLK_MODE         = 2         ; open mode
IO_BLK_CLIENT       = 3         ; the task making the request
IO_BLK_OFS          = 4         ; 32-bit offset (read / write)
IO_BLK_COUNT        = 8         ; 16-bit byte count: requested (in), done (out)
IO_BLK_CTL_CODE     = 11        ; H9_CTL
IO_BLK_CTL_ARG      = 12
IO_BLK_NS           = $20       ; The task's namespace (below)
IO_BLK_DATA         = $100

; Per-task namespace (IO_MOUNT, IO_BIND): in the task's IO transfer area, after the request block.
; IO_OPEN applies the entry with the longest matching path prefix: a mount sends the rest of the name to
; a device's server, a bind rewrites the name (prefix -> target) and looks again.  No match: the global
; /dev.  New tasks get a copy of their parent's namespace.
NS_ENTRIES          = 7         ; ($20-$FF of the area)
NS_ENTRY_SIZE       = 32
NS_TYPE             = 0         ; NS_FREE, NS_MOUNT or NS_BIND
NS_DEV              = 1         ; Mount: the device (device table index)
NS_PREFIX           = 2         ; The path prefix, zero-terminated (13 characters at most)
NS_PREFIX_MAX       = 13
NS_TARGET           = 16        ; Bind: what the prefix stands for, zero-terminated (15 at most)
NS_TARGET_MAX       = 15
NS_FREE             = 0
NS_MOUNT            = 1
NS_BIND             = 2
NS_MAX_REWRITES     = 4         ; Binds applied to one name, at most
.assert     IO_BLK_NS + NS_ENTRIES * NS_ENTRY_SIZE <= IO_BLK_DATA, error, "The namespace must fit below the data area"

; H9P requests (serve routine: .A = request, .X = client task, .Y = fid; the request block is in the
; client's transfer area, see IO_SRV_MAP.  OUT: C = 0 (.A = fid for H9_OPEN), or C = 1 and .A = error)
H9_OPEN             = 1         ; name remainder (after the device name) in the data area -> fid
H9_READ             = 2         ; offset, count -> data, count done
H9_WRITE            = 3         ; offset, count, data -> count done
H9_CLUNK            = 4         ; close fid
H9_STAT             = 5         ; -> 16-byte stat block in the data area
H9_CTL              = 6         ; device-specific control: code, arg
H9_DUP              = 7         ; another fd refers to the fid now (IO_DUP2, or inherited by a new task)
; Every request also carries the fd's mode (IO_BLK_MODE), so a server can tell the ends of a pipe apart.

; Sound driver: /dev/snd (sound.s, snd_srv.s).  Writes are YM2151 register/value byte pairs.
SND_CTL_INIT        = 1         ; IO_CTL code: clear the YM2151
SND_CTL_TEST        = 2         ; IO_CTL code: play the test tune, in the background (a player task);
                                ;   ERR_TASK_BUSY if it's playing already
SND_CTL_STOP        = 3         ; IO_CTL code: stop the player (and key off all channels)

; Storage: /dev/sd/N/data and /dev/sd/N/ctl (storage.s, spi.s, sd.s, sd_srv.s; ROM page 3), a card on each
; SPI device N (0-7; the board's SD socket, header J18, is device 0).  A fid is SD_FID_* | the card.
SD_MAX_CARDS        = 8
SD_FID_DATA         = $00       ; /dev/sd/N/data: the card as bytes
SD_FID_CTL          = $10       ; /dev/sd/N/ctl: its details (read) and commands (write)
SD_STATE_SDSC       = 1         ; A card's state: 0 = not started; byte addresses
SD_STATE_SDHC       = 2         ;   block addresses (SDHC, SDXC)
SD_CTL_INIT         = 1         ; IO_CTL code: start the card again (e.g. after changing it)
SD_CARD_STATE       = $0200     ; In the storage task's RAM: each card's state (SD_MAX_CARDS bytes)
SD_CARD_BLOCKS      = $0208     ;   and size in blocks (4 bytes each, from its CSD register)
SD_CSD              = $0228     ; The last CSD register read (16 bytes)

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

; Serial driver: the /dev/cons and /dev/ser server (drivers/serial.s, io/ser_srv.s)
SER_RX_BUF          = $0200     ; 256-byte RX ring, in the serial task's RAM
SER_TX_BUF          = $0300     ; 256-byte TX ring, in the serial task's RAM
SER_FID_CONS        = 0         ; /dev/cons: reads only for the foreground task
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

; /dev/proc (io/proc_srv.s): the tasks.  A fid is PROC_FID_* | task
PROC_FID_LIST       = $00       ; /dev/proc: a line per task (read)
PROC_FID_STATUS     = $10       ; /dev/proc/N, /dev/proc/N/status: task N's line (read)
PROC_FID_CTL        = $20       ; /dev/proc/N/ctl: kill, break or fg (write)
