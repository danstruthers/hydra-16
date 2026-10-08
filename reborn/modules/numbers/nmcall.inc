; ****************************************************************************
; nmcall.inc - a call into the numbers library: its start and end, its failure from however deep, and the caller's
; memory read and written.  An entry (the jump table's routines) starts with jsr nm_begin (or nm_begin0: INIT's, before
; the state is there) and ends with jmp nm_end, C and .A, .X its answer; nm_fail, .A an error (NE_*), goes back to the
; entry's caller from anywhere in it.  The caller's numbers and text are wherever it sees them: below $8000 they're read
; and written as they are; at $8000-$9FFF they're in its bank (v_bank), selected for each byte (the library's own bank
; is there the rest of the time); its numbers can't be in its paged ROM ($A000-$DFFF: the library's is there).

; ---- A call's start and end

; The library's bank selected (r13's), the caller's kept (v_bank), the stack as the entry has it noted (nm_fail goes
; back to it).  Then the state checked: INIT must have run (NE_INIT); and the caller's zero page $70-$7F kept (v_zp)
; unless it said it needn't be (OPTIONS: st_zpfree).  Keeps .A, .X (in v_ax too), .Y
nm_begin:
            jsr         nm_bank
            tsx                                             ; (The stack as the entry has it, past this return)
            inx
            inx
            stx         v_sp
            lda         st_magic
            cmp         #NUMBERS_MAGIC
            bne         @init
            lda         st_zpfree
            sta         v_zpfree
            bne         :+
            jsr         nm_keep
:
            lda         v_ax
            ldx         v_ax + 1
            rts
@init:
            lda         #1                                  ; (Nothing kept: nothing put back)
            sta         v_zpfree
            lda         #NE_INIT
            jmp         nm_fail

; INIT's: as nm_begin, the zero page always kept, no state yet
nm_begin0:
            jsr         nm_bank
            tsx
            inx
            inx
            stx         v_sp
            jsr         nm_keep
            lda         v_ax
            ldx         v_ax + 1
            rts

; The library's bank selected, the caller's kept (v_bank), .A, .X kept (v_ax), the arena empty
nm_bank:
            pha
            phx
            lda         RAM_BANK                            ; (The caller's bank, kept once the library's is there)
            ldx         r13
            stx         RAM_BANK
            sta         v_bank
            plx
            stx         v_ax + 1
            pla
            sta         v_ax
            LDR         a_top, arena                        ; (The arena empty)
            rts

; The caller's zero page kept (v_zp), and put back as the call ends (v_zpfree 0)
nm_keep:
.repeat 16, I                                               ; (Unrolled: each call's)
            lda         na + I
            sta         v_zp + I
.endrep
            stz         v_zpfree
            rts

; The call's end: the caller's zero page and bank back, C, .A and .X as they are
nm_end:
            php
            pha
            phx
            lda         v_zpfree
            bne         :+
.repeat 16, I
            lda         v_zp + I
            sta         na + I
.endrep
:
            lda         v_bank
            sta         RAM_BANK
            plx
            pla
            plp
            rts

; A failure, .A its error (NE_*): back to the entry's caller from however deep, C = 1
nm_fail:
            ldx         v_sp
            txs
            ldx         #0
            sec
            jmp         nm_end

; ---- The caller's memory

; .A = the caller's byte at np, np moved on.  Keeps .X, .Y
c_get:
            lda         np + 1
            cmp         #$80
            bcc         @here
            cmp         #$A0
            bcs         @here
            lda         v_bank                              ; (Its bank, for the byte)
            sta         RAM_BANK
            lda         (np)
            pha
            lda         r13
            sta         RAM_BANK
            pla
            bra         @next
@here:
            lda         (np)
@next:
            inc         np
            bne         :+
            inc         np + 1
:
            rts

; The caller's byte at np = .A, np moved on.  Keeps .A, .X, .Y
c_put:
            pha
            lda         np + 1
            cmp         #$80
            bcc         @here
            cmp         #$A0
            bcs         @here
            lda         v_bank
            sta         RAM_BANK
            pla
            sta         (np)
            pha
            lda         r13
            sta         RAM_BANK
            pla
            bra         @next
@here:
            pla
            sta         (np)
@next:
            inc         np
            bne         :+
            inc         np + 1
:
            rts

; np moved on .A bytes
c_skip:
            clc
            adc         np
            sta         np
            bcc         :+
            inc         np + 1
:
            rts

; ---- Numbers in the stored format, in and out

; v_len = the length of the number at np (the caller's), its structure checked as it's measured (its tags and
; lengths, not its one form: BYTES checks that); np at its end.  Not a number: NE_NOTNUM
nm_measure:
            stz         v_len
            stz         v_len + 1
            jsr         c_get
            jsr         m_count1
            cmp         #NT_COMPLEX
            bne         @real
            jsr         m_real
            jmp         m_real
@real:
            jmp         m_real_tag

; A real number's length counted (np at its tag)
m_real:
            jsr         c_get
            jsr         m_count1
m_real_tag:
            cmp         #NT_FIXED
            bcc         m_int_tag                           ; (An integer)
            cmp         #NT_FIXED + 16
            bcc         m_int                               ; (A fixed decimal of 0-15 places: its digits)
            cmp         #NT_FIXED_LONG
            bne         @rat
            lda         #2                                  ; (Its places, two bytes, then its digits)
            jsr         m_skip
            bra         m_int
@rat:
            cmp         #NT_RATIONAL
            bne         m_bad
            jsr         m_int
m_int:
            jsr         c_get
            jsr         m_count1
m_int_tag:
            cmp         #$80
            bcc         @rts                                ; (-64 to 63: the tag alone)
            cmp         #$A0
            bcs         @long
            and         #$0F                                ; (1 to 16 bytes)
            inc         a
            jmp         m_skip
@long:
            cmp         #NT_LONG_NEG + 1
            bcs         m_bad
            jsr         c_get                               ; (Its length, 17 to 255)
            jsr         m_count1
            cmp         #17
            bcc         m_bad
            jmp         m_skip
@rts:
            rts
m_bad:
            lda         #NE_NOTNUM
            jmp         nm_fail

; v_len += 1 (keeps .A); v_len += .A, np moved on as many (m_skip).  Past NUM_MAX: NE_NOTNUM
m_count1:
            pha
            lda         #1
            jsr         m_add
            pla
            rts
m_skip:
            pha
            jsr         m_add
            pla
            jmp         c_skip
m_add:
            clc
            adc         v_len
            sta         v_len
            bcc         :+
            inc         v_len + 1
:
            lda         v_len + 1
            cmp         #>(NUM_MAX + 1)
            bcc         :+
            bne         m_bad
            lda         v_len
            cmp         #<(NUM_MAX + 1)
            bcs         m_bad
:
            rts

; The caller's number at .A, .X (an operand: r0's, r1's) copied to the arena.  OUT: .A, .X = its copy (a value);
; v_len its length.  Not a number: NE_NOTNUM; no room: NE_BIG
nm_in:
            sta         np
            stx         np + 1
            cpx         #$7E                                ; (A short one below $7E00: its length from its tags,
            bcs         @measure                            ;   then copied straight)
            lda         (np)
            cmp         #NT_POS
            bcc         @one
            cmp         #NT_LONG_POS
            bcc         @int
            cmp         #NT_FIXED
            bcc         @measure
            cmp         #NT_FIXED + 16
            bcs         @measure
            ldy         #1                                  ; (A fixed decimal of 0-15 places: its digits' tag)
            lda         (np),y
            cmp         #NT_POS
            bcc         @two
            cmp         #NT_LONG_POS
            bcs         @measure
            and         #$0F
            adc         #3                                  ; (C = 0: 1 + 1 + its bytes)
            bra         @short
@one:
            lda         #1
            bra         @short
@two:
            lda         #2
            bra         @short
@int:
            and         #$0F
            clc
            adc         #2
@short:
            sta         v_len
            stz         v_len + 1
            ldx         #0
            jsr         a_room
            jsr         m_copy_in
            jmp         a_made
@measure:
            jsr         nm_measure
            lda         np + 1                              ; (Its end below $8000, all of it in the caller's own
            pha                                             ;   RAM: copied straight)
            lda         np                                  ; (Back to its start: np less its length)
            sec
            sbc         v_len
            sta         np
            lda         np + 1
            sbc         v_len + 1
            sta         np + 1
            lda         v_len
            ldx         v_len + 1
            jsr         a_room
            pla
            cmp         #$80
            bcs         :+
            jsr         m_copy_in
            jmp         a_made
:
            MOVR        v_left, v_len
@byte:
            lda         v_left
            ora         v_left + 1
            beq         @done
            jsr         c_get
            jsr         a_put
            lda         v_left
            bne         :+
            dec         v_left + 1
:
            dec         v_left
            bra         @byte
@done:
            jmp         a_made

; v_len bytes copied from the caller's np to the arena's nu, each moved on past them: both in the RAM as it is (the
; caller's below $8000)
m_copy_in:
            ldy         #0
            ldx         v_len + 1
            beq         @rest
@page:
            lda         (np),y
            sta         (nu),y
            iny
            bne         @page
            inc         np + 1
            inc         nu + 1
            dex
            bne         @page
@rest:
            cpy         v_len
            beq         @end
            lda         (np),y
            sta         (nu),y
            iny
            bra         @rest
@end:
            tya
            clc
            adc         nu
            sta         nu
            bcc         :+
            inc         nu + 1
:
            tya
            clc
            adc         np
            sta         np
            bcc         :+
            inc         np + 1
:
            rts

; v_len bytes copied from nq (the library's) to the caller's np (below $8000), each moved on past them
m_copy_out:
            ldy         #0
            ldx         v_len + 1
            beq         @rest
@page:
            lda         (nq),y
            sta         (np),y
            iny
            bne         @page
            inc         nq + 1
            inc         np + 1
            dex
            bne         @page
@rest:
            cpy         v_len
            beq         @end
            lda         (nq),y
            sta         (np),y
            iny
            bra         @rest
@end:
            tya
            clc
            adc         nq
            sta         nq
            bcc         :+
            inc         nq + 1
:
            tya
            clc
            adc         np
            sta         np
            bcc         :+
            inc         np + 1
:
            rts

; The call's end with the number .A, .X (a value), copied to the caller's place r2 (its room r3): .A, .X = its
; length, C = 0.  No room: NE_ROOM
nm_out:
            sta         nq
            stx         nq + 1
            jsr         n_size
            sta         v_len
            stx         v_len + 1
            lda         r3 + 1
            cmp         v_len + 1
            bcc         @noroom
            bne         :+
            lda         r3
            cmp         v_len
            bcs         :+
@noroom:
            lda         #NE_ROOM
            jmp         nm_fail
:
            MOVR        np, r2
            lda         r2                                  ; (Its end below $8000, all of it in the caller's own
            clc                                             ;   RAM: copied straight)
            adc         v_len
            lda         r2 + 1
            adc         v_len + 1
            bcs         :+
            cmp         #$80
            bcs         :+
            jsr         m_copy_out
            bra         @done
:
            lda         v_len + 1                           ; (Its end in the caller's bank, 32 bytes at most: through
            bne         @slow                               ;   the stack, the caller's bank selected once)
            lda         v_len
            cmp         #33
            bcs         @slow
            lda         r2 + 1
            cmp         #$80
            bcc         @slow
            cmp         #$9F
            bcs         @slow
            ldy         v_len
@push:
            dey
            lda         (nq),y
            pha
            tya
            bne         @push
            ldx         v_len                               ; (Its count: v_len is the library's bank's)
            lda         v_bank
            sta         RAM_BANK
@pull:
            pla
            sta         (np),y
            iny
            dex
            bne         @pull
            lda         r13
            sta         RAM_BANK
            bra         @done
@slow:
            MOVR        v_left, v_len
@byte:
            lda         v_left
            ora         v_left + 1
            beq         @done
            lda         (nq)
            jsr         c_put
            inc         nq
            bne         :+
            inc         nq + 1
:
            lda         v_left
            bne         :+
            dec         v_left + 1
:
            dec         v_left
            bra         @byte
@done:
            lda         v_len
            ldx         v_len + 1
            clc
            jmp         nm_end
@room:
            lda         #NE_ROOM
            jmp         nm_fail

; The call's end with two numbers: n_v2 (a value) copied to the caller's r5 (its room r6; r6 its length after), and
; .A, .X to r2 (room r3), whose length .A, .X are.  No room for either: NE_ROOM, neither written
nm_out2:
            sta         n_v1
            stx         n_v1 + 1
            jsr         n_size
            cpx         r3 + 1
            bcc         :+
            bne         @room
            cmp         r3
            beq         :+
            bcs         @room
:
            lda         n_v2
            ldx         n_v2 + 1
            jsr         n_size
            sta         v_len
            stx         v_len + 1
            cpx         r6 + 1
            bcc         :+
            bne         @room
            cmp         r6
            beq         :+
            bcs         @room
:
            MOVR        np, r5
            MOVR        nq, n_v2
            MOVR        v_left, v_len
@byte:
            lda         v_left
            ora         v_left + 1
            beq         @done
            lda         (nq)
            jsr         c_put
            inc         nq
            bne         :+
            inc         nq + 1
:
            lda         v_left
            bne         :+
            dec         v_left + 1
:
            dec         v_left
            bra         @byte
@done:
            MOVR        r6, v_len
            lda         n_v1
            ldx         n_v1 + 1
            jmp         nm_out
@room:
            lda         #NE_ROOM
            jmp         nm_fail

; n_x = the caller's r0 (its copy), n_y = its r1
nm_in2:
            lda         r0
            ldx         r0 + 1
            jsr         nm_in
            sta         n_x
            stx         n_x + 1
            lda         r1
            ldx         r1 + 1
            jsr         nm_in
            sta         n_y
            stx         n_y + 1
            rts

; n_x = the caller's r0 (its copy)
nm_in1:
            lda         r0
            ldx         r0 + 1
            jsr         nm_in
            sta         n_x
            stx         n_x + 1
            rts
