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
            bcc         @far2
            jmp         @done
@far2:

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
            jsr         SH_ARGS_OUT                         ; Its arguments, on SH_ARGS_FD
            lda         #<SH_LOAD                           ; Its task: the loader, which has SH_RUN_FD
            ldy         #>SH_LOAD                           ;   and SH_ARGS_FD
            ldx         #7
            jsr         TASK_RUN
            php
            pha
            lda         #SH_ARGS_FD                         ; (Ours goes)
            jsr         IO_CLOSE
            pla
            plp
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

; edit: the editor (page 8: edit.s) on the file .A.Y (.Y = 0: none yet), in a task of its own, with the name
; as its argument; the shell waits for it
SH_EDIT:
            jsr         SH_KEEP
            ldx         #0                                  ; PAGE1::ARGLINE = the name
            cpy         #0
            beq         @named
            ldy         #0
:
            lda         (SH_PTR),Y
            beq         :+
            sta         PAGE1::ARGLINE,Y
            iny
            cpy         #HYX_ARGS_SIZE - 1
            bne         :-
:
            tya
            tax

@named:
            stz         PAGE1::ARGLINE,X
            jsr         SH_ARGS_OUT
            lda         #<::ED_MAIN_P8
            ldy         #>::ED_MAIN_P8
            ldx         #8
            jsr         TASK_RUN
            php
            pha
            lda         #SH_ARGS_FD                         ; (Ours goes: the editor has it)
            jsr         IO_CLOSE
            pla
            plp
            bcs         @done
            jmp         SH_WAIT

@done:
            rts

; SH_ARGS_FD = a pipe with a program's arguments in it (PAGE1::ARGLINE; its writing end closed, so the reader
; gets them, then the end of the file).  No pipe free: it isn't open, and the program gets none.
; Modifies: .A, .X, .Y
SH_ARGS_OUT:
            jsr         IO_PIPE                             ; .A = the reading end, .X = the writing end
            bcs         @done
            pha
            phx
            LOAD_ADDR   PAGE1::ARGLINE, ZP_IO_BUF
            ldy         #$FF                                ; (Their length)
:
            iny
            lda         PAGE1::ARGLINE,Y
            bne         :-
            sty         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            pla                                             ; The writing end: them, then closed
            pha
            jsr         IO_WRITE
            pla
            jsr         IO_CLOSE
            pla                                             ; The reading end: SH_ARGS_FD
            pha
            ldx         #SH_ARGS_FD
            jsr         IO_DUP2
            pla
            jmp         IO_CLOSE

@done:
            rts

; A program by its name (.A.Y, with no .hyx or .hys; a word HyForth doesn't know): name.hyx or name.hys,
; in the current directory; then, for a name with no '/', in $PATH's directories, or /bin on the current
; directory's card.  The first one there is run as SH_RUN does.  None: .A = ERR_IO_NOT_FOUND
SH_EXEC:
            ldx         #SH_FIND_PROG
            bra         SH_FIND

; A HyForth library by its name (.A.Y, with no .hyl; lib): name.hyl, looked for as SH_EXEC looks for a
; program, but in $LIBPATH's directories, or /lib on the current directory's card.  The first one there is
; opened.  OUT: C = 0, .A = its fd (read); or C = 1, .A = ERR_IO_NOT_FOUND
SH_LIBOPEN:
            ldx         #SH_FIND_LIB

; ... either (.X = SH_FIND_PROG, SH_FIND_LIB: what to look for, in the tables below)
SH_FIND:
            stx         PAGE1::SHFIND
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
            ldx         PAGE1::SHFIND                       ; $PATH ($LIBPATH): the directories to look in,
            lda         SH_FIND_ENV,X                       ;   with :s between them (SHOWBUF)
            ldy         SH_FIND_ENV + 1,X
            jsr         SH_ENV_READ
            bcs         @card                               ; (None: the card's /bin)
            stz         PAGE1::SHSEL                        ; (Where the next one starts)

@dir:
            ldy         PAGE1::SHSEL                        ; SHBUF = the next, and a '/'
            ldx         #0
:
            lda         PAGE1::SHOWBUF,Y
            beq         :+
            iny
            cmp         #':'
            beq         :++
            sta         PAGE1::SHBUF,X
            inx
            cpx         #64 - 6
            bcc         :-
            bra         @none                               ; (Too long)
:                                                           ; (The end: .Y stays at the 0)
:
            sty         PAGE1::SHSEL
            txa
            beq         @next                               ; (An empty one: none)
            lda         #'/'
            sta         PAGE1::SHBUF,X
            inx
            jsr         SH_EXEC_TRY                         ; (Found: it doesn't come back)

@next:
            ldy         PAGE1::SHSEL
            lda         PAGE1::SHOWBUF,Y
            bne         @dir
            bra         @none

@card:
            LOAD_ADDR   PAGE1::SHBUF, ZP_IO_BUF             ; /sd/N/bin/name (/lib/): after the card's root
            jsr         IO_GETCWD
            jsr         SH_ON_CARD                          ; (.X = where its path starts; 0: not a card)
            txa
            beq         @none
            phx
            ldx         PAGE1::SHFIND
            ldy         SH_FIND_DIR,X
            plx
            lda         #SH_S_DIRS_LEN
            sta         PAGE1::SHN
:
            lda         SH_S_DIRS,Y
            sta         PAGE1::SHBUF,X
            inx
            iny
            dec         PAGE1::SHN
            bne         :-
            jsr         SH_EXEC_TRY

@none:
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

; What SH_FIND looks for (by SH_FIND_*, word tables): the environment variable with its directories, the
; card's directory (in SH_S_DIRS), and the extensions to try (in SH_S_EXTS: from, to)
SH_FIND_PROG    = 0
SH_FIND_LIB     = 2
SH_FIND_ENV:    .word   SH_S_PATH, SH_S_LIBPATH
SH_FIND_DIR:    .byte   0, 0, SH_S_DIRS_LEN, 0
SH_FIND_EXT:    .byte   0, 0, SH_S_EXT_LIB - SH_S_EXTS, 0
SH_FIND_EXTEND: .byte   SH_S_EXT_LIB - SH_S_EXTS, 0, SH_S_EXTS_END - SH_S_EXTS, 0
SH_S_DIRS:      .byte   "/bin/", "/lib/"
SH_S_DIRS_LEN   = 5
SH_S_PATH:      .byte   "/env/PATH", 0
SH_S_LIBPATH:   .byte   "/env/LIBPATH", 0

; An environment variable's value (its file: .A.Y, "/env/NAME") into SHOWBUF, zero-terminated (255 at most).
; OUT: C = 0: .X = its length (not 0); or C = 1: there's none (or it's empty).  Modifies: .A, .Y
SH_ENV_READ:
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcs         @done
            sta         PAGE1::SHFD
            jsr         SH_READ
            php
            lda         PAGE1::SHFD
            jsr         IO_CLOSE
            plp
            bcs         @done
            ldx         ZP_IO_CNT                           ; (256: 255)
            lda         ZP_IO_CNT + 1
            beq         :+
            ldx         #$FF
:
            stz         PAGE1::SHOWBUF,X
            txa
            beq         @none
            clc
            rts

@none:
            sec

@done:
            rts
SH_S_EXTS:  .byte   ".hyx", 0, ".hys", 0
SH_S_EXT_LIB:
            .byte   ".hyl", 0
SH_S_EXTS_END:

; SHBUF from .X on = the name (SH_PTR), then .hyx or .hys (SH_FIND_LIB: .hyl): the first that opens is run
; (SH_RUN_OPEN), and its result is SH_EXEC's (it doesn't come back here); SH_FIND_LIB: its fd is SH_FIND's
; result.  None: it returns
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
            ldx         PAGE1::SHFIND                       ; (The first to try)
            ldy         SH_FIND_EXT,X

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
            tya                                             ; (The last to try: SH_FIND_EXTEND)
            ldx         PAGE1::SHFIND
            cmp         SH_FIND_EXTEND,X
            bcc         @ext
            rts

@found:
            ply                                             ; (Not back to SH_FIND: to its caller)
            ply
            ldx         PAGE1::SHFIND                       ; A program: run it.  A library: its fd
            bne         :+
            jmp         SH_RUN_OPEN
:
            clc
            rts
.assert     SH_FIND_PROG = 0, error, "SH_EXEC_TRY: SH_FIND_PROG must be 0"

; Wait for task .A (one the shell started) to end.  While it runs, it has the console, if we have it: Ctrl-C
; goes to it, and the console comes back to us when it ends (CONS_RELEASE).  OUT: C = 0
SH_WAIT:
            pha                                             ; (The task)
            jsr         SH_FG
            php                                             ; (Z = 1: we had the console: it's ours again after)
            bne         :+
            tsx
            lda         $0102,X                             ; (The task, under the php)
            jsr         CONS_SET_FG
:
            tsx
            lda         $0102,X
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
            plp                                             ; The console ours again, if we had it: the task
            bne         :+                                  ;   gives it back as it ends (CONS_RELEASE), but
            lda         T_REGISTER                          ;   not if it ended before it got it
            and         #$0F
            jsr         CONS_SET_FG
:
            pla
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
            bcc         @far1
            jmp         @done
@far1:
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
            LOAD_ADDR   HYX_ARGS, ZP_IO_BUF                 ; Its arguments, from SH_ARGS_FD (none, if it
            lda         #HYX_ARGS_SIZE - 1                  ;   isn't open)
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            lda         #SH_ARGS_FD
            jsr         IO_READ
            ldx         #0
            bcs         :+
            ldx         ZP_IO_CNT
:
            stz         HYX_ARGS,X
            lda         #SH_ARGS_FD
            jsr         IO_CLOSE
            lda         SH_HDR + HYX_ENTRY
            sta         ZP_FAR_VEC
            lda         SH_HDR + HYX_ENTRY + 1
            sta         ZP_FAR_VEC + 1
            stz         ZP_FAR_PAGE
            lda         #<HYX_ARGS                          ; (.A.Y = the arguments)
            sta         ZP_FAR_A
            ldy         #>HYX_ARGS
            jsr         FAR_CALL_A                          ; The program (ROM page 0: the $F8xx calls)

@done:
            rts
