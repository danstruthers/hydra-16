; ****************************************************************************
; t_mouse - the mouse (docs/design/plans/VIDEO.md, step 6), run as init: vid's /mouse, /mousein and /mousectl
; (mousectl's state; /mouse's first read at once, Plan 9's 49 bytes, then a non-blocking read's E_AGAIN; moves from
; /mousein and the pointer's sprite at them, kept on the screen; swap; the buttons' changes queued, read in turn;
; the pointer off and on; a write to /mouse; bad lines; a claim, the pointer the claimer's, and its release; the
; screen's size in mode 40x30), then the input program (spawned, as init does) reading the emulator's SMC: a move,
; a click and the wheel, from its PS/2 packets.  Its report goes out on the serial port raw (#c/ser).

.include "hydra.inc"
.include "hw.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "testlib.inc"

            HYX2_PROGRAM "t_mouse", main

REC             = 49            ; A /mouse read: m, then 4 fields of 11 and a space
XYB             = 37            ; Its x, y and buttons: m and 3 fields

.zeropage
ms:         .res        1                                   ; /mouse ...
nb:         .res        1                                   ;   another, non-blocking ...
mi:         .res        1                                   ;   /mousein ...
mc:         .res        1                                   ;   /mousectl ...
vc:         .res        1                                   ;   and /dev/vid/ctl
fd:         .res        1
k:          .res        1
n:          .res        2
cnt:        .res        1                                   ; (same's)

.bss
buf:        .res        256

.code

; WRITE the string after the jsr to fd .A.  OUT: C, .A
.macro TO       fdv, text
            lda         fdv
            jsr         put_str
            .byte       text, 0
.endmacro

; buf's first .A bytes the string at label?  OUT: .A = 0 yes
.macro SAME     label
            ldx         #<label
            ldy         #>label
            jsr         same
.endmacro

main:
            stz         T_FAILS
            LDR         r0, s_ser                           ; Fds 0-2: the serial port, raw
            lda         #O_WRITE
            jsr         OPEN
            lda         #0
            jsr         DUP
            lda         #0
            jsr         DUP
            LDR         r0, s_mousectl                      ; ---- mousectl's state
            lda         #O_RDWR
            jsr         OPEN
            sta         mc
            EXPECT_OK   "/dev/vid/mousectl opened"
            jsr         read_mc
            SAME        s_mc0
            EXPECT_A    0, "mousectl: pointer on, swap off"
            LDR         r0, s_mouse                         ; ---- /mouse: at once, as it is
            lda         #O_RDWR
            jsr         OPEN
            sta         ms
            EXPECT_OK   "/dev/vid/mouse opened"
            jsr         read_ms
            cmp         #REC
            php
            pla
            and         #$02                                ; (Z: 49 bytes)
            EXPECT_A    $02, "a read: 49 bytes"
            lda         #REC
            SAME        s_m0
            EXPECT_A    0, "the first read at once: the middle of the screen, no buttons, no time yet"
            LDR         r0, s_mouse
            lda         #O_READ | O_NONBLOCK
            jsr         OPEN
            sta         nb
            jsr         read_nb
            EXPECT_OK   "another's first read, at once"
            jsr         read_nb
            EXPECT_ERR  E_AGAIN, "then nothing new: E_AGAIN"
            LDR         r0, s_mousein                       ; ---- /mousein's moves
            lda         #O_WRITE
            jsr         OPEN
            sta         mi
            EXPECT_OK   "/dev/vid/mousein opened"
            TO          mi, "m 10 -5 1"
            EXPECT_OK   "mousein: m 10 -5 1"
            jsr         read_ms
            lda         #XYB
            SAME        s_m1
            EXPECT_A    0, "the mouse moved: 330, 235, the left button"
            lda         buf + 46                            ; (Its time: some seconds after the start)
            cmp         #' '
            bne         :+
            NOTOK       "its time, in ms"
            bra         :++
:
            OK          "its time, in ms"
:
            jsr         sprite1
            lda         buf
            EXPECT_A    $C1, "the pointer, sprite 1: its image at $1F820 ..."
            lda         buf + 1
            EXPECT_A    $0F, "  4 bits a pixel ..."
            lda         buf + 2
            EXPECT_A    <330, "  x 330 ..."
            lda         buf + 3
            EXPECT_A    >330, "  (its high bits)"
            lda         buf + 4
            EXPECT_A    235, "  y 235 ..."
            lda         buf + 6
            EXPECT_A    $0C, "  shown, in front (z 3) ..."
            lda         buf + 7
            EXPECT_A    $51, "  16 x 16, palette offset 1"
            TO          mi, "m 1000 1000 0"
            jsr         read_ms
            lda         #XYB
            SAME        s_m2
            EXPECT_A    0, "kept on the screen: 639, 479"
            TO          mi, "m -2000 -2000 0"
            jsr         read_ms
            lda         #XYB
            SAME        s_m3
            EXPECT_A    0, "and at its top left: 0, 0"
            TO          mc, "swap on"                       ; ---- swap
            EXPECT_OK   "mousectl: swap on"
            TO          mi, "m 0 0 1"
            jsr         read_ms
            lda         buf + 35
            EXPECT_A    '4', "the left button as the right (4)"
            TO          mi, "m 0 0 4"
            jsr         read_ms
            lda         buf + 35
            EXPECT_A    '1', "the right as the left (1)"
            TO          mc, "swap off"
            TO          mi, "m 0 0 0"
            jsr         read_ms
            jsr         read_mc
            SAME        s_mc0
            EXPECT_A    0, "mousectl: swap off again"
            LDR         r0, s_mouse                         ; ---- The buttons' changes, queued
            lda         #O_READ | O_NONBLOCK
            jsr         OPEN
            sta         nb
            jsr         read_nb
            TO          mi, "m 1 1 1"
            TO          mi, "m 1 1 0"
            TO          mi, "m 1 1 2"
            jsr         read_nb
            lda         buf + 35
            EXPECT_A    '1', "three changes of the buttons, read in turn: 1 ..."
            lda         buf + 11
            EXPECT_A    '1', "  (at 1, 1) ..."
            jsr         read_nb
            lda         buf + 35
            EXPECT_A    '0', "  0 ..."
            lda         buf + 11
            EXPECT_A    '2', "  (at 2, 2) ..."
            jsr         read_nb
            lda         buf + 35
            EXPECT_A    '2', "  2 (the middle) ..."
            jsr         read_nb
            EXPECT_ERR  E_AGAIN, "  and nothing more"
            TO          mi, "m 0 0 0"
            lda         ms                                  ; (/mouse again: the changes it had queued, gone)
            jsr         CLOSE
            LDR         r0, s_mouse
            lda         #O_RDWR
            jsr         OPEN
            sta         ms
            jsr         read_ms
            TO          mc, "pointer off"                   ; ---- The pointer off, on
            EXPECT_OK   "mousectl: pointer off"
            jsr         sprite1
            lda         buf + 6
            EXPECT_A    0, "the pointer hidden (z 0)"
            TO          mc, "pointer on"
            jsr         sprite1
            lda         buf + 6
            EXPECT_A    $0C, "pointer on: shown"
            TO          ms, "m 5 6"                         ; ---- A write to /mouse
            EXPECT_OK   "/mouse: m 5 6"
            jsr         read_ms
            lda         #XYB
            SAME        s_m4
            EXPECT_A    0, "the mouse there: 5, 6"
            TO          ms, "x 1 2"
            EXPECT_ERR  E_INVAL, "/mouse: x 1 2, E_INVAL"
            TO          mi, "m 1"
            EXPECT_ERR  E_INVAL, "mousein: m 1, E_INVAL"
            TO          mc, "pointer maybe"
            EXPECT_ERR  E_INVAL, "mousectl: pointer maybe, E_INVAL"
            LDR         r0, s_ctl                           ; ---- A claim: the pointer the claimer's
            lda         #O_WRITE
            jsr         OPEN
            sta         vc
            TO          vc, "claim"
            EXPECT_OK   "claim"
            lda         #<$FC0E                             ; (Sprite 1's z, read from the chip: ADDR0)
            sta         VERA_ADDR_L
            lda         #>$FC0E
            sta         VERA_ADDR_M
            lda         #1
            sta         VERA_ADDR_H
            lda         VERA_DATA0
            EXPECT_A    0, "claimed: the pointer off (the claimer draws its own)"
            TO          mi, "m 1 1 0"
            jsr         read_ms
            lda         #XYB
            SAME        s_m5
            EXPECT_A    0, "the mouse still read while claimed: 6, 7"
            TO          vc, "release"
            jsr         sprite1
            lda         buf + 2
            EXPECT_A    6, "released: the pointer at 6, 7 again ..."
            lda         buf + 6
            EXPECT_A    $0C, "  shown"
            TO          vc, "mode 40x30"                    ; ---- The screen's size: 320 x 240
            TO          mi, "m 1000 1000 0"
            jsr         read_ms
            lda         #XYB
            SAME        s_m6
            EXPECT_A    0, "mode 40x30: kept to 319, 239"
            TO          vc, "mode 80x60"
            TO          mi, "m 0 0 0"
            jsr         read_ms
            lda         #0                                  ; ---- The input program, on the SMC's mouse
            LDR         r0, s_input
            stz         r1
            stz         r1 + 1
            lda         #SPAWN_NEWGROUP
            jsr         SPAWN
            EXPECT_OK   "the input program started"
            lda         #'0'                                ; (The script's: a move of 30, 20, a click, the wheel up;
            sta         seen_last                           ;   the last read's buttons 0)
            ldx         #0
@change:
            phx
            jsr         read_ms
            plx
            lda         buf + 35                            ; (Its buttons' digit: 0, 1, 8; a change of them)
            cmp         seen_last
            beq         @same
            sta         seen_last
            sta         seen_list,X
            inx
            cpx         #4
            bcc         @change
            bra         @seen
@same:
            bra         @change
@seen:
            ldx         #<s_buttons
            ldy         #>s_buttons
            lda         #4
            jsr         same_mem
            EXPECT_A    0, "the input program: the buttons from its packets, 1, 0, 8, 0 (a click, the wheel up)"
            lda         #XYB
            SAME        s_m7
            EXPECT_A    0, "  the move from its packets: 349, 259"
            jsr         sprite1
            lda         buf + 2
            EXPECT_A    <349, "  and the pointer there"
            DONE        "t_mouse"

; ****************************************************************************

; The string after the jsr written to fd .A.  OUT: C, .A (the error)
put_str:
            sta         fd
            pla                                                 ; (The string: past the jsr's return)
            clc
            adc         #1
            sta         n
            pla
            adc         #0
            sta         n + 1
            ldy         #0
:
            lda         (n),Y
            beq         :+
            iny
            bra         :-
:
            sty         k
            tya                                                 ; (The return: past its 0)
            clc
            adc         n
            tax
            lda         n + 1
            adc         #0
            pha
            phx
            MOVR        r0, n
            lda         k
            sta         r1
            stz         r1 + 1
            lda         fd
            jmp         WRITE

; /mouse read (ms) into buf.  OUT: .A, C
read_ms:
            LDR         r0, buf
            LDR         r1, REC
            lda         ms
            jmp         READ

; The non-blocking /mouse read (nb) into buf.  OUT: C, .A
read_nb:
            LDR         r0, buf
            LDR         r1, REC
            lda         nb
            jmp         READ

; mousectl's state into buf.  OUT: .A, its length
read_mc:
            LDR         r0, 0
            stz         r1
            stz         r1 + 1
            lda         mc
            ldx         #0
            jsr         SEEK
            LDR         r0, buf
            LDR         r1, 255
            lda         mc
            jmp         READ

; Sprite 1's 8 bytes into buf, from /sprites
sprite1:
            LDR         r0, s_sprites
            lda         #O_READ
            jsr         OPEN
            sta         k
            LDR         r0, 8
            stz         r1
            stz         r1 + 1
            lda         k
            ldx         #0
            jsr         SEEK
            LDR         r0, buf
            LDR         r1, 8
            lda         k
            jsr         READ
            lda         k
            jmp         CLOSE

; buf's first .A bytes the string at .X/.Y (its length .A too)?  OUT: .A = 0 yes
same:
            sta         cnt
            stx         n
            sty         n + 1
            ldy         #0
:
            lda         (n),Y
            beq         @end
            cpy         cnt
            bcs         @no
            cmp         buf,Y
            bne         @no
            iny
            bra         :-
@end:
            cpy         cnt
            bne         @no
            lda         #0
            rts
@no:
            lda         #1
            rts

; .A bytes at seen_list the same as at .X/.Y?  OUT: .A = 0 yes
same_mem:
            sta         cnt
            stx         n
            sty         n + 1
            ldy         #0
:
            lda         (n),Y
            cmp         seen_list,Y
            bne         @no
            iny
            cpy         cnt
            bcc         :-
            lda         #0
            rts
@no:
            lda         #1
            rts

.bss
seen_last:  .res        1
seen_list:  .res        6

.rodata
s_ser:      .byte       "#c/ser", 0
s_mouse:    .byte       "#v/mouse", 0
s_mousein:  .byte       "#v/mousein", 0
s_mousectl: .byte       "#v/mousectl", 0
s_ctl:      .byte       "#v/ctl", 0
s_sprites:  .byte       "#v/sprites", 0
s_input:    .byte       "#m/input", 0
s_mc0:      .byte       "pointer on", LF, "swap off", LF, 0
s_m0:       .byte       "m        320         240           0           0 ", 0
s_m1:       .byte       "m        330         235           1 ", 0
s_m2:       .byte       "m        639         479           0 ", 0
s_m3:       .byte       "m          0           0           0 ", 0
s_m4:       .byte       "m          5           6           0 ", 0
s_m5:       .byte       "m          6           7           0 ", 0
s_m6:       .byte       "m        319         239           0 ", 0
s_m7:       .byte       "m        349         259           0 ", 0
s_buttons:  .byte       "1080"
