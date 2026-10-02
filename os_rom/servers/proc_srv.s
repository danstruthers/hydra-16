.debuginfo

; ****************************************************************************
; /proc: the tasks, as files (like Plan 9's).  BIOS ROM page 9, included inside `.scope PAGE9` (see all.s).
; It runs in the client's task (IO_DEV_CALLER_TASK; registered by IO_INIT, io_p0.s).  The device is proc:
; /dev/proc, and /proc (the boot shell mounts it there).
;   /proc                   read: a line for each busy task
;   /proc/N                 read: task N's line (N = 0-F; also /proc/N/status)
;   /proc/N/ctl             write: "kill" (TASK_SIGNAL), "break" (as Ctrl-C does) or "fg" (bring it to
;                           the front: CONS_SET_FG); for task N's family and task 0 (TASK_MAY)
;   /proc/N/cwd, env        read: its current directory, its environment
;   /proc/N/pages           read: "pages PP floor FF", the MMU pages it has and its page floor
;   /proc/N/ns              read: its namespace, as the lines that would make it (ns: PROC_NS_LIST)
;   /proc/N/cmd             write: a line for task N's shell to run, as if typed (PROC_CMD); for its family
;                           and task 0
;   /proc/N/mem             read, write: its address space as it sees it, offset = address (PROC_MEM_IO); for
;                           its family and task 0
;   /proc/N/ram             read, write: its banks on the RAM modules, offset = bank * 8K + offset in it; the
;                           same
; A line is "N S O" and CR LF: the task, its state (R runnable, W waiting for IO, P paused: waiting for a
; task it started, D a driver, - free) and the task that started it (- none), then " *" for the
; foreground task.  The text is made again for each read, from the fd's offset.
; Server ZP: ZP_PROC_* (in the client's task: the IO layer's ZP_IO_* are in use around the request), and
; these (ZP_CS; and for PROC_NS_LIST and PROC_CMD, in a task of its own: no request is under way then)
ZP_PROC_NSE     = ZP_CS + 4         ; The entry
ZP_PROC_TYPE    = ZP_CS + 5         ;   its NS_TYPE
ZP_PROC_PTR     = ZP_CS + 6         ; A byte's address, in a bank (PROC_FAR_PEEK)
ZP_PROC_SKIP    = ZP_CS + 8         ; A read's: the bytes before its offset, still to skip
ZP_PROC_A       = ZP_CS + 10        ; -a: after a member with the same path
ZP_PROC_T       = ZP_CS + 11        ; (PROC_FAR_PEEK's)
ZP_PROC_C       = ZP_CS + 12        ; (A character, compared)
            CS_FITS     ZP_PROC_NSE, 9
ZP_PROC_TASK    = ZP_CS + 4         ; mem and ram (PROC_MEM_IO): the task
ZP_PROC_PAGE    = ZP_CS + 5         ;   its BIOS ROM page

.segment "SYS_P9"

; A request.  IN: .A = request, .X = client, .Y = fid
PROC_SERVE:
            cmp         #H9_CREATE
            bcs         PROC_BAD                            ; (The filesystem's requests)
            cmp         #H9_OPEN
            beq         PROC_OPEN
            cmp         #H9_READ
            bne         :+
            jmp         PROC_READ
:
            cmp         #H9_WRITE
            bne         :+
            jmp         PROC_WRITE
:
            cmp         #H9_STAT
            bne         :+
            jmp         PROC_STAT
:
            cmp         #H9_CTL
            beq         PROC_BAD                            ; H9_CLUNK, H9_DUP: nothing to do

PROC_OK:
            lda         #0
            clc
            rts

PROC_BAD:
            lda         #ERR_IO_BAD_REQ
            sec
            rts

PROC_NOT_FOUND:
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

; The rest of the name is in the data area: "" or "/" (the list), "/N", "/N/status" or "/N/ctl"
PROC_OPEN:
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0
            lda         (ZP_IO_REQ),Y
            beq         @list
            cmp         #'/'
            bne         @not_found
            iny
            lda         (ZP_IO_REQ),Y
            beq         @list
            jsr         PROC_HEX_DIGIT                      ; .A = the task
            bcs         @not_found
            sta         ZP_PROC_OWN
            iny
            lda         (ZP_IO_REQ),Y
            beq         @status
            cmp         #'/'
            bne         @not_found
            iny
            sty         ZP_PROC_IDX                         ; (Where the file's name starts)
            stz         ZP_PROC_LEN                         ; Which of PROC_NAMES (its fid: PROC_FIDS)
            ldx         #0

@name:
            ldy         ZP_PROC_IDX
            jsr         PROC_MATCH
            bcc         @named
:
            lda         PROC_NAMES,X                        ; (Not it: the next name)
            inx
            cmp         #0
            bne         :-
            inc         ZP_PROC_LEN
            lda         PROC_NAMES,X
            bne         @name
            bra         @not_found

@named:
            ldx         ZP_PROC_LEN
            lda         PROC_FIDS,X
            bra         @fid

@status:
            lda         #PROC_FID_STATUS

@fid:
            ora         ZP_PROC_OWN
            cmp         #PROC_FID_MEM
            bcc         @open
            sta         ZP_PROC_IDX                         ; mem, ram: a busy task, and the asker its family
            ldx         ZP_PROC_OWN                         ;   or task 0
            jsr         PROC_MEM_MAY
            bcs         @refused
            lda         ZP_PROC_IDX
            bra         @open

@list:
            lda         #PROC_FID_LIST

@open:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (Keeps .A)
            clc
            rts

@refused:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            sec
            rts

@not_found:
            dec         ZP_IO_REQ + 1
            bra         PROC_NOT_FOUND

; Does the name at (ZP_IO_REQ),Y end with the name at PROC_NAMES,X?  OUT: C = 0 yes.  Modifies: .A, .X, .Y
PROC_MATCH:
            lda         PROC_NAMES,X
            cmp         (ZP_IO_REQ),Y
            bne         @no
            inx
            iny
            ora         #0
            bne         PROC_MATCH                          ; (Both ended: a match)
            clc
            rts

@no:
            sec
            rts

PROC_S_MEM:     .byte   "pages floor "
PROC_S_MEM_END:
PROC_NAMES:     .byte   "status", 0, "ctl", 0, "cwd", 0, "env", 0, "pages", 0, "ns", 0, "cmd", 0, "mem", 0, "ram", 0, 0
PROC_FIDS:      .byte   PROC_FID_STATUS, PROC_FID_CTL, PROC_FID_CWD, PROC_FID_ENV, PROC_FID_PAGES, PROC_FID_NS, PROC_FID_CMD
                .byte   PROC_FID_MEM, PROC_FID_RAM

; .A = a hex digit's value (0-F; upper or lower case).  OUT: C = 0; or C = 1 (not a hex digit)
PROC_HEX_DIGIT:
            ora         #$20                                ; (Letters in lower case; digits stay)
            sec
            sbc         #'0'
            cmp         #10
            bcc         @ok
            sbc         #'a' - '0' - 10                     ; (C = 1)
            cmp         #10
            bcc         @no
            cmp         #16
            bcs         @no

@ok:
            clc
            rts

@no:
            sec
            rts

; Read: make the text in the data area, then hand over what's after the fd's offset (up to the count)
PROC_READ:
            tya
            and         #$F0
            cmp         #PROC_FID_MEM
            bcc         :+
            clc                                             ; mem, ram
            jmp         PROC_MEM_IO
:
            cmp         #PROC_FID_CTL
            beq         @empty
            cmp         #PROC_FID_CMD
            bne         :+

@empty:
            jsr         IO_SRV_MAP                          ; ctl, cmd: nothing to read (end of file)
            lda         #0
            jsr         IO_SRV_COUNT
            jmp         PROC_OK
:
            cmp         #PROC_FID_NS
            bne         :+
            jmp         PROC_NS_READ
:
            cmp         #PROC_FID_CWD
            bcs         @far1
            jmp         @list_status
@far1:
            pha                                             ; cwd, env, pages: task N's
            tya
            and         #$0F
            sta         ZP_PROC_OWN
            jsr         IO_SRV_MAP
            stz         ZP_PROC_IDX                         ; (The text: none yet)
            pla
            cmp         #PROC_FID_ENV
            bne         :+
            lda         ZP_PROC_OWN                         ; env: as /env's list
            jsr         ENV_LIST
            stx         ZP_PROC_IDX
            jmp         PROC_TEXT_OUT
:
            inc         ZP_IO_REQ + 1                       ; (The data area: PROC_PUT)
            cmp         #PROC_FID_PAGES
            beq         @mem
            jsr         PROC_AREA                           ; cwd: in its IO transfer area ("/" for none), in
            stx         ZP_PROC_LEN                         ;   its bank, which may not be the client's: read a
            lda         #IO_BLK_CWD                         ;   byte at a time with its mapped (ZP_PROC_LEN), and
            sta         ZP_ENV_Q                            ;   written with the client's (ZP_ENV_P)
            .assert     ZP_ENV_Q = ZP_PROC_PTR, error, "proc's cwd: PROC_AREA's page is ZP_ENV_Q's"
            lda         RAM_BANK_REG                        ; (The client's, mapped: ZP_ENV_P)
            sta         ZP_ENV_P
            ldx         #0
:
            txa
            tay
            lda         ZP_PROC_LEN
            sta         RAM_BANK_REG
            lda         (ZP_ENV_Q),Y
            ldy         ZP_ENV_P
            sty         RAM_BANK_REG
            cmp         #0
            bne         :+
            txa                                             ; (Its end: "/" if it's empty)
            bne         @line
            lda         #'/'
            jsr         PROC_PUT
            bra         @line
:
            jsr         PROC_PUT
            inx
            cpx         #IO_CWD_MAX - 1
            bne         :--

@line:
            lda         #ASCII_CR
            jsr         PROC_PUT
            lda         #ASCII_LF
            jsr         PROC_PUT
            dec         ZP_IO_REQ + 1
            jmp         PROC_TEXT_OUT

@mem:                                                       ; pages: "pages PP floor FF" (hex), counted
            ldx         ZP_PROC_OWN                         ;   in the task (PROC_MEM_COUNT); a free one: "-"
            jsr         PROC_PEEK
            and         #TASK_BUSY_FLAG
            bne         :+
            lda         #'-'
            jsr         PROC_PUT
            bra         @line
:
            stx         ZP_TC_TASK                          ; (The task: PROC_PEEK keeps .X)
            LOAD_ADDR   ::PROC_MEM_COUNT, ZP_TC_VEC         ; (Its page 0 gate)
            jsr         TASK_CALL                           ; .A = pages, .Y = its page floor
            phy
            pha
            ldx         #0

@field:
            lda         PROC_S_MEM,X                        ; "pages ", "floor "
            jsr         PROC_PUT
            inx
            cmp         #' '
            bne         @field
            pla
            pha
            lsr
            lsr
            lsr
            lsr
            jsr         PROC_PUT_HEX
            pla
            and         #$0F
            jsr         PROC_PUT_HEX
            cpx         #PROC_S_MEM_END - PROC_S_MEM
            beq         @line
            lda         #' '
            jsr         PROC_PUT
            bra         @field

@list_status:
            phy
            jsr         IO_SRV_MAP
            php
            sei                                             ; (The foreground task: a quick look at
            ldx         T_REGISTER                          ;   the serial task)
            ldy         #SERIAL_TASK_NUM
            sty         T_REGISTER                          ; Quick look (no stack use!)
            ldy         ZP_SER_CAPTURE
            stx         T_REGISTER
            plp
            sty         ZP_PROC_FG
            lda         T_REGISTER
            and         #$0F
            sta         ZP_PROC_LEN                         ; (This task, for PROC_LINE, until @made)
            inc         ZP_IO_REQ + 1                       ; The data area
            stz         ZP_PROC_IDX
            pla                                             ; The fid
            cmp         #PROC_FID_STATUS
            bcs         @one
            ldx         #0                                  ; The list: every busy task

@list:
            jsr         PROC_PEEK
            and         #TASK_BUSY_FLAG
            beq         :+
            jsr         PROC_LINE
:
            inx
            cpx         #MAX_TASK_NUMBER + 1
            bne         @list
            bra         @made

@one:
            and         #$0F
            tax
            jsr         PROC_LINE

@made:                                                      ; The text is ZP_PROC_IDX bytes (144 at most)
            dec         ZP_IO_REQ + 1

; The text made in the data area (ZP_PROC_IDX bytes; the client's transfer area mapped, ZP_IO_REQ at the
; request): what's after the fd's offset, up to the count, handed over (IO_SRV_COUNT: and unmapped).  For
; /env too (env_srv.s).  OUT: C = 0, .A = 0
PROC_TEXT_OUT:
            ldy         #IO_BLK_OFS + 3                     ; Past the end: nothing more (end of file)
            lda         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            bne         @eof
            dey
            lda         (ZP_IO_REQ),Y
            cmp         ZP_PROC_IDX
            bcs         @eof
            tax                                             ; .X = the offset
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            sta         ZP_PROC_LEN                         ; Bytes wanted (1-256; 256 = 0)
            inc         ZP_IO_REQ + 1
            ldy         #0                                  ; Move the text after the offset down

@move:
            phy
            txa
            tay
            lda         (ZP_IO_REQ),Y
            ply
            sta         (ZP_IO_REQ),Y
            iny
            cpy         ZP_PROC_LEN                         ; (256: 0, never reached: the text is shorter)
            beq         @moved
            inx
            cpx         ZP_PROC_IDX
            bne         @move

@moved:
            dec         ZP_IO_REQ + 1
            tya                                             ; The count
            bra         @count

@eof:
            lda         #0

@count:
            jsr         IO_SRV_COUNT
            jmp         PROC_OK

; .A = task .X's state (TASK_STATUS_REG), and ZP_PROC_OWN = its owner.  Preserves .X
PROC_PEEK:
            php
            sei
            ldy         T_REGISTER
            stx         T_REGISTER                          ; Quick look (no stack use!)
            lda         ZP_TASK_OWNER
            sty         T_REGISTER
            sta         ZP_PROC_OWN
            stx         T_REGISTER                          ; Quick look (no stack use!)
            lda         TASK_STATUS_REG
            sty         T_REGISTER
            plp
            rts

; Add task .X's line to the text: "N S O", " *" if it's the foreground task, CR LF.  Preserves .X
PROC_LINE:
            txa
            jsr         PROC_PUT_HEX
            jsr         PROC_PUT_SPACE
            jsr         PROC_PEEK
            ldy         #'R'                                ; The state: this task is running (the IO
            cpx         ZP_PROC_LEN                         ;   layer has it marked waiting for this
            beq         @state                              ;   request)
            ldy         #'-'
            bit         #TASK_BUSY_FLAG
            beq         @state
            ldy         #'D'
            bit         #TASK_RESIDENT_FLAG
            bne         @state
            ldy         #'P'
            bit         #TASK_PAUSED_FLAG
            bne         @state
            ldy         #'W'
            bit         #TASK_WAITING_FLAG
            bne         @state
            ldy         #'R'

@state:
            tya
            jsr         PROC_PUT
            jsr         PROC_PUT_SPACE
            lda         ZP_PROC_OWN                         ; The owner
            cmp         #MAX_TASK_NUMBER + 1
            bcs         :+
            jsr         PROC_PUT_HEX
            bra         @fg
:
            lda         #'-'
            jsr         PROC_PUT

@fg:
            cpx         ZP_PROC_FG
            bne         :+
            jsr         PROC_PUT_SPACE
            lda         #'*'
            jsr         PROC_PUT
:
            lda         #ASCII_CR
            jsr         PROC_PUT
            lda         #ASCII_LF
            bra         PROC_PUT

PROC_PUT_SPACE:
            lda         #' '
            bra         PROC_PUT

; Add a hex digit (.A = 0-F) to the text
PROC_PUT_HEX:
            cmp         #10
            bcc         :+
            adc         #'A' - '9' - 2                      ; (C = 1)
:
            adc         #'0'

; Add .A to the text (in the data area: ZP_IO_REQ + $100).  Modifies: .Y
PROC_PUT:
            ldy         ZP_PROC_IDX
            sta         (ZP_IO_REQ),Y
            inc         ZP_PROC_IDX
            rts

; Write to a ctl file: a command, "kill", "break" or "fg" (a space, CR or LF may follow it).  The whole
; write is taken.
PROC_WRITE:
            tya
            and         #$F0
            cmp         #PROC_FID_MEM
            bcc         :+
            jmp         PROC_MEM_IO                         ; mem, ram (C = 1: a write)
:
            cmp         #PROC_FID_CMD
            bne         :+
            jmp         PROC_CMD_WRITE
:
            cmp         #PROC_FID_CTL
            beq         :+
            lda         #ERR_IO_MODE
            sec
            rts
:
            tya
            and         #$0F
            sta         ZP_PROC_OWN                         ; The task
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT
            lda         (ZP_IO_REQ),Y
            bne         :+
            dec                                             ; (256 bytes: look at 255)
:
            sta         ZP_PROC_LEN
            inc         ZP_IO_REQ + 1                       ; The data area
            ldx         #0                                  ; The command: offset in PROC_CMDS
            stz         ZP_PROC_IDX                         ;   and number

@cmd:
            lda         PROC_CMDS,X
            beq         @bad                                ; (The end of the table)
            ldy         #0

@char:
            lda         PROC_CMDS,X
            beq         @word
            cpy         ZP_PROC_LEN
            beq         @skip                               ; (The write is shorter)
            cmp         (ZP_IO_REQ),Y
            bne         @skip
            inx
            iny
            bra         @char

@word:                                                      ; It matches if the write ends here, or a
            cpy         ZP_PROC_LEN                         ;   space, CR, LF or 0 comes next
            beq         @match
            lda         (ZP_IO_REQ),Y
            beq         @match
            cmp         #' '
            beq         @match
            cmp         #ASCII_CR
            beq         @match
            cmp         #ASCII_LF
            beq         @match

@skip:
            inx
            lda         PROC_CMDS - 1,X
            bne         @skip
            inc         ZP_PROC_IDX
            bra         @cmd

@bad:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP
            jmp         PROC_BAD

@match:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (The count stays: all of it taken)
            jsr         PROC_MAY
            bcc         :+
            rts                                             ; (ERR_IO_PERM)
:
            ldx         ZP_PROC_OWN
            lda         ZP_PROC_IDX
            beq         @kill
            cmp         #1
            beq         @break
            txa                                             ; fg
            jsr         CONS_SET_FG
            bra         @done

@kill:
            lda         #TASK_KILL_FLAG
            bra         :+

@break:
            lda         #TASK_BREAK_FLAG
:
            jsr         TASK_SIGNAL

@done:
            bcs         :+
            jmp         PROC_OK
:
            rts                                             ; (.A = the error)

PROC_CMDS:  .byte   "kill", 0, "break", 0, "fg", 0, 0

; May the asker (this task) use task ZP_PROC_OWN's ctl and cmd?  Its family (a task that started it, or itself)
; and task 0 may (TASK_MAY).  OUT: C = 0 yes; or .A = ERR_IO_PERM, C = 1.  Modifies: .A, .X, .Y
PROC_MAY:
            lda         T_REGISTER
            and         #$0F
            ldx         ZP_PROC_OWN
            ldy         #1
            jsr         TASK_MAY
            bcc         :+
            lda         #ERR_IO_PERM
:
            rts

; ****************************************************************************
; /proc/N/mem and /proc/N/ram: task N's memory, copied with T its own (MEM_COPY, ram_srv.s), MC_MAX bytes at most
; a request and not past a 256-byte page's end (a short read or write: the IO layer asks again).
;   mem:    $0000-$7FFF its task RAM; $8000-$9FFF the bank it has there (its $00, and U for a shared one);
;           $A000-$DFFF its paged ROM bank; $E000-$FEFF the BIOS ROM page it's on; $FF00-$FFFF zeros (the I/O
;           space isn't read: a read can have effects).  Past $FFFF: the end.  It writes $0000-$9FFF only.
;   ram:    its bank b ($00-$EF) at b * $2000, if it has it (the MMU's bank map; else ERR_IO_NOT_FOUND).
; Its $00 is its own (in task RAM); U and W are in its frame when the scheduler switched it out.  In a call
; (TASK_CALLING_FLAG), a driver, or the asker itself, it has no frame there: page 0 and U 0 (where its calls are).

; May the asker use task .X's mem and ram?  It's busy, and the asker is its family, or task 0 (PROC_MAY).
; OUT: C = 0 yes, ZP_PROC_OWN = .X; or C = 1, .A = ERR_IO_NOT_FOUND or ERR_IO_PERM.  Modifies: .A, .X, .Y
PROC_MEM_MAY:
            jsr         PROC_PEEK
            stx         ZP_PROC_OWN                         ; (PROC_PEEK's is its owner)
            and         #TASK_BUSY_FLAG
            bne         :+
            lda         #ERR_IO_NOT_FOUND
            sec
            rts
:
            jmp         PROC_MAY

; Read (C = 0) or write (C = 1) /proc/N/mem or ram (.Y = the fid)
PROC_MEM_IO:
            ror         ZP_PROC_FG                          ; (Bit 7: a write)
            tya
            and         #$F0
            sta         ZP_PROC_IDX                         ; mem or ram
            tya
            and         #$0F
            sta         ZP_PROC_TASK
            tax
            jsr         PROC_MEM_MAY
            bcc         :+
            rts
:
            lda         T_REGISTER                          ; (The client: this task)
            and         #$0F
            tax
            jsr         IO_SRV_MAP
            jsr         MEM_COUNT                           ; MC_N
            lda         ZP_PROC_TASK
            sta         MC_TASK
            ldy         #IO_BLK_OFS
            lda         (ZP_IO_REQ),Y
            sta         MC_ADDR
            lda         #$FF
            sta         MC_BANK                             ; (Its address space as it is, so far)
            sta         MC_U
            ldy         #IO_BLK_OFS + 3
            lda         (ZP_IO_REQ),Y
            bne         @past
            dey
            lda         ZP_PROC_IDX
            cmp         #PROC_FID_RAM
            beq         @ram
            lda         (ZP_IO_REQ),Y                       ; mem: 64K
            bne         @past
            dey
            lda         (ZP_IO_REQ),Y
            sta         MC_ADDR + 1
            bpl         @copy                               ; $0000-$7FFF: its task RAM
            cmp         #>PAGED_ROM_BASE
            bcs         @rom
            jsr         PROC_WINDOW                         ; $8000-$9FFF: its bank there
            bra         @copy

@rom:
            bit         ZP_PROC_FG
            bmi         @refuse                             ; (No writes: ROM, and the I/O space)
            cmp         #>$E000
            bcc         @copy                               ; $A000-$DFFF: its paged ROM bank (its $01)
            cmp         #>IO_PORT_BASE
            bcs         @zeros
            jsr         PROC_BIOS                           ; $E000-$FEFF: its BIOS ROM page
            bra         @count

@zeros:
            inc         ZP_IO_REQ + 1                       ; (The data area)
            ldy         #0
            tya
:
            sta         (ZP_IO_REQ),Y
            iny
            cpy         MC_N
            bne         :-
            dec         ZP_IO_REQ + 1
            bra         @count

@ram:                                                       ; ram: bank (offset bits 13-20) $00-$EF, one it has
            lda         (ZP_IO_REQ),Y
            cmp         #(MMU_BANK_TOP + 1) >> 3
            bcs         @past
            asl
            asl
            asl
            sta         MC_T
            dey
            lda         (ZP_IO_REQ),Y
            pha
            and         #$1F                                ; (The window: $8000 + the offset in the bank)
            ora         #>PAGED_RAM_BASE
            sta         MC_ADDR + 1
            pla
            lsr
            lsr
            lsr
            lsr
            lsr
            ora         MC_T
            sta         MC_BANK
            lsr                                             ; Its byte of the bank map ...
            lsr
            lsr
            tax
            lda         ZP_PROC_TASK
            jsr         PROC_BANKS_PEEK
            pha
            lda         MC_BANK                             ; ... and its bit
            and         #7
            tax
            pla
            and         PROC_BITS,X
            bne         @copy
            lda         #ERR_IO_NOT_FOUND
            bra         @error

@past:
            bit         ZP_PROC_FG
            bmi         @refuse
            lda         #0                                  ; (A read: end of file)
            bra         @counted

@refuse:
            lda         #ERR_IO_MODE

@error:
            jsr         IO_SRV_UNMAP
            sec
            rts

@copy:
            bit         ZP_PROC_FG
            bmi         @write
            jsr         MEM_READ
            bra         @count

@write:
            jsr         MEM_WRITE

@count:
            lda         MC_N

@counted:
            jsr         IO_SRV_COUNT
            jmp         PROC_OK

PROC_BITS:      .byte   $01, $02, $04, $08, $10, $20, $40, $80   ; (The MMU's bitmaps: MMU_BIT_MASKS)

; MC_BANK, MC_U = the bank task ZP_PROC_TASK has at $8000 (its $00), and its U for a shared one (MC_U stays $FF
; for one of its own).  Modifies: .A, .X, .Y
PROC_WINDOW:
            lda         T_REGISTER
            and         #$0F
            cmp         ZP_PROC_TASK
            bne         @other
            lda         ZP_IO_SAVEB                         ; The asker itself: as it was before IO_SRV_MAP
            ldx         ZP_IO_SAVEU
            bra         @bank

@other:
            ldx         #RAM_BANK_REG                       ; (Its own, in its task RAM)
            lda         ZP_PROC_TASK
            jsr         PROC_ZP_PEEK
            pha
            ldx         #1                                  ; (U: its frame's first byte)
            jsr         PROC_FRAMED
            tax
            pla

@bank:
            sta         MC_BANK
            cmp         #$F0
            bcc         :+
            stx         MC_U
:
            rts

; Read MC_N bytes at MC_ADDR ($E000-$FEFF) from the BIOS ROM page task ZP_PROC_TASK is on, to the data area
; (PEEK_PAGE, IRQs on).  Modifies: .A, .X, .Y
PROC_BIOS:
            ldx         #6                                  ; (W: its frame's sixth byte)
            jsr         PROC_FRAMED
            sta         ZP_PROC_PAGE
            lda         ZP_FP                               ; (PEEK_PAGE's pointer: ZP_FP's address)
            pha
            lda         ZP_FP + 1
            pha
            lda         MC_ADDR
            sta         ZP_FP
            lda         MC_ADDR + 1
            sta         ZP_FP + 1
            inc         ZP_IO_REQ + 1                       ; (The data area)
            ldy         #0
:
            lda         ZP_PROC_PAGE
            jsr         PEEK_PAGE                           ; (Keeps .Y)
            sta         (ZP_IO_REQ),Y
            iny
            cpy         MC_N
            bne         :-
            dec         ZP_IO_REQ + 1
            pla
            sta         ZP_FP + 1
            pla
            sta         ZP_FP
            rts

; .A = byte .X of task ZP_PROC_TASK's frame (1: U, 6: W), or 0 if it has none: in a call, a driver, or the
; asker itself.  Modifies: .X, .Y
PROC_FRAMED:
            lda         T_REGISTER
            and         #$0F
            cmp         ZP_PROC_TASK
            beq         @none
            phx
            ldx         ZP_PROC_TASK
            jsr         PROC_PEEK                           ; (Its state; ZP_PROC_OWN: its owner)
            plx
            and         #TASK_CALLING_FLAG | TASK_RESIDENT_FLAG
            bne         @none
            lda         ZP_PROC_TASK
            php                                             ; The byte: $0100 + its SP + .X
            sei
            ldy         T_REGISTER
            sta         T_REGISTER                          ; Quick look (no stack use!)
            txa
            clc
            adc         STACK_SAVE_REG
            tax
            lda         $0100,X
            sty         T_REGISTER
            plp
            rts

@none:
            lda         #0
            rts

; .A = task .A's zero page byte .X.  Modifies: .Y
PROC_ZP_PEEK:
            php
            sei
            ldy         T_REGISTER
            sta         T_REGISTER                          ; Quick look (no stack use!)
            lda         $00,X
            sty         T_REGISTER
            plp
            rts

; .A = byte .X of task .A's MMU bank map (bit n: bank n, the MMU's way).  Preserves .X.  Modifies: .Y
PROC_BANKS_PEEK:
            php
            sei
            ldy         T_REGISTER
            sta         T_REGISTER                          ; Quick look (no stack use!)
            lda         MMU_BANK_MAP,X
            sty         T_REGISTER
            plp
            rts

; Stat: zeros, and for mem its size (64K), for ram up to the last bank task N has.  .Y = the fid, .X = the client
PROC_STAT:
            phy
            jsr         STAT_ZERO
            pla
            tay
            and         #$F0
            cmp         #PROC_FID_MEM
            bcc         @done
            stz         ZP_PROC_LEN                         ; (The size's bits 13-20: 0)
            tax
            tya
            and         #$0F
            cpx         #PROC_FID_RAM
            beq         @ram
            lda         #1 << 3                             ; mem: 64K (bit 16)
            bra         @size

@ram:
            ldx         #MMU_BANK_TOP >> 3                  ; ram: its highest bank's byte of the map ...

@byte:
            pha
            jsr         PROC_BANKS_PEEK
            ply
            cmp         #0
            bne         @found
            tya
            dex
            bpl         @byte
            bra         @done                               ; (None: 0)

@found:
            ldy         #8                                  ; ... and bit: banks up to it
:
            dey
            asl
            bcc         :-
            sty         ZP_PROC_LEN
            txa
            asl
            asl
            asl
            ora         ZP_PROC_LEN
            inc                                             ; (Banks: $F0 at most)

@size:
            sta         ZP_PROC_LEN                         ; Size = .A * $2000
            lda         T_REGISTER
            and         #$0F
            tax
            jsr         IO_SRV_MAP
            inc         ZP_IO_REQ + 1                       ; (The data area: the stat record)
            lda         ZP_PROC_LEN
            asl
            asl
            asl
            asl
            asl
            ldy         #IO_ST_SIZE + 1
            sta         (ZP_IO_REQ),Y
            lda         ZP_PROC_LEN
            lsr
            lsr
            lsr
            iny
            sta         (ZP_IO_REQ),Y
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP

@done:
            jmp         PROC_OK

; ****************************************************************************
; /proc/N/cmd: a line for task N's shell to run, as if typed at its prompt.  It waits in task N's IO transfer area
; (IO_BLK_CMD) till the shell is at its prompt (PROC_CMD); if it's there already, waiting for the console, a break
; wakes it (a quiet one: IO_CMD_WOKEN), and it takes the line.  One line waits at a time (ERR_IO_BUSY); a write
; ends at its CR, LF or 0, and up to IO_CMD_MAX - 1 characters are kept.  The whole write is taken.

; Write to /proc/N/cmd (.Y = the fid)
PROC_CMD_WRITE:
            tya
            and         #$0F
            tax                                             ; A task?
            jsr         PROC_PEEK
            stx         ZP_PROC_OWN                         ; (PROC_PEEK's is its owner)
            and         #TASK_BUSY_FLAG
            bne         :+
            lda         #ERR_IO_NOT_FOUND
            sec
            rts
:
            jsr         PROC_MAY
            bcc         :+
            rts                                             ; (ERR_IO_PERM)
:
            lda         T_REGISTER                          ; (The client: this task)
            and         #$0F
            tax
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_COUNT + 1                   ; The characters to look at
            lda         (ZP_IO_REQ),Y
            bne         @most
            dey
            lda         (ZP_IO_REQ),Y
            beq         @most                               ; (256)
            cmp         #IO_CMD_MAX
            bcc         :+

@most:
            lda         #IO_CMD_MAX - 1
:
            sta         ZP_PROC_LEN
            jsr         PROC_CMD_PTR                        ; ZP_PROC_PTR, .X: task N's
            jsr         NO_PREEMPT                          ; (Its shell takes it whole, or not at all)
            ldy         #1
            jsr         PROC_FAR_PEEK                       ; One waiting already?
            beq         :+
            jsr         PREEMPT
            jsr         IO_SRV_UNMAP
            lda         #ERR_IO_BUSY
            sec
            rts
:
            inc         ZP_IO_REQ + 1                       ; The data area
            ldy         #0

@copy:
            cpy         ZP_PROC_LEN
            beq         @end
            lda         (ZP_IO_REQ),Y
            beq         @end
            cmp         #ASCII_CR
            beq         @end
            cmp         #ASCII_LF
            beq         @end
            iny
            jsr         PROC_FAR_POKE                       ; (Its character .Y - 1: at IO_BLK_CMD + .Y - 1)
            bra         @copy

@end:
            dec         ZP_IO_REQ + 1
            sty         ZP_PROC_IDX                         ; (Its length)
            iny
            lda         #0
            jsr         PROC_FAR_POKE
            lda         ZP_PROC_IDX
            beq         @done                               ; (An empty line: nothing)
            ldy         #0
            jsr         PROC_FAR_PEEK                       ; At its prompt?  Woken
            bpl         @done                               ; (IO_CMD_PROMPT)
            ora         #IO_CMD_WOKEN
            jsr         PROC_FAR_POKE
            ldx         ZP_PROC_OWN
            jsr         PROC_BREAK

@done:
            jsr         PREEMPT
            jsr         IO_SRV_UNMAP                        ; (The count stays: all of it taken)
            jmp         PROC_OK

; The shell's side of /proc/N/cmd, in its own task (HyForth: LINE_PROMPT, LINE_READ, fbreak).
; IN: .X = 1: at its prompt, about to read a line from the console: a line waiting is taken, to (ZP_IO_BUF) + 1
;             on, 0-terminated, as the line editor puts one in TIB; OUT: .A = its length.  None (.A = 0): it's
;             marked at its prompt, so a line written now wakes it
;     .X = 0: a line came: not at its prompt
;     .X = 2: a break came: a line's wake-up?  OUT: C = 1 yes (the shell shows no message)
; Modifies: .A, .X, .Y
PROC_CMD:
            lda         T_REGISTER
            and         #$0F
            sta         ZP_PROC_OWN
            stz         ZP_PROC_IDX
            phx
            jsr         PROC_CMD_PTR                        ; .X = the bank
            jsr         NO_PREEMPT
            pla
            beq         @mark                               ; (.A = 0: not at its prompt)
            cmp         #1
            beq         @prompt
            ldy         #0                                  ; A wake-up?  Told once
            jsr         PROC_FAR_PEEK
            sta         ZP_PROC_C
            and         #<~IO_CMD_WOKEN
            jsr         PROC_FAR_POKE
            jsr         PREEMPT
            lda         ZP_PROC_C
            asl                                             ; C = IO_CMD_WOKEN (bit 6)
            asl
            rts

@prompt:
            ldy         #1

@copy:
            jsr         PROC_FAR_PEEK
            sta         (ZP_IO_BUF),Y
            beq         @copied
            iny
            cpy         #IO_CMD_MAX
            bne         @copy
            lda         #0
            sta         (ZP_IO_BUF),Y

@copied:
            dey
            sty         ZP_PROC_IDX                         ; Its length
            lda         #IO_CMD_PROMPT                      ; None: at its prompt
            cpy         #0
            beq         @mark
            lda         #0                                  ; Taken: gone, and not at its prompt (it runs)
            ldy         #1
            jsr         PROC_FAR_POKE

@mark:
            ldy         #0
            jsr         PROC_FAR_POKE
            jsr         PREEMPT
            lda         ZP_PROC_IDX
            rts

; ZP_PROC_PTR = task ZP_PROC_OWN's IO_BLK_CMDST, in its IO transfer area; .X = its bank.  Modifies: .A
PROC_CMD_PTR:
            lda         #IO_BLK_CMDST
            sta         ZP_PROC_PTR

; ZP_PROC_PTR + 1 = task ZP_PROC_OWN's IO transfer area's first page ($8000 + (task & 3) * IO_XFER_SIZE), .X =
; its bank (IO_XFER_BANK + task / 4: IO_XFER_OF's).  Modifies: .A
PROC_AREA:
            lda         ZP_PROC_OWN
            and         #IO_XFER_PER_BANK - 1
            tax
            lda         PROC_XFER_PAGE,X
            sta         ZP_PROC_PTR + 1
            lda         ZP_PROC_OWN
            lsr
            lsr
            clc
            adc         #IO_XFER_BANK
            tax
            rts

PROC_XFER_PAGE: .byte   IO_XFER_PAGES

; Break task .X alone (not the tasks it started, as TASK_SIGNAL does), if it's busy and not a driver: as
; TASK_FLAG does (page 0), it stops waiting.  Modifies: .A, .Y
PROC_BREAK:
            php
            sei
            ldy         T_REGISTER
            stx         T_REGISTER                          ; Quick switch (no stack use!)
            bbr0        TASK_STATUS_REG, @done              ; Free (TASK_BUSY_FLAG)
            bbs3        TASK_STATUS_REG, @done              ; A driver (TASK_RESIDENT_FLAG)
            lda         #TASK_BREAK_FLAG
            tsb         TASK_STATUS_REG
            rmb2        TASK_STATUS_REG                     ; Not waiting (TASK_WAITING_FLAG)

@done:
            sty         T_REGISTER
            plp
            rts
.assert     TASK_BUSY_FLAG = 1 .and TASK_RESIDENT_FLAG = 8 .and TASK_WAITING_FLAG = 4, error, "PROC_BREAK: bits 0, 3, 2"

; ****************************************************************************
; Namespaces as text: a task's entries (in its IO transfer area: ns.s) as the lines that would make them, as
; Plan 9's ns prints them: "mount [-ac] device /path", "bind [-ac] /target /path", "hide /path"; a union's
; members after its first get -a, and NS_C c.  Printed (IO_NS_LIST: PROC_NS_LIST), or a read's text
; (/proc/N/ns: PROC_NS_READ).  The scratch: ZP_PROC_* (above)
.assert     <IO_BLK_NS = 0 .and NS_ENTRIES <= 64, error, "PROC_NS_GETC: the entries start a page"

; Print this task's namespace (IO_NS_LIST, through page 0's gate).  OUT: C = 0.  Preserves .A, .X, .Y
PROC_NS_LIST:
            PUSH_AXY
            lda         T_REGISTER
            and         #$0F
            sta         ZP_PROC_OWN
            stz         ZP_PROC_FG                          ; (Printed)
            jsr         PROC_NS_TEXT
            PULL_YXA
            clc
            rts

; Read /proc/N/ns (.Y = the fid): the text from the fd's offset, up to the count (255 at most), made
; again for each read
PROC_NS_READ:
            tya
            and         #$0F
            sta         ZP_PROC_OWN
            jsr         IO_SRV_MAP
            ldy         #IO_BLK_OFS + 3                     ; Past 64K: the end
            lda         (ZP_IO_REQ),Y
            dey
            ora         (ZP_IO_REQ),Y
            bne         @count                              ; (.A <> 0, but nothing: .X)
            dey
            lda         (ZP_IO_REQ),Y
            sta         ZP_PROC_SKIP + 1
            dey
            lda         (ZP_IO_REQ),Y
            sta         ZP_PROC_SKIP
            ldy         #IO_BLK_COUNT + 1
            lda         (ZP_IO_REQ),Y
            bne         @most
            dey
            lda         (ZP_IO_REQ),Y
            bne         :+

@most:
            lda         #255
:
            sta         ZP_PROC_LEN
            stz         ZP_PROC_IDX
            sta         ZP_PROC_FG                          ; (A read's: not 0)
            inc         ZP_IO_REQ + 1                       ; (The data area: PROC_NS_PUT)
            jsr         PROC_NS_TEXT
            dec         ZP_IO_REQ + 1

@count:
            lda         ZP_PROC_IDX
            jsr         IO_SRV_COUNT
            jmp         PROC_OK

; Task ZP_PROC_OWN's namespace, as text (PROC_NS_PUT: ZP_PROC_FG): the system namespace's lines (-s), then its
; own.  Modifies: .A, .X, .Y
PROC_NS_TEXT:
            lda         ZP_PROC_OWN                         ; (Bit 7: the system's table, PROC_NS_GETC)
            ora         #$80
            sta         ZP_PROC_OWN
            jsr         PROC_NS_TABLE
            lda         ZP_PROC_OWN
            and         #$7F
            sta         ZP_PROC_OWN

; ... a table's lines
PROC_NS_TABLE:
            stz         ZP_PROC_NSE

@entry:
            ldy         #NS_TYPE
            jsr         PROC_NS_GETC
            bne         :+
            rts                                             ; (The first free one: the end)
:
            sta         ZP_PROC_TYPE
            and         #NS_KIND
            tax
            lda         PROC_NS_KINDS - 1,X
            tax
            jsr         PROC_NS_STR                         ; "mount", "bind", "hide"
            stz         ZP_PROC_A
            lda         ZP_PROC_NSE
            beq         @flagged
            ldy         #NS_PREFIX                          ; The one before's path the same?  -a

@same:
            dec         ZP_PROC_NSE
            jsr         PROC_NS_GETC
            inc         ZP_PROC_NSE
            sta         ZP_PROC_C
            jsr         PROC_NS_GETC
            cmp         ZP_PROC_C
            bne         @flagged
            iny
            cmp         #0
            bne         @same
            inc         ZP_PROC_A

@flagged:
            lda         ZP_PROC_TYPE
            and         #NS_C
            ora         ZP_PROC_A
            ldx         ZP_PROC_OWN                         ; (The system's: s)
            bmi         :+
            cmp         #0
            beq         @names
:
            ldx         #PROC_S_FLAG - PROC_NS_WORDS        ; " -"
            jsr         PROC_NS_STR
            lda         ZP_PROC_A
            beq         :+
            lda         #'a'
            jsr         PROC_NS_PUT
:
            bit         ZP_PROC_TYPE                        ; (NS_C: bit 7)
            bpl         :+
            lda         #'c'
            jsr         PROC_NS_PUT
:
            bit         ZP_PROC_OWN
            bpl         @names
            lda         #'s'
            jsr         PROC_NS_PUT

@names:
            lda         #' '
            jsr         PROC_NS_PUT
            lda         ZP_PROC_TYPE
            and         #NS_KIND
            cmp         #NS_MOUNT
            bne         :+
            ldy         #NS_DEV
            jsr         PROC_NS_GETC                        ; .A = the device
            jsr         PROC_NS_DEV
            bra         @path
:
            cmp         #NS_BIND
            bne         @prefix
            ldy         #NS_TARGET
            jsr         PROC_NS_PUTS

@path:
            lda         #' '
            jsr         PROC_NS_PUT

@prefix:
            ldy         #NS_PREFIX
            jsr         PROC_NS_PUTS
            lda         ZP_PROC_TYPE                        ; A mount's spec, after: "mount hfs /rom x"
            and         #NS_KIND
            cmp         #NS_MOUNT
            bne         @line
            ldy         #NS_TARGET
            jsr         PROC_NS_GETC
            beq         @line
            lda         #' '
            jsr         PROC_NS_PUT
            ldy         #NS_TARGET + 1                      ; (After its '/')
            jsr         PROC_NS_PUTS

@line:
            lda         #ASCII_CR
            jsr         PROC_NS_PUT
            lda         #ASCII_LF
            jsr         PROC_NS_PUT
            inc         ZP_PROC_NSE
            lda         ZP_PROC_NSE
            cmp         #NS_ENTRIES
            bcs         :+
            jmp         @entry
:
            rts

PROC_NS_WORDS:
PROC_S_MOUNT:   .byte   "mount", 0
PROC_S_BIND:    .byte   "bind", 0
PROC_S_HIDE:    .byte   "hide", 0
PROC_S_FLAG:    .byte   " -", 0
PROC_NS_KINDS:  .byte   PROC_S_MOUNT - PROC_NS_WORDS, PROC_S_BIND - PROC_NS_WORDS, PROC_S_HIDE - PROC_NS_WORDS
.assert     NS_MOUNT = 1 .and NS_BIND = 2 .and NS_HIDE = 3, error, "PROC_NS_KINDS: NS_MOUNT, NS_BIND, NS_HIDE"

; Put the string at PROC_NS_WORDS + .X.  Modifies: .A, .X
PROC_NS_STR:
            lda         PROC_NS_WORDS,X
            beq         @done
            jsr         PROC_NS_PUT
            inx
            bra         PROC_NS_STR

@done:
            rts

; Put the entry's string from its byte .Y.  Modifies: .A, .Y
PROC_NS_PUTS:
            jsr         PROC_NS_GETC
            beq         @done
            jsr         PROC_NS_PUT
            iny
            bra         PROC_NS_PUTS

@done:
            rts

; Put device .A's name (the device table's, in the system's bank).  Modifies: .A, .X, .Y
PROC_NS_DEV:
            asl                                             ; Its entry: IO_DEV_TABLE + device * 16
            asl
            asl
            asl
            sta         ZP_PROC_PTR
            lda         #>IO_DEV_TABLE
            sta         ZP_PROC_PTR + 1
            ldx         #SYS_BANK
            ldy         #0

@char:
            jsr         PROC_FAR_PEEK
            beq         @done
            jsr         PROC_NS_PUT
            iny
            cpy         #IO_DEV_NAME_LEN
            bne         @char

@done:
            rts
.assert     <IO_DEV_TABLE = 0 .and IO_MAX_DEVS * IO_DEV_SIZE <= 256, error, "PROC_NS_DEV: the device table, a page"

; Byte .Y of entry ZP_PROC_NSE of task ZP_PROC_OWN's namespace: at its IO transfer area + IO_BLK_NS + entry * 32,
; in its bank (PROC_AREA).  OUT: .A (and Z).  Preserves .X, .Y
PROC_NS_GETC:
            phx
            lda         ZP_PROC_NSE
            asl
            asl
            asl
            asl
            asl
            sta         ZP_PROC_PTR
            lda         ZP_PROC_OWN                         ; The system's table (bit 7): NS_SYS, in any IO
            bpl         :+                                  ;   transfer bank (they're the same)
            lda         #>NS_SYS
            ldx         #IO_XFER_BANK
            bra         @page
:
            jsr         PROC_AREA                           ; .X = its bank
            lda         ZP_PROC_PTR + 1
            clc
            adc         #>IO_BLK_NS

@page:
            sta         ZP_PROC_PTR + 1
            lda         ZP_PROC_NSE                         ; (8 entries a page)
            lsr
            lsr
            lsr
            clc
            adc         ZP_PROC_PTR + 1
            sta         ZP_PROC_PTR + 1
            jsr         PROC_FAR_PEEK
            plx
            ora         #0
            rts

; Byte .Y at ZP_PROC_PTR in shared bank .X (mapped for the one byte: U = 0, then back as it was).
; OUT: .A (and Z).  Preserves .X, .Y
PROC_FAR_PEEK:
            sty         ZP_PROC_T
            ldy         RAM_BANK_REG
            phy
            ldy         U_REGISTER
            phy
            stz         U_REGISTER
            stx         RAM_BANK_REG
            ldy         ZP_PROC_T
            lda         (ZP_PROC_PTR),Y
            ply
            sty         U_REGISTER
            ply
            sty         RAM_BANK_REG
            ldy         ZP_PROC_T
            ora         #0
            rts

; Byte .Y at ZP_PROC_PTR in shared bank .X = .A (mapped for the one byte, as PROC_FAR_PEEK).  Preserves .A, .X, .Y
PROC_FAR_POKE:
            sty         ZP_PROC_T
            ldy         RAM_BANK_REG
            phy
            ldy         U_REGISTER
            phy
            stz         U_REGISTER
            stx         RAM_BANK_REG
            ldy         ZP_PROC_T
            sta         (ZP_PROC_PTR),Y
            ply
            sty         U_REGISTER
            ply
            sty         RAM_BANK_REG
            ldy         ZP_PROC_T
            rts

; Put .A: printed (ZP_PROC_FG = 0), or a read's text (in the data area: ZP_IO_REQ), once ZP_PROC_SKIP bytes
; are skipped, and while it's under ZP_PROC_LEN bytes.  Preserves .X, .Y
PROC_NS_PUT:
            phy
            ldy         ZP_PROC_FG
            bne         @text
            jsr         WRITE_CHAR
            bra         @done

@text:
            ldy         ZP_PROC_SKIP
            bne         @skip
            ldy         ZP_PROC_SKIP + 1
            beq         @put
            dec         ZP_PROC_SKIP + 1

@skip:
            dec         ZP_PROC_SKIP
            bra         @done

@put:
            ldy         ZP_PROC_IDX
            cpy         ZP_PROC_LEN
            beq         @done                               ; (Full)
            sta         (ZP_IO_REQ),Y
            inc         ZP_PROC_IDX

@done:
            ply
            rts
