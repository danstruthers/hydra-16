; ****************************************************************************
; disasm.s - HyForth's disassembler (/lib/forth/disasm.fl: lib disasm): disasm, every W65C02S opcode, each
; instruction as the assembler as writes it (the asm library's DIS, spec/asm.def: db's and dis's way too); and
; (see-code), which see (tools.fl) runs for an assembly word while this library is loaded: its instructions, in
; place of its bytes.  A jsr's or jmp's to a word has "  \ name" after it.

.include "forthlib.inc"
.include "asmlib.inc"

.bss
da_bytes:   .res        3                                   ; An instruction's bytes (from p1) ...
da_kind:    .res        1                                   ;   its kind (DK_) ...
da_len:     .res        1                                   ;   its length ...
da_tgt:     .res        2                                   ;   where it branches or jumps to (0: nowhere) ...
da_text:    .res        DIS_MAX                             ;   and it, as as writes it
da_far:     .res        2                                   ; SEE of a code word: the furthest branch forward ...
da_lim:     .res        2                                   ;   and the next header (its code's end at the most)
asm_mod:    .res        1                                   ; The asm library's module (its paged ROM bank)
da_info:    .res        ME_SIZE                             ; (MODINFO's ...
da_idx:     .res        1                                   ;   and its entry)
.code

; Its start: the asm library found (MODINFO: the module "asm"), else THROW -21 (unsupported)
lib_init:
            stz         da_idx
@find:
            LDR         r0, da_info
            lda         da_idx
            stx         xsave
            jsr         MODINFO
            ldx         xsave
            bcs         @none
            inc         da_idx
            lda         da_info + ME_TYPE
            cmp         #HT_LIBRARY
            bne         @find
            ldy         #0
:
            lda         s_asm,y
            cmp         da_info + ME_NAME,y
            bne         @find
            iny
            cmp         #0
            bne         :-
            lda         da_info + ME_BANK
            sta         asm_mod
            rts
@none:
            lda         #<-21
            jmp         throw_a
s_asm:      .byte       "asm", 0

            HEADER      "(see-code)", 0
seecode:                                                    ; ( nt -- ): the code word nt (an assembly word's
            lda         dlo,x                               ;   header), as see shows it
            sta         w
            lda         dhi,x
            sta         w + 1
            inx
            jmp         see_code

s_immed:    .byte       " immediate", 0

            HEADER      "disasm", 0
disasm:                                                     ; ( addr u -- ): u instructions from addr, a line each:
            lda         dlo,x                               ;   its address, its bytes, it (a jsr's or jmp's to a
            sta         tmp3                                ;   word, with the word's name)
            lda         dhi,x
            sta         tmp3 + 1
            lda         dlo + 1,x
            sta         p1
            lda         dhi + 1,x
            sta         p1 + 1
            inx
            inx
@line:
            lda         tmp3
            ora         tmp3 + 1
            beq         @done
            bit         intr                                ; (Ctrl-C)
            bvc         :+
            jmp         intr_throw
:
            jsr         da_line
            lda         tmp3
            bne         :+
            dec         tmp3 + 1
:
            dec         tmp3
            bra         @line
@done:
            rts

; SEE of the code word w (an assembly word: hdr_asm's): "code name", its instructions to its end (an rts, rti, jmp,
; bra or stp past every branch forward in it, or the next header), "end-code"
see_code:
            LDR         w2, s_code
            jsr         w2_out
            jsr         hdr_out
            jsr         hdr_xt
            lda         cnt
            pha
            jsr         cr
            lda         w2
            sta         p1
            sta         da_far
            lda         w2 + 1
            sta         p1 + 1
            sta         da_far + 1
            jsr         da_limit
@line:
            bit         intr                                ; (Ctrl-C)
            bvc         :+
            pla
            jmp         intr_throw
:
            lda         p1                                  ; (At the next header: the end)
            cmp         da_lim
            lda         p1 + 1
            sbc         da_lim + 1
            bcs         @end
            jsr         da_line
            lda         da_tgt + 1                          ; A branch forward in it: its end is after there
            beq         @ends
            lda         da_far
            cmp         da_tgt
            lda         da_far + 1
            sbc         da_tgt + 1
            bcs         @ends
            lda         da_tgt
            cmp         da_lim
            lda         da_tgt + 1
            sbc         da_lim + 1
            bcs         @ends
            lda         da_tgt
            sta         da_far
            lda         da_tgt + 1
            sta         da_far + 1
@ends:
            lda         da_kind                             ; Its end: an instruction that goes no further (but
            and         #DK_END                             ;   brk), past every branch forward
            beq         @line
            lda         da_bytes
            beq         @line
            lda         da_far
            cmp         p1
            lda         da_far + 1
            sbc         p1 + 1
            bcs         @line
@end:
            LDR         w2, s_endcode
            jsr         w2_out
            pla
            bpl         :+
            LDR         w, s_immed
            jsr         type_z
:
            jmp         cr

s_code:     .byte       "code ", 0
s_endcode:  .byte       "end-code", 0

; The zero-terminated string at w2 out (type_z's, w kept)
w2_out:
            ldy         #0
:
            lda         (w2),y
            beq         :+
            jsr         emit_a
            iny
            bra         :-
:
            rts

; da_lim: the first header after the code at p1 (any word list's), or HERE (code in RAM), or the module's end
da_limit:
            lda         here
            sta         da_lim
            lda         here + 1
            sta         da_lim + 1
            lda         p1 + 1
            cmp         #$A0
            bcc         :+
            stz         da_lim
            lda         #$E0
            sta         da_lim + 1
:
            lda         wl_last
            sta         w3
            lda         wl_last + 1
            sta         w3 + 1
@wl:
            lda         w3
            ora         w3 + 1
            beq         @done
            lda         (w3)
            sta         w2
            ldy         #1
            lda         (w3),y
            sta         w2 + 1
@hdr:
            lda         w2
            ora         w2 + 1
            beq         @next
            lda         p1                                  ; (After p1 ...
            cmp         w2
            lda         p1 + 1
            sbc         w2 + 1
            bcs         :+
            lda         w2                                  ;   and before da_lim: da_lim)
            cmp         da_lim
            lda         w2 + 1
            sbc         da_lim + 1
            bcs         :+
            lda         w2
            sta         da_lim
            lda         w2 + 1
            sta         da_lim + 1
:
            ldy         #1
            lda         (w2),y
            pha
            lda         (w2)
            sta         w2
            pla
            sta         w2 + 1
            bra         @hdr
@next:
            ldy         #3                                  ; (The word list before it)
            lda         (w3),y
            pha
            dey
            lda         (w3),y
            sta         w3
            pla
            sta         w3 + 1
            bra         @wl
@done:
            rts

; The instruction at p1 out, a line: its address, its bytes, it as as writes it (a jsr's or jmp's to a word with
; "  \ name"); p1 past it.  OUT: da_bytes, da_kind, da_len its; da_tgt the address a branch or jmp goes to (else 0).
; Keeps tmp3
da_line:
            jsr         space
            lda         p1 + 1
            jsr         hex2
            lda         p1
            jsr         hex2
            jsr         space
            jsr         da_decode
            ldy         #0                                  ; Its bytes, in 3 bytes' room
:
            jsr         space
            lda         da_bytes,y
            jsr         hex2
            iny
            cpy         da_len
            bne         :-
:
            cpy         #3
            beq         :+
            jsr         space
            jsr         space
            jsr         space
            iny
            bra         :-
:
            jsr         space
            jsr         space
            LDR         w2, da_text                         ; It
            jsr         w2_out
            lda         da_bytes                            ; A jsr or jmp: to a word?
            cmp         #$20
            beq         :+
            cmp         #$4C
            bne         @end
:
            lda         w
            pha
            lda         w + 1
            pha
            lda         da_bytes + 1
            ldy         da_bytes + 2
            PUSHAY
            jsr         hdr_of_xt
            inx
            bcs         :+
            LDR         w2, s_comment
            jsr         w2_out
            jsr         hdr_out
:
            pla
            sta         w + 1
            pla
            sta         w
@end:
            clc                                             ; p1 past it
            lda         p1
            adc         da_len
            sta         p1
            bcc         :+
            inc         p1 + 1
:
            jmp         cr

s_comment:  .byte       "  \ ", 0

; The instruction at p1, by the asm library's DIS: da_bytes (its opcode, then the next two: copied, as p1 may be
; in the paged ROM, which is the library's while it runs), da_text, da_len, da_kind; da_tgt where a branch or jmp
; goes (else 0)
da_decode:
            ldy         #2
:
            lda         (p1),y
            sta         da_bytes,y
            dey
            bpl         :-
            LDR         r0, da_bytes
            lda         p1
            sta         r1
            lda         p1 + 1
            sta         r1 + 1
            LDR         r2, da_text
            stz         r3
            stz         r3 + 1
            stx         xsave
            lda         #0
            ASMCALL     ASM_DIS
            sta         da_len
            stx         da_kind
            ldx         xsave
            stz         da_tgt
            stz         da_tgt + 1
            lda         da_kind                             ; (A branch or a jmp: where it goes; not a jsr)
            and         #DK_GO | DK_CALL
            cmp         #DK_GO
            bne         :+
            lda         r4
            sta         da_tgt
            lda         r4 + 1
            sta         da_tgt + 1
:
            rts
