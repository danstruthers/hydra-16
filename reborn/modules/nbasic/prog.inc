; ****************************************************************************
; prog.inc - the program's text (the seventh bank): the text region's lines, each its length and its bytes, a length
; of $FF after the last; a file loaded into it (LOAD, a script), LIST, NEW, the lines typed with a number first; the
; prompt's commands; and the line table's look-up for an error's line.

FB_SIZE         = 128           ; (A file read this much at a time)

.bss
fl_fd:      .res        1                                   ; (A file loaded: its fd, its buffer, its bytes, the next)
fl_buf:     .res        FB_SIZE
fl_len:     .res        1
fl_pos:     .res        1
fl_line:    .res        2                                   ;   its lines so far
pc_kw:      .res        1                                   ; (prompt_cmd's keyword)

.segment "CODE7"

; The text emptied
text_init:
            stz         t0
            stz         t0 + 1
            lda         #LB_TEXT
            jsr         cb_reg7
            lda         #$FF
            jsr         cb_wr
            stz         pname
            jmp         code_sel

; Cursor B at the region address t0 of the region whose first logical bank is .A (comp.inc's cb_reg, this bank's)
cb_reg7:
            sta         cbl
            lda         t0 + 1
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            clc
            adc         cbl
            sta         cbl
            lda         t0 + 1
            and         #$1F
            ora         #WIN
            sta         cb + 1
            lda         t0
            sta         cb
            jmp         cb_sel

; A script's file (the first argument: argp) loaded; pname its path
script_load:
            ldy         #0
:
            lda         (argp),y
            sta         pname,y
            beq         :+
            iny
            cpy         #PATH_MAX
            bne         :-
            lda         #0
            sta         pname,y
:
            ; (fall through)

; The file at pname loaded into the text (its lines; a CR before an LF dropped).  An error: err_entry's
text_load:
            LDR         r0, pname
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            ldx         #1                                  ; (The system's error)
            jmp         b_error
:
            sta         fl_fd
            stz         fl_len
            stz         fl_pos
            stz         fl_line
            stz         fl_line + 1
            stz         t0
            stz         t0 + 1
            lda         #LB_TEXT
            jsr         cb_reg7
@line:
            ldx         #0                                  ; (A line into lbuf: x its length)
@byte:
            phx
            jsr         fl_getc
            plx
            bcs         @eof
            cmp         #LF
            beq         @put
            cpx         #254
            bcs         @long
            sta         lbuf,x
            inx
            bra         @byte
@eof:
            cpx         #0
            beq         @done
@put:
            inc         fl_line
            bne         :+
            inc         fl_line + 1
:
            cpx         #0                                  ; (A CR at its end: dropped)
            beq         :+
            lda         lbuf - 1,x
            cmp         #CR
            bne         :+
            dex
:
            stx         t4
            jsr         cb_sel
            lda         t4
            jsr         cb_wr
            ldy         #0
@w:
            cpy         t4                                  ; (Its bytes)
            beq         @wd
            lda         lbuf,y
            jsr         cb_wr
            iny
            bra         @w
@wd:
            jmp         @line
@long:
            lda         fl_fd
            jsr         CLOSE
            MOVW        err_line, fl_line
            lda         #ER_TOOLONG
            ldx         #0
            jmp         b_error
@done:
            jsr         cb_sel
            lda         #$FF
            jsr         cb_wr
            lda         fl_fd
            jsr         CLOSE
            jmp         code_sel

; The loaded file's next byte.  OUT: C = 1 at its end
fl_getc:
            ldx         fl_pos
            cpx         fl_len
            bcc         @have
            LDR         r0, fl_buf
            LDR         r1, FB_SIZE
            lda         fl_fd
            jsr         READ
            bcs         @end
            sta         fl_len
            stz         fl_pos
            cmp         #0
            beq         @end
            ldx         #0
@have:
            lda         fl_buf,x
            inx
            stx         fl_pos
            clc
            rts
@end:
            sec
            rts

; ---- The prompt's commands: the line in lbuf (lp).  OUT: C = 0 it was one (done); C = 1 not one
prompt_cmd:
            FARN        2, lex_next
            lda         tk
            cmp         #TK_KW
            bne         @not
            lda         tkv
            cmp         #K_RUN
            beq         @run
            cmp         #K_LIST
            beq         @list
            cmp         #K_NEW
            beq         @new
            cmp         #K_SYSTEM
            beq         @bye
            cmp         #K_BYE
            beq         @bye
@not:
            LDW         lp, lbuf                            ; (Read again by the compiler)
            sec
            rts
@run:
            FARN        1, prog_compile
            FARN        1, prog_run
            clc
            rts
@list:
            jsr         text_list
            clc
            rts
@new:
            jsr         text_init
            FARN        2, prog_init
            clc
            rts
@bye:
            jsr         out_flush
            lda         #0
            LDR         r0, 0
            jmp         EXITS

; LIST: every line of the text
text_list:
            stz         t0
            stz         t0 + 1
            lda         #LB_TEXT
            jsr         cb_reg7
@line:
            jsr         cb_sel
            jsr         cb_rd
            cmp         #$FF
            beq         @done
            tax
            beq         @nl
:
            jsr         cb_sel
            jsr         cb_rd
            jsr         out_byte
            dex
            bne         :-
@nl:
            lda         #LF
            jsr         out_byte
            bra         @line
@done:
            jmp         code_sel

; A line typed with a number first: into the program at its number (not yet)
text_line:
            lda         #73
            ldx         #0
            jmp         b_error

; ---- An error's line: the line table's last statement at or before the code address t0 (the program's code: the
; prompt's, none).  OUT: err_line
lt_find:
            stz         err_line
            stz         err_line + 1
            lda         t0 + 1
            cmp         #LB_IMM << 5
            bcs         @done
            MOVW        t4, t0                              ; (The address sought)
            stz         t0
            stz         t0 + 1
            lda         #LB_LT
            jsr         ca_reg7
            stz         t0 + 2                              ; (The entries read so far)
            stz         t0 + 3
@entry:
            lda         t0 + 2                              ; (Past the last: done)
            cmp         ltop
            lda         t0 + 3
            sbc         ltop + 1
            bcs         @done
            jsr         ca_rd                               ; (Its address: past the one sought, done)
            sta         t4 + 2
            jsr         ca_rd
            sta         t4 + 3
            lda         t4
            cmp         t4 + 2
            lda         t4 + 1
            sbc         t4 + 3
            bcc         @done
            jsr         ca_rd
            sta         err_line
            jsr         ca_rd
            sta         err_line + 1
            lda         t0 + 2
            clc
            adc         #4
            sta         t0 + 2
            bcc         @entry
            inc         t0 + 3
            bra         @entry
@done:
            jmp         code_sel

ca_reg7:
            sta         cal
            lda         t0 + 1
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            clc
            adc         cal
            sta         cal
            lda         t0 + 1
            and         #$1F
            ora         #WIN
            sta         ca + 1
            lda         t0
            sta         ca
            jmp         ca_sel
