; ****************************************************************************
; dice.inc - (the referees': referee.h's dice_key, a referee's .s includes it) the keyed dice (phase C, dice2,
; 2026-11-24): a request's dice a function of its key alone, so no roll elsewhere moves them.  The key: the game's
; seed and its dice offset (the hub's dice, dq0 -R's), the asker's slot (the world's 0, the UI's 1, the denizens' 2:
; DSLOT), the request's kind, actor, other, a (a test's purpose, a blow's aim, a harm's cause) and b (a test's how),
; and its moment: the hub's clock (now), a blow's its own (RS_ATTACK's when, the world's: a denizen's intent's
; moment), a creature's sheet (RS_NEW_ACTOR) its record's number in place of a moment and of its place in the hub's
; table (the same creature the same sheet whenever it's met); and n, how many requests at that moment had the key's
; tag (3 bits of it) before it, so two requests are never one roll unless they are the same act at the same moment (a
; search's every hidden one, the denizens' every look: G14's quiet dice).  Hashed into xs_seed (dice.h's generator),
; its dice the generator's from there: four Pearson lanes (a table of 0-255 shuffled; each byte through the four in
; turn, each lane T[itself ^ the byte as the lane before left it], the first after the last), then two bytes more, so
; every bit of the key reaches every bit of the state, and not linearly (xorshift's own step is linear: two keys a
; fixed mask apart would roll dice a fixed mask apart).  What it keeps is the hub's (hub.h's struct dkey, the
; referee's): the lanes as the game's seed and dice offset leave them (base, once), the counts' moment and the counts, and own: 1 a test's own stream (RS_DICE's seed, t_rules), no keys
; for the rest of the referee's life; 2 the last request rigged.  In assembly, and in an overlay if the referee says
; (DICE_OVL n, its overlay's number, and .define DICE_SEG "OVnCODE", DICE_RO "OVnRODATA" before including it: its
; stub and its reader and writer of the hub here, the main RAM's; the overlay reaches the hub through them, the hub's
; bank the one the stub found, ovstk's top)
;
;   void __fastcall__ dice_key (unsigned char s);   slot s's request (0 and 1 the hub's slot[], NSLOTS DSLOT): its
;                                                   key hashed into xs_seed
;
; And a test's rigged dice (DICE_RIG, a referee that has them: hub.h's struct rig, set by its scene): the first entry
; that matches the request has its first dice show its faces (the referee's RS_DICE hook, fixed), the rest the key's

            .import     _xs_seed
            .importzp   ptr1, ptr2, ptr3, ptr4, tmp1, tmp2, tmp3, sreg
            .export     _dice_key
            .include    "dq0hub_dice.inc"   ; (The hub's places, the request's, the kinds': the build's, hubgen.js)

.ifdef DICE_OVL
            .code
_dice_key:  jsr         ovcall              ; (Its stub, ovcall's: the overlay, the function)
            .byte       DICE_OVL
            .addr       dice_entry

; .A = byte .Y of ptr1, the hub's (its bank sreg: the one the stub found there, ovstk's top), the window put back
dk_rd:      ldx         BANKREG
            lda         sreg
            sta         BANKREG
            lda         (ptr1),y
            stx         BANKREG
            rts

; Byte .Y of ptr1, the hub's, = .A, the window put back
dk_wr:      ldx         BANKREG
            pha
            lda         sreg
            sta         BANKREG
            pla
            sta         (ptr1),y
            stx         BANKREG
            rts

            .segment    DICE_SEG
dice_entry: ldx         ovsp                ; (The hub's bank, the one the stub found: dk_rd's)
            ldy         ovstk - 1,x
            sty         sreg
.else
            .code
dk_rd:      lda         (ptr1),y
            rts
dk_wr:      sta         (ptr1),y
            rts
_dice_key:
.endif
            sta         tmp1                ; tmp1: the slot
            jsr         dk_keep             ; (1: a test's own stream, no keys)
            ldy         #DK_OWN
            jsr         dk_rd
            cmp         #1
            bne         :+
            rts
:           lda         tmp1                ; ptr2: its request (DSLOT, slot[0] or slot[1])
            cmp         #NSLOTS
            bcc         @slot
            ldx         #<(HUB_WINDOW + HUBO_DSLOT)
            ldy         #>(HUB_WINDOW + HUBO_DSLOT)
            bra         @at
@slot:      ldx         #<(HUB_WINDOW + HUBO_SLOT)
            ldy         #>(HUB_WINDOW + HUBO_SLOT)
            lsr         a
            bcc         @at
            ldx         #<(HUB_WINDOW + HUBO_SLOT + REQ_SIZE)
            ldy         #>(HUB_WINDOW + HUBO_SLOT + REQ_SIZE)
@at:        stx         ptr2
            sty         ptr2 + 1
            jsr         dk_keep             ; The lanes (xs_seed) begun: the game's, kept (base)
            ldy         #DK_BASE + 3
:           jsr         dk_rd
            sta         _xs_seed - DK_BASE,y
            dey
            cpy         #DK_BASE
            bcs         :-
            lda         _xs_seed
            ora         _xs_seed + 1
            ora         _xs_seed + 2
            ora         _xs_seed + 3
            bne         @based
            lda         #$5A                ; (None yet: the lanes begun, the game's dice offset and its seed
            sta         _xs_seed            ;   fed, kept)
            lda         #$C3
            sta         _xs_seed + 1
            lda         #$96
            sta         _xs_seed + 2
            lda         #$3C
            sta         _xs_seed + 3
            lda         #<(HUB_WINDOW + HUBO_DICE)
            ldx         #>(HUB_WINDOW + HUBO_DICE)
            jsr         dk_at
            ldy         #0
            jsr         dk_rd
            jsr         dk_feed
            lda         #<(HUB_WINDOW + HUBO_SEED)
            ldx         #>(HUB_WINDOW + HUBO_SEED)
            jsr         dk_at
            jsr         dk_get
            jsr         dk_moment
            jsr         dk_keep
            ldy         #DK_BASE + 3
:           lda         _xs_seed - DK_BASE,y
            jsr         dk_wr
            dey
            cpy         #DK_BASE
            bcs         :-
@based:
            lda         tmp1                ; The slot
            jsr         dk_feed
            lda         ptr2                ; The request's kind, actor, other, a and b
            ldx         ptr2 + 1
            jsr         dk_at
            ldy         #RQ_KIND
            jsr         dk_rd
            sta         tmp1                ; (tmp1 now its kind)
            jsr         dk_feed
            ldy         #RQ_ACTOR
            jsr         dk_rd
            sta         tmp2                ; (tmp2: its actor)
            ldx         tmp1
            cpx         #RS_NEW_ACTOR
            bne         :+
            lda         #0                  ; (A creature's sheet: not by its place in the hub's table)
:           jsr         dk_feed
            ldy         #RQ_OTHER
            jsr         dk_rd
            jsr         dk_feed
            ldy         #RQ_A
            jsr         dk_rd
            sta         tmp3                ; (tmp3: its a)
            jsr         dk_feed
            ldy         #RQ_B
            jsr         dk_rd
            jsr         dk_feed
            lda         tmp1                ; The moment: a creature's sheet its record's (w1, | $FFFF0000)
            cmp         #RS_NEW_ACTOR
            bne         @blow
            ldy         #RQ_W1
            jsr         dk_rd
            sta         ptr3
            ldy         #RQ_W1 + 1
            jsr         dk_rd
            sta         ptr3 + 1
            lda         #$FF
            sta         ptr4
            sta         ptr4 + 1
            bra         @time
@blow:      cmp         #RS_ATTACK          ; (A blow's its own when, x[4..7], if it's set)
            bne         @now
            ldy         #RQ_X + 4 + 3
            ldx         #3
:           phx
            jsr         dk_rd
            plx
            sta         ptr3,x
            dey
            dex
            bpl         :-
            ora         ptr3 + 1
            ora         ptr4
            ora         ptr4 + 1
            bne         @time
@now:       lda         #<(HUB_WINDOW + HUBO_NOW)       ; (The hub's clock)
            ldx         #>(HUB_WINDOW + HUBO_NOW)
            jsr         dk_at
            jsr         dk_get
@time:      jsr         dk_keep             ; (Another moment than the counts': them begun again)
            ldy         #DK_T
            jsr         dk_rd
            cmp         ptr3
            bne         :+
            iny
            jsr         dk_rd
            cmp         ptr3 + 1
            beq         @fed
:           ldy         #DK_T
            lda         ptr3
            jsr         dk_wr
            iny
            lda         ptr3 + 1
            jsr         dk_wr
            ldy         #DK_N + 7
:           lda         #0
            jsr         dk_wr
            dey
            cpy         #DK_N
            bcs         :-
@fed:       jsr         dk_moment
            lda         _xs_seed + 3        ; n: the count of this tag's at this moment, then one more
            and         #7
            clc
            adc         #DK_N
            tay
            jsr         dk_rd
            pha
            inc         a
            beq         :+                  ; (255 at most)
            jsr         dk_wr
:           pla
            jsr         dk_feed
            ldx         #2                  ; Two bytes more
:           phx
            lda         #0
            jsr         dk_feed
            plx
            dex
            bne         :-
            lda         _xs_seed            ; (Never 0: xorshift's one value it can't leave)
            ora         _xs_seed + 1
            ora         _xs_seed + 2
            ora         _xs_seed + 3
            bne         :+
            inc         _xs_seed
:
.ifdef DICE_RIG
            jmp         dk_rig
.else
            rts
.endif

.ifdef DICE_RIG
; The rigged dice (a test's: the hub's rig, its scene's, hub.h's struct rig): the first entry that matches the
; request (its kind, tmp1; its a, tmp3, or RIG_ANY; its actor, tmp2, or RIG_ANY) with uses left (RIG_ANY every time:
; its used counted, the test's 0 as it sets it) has its first dice show the entry's faces (the referee's RS_DICE
; hook, fixed: its faces dface, the next dk, after them drep, 0 the generator's), the rest as the key has them; the
; hook off again at the next request
            .import     _dice_hook, _dface, _dk, _drep  ; (_fixed: the referee's parts.inc's stub, before this)
dk_rig:     jsr         dk_keep             ; (The last request rigged: the hook off)
            ldy         #DK_OWN
            jsr         dk_rd
            tax
            beq         :+
            stz         _dice_hook
            stz         _dice_hook + 1
            lda         #0
            jsr         dk_wr
:           lda         #<(HUB_WINDOW + HUBO_RIG + RIG_E)   ; ptr1 the entry
            ldx         #>(HUB_WINDOW + HUBO_RIG + RIG_E)
            jsr         dk_at
            lda         #RIG_N
            sta         ptr4                ; (ptr4: the entries left)
@e:         ldy         #RE_KIND
            jsr         dk_rd
            tax
            beq         @none               ; (The table's end)
            cmp         tmp1
            bne         @next
            ldy         #RE_A
            jsr         dk_rd
            cmp         #RIG_ANY
            beq         :+
            cmp         tmp3
            bne         @next
:           ldy         #RE_ACTOR
            jsr         dk_rd
            cmp         #RIG_ANY
            beq         :+
            cmp         tmp2
            bne         @next
:           ldy         #RE_USED
            jsr         dk_rd
            sta         ptr4 + 1
            ldy         #RE_USES
            jsr         dk_rd
            cmp         #RIG_ANY
            beq         @rig
            cmp         ptr4 + 1
            beq         @next               ; (Its uses spent)
            bcc         @next
            lda         ptr4 + 1
            inc         a
            ldy         #RE_USED
            jsr         dk_wr
@rig:       ldx         #0                  ; (Its faces, then the generator's)
            ldy         #RE_FACE
:           phx
            jsr         dk_rd
            plx
            sta         _dface,x
            iny
            inx
            cpx         #4
            bne         :-
            stz         _dface + 4
            stz         _dk
            stz         _drep
            lda         #<_fixed
            sta         _dice_hook
            lda         #>_fixed
            sta         _dice_hook + 1
            jsr         dk_keep
            ldy         #DK_OWN
            lda         #2
            jmp         dk_wr
@next:      lda         ptr1
            clc
            adc         #RE_SIZE
            sta         ptr1
            bcc         :+
            inc         ptr1 + 1
:           dec         ptr4
            beq         @none
            jmp         @e
@none:      rts
.endif

; ptr1 the hub's struct dkey (what the keys keep)
dk_keep:    lda         #<(HUB_WINDOW + HUBO_DKEY)
            ldx         #>(HUB_WINDOW + HUBO_DKEY)

; ptr1 = .A, .X
dk_at:      sta         ptr1
            stx         ptr1 + 1
            rts

; Four bytes from ptr1 into ptr3, ptr4, low byte first
dk_get:     ldy         #3
:           jsr         dk_rd
            sta         ptr3,y
            dey
            bpl         :-
            rts

; ptr3, ptr4's four bytes fed, low byte first
dk_moment:  ldx         #0
:           phx
            lda         ptr3,x
            jsr         dk_feed
            plx
            inx
            cpx         #4
            bne         :-
            rts

; .A fed through the four lanes, each in turn: c = .A ^ the last lane; each lane T[its own ^ c], c it
dk_feed:    eor         _xs_seed + 3
            ldx         #0
:           eor         _xs_seed,x
            tay
            lda         dk_perm,y
            sta         _xs_seed,x
            inx
            cpx         #4
            bne         :-
            rts

.ifdef DICE_OVL
            .segment    DICE_RO
.else
            .rodata
.endif
dk_perm:                                    ; (0-255 shuffled, a fixed shuffle: Pearson's table, its own)
            .byte       $49, $03, $A4, $26, $A6, $17, $9C, $94, $DF, $4D, $C6, $05, $B3, $C5, $13, $F7
            .byte       $B2, $A7, $0A, $0C, $12, $51, $21, $3C, $29, $DA, $1D, $BF, $F8, $86, $67, $A8
            .byte       $4A, $71, $C2, $40, $FA, $37, $02, $34, $57, $44, $AC, $09, $10, $6D, $31, $3D
            .byte       $E2, $5A, $89, $CA, $01, $BA, $62, $2C, $6E, $1E, $36, $CF, $00, $BE, $FB, $63
            .byte       $75, $AD, $5F, $98, $47, $7A, $CE, $56, $87, $32, $C7, $D5, $64, $E7, $E6, $4F
            .byte       $D2, $9E, $30, $F0, $C9, $F2, $F9, $D6, $BC, $8C, $9F, $F4, $38, $7C, $22, $ED
            .byte       $0F, $2D, $E8, $D9, $55, $42, $C4, $E0, $3B, $8B, $7E, $8F, $3F, $F6, $E5, $04
            .byte       $7B, $7D, $08, $48, $73, $5B, $1C, $06, $2A, $EC, $A9, $4E, $6A, $C0, $FF, $D8
            .byte       $5E, $F3, $BB, $39, $DD, $76, $E3, $46, $43, $A5, $AA, $07, $11, $61, $66, $2F
            .byte       $18, $B1, $81, $CB, $20, $A0, $97, $35, $82, $95, $AF, $9A, $83, $1F, $84, $9B
            .byte       $F5, $1B, $60, $80, $68, $9D, $96, $DE, $B5, $6C, $3A, $B6, $69, $AE, $DC, $85
            .byte       $90, $5C, $74, $93, $E9, $6F, $50, $EB, $28, $52, $C3, $E1, $5D, $70, $8E, $53
            .byte       $D7, $B8, $14, $FD, $CD, $B9, $B0, $27, $92, $A3, $15, $D0, $58, $EF, $BD, $AB
            .byte       $41, $8D, $A1, $E4, $0E, $25, $EA, $45, $65, $F1, $CC, $FE, $0B, $DB, $2B, $4B
            .byte       $99, $88, $6B, $C1, $72, $8A, $D1, $54, $33, $3E, $77, $23, $FC, $59, $4C, $1A
            .byte       $16, $B4, $2E, $B7, $D3, $C8, $79, $91, $EE, $D4, $0D, $7F, $78, $24, $A2, $19
