.debuginfo

; ****************************************************************************
; /dev/proc: the tasks, as files (like Plan 9's /proc).  BIOS ROM page 9, included inside `.scope PAGE9`
; (see all.s).  It runs in the client's task (IO_DEV_CALLER_TASK; registered by IO_INIT, io_p0.s).
;   /dev/proc               read: a line for each busy task
;   /dev/proc/N             read: task N's line (N = 0-F; also /dev/proc/N/status)
;   /dev/proc/N/ctl         write: "kill" (TASK_SIGNAL), "break" (as Ctrl-C does) or "fg" (bring it to
;                           the front: CONS_SET_FG)
; A line is "N S O" and CR LF: the task, its state (R runnable, W waiting for IO, P paused: waiting for a
; task it started, D a driver, - free) and the task that started it (- none), then " *" for the
; foreground task.  The text is made again for each read, from the fd's offset.
; Server ZP: ZP_PROC_* (in the client's task: the IO layer's ZP_IO_* are in use around the request).

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
            jsr         STAT_ZERO
            bra         PROC_OK
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
            bra         @open

@list:
            lda         #PROC_FID_LIST

@open:
            dec         ZP_IO_REQ + 1
            jsr         IO_SRV_UNMAP                        ; (Keeps .A)
            clc
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
PROC_NAMES:     .byte   "status", 0, "ctl", 0, "cwd", 0, "env", 0, "mem", 0, 0
PROC_FIDS:      .byte   PROC_FID_STATUS, PROC_FID_CTL, PROC_FID_CWD, PROC_FID_ENV, PROC_FID_MEM

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
            cmp         #PROC_FID_CTL
            bne         :+
            jsr         IO_SRV_MAP                          ; ctl: nothing to read (end of file)
            lda         #0
            jsr         IO_SRV_COUNT
            jmp         PROC_OK
:
            cmp         #PROC_FID_CWD
            bcs         @far1
            jmp         @list_status
@far1:
            pha                                             ; cwd, env, mem: task N's
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
            cmp         #PROC_FID_MEM
            beq         @mem
            lda         ZP_PROC_OWN                         ; cwd: in its IO transfer area (the same bank),
            asl                                             ;   "/" for none
            ora         #>PAGED_RAM_BASE
            sta         ZP_ENV_Q + 1
            lda         #IO_BLK_CWD
            sta         ZP_ENV_Q
            lda         (ZP_ENV_Q)
            bne         :+
            lda         #'/'
            jsr         PROC_PUT
:
            ldx         #0
:
            txa
            tay
            lda         (ZP_ENV_Q),Y
            beq         @line
            jsr         PROC_PUT
            inx
            cpx         #IO_CWD_MAX - 1
            bne         :-

@line:
            lda         #ASCII_CR
            jsr         PROC_PUT
            lda         #ASCII_LF
            jsr         PROC_PUT
            dec         ZP_IO_REQ + 1
            jmp         PROC_TEXT_OUT

@mem:                                                       ; mem: "pages PP floor FF" (hex), counted
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
