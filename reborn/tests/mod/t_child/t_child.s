; ****************************************************************************
; t_child - a program the tests start, to do one thing, named by its arguments: a letter, then a hex byte (hh):
;   "e" c       end with code c (a character) and its arguments as the message
;   "s" hh      spin (no yields) for hh ticks, then end with code 0
;   "y" hh      yield hh times, then end with code 0
;   "k"         end with code = the tick count's low byte when it started
;   "p"         pause until woken (WAKE), then end with code "p"
;   "o"         start "t_child e9" and end at once with code "o" (leaving an orphan)
;   "c" hh      call task F's serve entry (the test driver, t_drv): op 3, sleep hh ticks; end with its .A
;   "g" hh      attach to shared segment hh, and end with the byte at its first bank's $8000 ($EE: no segment)
;   "n"         a note handler that keeps the note and goes on; PAUSE; end with the note it kept
;   "d"         a note handler that asks for the default (C = 1); then pause, for ever
;   "t"         note init (task 1) with note 20; then pause, for ever
;   "b"         a BRK
;   "r"         open #T/wait (t_srv) and read 3 bytes: end with the first ($E0 + the error, if one)
;   "w"         write "W" to fd 1: end with code 0 ($E0 + the error, if one)
;   "i"         read a byte from fd 0: end with it ($EF: the end of the input; $E0 + the error, if one)
;   "h"         open /hello (its namespace's): end with code 0 ($E0 + the error, if one)
;   "m"         bind #T/sub at / (in place), then open /inner: end with code 0 ($E0 + the error, if one)
;   "j"         claim console window 1's notes ("group" to #c1/consctl), then as "i"
;   anything else: end with code $EE

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "t_child", main

.zeropage
args:       .res        2
param:      .res        1
start:      .res        2
kept:       .res        1                                   ; ("n": the note its handler kept)
buf:        .res        4                                   ; ("r": what it read)

.code
main:
            MOVR        args, r0
            ldy         #1                                  ; The hex byte after the letter, if any
            jsr         hex
            sta         param
            lda         (args)                              ; The letter: its routine
            ldx         #OPS - 1
:
            cmp         ops,X
            beq         :+
            dex
            bpl         :-
            lda         #$EE
            bra         end
:
            txa
            asl
            tax
            jmp         (op_vec,X)

; Each ends with .A = its code (end), or ends itself
op_e:
            MOVR        r0, args
            ldy         #1
            lda         (args),Y
            jmp         EXITS

op_s:
            jsr         TICKS
            sta         start
:
            jsr         TICKS
            sec
            sbc         start
            cmp         param
            bcc         :-
            lda         #0
            bra         end

op_y:
            lda         param
            beq         end
            jsr         YIELD
            dec         param
            bra         op_y

op_k:
            jsr         TICKS
            bra         end

op_p:
            jsr         PAUSE
            lda         #'p'
            bra         end

op_o:
            LDR         r0, s_child
            LDR         r1, s_e9
            lda         #0
            jsr         SPAWN
            lda         #'o'
            bra         end

op_c:
            lda         #3
            ldx         param
            ldy         #$0F
            jsr         DBG_SCALL
end:
            stz         r0
            stz         r0 + 1
            jmp         EXITS

op_g:
            lda         param
            jsr         SEG_ATTACH
            bcs         :+
            lda         param
            ldx         #0
            jsr         SEG_MAP
            bcs         :+
            sta         U_REGISTER
            stx         RAM_BANK
            lda         BANK_WINDOW
            bra         end
:
            lda         #$EE
            bra         end

op_n:
            stz         kept
            LDR         r0, keep
            jsr         NOTIFY
            jsr         PAUSE                               ; (A note: the handler, then back here)
            lda         kept
            bra         end

op_d:
            LDR         r0, refuse
            jsr         NOTIFY
            bra         forever

op_t:
            lda         #1
            ldx         #20
            jsr         NOTE
forever:
            jsr         PAUSE
            bra         forever

op_b:
            brk                                             ; (sys: brk: the end, 133)
            .byte       0
            lda         #$EE
            bra         end

op_r:
            LDR         r0, s_wait
            lda         #O_READ
            jsr         OPEN
            bcs         @err
            sta         param                               ; (The fd)
            LDR         r0, buf
            LDR         r1, 3
            lda         param
            jsr         READ
            bcs         @err
            lda         buf
            jmp         end

@err:
            ora         #$E0
            jmp         end

op_j:
            LDR         r0, s_c1ctl
            lda         #O_WRITE
            jsr         OPEN
            bcs         @err
            sta         param
            LDR         r0, s_group
            LDR         r1, 5
            lda         param
            jsr         WRITE
            bcs         @err
            lda         param
            jsr         CLOSE
            bra         op_i

@err:
            ora         #$E0
            jmp         end

op_i:
            LDR         r0, buf
            LDR         r1, 1
            lda         #0
            jsr         READ
            bcs         @err
            cmp         #0
            beq         @eof
            lda         buf
            jmp         end

@eof:
            lda         #$EF
            jmp         end

@err:
            ora         #$E0
            jmp         end

op_h:
            LDR         r0, s_hello
open:
            lda         #O_READ
            jsr         OPEN
            bcs         :+
            lda         #0
            jmp         end
:
            ora         #$E0
            jmp         end

op_m:
            LDR         r0, s_tsub
            LDR         r1, s_root
            lda         #MREPL
            jsr         BIND
            bcs         :-
            LDR         r0, s_inner
            bra         open

op_w:
            LDR         r0, s_w
            LDR         r1, 1
            lda         #1
            jsr         WRITE
            bcs         :+
            lda         #0
            jmp         end
:
            ora         #$E0
            jmp         end

; Note handlers: keep the note and go on, or ask for the default
keep:
            sta         kept
            clc
            rts

refuse:
            sec
            rts

; .A = the hex byte at (args),Y (two digits), or 0
hex:
            jsr         @digit
            bcs         @none
            asl
            asl
            asl
            asl
            sta         param
            iny
            jsr         @digit
            bcs         @none
            ora         param
            rts

@none:
            lda         #0
            rts

@digit:                                                     ; C = 0 and .A = a digit's value, or C = 1
            lda         (args),Y
            sec
            sbc         #'0'
            cmp         #10
            bcc         @ok
            sbc         #'a' - '0' - 10                     ; (Lower case, C = 1)
            cmp         #10
            bcc         @bad
            cmp         #16
            bcs         @bad
@ok:
            clc
            rts

@bad:
            sec
            rts

.rodata
ops:        .byte       "esykpocgndtbrwihmj"
OPS         = * - ops
op_vec:     .word       op_e, op_s, op_y, op_k, op_p, op_o, op_c, op_g, op_n, op_d, op_t, op_b, op_r, op_w, op_i
            .word       op_h, op_m, op_j
s_c1ctl:    .byte       "#c1/consctl", 0
s_group:    .byte       "group"
s_hello:    .byte       "/hello", 0
s_inner:    .byte       "/inner", 0
s_tsub:     .byte       "#T/sub", 0
s_root:     .byte       "/", 0
s_child:    .byte       "#m/t_child", 0
s_e9:       .byte       "e9", 0
s_wait:     .byte       "#T/wait", 0
s_w:        .byte       "W"
