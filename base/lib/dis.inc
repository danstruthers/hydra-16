; ****************************************************************************
; dis.inc - the disassembler, the base's monitor's (modules/wozmon) and HydraOS's asm library's (spec/asm.def's
; DIS): an instruction as as writes it, from w65c02.inc's table.  Lower case; an operand's address in hex ($nn on the
; zero page, $nnnn else) or the name the caller gives for it; a: before an absolute address under $100 where the name
; has a zero page form (as would take that); an opcode the W65C02S doesn't define, its bytes as .byte.  So a line's
; text, assembled, is its bytes again.  Its scratch is the call registers (r5-r9), so it needs no RAM of its own.

; Its flags and kinds (spec/asm.def's, HydraOS's: its asmlib.inc defines them first; the base's monitor, without it,
; these)
.ifndef DF_PAD
DF_PAD          = $01          ; Its name padded to 12 columns, else one space
DK_ADDR         = $01          ; Kinds: its operand is an address (r4) ...
DK_GO           = $02          ;   it goes there (a jump, jsr, a branch) ...
DK_CALL         = $04          ;   a jsr ...
DK_END          = $08          ;   nothing after it is reached by going on from it ...
DK_UNDEF        = $10          ;   an opcode the W65C02S doesn't define
.endif

d_flags         = r5            ; The flags (DF_)
d_op            = r5 + 1        ; The opcode ...
d_mode          = r6            ;   its mode ...
d_kind          = r6 + 1        ;   its kind (DK_) ...
d_len           = r7            ;   its length ...
d_ti            = r7 + 1        ;   (the text's length so far, while a name's copied)
d_b1            = r8            ;   and its operand's bytes
d_b2            = r8 + 1
d_t             = r9            ; (Scratch)
NAME_MAX        = 40            ; A name's characters, at most (the rest cut)

; DIS: r0 = the bytes, r1 = their address, r2 = the text's place, r3 = a name for the operand's address (0: none), .A
; = the flags.  OUT: .A = its length, .X = its kind, .Y = the text's length (a 0 after it), r4 = the address its operand
; names (where a branch goes)
as_dis:
            sta         d_flags
            lda         (r0)
            sta         d_op
            ldy         #1
            lda         (r0),y
            sta         d_b1
            iny
            lda         (r0),y
            sta         d_b2
            ldx         d_op
            lda         op_mode,x
            sta         d_mode
            tax
            lda         modelen,x
            sta         d_len
            stz         d_kind
            stz         r4
            stz         r4 + 1
            ldy         #0                                  ; (.Y: the text's length, throughout)
            lda         d_mode
            cmp         #MODES
            bcc         :+
            jmp         dis_undef
:
            ldx         d_op                                ; Its name: 3 letters ...
            lda         op_name,x
            sta         d_t
            asl         a                                   ; (C = 0: 70 at most)
            adc         d_t
            tax
            lda         names,x
            jsr         dis_put
            lda         names + 1,x
            jsr         dis_put
            lda         names + 2,x
            jsr         dis_put
            lda         d_op                                ;   and RMB's, SMB's, BBR's and BBS's bit
            and         #7
            cmp         #7
            bne         :+
            lda         d_op
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            and         #7
            ora         #'0'
            jsr         dis_put
:
            lda         d_mode                              ; Its operand, by its mode
            bne         :+
            jmp         dis_end
:
            jsr         dis_sep
            lda         d_mode
            asl         a
            tax
            jmp         (dis_modes,x)
dis_modes:  .word       dis_end, dis_acc, dis_imm, dis_zp, dis_zp, dis_zp, dis_izx, dis_izy, dis_izp
            .word       dis_abs, dis_abs, dis_abs, dis_ind, dis_iax, dis_rel, dis_zpr

dis_acc:
            lda         #'a'
            jsr         dis_put
            jmp         dis_end
dis_imm:
            lda         #'#'
            jsr         dis_put
            lda         d_b1
            jsr         dis_byte
            jmp         dis_end
dis_zp:
            jsr         dis_zaddr
            jsr         dis_index                           ; (,x ,y: zp,x zp,y)
            jmp         dis_end
dis_izx:
            jsr         dis_open
            jsr         dis_zaddr
            lda         #','
            jsr         dis_put
            lda         #'x'
            jsr         dis_put
            bra         dis_close
dis_izy:
            jsr         dis_open
            jsr         dis_zaddr
            lda         #')'
            jsr         dis_put
            lda         #','
            jsr         dis_put
            lda         #'y'
            jsr         dis_put
            jmp         dis_end
dis_izp:
            jsr         dis_open
            jsr         dis_zaddr
            bra         dis_close
dis_abs:
            jsr         dis_aaddr
            lda         d_op
            cmp         #$20                                ; (jsr, jmp: where it goes)
            beq         @call
            cmp         #$4C
            beq         @go
            jsr         dis_absz                            ; (a: if as would take the zero page form)
            jsr         dis_addr4
            jsr         dis_index
            jmp         dis_end
@call:
            lda         #DK_CALL
            tsb         d_kind
@go:
            lda         #DK_ADDR
            trb         d_kind
            lda         #DK_GO
            tsb         d_kind
            jsr         dis_addr4
            jmp         dis_end
dis_ind:
            jsr         dis_open
            jsr         dis_aaddr
            jsr         dis_addr4
            bra         dis_close
dis_iax:
            jsr         dis_open
            jsr         dis_aaddr
            jsr         dis_addr4
            lda         #','
            jsr         dis_put
            lda         #'x'
            jsr         dis_put
dis_close:
            lda         #')'
            jsr         dis_put
            ; (fall through)

; The end: its kind's DK_END if nothing after it is reached through it; the text's 0
dis_end:
            ldx         #dis_ends_n - 1
            lda         d_op
:
            cmp         dis_ends,x
            beq         :+
            dex
            bpl         :-
            bra         @kind
:
            lda         #DK_END
            tsb         d_kind
@kind:
            lda         #0
            sta         (r2),y
            lda         d_len
            ldx         d_kind
            rts
dis_ends:   .byte       $00, $40, $60, $DB, $4C, $6C, $7C, $80 ; brk rti rts stp jmp jmp ( ) jmp ( ,x) bra
dis_ends_n  = * - dis_ends

dis_rel:
            lda         d_b1                                ; (Where it goes: after it, + the offset)
            ldx         #2
            jsr         dis_target
            jsr         dis_addr4
            jmp         dis_end
dis_zpr:
            lda         d_b1                                ; (bbr, bbs: the zero page's byte, then where)
            jsr         dis_byte
            lda         #','
            jsr         dis_put
            lda         #' '
            jsr         dis_put
            lda         d_b2
            ldx         #3
            jsr         dis_target
            jsr         dis_addr4
            jmp         dis_end

; An opcode the W65C02S doesn't define: .byte and its bytes (the NOP the chip runs: 1, 2 or 3)
dis_undef:
            lda         #DK_UNDEF
            sta         d_kind
            ldx         #0
:
            lda         s_byte,x
            jsr         dis_put
            inx
            cpx         #5
            bne         :-
            jsr         dis_sep
            lda         d_op
            jsr         dis_byte
            lda         d_len
            cmp         #2
            bcc         @done
            jsr         dis_comma
            lda         d_b1
            jsr         dis_byte
            lda         d_len
            cmp         #3
            bcc         @done
            jsr         dis_comma
            lda         d_b2
            jsr         dis_byte
@done:
            jmp         dis_end
s_byte:     .byte       ".byte"

; ---- Its parts

; .A at the text's end
dis_put:
            sta         (r2),y
            iny
            rts

; ", "
dis_comma:
            lda         #','
            jsr         dis_put
            lda         #' '
            bra         dis_put

; "("
dis_open:
            lda         #'('
            bra         dis_put

; The space between its name and its operand: one, or to column 12 (DF_PAD)
dis_sep:
            lda         #' '
            jsr         dis_put
            lda         d_flags
            and         #DF_PAD
            beq         @rts
:
            cpy         #12
            bcs         @rts
            lda         #' '
            jsr         dis_put
            bra         :-
@rts:
            rts

; ",x" or ",y" after an indexed address (zp,x zp,y abs,x abs,y)
dis_index:
            lda         d_mode
            cmp         #M_ZPX
            beq         @x
            cmp         #M_ABX
            beq         @x
            cmp         #M_ZPY
            beq         @y
            cmp         #M_ABY
            bne         @rts
@y:
            lda         #'y'
            bra         :+
@x:
            lda         #'x'
:
            pha
            lda         #','
            jsr         dis_put
            pla
            jmp         dis_put
@rts:
            rts

; The zero page's address (r4 = the byte; DK_ADDR): its name, or $nn
dis_zaddr:
            lda         d_b1
            sta         r4
            lda         #DK_ADDR
            tsb         d_kind
            lda         r3
            ora         r3 + 1
            bne         dis_name
            lda         #'$'
            jsr         dis_put
            lda         r4
            jmp         dis_hex

; r4 = the absolute address (DK_ADDR)
dis_aaddr:
            lda         d_b1
            sta         r4
            lda         d_b2
            sta         r4 + 1
            lda         #DK_ADDR
            tsb         d_kind
            rts

; r4 = where a branch goes: its address (r1) + .X (its length) + the offset .A (signed); DK_GO
dis_target:
            stx         d_t
            ldx         #0
            ora         #0
            bpl         :+
            dex
:
            clc
            adc         r1
            sta         r4
            txa
            adc         r1 + 1
            sta         r4 + 1
            lda         r4
            clc
            adc         d_t
            sta         r4
            bcc         :+
            inc         r4 + 1
:
            lda         #DK_GO
            tsb         d_kind
            rts

; r4 as an operand: its name, or $nnnn
dis_addr4:
            lda         r3
            ora         r3 + 1
            bne         dis_name
            lda         #'$'
            jsr         dis_put
            lda         r4 + 1
            jsr         dis_hex
            lda         r4
            jmp         dis_hex

; The name at r3 (NAME_MAX characters at most)
dis_name:
            sty         d_ti
            ldy         #0
:
            lda         (r3),y
            beq         :+
            phy
            ldy         d_ti
            sta         (r2),y
            inc         d_ti
            ply
            iny
            cpy         #NAME_MAX
            bne         :-
:
            ldy         d_ti
            rts

; "a:" before an absolute address under $100 (abs, abs,x, abs,y), if the name has the zero page form too (zp, zp,x,
; zp,y): as would take that one for it
dis_absz:
            lda         r4 + 1
            bne         @rts
            lda         d_mode                              ; (The zero page's mode: M_ZP, M_ZPX, M_ZPY)
            sec
            sbc         #M_ABS - M_ZP
            sta         d_t
            ldx         d_op
            lda         op_name,x
            sta         d_t + 1
            ldx         #0
:
            lda         op_name,x                           ; (Any opcode of the same name in that mode)
            cmp         d_t + 1
            bne         @next
            lda         op_mode,x
            cmp         d_t
            beq         @a
@next:
            inx
            bne         :-
@rts:
            rts
@a:
            lda         #'a'
            jsr         dis_put
            lda         #':'
            jmp         dis_put

; "$nn": .A
dis_byte:
            pha
            lda         #'$'
            jsr         dis_put
            pla
; .A as two hex digits
dis_hex:
            pha
            lsr         a
            lsr         a
            lsr         a
            lsr         a
            jsr         @nib
            pla
            and         #$0F
@nib:
            cmp         #10
            bcc         :+
            adc         #6                                  ; (C = 1: 7 more)
:
            adc         #'0'
            jmp         dis_put
