; ****************************************************************************
; mtargs.inc - the math entries' argument and result (both banks': mtfun.inc's and mttrig.inc's)

            NM_CODE
; ---- The argument (the first and second banks': the third has its own way, mtcplx.inc's)
.ifndef CPX_BANK

; r0 read (.Y the entry: mtcplx.inc's c_go's function): n_x its copy, XN / XD its fraction (n_kv its kind).  A complex
; one: the call the third bank's (mt_tocpx)
mt_arg:
            sty         mt_cfn
            jsr         nm_in1
            jsr         n_kind
            cmp         #NK_COMPLEX
            bne         :+
            ldy         mt_cfn
            jmp         mt_tocpx
:
            lda         n_x
            ldx         n_x + 1
            ldy         #XN
            jmp         n_load

; A complex argument (or RPOW's x or y complex, or x below 0: .Y 5): the call made in the third bank, by mtcplx.inc's
; c_go, the stack as the entry had it (n_x, n_y its operands, mt_cfn .Y)
mt_tocpx:
            sty         mt_cfn
            ldx         v_sp
            txs
            ldy         mt_cfn
.ifdef NM_COPY
            NMFAR       mt_cgo, 2 - NM_COPY
.else
            NMFAR       mt_cgo, 2
.endif
            rts

.endif

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
