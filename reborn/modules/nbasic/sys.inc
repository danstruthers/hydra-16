; ****************************************************************************
; sys.inc - the seventh bank: an error's message and where it was (err_say), END's status, and the system's
; functions and statements (fn.inc's F_B7 and ST_B7 groups).

.bss
err_code:   .res        2                                   ; The last error: its code (ERR) ...
err_line:   .res        2                                   ;   its line (ERL: 0 none) ...
end_status: .res        1                                   ; END n's status (a script's)
pname:      .res        PATH_MAX + 1                        ; The program's file (LOAD's, a script's): "" none
esbuf:      .res        32                                  ; (A system error's text: ERRSTR's)

.segment "CODE7"

; The error err_code said on stderr: "file:line: message" (the program's line; a line at the prompt: the message
; alone).  err_line its line (0: none)
err_say:
            jsr         out_flush
            lda         #2
            sta         ofd
            lda         err_line
            ora         err_line + 1
            beq         @msg
            ldx         #0                                  ; (The program's file, or "line")
            lda         pname
            bne         :+
            LDW         t0, s_line
            jsr         msg_out
            bra         @num
:
            LDW         t0, pname
            jsr         msg_out_ram
            lda         #':'
            jsr         out_byte
@num:
            MOVW        t0, err_line
            stz         t0 + 2
            stz         t0 + 3
            jsr         dec_out
            lda         #':'
            jsr         out_byte
            lda         #' '
            jsr         out_byte
@msg:
            jsr         err_text_out
            lda         #LF
            jsr         out_byte
            jsr         out_flush
            lda         #1
            sta         ofd
            rts
s_line:     .byte       "line ", 0

; The error's message (err_code) written
err_text_out:
            lda         err_code + 1                        ; (A system error's: its text)
            beq         @basic
            lda         err_code
            LDR         r0, esbuf
            jsr         ERRSTR
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
            jsr         out_byte
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
            jsr         out_byte
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
            .byte       0

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
            .repeat     F_END - F_B7
            .word       rt_todo7
            .endrepeat

st_disp7:
            tax
            jmp         (st_jt7,x)
st_jt7:
            .word       st_endn

rt_todo7:
            lda         #73
            ldx         #0
            jmp         b_error
