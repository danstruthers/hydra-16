.debuginfo

; ****************************************************************************
; /dev/proc: the tasks, as files (like Plan 9's /proc).  BIOS ROM page 2, included inside `.scope PAGE2`
; (see all.s).  It runs in the client's task (IO_DEV_CALLER_TASK; registered by IO_INIT, io_p0.s).
;   /dev/proc               read: a line for each busy task
;   /dev/proc/N             read: task N's line (N = 0-F; also /dev/proc/N/status)
;   /dev/proc/N/ctl         write: "kill" (TASK_SIGNAL), "break" (as Ctrl-C does) or "fg" (bring it to
;                           the front: CONS_SET_FG)
; A line is "N S O" and CR LF: the task, its state (R runnable, W waiting for IO, P paused: waiting for a
; task it started, D a driver, - free) and the task that started it (- none), then " *" for the
; foreground task.  The text is made again for each read, from the fd's offset.
; Server ZP: ZP_PROC_* (in the client's task: the IO layer's ZP_IO_* are in use around the request).

.segment "IO_P2"

; A request.  IN: .A = request, .X = client, .Y = fid
PROC_SERVE:
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
            ldx         #PROC_S_STATUS - PROC_NAMES
            jsr         PROC_MATCH
            bcc         @status
            ldy         ZP_PROC_IDX
            ldx         #PROC_S_CTL - PROC_NAMES
            jsr         PROC_MATCH
            bcs         @not_found
            lda         #PROC_FID_CTL
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

PROC_NAMES:
PROC_S_STATUS:  .byte   "status", 0
PROC_S_CTL:     .byte   "ctl", 0

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
