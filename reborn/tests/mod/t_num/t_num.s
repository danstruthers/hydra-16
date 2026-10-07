; ****************************************************************************
; t_num - the numbers library (modules/numbers), run as init: its calls as a card's file has them (#f/0/num.in, made
; by tests.js with sim/tools/numref.js), what each gave back written to another (#f/0/num.out), which the test's
; check compares with numref.js's answers.  Each call is made as a program makes it (XCALL, the library's bank from
; MODINFO, r13 a bank of this task's), and checked to keep what the rules say it keeps: the caller's bank at $8000,
; its zero page $70-$7F, r0-r3.
;
; num.in: records, each
;   op              the entry (its slot: 0 INIT, 1 SET_BASE ...); $FF: the end
;   flags           bit 0: the result is .A/.X bytes at r2; bit 1: r2 in the bank at $8000 (this task's other bank,
;                   selected as every call's made), else in this task's RAM; bit 2: a second result, r6 bytes at r5
;                   (IDIV's remainder)
;   .A .X .Y        the registers for the call
;   room            (2 bytes) r3
;   r0 r1 r4 r5 r6  each a kind, then 0: a value (2 bytes); 1: data in this task's RAM (2 bytes, its length, then
;                   its bytes), the register its address; 2: data in the bank at $8000, the same way; 3: the second
;                   result's place (in this task's RAM)
; num.out: for each call
;   C .A .X .Y      what it gave back
;   r4 r5 r6        (2 bytes each)
;   kept            bit 0: the bank at $8000 isn't the one selected for the call; bit 1: $78-$7F changed; bit 2:
;                   r0-r3 changed
;   then, when C = 0 and flags bit 0: the result, .A/.X bytes from r2; and when C = 0 and flags bit 2: the second
;                   result, r6 bytes from r5

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "numbers.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_num", main

ARG_ROOM        = 2560                                      ; (Each argument's data in RAM at most)
RES_ROOM        = 4096                                      ; (A result in RAM)
FD_IN           = 5                                         ; (Its files' fds: init's own 0-2 not open)
FD_OUT          = 6
CANARY          = $78                                       ; ($78-$7F: the library's zero page, the caller's kept)

.zeropage
lib:        .res        1                                   ; The library's bank (paged ROM)
data:       .res        1                                   ; This task's bank at $8000 for the calls
entry:      .res        1
op:         .res        1
flags:      .res        1
c_a:        .res        1                                   ; The call's .A .X .Y
c_x:        .res        1
c_y:        .res        1
room:       .res        2
regs:       .res        10                                  ; r0, r1, r4, r5, r6 for the call
keep:       .res        8                                   ; r0-r3 as the call had them
ret:        .res        6                                   ; r4-r6 as it left them
ptr:        .res        2
cnt:        .res        2
in_at:      .res        2                                   ; The input buffer: the next byte, the bytes left
in_left:    .res        2
out_n:      .res        2                                   ; The output buffer's bytes
kept:       .res        1                                   ; What the call kept (num.out's)
carry:      .res        1                                   ; Its C

.bss
me:         .res        ME_SIZE
in_buf:     .res        512
out_buf:    .res        512
arg0:       .res        ARG_ROOM
arg1:       .res        ARG_ROOM
arg4:       .res        512
result:     .res        RES_ROOM
result2:    .res        1100

BANK_ARG0       = $8000                                     ; (The arguments and the result, in the bank)
BANK_ARG1       = $8800
BANK_ARG4       = $9000
BANK_RESULT     = $9400

.code
main:
            stz         T_FAILS
            stz         entry                               ; numbers, in the module directory
@find:
            LDR         r0, me
            lda         entry
            jsr         MODINFO
            bcs         @none
            ldx         #0
:
            lda         me + ME_NAME,x
            cmp         s_lib,x
            bne         @next
            inx
            cmp         #0
            bne         :-
            lda         me + ME_TYPE
            EXPECT_A    HT_LIBRARY, "numbers: in the module directory, a library"
            lda         me + ME_BANK
            sta         lib
            bra         @found
@next:
            inc         entry
            bra         @find
@none:
            NOTOK       "numbers: in the module directory"
            jmp         @done

@found:
            lda         #2                                  ; Two banks: the library's, and one for the calls' data
            jsr         BANKS_ALLOC
            EXPECT_OK   "BANKS_ALLOC 2"
            sta         r13
            inc         a
            sta         data
            sta         RAM_BANK
            LDR         r0, s_in
            lda         #O_READ
            jsr         OPEN
            ldx         #FD_IN
            jsr         move_fd
            EXPECT_OK   "OPEN #f/0/num.in"
            LDR         r0, s_out
            lda         #O_WRITE | O_TRUNC
            jsr         OPEN
            ldx         #FD_OUT
            jsr         move_fd
            EXPECT_OK   "OPEN #f/0/num.out"
            stz         in_left
            stz         in_left + 1
            stz         out_n
            stz         out_n + 1

@call:
            jsr         getb                                ; ---- A record
            sta         op
            cmp         #$FF
            bne         :+
            jmp         @end
:
            jsr         getb
            sta         flags
            jsr         getb
            sta         c_a
            jsr         getb
            sta         c_x
            jsr         getb
            sta         c_y
            jsr         getb
            sta         room
            jsr         getb
            sta         room + 1
            ldx         #0                                  ; r0, r1, r4, r5, r6
@arg:
            phx
            jsr         getarg
            plx
            sta         regs,x
            tya
            sta         regs + 1,x
            inx
            inx
            cpx         #10
            bne         @arg

            ldx         #7                                  ; The canary in $78-$7F
:
            txa
            eor         #$A5
            sta         CANARY,x
            dex
            bpl         :-
            MOVR        r0, regs
            MOVR        r1, regs + 2
            MOVR        r4, regs + 4
            MOVR        r5, regs + 6
            MOVR        r6, regs + 8
            lda         flags
            and         #2
            beq         :+
            LDR         r2, BANK_RESULT
            bra         :++
:
            LDR         r2, result
:
            MOVR        r3, room
            ldx         #7
:
            lda         r0,x
            sta         keep,x
            dex
            bpl         :-
            lda         lib
            sta         r14
            lda         op                                  ; (r15: the entry, $A030 + 3 * op)
            asl         a
            adc         op
            adc         #<NUM_INIT
            sta         r15
            lda         #>NUM_INIT
            adc         #0
            sta         r15 + 1
            lda         data                                ; (r13, the library's bank: data less 1)
            dec         a
            sta         r13
            lda         c_a
            ldx         c_x
            ldy         c_y
            jsr         XCALL
            php                                             ; ---- What it gave back
            sta         c_a
            stx         c_x
            sty         c_y
            pla
            and         #1
            sta         carry
            ldx         #5                                  ; (r4-r6, kept from the writes below)
:
            lda         r4,x
            sta         ret,x
            dex
            bpl         :-
            stz         kept                                ; (What it kept: the bank at $8000)
            lda         RAM_BANK
            cmp         data
            beq         :+
            lda         #1
            tsb         kept
:
            ldx         #7                                  ; ($78-$7F)
@canary:
            txa
            eor         #$A5
            cmp         CANARY,x
            beq         :+
            lda         #2
            tsb         kept
:
            dex
            bpl         @canary
            ldx         #7                                  ; (r0-r3)
@regs:
            lda         r0,x
            cmp         keep,x
            beq         :+
            lda         #4
            tsb         kept
:
            dex
            bpl         @regs
            lda         carry
            jsr         putb
            lda         c_a
            jsr         putb
            lda         c_x
            jsr         putb
            lda         c_y
            jsr         putb
            ldx         #0
:
            lda         ret,x
            jsr         putb
            inx
            cpx         #6
            bne         :-
            lda         kept
            jsr         putb
            lda         kept
            beq         :+
            NOTOK       "a call kept the caller's bank, its zero page and r0-r3"
:
            lda         flags                               ; (Its result, if it has one)
            and         #1
            beq         @again
            lda         carry
            bne         @again
            MOVR        ptr, keep + 4                       ; (r2, as it was given)
            lda         c_a
            sta         cnt
            lda         c_x
            sta         cnt + 1
@res:
            lda         cnt
            ora         cnt + 1
            beq         @again
            lda         (ptr)
            jsr         putb
            inc         ptr
            bne         :+
            inc         ptr + 1
:
            lda         cnt
            bne         :+
            dec         cnt + 1
:
            dec         cnt
            bra         @res
@again:
            lda         flags                               ; (A second result, r6 bytes at r5)
            and         #4
            beq         @next2
            lda         carry
            bne         @next2
            MOVR        ptr, regs + 6
            MOVR        cnt, ret + 4
@res2:
            lda         cnt
            ora         cnt + 1
            beq         @next2
            lda         (ptr)
            jsr         putb
            inc         ptr
            bne         :+
            inc         ptr + 1
:
            lda         cnt
            bne         :+
            dec         cnt + 1
:
            dec         cnt
            bra         @res2
@next2:
            jmp         @call

@end:
            jsr         flush
            lda         #FD_OUT
            jsr         CLOSE
            EXPECT_OK   "num.out closed"
            OK          "the calls made (num.in)"
@done:
            DONE        "t_num"

; ---- Reading num.in, writing num.out

; The fd .A (OPEN's, C its result) moved to .X
move_fd:
            bcs         @done
            phx
            pha
            jsr         DUP2
            pla
            jsr         CLOSE
            pla
            clc
@done:
            rts

; .A = num.in's next byte (the end of the file: $FF, the end's record)
getb:
            lda         in_left
            ora         in_left + 1
            bne         @have
            LDR         r0, in_buf
            LDR         r1, 512
            lda         #FD_IN
            jsr         READ
            bcs         @end
            sta         in_left
            stx         in_left + 1
            ora         in_left + 1
            beq         @end
            LDR         in_at, in_buf
@have:
            lda         in_left
            bne         :+
            dec         in_left + 1
:
            dec         in_left
            lda         (in_at)
            inc         in_at
            bne         :+
            inc         in_at + 1
:
            rts
@end:
            lda         #$FF
            rts

; An argument: its kind, then a value or data.  OUT: .A/.Y = the register's value.  Uses .X (0, 2, 4: r0, r1, r4)
getarg:
            stx         cnt                                 ; (Which)
            jsr         getb
            cmp         #0
            bne         @data
            jsr         getb
            pha
            jsr         getb
            tay
            pla
            rts
@data:
            cmp         #3
            bne         @data1
            lda         #<result2                           ; (The second result's place)
            ldy         #>result2
            rts
@data1:
            ldx         cnt
            cmp         #2
            beq         @bank
            lda         ram_lo,x
            sta         ptr
            lda         ram_hi,x
            sta         ptr + 1
            bra         @len
@bank:
            lda         bank_lo,x
            sta         ptr
            lda         bank_hi,x
            sta         ptr + 1
@len:
            lda         ptr                                 ; (Its address, the register's)
            pha
            lda         ptr + 1
            pha
            jsr         getb
            sta         cnt
            jsr         getb
            sta         cnt + 1
@byte:
            lda         cnt
            ora         cnt + 1
            beq         @got
            jsr         getb
            sta         (ptr)
            inc         ptr
            bne         :+
            inc         ptr + 1
:
            lda         cnt
            bne         :+
            dec         cnt + 1
:
            dec         cnt
            bra         @byte
@got:
            ply
            pla
            rts

; .A to num.out (its buffer, written when it's full).  Keeps .X, .Y
putb:
            phx
            phy
            ldx         out_n + 1
            bne         @full
@put:
            ldy         out_n
            sta         out_buf,y
            inc         out_n
            bne         :+
            inc         out_n + 1
:
            ply
            plx
            rts
@full:
            pha
            jsr         flush
            pla
            bra         @put

flush:
            lda         out_n
            ora         out_n + 1
            beq         @rts
            LDR         r0, out_buf
            MOVR        r1, out_n
            lda         #FD_OUT
            jsr         WRITE
            bcc         :+
            NOTOK       "WRITE num.out"
:
            stz         out_n
            stz         out_n + 1
@rts:
            rts

.rodata
s_lib:      .byte       "numbers", 0
s_in:       .byte       "#f/0/num.in", 0
s_out:      .byte       "#f/0/num.out", 0
ram_lo:     .byte       <arg0, 0, <arg1, 0, <arg4, 0, <arg4, 0, <arg4
ram_hi:     .byte       >arg0, 0, >arg1, 0, >arg4, 0, >arg4, 0, >arg4
bank_lo:    .byte       <BANK_ARG0, 0, <BANK_ARG1, 0, <BANK_ARG4, 0, <BANK_ARG4, 0, <BANK_ARG4
bank_hi:    .byte       >BANK_ARG0, 0, >BANK_ARG1, 0, >BANK_ARG4, 0, >BANK_ARG4, 0, >BANK_ARG4

