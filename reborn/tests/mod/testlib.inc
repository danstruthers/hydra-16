; ****************************************************************************
; testlib.inc - for the test modules: checks reported the way sim/test.js reads them, one line each:
;   "ok - what"  or  "not ok - what ($xx)"     and at the end  "NAME: PASS"  or  "NAME: FAIL"
; Marks for the time budgets: MARK "<name" and MARK "name>" around the work; sim/test.js takes the cycles between.
;
;   OK "what"                   a check passed
;   NOTOK "what"                a check failed (.A: shown as a detail)
;   EXPECT_OK "what"            C = 0: ok; C = 1: not ok, with the error code in .A
;   EXPECT_ERR code, "what"     C = 1 and .A = code: ok; else not ok
;   EXPECT_A value, "what"      .A = value: ok; else not ok, with .A
;   MARK "text"                 the text, for the harness's clock (no new line)
;   SAY "text"                  a line of text
;   DONE "name"                 the result line, then EXITS with the count of failures
; Each string follows its jsr (no labels: the test's cheap local labels keep their scope).  They use .A, .X,
; .Y and r0; the test's zero page from $70 is the library's.

T_FAILS         = $70
T_SAVE          = $71
T_R0            = $72           ; (2) r0, kept across a print

.macro OK       text
            jsr         t_ok
            .byte       text, 0
.endmacro

.macro NOTOK    text
            jsr         t_notok
            .byte       text, 0
.endmacro

.macro EXPECT_OK text
            jsr         t_expect_ok
            .byte       text, 0
.endmacro

.macro EXPECT_ERR code, text
            ldx         #code
            jsr         t_expect_err
            .byte       text, 0
.endmacro

.macro EXPECT_A value, text
            ldx         #value
            jsr         t_expect_a
            .byte       text, 0
.endmacro

.macro MARK     text
            jsr         t_mark
            .byte       text, 0
.endmacro

.macro SAY      text
            jsr         t_say
            .byte       text, 0
.endmacro

.macro DONE     name
            jsr         t_done
            .byte       name, 0
.endmacro

.pushseg
.code

; r0 = the string after the test's jsr (two returns up), and that return moved past it.  Keeps .A (and in
; T_SAVE), .X, .Y and the flags
t_grab:
            php
            sta         T_SAVE
            phx
            phy
            tsx                                             ; S+1 Y, S+2 X, S+3 P, S+4/5 to t_*, S+6/7 to the test
            lda         $0106,X
            clc
            adc         #1
            sta         r0
            lda         $0107,X
            adc         #0
            sta         r0 + 1
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            iny
            bne         :-
:
            tya                                             ; (Its 0: the byte before where the test goes on)
            clc
            adc         r0
            sta         $0106,X
            lda         r0 + 1
            adc         #0
            sta         $0107,X
            ply
            plx
            lda         T_SAVE
            plp
            rts

t_ok:
            jsr         t_grab
t_ok_r0:
            jsr         t_r0save
            PRINT       s_t_ok
            jsr         t_r0back
            jsr         PUTS
            jmp         t_crlf

t_notok:                                                    ; (.A: the detail)
            jsr         t_grab
t_notok_r0:
            sta         T_SAVE
            inc         T_FAILS
            jsr         t_r0save
            PRINT       s_t_notok
            jsr         t_r0back
            jsr         PUTS
            PRINT       s_t_detail
            lda         T_SAVE
            jsr         PUTHEX
            lda         #')'
            jsr         PUTC
            jmp         t_crlf

t_expect_ok:
            jsr         t_grab
            bcs         t_notok_r0
            bra         t_ok_r0

t_expect_err:
            jsr         t_grab
            bcc         t_noerr
            stx         T_SAVE
            cmp         T_SAVE
            beq         t_ok_r0
            bra         t_notok_r0

t_noerr:
            lda         #0
            bra         t_notok_r0

t_expect_a:
            jsr         t_grab
            stx         T_SAVE
            cmp         T_SAVE
            beq         t_ok_r0
            bra         t_notok_r0

t_mark:
            jsr         t_grab
            jmp         PUTS

t_say:
            jsr         t_grab
            jsr         PUTS
t_crlf:
            lda         #CR
            jsr         PUTC
            lda         #LF
            jmp         PUTC

t_done:                                                     ; (Doesn't return)
            jsr         t_grab
            jsr         PUTS
            lda         T_FAILS
            bne         t_failed
            PRINT       s_t_pass
            lda         #0
            bra         t_exit

t_failed:
            PRINT       s_t_fail
            lda         T_FAILS
t_exit:
            stz         r0
            stz         r0 + 1
            jmp         EXITS

t_r0save:                                                   ; (r0 kept in T_R0 across a print)
            lda         r0
            sta         T_R0
            lda         r0 + 1
            sta         T_R0 + 1
            rts

t_r0back:
            lda         T_R0
            sta         r0
            lda         T_R0 + 1
            sta         r0 + 1
            rts

.rodata
s_t_ok:     .byte       "ok - ", 0
s_t_notok:  .byte       "not ok - ", 0
s_t_detail: .byte       " ($", 0
s_t_pass:   .byte       ": PASS", CR, LF, 0
s_t_fail:   .byte       ": FAIL", CR, LF, 0

.popseg
