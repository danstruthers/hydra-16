; ****************************************************************************
; act.s - (dq0w) the character's continuations, a stack (activity.h's struct pc_activity: q, nq): one pushed as it's
; planned (act_plan), the top popped as the character's free (act_next).  In assembly for the main RAM's room; the
; record's offsets below held to activity.h's by dq0w.c (ACT_AT).

            .export     _act_plan, _act_next
            .import     _act, _next_, callax

ACT_GATH    = 2                             ; (struct pc_activity's gath: trav's 4 bytes on, AK_GATH ... AK_TRAV's)
ACT_Q       = 9                             ; (q[6])
ACT_NQ      = 15                            ; (nq)

            .bss
k:          .res        1

            .code

; void __fastcall__ act_plan (unsigned char k): continuation k (AK_*) planned, its field just set: taken out of the stack
; if it was on it, and pushed
_act_plan:
            sta         k
            ldx         #0                  ; (Where the next kept goes)
            ldy         #0
@each:
            cpy         _act + ACT_NQ
            beq         @push
            lda         _act + ACT_Q,y
            cmp         k
            beq         @out
            sta         _act + ACT_Q,x
            inx
@out:
            iny
            bra         @each
@push:
            lda         k
            sta         _act + ACT_Q,x
            inx
            stx         _act + ACT_NQ
            rts

; unsigned char act_next (void): the top's handler's answer (next_[k]: 2 busy still, 1 looked at, 0 free), those under
; it whose field's 0 (done since they were planned) popped first; 0, none
_act_next:
            ldy         _act + ACT_NQ
            beq         @none
            ldx         _act + ACT_Q - 1,y
            bne         @byte
            lda         _act                ; (AK_TAKE: take, a word)
            ora         _act + 1
            bra         @is
@byte:
            lda         _act + ACT_GATH - 1,x
@is:
            bne         @go
            dec         _act + ACT_NQ       ; (Done: popped)
            bra         _act_next
@go:
            txa
            asl         a
            tay
            lda         _next_,y
            ldx         _next_ + 1,y
            jmp         callax
@none:
            lda         #0
            tax
            rts
