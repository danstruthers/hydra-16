; ****************************************************************************
; vmx.inc - hylang's native code (docs/hylang.md, "The bytecode machine"), in its eighth bank: a function's bytecode,
; as vm_compile made it in its scratch bank (vm_sb, from $8000), made the 65C02's own code in the arena (vm_xlate).
; Each op is a stub that points vm_ip at its data, the op as it was in the bytecode (EXT left out), and jumps to
; its code in the machine (vm.inc's), whose next is jmp (vm_ip): the op's data past, the next op's stub.  The usual
; ops are in line, from a template (vmxt.inc: tools/hyvmxt.js makes it), its operands patched in; one that has a slow way
; (not fixnums, a scope made ...) has its stub after it (TF_STUB), which it jumps to.  Each place
; in the data that's code (a jump's target, a block's parent, the blocks' table, the function's start, a call's
; return) is the native code's.
;   Two passes: each op's stub's place (its entry, in the map bank vm_mb at the op's own place, its high byte $1000
; on), then the stubs, and the blocks' table after them.

VX_STUB         = 11            ; (A stub: lda #<data, sta vm_ip, lda #>data, sta vm_ip + 1, jmp its code)

; The address operands' kinds (vx_ta1 ...: offset * 8 + kind, its offset in the op's data)
VXK_T           = 1             ; A jump's target: its op's entry
VXK_P           = 2             ; A block's parent (0: none): its BLOCK op's data
VXK_B           = 3             ; The blocks' table (0: none): the native one's place
VXK_C           = 4             ; The function's start
VXK_R           = 5             ; A call's return: the place (a fixnum) past the op's data

; A template's flags, its patches' kinds
TF_STUB         = 1             ; Its op's stub after it (its slow way)
TF_NOR          = 2             ; Not for an op with VM_R (its stub alone)
TPK_B           = 1             ; The op's data byte
TPK_U           = 2             ;   untagged (bit 0 clear)
TPK_T           = 3             ; A jump's target in the data: its native place
TPK_N           = 4             ; The next op's entry

.bss
vx_end:     .res        2                                   ; The bytecode's end (its blocks' table there ...
vx_ntbl:    .res        1                                   ;   if it has one: its blocks)
vx_place:   .res        2                                   ; OUT: the native code's place (a fixnum: word 4's)
vx_a0:      .res        2                                   ; The native code: its start ...
vx_nat:     .res        2                                   ;   its end (the table's place) ...
vx_top:     .res        2                                   ;   past its table (the arena's next)
vx_idx:     .res        1                                   ;   its bank's index in the arena (times 32) ...
vx_bank:    .res        1                                   ;   and number
vx_len:     .res        1                                   ; (An op: its bytecode's length ...
vx_dl:      .res        1                                   ;   its data's (an EXT left out) ...
vx_d0:      .res        1                                   ;   its data's start in vx_buf ...
vx_h:       .res        2                                   ;   its code ...
vx_a:       .res        2                                   ;   its address operands ...
vx_k:       .res        1                                   ;   (scratch)
vx_d:       .res        2                                   ;   its data's place)
vx_buf:     .res        48
vx_tp:      .res        2                                   ; (An op's template: its header, 0 if none ...
vx_tln:     .res        1                                   ;   its code's length ...
vx_tfl:     .res        1                                   ;   its flags ...
vx_tnp:     .res        1                                   ;   its patches
vx_tpp:     .res        2
vx_tcode:   .res        2
vx_op:      .res        1                                   ;   its op ...
vx_nx:      .res        2                                   ;   the next op's entry ...
vx_pi:      .res        1                                   ;   its patches left)

.segment "CODE8"

; The function compiled (vm_compile's): its bytecode at $8000 in vm_sb to vx_end (its blocks' table there if vx_ntbl),
; made native code in the arena, at its top (the next bank, from its start, if it isn't room).  OUT: C = 0, vx_place
; its place; C = 1 if it can't be: .A = 0, the arena's full (vm_full); .A = 1, it's too big for a bank
vm_xlate:
@try:
            ldx         ar_cur
            lda         ar_bank,x
            sta         vx_bank
            txa
            asl
            asl
            asl
            asl
            asl
            sta         vx_idx
            lda         ar_top
            sta         vx_a0
            lda         ar_top + 1
            sta         vx_a0 + 1
            jsr         vx_pass1
            bcc         @fits
            lda         vx_a0                               ; (Not room from a bank's start: too big)
            bne         :+
            lda         vx_a0 + 1
            cmp         #$80
            bne         :+
            lda         #1
            sec
            rts
:
            ldx         ar_cur                              ; (The next bank, taken if it isn't yet)
            inx
            cpx         ar_n
            bcc         @have
            cpx         #4
            bcs         @full
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @full
            ldx         ar_n
            sta         ar_bank,x
            inc         ar_n
            ldx         ar_cur
            inx
@have:
            stx         ar_cur
            stz         ar_top
            lda         #$80
            sta         ar_top + 1
            bra         @try
@full:
            lda         #1
            sta         vm_full
            lda         #0
            sec
            rts
@fits:
            jsr         vx_pass2
            jsr         vx_table
            lda         vx_top
            sta         ar_top
            lda         vx_top + 1
            sta         ar_top + 1
            lda         vx_a0                               ; (Its place: vm_encode's)
            asl
            ora         #1
            sta         vx_place
            lda         vx_a0 + 1
            and         #$1F
            ora         vx_idx
            rol
            sta         vx_place + 1
            clc
            rts

; Each op's stub's place (its entry), in the map: vm_mb at the op's place (its low byte), $1000 on (its high);
; vx_nat their end, vx_top the table's.  OUT: C = 1 if they don't fit in the bank (below $A000)
vx_pass1:
            stz         hq
            lda         #$80
            sta         hq + 1
            lda         vx_a0
            sta         ht
            lda         vx_a0 + 1
            sta         ht + 1
@op:
            jsr         vx_atend
            bcs         @end
            jsr         vx_oplen
            lda         vm_mb
            sta         $00
            lda         ht
            sta         (hq)
            lda         hq + 1
            clc
            adc         #$10
            sta         hq + 1
            lda         ht + 1
            sta         (hq)
            lda         hq + 1
            sec
            sbc         #$10
            sta         hq + 1
            jsr         vx_size
            jsr         vx_htadd
            lda         vx_len
            jsr         vx_hqadd
            lda         ht + 1
            cmp         #$A0
            bcc         @op
            rts
@end:
            lda         ht
            sta         vx_nat
            lda         ht + 1
            sta         vx_nat + 1
            lda         vx_ntbl                             ; (The table: n, then each block's start and end)
            beq         @top
            asl
            asl
            inc         a
            jsr         vx_htadd
@top:
            lda         ht
            sta         vx_top
            lda         ht + 1
            sta         vx_top + 1
            cmp         #$A0
            rts

; Each op's stub, then its data
vx_pass2:
            stz         hq
            lda         #$80
            sta         hq + 1
            lda         vx_a0
            sta         ht
            lda         vx_a0 + 1
            sta         ht + 1
@op:
            jsr         vx_atend
            bcc         :+
            rts
:
            jsr         vx_oplen
            jsr         vx_size                             ; (The next op's entry)
            clc
            adc         ht
            sta         vx_nx
            lda         ht + 1
            adc         #0
            sta         vx_nx + 1
            lda         vm_sb                               ; (The op, to vx_buf)
            sta         $00
            ldy         #0
:
            lda         (hq),y
            sta         vx_buf,y
            iny
            cpy         vx_len
            bne         :-
            lda         vx_tp + 1                           ; (Its template, then its stub if it has one)
            beq         :+
            jsr         vx_tcopy
            lda         vx_tfl
            lsr
            bcc         @next
:
            lda         ht                                  ; (Its data's place: past the stub)
            clc
            adc         #VX_STUB
            sta         vx_d
            lda         ht + 1
            adc         #0
            sta         vx_d + 1
            lda         vx_a                                ; (Its address operands)
            jsr         vx_patch
            lda         vx_a + 1
            jsr         vx_patch
            lda         vx_bank                             ; (The stub)
            sta         $00
            ldy         #0
            lda         #$A9                                ; (lda #<data)
            sta         (ht),y
            iny
            lda         vx_d
            sta         (ht),y
            iny
            lda         #$85                                ; (sta vm_ip)
            sta         (ht),y
            iny
            lda         #<vm_ip
            sta         (ht),y
            iny
            lda         #$A9                                ; (lda #>data)
            sta         (ht),y
            iny
            lda         vx_d + 1
            sta         (ht),y
            iny
            lda         #$85                                ; (sta vm_ip + 1)
            sta         (ht),y
            iny
            lda         #<(vm_ip + 1)
            sta         (ht),y
            iny
            lda         #$4C                                ; (jmp its code)
            sta         (ht),y
            iny
            lda         vx_h
            sta         (ht),y
            iny
            lda         vx_h + 1
            sta         (ht),y
            iny
            ldx         vx_d0                               ; (Its data)
            lda         vx_dl
            sta         vx_k
:
            lda         vx_buf,x
            sta         (ht),y
            inx
            iny
            dec         vx_k
            bne         :-
@next:
            lda         vx_nx
            sta         ht
            lda         vx_nx + 1
            sta         ht + 1
            lda         vx_len
            jsr         vx_hqadd
            jmp         @op

; .A = the op's native size (vx_oplen's, at hq): its stub; or its template (vx_tpl), and its stub if it has one
vx_size:
            jsr         vx_tpl
            lda         vx_tp + 1
            bne         :+
            lda         vx_dl
            clc
            adc         #VX_STUB
            rts
:
            lda         vx_tfl
            lsr
            bcs         :+
            lda         vx_tln
            rts
:
            lda         vx_dl
            clc
            adc         #VX_STUB
            adc         vx_tln
            rts

; vx_tp = the op's template (at hq, in vm_sb; 0 if it has none: its stub alone), its header read (vx_tln ...)
vx_tpl:
            stz         vx_tp + 1
            lda         vm_sb
            sta         $00
            lda         (hq)
            sta         vx_op
            and         #$7F
            ldx         #<vx_tlq
            ldy         #>vx_tlq
            cmp         #VO_LQ
            beq         @fused
            ldx         #<vx_tlqp
            ldy         #>vx_tlqp
            cmp         #VO_LQP
            beq         @fused
            ldx         #<vx_tjlq
            ldy         #>vx_tjlq
            cmp         #VO_JLQ
            beq         @fused
            ldx         #<vx_tjq
            ldy         #>vx_tjq
            cmp         #VO_JQ
            beq         @fused
            ldx         #<vx_tjll
            ldy         #>vx_tjll
            cmp         #VO_JLL
            beq         @fused
            cmp         #VO_LL
            bne         @main
            ldy         #1                                  ; (LL: s's bit 0, pushed)
            lda         (hq),y
            ldx         #<vx_tll
            ldy         #>vx_tll
            lsr
            bcc         @fused
            ldx         #<vx_tllp
            ldy         #>vx_tllp
            bra         @fused
@main:
            tax
            lda         vx_tmain,x
            sta         vx_tp
            lda         vx_tmain + 1,x
            sta         vx_tp + 1
            bra         @hdr
@fused:
            stx         hb
            sty         hb + 1
            ldy         #1
            lda         (hq),y
            and         #$FE
            sec
            sbc         #VO_ADD
            tay
            lda         (hb),y
            sta         vx_tp
            iny
            lda         (hb),y
            sta         vx_tp + 1
@hdr:
            lda         vx_tp + 1
            beq         @rts
            sta         hb + 1
            lda         vx_tp
            sta         hb
            ldy         #1
            lda         (hb),y
            sta         vx_tfl
            and         #TF_NOR
            beq         :+
            lda         vx_op
            bpl         :+
            stz         vx_tp + 1
            rts
:
            lda         (hb)
            sta         vx_tln
            ldy         #2
            lda         (hb),y
            sta         vx_tnp
            lda         hb
            clc
            adc         #3
            sta         vx_tpp
            lda         hb + 1
            adc         #0
            sta         vx_tpp + 1
            lda         vx_tnp
            asl
            adc         vx_tnp
            clc
            adc         vx_tpp
            sta         vx_tcode
            lda         vx_tpp + 1
            adc         #0
            sta         vx_tcode + 1
@rts:
            rts

; The op's template's code at ht (the arena's bank), its patches made; ht past it
vx_tcopy:
            lda         vx_tcode
            sta         hb
            lda         vx_tcode + 1
            sta         hb + 1
            lda         vx_bank
            sta         $00
            ldy         #0
:
            lda         (hb),y
            sta         (ht),y
            iny
            cpy         vx_tln
            bne         :-
            lda         vx_tpp
            sta         hb
            lda         vx_tpp + 1
            sta         hb + 1
            lda         vx_tnp
            sta         vx_pi
@p:
            lda         vx_pi
            bne         :+
            lda         vx_tln
            jmp         vx_htadd
:
            ldy         #2                                  ; (Its data byte, in vx_buf)
            lda         (hb),y
            clc
            adc         vx_d0
            tax
            ldy         #1
            lda         (hb),y
            cmp         #TPK_N
            beq         @n
            cmp         #TPK_T
            beq         @t
            cmp         #TPK_U
            lda         vx_buf,x
            bcc         @b
            and         #$FE
@b:
            jsr         vx_pb
            bra         @pn
@n:
            lda         vx_nx
            ldx         vx_nx + 1
            bra         @w
@t:
            lda         vx_buf + 1,x
            pha
            lda         vx_buf,x
            plx
            jsr         vm_xmap
@w:
            jsr         vx_pw
@pn:
            lda         hb
            clc
            adc         #3
            sta         hb
            bcc         :+
            inc         hb + 1
:
            dec         vx_pi
            bra         @p

; .A (vx_pb), .A, .X (vx_pw) at the template's place at ht + the patch's offset ((hb)), in the arena's bank
vx_pb:
            pha
            lda         vx_bank
            sta         $00
            lda         (hb)
            tay
            pla
            sta         (ht),y
            rts
vx_pw:
            jsr         vx_pb
            iny
            txa
            sta         (ht),y
            rts

; The blocks' table, if there's one: n, then each block's start (its BLOCK op's data) and end (an op's entry, or the
; code's end)
vx_table:
            lda         vx_ntbl
            bne         :+
            rts
:
            lda         vx_end
            sta         hq
            lda         vx_end + 1
            sta         hq + 1
            lda         vx_nat
            sta         ht
            lda         vx_nat + 1
            sta         ht + 1
            lda         vx_bank
            sta         $00
            lda         vx_ntbl
            sta         (ht)
            sta         vx_k
            lda         #1
            jsr         vx_htadd
            lda         #1
            jsr         vx_hqadd
@entry:
            jsr         vx_word                             ; (Its start: the BLOCK op's data)
            jsr         vm_xmap
            clc
            adc         #VX_STUB
            bcc         :+
            inx
:
            jsr         vx_put
            jsr         vx_word                             ; (Its end)
            cmp         vx_end
            bne         @map
            cpx         vx_end + 1
            bne         @map
            lda         vx_nat
            ldx         vx_nat + 1
            bra         @end
@map:
            jsr         vm_xmap
@end:
            jsr         vx_put
            dec         vx_k
            bne         @entry
            rts

; .A, .X = the word at hq in vm_sb; hq past it
vx_word:
            lda         vm_sb
            sta         $00
            ldy         #1
            lda         (hq),y
            tax
            lda         (hq)
            pha
            lda         #2
            jsr         vx_hqadd
            pla
            rts

; .A, .X put at ht in the arena's bank; ht past it
vx_put:
            pha
            lda         vx_bank
            sta         $00
            pla
            sta         (ht)
            ldy         #1
            txa
            sta         (ht),y
            lda         #2
            jmp         vx_htadd

; C = 1 if hq is at the bytecode's end (vx_end)
vx_atend:
            lda         hq
            cmp         vx_end
            lda         hq + 1
            sbc         vx_end + 1
            rts

; ht (hq) .A bytes on
vx_htadd:
            clc
            adc         ht
            sta         ht
            bcc         :+
            inc         ht + 1
:
            rts
vx_hqadd:
            clc
            adc         hq
            sta         hq
            bcc         :+
            inc         hq + 1
:
            rts

; .A, .X = a bytecode place (an op's): its stub's entry (the map's)
vm_xmap:
            sta         hp
            stx         hp + 1
            lda         vm_mb
            sta         $00
            lda         (hp)
            pha
            lda         hp + 1
            clc
            adc         #$10
            sta         hp + 1
            lda         (hp)
            tax
            pla
            rts

; The op at hq (in vm_sb): vx_len its length, vx_dl its data's (an EXT left out), vx_d0 where its data begins in it,
; vx_a its address operands, vx_h its code
vx_oplen:
            lda         vm_sb
            sta         $00
            lda         (hq)
            and         #$7F
            cmp         #VO_EXT
            beq         @ext
            tax                                             ; (Its code)
            lda         vx_ops,x
            sta         vx_h
            lda         vx_ops + 1,x
            sta         vx_h + 1
            txa
            lsr
            tax
            lda         vx_ta1,x
            sta         vx_a
            lda         vx_ta2,x
            sta         vx_a + 1
            lda         vx_tlen,x
            bne         @len
            ldy         #2                                  ; (LOCALS: 5 + 2kn; BLOCK: 6 + 2k)
            lda         (hq),y
            asl
            clc
            adc         #5
            cpx         #VO_BLOCK / 2
            bne         @len
            inc         a
@len:
            sta         vx_len
            sta         vx_dl
            stz         vx_d0
            rts
@ext:
            ldy         #1
            lda         (hq),y
            and         #$7F
            tax
            lda         vx_xops,x
            sta         vx_h
            lda         vx_xops + 1,x
            sta         vx_h + 1
            txa
            lsr
            tax
            lda         vx_xa1,x
            sta         vx_a
            stz         vx_a + 1
            lda         vx_xlen,x
            sta         vx_dl
            inc         a
            sta         vx_len
            lda         #1
            sta         vx_d0
            rts

; An address operand (.A: offset * 8 + kind; 0: none) of the op in vx_buf made the native code's
vx_patch:
            cmp         #0
            bne         :+
            rts
:
            pha
            lsr
            lsr
            lsr
            clc
            adc         vx_d0
            tax                                             ; (Its place in vx_buf)
            pla
            and         #7
            cmp         #VXK_C
            beq         @c
            cmp         #VXK_R
            beq         @r
            cmp         #VXK_T
            beq         @t
            pha                                             ; (P, B: 0 stays 0)
            lda         vx_buf,x
            ora         vx_buf + 1,x
            bne         :+
            pla
            rts
:
            pla
            cmp         #VXK_B
            beq         @b
            phx                                             ; (P: the parent's data, past its stub)
            lda         vx_buf + 1,x
            pha
            lda         vx_buf,x
            plx
            jsr         vm_xmap
            clc
            adc         #VX_STUB
            bcc         :+
            inx
:
            stx         vx_k
            plx
            sta         vx_buf,x
            lda         vx_k
            sta         vx_buf + 1,x
            rts
@b:
            lda         vx_nat
            sta         vx_buf,x
            lda         vx_nat + 1
            sta         vx_buf + 1,x
            rts
@c:
            lda         vx_a0
            sta         vx_buf,x
            lda         vx_a0 + 1
            sta         vx_buf + 1,x
            rts
@t:
            phx
            lda         vx_buf + 1,x
            pha
            lda         vx_buf,x
            plx
            jsr         vm_xmap
            stx         vx_k
            plx
            sta         vx_buf,x
            lda         vx_k
            sta         vx_buf + 1,x
            rts
@r:
            lda         vx_d                                ; (Past the data: vx_d + vx_dl, encoded)
            clc
            adc         vx_dl
            pha
            lda         vx_d + 1
            adc         #0
            and         #$1F
            ora         vx_idx
            sta         vx_k
            pla
            asl
            ora         #1
            sta         vx_buf,x
            lda         vx_k
            rol
            sta         vx_buf + 1,x
            rts

.segment "RODATA8"

; The ops' code (vm_ops' again, for this bank), the rare ones' (EXT's: vm_xops')
vx_ops:
            VM_OPS
vx_xops:    .word       op_eval, op_setbg, op_defg, op_tryv, op_tryh, op_tryf

; Each op (by its number / 2): its length (0: LOCALS, BLOCK: from its count), its address operands (vx_patch's)
.define VXA(o, k) ((o) * 8 + (k))
vx_tlen:    .byte       1, 3, 2, 3, 3, 1, 3, 3                          ; (RET CONST LOCAL GLOBAL LOOKUP PUSH JF JT)
            .byte       3, 4, 6, 14, 4, 5, 0, 3                         ; (JMP JE HEAD CALL TCALL BCALL EXT QUOTE)
            .byte       3, 1, 1, 1, 1, 1, 1, 1                          ; (QUOTE0 ADD SUB LT GT LE GE NEQ)
            .byte       1, 1, 1, 1, 3, 3, 3, 3                          ; (INC DEC ZEROP ONEP ADDC SUBC LTC GTC)
            .byte       3, 3, 3, 2, 3, 12, 8, 5                         ; (LEC GEC EQC LPUSH CPUSH SHEAD CSELF TSELF)
            .byte       0, 4, 2, 4, 5, 7, 1, 1                          ; (LOCALS LOCALH SETL SETBL LQ JLQ STT POPX)
            .byte       6, 0, 6, 4, 6, 1, 3, 1                          ; (LOOP BLOCK LOCALB SETLB SETBLB DOTN DOTI DOTINC)
            .byte       1, 3, 2, 3, 6, 4, 6, 5                          ; (EACHL EACHI DROP TRYE JQ LL JLL LQP)
vx_ta1:     .byte       0, 0, 0, 0, 0, 0, VXA(1, VXK_T), VXA(1, VXK_T)
            .byte       VXA(1, VXK_T), VXA(2, VXK_T), VXA(4, VXK_T), VXA(2, VXK_R), 0, 0, 0, 0
            .byte       0, 0, 0, 0, 0, 0, 0, 0
            .byte       0, 0, 0, 0, 0, 0, 0, 0
            .byte       0, 0, 0, 0, 0, VXA(4, VXK_T), VXA(2, VXK_C), VXA(2, VXK_C)
            .byte       VXA(3, VXK_B), 0, 0, 0, 0, VXA(5, VXK_T), 0, 0
            .byte       VXA(1, VXK_T), VXA(4, VXK_P), 0, 0, 0, 0, VXA(1, VXK_T), 0
            .byte       0, VXA(1, VXK_T), 0, VXA(1, VXK_T), VXA(4, VXK_T), 0, VXA(4, VXK_T), 0
vx_ta2:     .byte       0, 0, 0, 0, 0, 0, 0, 0
            .byte       0, 0, 0, 0, 0, 0, 0, 0
            .byte       0, 0, 0, 0, 0, 0, 0, 0
            .byte       0, 0, 0, 0, 0, 0, 0, 0
            .byte       0, 0, 0, 0, 0, 0, VXA(4, VXK_R), 0
            .byte       0, 0, 0, 0, 0, 0, 0, 0
            .byte       VXA(4, VXK_T), 0, 0, 0, 0, 0, 0, 0
            .byte       0, 0, 0, 0, 0, 0, 0, 0
; The rare ones (by their number / 2: EVAL SETBG DEFG TRYV TRYH TRYF): their data's length, address operand
vx_xlen:    .byte       5, 3, 3, 3, 3, 1
vx_xa1:     .byte       0, 0, 0, 0, VXA(1, VXK_T), 0

.include "vmxt.inc"
.assert     VO_COUNT = 128, error, "vmx.inc: vx_tlen ... are for 64 ops"
