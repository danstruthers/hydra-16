; ****************************************************************************
; nmcall.inc - a call into the numbers library: its start and end, its failure from however deep, and the caller's
; memory read and written.  An entry (the jump table's routines) starts with jsr nm_begin (or nm_begin0: INIT's, before
; the state is there) and ends with jmp nm_end, C and .A, .X its answer; nm_fail, .A an error (NE_*), goes back to the
; entry's caller from anywhere in it.  The caller's numbers and text are wherever it sees them: below $8000 they're read
; and written as they are; at $8000-$9FFF they're in its bank (v_bank), selected for each byte (the library's own bank
; is there the rest of the time); its numbers can't be in its paged ROM ($A000-$DFFF: the library's is there).

; ---- A call's start and end

; The library's bank selected (r13's), the caller's kept (v_bank), its zero page $70-$7F kept (v_zp), the stack as
; the entry has it noted (nm_fail goes back to it).  Then the state checked: INIT must have run (NE_INIT)
nm_begin:
            jsr         nm_begin0
            inc         v_sp                                ; (Past this return too)
            inc         v_sp
            lda         st_magic
            cmp         #NUMBERS_MAGIC
            bne         @init
            rts
@init:
            lda         #NE_INIT
            jmp         nm_fail

nm_begin0:
            lda         RAM_BANK                            ; (The caller's bank, kept once the library's is there)
            ldx         r13
            stx         RAM_BANK
            sta         v_bank
            ldx         #15
@zp:
            lda         na,x
            sta         v_zp,x
            dex
            bpl         @zp
            tsx                                             ; (The stack as the entry has it, past this return)
            inx
            inx
            stx         v_sp
            rts

; The call's end: the caller's zero page and bank back, C, .A and .X as they are
nm_end:
            php
            pha
            phx
            ldx         #15
@zp:
            lda         v_zp,x
            sta         na,x
            dex
            bpl         @zp
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

; The caller's number at .A, .X (an operand: r0's, r1's) copied to the bank's buffer at .Y (0 num_a, 1 num_b); v_len
; its length.  Not a number: NE_NOTNUM
nm_in:
            sta         np
            stx         np + 1
            phy
            jsr         nm_measure
            lda         np                                  ; (Back to its start: np less its length)
            sec
            sbc         v_len
            sta         np
            lda         np + 1
            sbc         v_len + 1
            sta         np + 1
            ply
            lda         buf_lo,y
            sta         nq
            lda         buf_hi,y
            sta         nq + 1
            MOVR        v_left, v_len
@byte:
            lda         v_left
            ora         v_left + 1
            beq         @done
            jsr         c_get
            sta         (nq)
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
            rts

; The call's end with a number, v_len bytes of the bank's buffer .Y (0 num_a, 1 num_b, 2 num_r: the result), copied
; to the caller's place r2 (its room r3): .A, .X = its length, C = 0.  No room: NE_ROOM
nm_out:
            lda         r3 + 1
            cmp         v_len + 1
            bcc         @room
            bne         :+
            lda         r3
            cmp         v_len
            bcc         @room
:
            MOVR        np, r2
            lda         buf_lo,y
            sta         nq
            lda         buf_hi,y
            sta         nq + 1
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

.rodata
buf_lo:     .lobytes    num_a, num_b, num_r
buf_hi:     .hibytes    num_a, num_b, num_r
.code
