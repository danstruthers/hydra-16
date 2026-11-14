; ****************************************************************************
; walk.s - (dq0lib) one way-finder over one rule for a step (ENGINE-CRITIQUE.md's §4, step 2's second part: one
; source 2026-11-09, games/dq0w/walk.inc compiled into the world's part 21 and the denizens' part 18; the paged ROM's
; 2026-11-14): the character's ways across a room or about a passage (the world's s_ways_go), an actor's way to its
; mark (the denizens' route).  Its contract the zero page's block (../games/dq0/lnzp.inc's LZ_CL ...: walk.h's wk_*
; for the C), the task's tables (struct ats's t_kinds, cornk, dir_x, dir_y).
;
; The rule (the one chunk.c's step_ok and slant.s's slant go by, a step at a time): a square can be stood on if its
; kind can (world.h's T_PASS: not rock, a wall, a window, a pit, a cut), and a door's only if it's open (or broken
; down) and not hidden; a step to the square beside it rises or falls no more than STEP_MAX feet (STAIR_MAX on steps,
; either end); a diagonal step only a diagonal way's, past two corners neither of which can be stood on (rock, a wall,
; a door, a window, a pit), one of them cut.  Each walker's differences, its rules (LZ_RULES: walk.h's WK_HANDS, a
; shut door not hidden a way, worked as it gets there; WK_NODOOR, no door's square a way), and what its reader adds as
; it reads the squares in (walk.h's WD_NO: a square it won't go onto, the character's a denizen's square or a trap it
; knows of, a denizen's water deeper than it wades or bad air; WD_TRAP, a hidden trap the walk would set off).
;
;   WK_ROW      a row's squares' cells, by the rule (each reader's wk_cell before): LZ_U the kinds' bytes (made their
;               cells), LZ_V their doors' (walk.h's WD_*: a portal's PS_HIDDEN and PS_OPEN, or WD_NONE; WD_NO,
;               WD_TRAP), LZ_DIR how many
;   WK_SEED     square LZ_V queued, a seed (where the search begins: the character's square; the mark, or the
;               field's edges toward it)
;   WK_SEARCH   the search over the grid of CW by CW cells (LZ_CL; their floors LZ_EL) from the cells queued (LZ_RING,
;               LZ_QH to LZ_QT) outward a step at a time, the four ways then the diagonal ways in dir_x's order, the
;               first way to each square found kept: its cell then the way it was come into (0-7; WC_SEED a seed),
;               WC_STEPS and WC_CORNER kept; within the box LZ_X0, LZ_Y0 (LZ_W by LZ_H); stopped as it comes to
;               LZ_STOP; each square reached told to LZ_REACH's routine, if there's one (LZ_U from, LZ_V reached,
;               LZ_DIR the way, LZ_DZ its floor above u's, LZ_V8 its cell before: the world's times and traps)

.include "hydra.inc"
            .include    "../games/dq0/lnzp.inc"
            .include    "dq0hub_walk.inc"       ; (world.h's CW, T_DOOR, T_STEPS, STEP_MAX, STAIR_MAX, PS_HIDDEN, PS_OPEN)
            .macpack    longbranch
            .export     wk_row, wk_seed, wk_search

WC_NEW      = $80                           ; (walk.h's: not reached, or never; it can be stood on; a diagonal may
WC_OPEN     = $40                           ;   pass its corner; steps; its corner cut, a trap on it (not reached);
WC_CORNER   = $20                           ;   a seed (reached))
WC_STEPS    = $10
WC_CUT      = $01
WC_TRAP     = $02
WC_SEED     = 8
WD_NONE     = $80                           ; (walk.h's: a square's door byte: no portal; the walker won't; a hidden
WD_NO       = $40                           ;   trap)
WD_TRAP     = $10
.assert     (PS_HIDDEN | PS_OPEN) & (WD_NONE | WD_NO | WD_TRAP) = 0, error, "walk.s: a door's byte"
.assert     CW = 32, error, "walk.s: CW"

.code

; WK_ROW
wk_row:
            ldy         #ATS_KINDS              ; (r0 t_kinds, r1 cornk)
            ldx         #0
:
            lda         (LZ_ATS),y
            sta         r0,x
            iny
            inx
            cpx         #4
            bne         :-
.assert     ATS_CORNK = ATS_KINDS + 2 && r1 = r0 + 2, error, "walk.s: ats's kinds"
            ldy         #0
@sq:
            cpy         LZ_DIR
            bcs         @done
            lda         (LZ_V),y
            sta         r2                      ; (Its door's byte)
            lda         (LZ_U),y
            and         #$0F
            sta         r3                      ; (Its kind)
            phy
            tay
            lda         (r1),y                  ; A diagonal's corner: 0 stood on, 1 not, 2 cut
            beq         :+
            lsr         a
            ora         #WC_CORNER
:
            ora         #WC_NEW
            cpy         #T_STEPS
            bne         :+
            ora         #WC_STEPS
:
            sta         r3 + 1                  ; (Its cell)
            lda         (r0),y                  ; T_PASS
            ply
            lsr         a
            bcc         @put
            lda         r3                      ; (A door's: open, not hidden, or shut and its hands to work it)
            cmp         #T_DOOR
            bne         @open
            bit         LZ_RULES
            bmi         @put                    ; (WK_NODOOR)
            lda         r2
            bmi         @open                   ; (No portal)
            and         #PS_HIDDEN
            bne         @put
            lda         r2
            and         #PS_OPEN
            bne         @open
            bit         LZ_RULES
            bvc         @put                    ; (WK_HANDS)
@open:
            lda         r2                      ; (Not the walker's: no)
            and         #WD_NO
            bne         @put
            lda         r2
            and         #WD_TRAP
            beq         :+
            lda         #WC_TRAP
:
            ora         #WC_OPEN
            ora         r3 + 1
            sta         r3 + 1
@put:
            lda         r3 + 1
            sta         (LZ_U),y
            iny
            bra         @sq
@done:
            rts

; WK_SEED
wk_seed:
            lda         LZ_CL                   ; Its cell: a seed's, its steps and its corner kept
            clc
            adc         LZ_V
            sta         r0
            lda         LZ_CL + 1
            adc         LZ_V + 1
            sta         r0 + 1
            lda         (r0)
            and         #WC_STEPS | WC_CORNER
            ora         #WC_SEED
            sta         (r0)
            ; (fall through)

; LZ_V queued
queue:
            lda         LZ_QT
            asl         a
            sta         r0
            lda         LZ_QT + 1
            rol         a
            sta         r0 + 1
            lda         r0
            clc
            adc         LZ_RING
            sta         r0
            lda         r0 + 1
            adc         LZ_RING + 1
            sta         r0 + 1
            lda         LZ_V
            sta         (r0)
            ldy         #1
            lda         LZ_V + 1
            sta         (r0),y
            inc         LZ_QT
            bne         :+
            inc         LZ_QT + 1
:
            lda         LZ_QT + 1
            and         #>(WK_RN - 1)
            sta         LZ_QT + 1
            rts
WK_RN       = 512                           ; (walk.h's: the queue's ring)

; WK_SEARCH
wk_search:
@loop:
            lda         LZ_QH                   ; (The queue empty: done)
            cmp         LZ_QT
            bne         :+
            lda         LZ_QH + 1
            cmp         LZ_QT + 1
            bne         :+
            rts
:
            lda         LZ_QH                   ; u, the next
            asl         a
            sta         r0
            lda         LZ_QH + 1
            rol         a
            sta         r0 + 1
            lda         r0
            clc
            adc         LZ_RING
            sta         r0
            lda         r0 + 1
            adc         LZ_RING + 1
            sta         r0 + 1
            lda         (r0)
            sta         LZ_U
            ldy         #1
            lda         (r0),y
            sta         LZ_U + 1
            inc         LZ_QH
            bne         :+
            inc         LZ_QH + 1
:
            lda         LZ_QH + 1
            and         #>(WK_RN - 1)
            sta         LZ_QH + 1
            lda         LZ_U                    ; (The actor reached: no further)
            cmp         LZ_STOP
            bne         :+
            lda         LZ_U + 1
            cmp         LZ_STOP + 1
            bne         :+
            rts
:
            lda         LZ_U                    ; Its x and y, its cell and floor
            and         #CW - 1
            sta         LZ_UX
            lda         LZ_U + 1
            sta         r0
            lda         LZ_U
            ldx         #5
:
            lsr         r0
            ror         a
            dex
            bne         :-
            sta         LZ_UY
            lda         LZ_CL
            ldx         LZ_CL + 1
            jsr         atu
            sta         LZ_U8
            lda         LZ_EL
            ldx         LZ_EL + 1
            jsr         atu
            sta         LZ_EU
            stz         LZ_DIR
            jsr         dirs
@way:
            ldy         LZ_DIR                  ; The square beside it, the way e: in the grid, in the box
            lda         (r2),y
            clc
            adc         LZ_UX
            sta         LZ_VX
            lda         (r3),y
            clc
            adc         LZ_UY
            sta         LZ_VY
            ora         LZ_VX
            and         #<~(CW - 1)
            jne         @next
            lda         LZ_VX
            sec
            sbc         LZ_X0
            cmp         LZ_W
            jcs         @next
            lda         LZ_VY
            sec
            sbc         LZ_Y0
            cmp         LZ_H
            jcs         @next
            lda         LZ_VY                   ; v: vy << CW_BITS | vx
            lsr         a
            lsr         a
            lsr         a
            sta         LZ_V + 1
            lda         LZ_VY
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            ora         LZ_VX
            sta         LZ_V
            lda         LZ_CL                   ; (Reached already, or not to be stood on: none)
            ldx         LZ_CL + 1
            jsr         atv
            sta         LZ_V8
            and         #WC_NEW | WC_OPEN
            cmp         #WC_NEW | WC_OPEN
            jne         @next
            lda         LZ_DIR                  ; A diagonal way: its corners neither stood on, one cut
            cmp         #4
            bcc         @rise
            ldx         LZ_UY
            lda         LZ_VX
            jsr         cell
            sta         LZ_C
            ldx         LZ_VY
            lda         LZ_UX
            jsr         cell
            sta         LZ_I
            and         LZ_C
            and         #WC_CORNER
            jeq         @next
            lda         LZ_C
            and         #WC_NEW | WC_CUT
            cmp         #WC_NEW | WC_CUT
            beq         @rise
            lda         LZ_I
            and         #WC_NEW | WC_CUT
            cmp         #WC_NEW | WC_CUT
            jne         @next
@rise:
            lda         LZ_U8                   ; Its rise or fall: STEP_MAX, STAIR_MAX on steps
            ora         LZ_V8
            and         #WC_STEPS
            beq         :+
            lda         #STAIR_MAX
            bra         :++
:
            lda         #STEP_MAX
:
            sta         LZ_LIM
            lda         LZ_EL
            ldx         LZ_EL + 1
            jsr         atv
            sec
            sbc         LZ_EU
            sta         LZ_DZ
            clc                                 ; (-lim to lim: dz + lim not past 2 lim)
            adc         LZ_LIM
            sta         r4
            lda         LZ_LIM
            asl         a
            cmp         r4
            jcc         @next
            lda         LZ_CL                   ; Reached: the way it came into it, its steps and corner kept
            ldx         LZ_CL + 1
            jsr         atv
            lda         LZ_V8
            and         #WC_STEPS | WC_CORNER
            ora         LZ_DIR
            sta         (r0)
            lda         LZ_REACH + 1            ; (Told, the task's routine: its times, its traps)
            beq         :+
            jsr         reach
            jsr         dirs
:
            jsr         queue
@next:
            inc         LZ_DIR
            lda         LZ_DIR
            cmp         #8
            jcc         @way
            jmp         @loop

reach:
            jmp         (LZ_REACH)

; (dir_x, dir_y: r2, r3)
dirs:
            ldy         #ATS_DIRX
            ldx         #0
:
            lda         (LZ_ATS),y
            sta         r2,x
            iny
            inx
            cpx         #4
            bne         :-
            rts
.assert     ATS_DIRY = ATS_DIRX + 2 && r3 = r2 + 2, error, "walk.s: ats's dir_x, dir_y"

; The grid at .A .X (the cells, the floors): u's (atu), v's (atv): .A, r0 its place
atu:
            clc
            adc         LZ_U
            sta         r0
            txa
            adc         LZ_U + 1
            sta         r0 + 1
            lda         (r0)
            rts
atv:
            clc
            adc         LZ_V
            sta         r0
            txa
            adc         LZ_V + 1
            sta         r0 + 1
            lda         (r0)
            rts

; The cell at x .A, y .X
cell:
            sta         r1
            txa
            lsr         a
            lsr         a
            lsr         a
            sta         r1 + 1
            txa
            asl         a
            asl         a
            asl         a
            asl         a
            asl         a
            ora         r1
            clc
            adc         LZ_CL
            sta         r1
            lda         r1 + 1
            adc         LZ_CL + 1
            sta         r1 + 1
            lda         (r1)
            rts
