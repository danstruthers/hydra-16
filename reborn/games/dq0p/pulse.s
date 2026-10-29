; ****************************************************************************
; pulse.s - (dq0p) the sound layer's pulse (sound.c's s_pulse, part 5's), in the hub's SIGHT_BANK (in the window): a
; grid of the squares round the character, SND_W across (hub.h's SOUND_AT, SND_GRID: a row a border square each side),
; each a byte: $FF rock, none; $FE open; $FD shut (a door, a closed window); $FC a wall; worked out, the squares a
; sound goes from the character's to each it reaches, round corners (the eight ways, a corner cut only beside one
; that isn't rock), a shut one DOOR_SND more, a wall's WALLX more (SOUND.md's four a square of it), either marked
; SND_AIR, as each square it comes from is (the air doesn't pass): the least, Dijkstra's way with three queues (the
; open squares', the shut ones', the walls'), each taken from the one whose next is the nearest, so every square's
; first mark is its least.  (In C, a square's eight ways a few hundred cycles each: the pulse some 1.5M cycles.)
; Nothing past MAXC squares.
;
;   void __fastcall__ pulse_run (unsigned char* centre);    the character's square in the grid (0 put on it)

            .export     _pulse_run
            .importzp   ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, tmp4, sreg, regsave

GRID        = $8000 + $1100 + 48            ; (hub.h's SND_GRID: SOUND_AT, its head)
W           = 43                            ; (SND_W)
QM          = GRID + W * W                  ; (SND_QUEUE: the open squares' queue, 512 of them, each its address)
QMEND       = QM + 1024
QDL         = QMEND                         ; (The shut squares', 64: their addresses' low bytes, high)
QDH         = QDL + 64
QWL         = QDH + 64                      ; (The walls', 128)
QWH         = QWL + 128
MAXC        = 58
OPEN        = $FE
SHUT        = $FD
WALL        = $FC
AIR         = $40
DOORX       = 2                             ; (DOOR_SND)
WALLX       = 4

            .segment    "SW5CODE"

; ptr1: the square taken (less W + 1: its eight about it at (ptr1),y, y 0 ... 2 W + 2); ptr2, ptr3 the open queue's
; head and tail; tmp3, tmp4 the shut queue's; ptr4, ptr4 + 1 the walls'; tmp1, tmp2, regsave + 2 the marks for a
; square open, shut and a wall from it
_pulse_run:
            sta         ptr1
            stx         ptr1 + 1
            lda         #0
            tay
            sta         (ptr1),y
            sta         tmp3
            sta         tmp4
            sta         ptr4
            sta         ptr4 + 1
            lda         #<QM
            sta         ptr2
            sta         ptr3
            lda         #>QM
            sta         ptr2 + 1
            sta         ptr3 + 1
            lda         ptr1
            ldx         ptr1 + 1
            jsr         pushm
loop:       ldx         #$FF                ; (The queue whose next is the nearest: .X, its cost regsave)
            lda         ptr2
            cmp         ptr3
            bne         @main
            lda         ptr2 + 1
            cmp         ptr3 + 1
            beq         @door
@main:      ldy         #0
            lda         (ptr2),y
            sta         sreg
            iny
            lda         (ptr2),y
            sta         sreg + 1
            dey
            lda         (sreg),y
            and         #$3F
            sta         regsave
            ldx         #0
@door:      ldy         tmp3
            cpy         tmp4
            beq         @wall
            lda         QDL,y
            sta         sreg
            lda         QDH,y
            sta         sreg + 1
            ldy         #0
            lda         (sreg),y
            and         #$3F
            cpx         #$FF
            beq         :+
            cmp         regsave
            bcs         @wall
:           sta         regsave
            ldx         #1
@wall:      ldy         ptr4
            cpy         ptr4 + 1
            beq         @pick
            lda         QWL,y
            sta         sreg
            lda         QWH,y
            sta         sreg + 1
            ldy         #0
            lda         (sreg),y
            and         #$3F
            cpx         #$FF
            beq         :+
            cmp         regsave
            bcs         @pick
:           ldx         #2
@pick:      cpx         #0
            beq         popm
            cpx         #1
            beq         popd
            cpx         #2
            beq         popw
            rts                                 ; (All empty: done)
popw:       ldy         ptr4
            lda         QWL,y
            sta         ptr1
            lda         QWH,y
            sta         ptr1 + 1
            iny
            tya
            and         #127
            sta         ptr4
            jmp         expand
popd:       ldy         tmp3
            lda         QDL,y
            sta         ptr1
            lda         QDH,y
            sta         ptr1 + 1
            iny
            tya
            and         #63
            sta         tmp3
            jmp         expand
popm:       ldy         #0                  ; (The open queue's head)
            lda         (ptr2),y
            sta         ptr1
            iny
            lda         (ptr2),y
            sta         ptr1 + 1
            clc
            lda         ptr2
            adc         #2
            sta         ptr2
            bcc         :+
            inc         ptr2 + 1
:           lda         ptr2
            cmp         #<QMEND
            bne         expand
            lda         ptr2 + 1
            cmp         #>QMEND
            bne         expand
            lda         #<QM
            sta         ptr2
            lda         #>QM
            sta         ptr2 + 1
expand:     sec                             ; (ptr1 the square: less W + 1)
            lda         ptr1
            sbc         #W + 1
            sta         ptr1
            bcs         :+
            dec         ptr1 + 1
:           ldy         #W + 1
            lda         (ptr1),y
            tax
            and         #$3F
            cmp         #MAXC
            bcc         :+
            jmp         loop
:           txa                             ; (Open from it: a square more, its air as it is;
            clc
            adc         #1
            sta         tmp1
            txa                             ;   shut: DOORX more, the air stopped; a wall WALLX more)
            and         #$3F
            clc
            adc         #1 + DOORX
            ora         #AIR
            sta         tmp2
            clc
            adc         #WALLX - DOORX
            sta         regsave + 2
            ldy         #1                  ; (N, W, E, S)
            jsr         try
            ldy         #W
            jsr         try
            ldy         #W + 2
            jsr         try
            ldy         #2 * W + 1
            jsr         try
            ldy         #0                  ; (NW: by N or W)
            lda         #1
            ldx         #W
            jsr         tryd
            ldy         #2                  ; (NE: by N or E)
            lda         #1
            ldx         #W + 2
            jsr         tryd
            ldy         #2 * W              ; (SW: by W or S)
            lda         #W
            ldx         #2 * W + 1
            jsr         tryd
            ldy         #2 * W + 2          ; (SE: by E or S)
            lda         #W + 2
            ldx         #2 * W + 1
            jsr         tryd
            jmp         loop

; The square at (ptr1),y: not marked yet, marked from the square taken (open, shut, a wall) and queued
try:        lda         (ptr1),y
            cmp         #WALL
            bcc         @no                 ; (Marked already)
            cmp         #$FF
            beq         @no                 ; (Rock)
            cmp         #OPEN
            beq         @open
            cmp         #SHUT
            beq         @shut
            lda         regsave + 2         ; (A wall: its queue)
            sta         (ptr1),y
            tya
            clc
            adc         ptr1
            ldx         ptr4 + 1
            sta         QWL,x
            lda         ptr1 + 1
            adc         #0
            sta         QWH,x
            inx
            txa
            and         #127
            sta         ptr4 + 1
            rts
@shut:      lda         tmp2                ; (Shut: its queue)
            sta         (ptr1),y
            tya
            clc
            adc         ptr1
            ldx         tmp4
            sta         QDL,x
            lda         ptr1 + 1
            adc         #0
            sta         QDH,x
            inx
            txa
            and         #63
            sta         tmp4
@no:        rts
@open:      lda         tmp1
            sta         (ptr1),y
            tya
            clc
            adc         ptr1
            pha
            lda         ptr1 + 1
            adc         #0
            tax
            pla
; The open queue's tail: .A, .X a square's address
pushm:      ldy         #0
            sta         (ptr3),y
            txa
            iny
            sta         (ptr3),y
            clc
            lda         ptr3
            adc         #2
            sta         ptr3
            bcc         :+
            inc         ptr3 + 1
:           lda         ptr3
            cmp         #<QMEND
            bne         @done
            lda         ptr3 + 1
            cmp         #>QMEND
            bne         @done
            lda         #<QM
            sta         ptr3
            lda         #>QM
            sta         ptr3 + 1
@done:      rts

; A corner square at (ptr1),y, by the squares at (ptr1),a or (ptr1),x: the same, if either isn't rock
tryd:       sta         sreg
            stx         sreg + 1
            lda         (ptr1),y
            cmp         #WALL
            bcc         @no
            cmp         #$FF
            beq         @no
            sty         regsave + 1
            ldy         sreg
            lda         (ptr1),y
            cmp         #$FF
            bne         @by
            ldy         sreg + 1
            lda         (ptr1),y
            cmp         #$FF
            beq         @no
@by:        ldy         regsave + 1
            jmp         try
@no:        rts
