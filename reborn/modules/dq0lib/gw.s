; ****************************************************************************
; gw.s - (dq0lib) the one give-way (ONE-PATH.md's 2.6, the sense audit's 3.5, GAME-CRITIQUE.md's E7: 2026-11-19): a
; task's work held back while the UI's command is in hand or a line is being typed, at the real paces, on real ticks,
; for every task by one rule.  It was four copies with their own state and three amounts on two clocks: the world's
; den_held and ui_first (50 ticks a command, 600 a line's keys), the minds' held (30 tenths of the game's clock, 600
; ticks), the senses' sense_held (100, 600) and the maker's giving (100, 600).
;
; GW_HELD: r0 the caller's state (games/dq0/hub.h's struct gw, its own RAM: a line's hold its own), r1's low byte its
; flags (hub.h's GW_*).  The hub's header in the window (the caller's hub_map (0)).
;   The turns pace (GW_PACE: the hub's pace, a long wait held as well; else its tpr 0, a long wait the turns pace's
;   too), or nothing in hand (ui_cmd, and ui_keys unless GW_NOKEYS): 0, the hold over.
;   A line its own (the parser's request's seq, the UI's ui_cmd and its keys' number ui_keys, GW_NOKEYS too: a line
;   typed again, its command's answer begun, a new line's first key before its seq's posted, each its own hold): held from the first time it's asked, GW_CMD ticks
;   while a command's in hand, GW_KEYS while its keys go on; then 0 till the next line.  GW_LOOK: a look one way's
;   rest awaited (the hub's way_want) in hand as a command is; GW_ALL: held as long as anything's in hand, no limit
;   (the world's time believed: speed6's, 2026-11-19, its pass run at the next that's free).
;   GW_REST: at the real paces (tpr not 0), a look one way's rest awaited (the maker's chunk: speed7's, the
;   denizens' intents not taken beside it): held, no limit, the state as it was; GW_REST alone, that test only (0 else).
; OUT: .A 1 held, 0 not; .X 0.  Modifies .A, .X, .Y, r2, r3

.include "hydra.inc"
            .include    "dq0hub_gw.inc"         ; (The headers' numbers: the build's, games/dq0/hubgen.js)

            .export     gw_held
            .macpack    longbranch


GS_ST       = 0                                 ; (struct gw's: the hold's state, 0 none, 1 since t0, 2 run out;
GS_SQ       = 1                                 ;   its line's seq, ui_cmd and ui_keys; its start, the ticks')
GS_CM       = 2
GS_KS       = 3
GS_T0       = 4

.code

gw_held:
            lda         r1                              ; A look's rest awaited at the real paces: held (GW_REST)
            and         #GW_REST
            beq         @go
            lda         HUB_WINDOW + HUBO_TPR
            ora         HUB_WINDOW + HUBO_TPR + 1
            beq         @norest
            lda         HUB_WINDOW + HUBO_WWST
            jne         @held
@norest:
            lda         r1
            cmp         #GW_REST
            jeq         @over                           ; (That test alone: not held, the state untouched)
@go:
            lda         r1                              ; The turns pace: no hold
            and         #GW_PACE
            beq         @tpr
            lda         HUB_WINDOW + HUBO_PACE
            and         #3
            cmp         #3
            jeq         @none
            bra         @real
@tpr:
            lda         HUB_WINDOW + HUBO_TPR
            ora         HUB_WINDOW + HUBO_TPR + 1
            jeq         @none
@real:
            lda         r1                              ; r2 the keys (none: GW_NOKEYS), r3 ui_cmd
            and         #GW_NOKEYS
            beq         :+
            lda         #0
            bra         :++
:
            lda         HUB_WINDOW + HUBO_KEYS
:
            sta         r2
            lda         HUB_WINDOW + HUBO_CMD
            sta         r3
            lda         r1                              ; (A look's rest awaited: in hand, GW_LOOK)
            and         #GW_LOOK
            beq         :+
            lda         HUB_WINDOW + HUBO_WWST
            beq         :+
            lda         #$80
            tsb         r3
:
            lda         r3
            ora         r2
            jeq         @none                           ; (Nothing in hand)
            ldy         #GS_SQ                          ; A line of its own: the hold begun again
            lda         HUB_WINDOW + HUBO_CQ + RQ_SEQ
            cmp         (r0),Y
            bne         @line
            iny
            lda         r3
            cmp         (r0),Y
            bne         @line
            iny
            lda         HUB_WINDOW + HUBO_KEYS          ; (The line's keys' own number, GW_NOKEYS too: a new
            cmp         (r0),Y                          ;   line's first key its own hold, its seq not yet posted)
            beq         @same
@line:
            ldy         #GS_SQ
            lda         HUB_WINDOW + HUBO_CQ + RQ_SEQ
            sta         (r0),Y
            iny
            lda         r3
            sta         (r0),Y
            iny
            lda         HUB_WINDOW + HUBO_KEYS
            sta         (r0),Y
            lda         #0
            sta         (r0)                            ; (GS_ST)
@same:
            lda         (r0)                            ; GS_ST
            bne         @running
            lda         #1                              ; Begun now
            sta         (r0)
            jsr         TICKS                           ; (.A its low byte, .X its high)
            phx
            ldy         #GS_T0
            sta         (r0),Y
            iny
            pla
            sta         (r0),Y
            bra         @held
@running:
            cmp         #1
            bne         @over                           ; (Run out: on, till the next line)
            lda         r1                              ; (No limit: held while anything's in hand)
            and         #GW_ALL
            bne         @held
            jsr         TICKS                           ; Run out yet: the ticks since t0 against GW_CMD or GW_KEYS
            phx
            sec
            ldy         #GS_T0
            sbc         (r0),Y
            sta         r2
            pla
            iny
            sbc         (r0),Y
            sta         r2 + 1
            lda         r3
            beq         @keys
            lda         r2
            cmp         #<GW_CMD
            lda         r2 + 1
            sbc         #>GW_CMD
            bcc         @held
            bra         @out
@keys:
            lda         r2
            cmp         #<GW_KEYS
            lda         r2 + 1
            sbc         #>GW_KEYS
            bcc         @held
@out:
            lda         #2
            sta         (r0)
@over:
            lda         #0
            tax
            rts
@none:
            lda         #0                              ; Nothing held: the state none
            sta         (r0)
            tax
            rts
@held:
            lda         #1
            ldx         #0
            rts
