.debuginfo

; ****************************************************************************
; Running programs (BIOS ROM page 7): what run and a program's name do (SH_CMD's SHC_RUN, SHC_EXEC), the
; executable loader (SH_LOAD, in the program's own task), and the wait for a program to end (SHC_WAIT).
;
;   A program is a file: a Hydra executable (.hyx), which starts with a header (include/shell.inc), or
; else a HyForth script (.hys).  run tells them apart by the header, not the name.  An executable gets a
; new task: the loader reads it into the task's RAM and calls it.  A script is read by a copy of the shell
; (HyForth's run: TASK_CLONE).  Either way the program's file is on fd SH_RUN_FD for its task, which has
; copies of the shell's fds, namespace and current directory, and the console while it runs, if the shell
; has it (SH_WAIT).

.segment "SHELL_P7"

SH_HDR          = $0100                                     ; The loader's copy of the header (the bottom of
                                                            ;   its new task's stack page)

; run: the program at .A.Y.  An executable is run, and waited for: .A = 0.  A script is left open on
; SH_RUN_FD, at its start, for the caller to run (HyForth's run): .A = SH_RUN_FD
SH_RUN:
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcc         SH_RUN_OPEN
            rts

; ... the program open on fd .A
SH_RUN_OPEN:
            cmp         #SH_RUN_FD                          ; On SH_RUN_FD (its task has it)
            beq         @on_fd
            ldx         #SH_RUN_FD
            pha
            jsr         IO_DUP2
            pla
            php
            pha
            jsr         IO_CLOSE
            pla
            plp
            bcs         @done

@on_fd:
            lda         #SH_RUN_FD
            sta         PAGE1::SHFD
            jsr         SH_READ                             ; SHOWBUF = its start (a header?)
            bcs         @close
            stz         ZP_IO_OFS                           ; (Then back to its start)
            stz         ZP_IO_OFS + 1
            stz         ZP_IO_OFS + 2
            stz         ZP_IO_OFS + 3
            lda         #SH_RUN_FD
            jsr         IO_SEEK
            bcs         @close
            lda         ZP_IO_CNT + 1                       ; An executable: a whole header, and "HYX1"?
            bne         :+
            lda         ZP_IO_CNT
            cmp         #HYX_HEADER_SIZE
            bcc         @script
:
            ldx         #3
:
            lda         PAGE1::SHOWBUF + HYX_MAGIC,X
            cmp         SH_S_HYX,X
            bne         @script
            dex
            bpl         :-
            lda         PAGE1::SHOWBUF + HYX_LOAD + 1       ; It fits in task RAM?  From HYX_RAM_LOW ...
            cmp         #>HYX_RAM_LOW
            bcc         @bad
            lda         PAGE1::SHOWBUF + HYX_LOAD           ; ... to HYX_RAM_END
            clc
            adc         PAGE1::SHOWBUF + HYX_LENGTH
            tax
            lda         PAGE1::SHOWBUF + HYX_LOAD + 1
            adc         PAGE1::SHOWBUF + HYX_LENGTH + 1
            bcs         @bad
            cpx         #1                                  ; (C = 1: part of a page more)
            adc         #0
            cmp         #(>HYX_RAM_END) + 1
            bcs         @bad
            lda         #<SH_LOAD                           ; Its task: the loader, which has SH_RUN_FD
            ldy         #>SH_LOAD
            ldx         #7
            jsr         TASK_RUN
            bcs         @close
            jsr         SH_WAIT
            lda         #SH_RUN_FD                          ; (Ours goes)
            jsr         IO_CLOSE
            lda         #0
            clc
            rts

@script:
            lda         #SH_RUN_FD
            clc
            rts

@bad:
            lda         #ERR_IO_NOT_EXEC

@close:
            pha
            lda         #SH_RUN_FD
            jsr         IO_CLOSE
            pla
            sec

@done:
            rts

SH_S_HYX:   .byte   "HYX1"

; A program by its name (.A.Y, with no .hyx or .hys; a word HyForth doesn't know): name.hyx or name.hys,
; in the current directory; then, for a name with no '/', in /bin on the current directory's card.  The
; first one there is run as SH_RUN does.  None: .A = ERR_IO_NOT_FOUND
SH_EXEC:
            jsr         SH_KEEP                             ; SH_PTR = the name
            ldx         #0
            jsr         SH_EXEC_TRY
            ldy         #0                                  ; (Back here: not found)

@slash:
            lda         (SH_PTR),Y
            beq         @bin
            cmp         #'/'
            beq         @none
            iny
            bne         @slash

@bin:
            LOAD_ADDR   PAGE1::SHBUF, ZP_IO_BUF             ; /sd/N/bin/name: after the card's root
            jsr         IO_GETCWD
            jsr         SH_ON_CARD                          ; (.X = where its path starts; 0: not a card)
            txa
            beq         @none
            ldy         #0
:
            lda         SH_S_BIN,Y
            sta         PAGE1::SHBUF,X
            inx
            iny
            cpy         #SH_S_BIN_END - SH_S_BIN
            bne         :-
            jsr         SH_EXEC_TRY

@none:
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

SH_S_BIN:   .byte   "/bin/"
SH_S_BIN_END:
SH_S_EXTS:  .byte   ".hyx", 0, ".hys", 0
SH_S_EXTS_END:

; SHBUF from .X on = the name (SH_PTR), then .hyx or .hys: the first that opens is run (SH_RUN_OPEN), and
; its result is SH_EXEC's (it doesn't come back here).  Neither: it returns
SH_EXEC_TRY:
            ldy         #0
:
            lda         (SH_PTR),Y
            beq         :+
            sta         PAGE1::SHBUF,X
            inx
            iny
            cpx         #64 - 5                             ; (Too long: not found)
            bcc         :-
            rts
:
            stx         PAGE1::SHN                          ; (Where the extension goes)
            ldy         #0

@ext:
            ldx         PAGE1::SHN
:
            lda         SH_S_EXTS,Y
            sta         PAGE1::SHBUF,X
            inx
            iny
            cmp         #0
            bne         :-
            phy
            lda         #<PAGE1::SHBUF
            ldy         #>PAGE1::SHBUF
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            ply
            bcc         @found
            cpy         #SH_S_EXTS_END - SH_S_EXTS
            bcc         @ext
            rts

@found:
            ply                                             ; (Not back to SH_EXEC: to its caller)
            ply
            jmp         SH_RUN_OPEN

; Wait for task .A (one the shell started) to end.  While it runs, it has the console, if we have it: Ctrl-C
; goes to it, and the console comes back to us when it ends (CONS_RELEASE).  OUT: C = 0
SH_WAIT:
            pha
            jsr         SH_FG
            bne         :+
            pla
            pha
            jsr         CONS_SET_FG
:
            pla
            php
            sei
            ldy         T_REGISTER
            sta         T_REGISTER                          ; Quick look at the task (no stack use!)
            bbr0        TASK_STATUS_REG, @ended             ; (Ended already: free, ...
            cpy         ZP_TASK_OWNER
            bne         @ended                              ;   or someone else's now)
            sty         TASK_PARENT                         ; It wakes us when it ends (TASK_EXIT) ...
            sty         T_REGISTER
            smb1        TASK_STATUS_REG                     ;   and till then we're paused (TASK_PAUSED_FLAG)
            jsr         YIELD
            bra         @done

@ended:
            sty         T_REGISTER

@done:
            plp
            clc
            rts

.assert     TASK_BUSY_FLAG = 1 .and TASK_PAUSED_FLAG = 2, error, "SH_WAIT tests bit 0 and sets bit 1"

; Do we have the console (the serial driver's foreground task)?  OUT: Z = 1 yes.  Modifies: .A, .X
SH_FG:
            php
            sei
            ldx         T_REGISTER
            lda         #SERIAL_TASK_NUM
            sta         T_REGISTER                          ; Quick look (no stack use!)
            lda         ZP_SER_CAPTURE
            stx         T_REGISTER
            plp
            eor         T_REGISTER
            and         #$0F
            rts

; ****************************************************************************
; The loader: an executable's task starts here (SH_RUN), with its file on SH_RUN_FD.  It reads the header
; (SH_RUN has checked it) and the code, keeps the code's pages from the MMU (its floor above them), and
; calls the entry point, on ROM page 0.  The task ends when that returns (or the loading fails).
SH_LOAD:
            LOAD_ADDR   SH_HDR, ZP_IO_BUF
            lda         #HYX_HEADER_SIZE
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            lda         #SH_RUN_FD
            jsr         IO_READ
            bcs         @done
            lda         SH_HDR + HYX_LOAD                   ; The page floor: the page after the code's end
            clc
            adc         SH_HDR + HYX_LENGTH
            tax
            lda         SH_HDR + HYX_LOAD + 1
            adc         SH_HDR + HYX_LENGTH + 1
            cpx         #1                                  ; (C = 1: part of a page more)
            adc         #0
            jsr         MM_SET_FLOOR
            bcs         @done
            lda         SH_HDR + HYX_LOAD
            sta         ZP_IO_BUF
            lda         SH_HDR + HYX_LOAD + 1
            sta         ZP_IO_BUF + 1

@read:                                                      ; The code (IO_READ moves ZP_IO_BUF on)
            lda         SH_HDR + HYX_LENGTH
            sta         ZP_IO_CNT
            lda         SH_HDR + HYX_LENGTH + 1
            sta         ZP_IO_CNT + 1
            ora         ZP_IO_CNT
            beq         @loaded
            lda         #SH_RUN_FD
            jsr         IO_READ
            bcs         @done
            lda         ZP_IO_CNT
            ora         ZP_IO_CNT + 1
            beq         @done                               ; (The file's end, too soon)
            lda         SH_HDR + HYX_LENGTH                 ; What's left
            sec
            sbc         ZP_IO_CNT
            sta         SH_HDR + HYX_LENGTH
            lda         SH_HDR + HYX_LENGTH + 1
            sbc         ZP_IO_CNT + 1
            sta         SH_HDR + HYX_LENGTH + 1
            bra         @read

@loaded:
            lda         #SH_RUN_FD
            jsr         IO_CLOSE
            lda         SH_HDR + HYX_ENTRY
            sta         ZP_FAR_VEC
            lda         SH_HDR + HYX_ENTRY + 1
            sta         ZP_FAR_VEC + 1
            stz         ZP_FAR_PAGE
            jsr         FAR_CALL_A                          ; The program (ROM page 0: the $F8xx calls)

@done:
            rts
