; ****************************************************************************
; mtargs.inc - the math entries' argument and result (both banks': mtfun.inc's and mttrig.inc's)

            NM_CODE
; ---- The argument

; r0 read: n_x its copy, XN / XD its fraction (n_kv its kind).  NE_REAL: it's complex
mt_arg:
            jsr         nm_in1
            jsr         n_kind
            cmp         #NK_COMPLEX
            bne         :+
            lda         #NE_REAL
            jmp         nm_fail
:
            lda         n_x
            ldx         n_x + 1
            ldy         #XN
            jmp         n_load

; The integer .A (0 or 1) the result, the call's end
mt_small:
            ldx         #0
            stz         nrc
            jsr         r_set16
            stz         nrc
            jsr         r_store
            jmp         nm_out

; hv the result, the call's end
mt_out:
            lda         hv
            ldx         hv + 1
            jmp         nm_out
