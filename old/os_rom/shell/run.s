.debuginfo

; ****************************************************************************
; Running programs (BIOS ROM page 7): what run and a program's name do (SH_CMD's SHC_RUN, SHC_EXEC), the
; executable loader (SH_LOAD, in the program's own task), and the wait for a program to end (SHC_WAIT).
;
;   A program is a file: a Hydra executable (.hyx), which starts with a header (include/shell.inc), a song
; (.zsm: "zm"), or else a HyForth script (.hys).  run tells them apart by the header, not the name.  An
; executable gets a new task: the loader reads it into the task's RAM and calls it.  A song gets one too, for
; the song player (page C: sound/player.s).  A script is read by a copy of the shell (HyForth's run:
; TASK_CLONE).  Either way the program's file is on fd SH_RUN_FD for its task, which has
; copies of the shell's fds, namespace and current directory, and the console while it runs, if the shell
; has it (SH_WAIT).

.segment "SHELL_P7"

SH_HDR          = $0100                                     ; The loader's copy of the header (the bottom of
                                                            ;   its new task's stack page)

; run: the program at .A.Y.  An executable is run, and waited for: .A = 0.  A script is left open on
; SH_RUN_FD, at its start, for the caller to run (HyForth's run): .A = SH_RUN_FD
SH_RUN:
            sta         PAGE1::SHNAMEP                      ; (Its name, as run: HYX_NAME)
            sty         PAGE1::SHNAMEP + 1
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
            bcc         :+
            jmp         @close
:
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
            LOAD_ADDR   PAGE1::SHOWBUF, ZP_TEMP_VEC3        ; ... to HYX_RAM_END (its code, BSS and HYX_TOP)
            jsr         SH_HYX_TOP
            bcs         @bad
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
            lda         ZP_IO_CNT + 1                       ; A song: a whole ZSM header, and "zm"?
            bne         :+
            lda         ZP_IO_CNT
            cmp         #SH_ZSM_HDR_SIZE
            bcc         @not_song
:
            lda         PAGE1::SHOWBUF
            cmp         #'z'
            bne         @not_song
            lda         PAGE1::SHOWBUF + 1
            cmp         #'m'
            bne         @not_song
            jmp         SH_SONG

@not_song:
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
SH_S_EDIT:  .byte   "edit", 0
SH_ZSM_HDR_SIZE = 16

; A song, on SH_RUN_FD at its start: the song player (page C) in a task of its own, which has it, and the
; arguments (PAGE1::ARGLINE: how many times to play its loop), on SH_ARGS_FD; the shell waits for it, as for a
; program (or not: &).  OUT: C = 0, .A = 0; or C = 1, .A = an error
SH_SONG:
            jsr         SH_ARGS_OUT
            lda         #<::ZSM_PLAY_PC
            ldy         #>::ZSM_PLAY_PC
            ldx         #$C
            jsr         TASK_RUN
            php
            pha
            lda         #SH_ARGS_FD                         ; (Ours go: the player has them)
            jsr         IO_CLOSE
            pla
            plp
            bcs         @failed
            jsr         SH_WAIT
            lda         #SH_RUN_FD
            jsr         IO_CLOSE
            lda         #0
            clc
            rts

@failed:
            pha
            lda         #SH_RUN_FD
            jsr         IO_CLOSE
            pla
            sec
            rts

; play: the song at .A.Y (a ZSM file), as run would play it; anything else is ERR_IO_NOT_EXEC (an executable
; runs, as run would: play is for songs, but it's harmless).  OUT: C = 0; or C = 1, .A = an error
SH_PLAY:
            jsr         SH_RUN
            bcs         @done
            cmp         #SH_RUN_FD
            bne         @done                               ; (It ran: C = 0)
            jsr         IO_CLOSE                            ; A script: not for play
            lda         #ERR_IO_NOT_EXEC
            sec

@done:
            rts

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
            lda         #<SH_S_EDIT                         ; (Its name)
            sta         PAGE1::SHNAMEP
            lda         #>SH_S_EDIT
            sta         PAGE1::SHNAMEP + 1
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

; SH_ARGS_FD = a pipe with a program's arguments in it (its writing end closed, so the reader gets them, then the
; end of the file): PAGE1::ARGLINE, as a block of HYX_ARGS_SIZE bytes, then its name (PAGE1::SHNAMEP: as it was
; run) as a block of HYX_NAME_SIZE (in PAGE1::SHBUF: the path's done with).  The loader reads them to HYX_ARGS
; and HYX_NAME; the editor reads the first.  No pipe free: it isn't open, and the program gets none.
; Modifies: .A, .X, .Y
SH_ARGS_OUT:
            jsr         IO_PIPE                             ; .A = the reading end, .X = the writing end
            bcs         @done
            pha
            phx
            LOAD_ADDR   PAGE1::ARGLINE, ZP_IO_BUF
            lda         #HYX_ARGS_SIZE
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            pla                                             ; The writing end: them ...
            pha
            jsr         IO_WRITE
            lda         ZP_TEMP_VEC3                        ; ... its name ...
            pha
            lda         ZP_TEMP_VEC3 + 1
            pha
            lda         PAGE1::SHNAMEP
            sta         ZP_TEMP_VEC3
            lda         PAGE1::SHNAMEP + 1
            sta         ZP_TEMP_VEC3 + 1
            ldy         #0
:
            lda         (ZP_TEMP_VEC3),Y
            sta         PAGE1::SHBUF,Y
            beq         :+
            iny
            cpy         #HYX_NAME_SIZE - 1
            bne         :-
            lda         #0
            sta         PAGE1::SHBUF,Y
:
            pla
            sta         ZP_TEMP_VEC3 + 1
            pla
            sta         ZP_TEMP_VEC3
            LOAD_ADDR   PAGE1::SHBUF, ZP_IO_BUF
            lda         #HYX_NAME_SIZE
            sta         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            pla
            pha
            jsr         IO_WRITE
            pla                                             ; ... then closed
            jsr         IO_CLOSE
            pla                                             ; The reading end: SH_ARGS_FD
            pha
            ldx         #SH_ARGS_FD
            jsr         IO_DUP2
            pla
            jmp         IO_CLOSE

@done:
            rts

; A program by its name (.A.Y, with no .hyx, .hys or .zsm; a word HyForth doesn't know): name.hyx, name.hys or
; name.zsm (a song), in the current directory; then, for a name with no '/', in /bin, as Plan 9's path=(. /bin):
; the namespace's union of the program caches, the boot card's /bin and the ROM's (docs/plans/NAMESPACES.md);
; then in $PATH's directories.  The first one there is run as SH_RUN does.  None: .A = ERR_IO_NOT_FOUND
SH_EXEC:
            ldx         #SH_FIND_PROG
            bra         SH_FIND

; A HyForth library by its name (.A.Y, with no .hyl; lib): name.hyl, looked for as SH_EXEC looks for a
; program, but in /lib (a union, as /bin) and $LIBPATH's directories.  The first one there is opened.
; OUT: C = 0, .A = its fd (read); or C = 1, .A = ERR_IO_NOT_FOUND
SH_LIBOPEN:
            ldx         #SH_FIND_LIB

; ... either (.X = SH_FIND_PROG, SH_FIND_LIB: what to look for, in the tables below)
SH_FIND:
            stx         PAGE1::SHFIND
            sta         PAGE1::SHNAMEP                      ; (Its name, as run: HYX_NAME)
            sty         PAGE1::SHNAMEP + 1
            jsr         SH_KEEP                             ; SH_PTR = the name
            ldx         #0
            jsr         SH_EXEC_TRY
            ldy         #0                                  ; (Back here: not found)

@slash:
            lda         (SH_PTR),Y
            beq         @bin
            cmp         #'/'
            bne         :+
            jmp         @none                               ; (A '/': only where it says)
:
            iny
            bne         @slash

@bin:
            ldx         #0                                  ; /bin/name (/lib/): the union
            jsr         SH_FIND_IN                          ; (Found: it doesn't come back)

@path:
            ldx         PAGE1::SHFIND                       ; $PATH ($LIBPATH): the directories to look in,
            lda         SH_FIND_ENV,X                       ;   with :s between them (SHOWBUF)
            ldy         SH_FIND_ENV + 1,X
            jsr         SH_ENV_READ
            bcs         @none                               ; (None)
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

@none:
            lda         #ERR_IO_NOT_FOUND
            sec
            rts

; SHBUF from .X on = /bin/ (SH_FIND_LIB: /lib/), then the name (SH_EXEC_TRY): tried.  Found: it doesn't come back
SH_FIND_IN:
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
            jmp         SH_EXEC_TRY

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
SH_S_EXTS:  .byte   ".hyx", 0, ".hys", 0, ".zsm", 0
SH_S_EXT_LIB:
            .byte   ".hyl", 0
SH_S_EXTS_END:

; SHBUF from .X on = the name (SH_PTR), then .hyx, .hys or .zsm (SH_FIND_LIB: .hyl): the first that opens is run
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

; Wait for task .A (one the shell started) to end (TASK_JOIN: while it runs, it has the console, if we have it,
; so Ctrl-C goes to it), and keep its exit status: HyForth's (PAGE1::HYSTAT, HYSTATMSG) and $status
; (SH_STATUS_OUT).  A program started with & (PAGE1::SHBG) isn't waited for: its task is shown ("[B]") and kept in
; $apid, and it runs alongside the shell (fg brings it to the front; HyForth's wait waits for it).
; OUT: C = 0, .A = its code (0: success)
SH_WAIT:
            ldx         PAGE1::SHBG
            beq         @wait
            stz         PAGE1::SHBG
            jmp         SH_BACKGROUND

@wait:
            pha                                             ; (The task: LOAD_ADDR uses .A)
            LOAD_ADDR   PAGE1::HYSTATMSG, ZP_IO_BUF
            pla
            jsr         TASK_JOIN
            bcs         @done
            sta         PAGE1::HYSTAT
            jsr         SH_STATUS_OUT
            lda         PAGE1::HYSTAT
            clc

@done:
            rts

; $status (/env/status, as Plan 9's rc has it): HyForth's status as text: its message (PAGE1::HYSTATMSG), or, with
; none, its code (PAGE1::HYSTAT) in decimal, or "" for success (0).  (No /env: nothing.)  OUT: C = 0
; Modifies: .A, .X, .Y
SH_STATUS_OUT:
            lda         PAGE1::HYSTATMSG
            bne         @text
            lda         PAGE1::HYSTAT
            beq         @text
            ldx         #0                                  ; The code in decimal, in HYSTATMSG
            ldy         #100
            jsr         SH_DIGIT
            ldy         #10
            jsr         SH_DIGIT
            ora         #'0'
            sta         PAGE1::HYSTATMSG,X
            stz         PAGE1::HYSTATMSG + 1,X

@text:
            LOAD_ADDR   PAGE1::HYSTATMSG, ZP_TEMP_VEC3
            lda         #<SH_S_STATUS
            ldy         #>SH_S_STATUS

; ... the text at ZP_TEMP_VEC3 (in RAM) to the environment's variable .A.Y (a name on this page)
SH_ENV_OUT:
            stz         ZP_IO_BUF                           ; (IO_CREATE: its mode bits)
            ldx         #IO_MODE_WRITE
            jsr         IO_CREATE                           ; (Made, or emptied)
            bcs         @done
            pha
            lda         ZP_TEMP_VEC3
            sta         ZP_IO_BUF
            lda         ZP_TEMP_VEC3 + 1
            sta         ZP_IO_BUF + 1
            ldy         #$FF
:
            iny
            lda         (ZP_TEMP_VEC3),Y
            bne         :-
            sty         ZP_IO_CNT
            stz         ZP_IO_CNT + 1
            pla
            pha
            jsr         IO_WRITE
            pla
            jsr         IO_CLOSE

@done:
            clc
            rts

; .A's digit for .Y (100, 10) into HYSTATMSG at .X, if it's not a leading 0 (.X = 0 still).
; OUT: .A = what's left.  Modifies: .X, .Y, ZP_TEMP
SH_DIGIT:
            sty         ZP_TEMP
            ldy         #'0' - 1
:
            iny
            sec
            sbc         ZP_TEMP
            bcs         :-
            adc         ZP_TEMP                             ; (C = 0)
            cpy         #'0'
            bne         :+
            cpx         #0
            beq         @done                               ; (A leading 0)
:
            pha
            tya
            sta         PAGE1::HYSTATMSG,X
            inx
            pla

@done:
            rts

; A program started with & (task .A): "[B]" (its task) on the screen, and $apid.  OUT: C = 0
SH_BACKGROUND:
            and         #$0F
            ora         #'0'
            cmp         #'9' + 1
            bcc         :+
            adc         #'A' - '9' - 2                      ; (C = 1: A-F)
:
            sta         PAGE1::SHBUF2                       ; (Its digit: $apid's text)
            stz         PAGE1::SHBUF2 + 1
            lda         #'['
            jsr         WRITE_CHAR
            lda         PAGE1::SHBUF2
            jsr         WRITE_CHAR
            lda         #']'
            jsr         WRITE_CHAR
            LOAD_ADDR   PAGE1::SHBUF2, ZP_TEMP_VEC3
            lda         #<SH_S_APID
            ldy         #>SH_S_APID
            jmp         SH_ENV_OUT

SH_S_STATUS:    .byte   "/env/status", 0
SH_S_APID:      .byte   "/env/apid", 0

; ****************************************************************************
; The command shell (SHELL_CMD, $F8F6: a task's entry point, for TASK_RUN; C's system(): Plan 9's rc -c): HyForth,
; reading its stdin (its starter's: a pipe with a command line, say) with no banner and no prompt (CMDFLAG), and
; ending at its end with the last command's status (HyForth's LINE_EOF: TASK_EXITS).  Its fds, namespace, current
; directory and environment are its starter's.
SH_CMDSHELL:
            jsr         COPYTORAM
            lda         #1
            sta         PAGE1::CMDFLAG
            jsr         forth_main                          ; (It ends at its input's end; or, bye: here)
            LOAD_ADDR   PAGE1::HYSTATMSG, ZP_IO_BUF
            lda         PAGE1::HYSTAT
            jmp         TASK_EXITS

; The page after a program's RAM (its code, its BSS: HYX_BSS, and HYX_TOP, whichever's higher), from its header
; at ZP_TEMP_VEC3.  OUT: C = 0, .A = the page; or C = 1: past $FFFF.  Modifies: .X, .Y
SH_HYX_TOP:
            ldy         #HYX_LOAD                           ; Its code's end ...
            lda         (ZP_TEMP_VEC3),Y
            ldy         #HYX_LENGTH
            clc
            adc         (ZP_TEMP_VEC3),Y
            tax
            ldy         #HYX_LOAD + 1
            lda         (ZP_TEMP_VEC3),Y
            ldy         #HYX_LENGTH + 1
            adc         (ZP_TEMP_VEC3),Y
            bcs         @done
            pha                                             ; ... and its BSS's
            txa
            ldy         #HYX_BSS
            clc
            adc         (ZP_TEMP_VEC3),Y
            tax
            pla
            ldy         #HYX_BSS + 1
            adc         (ZP_TEMP_VEC3),Y
            bcs         @done
            pha                                             ; (.A.X, on the stack: the end)
            txa
            ldy         #HYX_TOP                            ; HYX_TOP higher?
            cmp         (ZP_TEMP_VEC3),Y
            pla
            pha
            ldy         #HYX_TOP + 1
            sbc         (ZP_TEMP_VEC3),Y
            pla
            bcs         :+
            ldy         #HYX_TOP                            ; (Yes)
            lda         (ZP_TEMP_VEC3),Y
            tax
            iny
            lda         (ZP_TEMP_VEC3),Y
:
            cpx         #1                                  ; (C = 1: part of a page more)
            adc         #0                                  ; (C = 1: past $FF)

@done:
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
            LOAD_ADDR   SH_HDR, ZP_TEMP_VEC3                ; The page floor: after its RAM (its code, its BSS,
            jsr         SH_HYX_TOP                          ;   its HYX_TOP; SH_RUN checked it fits)
            jsr         MM_SET_FLOOR
            bcc         :+
            rts                                             ; (The loading fails: the task ends)
:
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
            bcc         :+
            rts
:
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
            lda         SH_HDR + HYX_BSS                    ; Its BSS: after its code (IO_READ left ZP_IO_BUF
            ldx         SH_HDR + HYX_BSS + 1                ;   there), cleared
            ldy         #0
@clear:
            cmp         #0
            bne         :+
            cpx         #0
            beq         @cleared
:
            pha
            lda         #0
            sta         (ZP_IO_BUF),Y
            inc         ZP_IO_BUF
            bne         :+
            inc         ZP_IO_BUF + 1
:
            pla
            sec
            sbc         #1
            bcs         @clear
            dex
            bra         @clear

@cleared:
            lda         #SH_RUN_FD
            jsr         IO_CLOSE
            LOAD_ADDR   HYX_ARGS, ZP_IO_BUF                 ; Its arguments and its name, from SH_ARGS_FD (none,
            lda         #HYX_ARGS_SIZE + HYX_NAME_SIZE      ;   if it isn't open): their blocks, at HYX_ARGS
            sta         ZP_IO_CNT                           ;   and HYX_NAME
            stz         ZP_IO_CNT + 1
            lda         #SH_ARGS_FD
            jsr         IO_READ
            ldx         #0
            bcs         :+
            ldx         ZP_IO_CNT
:
            cpx         #HYX_ARGS_SIZE + 1
            bcs         :+
            stz         HYX_NAME                            ; (No name came)
:
            stz         HYX_ARGS,X
            stz         HYX_ARGS + HYX_ARGS_SIZE - 1        ; (Both ended)
            stz         HYX_NAME + HYX_NAME_SIZE - 1
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
