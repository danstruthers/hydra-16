; ****************************************************************************
; line.s - (dq0lib) one line between two squares (ENGINE-CRITIQUE.md's §4, step 2: one source 2026-11-08, the world's
; and the senses' dq0w/line.inc assembled into each; the paged ROM's 2026-11-14), line.h's: the eye's (a denizen's
; sight of the character, the lights the character sees far off), a missile's (the world's line of fire, wshoot.c's)
; and the air's (a smell's past the sound layer).  One rule for what a square lets through and one way across, so that
; what the character sees it can shoot, and a denizen sees the character just when the character would see it.  The
; look's own lines (the senses' cast.s's ray, in its grid: thousands a look) are this one worked out in the grid: the
; same squares and heights, its doors and windows by ln_portal.
;
; The squares: Bresenham's, as cast.s's ray steps them (a step along if 2e >= -|dy|, across if 2e <= |dx|), from the
; first end, those between the two ends, 255 squares each way at most (at a tie not the same squares as from the other
; end: a line between the character and another is drawn from the character's end, line.h's).  What stops it: no
; square (outside the stratum), rock, a wall; a portal by ln_portal (a door shut, a window's shutters closed; glass
; the eye's alone; bars, a portcullis's up or down or a grate's, and an arrow slit let anything through); and for the
; eye and a missile a floor above the line from the eyes (ln_eye feet above the first square's floor) to the last
; square's floor, cast.s's: its height a step on ez + (tz - ez) k / n, worked out without dividing.
;
; Its squares read by the library's own at () (sq.s's sq_at: the task's chunk state, ../games/dq0/lnzp.inc's struct
; ats), its portals by sq_door, the cache asked by ck_at; what it keeps the zero page's block's (lnzp.inc's LZ_*: the
; task's stubs, games/dq0w/line.inc, put its arguments there).  The entries (lnzp.inc's):
;
;   LN_LINE     line (sn, x, y, tx, ty, mode): .A 0 clear, LN_STOP a square stops it, LN_WAY ln_way's said so (mode
;               LN_CACHED too: a chunk not in the cache stops it, none made: the senses' lights far off); .X 0
;   LN_PASS     ln_pass (sn, x, y, mode): a square: .A 0 it stops a line, 1 it lets it through, 2 narrowly (a slit, a
;               window); .X 0
;   LN_PORTAL   ln_portal (o, mode): portal r0, the same

.include "hydra.inc"
            .include    "../games/dq0/lnzp.inc"
            .include    "dq0hub_line.inc"       ; (world.h's squares, T_*; struct portal's kind, state, parts; PK_*, PP_*,
                                                ;   PS_*: the build's, games/dq0/hubgen.js)
            .macpack    longbranch
            .export     ln_line, ln_pass, ln_portal
            .import     sq_at, sq_door, ck_at

LN_AIR      = 2                             ; (line.h's: the modes,
LN_STOP     = 1                             ;   line's answers)
LN_WAY      = 2

xy          = LZ_XY                         ; (The last square (ty, tx), the square it's at (y, x), as the C stack had
lsn         = LZ_SN                         ;   them; the stratum, the mode; at ()'s pointer to the square; how far,
lmode       = LZ_MODE                       ;   each way, and the way (1, or $FF back); its steps; Bresenham's error,
lp          = LZ_LP                         ;   twice it; the air's (no heights); the line's height, the rise or fall
ax8         = LZ_AX8                        ;   to the last square's floor, the way it goes (not 0: down), what's
ay8         = LZ_AY8                        ;   left of it so far; LN_CACHED's chunk asked of the cache last, ok if
sx          = LZ_SX                         ;   it's there (kok 0: none))
sy          = LZ_SY
n           = LZ_NS
e           = LZ_E
e2          = LZ_E2
air         = LZ_AIR
lz          = LZ_LZ
ad          = LZ_AD
neg         = LZ_NEG
acc         = LZ_ACC
kx          = LZ_KX
kok         = LZ_KOK
ok          = LZ_OK
kt          = r8                            ; (The square's chunk: r8, r9)
pp          = r10                           ; (A portal)
fp          = r11                           ; (A floor)

.code

; LN_LINE
ln_line:
            lda         #0
            sta         LZ_N
            lda         #1
            sta         LZ_LAST
            ldx         #2                  ; How far, each way, and the way
            jsr         span
            sta         ax8
            sty         sx
            ldx         #0
            jsr         span
            sta         ay8
            sty         sy
            cmp         ax8                 ; Its steps (none: clear)
            bcs         @n
            lda         ax8
@n:         sta         n
            tax                             ; (Z: none; cmp's Z is equal's)
            jeq         @clear
            lda         ax8                 ; Its error: |dx| - |dy|
            sec
            sbc         ay8
            sta         e
            lda         #0
            sbc         #0
            sta         e + 1
            lda         lmode
            and         #3
            eor         #LN_AIR
            sta         air                 ; (0: the air's)
            ldy         #4                  ; Its first square (none: stopped)
            jsr         args
            jsr         sq_at
            jsr         keep
            jeq         stop
            stz         kok
            bit         lmode               ; (LN_CACHED: its last square's chunk in the cache, or stopped)
            bpl         @made
            ldy         #0
            jsr         args
            jsr         ck_at
            tax
            jeq         stop
@made:
            lda         air                 ; The line's height: from the eyes to the last square's floor
            beq         @step
            jsr         floor
            clc
            adc         LZ_EYE
            sta         lz
            txa
            adc         #0
            sta         lz + 1
            ldy         #0
            jsr         args
            jsr         sq_at
            jsr         keep
            beq         @ad                 ; (None: 0, .A .X)
            jsr         floor
@ad:        sec                             ; (Its rise or fall, and which way)
            sbc         lz
            sta         ad
            txa
            sbc         lz + 1
            sta         ad + 1
            stz         neg
            stz         acc
            stz         acc + 1
            bpl         @step
            inc         neg
            lda         #0
            sec
            sbc         ad
            sta         ad
            lda         #0
            sbc         ad + 1
            sta         ad + 1

@step:      lda         e                   ; A step: e2 = 2e; along if e2 >= -|dy|, across if e2 <= |dx|
            asl         a
            sta         e2
            lda         e + 1
            rol         a
            sta         e2 + 1
            lda         e2                  ; (e2 + |dy|: not below 0, along)
            clc
            adc         ay8
            lda         e2 + 1
            adc         #0
            bmi         @across
            lda         e
            sec
            sbc         ay8
            sta         e
            bcs         @x
            dec         e + 1
@x:         ldx         #6
            lda         sx
            jsr         move
@across:    lda         ax8                 ; (|dx| - e2: not below 0, across)
            sec
            sbc         e2
            lda         #0
            sbc         e2 + 1
            bmi         @there
            lda         e
            clc
            adc         ax8
            sta         e
            bcc         @y
            inc         e + 1
@y:         ldx         #4
            lda         sy
            jsr         move
@there:     ldx         #3                  ; The last square: clear
@cmp:       lda         xy + 4,x
            cmp         xy,x
            bne         @on
            dex
            bpl         @cmp
@clear:     lda         #0
            tax
            rts
@on:        inc         LZ_N
            bit         lmode               ; (LN_CACHED: its chunk in the cache, asked once a chunk)
            bpl         @pass
            jsr         chunk
            lda         ok
            jeq         stop
@pass:
            jsr         pass                ; What it lets through (none: stopped)
            tax
            jeq         stop
            sta         LZ_LAST
            ldx         LZ_N                ; (The first's too)
            dex
            bne         @h
            sta         LZ_FIRST
@h:         lda         air
            beq         @way
            lda         acc                 ; The line's height a step on: (ad * k) / n, a step at a time
            clc
            adc         ad
            sta         acc
            lda         acc + 1
            adc         ad + 1
            sta         acc + 1
@dda:       lda         acc
            sec
            sbc         n
            tax
            lda         acc + 1
            sbc         #0
            bcc         @floor
            stx         acc
            sta         acc + 1
            lda         neg
            bne         @down
            inc         lz
            bne         @dda
            inc         lz + 1
            bra         @dda
@down:      lda         lz
            bne         @dec
            dec         lz + 1
@dec:       dec         lz
            bra         @dda
@floor:     jsr         floor               ; A floor above the line: stopped
            sta         e2                  ; (e2 free till the next step)
            stx         e2 + 1
            lda         lz
            cmp         e2
            lda         lz + 1
            sbc         e2 + 1
            bvc         @sign
            eor         #$80
@sign:      jmi         stop
@way:       lda         LZ_WAY + 1          ; Asked of the square, if anything's to be (the task's: ln_way)
            jeq         @step
            lda         LZ_N
            jsr         @call
            tax
            jeq         @step
            lda         #LN_WAY
            ldx         #0
            rts
@call:      jmp         (LZ_WAY)

stop:       lda         #LN_STOP
            ldx         #0
            rts

; How far the last square is from the square, the way .X says (0: y, 2: x): .A (.Y the way: 1, or $FF back); more
; than 255: stopped (the caller's caller's return)
span:       lda         xy,x
            sec
            sbc         xy + 4,x
            pha
            lda         xy + 1,x
            sbc         xy + 5,x
            bmi         @back
            bne         @far
            pla
            ldy         #1
            rts
@back:      inc         a
            bne         @far
            pla
            eor         #$FF
            inc         a
            beq         @far1               ; (-256)
            ldy         #$FF
            rts
@far:       pla
@far1:      pla                             ; (span's return, then ln_line's caller's)
            pla
            bra         stop

; The square a step on, the way .X says (4: y, 6: x), .A (1, or $FF back)
move:       dec         a
            beq         @fwd
            lda         xy,x
            bne         @lo
            dec         xy + 1,x
@lo:        dec         xy,x
            rts
@fwd:       inc         xy,x
            bne         @r
            inc         xy + 1,x
@r:         rts

; lp's square's floor (the chunk's elev, CW * CW after its tile), in .A .X
floor:      lda         lp
            sta         fp
            lda         lp + 1
            clc
            adc         #4
            sta         fp + 1
            ldx         #0
            lda         (fp)
            bpl         @p
            dex
@p:         rts

; The square's chunk in the cache (ok), asked once a chunk (its place from the stratum's x0, y0, at ()'s: kt, the
; low bytes' top three bits and the high bytes; kx, ky the last asked)
chunk:      lda         xy + 6
            sec
            ldy         #ATS_X0
            sbc         (LZ_ATS),y
            and         #$E0
            sta         kt
            lda         xy + 7
            iny
            sbc         (LZ_ATS),y
            sta         kt + 1
            lda         xy + 4
            sec
            iny
            sbc         (LZ_ATS),y
            and         #$E0
            sta         kt + 2
            lda         xy + 5
            iny
            sbc         (LZ_ATS),y
            sta         kt + 3
            ldx         #3
@k:         lda         kt,x
            cmp         kx,x
            bne         @ask
            dex
            bpl         @k
            lda         kok
            bne         @r
@ask:       ldx         #3                  ; (Asked: its place kept)
@c:         lda         kt,x
            sta         kx,x
            dex
            bpl         @c
            ldy         #4
            jsr         args
            jsr         ck_at
            sta         ok
            lda         #1
            sta         kok
@r:         rts
.assert     ATS_Y0 = ATS_X0 + 2 && LZ_KY = LZ_KX + 2 && r9 = r8 + 2, error, "line.s: x0, y0; kx, ky; kt"

; at ()'s and ck_at's arguments: sn and a square (.Y 4: the square it's at; 0: the last) in r0, r1 (x), r2 (y)
args:       lda         lsn
            sta         r0
            stz         r0 + 1
            lda         xy + 2,y
            sta         r1
            lda         xy + 3,y
            sta         r1 + 1
            lda         xy,y
            sta         r2
            lda         xy + 1,y
            sta         r2 + 1
            rts

; at ()'s pointer kept (lp): Z set if it's none
keep:       sta         lp
            stx         lp + 1
            ora         lp + 1
            rts

; What the square it's at lets through, for a line of lmode's (no square, rock, a wall: 0; a door's or a window's,
; its portal's: ln_portal's); lp at ()'s pointer to it
pass:       ldy         #4
            jsr         args
            jsr         sq_at
            jsr         keep
            beq         @r
            lda         (lp)
            and         #$0F                ; (TILE's)
            beq         @r                  ; (T_ROCK)
            cmp         #T_WALL
            beq         @no
            cmp         #T_DOOR
            beq         @portal
            cmp         #T_WINDOW
            bne         @yes
@portal:    jsr         sq_door
            sta         pp
            stx         pp + 1
            ora         pp + 1
            bne         rule
@yes:       lda         #1
@r:         rts
@no:        lda         #0
            rts

; LN_PASS: square LZ_LX, LZ_LY of LZ_SN, for a line of LZ_MODE's
ln_pass:
            jsr         pass
            ldx         #0
            rts

; LN_PORTAL: portal r0, for a line of LZ_MODE's
ln_portal:
            lda         r0
            sta         pp
            lda         r0 + 1
            sta         pp + 1
            ; (fall through)

; Portal pp for a line of lmode's: 0 it stops it, 1 it lets it through, 2 narrowly (in .A, .X 0)
rule:       ldx         #0
            ldy         #P_KIND
            lda         (pp),y
            cmp         #PK_PORTCULLIS      ; (Bars, up or down)
            beq         @one
            cmp         #PK_GRATE
            beq         @one
            cmp         #PK_SLIT
            beq         @two
            cmp         #PK_WINDOW
            bne         @door
            ldy         #P_PARTS            ; (A window: its shutters closed nothing, its glass the eye alone)
            lda         (pp),y
            and         #PP_SHUTTERS
            beq         @glass
            jsr         state
            beq         @zero
@glass:     ldy         #P_PARTS
            lda         (pp),y
            and         #PP_GLASS
            beq         @two
            lda         lmode
            and         #3
            beq         @two                ; (LN_SIGHT)
@zero:      lda         #0
            rts
@door:      jsr         state               ; (A door open, or broken down)
            beq         @zero
@one:       lda         #1
            rts
@two:       lda         #2
            rts
state:      ldy         #P_STATE
            lda         (pp),y
            and         #PS_OPEN | PS_WRECKED
            rts
