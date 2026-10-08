; ****************************************************************************
; sys.inc - the seventh bank: an error's message and where it was (err_say), END's status, and the system's
; functions and statements (fn.inc's F_B7 and ST_B7 groups).

.bss
err_code:   .res        2                                   ; The last error: its code (ERR) ...
err_line:   .res        2                                   ;   its line (ERL: 0 none) ...
end_status: .res        1                                   ; END n's status (a script's)
pname:      .res        PATH_MAX + 1                        ; The program's file (LOAD's, a script's): "" none
esbuf:      .res        32                                  ; (A system error's text: ERRSTR's)
err_ip:     .res        2                                   ; A trapped error's statement: its start, the next's ...
err_next:   .res        2
err_vsp:    .res        2                                   ;   and the value stack as it began
eh_addr:    .res        2                                   ; ON ERROR's handler ($FFFF: none)
eh_in:      .res        1                                   ; <> 0: in it
in_prog:    .res        1                                   ; <> 0: the program running (not the prompt's line)
err_name:   .res        2                                   ; (A file the error's: its name said first; 0 none)
running:    .res        1                                   ; <> 0: code running (a program's, the prompt's line's):
                                                            ;   an error's line from it
eo_cap:     .res        1                                   ; $80: the messages' output to eo_buf (ERR$'s)
eo_len:     .res        1
eo_buf:     .res        EO_SIZE
ubank:      .res        1                                   ; BANK's: the RAM bank PEEK and POKE see at $8000
sh_status:  .res        2                                   ; STATUS: the last SHELL's
sh_task:    .res        1                                   ; (SHELL's: its task, its pipe's ends, its fds, its output's
sh_rfd:     .res        1                                   ;   length, rc's path)
sh_wfd:     .res        1
sh_map:     .res        4
sh_len:     .res        2
sh_path:    .res        16
tm_anch:    .res        4                                   ; (TIMER's: the clock's second at its anchor, the ticks
tm_prev:    .res        2                                   ;   last seen, the ticks since the anchor, its ticks in
tm_acc:     .res        4                                   ;   the second; an anchor yet)
tm_rem:     .res        1
tm_have:    .res        1
tm_secs:    .res        4                                   ; (The clock's seconds since 2000; a division's divisor,
dv_d:       .res        2                                   ;   remainder; a multiplication's)
dv_r:       .res        2
mu_a:       .res        4
mu_m:       .res        1
dt_year:    .res        2                                   ; (DATE$'s)
dt_mon:     .res        1
dt_days:    .res        2
dir_fd:     .res        1                                   ; (DIR$'s directory: 0 none)
sh_args:    .res        2

.segment "CODE7"

; ERR$'s text: the message (err_code's) in eo_buf, eo_len bytes
err_text_cap:
            lda         #$80
            sta         eo_cap
            stz         eo_len
            jsr         err_text_out
            stz         eo_cap
            rts

; .A out, or into eo_buf (eo_cap).  Keeps .A, .X, .Y
eo_byte:
            bit         eo_cap
            bmi         :+
            jmp         out_byte
:
            phx
            ldx         eo_len
            cpx         #EO_SIZE
            bcs         :+
            sta         eo_buf,x
            inc         eo_len
:
            plx
            rts

; ERROR n: the error n (1 to 255) made
st_error:
            jsr         pop_int16
            lda         t0 + 1
            bne         :+
            lda         t0
            beq         :+
            ldx         #0
            jmp         b_error
:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; The error err_code said on stderr: "file:line: message" (the program's line; a line at the prompt: the message
; alone).  err_line its line (0: none)
err_say:
            jsr         out_flush
            lda         ofd                                 ; (The output's line not ended: ended first)
            cmp         #1
            bne         :+
            lda         ocol
            beq         :+
            lda         #LF
            jsr         out_byte
            stz         ocol
            jsr         out_flush
:
            lda         #2
            sta         ofd
            lda         err_name + 1                        ; (A file's: its name first, as rc's errors)
            beq         :+
            MOVW        t0, err_name
            stz         err_name + 1
            ldx         #0
            jsr         msg_out_ram
            lda         #':'
            jsr         out_byte
            lda         #' '
            jsr         out_byte
            jmp         @msg
:
            lda         err_line
            ora         err_line + 1
            beq         @msg
            jsr         brk_q                               ; (Ctrl-C's, STOP's: "Break in" the place)
            bne         :+
            LDW         t0, s_brkin
            jsr         msg_out
:
            lda         err_line + 1                        ; (An INCLUDE's file's line: its path)
            cmp         #>INC_LINE
            bcc         @prog
            sec
            sbc         #>INC_LINE
            lsr         a
            lsr         a
            lsr         a
            jsr         inc_pathat
            ldx         #0
            jsr         msg_out_ram
            bra         @colon
@prog:
            ldx         #0                                  ; (The program's file, or "line" and its number: a
            lda         pname                               ;   typed program's line's, as LIST shows it)
            bne         :+
            LDW         t0, s_line
            jsr         msg_out
            jsr         erl_num
            sta         t0
            stx         t0 + 1
            bra         @dec
:
            LDW         t0, pname
            jsr         msg_out_ram
@colon:
            lda         #':'
            jsr         out_byte
@num:
            MOVW        t0, err_line
            lda         t0 + 1                              ; (An INCLUDE's file's: its own line)
            cmp         #>INC_LINE
            bcc         @dec
            and         #>(INC_STEP - 1)
            sta         t0 + 1
@dec:
            stz         t0 + 2
            stz         t0 + 3
            jsr         dec_out
            jsr         brk_q
            beq         @end
            lda         #':'
            jsr         out_byte
            lda         #' '
            jsr         out_byte
@msg:
            jsr         err_text_out
@end:
            lda         #LF
            jsr         out_byte
            jsr         out_flush
            lda         #1
            sta         ofd
            rts
s_line:     .byte       "line ", 0
s_brkin:    .byte       "Break in ", 0

; Is the error Ctrl-C's (or STOP's)?  OUT: Z = 1 yes
brk_q:
            lda         err_code + 1
            bne         :+
            lda         err_code
            cmp         #ER_BREAK
:
            rts

; The error's message (err_code) written
err_text_out:
            lda         err_code + 1                        ; (A system error's: its text; $FF's, esbuf's own: PLAY's)
            beq         @basic
            LDR         r0, esbuf
            lda         err_code
            cmp         #$FF
            beq         :+
            jsr         ERRSTR
:
            LDW         t0, esbuf
            jmp         msg_out_ram
@basic:
            LDW         t0, err_msgs
@find:
            lda         (t0)                                ; (Each entry: its code, its text, a 0)
            beq         @none
            cmp         err_code
            beq         @found
:
            jsr         t0_inc                              ; (Past its code and text)
            lda         (t0)
            bne         :-
            jsr         t0_inc
            bra         @find
@found:
            jsr         t0_inc
            lda         err_code                            ; (Syntax error, an expected word after it)
            cmp         #ER_EXPECT
            bne         msg_out
            jsr         msg_out
            MOVW        t0, ex_text
            jsr         msg_out_ram
            LDW         t0, s_expected
            jmp         msg_out
@none:
            LDW         t0, s_error
            jsr         msg_out
            MOVW        t0, err_code
            stz         t0 + 2
            stz         t0 + 3
            jmp         dec_out
s_error:    .byte       "error ", 0
s_expected: .byte       " expected", 0

; t0 on past a byte
t0_inc:
            inc         t0
            bne         :+
            inc         t0 + 1
:
            rts

; The zero-terminated string at (t0), in this bank's ROM (msg_out) or the RAM (msg_out_ram), out
msg_out:
msg_out_ram:
            ldy         #0
:
            lda         (t0),y
            beq         :+
            jsr         eo_byte
            iny
            bne         :-
:
            rts

; The number t0-t3 (32 bits, not below 0) in decimal, out
dec_out:
            ldx         #0                                  ; (Its digits, least first, on the 6502's stack)
@digit:
            lda         #0                                  ; (t0 / 10: a bit at a time, the remainder in .A)
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
            pha
            inx
            lda         t0
            ora         t0 + 1
            ora         t0 + 2
            ora         t0 + 3
            bne         @digit
:
            pla
            ora         #'0'
            jsr         eo_byte
            dex
            bne         :-
            rts

; The errors' messages: each its code, its text
err_msgs:
            .byte       ER_NEXT, "NEXT without FOR", 0
            .byte       ER_SYNTAX, "syntax error", 0
            .byte       ER_RETURN, "RETURN without GOSUB", 0
            .byte       ER_DATA, "out of DATA", 0
            .byte       ER_FUNC, "illegal function call", 0
            .byte       ER_OVERFLOW, "overflow", 0
            .byte       ER_MEMORY, "out of memory", 0
            .byte       ER_LABEL, "label not defined", 0
            .byte       ER_SUBSCRIPT, "subscript out of range", 0
            .byte       ER_DUP, "duplicate definition", 0
            .byte       ER_DIV0, "division by zero", 0
            .byte       ER_DIRECT, "illegal in direct mode", 0
            .byte       ER_TYPE, "type mismatch", 0
            .byte       ER_STRING, "string too long", 0
            .byte       ER_CONT, "cannot continue", 0
            .byte       ER_NOFUNC, "function not defined", 0
            .byte       ER_NORESUME, "no RESUME", 0
            .byte       ER_RESUME, "RESUME without error", 0
            .byte       ER_FORNEXT, "FOR without NEXT", 0
            .byte       ER_WHILE, "WHILE without WEND", 0
            .byte       ER_WEND, "WEND without WHILE", 0
            .byte       ER_DUPLABEL, "duplicate label", 0
            .byte       ER_NOSUB, "subprogram not defined", 0
            .byte       ER_ARGS, "argument-count mismatch", 0
            .byte       ER_VARREQ, "variable required", 0
            .byte       ER_BADFILE, "bad file number", 0
            .byte       ER_NOTFOUND, "file not found", 0
            .byte       ER_MODE, "bad file mode", 0
            .byte       ER_OPEN, "file already open", 0
            .byte       ER_IO, "device I/O error", 0
            .byte       ER_EXISTS, "file already exists", 0
            .byte       ER_FULL, "disk full", 0
            .byte       ER_PASTEND, "input past end of file", 0
            .byte       ER_BADNAME, "bad file name", 0
            .byte       ER_TOOMANY, "too many files", 0
            .byte       ER_DENIED, "permission denied", 0
            .byte       73, "not done yet", 0
            .byte       ER_PATH, "path not found", 0
            .byte       ER_BLOCK, "block without its start or end", 0
            .byte       ER_STACK, "out of stack space", 0
            .byte       ER_BREAK, "break", 0
            .byte       ER_TOOLONG, "line too long", 0
            .byte       ER_PARAM, "parameter type mismatch", 0
            .byte       ER_INSUB, "not in a procedure", 0
            .byte       ER_EXPECT, "syntax error: ", 0
            .byte       ER_CONST, "not a constant", 0
            .byte       ER_FIELD, "no such field", 0
            .byte       ER_PROMPT, "only at the prompt", 0
            .byte       0

; ---- The system's errors

; The system's error .A (E_*) made BASIC's: QuickBASIC's code for a few, else 256 and its own
sys_err:
            ldx         #sys_from_n - 1
:
            cmp         sys_from,x
            beq         :+
            dex
            bpl         :-
            ldx         #1
            jmp         b_error
:
            lda         sys_to,x
            ldx         #0
            jmp         b_error
sys_from:   .byte       E_NOENT, E_EXIST, E_NOSPC, E_PERM, E_ROFS, E_NOTDIR
sys_from_n  = * - sys_from
sys_to:     .byte       ER_NOTFOUND, ER_EXISTS, ER_FULL, ER_DENIED, ER_DENIED, ER_PATH

; ---- Strings to and from the RAM

; The top string (taken off) copied to lbuf, zero-terminated: its first 255 bytes.  OUT: .A its length
str_lbuf:
            jsr         top_vt
            FARN        5, str_get_a
            lda         sla + 1
            beq         :+
            lda         #255
            sta         sla
:
            MOVW        ca, sfa
            lda         sfa + 2
            sta         cal
            ldy         #0
            lda         sla
            beq         @end
            jsr         ca_sel
:
            jsr         ca_rd
            sta         lbuf,y
            iny
            cpy         sla
            bne         :-
@end:
            lda         #0
            sta         lbuf,y
            jsr         code_sel
            jsr         pop_vt
            tya
            rts

; The .A bytes at rs_ptr pushed, a string (ram_str's, the sixth bank's)
str_push:
            FARN        6, ram_str
            rts

; The .A bytes at eo_buf pushed, a string
eo_push:
            pha
            LDW         rs_ptr, eo_buf
            pla
            bra         str_push

; .A/.X (0-65535) pushed, an integer
push_ax7:
            FARN        6, push_ax
            rts

; ---- The clock: TIMER, DATE$, TIME$

; tm_secs = the clock (TIME: seconds since 2000-01-01)
time_now:
            jsr         TIME
            ldx         #3
:
            lda         r0,x
            sta         tm_secs,x
            dex
            bpl         :-
            rts

; tm_secs split: t0-t3 the days since 2000-01-01, t4-t4+2 the second of the day (0-86399)
day_split:
            ldx         #3
:
            lda         tm_secs,x
            sta         t0,x
            dex
            bpl         :-
            lda         #<400                               ; (/ 86400: / 400, then / 216)
            ldx         #>400
            jsr         div32
            lda         dv_r                                ; (r1 kept)
            pha
            lda         dv_r + 1
            pha
            lda         #216
            ldx         #0
            jsr         div32
            MOVW        dt_days, t0
            lda         dv_r                                ; (The second: r2 * 400 + r1)
            sta         t0
            lda         dv_r + 1
            sta         t0 + 1
            stz         t0 + 2
            stz         t0 + 3
            lda         #200
            jsr         mul8
            asl         t0
            rol         t0 + 1
            rol         t0 + 2
            pla
            tax
            pla
            clc
            adc         t0
            sta         t4
            txa
            adc         t0 + 1
            sta         t4 + 1
            lda         t0 + 2
            adc         #0
            sta         t4 + 2
            MOVW        t0, dt_days
            stz         t0 + 2
            stz         t0 + 3
            rts

; t0-t3 / .A/.X: the quotient in t0-t3, the remainder in dv_r (a divisor below 32768)
div32:
            sta         dv_d
            stx         dv_d + 1
            stz         dv_r
            stz         dv_r + 1
            ldy         #32
@bit:
            asl         t0
            rol         t0 + 1
            rol         t0 + 2
            rol         t0 + 3
            rol         dv_r
            rol         dv_r + 1
            sec
            lda         dv_r
            sbc         dv_d
            tax
            lda         dv_r + 1
            sbc         dv_d + 1
            bcc         :+
            stx         dv_r
            sta         dv_r + 1
            inc         t0
:
            dey
            bne         @bit
            rts

; t0-t3 * .A (32 bits)
mul8:
            sta         mu_m
            ldx         #3
:
            lda         t0,x
            sta         mu_a,x
            stz         t0,x
            dex
            bpl         :-
            ldy         #8
@bit:
            asl         t0
            rol         t0 + 1
            rol         t0 + 2
            rol         t0 + 3
            asl         mu_m
            bcc         :+
            clc
            lda         t0
            adc         mu_a
            sta         t0
            lda         t0 + 1
            adc         mu_a + 1
            sta         t0 + 1
            lda         t0 + 2
            adc         mu_a + 2
            sta         t0 + 2
            lda         t0 + 3
            adc         mu_a + 3
            sta         t0 + 3
:
            dey
            bne         @bit
            rts

; TIMER: the seconds since midnight, to a tick (1/200): a fixed decimal of 3 places.  The ticks into the second count
; from when TIMER first saw it
f_timer:
            jsr         time_now                            ; (The clock's second, the ticks)
            jsr         TICKS
            sta         t4
            stx         t4 + 1
            lda         tm_have
            bne         :+
            jmp         @anchor
:
            sec                                             ; (The ticks since the last: on the count)
            lda         t4
            sbc         tm_prev
            sta         t0
            lda         t4 + 1
            sbc         tm_prev + 1
            sta         t0 + 1
            clc
            lda         tm_acc
            adc         t0
            sta         tm_acc
            lda         tm_acc + 1
            adc         t0 + 1
            sta         tm_acc + 1
            bcc         :+
            inc         tm_acc + 2
            bne         :+
            inc         tm_acc + 3
:
            MOVW        tm_prev, t4
            ldx         #3                                  ; (Its seconds since the anchor, its ticks in the second)
:
            lda         tm_acc,x
            sta         t0,x
            dex
            bpl         :-
            lda         #<TICK_HZ
            ldx         #>TICK_HZ
            jsr         div32
            lda         dv_r
            sta         tm_rem
            clc                                             ; (The second: the anchor's on)
            ldx         #0
            ldy         #4
:
            lda         t0,x
            adc         tm_anch,x
            sta         t0,x
            inx
            dey
            bne         :-
            sec                                             ; (The clock's less it: 0 or 1, else anchored again)
            ldx         #0
            ldy         #4
:
            lda         tm_secs,x
            sbc         t0,x
            sta         t4,x
            inx
            dey
            bne         :-
            lda         t4 + 1
            ora         t4 + 2
            ora         t4 + 3
            bne         @anchor
            lda         t4
            cmp         #2
            bcs         @anchor
            ldx         #3
:
            lda         t0,x
            sta         tm_secs,x
            dex
            bpl         :-
            bra         @have
@anchor:
            ldx         #3
:
            lda         tm_secs,x
            sta         tm_anch,x
            stz         tm_acc,x
            dex
            bpl         :-
            jsr         TICKS
            sta         tm_prev
            stx         tm_prev + 1
            stz         tm_rem
            lda         #1
            sta         tm_have
@have:
            lda         tm_rem
            pha
            jsr         day_split                           ; (The second of the day: t4-t4+2)
            lda         t4
            sta         t0
            lda         t4 + 1
            sta         t0 + 1
            lda         t4 + 2
            sta         t0 + 2
            stz         t0 + 3
            lda         #125                                ; (* 1000: * 125 * 8)
            jsr         mul8
            ldx         #3
:
            asl         t0
            rol         t0 + 1
            rol         t0 + 2
            rol         t0 + 3
            dex
            bne         :-
            pla                                             ; (The ticks * 5)
            sta         t4
            stz         t4 + 1
            asl         a
            rol         t4 + 1
            asl         a
            rol         t4 + 1
            clc
            adc         t4
            sta         t4
            lda         t4 + 1
            adc         #0
            sta         t4 + 1
            clc
            lda         t0
            adc         t4
            sta         t0
            lda         t0 + 1
            adc         t4 + 1
            sta         t0 + 1
            bcc         :+
            inc         t0 + 2
            bne         :+
            inc         t0 + 3
:
            FARN        4, num_ms
            rts

; TIME$: "hh:mm:ss"
f_time:
            jsr         time_now
            jsr         day_split
            lda         t4
            sta         t0
            lda         t4 + 1
            sta         t0 + 1
            lda         t4 + 2
            sta         t0 + 2
            stz         t0 + 3
            lda         #60
            ldx         #0
            jsr         div32
            lda         dv_r                                ; (The seconds)
            pha
            lda         #60
            ldx         #0
            jsr         div32
            ldx         #0
            lda         t0                                  ; (The hours)
            jsr         dig2
            lda         #':'
            jsr         eo_put
            lda         dv_r
            jsr         dig2
            lda         #':'
            jsr         eo_put
            pla
            jsr         dig2
            lda         #8
            jmp         eo_push

; DATE$: "yyyy-mm-dd"
f_date:
            jsr         time_now
            jsr         day_split                           ; (t0/t0+1: the days since 2000-01-01)
            LDW         dt_year, 2000
@year:
            lda         dt_year                             ; (A year's days: 366 in a leap year, 2000-2099)
            and         #3
            beq         :+
            LDW         t4, 365
            bra         :++
:
            LDW         t4, 366
:
            lda         t0
            cmp         t4
            lda         t0 + 1
            sbc         t4 + 1
            bcc         @month
            sec
            lda         t0
            sbc         t4
            sta         t0
            lda         t0 + 1
            sbc         t4 + 1
            sta         t0 + 1
            inc         dt_year
            bne         @year
            inc         dt_year + 1
            bra         @year
@month:
            stz         dt_mon
@mdays:
            ldx         dt_mon
            lda         mon_days,x
            cpx         #1                                  ; (February in a leap year: 29)
            bne         :+
            ldy         t4
            cpy         #<366
            bne         :+
            inc         a
:
            sta         t4 + 2
            lda         t0 + 1
            bne         :+
            lda         t0
            cmp         t4 + 2
            bcc         @have
:
            sec
            lda         t0
            sbc         t4 + 2
            sta         t0
            lda         t0 + 1
            sbc         #0
            sta         t0 + 1
            inc         dt_mon
            bra         @mdays
@have:
            lda         t0                                  ; (Its day)
            inc         a
            pha
            ldx         #0
            lda         dt_year                             ; (The year: 2000 + its two digits)
            sec
            sbc         #<2000
            pha
            lda         #20
            jsr         dig2
            pla
            jsr         dig2
            lda         #'-'
            jsr         eo_put
            lda         dt_mon
            inc         a
            jsr         dig2
            lda         #'-'
            jsr         eo_put
            pla
            jsr         dig2
            lda         #10
            jmp         eo_push
mon_days:   .byte       31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31

; .A (0-99) at eo_buf + .X as two digits, .X past them
dig2:
            ldy         #'0'
:
            cmp         #10
            bcc         :+
            sbc         #10
            iny
            bra         :-
:
            pha
            tya
            jsr         eo_put
            pla
            ora         #'0'
            ; (fall through)
; .A at eo_buf + .X, .X on
eo_put:
            sta         eo_buf,x
            inx
            rts

; ---- DIR$

; DIR$(path$): the directory's first name (a directory's with /), "" if none; DIR$: the next, "" after the last
f_dir:
            jsr         pop_int16                           ; (Its arguments' count)
            lda         t0
            beq         @next
            lda         dir_fd                              ; (A directory before: closed)
            beq         :+
            dec         a
            jsr         CLOSE
            stz         dir_fd
:
            jsr         str_lbuf
            cmp         #0
            bne         :+
            lda         #'.'
            sta         lbuf
            stz         lbuf + 1
:
            LDR         r0, lbuf
            lda         #O_READ
            jsr         OPEN
            bcc         :+
            jmp         sys_err
:
            inc         a                                   ; (Kept: fd + 1)
            sta         dir_fd
@next:
            lda         dir_fd
            beq         @none
            LDR         r0, sr_buf
            LDR         r1, SR_SIZE
            lda         dir_fd
            dec         a
            jsr         READ
            bcs         @last
            cmp         #0
            beq         @last
            ldx         #0                                  ; (Its name, to eo_buf)
:
            lda         sr_buf + SR_NAME,x
            beq         :+
            sta         eo_buf,x
            inx
            cpx         #32
            bcc         :-
:
            lda         sr_buf + SR_MODE + 1
            and         #DM_DIR
            beq         :+
            lda         #'/'
            sta         eo_buf,x
            inx
:
            txa
            jmp         eo_push
@last:
            lda         dir_fd
            dec         a
            jsr         CLOSE
            stz         dir_fd
@none:
            lda         #0
            jmp         str_push

; ---- The environment, the arguments

; ENV$(name$), ENVIRON$(name$): rc's variable (its words, spaces between), "" if there's none
f_env:
            jsr         str_lbuf
            LDR         r0, lbuf
            LDR         r1, lbuf + 256
            LDR         r2, 255
            LDR         r3, 0
            lda         #$FF
            jsr         ENV_GET
            bcc         :+
            lda         #0
            bra         @push
:
            cpx         #0
            beq         :+
            lda         #255
:
            tay
            beq         @push
            lda         lbuf + 255,y                        ; (Its last 0 dropped)
            bne         :+
            dey
:
            tya
            pha
            beq         @pop
:
            dey                                             ; (The 0s between its words: spaces)
            lda         lbuf + 256,y
            bne         :+
            lda         #' '
            sta         lbuf + 256,y
:
            cpy         #0
            bne         :--
@pop:
            pla
@push:
            pha
            LDW         rs_ptr, lbuf + 256
            pla
            jmp         str_push

; ENVIRON "name=value": rc's variable set (no value: removed)
st_environ:
            jsr         str_lbuf
            sta         t4
            ldy         #0
:
            lda         lbuf,y
            beq         @bad
            cmp         #'='
            beq         :+
            iny
            bra         :-
:
            lda         #0                                  ; (Its name: zero-terminated)
            sta         lbuf,y
            iny
            sty         t4 + 1
            clc
            tya
            adc         #<lbuf
            sta         r1
            lda         #>lbuf
            adc         #0
            sta         r1 + 1
            sec
            lda         t4
            sbc         t4 + 1
            sta         r2
            stz         r2 + 1
            LDR         r0, lbuf
            lda         r2
            beq         @del
            LDR         r3, 0
            lda         #$FF
            jsr         ENV_PUT
            bcc         :+
            jmp         sys_err
:
            rts
@del:
            lda         #$FF
            jmp         ENV_DEL
@bad:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; ARG$(n): a script's argument n (0: its name); "" past the last
f_arg:
            jsr         pop_int16
            lda         t0 + 1
            bne         @none
            jsr         arg_first
            bcs         @none
            ldx         t0
@next:
            lda         (t4)
            beq         @none
            txa
            beq         @have
            jsr         arg_past
            dex
            bra         @next
@have:
            ldy         #0
:
            lda         (t4),y
            beq         :+
            iny
            bne         :-
:
            MOVW        rs_ptr, t4
            tya
            jmp         str_push
@none:
            lda         #0
            jmp         str_push

; t4 = the first argument (a script's name).  OUT: C = 1 none
arg_first:
            lda         argp
            ora         argp + 1
            bne         :+
            sec
            rts
:
            MOVW        t4, argp
            clc
            rts

; t4 past its argument (and its 0)
arg_past:
:
            lda         (t4)
            inc         t4
            bne         :+
            inc         t4 + 1
:
            cmp         #0
            bne         :--
            rts

; COMMAND$: a script's arguments after its name, spaces between (lbuf's 255 bytes at most)
f_command:
            ldx         #0
            jsr         arg_first
            bcs         @push
            lda         (t4)
            beq         @push
            jsr         arg_past
@arg:
            lda         (t4)
            beq         @push
            cpx         #0
            beq         :+
            lda         #' '
            jsr         @put
:
            lda         (t4)
            beq         :+
            jsr         @put
            inc         t4
            bne         :-
            inc         t4 + 1
            bra         :-
:
            jsr         arg_past
            bra         @arg
@put:
            cpx         #255
            bcs         :+
            sta         lbuf,x
            inx
:
            rts
@push:
            LDW         rs_ptr, lbuf
            txa
            jmp         str_push

; ---- SHELL, SHELL$, STATUS

; ST_SHELL: the line (the top string; "": rc alone) run by rc and waited for; STATUS its status
st_shell:
            jsr         sh_begin
            lda         #0
            jsr         rc_spawn
            bcc         :+
            jmp         sys_err
:
            jmp         rc_wait

; SHELL$(line$): rc's output of the line, a string (8K at most)
f_shells:
            jsr         sh_begin
            jsr         PIPE
            bcc         :+
            jmp         sys_err
:
            sta         sh_rfd
            stx         sh_wfd
            lda         #3                                  ; (Its fds: 0, the pipe's, 2)
            sta         sh_map
            stz         sh_map + 1
            stx         sh_map + 2
            lda         #2
            sta         sh_map + 3
            lda         #SPAWN_FDMAP
            jsr         rc_spawn
            bcc         :+
            pha
            lda         sh_rfd
            jsr         CLOSE
            lda         sh_wfd
            jsr         CLOSE
            pla
            jmp         sys_err
:
            sta         sh_task
            lda         sh_wfd
            jsr         CLOSE
            stz         sh_len                              ; (Its output: to the text bank, 8K at most)
            stz         sh_len + 1
            LDW         cb, $8000
            lda         #LB_TXB
            sta         cbl
@read:
            LDR         r0, lbuf
            LDR         r1, 128
            lda         sh_rfd
            jsr         READ
            bcs         @end
            cmp         #0
            beq         @end
            sta         t4
            jsr         cb_sel
            ldy         #0
:
            lda         sh_len + 1                          ; (Room: below 8191)
            cmp         #$1F
            bcc         :+
            lda         sh_len
            cmp         #$FF
            bcs         @full
:
            lda         lbuf,y
            jsr         cb_wr
            inc         sh_len
            bne         :+
            inc         sh_len + 1
:
@full:
            iny
            cpy         t4
            bne         :---
            jsr         code_sel
            bra         @read
@end:
            jsr         code_sel
            lda         sh_rfd
            jsr         CLOSE
            lda         sh_task
            jsr         rc_wait
@trim:
            lda         sh_len                              ; (Its newlines at its end dropped)
            ora         sh_len + 1
            beq         @trimmed
            sec
            lda         sh_len
            sbc         #1
            sta         ca
            lda         sh_len + 1
            sbc         #0
            ora         #$80
            sta         ca + 1
            lda         #LB_TXB
            sta         cal
            jsr         ca_sel
            lda         (ca)
            jsr         code_sel
            cmp         #LF
            beq         :+
            cmp         #CR
            bne         @trimmed
:
            lda         sh_len
            bne         :+
            dec         sh_len + 1
:
            dec         sh_len
            bra         @trim
@trimmed:
            lda         sh_len                              ; (Its string: from the text bank)
            ora         sh_len + 1
            bne         :+
            lda         #0
            jmp         str_push
:
            lda         sh_len
            ldx         sh_len + 1
            FARN        5, heap_str
            LDW         ca, $8000
            lda         #LB_TXB
            sta         cal
            MOVW        cn, sh_len
            jsr         far_copy
            lda         #VT_STR
            sta         (vsp)
            ldy         #1
            lda         hres
            sta         (vsp),y
            iny
            lda         hres + 1
            sta         (vsp),y
            iny
            lda         hres + 2
            sta         (vsp),y
            jmp         vsp_up

; SHELL's and SHELL$'s start: the line (the top string) as rc's arguments (-c and it; "": none), in lbuf + 256; the
; output written, the console's lines cooked
sh_begin:
            jsr         str_lbuf
            pha
            jsr         out_flush
            FARN        6, con_cooked
            pla
            bne         :+
            stz         sh_args                             ; (None: rc alone)
            stz         sh_args + 1
            rts
:
            cmp         #170
            bcc         :+
            lda         #E_TOOBIG
            jmp         sys_err
:
            ldx         #0
            lda         #'-'
            sta         lbuf + 256
            lda         #'c'
            sta         lbuf + 257
            stz         lbuf + 258
            ldy         #0
:
            lda         lbuf,y
            sta         lbuf + 259,y
            beq         :+
            iny
            bra         :-
:
            lda         #0
            sta         lbuf + 260,y
            LDW         sh_args, lbuf + 256
            rts

; rc SPAWNed (/bin/rc, else the ROM's, #m/rc): sh_args its arguments, sh_map its fds' map, .A the flags.  OUT: C = 0, .A the
; task; C = 1, .A the error
rc_spawn:
            pha
            ldx         #7
:
            lda         s_binrc,x
            sta         sh_path,x
            dex
            bpl         :-
            jsr         rc_regs
            pla
            pha
            jsr         SPAWN
            bcc         @done
            cmp         #E_NOENT
            bne         @done
            ldx         #5
:
            lda         s_mrc,x
            sta         sh_path,x
            dex
            bpl         :-
            jsr         rc_regs
            pla
            pha
            jsr         SPAWN
@done:
            plx
            rts
rc_regs:
            LDR         r0, sh_path
            MOVW        r1, sh_args
            LDR         r2, sh_map
            rts
s_binrc:    .byte       "/bin/rc", 0
s_mrc:      .byte       "#m/rc", 0

; Task .A waited for (a note: waited for again): STATUS its exit code, or rc's $status (its message, a number)
rc_wait:
            sta         sh_task
@wait:
            LDR         r0, esbuf
            lda         sh_task
            jsr         WAIT
            bcc         @ended
            cmp         #E_INTR
            beq         @wait
            rts
@ended:
            stx         sh_status
            stz         sh_status + 1
            cpx         #1
            bne         @done
            lda         esbuf                               ; (A number: its value)
            sec
            sbc         #'0'
            cmp         #10
            bcs         @done
            stz         sh_status
            ldy         #0
@digit:
            lda         esbuf,y
            sec
            sbc         #'0'
            cmp         #10
            bcs         @done
            pha
            lda         sh_status                           ; (* 10)
            ldx         sh_status + 1
            asl         sh_status
            rol         sh_status + 1
            asl         sh_status
            rol         sh_status + 1
            clc
            adc         sh_status
            sta         sh_status
            txa
            adc         sh_status + 1
            sta         sh_status + 1
            asl         sh_status
            rol         sh_status + 1
            pla
            clc
            adc         sh_status
            sta         sh_status
            bcc         :+
            inc         sh_status + 1
:
            iny
            bra         @digit
@done:
            rts

; STATUS: the last SHELL's status
f_status:
            lda         sh_status
            ldx         sh_status + 1
            jmp         push_ax7

; POINT(x, y): a pixel's colour (gfx.inc's)
f_point:
            FARN        8, g_point_f
            rts

; ---- PEEK, POKE, BANK

; PEEK(addr): a byte of the task's memory (at $8000-$9FFF, BANK's bank's)
f_peek:
            jsr         pop_int16
            jsr         peek_bank
            lda         (t0)
            jsr         code_sel
            ldx         #0
            jmp         push_ax7

; POKE addr, b
st_poke:
            jsr         pop_int16
            lda         t0 + 1
            bne         @bad
            lda         t0
            pha
            jsr         pop_int16
            jsr         peek_bank
            pla
            sta         (t0)
            jmp         code_sel
@bad:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; BANK's bank at $8000 if the address t0 is there
peek_bank:
            lda         t0 + 1
            cmp         #$80
            bcc         :+
            cmp         #$A0
            bcs         :+
            lda         ubank
            sta         RAM_BANK
:
            rts

; BANK n: the RAM bank PEEK and POKE see at $8000; BANK(): it
st_bank:
            jsr         pop_int16
            lda         t0 + 1
            bne         :+
            lda         t0
            sta         ubank
            rts
:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error
f_bank:
            lda         ubank
            ldx         #0
            jmp         push_ax7

; ---- SYSTEM [n] (BYE): BASIC's end, from a program or the prompt (its status n)
st_system:
            jsr         pop_int16
            lda         t0
            sta         end_status
            FARN        6, fl_closeall
            jsr         out_flush
            FARN        6, con_cooked
            LDR         r0, 0
            lda         end_status
            jmp         EXITS

; ---- END n: the program's end, its status n
st_endn:
            jsr         pop_int16
            lda         t0
            sta         end_status
            jmp         run_exit

; ---- The seventh bank's functions and statements (fn.inc)
fn_disp7:
            tax
            jmp         (fn_jt7,x)
fn_jt7:
            .word       f_command, f_date, f_dir, f_env, f_peek, f_point, f_shells, f_status, f_time, f_timer
            .word       f_arg, f_bank
.assert     * - fn_jt7 = (F_END - F_B7) * 2, error, "fn_jt7 and fn.inc's F_B7 group differ"

st_disp7:
            tax
            jmp         (st_jt7,x)
st_jt7:
            .word       st_endn, st_error, st_shell, st_environ, st_poke, st_bank, st_system
.assert     * - st_jt7 = (ST_B8 - ST_B7) * 2, error, "st_jt7 and fn.inc's ST_B7 group differ"

rt_todo7:
            lda         #73
            ldx         #0
            jmp         b_error
