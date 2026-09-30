.debuginfo

; ****************************************************************************
; Redirection (BIOS ROM page 7): a line's ">", ">>" and "<" (HyForth's getline calls SH_REDIR once a line is
; read, after its pipeline is split off: PIPECHK), and putting stdin and stdout back when the next line is
; read (SH_UNREDIR).  They apply to the command the shell runs itself: a whole line, or a pipeline's last.
;   file > name     stdout to a file: made, or emptied
;   file >> name    stdout added to a file's end (made if it isn't there)
;   file < name     stdin from a file
; The name is relative, or not, and "in quotes" if it has spaces.  Each is blanked out of the line, so the
; command doesn't see it.  The fd it replaces is kept in PAGE1::REDOUT or REDIN ($FF: none).

.segment "SHELL_P7"

SH_R_KIND       = PAGE1::SHN                                ; The redirection: 0 >, 1 >>, 2 <
SH_R_AT         = PAGE1::SHSEL                              ; Where it is in the line
SH_R_FD         = PAGE1::SHFD                               ; The file's fd

; The line's redirections, from CURBUF on (TIB), outside q^...^ and "..." strings: each is done, and blanked
; out.  OUT: C = 0; or C = 1, .A = error (the line mustn't run)
SH_REDIR:
            ldy         PAGE1::CURBUF
            ldx         #0                                  ; (In a string: 1 q^...^, 2 "..."; 0 not)

@scan:
            iny
            beq         @done
            lda         (PAGE1::TIB),Y
            beq         @done
            cmp         #'^'
            bne         @quote
            cpx         #2
            beq         @scan                               ; (A '^' in a "..." string)
            txa
            eor         #1
            tax
            bra         @scan

@quote:
            cmp         #'"'
            bne         @op
            cpx         #1
            beq         @scan                               ; (A '"' in a q^...^ string)
            txa
            eor         #2
            tax
            bra         @scan

@op:
            cpx         #0
            bne         @scan                               ; (In a string)
            cmp         #'<'
            beq         @in
            cmp         #'>'
            bne         @scan
            jsr         SH_R_ALONE                          ; ">" alone?
            bcs         :+
            lda         #0
            bra         @found
:
            iny                                             ; ">>" alone?
            lda         (PAGE1::TIB),Y
            cmp         #'>'
            bne         @back
            jsr         SH_R_ALONE
            dey
            bcs         @next
            lda         #1
            bra         @found

@in:
            jsr         SH_R_ALONE                          ; "<" alone?
            bcs         @next
            lda         #2

@found:
            sta         SH_R_KIND
            sty         SH_R_AT
            jsr         SH_R_DO                             ; Done, and blanked out
            bcs         @fail
            ldy         SH_R_AT                             ; (On from where it was: the next one)
            ldx         #0
            bra         @scan

@back:
            dey

@next:
            ldx         #0
            bra         @scan

@done:
            clc

@fail:
            rts

; Is the character at .Y (a '<' or '>'; or a '>>''s second) a word of its own: a space before it (or the
; first '>' of '>>'), and a space (or the line's end) after it?  OUT: C = 0: it is.  Keeps .X, .Y
SH_R_ALONE:
            dey
            lda         (PAGE1::TIB),Y
            iny
            cmp         #' '
            beq         :+
            cmp         #'>'
            bne         @no
:
            iny
            lda         (PAGE1::TIB),Y
            dey
            beq         @yes
            cmp         #' '
            bne         @no

@yes:
            clc
            rts

@no:
            sec
            rts

; The redirection at SH_R_AT (SH_R_KIND): its name into SHBUF2, the whole of it blanked out of the line, the
; file opened, and fd 1 or 0 pointed at it (the one it had kept, the first time).
; OUT: C = 0; or C = 1, .A = error
SH_R_DO:
            ldy         SH_R_AT                             ; After the '>', '>>' or '<'
            iny
            lda         SH_R_KIND
            cmp         #1
            bne         :+
            iny
:
            lda         (PAGE1::TIB),Y                      ; (Spaces before the name)
            cmp         #' '
            bne         :+
            iny
            bne         :-
:
            ldx         #' '                                ; The name: to a space; or "in quotes"
            cmp         #'"'
            bne         :+
            iny
            ldx         #'"'
:
            stx         SH_PTR2                             ; (What ends it)
            ldx         #0

@name:
            lda         (PAGE1::TIB),Y
            beq         @named
            cmp         SH_PTR2
            beq         @close
            cpx         #63
            bcs         :+                                  ; (Too long: the rest left out)
            sta         PAGE1::SHBUF2,X
            inx
:
            iny
            bne         @name

@close:
            cmp         #'"'                                ; (Past the closing '"')
            bne         @named
            iny

@named:
            stz         PAGE1::SHBUF2,X
            sty         SH_PTR2                             ; (Where it ends)
            ldy         SH_R_AT                             ; Blanked out, from the '<' or '>' to there
            lda         #' '
:
            sta         (PAGE1::TIB),Y
            iny
            cpy         SH_PTR2
            bne         :-
            txa
            bne         :+
            lda         #ERR_IO_NAME                        ; (No name)
            sec
            rts
:
            lda         SH_R_KIND                           ; The file
            beq         @create
            cmp         #1
            beq         @append
            lda         #<PAGE1::SHBUF2                     ; <: for reading
            ldy         #>PAGE1::SHBUF2
            ldx         #IO_MODE_READ
            jsr         IO_OPEN
            bcs         @done
            sta         SH_R_FD
            lda         PAGE1::REDIN                        ; stdin kept (the first time): with its read-ahead
            bpl         :+                                  ;   given back (SH_INSAVE)
            jsr         SH_INSAVE
            bcs         @close_fail
            sta         PAGE1::REDIN
:
            ldx         #0
            bra         @point

@append:
            lda         #<PAGE1::SHBUF2                     ; >>: the file, for writing; or a new one
            ldy         #>PAGE1::SHBUF2
            ldx         #IO_MODE_WRITE
            jsr         IO_OPEN
            bcc         :+
            cmp         #ERR_IO_NOT_FOUND
            beq         @create
            sec                                             ; (Another error)
            rts
:
            sta         SH_R_FD
            LOAD_ADDR   PAGE1::SHOWBUF, ZP_IO_BUF           ; ... at its end: its size (its stat record)
            lda         SH_R_FD
            jsr         IO_STAT
            bcs         @close_fail
            ldx         #3
:
            lda         PAGE1::SHOWBUF + IO_ST_SIZE,X
            sta         ZP_IO_OFS,X
            dex
            bpl         :-
            lda         SH_R_FD
            jsr         IO_SEEK
            bcs         @close_fail
            bra         @out

@create:                                                    ; >: made, or emptied
            stz         ZP_IO_BUF                           ; (A file: mode 0)
            lda         #<PAGE1::SHBUF2
            ldy         #>PAGE1::SHBUF2
            ldx         #IO_MODE_WRITE
            jsr         IO_CREATE
            bcs         @done
            sta         SH_R_FD

@out:
            lda         PAGE1::REDOUT                       ; stdout kept (the first time)
            bpl         :+
            lda         #1
            jsr         IO_DUP
            bcs         @close_fail
            sta         PAGE1::REDOUT
:
            ldx         #1

@point:                                                     ; fd .X = the file
            lda         SH_R_FD
            jsr         IO_DUP2
            bcs         @close_fail
            lda         SH_R_FD
            jsr         IO_CLOSE
            clc

@done:
            rts

@close_fail:                                                ; (Keeps the error)
            pha
            lda         SH_R_FD
            jsr         IO_CLOSE
            pla
            sec
            rts

; Keep stdin (for HyForth's include and pipelines too: its INSAVE): another fd for it, whose offset moves
; back by what stdin has read ahead but not used yet, as the read-ahead is dropped when fd 0 changes (e.g.
; the rest of a script that includes one).  OUT: C = 0: .A = the fd; or C = 1, .A = error.  Modifies: .X, .Y
SH_INSAVE:
            lda         #0
            jsr         IO_DUP
            bcs         @done
            pha
            asl                                             ; .X = its entry in the fd table
            asl
            asl
            tax
            lda         ZP_IN_CNT                           ; The bytes read ahead, not used
            sec
            sbc         ZP_IN_POS
            sta         SH_PTR2
            lda         IO_FD_OFS,X
            sec
            sbc         SH_PTR2
            sta         IO_FD_OFS,X
            lda         IO_FD_OFS + 1,X
            sbc         #0
            sta         IO_FD_OFS + 1,X
            lda         IO_FD_OFS + 2,X
            sbc         #0
            sta         IO_FD_OFS + 2,X
            lda         IO_FD_OFS + 3,X
            sbc         #0
            sta         IO_FD_OFS + 3,X
            pla
            clc

@done:
            rts

.assert     IO_FD_SIZE = 8, error, "SH_INSAVE: an fd's entry is 8 bytes"

; The line's done: stdin and stdout back as they were before its redirections (and the files closed:
; stdout's last output goes to its file first).  Modifies: .A, .X, .Y
SH_UNREDIR:
            lda         PAGE1::REDOUT
            bmi         :+
            ldx         #1
            jsr         IO_DUP2
            lda         PAGE1::REDOUT
            jsr         IO_CLOSE
            lda         #$FF
            sta         PAGE1::REDOUT
:
            lda         PAGE1::REDIN
            bmi         :+
            ldx         #0
            jsr         IO_DUP2
            lda         PAGE1::REDIN
            jsr         IO_CLOSE
            lda         #$FF
            sta         PAGE1::REDIN
:
            clc
            rts
