; ****************************************************************************
; slant.s - (the world's; the denizens' and the senses' include it) world.h's diagonal ways (the user's, 2026-10-31:
; passages off the compass), in assembly for the main RAM's room: slant (), a diagonal way from a square, past two
; corners neither of which can be stood on, one of them cut (T_CUT); step8 (), a heading's step, the four's by
; step_ok, a diagonal by slant; and the squares' kinds (t_kinds: world.h's T_PASS).

.ifdef SLANT_IN_PART                        ; (The denizens': slant a swapped part's, SLANT_SEG, its stub
            .export     _s_slant, _t_kinds, _cornk  ;   bank.s's; no step8)
.else
            .export     _slant, _step8, _t_kinds, _cornk
.endif
            .import     _at, _step_ok, _head_x, _head_y, pusha, pushax, incsp5
            .importzp   sp, ptr1

            .rodata
; Each kind's (world.h's T_ROCK ... T_CUT): 1 it can be stood on (T_PASS)
_t_kinds:   .byte       0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0
; A diagonal's corner, by its kind: 0 one that can be stood on (not a door), 1 one that can't, 2 a cut
_cornk:
cornk:      .byte       1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 2, 1, 1, 1, 1

            .bss
ssn:        .res        1                   ; (slant's: its arguments, kept: y, then x)
sy:         .res        2
sx:         .res        2
sh:         .res        1
ca:         .res        1                   ; (The first corner's)

.ifdef SLANT_IN_PART
            .segment    SLANT_SEG
.else
            .code

; unsigned char __fastcall__ step8 (unsigned char sn, unsigned x, unsigned y, unsigned char h): h in .A, the rest on
; the C stack: the four (h even) step_ok's, h >> 1; a diagonal slant's
_step8:
            lsr         a
            bcs         @diag
            jmp         _step_ok
@diag:      rol         a                   ; (h again: the carry its low bit)
            jmp         _slant
.endif

; unsigned char __fastcall__ slant (unsigned char sn, unsigned x, unsigned y, unsigned char h): from x, y heading h (a
; diagonal) a diagonal way: corners (x + head_x[h], y) and (x, y + head_y[h]) neither one that can be stood on, one
; of them cut, and its square walkable (step_ok's rule, by dir_x's diagonals: 4 + h / 2)
.ifdef SLANT_IN_PART
_s_slant:
.else
_slant:
.endif
            sta         sh
            ldy         #4
            lda         (sp),y
            sta         ssn
            dey
@arg:       lda         (sp),y              ; (x, y: the stack's four bytes, y first)
            sta         sy,y
            dey
            bpl         @arg
            jsr         incsp5
            lda         ssn                 ; The first corner: at (sn, x + head_x[h], y)
            jsr         pusha
            ldx         sh
            lda         _head_x,x
            ldx         #2
            jsr         add_x
            jsr         pushax
            lda         sy
            ldx         sy + 1
            jsr         _at
            jsr         corner
            beq         @no
            sta         ca
            lda         ssn                 ; The second: at (sn, x, y + head_y[h])
            jsr         pusha
            lda         sx
            ldx         sx + 1
            jsr         pushax
            ldx         sh
            lda         _head_y,x
            ldx         #0
            jsr         add_x
            jsr         _at
            jsr         corner
            beq         @no
            ora         ca
            and         #2
            beq         @no                 ; (Neither cut: squeezed between solid corners)
            lda         ssn                 ; Its square: step_ok (sn, x, y, 4 + h / 2)
            jsr         pusha
            lda         sx
            ldx         sx + 1
            jsr         pushax
            lda         sy
            ldx         sy + 1
            jsr         pushax
            lda         sh
            lsr         a
            clc
            adc         #4
            jmp         _step_ok
@no:        lda         #0
            tax
            rts

; y (.X 0) or x (.X 2) plus the signed byte in .A, in .A .X (none kept)
add_x:      ldy         #0
            cmp         #$80
            bcc         @pos
            dey
@pos:       clc
            adc         sy,x
            pha
            tya
            adc         sy + 1,x
            tax
            pla
            rts

; The corner at the square at ()'s .A .X points to (NULL: past the stratum, solid): cornk's, its flags set by it
corner:     sta         ptr1
            stx         ptr1 + 1
            ora         ptr1 + 1
            beq         @out
            ldy         #0
            lda         (ptr1),y
            and         #$0F
            tax
            lda         cornk,x
            rts
@out:       lda         #1
            rts
