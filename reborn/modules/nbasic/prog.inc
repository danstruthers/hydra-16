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
sv_ptr:     .res        2                                   ; (text_load's and text_save's path)
tx_at:      .res        2                                   ; (A line's place in the text, its length, its number, the
tx_len:     .res        1                                   ;   number sought, the text's end ($FF's place), a move's
tx_ln:      .res        2                                   ;   source, destination and count, scratch)
tx_want:    .res        2
tx_eq:      .res        1
tx_end:     .res        2
tx_src:     .res        2
tx_dst:     .res        2
tx_n:       .res        2
tx_t:       .res        2
tx_newl:    .res        1
tx_del:     .res        1
tx_lab:     .res        NAME_MAX + 2                        ; (A line's label, its length: 0 none)
tx_labl:    .res        1
ls_fk:      .res        1                                   ; (LIST's, DELETE's, EDIT's from and to: their kinds (0 none,
ls_tk:      .res        1                                   ;   1 a number, 2 a label), numbers, names)
ls_from:    .res        2
ls_to:      .res        2
ls_fname:   .res        NAME_MAX + 2
ls_fl:      .res        1
ls_tname:   .res        NAME_MAX + 2
ls_tl:      .res        1
ls_state:   .res        1
sp_k:       .res        1                                   ; (ls_spec's)
sp_n:       .res        2
ed_path:    .res        24                                  ; (EDIT's file: /ram/basicNN.bas)
ed_args:    .res        40

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
            LDW         sv_ptr, pname
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

; The file at sv_ptr (pname's, EDIT's) loaded into the text (its lines; a CR before an LF dropped).  An error:
; err_entry's
text_load:
            MOVW        r0, sv_ptr
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
            bne         @lb911
            jmp         @list
@lb911:
            cmp         #K_NEW
            bne         @lb909
            jmp         @new
@lb909:
            cmp         #K_SYSTEM
            bne         @lb908
            jmp         pc_bye
@lb908:
            cmp         #K_BYE
            bne         @lb907
            jmp         pc_bye
@lb907:
            cmp         #K_LOAD
            bne         @lb910
            jmp         @load
@lb910:
            cmp         #K_SAVE
            bne         @lb906
            jmp         @save
@lb906:
            cmp         #K_DELETE
            bne         @lb905
            jmp         @delete
@lb905:
            cmp         #K_CLEAR
            bne         @lb904
            jmp         @clear
@lb904:
            cmp         #K_CONT
            bne         @lb903
            jmp         @cont
@lb903:
            cmp         #K_EDIT
            bne         @not
            jsr         text_edit
            clc
            rts
@not:
            LDW         lp, lbuf                            ; (Read again by the compiler)
            sec
            rts
@run:
            FARN        2, lex_next                         ; (RUN "f.bas": loaded first)
            lda         tk
            cmp         #TK_STR
            bne         :+
            jsr         load_name
:
            FARN        1, prog_compile
            FARN        1, prog_run
            clc
            rts
@list:
            jsr         ls_args
            jsr         text_list
            clc
            rts
@new:
            jsr         text_init
            jsr         prog_gone
            clc
            rts
@load:
            FARN        2, lex_next
            lda         tk
            cmp         #TK_STR
            beq         @lb902
            jmp         @syntax
@lb902:
            jsr         load_name
            clc
            rts
@save:
            FARN        2, lex_next
            lda         tk
            cmp         #TK_STR
            bne         :+
            jsr         tsp_pname
:
            lda         pname
            bne         :+
            lda         #ER_BADNAME
            ldx         #0
            jmp         b_error
:
            LDW         sv_ptr, pname
            jsr         text_save
            clc
            rts
@delete:
            jsr         ls_args
            jsr         text_delete
            clc
            rts
@clear:
            stz         cont_ok
            FARN        6, fl_closeall
            FARN        1, prog_compile                     ; (Its variables made again)
            clc
            rts
@cont:
            FARN        1, prog_cont
            clc
            rts
@syntax:
            lda         #ER_SYNTAX
            ldx         #0
            jmp         b_error

; The program's compiled state gone (NEW, LOAD): its symbols and variables, CONT, its files
prog_gone:
            stz         cont_ok
            FARN        6, fl_closeall
            FARN        2, prog_init
            rts

; LOAD "f.bas" (the string token current): the text emptied, the file read into it (pname its path)
load_name:
            jsr         text_init
            jsr         tsp_pname
            jsr         prog_gone
            LDW         sv_ptr, pname
            jmp         text_load

; pname = the string token (tsp, tsl: PATH_MAX at most)
tsp_pname:
            ldy         #0
:
            cpy         tsl
            beq         :+
            cpy         #PATH_MAX
            bcs         :+
            lda         (tsp),y
            sta         pname,y
            iny
            bra         :-
:
            lda         #0
            sta         pname,y
            rts

; The text written to the file at sv_ptr (each line and an LF)
text_save:
            MOVW        r0, sv_ptr
            lda         #O_WRITE | O_TRUNC
            jsr         OPEN
            bcc         @have
            cmp         #E_NOENT
            bne         @err
            MOVW        r0, sv_ptr
            lda         #O_WRITE
            ldx         #0
            jsr         CREATE
            bcs         @err
@have:
            sta         fl_fd
            jsr         out_flush
            lda         ofd
            pha
            lda         fl_fd
            sta         ofd
            stz         tx_at
            stz         tx_at + 1
@line:
            jsr         tx_open
            cmp         #$FF
            beq         @end
            jsr         tx_print
            jsr         tx_skip
            bra         @line
@end:
            jsr         out_flush
            pla
            sta         ofd
            lda         fl_fd
            jsr         CLOSE
            jmp         code_sel
@err:
            jmp         sys_err

; The line at tx_open's cursor written (its tx_len bytes, then an LF)
tx_print:
            ldy         tx_len
            beq         @lf
:
            jsr         ca_sel
            jsr         ca_rd
            jsr         out_byte
            dey
            bne         :-
@lf:
            lda         #LF
            jmp         out_byte
pc_bye:
            jsr         out_flush
            FARN        6, con_cooked
            lda         #0
            LDR         r0, 0
            jmp         EXITS

; LIST [from][-[to]]: the text's lines (all; from a line's number or label to another's; a line or label alone:
; it).  Ctrl-C stops it
text_list:
            lda         #1                                  ; (ls_state 1: written)
            ldx         ls_fk
            beq         :+
            lda         #0
:
            sta         ls_state
            stz         tx_at
            stz         tx_at + 1
@line:
            bit         intr
            bmi         @done
            jsr         tx_open
            cmp         #$FF
            beq         @done
            lda         ls_state
            bne         @in
            lda         ls_fk                               ; (From: a number at least, or the label)
            cmp         #1
            bne         @flabel
            jsr         tx_num
            bcs         @next
            lda         tx_ln
            cmp         ls_from
            lda         tx_ln + 1
            sbc         ls_from + 1
            bcc         @next
            bra         @start
@flabel:
            jsr         tx_label
            jsr         lab_from
            bne         @next
@start:
            inc         ls_state
@in:
            lda         ls_tk                               ; (To: a number past it, done)
            cmp         #1
            bne         @print
            jsr         tx_open
            jsr         tx_num
            bcs         @print
            lda         ls_to
            cmp         tx_ln
            lda         ls_to + 1
            sbc         tx_ln + 1
            bcc         @done
@print:
            jsr         tx_open
            jsr         tx_print
            lda         ls_tk                               ; (To: the label's line written, done)
            cmp         #2
            bne         @next
            jsr         tx_open
            jsr         tx_label
            jsr         lab_to
            beq         @done
@next:
            jsr         tx_skip
            bra         @line
@done:
            lda         #$80
            trb         intr
            jmp         code_sel

; Is the line's label (tx_lab) LIST's from (lab_from) or to (lab_to)?  OUT: Z = 1 yes
lab_from:
            lda         tx_labl
            beq         @no
            cmp         ls_fl
            bne         @no
            tay
:
            lda         tx_lab - 1,y
            cmp         ls_fname - 1,y
            bne         @no
            dey
            bne         :-
            lda         #0
            rts
@no:
            lda         #1
            rts
lab_to:
            lda         tx_labl
            beq         @no
            cmp         ls_tl
            bne         @no
            tay
:
            lda         tx_lab - 1,y
            cmp         ls_tname - 1,y
            bne         @no
            dey
            bne         :-
            lda         #0
            rts
@no:
            lda         #1
            rts

; LIST's, DELETE's, EDIT's [from][-[to]] read (the keyword's token current): ls_fk, ls_from, ls_fname, ls_tk ...  A
; from alone: to the same
ls_args:
            stz         ls_fk
            stz         ls_tk
            FARN        2, lex_next
            lda         tk
            bne         @lb901
            jmp         @done
@lb901:
            cmp         #TK_OP
            bne         :+
            lda         tkv
            cmp         #'-'
            beq         @dash
:
            jsr         ls_spec
            lda         sp_k
            sta         ls_fk
            MOVW        ls_from, sp_n
            ldx         #NAME_MAX + 1
:
            lda         tname,x
            sta         ls_fname,x
            dex
            bpl         :-
            lda         tnl
            sta         ls_fl
            FARN        2, lex_next
            lda         tk
            cmp         #TK_OP
            bne         @single
            lda         tkv
            cmp         #'-'
            bne         @single
@dash:
            FARN        2, lex_next
            lda         tk
            beq         @done
            jsr         ls_spec
            lda         sp_k
            sta         ls_tk
            MOVW        ls_to, sp_n
            ldx         #NAME_MAX + 1
:
            lda         tname,x
            sta         ls_tname,x
            dex
            bpl         :-
            lda         tnl
            sta         ls_tl
            rts
@single:
            lda         ls_fk
            sta         ls_tk
            MOVW        ls_to, ls_from
            ldx         #NAME_MAX + 1
:
            lda         ls_fname,x
            sta         ls_tname,x
            dex
            bpl         :-
            lda         ls_fl
            sta         ls_tl
@done:
            rts

; A line's number (its digits, in decimal: sp_k 1, sp_n) or label (a name: sp_k 2, tname), the token current
ls_spec:
            lda         tk
            cmp         #TK_NAME
            bne         :+
            lda         #2
            sta         sp_k
            rts
:
            cmp         #TK_NUM
            beq         :+
            lda         #ER_SYNTAX
            ldx         #0
            jmp         b_error
:
            lda         #1
            sta         sp_k
            stz         sp_n
            stz         sp_n + 1
            ldy         #0
@dig:
            lda         (tstart),y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @done
            pha
            jsr         sp_x10
            pla
            clc
            adc         sp_n
            sta         sp_n
            bcc         :+
            inc         sp_n + 1
:
            iny
            bra         @dig
@done:
            rts

; sp_n * 10
sp_x10:
            lda         sp_n
            asl         a
            sta         tx_t
            lda         sp_n + 1
            rol         a
            sta         tx_t + 1
            asl         sp_n
            rol         sp_n + 1
            asl         sp_n
            rol         sp_n + 1
            asl         sp_n
            rol         sp_n + 1
            clc
            lda         sp_n
            adc         tx_t
            sta         sp_n
            lda         sp_n + 1
            adc         tx_t + 1
            sta         sp_n + 1
            rts

; ---- The buffer's lines

; Cursor A at the line at tx_at's bytes.  OUT: .A, tx_len its length ($FF: the text's end)
tx_open:
            MOVW        t0, tx_at
            lda         #LB_TEXT
            jsr         ca_reg7
            jsr         ca_rd
            sta         tx_len
            rts

; tx_at on past its line (tx_len's)
tx_skip:
            sec
            lda         tx_at
            adc         tx_len
            sta         tx_at
            bcc         :+
            inc         tx_at + 1
:
            rts

; The line's number (tx_open's: cursor A at its bytes): C = 0, tx_ln it; C = 1 it has none
tx_num:
            stz         tx_ln
            stz         tx_ln + 1
            ldx         tx_len
            ldy         #0
@next:
            txa
            beq         @end
            jsr         ca_rd
            dex
            cmp         #' '
            bne         @dig
            cpy         #0
            beq         @next
            bra         @end
@dig:
            sec
            sbc         #'0'
            cmp         #10
            bcs         @end
            pha
            lda         tx_ln                               ; (* 10: * 2 kept, * 8, added)
            asl         a
            sta         tx_t
            lda         tx_ln + 1
            rol         a
            sta         tx_t + 1
            asl         tx_ln
            rol         tx_ln + 1
            asl         tx_ln
            rol         tx_ln + 1
            asl         tx_ln
            rol         tx_ln + 1
            clc
            lda         tx_ln
            adc         tx_t
            sta         tx_ln
            lda         tx_ln + 1
            adc         tx_t + 1
            sta         tx_ln + 1
            pla
            clc
            adc         tx_ln
            sta         tx_ln
            bcc         :+
            inc         tx_ln + 1
:
            iny
            bra         @next
@end:
            cpy         #0
            beq         :+
            clc
            rts
:
            sec
            rts

; The line's label (tx_open's: a name then a :, spaces before it), upper-cased: tx_lab, tx_labl (0: none)
tx_label:
            stz         tx_labl
            ldx         tx_len
            ldy         #0
@sp:
            txa
            beq         @none
            jsr         ca_rd
            dex
            cmp         #' '
            beq         @sp
@ch:
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
:
            cmp         #'A'
            bcc         @notl
            cmp         #'Z' + 1
            bcc         @ok
@notl:
            cpy         #0
            beq         @none
            cmp         #'0'
            bcc         @end
            cmp         #'9' + 1
            bcc         @ok
            cmp         #'_'
            beq         @ok
            cmp         #'.'
            beq         @ok
            bra         @end
@ok:
            cpy         #NAME_MAX
            bcs         @none
            sta         tx_lab,y
            iny
            txa
            beq         @none
            jsr         ca_rd
            dex
            bra         @ch
@end:
            cmp         #':'
            bne         @none
            sty         tx_labl
@none:
            rts

; tx_at = the first line whose number is tx_want or more: C = 0 (tx_eq <> 0: tx_want's own); C = 1 the text's end
; (tx_at there)
tx_find:
            stz         tx_at
            stz         tx_at + 1
@line:
            jsr         tx_open
            cmp         #$FF
            beq         @end
            jsr         tx_num
            bcs         @next
            lda         tx_ln
            cmp         tx_want
            lda         tx_ln + 1
            sbc         tx_want + 1
            bcc         @next
            stz         tx_eq
            lda         tx_ln
            cmp         tx_want
            bne         :+
            lda         tx_ln + 1
            cmp         tx_want + 1
            bne         :+
            inc         tx_eq
:
            clc
            jmp         code_sel
@next:
            jsr         tx_skip
            bra         @line
@end:
            stz         tx_eq
            sec
            jmp         code_sel

; tx_end = the text's end (its $FF's place)
tx_endfind:
            lda         tx_at
            pha
            lda         tx_at + 1
            pha
            stz         tx_at
            stz         tx_at + 1
:
            jsr         tx_open
            cmp         #$FF
            beq         :+
            jsr         tx_skip
            bra         :-
:
            MOVW        tx_end, tx_at
            pla
            sta         tx_at + 1
            pla
            sta         tx_at
            jmp         code_sel

; The line at tx_at (.A bytes: 0 none) made .X bytes: the text after it moved (tx_end too).  Past 64K: Out of memory
tx_resize:
            sta         tx_t
            stx         tx_t + 1
            clc                                             ; (From after the old: tx_src, to after the new: tx_dst)
            lda         tx_at
            adc         tx_t
            sta         tx_src
            lda         tx_at + 1
            adc         #0
            sta         tx_src + 1
            clc
            lda         tx_at
            adc         tx_t + 1
            sta         tx_dst
            lda         tx_at + 1
            adc         #0
            sta         tx_dst + 1
            sec                                             ; (Its bytes: to the $FF and it)
            lda         tx_end
            sbc         tx_src
            sta         tx_n
            lda         tx_end + 1
            sbc         tx_src + 1
            sta         tx_n + 1
            inc         tx_n
            bne         :+
            inc         tx_n + 1
:
            clc                                             ; (The new end: below $FFFF)
            lda         tx_end
            adc         tx_t + 1
            tax
            lda         tx_end + 1
            adc         #0
            bcs         @full
            pha
            sec
            txa
            sbc         tx_t
            sta         tx_end
            pla
            sbc         #0
            sta         tx_end + 1
            cmp         #$FF
            bne         :+
            lda         tx_end
            cmp         #$FF
            bne         :+
@full:
            jmp         tx_full
:
            ; (fall through)

; tx_n bytes moved from tx_src to tx_dst (the text's region): from the end if they move up
tx_move:
            lda         tx_n
            ora         tx_n + 1
            bne         @lb900
            jmp         @done
@lb900:
            lda         tx_dst
            cmp         tx_src
            lda         tx_dst + 1
            sbc         tx_src + 1
            bcc         @fwd
@back:
            lda         tx_n
            ora         tx_n + 1
            beq         @done
            ldx         #FBUF                               ; (FBUF at a time, from the end)
            lda         tx_n + 1
            bne         :+
            lda         tx_n
            cmp         #FBUF
            bcs         :+
            tax
:
            stx         cn
            stz         cn + 1
            sec
            lda         tx_n
            sbc         cn
            sta         tx_n
            bcs         :+
            dec         tx_n + 1
:
            clc
            lda         tx_src
            adc         tx_n
            sta         t0
            lda         tx_src + 1
            adc         tx_n + 1
            sta         t0 + 1
            lda         #LB_TEXT
            jsr         ca_reg7
            clc
            lda         tx_dst
            adc         tx_n
            sta         t0
            lda         tx_dst + 1
            adc         tx_n + 1
            sta         t0 + 1
            lda         #LB_TEXT
            jsr         cb_reg7
            jsr         far_copy
            bra         @back
@fwd:
            MOVW        t0, tx_src
            lda         #LB_TEXT
            jsr         ca_reg7
            MOVW        t0, tx_dst
            lda         #LB_TEXT
            jsr         cb_reg7
            MOVW        cn, tx_n
            jsr         far_copy
@done:
            jmp         code_sel
tx_full:
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error

; A line typed with a number first (lbuf): into the program at its number (the line that has it replaced; a number
; alone: that line taken out).  CONT goes on no more
text_line:
            stz         cont_ok
            ldx         #0
:
            lda         lbuf,x
            cmp         #' '
            bne         :+
            inx
            bra         :-
:
            stz         sp_n
            stz         sp_n + 1
@dig:
            lda         lbuf,x
            sec
            sbc         #'0'
            cmp         #10
            bcs         :+
            pha
            jsr         sp_x10
            pla
            clc
            adc         sp_n
            sta         sp_n
            bcc         @dnext
            inc         sp_n + 1
@dnext:
            inx
            bra         @dig
:
            MOVW        tx_want, sp_n
            stz         tx_del                              ; (The rest: spaces alone, the line out)
:
            lda         lbuf,x
            beq         @del
            cmp         #' '
            bne         @keep
            inx
            bra         :-
@del:
            inc         tx_del
@keep:
            ldx         #0                                  ; (Its length: 254 at most)
:
            lda         lbuf,x
            beq         :+
            inx
            bne         :-
:
            cpx         #255
            bcc         :+
            lda         #ER_TOOLONG
            ldx         #0
            jmp         b_error
:
            stx         tx_newl
            jsr         tx_endfind
            jsr         tx_find
            lda         #0                                  ; (Its old size: its own's, else none)
            ldx         tx_eq
            beq         :+
            lda         tx_len
            inc         a
:
            ldx         tx_del                              ; (Its new size: none, to take it out)
            beq         :+
            cmp         #0
            beq         @done                               ; (A line not there taken out: nothing)
            ldx         #0
            jmp         tx_resize
:
            ldx         tx_newl
            inx
            jsr         tx_resize
            MOVW        t0, tx_at                           ; (The line: its length, its bytes)
            lda         #LB_TEXT
            jsr         cb_reg7
            lda         tx_newl
            jsr         cb_wr
            ldx         #0
:
            cpx         tx_newl
            beq         :+
            lda         lbuf,x
            jsr         cb_wr
            inx
            bra         :-
:
@done:
            jmp         code_sel

; DELETE from-to: the lines numbered from from to to taken out (CONT goes on no more)
text_delete:
            lda         ls_fk
            cmp         #1
            bne         @bad
            lda         ls_tk
            cmp         #1
            bne         @bad
            stz         cont_ok
@each:
            MOVW        tx_want, ls_from
            jsr         tx_endfind
            jsr         tx_find
            bcs         @done
            lda         ls_to                               ; (Past to: done)
            cmp         tx_ln
            lda         ls_to + 1
            sbc         tx_ln + 1
            bcc         @done
            lda         tx_len
            inc         a
            ldx         #0
            jsr         tx_resize
            bra         @each
@done:
            rts
@bad:
            lda         #ER_SYNTAX
            ldx         #0
            jmp         b_error

; EDIT [label]: the program in the screen editor (edit +N /ram/basicNN.bas), at the label's line (or a number's), or
; the last error's; back: read again and checked (compiled: its first error said)
text_edit:
            jsr         ls_args
            lda         #1                                  ; (Its line: the last error's, or 1)
            sta         sp_n
            stz         sp_n + 1
            lda         err_line
            ora         err_line + 1
            beq         :+
            MOVW        sp_n, err_line
:
            lda         ls_fk
            beq         @file
            stz         tx_at                               ; (The label's: counted)
            stz         tx_at + 1
            stz         tx_ln + 1
            lda         #1
            sta         sp_n
            stz         sp_n + 1
@line:
            jsr         tx_open
            cmp         #$FF
            beq         @file
            lda         ls_fk
            cmp         #1
            bne         @lab
            jsr         tx_num
            bcs         @next
            lda         tx_ln
            cmp         ls_from
            bne         @next
            lda         tx_ln + 1
            cmp         ls_from + 1
            beq         @file
            bra         @next
@lab:
            jsr         tx_label
            jsr         lab_from
            beq         @file
@next:
            jsr         tx_skip
            inc         sp_n
            bne         @line
            inc         sp_n + 1
            bra         @line
@file:
            jsr         code_sel
            jsr         GETPID                              ; (Its file: /ram/basicNN.bas, NN the task's)
            pha
            ldx         #0
:
            lda         s_edpath,x
            sta         ed_path,x
            beq         :+
            inx
            bra         :-
:
            pla
            pha
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            jsr         hex_dig
            sta         ed_path,x
            inx
            pla
            and         #$0F
            jsr         hex_dig
            sta         ed_path,x
            inx
            ldy         #0
:
            lda         s_bas,y
            sta         ed_path,x
            beq         :+
            inx
            iny
            bra         :-
:
            LDW         sv_ptr, ed_path
            jsr         text_save
            lda         #'+'                                ; (edit's arguments: +N, the file)
            sta         ed_args
            MOVW        tx_t, sp_n
            ldx         #1
            jsr         dec_put
            stz         ed_args,x
            inx
            ldy         #0
:
            lda         ed_path,y
            sta         ed_args,x
            beq         :+
            inx
            iny
            bra         :-
:
            inx
            stz         ed_args,x
            ldx         #9                                  ; (/bin/edit)
:
            lda         s_edit,x
            sta         sh_path,x
            dex
            bpl         :-
            LDR         r0, sh_path
            LDR         r1, ed_args
            lda         #0
            jsr         SPAWN
            bcc         :+
            jmp         sys_err
:
            jsr         rc_wait
            jsr         text_init                           ; (Read again)
            jsr         prog_gone
            LDW         sv_ptr, ed_path
            jsr         text_load
            LDR         r0, ed_path
            jsr         REMOVE
            FARN        1, prog_compile                     ; (Checked: its first error said)
            rts
s_edpath:   .byte       "/ram/basic", 0
s_bas:      .byte       ".bas", 0
s_edit:     .byte       "/bin/edit", 0

; .A (0-15) a hex digit's character
hex_dig:
            cmp         #10
            bcc         :+
            adc         #'A' - '0' - 10 - 1
:
            adc         #'0'
            rts

; tx_t (0-65535) at ed_args + .X in decimal, .X past it
dec_put:
            MOVW        t0, tx_t
            stz         t0 + 2
            stz         t0 + 3
            lda         #0                                  ; (Its digits, least first, on the stack)
            pha
@digit:
            lda         #0
            ldy         #32
:
            asl         t0
            rol         t0 + 1
            rol         t0 + 2
            rol         t0 + 3
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         t0
:
            dey
            bne         :--
            ora         #'0'
            pha
            lda         t0
            ora         t0 + 1
            bne         @digit
:
            pla
            beq         :+
            sta         ed_args,x
            inx
            bra         :-
:
            rts

; ---- An error's line: the line table's last statement at or before the code address t0 (the program's code: the
; prompt's, none).  OUT: err_line; err_ip its start, err_next the next statement's ($FFFF none)
lt_find:
            stz         err_line
            stz         err_line + 1
            lda         #$FF
            sta         err_next
            sta         err_next + 1
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
            bcc         @next
            lda         t4 + 2
            sta         err_ip
            lda         t4 + 3
            sta         err_ip + 1
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
@next:
            lda         t4 + 2
            sta         err_next
            lda         t4 + 3
            sta         err_next + 1
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
