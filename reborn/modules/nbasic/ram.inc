; ****************************************************************************
; ram.inc - what every bank of BASIC's calls, in the task's RAM (the DATA segment): the far memory (the logical banks'
; table, two cursors that read and write across banks, a copy between them), the output, the note handler, and the
; way into an error from any bank.

OBUF_SIZE       = 128           ; The output written this much at a time
FBUF            = 64            ; (far_copy's bounce buffer)


.bss
ltab:       .res        LB_MAX                              ; Each logical bank's physical one (0: not taken yet)
cbank:      .res        1                                   ; The code's bank, the one the interpreter has at $8000
obuf:       .res        OBUF_SIZE                           ; The output not written yet ...
olen:       .res        1
ofd:        .res        1                                   ;   and its fd
intr:       .res        1                                   ; $80: Ctrl-C (the note handler's)
fbuf:       .res        FBUF                                ; (far_copy's)
fk:         .res        1

.segment "DATA"

; ---- The far memory

; Cursor A's bank at $8000 (its logical bank's: taken first if it isn't yet).  Keeps .A, .Y
ca_sel:
            pha
            ldx         cal
            lda         ltab,x
            bne         :+
            jsr         lb_take
:
            sta         RAM_BANK
            pla
            rts

cb_sel:
            pha
            ldx         cbl
            lda         ltab,x
            bne         :+
            jsr         lb_take
:
            sta         RAM_BANK
            pla
            rts

; .A = the physical bank of logical bank .X, taken now (BANKS_ALLOC).  No bank left: Out of memory.  Keeps .X, .Y
lb_take:
            phx
            phy
            lda         #1
            jsr         BANKS_ALLOC
            ply
            plx
            bcs         @none
            sta         ltab,x
            rts
@none:
            lda         #ER_MEMORY
            ldx         #0
            jmp         b_error

; .A = the byte at cursor A, the cursor on past it (into the next bank at the window's end: selected).  Its bank
; selected (ca_sel).  Keeps .X, .Y
ca_rd:
            lda         (ca)
            inc         ca
            beq         :+
            rts
:
            inc         ca + 1
            pha
            lda         ca + 1
            cmp         #WIN_END
            beq         ca_wrap
            pla
            rts
ca_wrap:
            lda         #WIN
            sta         ca + 1
            inc         cal
            phx
            jsr         ca_sel
            plx
            pla
            rts

; The byte .A put at cursor A, the cursor on past it.  Keeps .A, .X, .Y
ca_wr:
            sta         (ca)
            inc         ca
            beq         :+
            rts
:
            inc         ca + 1
            pha
            lda         ca + 1
            cmp         #WIN_END
            beq         ca_wrap
            pla
            rts

cb_rd:
            lda         (cb)
            inc         cb
            beq         :+
            rts
:
            inc         cb + 1
            pha
            lda         cb + 1
            cmp         #WIN_END
            beq         cb_wrap
            pla
            rts
cb_wrap:
            lda         #WIN
            sta         cb + 1
            inc         cbl
            phx
            jsr         cb_sel
            plx
            pla
            rts

cb_wr:
            sta         (cb)
            inc         cb
            beq         :+
            rts
:
            inc         cb + 1
            pha
            lda         cb + 1
            cmp         #WIN_END
            beq         cb_wrap
            pla
            rts

; cn bytes copied from cursor A to cursor B, both on past them; forwards, so B may be below A where they overlap
; (the collector's slide).  The code's bank selected after
far_copy:
@chunk:
            lda         cn
            ora         cn + 1
            beq         @done
            ldy         #FBUF                               ; (FBUF bytes, or what's left)
            lda         cn + 1
            bne         :+
            lda         cn
            cmp         #FBUF
            bcs         :+
            tay
:
            sty         fk
            jsr         ca_sel
            ldy         #0
:
            jsr         ca_rd
            sta         fbuf,y
            iny
            cpy         fk
            bne         :-
            jsr         cb_sel
            ldy         #0
:
            lda         fbuf,y
            jsr         cb_wr
            iny
            cpy         fk
            bne         :-
            lda         cn
            sec
            sbc         fk
            sta         cn
            bcs         @chunk
            dec         cn + 1
            bra         @chunk
@done:
code_sel:
            pha
            lda         cbank
            sta         RAM_BANK
            pla
            rts

; ---- The output: to ofd (1, stdout), a buffer at a time

; .A out.  Keeps .A, .X, .Y and the window
out_byte:
            phy
            ldy         olen
            sta         obuf,y
            iny
            sty         olen
            cpy         #OBUF_SIZE
            bcc         :+
            jsr         out_flush
:
            ply
            rts

; The output written.  Keeps .A, .X, .Y
out_flush:
            pha
            phx
            phy
            lda         olen
            beq         @done
            lda         r0
            pha
            lda         r0 + 1
            pha
            lda         r1
            pha
            lda         r1 + 1
            pha
            LDR         r0, obuf
            lda         olen
            sta         r1
            stz         r1 + 1
            lda         ofd
            jsr         WRITE
            stz         olen
            pla
            sta         r1 + 1
            pla
            sta         r1
            pla
            sta         r0 + 1
            pla
            sta         r0
@done:
            ply
            plx
            pla
            rts

; ---- The value stack (every bank's)

; vt = the top value, taken off (vsp - 5)
pop_vt:
            sec
            lda         vsp
            sbc         #VAL_SIZE
            sta         vsp
            sta         vt
            lda         vsp + 1
            sbc         #0
            sta         vsp + 1
            sta         vt + 1
            rts

; vsp on past a value
vsp_up:
            clc
            lda         vsp
            adc         #VAL_SIZE
            sta         vsp
            bcc         :+
            inc         vsp + 1
:
            rts

; vt = the top value's place (not taken off)
top_vt:
            sec
            lda         vsp
            sbc         #VAL_SIZE
            sta         vt
            lda         vsp + 1
            sbc         #0
            sta         vt + 1
            rts

; vu = the top value, taken off
pop_vu:
            sec
            lda         vsp
            sbc         #VAL_SIZE
            sta         vsp
            sta         vu
            lda         vsp + 1
            sbc         #0
            sta         vsp + 1
            sta         vu + 1
            rts

; vt = the second value from the top, vu = the top; C = 0 if both are integers
two_vals:
            sec
            lda         vsp
            sbc         #VAL_SIZE
            sta         vu
            lda         vsp + 1
            sbc         #0
            sta         vu + 1
            sec
            lda         vu
            sbc         #VAL_SIZE
            sta         vt
            lda         vu + 1
            sbc         #0
            sta         vt + 1
            lda         (vt)
            ora         (vu)
            cmp         #1
            rts

; t0/t0+1 = the integer taken off the stack (an index, a count: -32768 to 65535, a number that isn't an integer
; rounded first: Illegal function call past them)
pop_int16:
            jsr         pop_vt
            lda         (vt)
            beq         :+
            FARN        4, num_to_int                       ; (vt's value an integer, in place, if it is one)
:
            ldy         #4                                  ; (Its top bytes all its sign's: 0 or $FF)
            lda         (vt),y
            dey
            cmp         (vt),y
            bne         @bad
            cmp         #0
            beq         @pos
            cmp         #$FF
            bne         @bad
            dey                                             ; (Negative: bit 15 must be set too)
            lda         (vt),y
            bpl         @bad
            bra         @take
@pos:
@take:
            ldy         #1
            lda         (vt),y
            sta         t0
            iny
            lda         (vt),y
            sta         t0 + 1
            rts
@bad:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error

; ---- END's way out of the interpreter, from any bank: back to run_code's caller (C = 0)
run_exit:
            lda         hyx2_bank1
            sta         $01
            ldx         run_s
            txs
            clc
            rts

; ---- Notes: Ctrl-C (NOTE_INTERRUPT) noted in intr; another, the default
notes:
            cmp         #NOTE_INTERRUPT
            bne         :+
            lda         #$80
            tsb         intr
            clc
            rts
:
            sec
            rts

; ---- An error, from any bank: .A/.X its code (ER_*, or 256 + a system error's).  The first bank's err_entry takes
; it (run.inc), from however deep, through however many banks
b_error:
            pha
            lda         hyx2_bank1
            sta         $01
            pla
            jmp         err_entry
