; ****************************************************************************
; vmxt.inc - the native code's templates (vmx.inc's), made by tools/hyvmxt.js: each its length, its flags (TF_STUB: its
; op's stub after it, its slow way; TF_NOR: not for an op with VM_R), its patches (each its offset, its kind: TPK_B
; a data byte, TPK_U one untagged, TPK_T a target's native place, TPK_N the next op's; and the data byte), its code

vxt_const:  .byte       vxt_const_e - vxt_const_c, 0, 2
            .byte       vxt_const_p0 - vxt_const_c, TPK_B, 1
            .byte       vxt_const_p1 - vxt_const_c, TPK_B, 2
vxt_const_c:
            lda         #0
vxt_const_p0 = * - 1
            sta         ex
            lda         #0
vxt_const_p1 = * - 1
            sta         ex + 1
vxt_const_e:

.assert vxt_const_e - vxt_const_c + VX_STUB + 48 < 256, error, "vxt_const: an op's code is at most 255 bytes"

vxt_push:   .byte       vxt_push_e - vxt_push_c, 0, 0
vxt_push_c:
            lda         ex
            sta         (sp)
            ldy         #1
            lda         ex + 1
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
vxt_push_e:

.assert vxt_push_e - vxt_push_c + VX_STUB + 48 < 256, error, "vxt_push: an op's code is at most 255 bytes"

vxt_cpush:  .byte       vxt_cpush_e - vxt_cpush_c, 0, 2
            .byte       vxt_cpush_p0 - vxt_cpush_c, TPK_B, 1
            .byte       vxt_cpush_p1 - vxt_cpush_c, TPK_B, 2
vxt_cpush_c:
            lda         #0
vxt_cpush_p0 = * - 1
            sta         (sp)
            ldy         #1
            lda         #0
vxt_cpush_p1 = * - 1
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
vxt_cpush_e:

.assert vxt_cpush_e - vxt_cpush_c + VX_STUB + 48 < 256, error, "vxt_cpush: an op's code is at most 255 bytes"

vxt_local:  .byte       vxt_local_e - vxt_local_c, TF_STUB | TF_NOR, 2
            .byte       vxt_local_p0 - vxt_local_c, TPK_B, 1
            .byte       vxt_local_p1 - vxt_local_c, TPK_N, 0
vxt_local_c:
            bit         vm_mat
            bmi         vxt_local_s
            ldy         #0
vxt_local_p0 = * - 1
            lda         (vm_s),y
            sta         ex
            iny
            lda         (vm_s),y
            sta         ex + 1
            jmp         $0000
vxt_local_p1 = * - 2
vxt_local_s:
vxt_local_e:

.assert vxt_local_e - vxt_local_c + VX_STUB + 48 < 256, error, "vxt_local: an op's code is at most 255 bytes"

vxt_lpush:  .byte       vxt_lpush_e - vxt_lpush_c, TF_STUB | TF_NOR, 2
            .byte       vxt_lpush_p0 - vxt_lpush_c, TPK_B, 1
            .byte       vxt_lpush_p1 - vxt_lpush_c, TPK_N, 0
vxt_lpush_c:
            bit         vm_mat
            bmi         vxt_lpush_s
            ldy         #0
vxt_lpush_p0 = * - 1
            lda         (vm_s),y
            sta         (sp)
            iny
            lda         (vm_s),y
            ldy         #1
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_lpush_p1 = * - 2
vxt_lpush_s:
vxt_lpush_e:

.assert vxt_lpush_e - vxt_lpush_c + VX_STUB + 48 < 256, error, "vxt_lpush: an op's code is at most 255 bytes"

vxt_setl:   .byte       vxt_setl_e - vxt_setl_c, TF_STUB, 2
            .byte       vxt_setl_p0 - vxt_setl_c, TPK_B, 1
            .byte       vxt_setl_p1 - vxt_setl_c, TPK_N, 0
vxt_setl_c:
            bit         vm_mat
            bmi         vxt_setl_s
            ldy         #0
vxt_setl_p0 = * - 1
            lda         ex
            sta         (vm_s),y
            iny
            lda         ex + 1
            sta         (vm_s),y
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_setl_p1 = * - 2
vxt_setl_s:
vxt_setl_e:

.assert vxt_setl_e - vxt_setl_c + VX_STUB + 48 < 256, error, "vxt_setl: an op's code is at most 255 bytes"

vxt_jmp:    .byte       vxt_jmp_e - vxt_jmp_c, 0, 1
            .byte       vxt_jmp_p0 - vxt_jmp_c, TPK_T, 1
vxt_jmp_c:
            jmp         $0000
vxt_jmp_p0 = * - 2
vxt_jmp_e:

.assert vxt_jmp_e - vxt_jmp_c + VX_STUB + 48 < 256, error, "vxt_jmp: an op's code is at most 255 bytes"

vxt_jf:     .byte       vxt_jf_e - vxt_jf_c, 0, 1
            .byte       vxt_jf_p0 - vxt_jf_c, TPK_T, 1
vxt_jf_c:
            lda         ex + 1
            bne         vxt_jf_no
            lda         ex
            beq         vxt_jf_to
            cmp         #<SNIL
            bne         vxt_jf_no
vxt_jf_to:
            jmp         $0000
vxt_jf_p0 = * - 2
vxt_jf_no:
vxt_jf_e:

.assert vxt_jf_e - vxt_jf_c + VX_STUB + 48 < 256, error, "vxt_jf: an op's code is at most 255 bytes"

vxt_jt:     .byte       vxt_jt_e - vxt_jt_c, 0, 1
            .byte       vxt_jt_p0 - vxt_jt_c, TPK_T, 1
vxt_jt_c:
            lda         ex + 1
            bne         vxt_jt_to
            lda         ex
            beq         vxt_jt_no
            cmp         #<SNIL
            beq         vxt_jt_no
vxt_jt_to:
            jmp         $0000
vxt_jt_p0 = * - 2
vxt_jt_no:
vxt_jt_e:

.assert vxt_jt_e - vxt_jt_c + VX_STUB + 48 < 256, error, "vxt_jt: an op's code is at most 255 bytes"

vxt_loop:   .byte       vxt_loop_e - vxt_loop_c, TF_STUB, 1
            .byte       vxt_loop_p0 - vxt_loop_c, TPK_T, 1
vxt_loop_c:
            bit         intr
            bmi         vxt_loop_s
            jmp         $0000
vxt_loop_p0 = * - 2
vxt_loop_s:
vxt_loop_e:

.assert vxt_loop_e - vxt_loop_c + VX_STUB + 48 < 256, error, "vxt_loop: an op's code is at most 255 bytes"

vxt_doti:   .byte       vxt_doti_e - vxt_doti_c, 0, 1
            .byte       vxt_doti_p0 - vxt_doti_c, TPK_T, 1
vxt_doti_c:
            lda         sp
            sec
            sbc         #4
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #2
            lda         (ht),y
            sta         ex
            cmp         (ht)
            iny
            lda         (ht),y
            sta         ex + 1
            ldy         #1
            sbc         (ht),y
            bvc         :+
            eor         #$80
:
            bmi         vxt_doti_in
            jmp         $0000
vxt_doti_p0 = * - 2
vxt_doti_in:
vxt_doti_e:

.assert vxt_doti_e - vxt_doti_c + VX_STUB + 48 < 256, error, "vxt_doti: an op's code is at most 255 bytes"

vxt_dotinc: .byte       vxt_dotinc_e - vxt_dotinc_c, 0, 0
vxt_dotinc_c:
            lda         sp
            sec
            sbc         #2
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)
            clc
            adc         #2
            sta         (ht)
            ldy         #1
            lda         (ht),y
            adc         #0
            sta         (ht),y
vxt_dotinc_e:

.assert vxt_dotinc_e - vxt_dotinc_c + VX_STUB + 48 < 256, error, "vxt_dotinc: an op's code is at most 255 bytes"

vxt_stt:    .byte       vxt_stt_e - vxt_stt_c, 0, 0
vxt_stt_c:
            lda         sp
            sec
            sbc         #2
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         ex
            sta         (ht)
            ldy         #1
            lda         ex + 1
            sta         (ht),y
vxt_stt_e:

.assert vxt_stt_e - vxt_stt_c + VX_STUB + 48 < 256, error, "vxt_stt: an op's code is at most 255 bytes"

vxt_popx:   .byte       vxt_popx_e - vxt_popx_c, 0, 0
vxt_popx_c:
            jsr         pop
            sta         ex
            stx         ex + 1
vxt_popx_e:

.assert vxt_popx_e - vxt_popx_c + VX_STUB + 48 < 256, error, "vxt_popx: an op's code is at most 255 bytes"

vxt_drop:   .byte       vxt_drop_e - vxt_drop_c, 0, 1
            .byte       vxt_drop_p0 - vxt_drop_c, TPK_B, 1
vxt_drop_c:
            lda         #0
vxt_drop_p0 = * - 1
            jsr         drop
            stz         ex
            stz         ex + 1
vxt_drop_e:

.assert vxt_drop_e - vxt_drop_c + VX_STUB + 48 < 256, error, "vxt_drop: an op's code is at most 255 bytes"

vxt_jlq_zerop:.byte       vxt_jlq_zerop_e - vxt_jlq_zerop_c, TF_STUB, 3
            .byte       vxt_jlq_zerop_p0 - vxt_jlq_zerop_c, TPK_B, 2
            .byte       vxt_jlq_zerop_p1 - vxt_jlq_zerop_c, TPK_N, 0
            .byte       vxt_jlq_zerop_p2 - vxt_jlq_zerop_c, TPK_T, 5
vxt_jlq_zerop_c:
            bit         vm_mat
            bmi         vxt_jlq_zerop_s
            ldy         #0
vxt_jlq_zerop_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_jlq_zerop_s
            cmp         #<FIX(0)
            bne         vxt_jlq_zerop_f
            iny
            lda         (vm_s),y
            cmp         #0
            bne         vxt_jlq_zerop_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_zerop_p1 = * - 2
vxt_jlq_zerop_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_zerop_p2 = * - 2
vxt_jlq_zerop_s:
vxt_jlq_zerop_e:

.assert vxt_jlq_zerop_e - vxt_jlq_zerop_c + VX_STUB + 48 < 256, error, "vxt_jlq_zerop: an op's code is at most 255 bytes"

vxt_jq_zerop:.byte       vxt_jq_zerop_e - vxt_jq_zerop_c, TF_STUB, 2
            .byte       vxt_jq_zerop_p0 - vxt_jq_zerop_c, TPK_N, 0
            .byte       vxt_jq_zerop_p1 - vxt_jq_zerop_c, TPK_T, 4
vxt_jq_zerop_c:
            lda         ex
            bit         #1
            beq         vxt_jq_zerop_s
            cmp         #<FIX(0)
            bne         vxt_jq_zerop_f
            lda         ex + 1
            cmp         #0
            bne         vxt_jq_zerop_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_zerop_p0 = * - 2
vxt_jq_zerop_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_zerop_p1 = * - 2
vxt_jq_zerop_s:
vxt_jq_zerop_e:

.assert vxt_jq_zerop_e - vxt_jq_zerop_c + VX_STUB + 48 < 256, error, "vxt_jq_zerop: an op's code is at most 255 bytes"

vxt_jlq_onep:.byte       vxt_jlq_onep_e - vxt_jlq_onep_c, TF_STUB, 3
            .byte       vxt_jlq_onep_p0 - vxt_jlq_onep_c, TPK_B, 2
            .byte       vxt_jlq_onep_p1 - vxt_jlq_onep_c, TPK_N, 0
            .byte       vxt_jlq_onep_p2 - vxt_jlq_onep_c, TPK_T, 5
vxt_jlq_onep_c:
            bit         vm_mat
            bmi         vxt_jlq_onep_s
            ldy         #0
vxt_jlq_onep_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_jlq_onep_s
            cmp         #<FIX(1)
            bne         vxt_jlq_onep_f
            iny
            lda         (vm_s),y
            cmp         #0
            bne         vxt_jlq_onep_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_onep_p1 = * - 2
vxt_jlq_onep_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_onep_p2 = * - 2
vxt_jlq_onep_s:
vxt_jlq_onep_e:

.assert vxt_jlq_onep_e - vxt_jlq_onep_c + VX_STUB + 48 < 256, error, "vxt_jlq_onep: an op's code is at most 255 bytes"

vxt_jq_onep:.byte       vxt_jq_onep_e - vxt_jq_onep_c, TF_STUB, 2
            .byte       vxt_jq_onep_p0 - vxt_jq_onep_c, TPK_N, 0
            .byte       vxt_jq_onep_p1 - vxt_jq_onep_c, TPK_T, 4
vxt_jq_onep_c:
            lda         ex
            bit         #1
            beq         vxt_jq_onep_s
            cmp         #<FIX(1)
            bne         vxt_jq_onep_f
            lda         ex + 1
            cmp         #0
            bne         vxt_jq_onep_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_onep_p0 = * - 2
vxt_jq_onep_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_onep_p1 = * - 2
vxt_jq_onep_s:
vxt_jq_onep_e:

.assert vxt_jq_onep_e - vxt_jq_onep_c + VX_STUB + 48 < 256, error, "vxt_jq_onep: an op's code is at most 255 bytes"

vxt_jlq_ltc:.byte       vxt_jlq_ltc_e - vxt_jlq_ltc_c, TF_STUB, 5
            .byte       vxt_jlq_ltc_p0 - vxt_jlq_ltc_c, TPK_B, 2
            .byte       vxt_jlq_ltc_p1 - vxt_jlq_ltc_c, TPK_B, 3
            .byte       vxt_jlq_ltc_p2 - vxt_jlq_ltc_c, TPK_B, 4
            .byte       vxt_jlq_ltc_p3 - vxt_jlq_ltc_c, TPK_N, 0
            .byte       vxt_jlq_ltc_p4 - vxt_jlq_ltc_c, TPK_T, 5
vxt_jlq_ltc_c:
            bit         vm_mat
            bmi         vxt_jlq_ltc_s
            ldy         #0
vxt_jlq_ltc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_jlq_ltc_s
            cmp         #0
vxt_jlq_ltc_p1 = * - 1
            iny
            lda         (vm_s),y
            sbc         #0
vxt_jlq_ltc_p2 = * - 1
            bvc         :+
            eor         #$80
:
            bpl         vxt_jlq_ltc_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_ltc_p3 = * - 2
vxt_jlq_ltc_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_ltc_p4 = * - 2
vxt_jlq_ltc_s:
vxt_jlq_ltc_e:

.assert vxt_jlq_ltc_e - vxt_jlq_ltc_c + VX_STUB + 48 < 256, error, "vxt_jlq_ltc: an op's code is at most 255 bytes"

vxt_jq_ltc: .byte       vxt_jq_ltc_e - vxt_jq_ltc_c, TF_STUB, 4
            .byte       vxt_jq_ltc_p0 - vxt_jq_ltc_c, TPK_B, 2
            .byte       vxt_jq_ltc_p1 - vxt_jq_ltc_c, TPK_B, 3
            .byte       vxt_jq_ltc_p2 - vxt_jq_ltc_c, TPK_N, 0
            .byte       vxt_jq_ltc_p3 - vxt_jq_ltc_c, TPK_T, 4
vxt_jq_ltc_c:
            lda         ex
            bit         #1
            beq         vxt_jq_ltc_s
            cmp         #0
vxt_jq_ltc_p0 = * - 1
            lda         ex + 1
            sbc         #0
vxt_jq_ltc_p1 = * - 1
            bvc         :+
            eor         #$80
:
            bpl         vxt_jq_ltc_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_ltc_p2 = * - 2
vxt_jq_ltc_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_ltc_p3 = * - 2
vxt_jq_ltc_s:
vxt_jq_ltc_e:

.assert vxt_jq_ltc_e - vxt_jq_ltc_c + VX_STUB + 48 < 256, error, "vxt_jq_ltc: an op's code is at most 255 bytes"

vxt_jlq_gtc:.byte       vxt_jlq_gtc_e - vxt_jlq_gtc_c, TF_STUB, 5
            .byte       vxt_jlq_gtc_p0 - vxt_jlq_gtc_c, TPK_B, 2
            .byte       vxt_jlq_gtc_p1 - vxt_jlq_gtc_c, TPK_B, 3
            .byte       vxt_jlq_gtc_p2 - vxt_jlq_gtc_c, TPK_B, 4
            .byte       vxt_jlq_gtc_p3 - vxt_jlq_gtc_c, TPK_N, 0
            .byte       vxt_jlq_gtc_p4 - vxt_jlq_gtc_c, TPK_T, 5
vxt_jlq_gtc_c:
            bit         vm_mat
            bmi         vxt_jlq_gtc_s
            ldy         #0
vxt_jlq_gtc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_jlq_gtc_s
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         #0
vxt_jlq_gtc_p1 = * - 1
            cmp         ht
            lda         #0
vxt_jlq_gtc_p2 = * - 1
            sbc         ht + 1
            bvc         :+
            eor         #$80
:
            bpl         vxt_jlq_gtc_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_gtc_p3 = * - 2
vxt_jlq_gtc_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_gtc_p4 = * - 2
vxt_jlq_gtc_s:
vxt_jlq_gtc_e:

.assert vxt_jlq_gtc_e - vxt_jlq_gtc_c + VX_STUB + 48 < 256, error, "vxt_jlq_gtc: an op's code is at most 255 bytes"

vxt_jq_gtc: .byte       vxt_jq_gtc_e - vxt_jq_gtc_c, TF_STUB, 4
            .byte       vxt_jq_gtc_p0 - vxt_jq_gtc_c, TPK_B, 2
            .byte       vxt_jq_gtc_p1 - vxt_jq_gtc_c, TPK_B, 3
            .byte       vxt_jq_gtc_p2 - vxt_jq_gtc_c, TPK_N, 0
            .byte       vxt_jq_gtc_p3 - vxt_jq_gtc_c, TPK_T, 4
vxt_jq_gtc_c:
            lda         ex
            bit         #1
            beq         vxt_jq_gtc_s
            lda         #0
vxt_jq_gtc_p0 = * - 1
            cmp         ex
            lda         #0
vxt_jq_gtc_p1 = * - 1
            sbc         ex + 1
            bvc         :+
            eor         #$80
:
            bpl         vxt_jq_gtc_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_gtc_p2 = * - 2
vxt_jq_gtc_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_gtc_p3 = * - 2
vxt_jq_gtc_s:
vxt_jq_gtc_e:

.assert vxt_jq_gtc_e - vxt_jq_gtc_c + VX_STUB + 48 < 256, error, "vxt_jq_gtc: an op's code is at most 255 bytes"

vxt_jlq_lec:.byte       vxt_jlq_lec_e - vxt_jlq_lec_c, TF_STUB, 5
            .byte       vxt_jlq_lec_p0 - vxt_jlq_lec_c, TPK_B, 2
            .byte       vxt_jlq_lec_p1 - vxt_jlq_lec_c, TPK_B, 3
            .byte       vxt_jlq_lec_p2 - vxt_jlq_lec_c, TPK_B, 4
            .byte       vxt_jlq_lec_p3 - vxt_jlq_lec_c, TPK_N, 0
            .byte       vxt_jlq_lec_p4 - vxt_jlq_lec_c, TPK_T, 5
vxt_jlq_lec_c:
            bit         vm_mat
            bmi         vxt_jlq_lec_s
            ldy         #0
vxt_jlq_lec_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_jlq_lec_s
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         #0
vxt_jlq_lec_p1 = * - 1
            cmp         ht
            lda         #0
vxt_jlq_lec_p2 = * - 1
            sbc         ht + 1
            bvc         :+
            eor         #$80
:
            bmi         vxt_jlq_lec_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_lec_p3 = * - 2
vxt_jlq_lec_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_lec_p4 = * - 2
vxt_jlq_lec_s:
vxt_jlq_lec_e:

.assert vxt_jlq_lec_e - vxt_jlq_lec_c + VX_STUB + 48 < 256, error, "vxt_jlq_lec: an op's code is at most 255 bytes"

vxt_jq_lec: .byte       vxt_jq_lec_e - vxt_jq_lec_c, TF_STUB, 4
            .byte       vxt_jq_lec_p0 - vxt_jq_lec_c, TPK_B, 2
            .byte       vxt_jq_lec_p1 - vxt_jq_lec_c, TPK_B, 3
            .byte       vxt_jq_lec_p2 - vxt_jq_lec_c, TPK_N, 0
            .byte       vxt_jq_lec_p3 - vxt_jq_lec_c, TPK_T, 4
vxt_jq_lec_c:
            lda         ex
            bit         #1
            beq         vxt_jq_lec_s
            lda         #0
vxt_jq_lec_p0 = * - 1
            cmp         ex
            lda         #0
vxt_jq_lec_p1 = * - 1
            sbc         ex + 1
            bvc         :+
            eor         #$80
:
            bmi         vxt_jq_lec_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_lec_p2 = * - 2
vxt_jq_lec_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_lec_p3 = * - 2
vxt_jq_lec_s:
vxt_jq_lec_e:

.assert vxt_jq_lec_e - vxt_jq_lec_c + VX_STUB + 48 < 256, error, "vxt_jq_lec: an op's code is at most 255 bytes"

vxt_jlq_gec:.byte       vxt_jlq_gec_e - vxt_jlq_gec_c, TF_STUB, 5
            .byte       vxt_jlq_gec_p0 - vxt_jlq_gec_c, TPK_B, 2
            .byte       vxt_jlq_gec_p1 - vxt_jlq_gec_c, TPK_B, 3
            .byte       vxt_jlq_gec_p2 - vxt_jlq_gec_c, TPK_B, 4
            .byte       vxt_jlq_gec_p3 - vxt_jlq_gec_c, TPK_N, 0
            .byte       vxt_jlq_gec_p4 - vxt_jlq_gec_c, TPK_T, 5
vxt_jlq_gec_c:
            bit         vm_mat
            bmi         vxt_jlq_gec_s
            ldy         #0
vxt_jlq_gec_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_jlq_gec_s
            cmp         #0
vxt_jlq_gec_p1 = * - 1
            iny
            lda         (vm_s),y
            sbc         #0
vxt_jlq_gec_p2 = * - 1
            bvc         :+
            eor         #$80
:
            bmi         vxt_jlq_gec_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_gec_p3 = * - 2
vxt_jlq_gec_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_gec_p4 = * - 2
vxt_jlq_gec_s:
vxt_jlq_gec_e:

.assert vxt_jlq_gec_e - vxt_jlq_gec_c + VX_STUB + 48 < 256, error, "vxt_jlq_gec: an op's code is at most 255 bytes"

vxt_jq_gec: .byte       vxt_jq_gec_e - vxt_jq_gec_c, TF_STUB, 4
            .byte       vxt_jq_gec_p0 - vxt_jq_gec_c, TPK_B, 2
            .byte       vxt_jq_gec_p1 - vxt_jq_gec_c, TPK_B, 3
            .byte       vxt_jq_gec_p2 - vxt_jq_gec_c, TPK_N, 0
            .byte       vxt_jq_gec_p3 - vxt_jq_gec_c, TPK_T, 4
vxt_jq_gec_c:
            lda         ex
            bit         #1
            beq         vxt_jq_gec_s
            cmp         #0
vxt_jq_gec_p0 = * - 1
            lda         ex + 1
            sbc         #0
vxt_jq_gec_p1 = * - 1
            bvc         :+
            eor         #$80
:
            bmi         vxt_jq_gec_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_gec_p2 = * - 2
vxt_jq_gec_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_gec_p3 = * - 2
vxt_jq_gec_s:
vxt_jq_gec_e:

.assert vxt_jq_gec_e - vxt_jq_gec_c + VX_STUB + 48 < 256, error, "vxt_jq_gec: an op's code is at most 255 bytes"

vxt_jlq_eqc:.byte       vxt_jlq_eqc_e - vxt_jlq_eqc_c, TF_STUB, 5
            .byte       vxt_jlq_eqc_p0 - vxt_jlq_eqc_c, TPK_B, 2
            .byte       vxt_jlq_eqc_p1 - vxt_jlq_eqc_c, TPK_B, 3
            .byte       vxt_jlq_eqc_p2 - vxt_jlq_eqc_c, TPK_B, 4
            .byte       vxt_jlq_eqc_p3 - vxt_jlq_eqc_c, TPK_N, 0
            .byte       vxt_jlq_eqc_p4 - vxt_jlq_eqc_c, TPK_T, 5
vxt_jlq_eqc_c:
            bit         vm_mat
            bmi         vxt_jlq_eqc_s
            ldy         #0
vxt_jlq_eqc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_jlq_eqc_s
            cmp         #0
vxt_jlq_eqc_p1 = * - 1
            bne         vxt_jlq_eqc_f
            iny
            lda         (vm_s),y
            cmp         #0
vxt_jlq_eqc_p2 = * - 1
            bne         vxt_jlq_eqc_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_eqc_p3 = * - 2
vxt_jlq_eqc_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jlq_eqc_p4 = * - 2
vxt_jlq_eqc_s:
vxt_jlq_eqc_e:

.assert vxt_jlq_eqc_e - vxt_jlq_eqc_c + VX_STUB + 48 < 256, error, "vxt_jlq_eqc: an op's code is at most 255 bytes"

vxt_jq_eqc: .byte       vxt_jq_eqc_e - vxt_jq_eqc_c, TF_STUB, 4
            .byte       vxt_jq_eqc_p0 - vxt_jq_eqc_c, TPK_B, 2
            .byte       vxt_jq_eqc_p1 - vxt_jq_eqc_c, TPK_B, 3
            .byte       vxt_jq_eqc_p2 - vxt_jq_eqc_c, TPK_N, 0
            .byte       vxt_jq_eqc_p3 - vxt_jq_eqc_c, TPK_T, 4
vxt_jq_eqc_c:
            lda         ex
            bit         #1
            beq         vxt_jq_eqc_s
            cmp         #0
vxt_jq_eqc_p0 = * - 1
            bne         vxt_jq_eqc_f
            lda         ex + 1
            cmp         #0
vxt_jq_eqc_p1 = * - 1
            bne         vxt_jq_eqc_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_eqc_p2 = * - 2
vxt_jq_eqc_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jq_eqc_p3 = * - 2
vxt_jq_eqc_s:
vxt_jq_eqc_e:

.assert vxt_jq_eqc_e - vxt_jq_eqc_c + VX_STUB + 48 < 256, error, "vxt_jq_eqc: an op's code is at most 255 bytes"

vxt_lq_addc:.byte       vxt_lq_addc_e - vxt_lq_addc_c, TF_STUB, 4
            .byte       vxt_lq_addc_p0 - vxt_lq_addc_c, TPK_B, 2
            .byte       vxt_lq_addc_p1 - vxt_lq_addc_c, TPK_U, 3
            .byte       vxt_lq_addc_p2 - vxt_lq_addc_c, TPK_B, 4
            .byte       vxt_lq_addc_p3 - vxt_lq_addc_c, TPK_N, 0
vxt_lq_addc_c:
            bit         vm_mat
            bmi         vxt_lq_addc_s
            ldy         #0
vxt_lq_addc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_lq_addc_s
            clc
            adc         #0
vxt_lq_addc_p1 = * - 1
            sta         ex
            iny
            lda         (vm_s),y
            adc         #0
vxt_lq_addc_p2 = * - 1
            bvs         vxt_lq_addc_s
            sta         ex + 1
            jmp         $0000
vxt_lq_addc_p3 = * - 2
vxt_lq_addc_s:
vxt_lq_addc_e:

.assert vxt_lq_addc_e - vxt_lq_addc_c + VX_STUB + 48 < 256, error, "vxt_lq_addc: an op's code is at most 255 bytes"

vxt_lqp_addc:.byte       vxt_lqp_addc_e - vxt_lqp_addc_c, TF_STUB, 4
            .byte       vxt_lqp_addc_p0 - vxt_lqp_addc_c, TPK_B, 2
            .byte       vxt_lqp_addc_p1 - vxt_lqp_addc_c, TPK_U, 3
            .byte       vxt_lqp_addc_p2 - vxt_lqp_addc_c, TPK_B, 4
            .byte       vxt_lqp_addc_p3 - vxt_lqp_addc_c, TPK_N, 0
vxt_lqp_addc_c:
            bit         vm_mat
            bmi         vxt_lqp_addc_s
            ldy         #0
vxt_lqp_addc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_lqp_addc_s
            clc
            adc         #0
vxt_lqp_addc_p1 = * - 1
            sta         ex
            iny
            lda         (vm_s),y
            adc         #0
vxt_lqp_addc_p2 = * - 1
            bvs         vxt_lqp_addc_s
            sta         ex + 1
            ldy         #1
            sta         (sp),y
            lda         ex
            sta         (sp)
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_lqp_addc_p3 = * - 2
vxt_lqp_addc_s:
vxt_lqp_addc_e:

.assert vxt_lqp_addc_e - vxt_lqp_addc_c + VX_STUB + 48 < 256, error, "vxt_lqp_addc: an op's code is at most 255 bytes"

vxt_lq_subc:.byte       vxt_lq_subc_e - vxt_lq_subc_c, TF_STUB, 4
            .byte       vxt_lq_subc_p0 - vxt_lq_subc_c, TPK_B, 2
            .byte       vxt_lq_subc_p1 - vxt_lq_subc_c, TPK_U, 3
            .byte       vxt_lq_subc_p2 - vxt_lq_subc_c, TPK_B, 4
            .byte       vxt_lq_subc_p3 - vxt_lq_subc_c, TPK_N, 0
vxt_lq_subc_c:
            bit         vm_mat
            bmi         vxt_lq_subc_s
            ldy         #0
vxt_lq_subc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_lq_subc_s
            sec
            sbc         #0
vxt_lq_subc_p1 = * - 1
            sta         ex
            iny
            lda         (vm_s),y
            sbc         #0
vxt_lq_subc_p2 = * - 1
            bvs         vxt_lq_subc_s
            sta         ex + 1
            jmp         $0000
vxt_lq_subc_p3 = * - 2
vxt_lq_subc_s:
vxt_lq_subc_e:

.assert vxt_lq_subc_e - vxt_lq_subc_c + VX_STUB + 48 < 256, error, "vxt_lq_subc: an op's code is at most 255 bytes"

vxt_lqp_subc:.byte       vxt_lqp_subc_e - vxt_lqp_subc_c, TF_STUB, 4
            .byte       vxt_lqp_subc_p0 - vxt_lqp_subc_c, TPK_B, 2
            .byte       vxt_lqp_subc_p1 - vxt_lqp_subc_c, TPK_U, 3
            .byte       vxt_lqp_subc_p2 - vxt_lqp_subc_c, TPK_B, 4
            .byte       vxt_lqp_subc_p3 - vxt_lqp_subc_c, TPK_N, 0
vxt_lqp_subc_c:
            bit         vm_mat
            bmi         vxt_lqp_subc_s
            ldy         #0
vxt_lqp_subc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_lqp_subc_s
            sec
            sbc         #0
vxt_lqp_subc_p1 = * - 1
            sta         ex
            iny
            lda         (vm_s),y
            sbc         #0
vxt_lqp_subc_p2 = * - 1
            bvs         vxt_lqp_subc_s
            sta         ex + 1
            ldy         #1
            sta         (sp),y
            lda         ex
            sta         (sp)
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_lqp_subc_p3 = * - 2
vxt_lqp_subc_s:
vxt_lqp_subc_e:

.assert vxt_lqp_subc_e - vxt_lqp_subc_c + VX_STUB + 48 < 256, error, "vxt_lqp_subc: an op's code is at most 255 bytes"

vxt_lq_inc: .byte       vxt_lq_inc_e - vxt_lq_inc_c, TF_STUB, 2
            .byte       vxt_lq_inc_p0 - vxt_lq_inc_c, TPK_B, 2
            .byte       vxt_lq_inc_p1 - vxt_lq_inc_c, TPK_N, 0
vxt_lq_inc_c:
            bit         vm_mat
            bmi         vxt_lq_inc_s
            ldy         #0
vxt_lq_inc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_lq_inc_s
            clc
            adc         #2
            sta         ex
            iny
            lda         (vm_s),y
            adc         #0
            bvs         vxt_lq_inc_s
            sta         ex + 1
            jmp         $0000
vxt_lq_inc_p1 = * - 2
vxt_lq_inc_s:
vxt_lq_inc_e:

.assert vxt_lq_inc_e - vxt_lq_inc_c + VX_STUB + 48 < 256, error, "vxt_lq_inc: an op's code is at most 255 bytes"

vxt_lqp_inc:.byte       vxt_lqp_inc_e - vxt_lqp_inc_c, TF_STUB, 2
            .byte       vxt_lqp_inc_p0 - vxt_lqp_inc_c, TPK_B, 2
            .byte       vxt_lqp_inc_p1 - vxt_lqp_inc_c, TPK_N, 0
vxt_lqp_inc_c:
            bit         vm_mat
            bmi         vxt_lqp_inc_s
            ldy         #0
vxt_lqp_inc_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_lqp_inc_s
            clc
            adc         #2
            sta         ex
            iny
            lda         (vm_s),y
            adc         #0
            bvs         vxt_lqp_inc_s
            sta         ex + 1
            ldy         #1
            sta         (sp),y
            lda         ex
            sta         (sp)
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_lqp_inc_p1 = * - 2
vxt_lqp_inc_s:
vxt_lqp_inc_e:

.assert vxt_lqp_inc_e - vxt_lqp_inc_c + VX_STUB + 48 < 256, error, "vxt_lqp_inc: an op's code is at most 255 bytes"

vxt_lq_dec: .byte       vxt_lq_dec_e - vxt_lq_dec_c, TF_STUB, 2
            .byte       vxt_lq_dec_p0 - vxt_lq_dec_c, TPK_B, 2
            .byte       vxt_lq_dec_p1 - vxt_lq_dec_c, TPK_N, 0
vxt_lq_dec_c:
            bit         vm_mat
            bmi         vxt_lq_dec_s
            ldy         #0
vxt_lq_dec_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_lq_dec_s
            sec
            sbc         #2
            sta         ex
            iny
            lda         (vm_s),y
            sbc         #0
            bvs         vxt_lq_dec_s
            sta         ex + 1
            jmp         $0000
vxt_lq_dec_p1 = * - 2
vxt_lq_dec_s:
vxt_lq_dec_e:

.assert vxt_lq_dec_e - vxt_lq_dec_c + VX_STUB + 48 < 256, error, "vxt_lq_dec: an op's code is at most 255 bytes"

vxt_lqp_dec:.byte       vxt_lqp_dec_e - vxt_lqp_dec_c, TF_STUB, 2
            .byte       vxt_lqp_dec_p0 - vxt_lqp_dec_c, TPK_B, 2
            .byte       vxt_lqp_dec_p1 - vxt_lqp_dec_c, TPK_N, 0
vxt_lqp_dec_c:
            bit         vm_mat
            bmi         vxt_lqp_dec_s
            ldy         #0
vxt_lqp_dec_p0 = * - 1
            lda         (vm_s),y
            bit         #1
            beq         vxt_lqp_dec_s
            sec
            sbc         #2
            sta         ex
            iny
            lda         (vm_s),y
            sbc         #0
            bvs         vxt_lqp_dec_s
            sta         ex + 1
            ldy         #1
            sta         (sp),y
            lda         ex
            sta         (sp)
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_lqp_dec_p1 = * - 2
vxt_lqp_dec_s:
vxt_lqp_dec_e:

.assert vxt_lqp_dec_e - vxt_lqp_dec_c + VX_STUB + 48 < 256, error, "vxt_lqp_dec: an op's code is at most 255 bytes"

vxt_ll_add: .byte       vxt_ll_add_e - vxt_ll_add_c, TF_STUB, 3
            .byte       vxt_ll_add_p0 - vxt_ll_add_c, TPK_B, 2
            .byte       vxt_ll_add_p1 - vxt_ll_add_c, TPK_B, 3
            .byte       vxt_ll_add_p2 - vxt_ll_add_c, TPK_N, 0
vxt_ll_add_c:
            bit         vm_mat
            bmi         vxt_ll_add_s
            ldy         #0
vxt_ll_add_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_ll_add_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_ll_add_s
            lda         (vm_s),y
            and         #$FE
            clc
            adc         ht
            sta         ex
            iny
            lda         (vm_s),y
            adc         ht + 1
            bvs         vxt_ll_add_s
            sta         ex + 1
            jmp         $0000
vxt_ll_add_p2 = * - 2
vxt_ll_add_s:
vxt_ll_add_e:

.assert vxt_ll_add_e - vxt_ll_add_c + VX_STUB + 48 < 256, error, "vxt_ll_add: an op's code is at most 255 bytes"

vxt_llp_add:.byte       vxt_llp_add_e - vxt_llp_add_c, TF_STUB, 3
            .byte       vxt_llp_add_p0 - vxt_llp_add_c, TPK_B, 2
            .byte       vxt_llp_add_p1 - vxt_llp_add_c, TPK_B, 3
            .byte       vxt_llp_add_p2 - vxt_llp_add_c, TPK_N, 0
vxt_llp_add_c:
            bit         vm_mat
            bmi         vxt_llp_add_s
            ldy         #0
vxt_llp_add_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_llp_add_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_llp_add_s
            lda         (vm_s),y
            and         #$FE
            clc
            adc         ht
            sta         ex
            iny
            lda         (vm_s),y
            adc         ht + 1
            bvs         vxt_llp_add_s
            sta         ex + 1
            lda         ex
            sta         (sp)
            ldy         #1
            lda         ex + 1
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_llp_add_p2 = * - 2
vxt_llp_add_s:
vxt_llp_add_e:

.assert vxt_llp_add_e - vxt_llp_add_c + VX_STUB + 48 < 256, error, "vxt_llp_add: an op's code is at most 255 bytes"

vxt_ll_sub: .byte       vxt_ll_sub_e - vxt_ll_sub_c, TF_STUB, 3
            .byte       vxt_ll_sub_p0 - vxt_ll_sub_c, TPK_B, 2
            .byte       vxt_ll_sub_p1 - vxt_ll_sub_c, TPK_B, 3
            .byte       vxt_ll_sub_p2 - vxt_ll_sub_c, TPK_N, 0
vxt_ll_sub_c:
            bit         vm_mat
            bmi         vxt_ll_sub_s
            ldy         #0
vxt_ll_sub_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_ll_sub_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_ll_sub_s
            lda         ht
            sec
            sbc         (vm_s),y
            ora         #1
            sta         ex
            iny
            lda         ht + 1
            sbc         (vm_s),y
            bvs         vxt_ll_sub_s
            sta         ex + 1
            jmp         $0000
vxt_ll_sub_p2 = * - 2
vxt_ll_sub_s:
vxt_ll_sub_e:

.assert vxt_ll_sub_e - vxt_ll_sub_c + VX_STUB + 48 < 256, error, "vxt_ll_sub: an op's code is at most 255 bytes"

vxt_llp_sub:.byte       vxt_llp_sub_e - vxt_llp_sub_c, TF_STUB, 3
            .byte       vxt_llp_sub_p0 - vxt_llp_sub_c, TPK_B, 2
            .byte       vxt_llp_sub_p1 - vxt_llp_sub_c, TPK_B, 3
            .byte       vxt_llp_sub_p2 - vxt_llp_sub_c, TPK_N, 0
vxt_llp_sub_c:
            bit         vm_mat
            bmi         vxt_llp_sub_s
            ldy         #0
vxt_llp_sub_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_llp_sub_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_llp_sub_s
            lda         ht
            sec
            sbc         (vm_s),y
            ora         #1
            sta         ex
            iny
            lda         ht + 1
            sbc         (vm_s),y
            bvs         vxt_llp_sub_s
            sta         ex + 1
            lda         ex
            sta         (sp)
            ldy         #1
            lda         ex + 1
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_llp_sub_p2 = * - 2
vxt_llp_sub_s:
vxt_llp_sub_e:

.assert vxt_llp_sub_e - vxt_llp_sub_c + VX_STUB + 48 < 256, error, "vxt_llp_sub: an op's code is at most 255 bytes"

vxt_jll_lt: .byte       vxt_jll_lt_e - vxt_jll_lt_c, TF_STUB, 4
            .byte       vxt_jll_lt_p0 - vxt_jll_lt_c, TPK_B, 2
            .byte       vxt_jll_lt_p1 - vxt_jll_lt_c, TPK_B, 3
            .byte       vxt_jll_lt_p2 - vxt_jll_lt_c, TPK_N, 0
            .byte       vxt_jll_lt_p3 - vxt_jll_lt_c, TPK_T, 4
vxt_jll_lt_c:
            bit         vm_mat
            bmi         vxt_jll_lt_s
            ldy         #0
vxt_jll_lt_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_jll_lt_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_jll_lt_s
            lda         ht
            cmp         (vm_s),y
            iny
            lda         ht + 1
            sbc         (vm_s),y
            bvc         :+
            eor         #$80
:
            bpl         vxt_jll_lt_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_lt_p2 = * - 2
vxt_jll_lt_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_lt_p3 = * - 2
vxt_jll_lt_s:
vxt_jll_lt_e:

.assert vxt_jll_lt_e - vxt_jll_lt_c + VX_STUB + 48 < 256, error, "vxt_jll_lt: an op's code is at most 255 bytes"

vxt_jll_ge: .byte       vxt_jll_ge_e - vxt_jll_ge_c, TF_STUB, 4
            .byte       vxt_jll_ge_p0 - vxt_jll_ge_c, TPK_B, 2
            .byte       vxt_jll_ge_p1 - vxt_jll_ge_c, TPK_B, 3
            .byte       vxt_jll_ge_p2 - vxt_jll_ge_c, TPK_N, 0
            .byte       vxt_jll_ge_p3 - vxt_jll_ge_c, TPK_T, 4
vxt_jll_ge_c:
            bit         vm_mat
            bmi         vxt_jll_ge_s
            ldy         #0
vxt_jll_ge_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_jll_ge_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_jll_ge_s
            lda         ht
            cmp         (vm_s),y
            iny
            lda         ht + 1
            sbc         (vm_s),y
            bvc         :+
            eor         #$80
:
            bmi         vxt_jll_ge_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_ge_p2 = * - 2
vxt_jll_ge_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_ge_p3 = * - 2
vxt_jll_ge_s:
vxt_jll_ge_e:

.assert vxt_jll_ge_e - vxt_jll_ge_c + VX_STUB + 48 < 256, error, "vxt_jll_ge: an op's code is at most 255 bytes"

vxt_jll_gt: .byte       vxt_jll_gt_e - vxt_jll_gt_c, TF_STUB, 4
            .byte       vxt_jll_gt_p0 - vxt_jll_gt_c, TPK_B, 2
            .byte       vxt_jll_gt_p1 - vxt_jll_gt_c, TPK_B, 3
            .byte       vxt_jll_gt_p2 - vxt_jll_gt_c, TPK_N, 0
            .byte       vxt_jll_gt_p3 - vxt_jll_gt_c, TPK_T, 4
vxt_jll_gt_c:
            bit         vm_mat
            bmi         vxt_jll_gt_s
            ldy         #0
vxt_jll_gt_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_jll_gt_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_jll_gt_s
            lda         (vm_s),y
            cmp         ht
            iny
            lda         (vm_s),y
            sbc         ht + 1
            bvc         :+
            eor         #$80
:
            bpl         vxt_jll_gt_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_gt_p2 = * - 2
vxt_jll_gt_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_gt_p3 = * - 2
vxt_jll_gt_s:
vxt_jll_gt_e:

.assert vxt_jll_gt_e - vxt_jll_gt_c + VX_STUB + 48 < 256, error, "vxt_jll_gt: an op's code is at most 255 bytes"

vxt_jll_le: .byte       vxt_jll_le_e - vxt_jll_le_c, TF_STUB, 4
            .byte       vxt_jll_le_p0 - vxt_jll_le_c, TPK_B, 2
            .byte       vxt_jll_le_p1 - vxt_jll_le_c, TPK_B, 3
            .byte       vxt_jll_le_p2 - vxt_jll_le_c, TPK_N, 0
            .byte       vxt_jll_le_p3 - vxt_jll_le_c, TPK_T, 4
vxt_jll_le_c:
            bit         vm_mat
            bmi         vxt_jll_le_s
            ldy         #0
vxt_jll_le_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_jll_le_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_jll_le_s
            lda         (vm_s),y
            cmp         ht
            iny
            lda         (vm_s),y
            sbc         ht + 1
            bvc         :+
            eor         #$80
:
            bmi         vxt_jll_le_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_le_p2 = * - 2
vxt_jll_le_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_le_p3 = * - 2
vxt_jll_le_s:
vxt_jll_le_e:

.assert vxt_jll_le_e - vxt_jll_le_c + VX_STUB + 48 < 256, error, "vxt_jll_le: an op's code is at most 255 bytes"

vxt_jll_eq: .byte       vxt_jll_eq_e - vxt_jll_eq_c, TF_STUB, 4
            .byte       vxt_jll_eq_p0 - vxt_jll_eq_c, TPK_B, 2
            .byte       vxt_jll_eq_p1 - vxt_jll_eq_c, TPK_B, 3
            .byte       vxt_jll_eq_p2 - vxt_jll_eq_c, TPK_N, 0
            .byte       vxt_jll_eq_p3 - vxt_jll_eq_c, TPK_T, 4
vxt_jll_eq_c:
            bit         vm_mat
            bmi         vxt_jll_eq_s
            ldy         #0
vxt_jll_eq_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            iny
            lda         (vm_s),y
            sta         ht + 1
            ldy         #0
vxt_jll_eq_p1 = * - 1
            lda         (vm_s),y
            and         ht
            lsr
            bcc         vxt_jll_eq_s
            lda         ht
            cmp         (vm_s),y
            bne         vxt_jll_eq_f
            iny
            lda         ht + 1
            cmp         (vm_s),y
            bne         vxt_jll_eq_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_eq_p2 = * - 2
vxt_jll_eq_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_jll_eq_p3 = * - 2
vxt_jll_eq_s:
vxt_jll_eq_e:

.assert vxt_jll_eq_e - vxt_jll_eq_c + VX_STUB + 48 < 256, error, "vxt_jll_eq: an op's code is at most 255 bytes"

vxt_shead:  .byte       vxt_shead_e - vxt_shead_c, TF_STUB, 5
            .byte       vxt_shead_p0 - vxt_shead_c, TPK_D, 8
            .byte       vxt_shead_p1 - vxt_shead_c, TPK_D, 9
            .byte       vxt_shead_p2 - vxt_shead_c, TPK_D, 10
            .byte       vxt_shead_p3 - vxt_shead_c, TPK_D, 11
            .byte       vxt_shead_p4 - vxt_shead_c, TPK_N, 0
vxt_shead_c:
            bit         vm_mat
            bmi         vxt_shead_s
            lda         a:$0000
vxt_shead_p0 = * - 2
            cmp         vm_ep
            bne         vxt_shead_s
            lda         a:$0000
vxt_shead_p1 = * - 2
            cmp         vm_ep + 1
            bne         vxt_shead_s
            lda         a:$0000
vxt_shead_p2 = * - 2
            ldx         a:$0000
vxt_shead_p3 = * - 2
            sta         (sp)
            ldy         #1
            txa
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_shead_p4 = * - 2
vxt_shead_s:
vxt_shead_e:

.assert vxt_shead_e - vxt_shead_c + VX_STUB + 48 < 256, error, "vxt_shead: an op's code is at most 255 bytes"

vxt_head:   .byte       vxt_head_e - vxt_head_c, TF_STUB, 1
            .byte       vxt_head_p0 - vxt_head_c, TPK_N, 0
vxt_head_c:
            lda         ex
            lsr
            bcs         vxt_head_s
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         vxt_head_s
            lda         pk,x
            cmp         #PK_BUFFER
            beq         vxt_head_p
            cmp         #PK_PARTIAL
            bne         vxt_head_s
vxt_head_p:
            lda         ex
            sta         (sp)
            ldy         #1
            lda         ex + 1
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            jmp         $0000
vxt_head_p0 = * - 2
vxt_head_s:
vxt_head_e:

.assert vxt_head_e - vxt_head_c + VX_STUB + 48 < 256, error, "vxt_head: an op's code is at most 255 bytes"

vxt_call:   .byte       vxt_call_e - vxt_call_c, TF_STUB | TF_MISS, 17
            .byte       vxt_call_p0 - vxt_call_c, TPK_D, 6
            .byte       vxt_call_p1 - vxt_call_c, TPK_D, 7
            .byte       vxt_call_p2 - vxt_call_c, TPK_M, 1
            .byte       vxt_call_p3 - vxt_call_c, TPK_D, 4
            .byte       vxt_call_p4 - vxt_call_c, TPK_D, 5
            .byte       vxt_call_p5 - vxt_call_c, TPK_RL, 0
            .byte       vxt_call_p6 - vxt_call_c, TPK_RH, 0
            .byte       vxt_call_p7 - vxt_call_c, TPK_S, 0
            .byte       vxt_call_p8 - vxt_call_c, TPK_RL, 0
            .byte       vxt_call_p9 - vxt_call_c, TPK_RH, 0
            .byte       vxt_call_p10 - vxt_call_c, TPK_M, 1
            .byte       vxt_call_p11 - vxt_call_c, TPK_M, 1
            .byte       vxt_call_p12 - vxt_call_c, TPK_D, 11
            .byte       vxt_call_p13 - vxt_call_c, TPK_D, 8
            .byte       vxt_call_p14 - vxt_call_c, TPK_D, 10
            .byte       vxt_call_p15 - vxt_call_c, TPK_D, 8
            .byte       vxt_call_p16 - vxt_call_c, TPK_D, 9
vxt_call_c:
            lda         a:$0000
vxt_call_p0 = * - 2
            cmp         vm_ep
            bne         vxt_call_sx
            lda         a:$0000
vxt_call_p1 = * - 2
            cmp         vm_ep + 1
            bne         vxt_call_sx
            lda         sp
            sec
            sbc         #0
vxt_call_p2 = * - 1
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)
            cmp         a:$0000
vxt_call_p3 = * - 2
            bne         vxt_call_sx
            ldy         #1
            lda         (ht),y
            cmp         a:$0000
vxt_call_p4 = * - 2
            bne         vxt_call_sx
            bit         intr
            bmi         vxt_call_sx
            bvs         vxt_call_sx
            lda         depth + 1
            cmp         #>MAX_DEPTH
            bcc         vxt_call_ok
            bne         vxt_call_s
            lda         depth
            cmp         #<MAX_DEPTH
            bcs         vxt_call_s
vxt_call_ok:
            lda         sp
            cmp         #$FC
            bcs         vxt_call_r2
            lda         #0
vxt_call_p5 = * - 1
            sta         (sp)
            ldy         #1
            lda         #0
vxt_call_p6 = * - 1
            sta         (sp),y
            iny
            lda         #0
            sta         (sp),y
            iny
            sta         (sp),y
            lda         sp
            adc         #4
            sta         sp
            lda         ht
            sta         vm_s
            lda         ht + 1
            sta         vm_s + 1
            bra         vxt_call_r4
vxt_call_sx:
            jmp         $0000
vxt_call_p7 = * - 2
vxt_call_r2:
            lda         #0
vxt_call_p8 = * - 1
            ldx         #0
vxt_call_p9 = * - 1
            ldy         #0
vxt_call_p10 = * - 1
            jsr         vm_rec
vxt_call_r4:
            lda         #0
vxt_call_p11 = * - 1
            sta         vm_rb
            stz         vm_mat
            inc         depth
            bne         :+
            inc         depth + 1
:
            lda         a:$0000
vxt_call_p12 = * - 2
            cmp         vm_idx
            bne         vxt_call_far
            jmp         ($0000)
vxt_call_p13 = * - 2
vxt_call_far:
            sta         vm_idx
            lda         a:$0000
vxt_call_p14 = * - 2
            sta         vm_bank
            lda         a:$0000
vxt_call_p15 = * - 2
            sta         vm_ip
            lda         a:$0000
vxt_call_p16 = * - 2
            sta         vm_ip + 1
            jmp         vm_next
vxt_call_s:
vxt_call_e:

.assert vxt_call_e - vxt_call_c + VX_STUB + 48 < 256, error, "vxt_call: an op's code is at most 255 bytes"

vxt_cself:  .byte       vxt_cself_e - vxt_cself_c, TF_STUB, 8
            .byte       vxt_cself_p0 - vxt_cself_c, TPK_M, 1
            .byte       vxt_cself_p1 - vxt_cself_c, TPK_RL, 0
            .byte       vxt_cself_p2 - vxt_cself_c, TPK_RH, 0
            .byte       vxt_cself_p3 - vxt_cself_c, TPK_S, 0
            .byte       vxt_cself_p4 - vxt_cself_c, TPK_RL, 0
            .byte       vxt_cself_p5 - vxt_cself_c, TPK_RH, 0
            .byte       vxt_cself_p6 - vxt_cself_c, TPK_M, 1
            .byte       vxt_cself_p7 - vxt_cself_c, TPK_C, 0
vxt_cself_c:
            lda         sp
            sec
            sbc         #0
vxt_cself_p0 = * - 1
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            cmp         (vm_s),y
            bne         vxt_cself_s
            lda         (ht)
            cmp         (vm_s)
            bne         vxt_cself_s
            bit         intr
            bmi         vxt_cself_s
            bvs         vxt_cself_s
            lda         depth + 1
            cmp         #>MAX_DEPTH
            bcc         vxt_cself_ok
            bne         vxt_cself_s
            lda         depth
            cmp         #<MAX_DEPTH
            bcs         vxt_cself_s
vxt_cself_ok:
            lda         sp
            cmp         #$FC
            bcs         vxt_cself_r2
            lda         #0
vxt_cself_p1 = * - 1
            sta         (sp)
            ldy         #1
            lda         #0
vxt_cself_p2 = * - 1
            sta         (sp),y
            iny
            lda         #0
            sta         (sp),y
            iny
            sta         (sp),y
            lda         sp
            adc         #4
            sta         sp
            lda         ht
            sta         vm_s
            lda         ht + 1
            sta         vm_s + 1
            bra         vxt_cself_r4
vxt_cself_sx:
            jmp         $0000
vxt_cself_p3 = * - 2
vxt_cself_r2:
            lda         #0
vxt_cself_p4 = * - 1
            ldx         #0
vxt_cself_p5 = * - 1
            ldy         #0
vxt_cself_p6 = * - 1
            jsr         vm_rec
vxt_cself_r4:
            stz         vm_mat
            inc         depth
            bne         :+
            inc         depth + 1
:
            jmp         $0000
vxt_cself_p7 = * - 2
vxt_cself_s:
vxt_cself_e:

.assert vxt_cself_e - vxt_cself_c + VX_STUB + 48 < 256, error, "vxt_cself: an op's code is at most 255 bytes"

vxt_tself:  .byte       vxt_tself_e - vxt_tself_c, TF_STUB, 5
            .byte       vxt_tself_p0 - vxt_tself_c, TPK_M, 1
            .byte       vxt_tself_p1 - vxt_tself_c, TPK_B, 1
            .byte       vxt_tself_p2 - vxt_tself_c, TPK_M, 1
            .byte       vxt_tself_p3 - vxt_tself_c, TPK_M, 1
            .byte       vxt_tself_p4 - vxt_tself_c, TPK_C, 0
vxt_tself_c:
            lda         sp
            sec
            sbc         #0
vxt_tself_p0 = * - 1
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            cmp         (vm_s),y
            bne         vxt_tself_s
            lda         (ht)
            cmp         (vm_s)
            bne         vxt_tself_s
            bit         intr
            bmi         vxt_tself_s
            bvs         vxt_tself_s
            ldx         #0
vxt_tself_p1 = * - 1
            beq         vxt_tself_args
            ldy         #2
vxt_tself_arg:
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            dex
            bne         vxt_tself_arg
vxt_tself_args:
            bit         vm_mat
            bpl         vxt_tself_kept
            ldy         #0
vxt_tself_p2 = * - 1
            iny
            iny
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            stz         vm_mat
vxt_tself_kept:
            lda         #0
vxt_tself_p3 = * - 1
            clc
            adc         #4
            adc         vm_s
            sta         sp
            lda         vm_s + 1
            adc         #0
            sta         sp + 1
            cmp         stk_basep
            bne         vxt_tself_go
            lda         spilled
            beq         vxt_tself_go
            jsr         unspill
vxt_tself_go:
            jmp         $0000
vxt_tself_p4 = * - 2
vxt_tself_s:
vxt_tself_e:

.assert vxt_tself_e - vxt_tself_c + VX_STUB + 48 < 256, error, "vxt_tself: an op's code is at most 255 bytes"

vxt_tself0: .byte       vxt_tself0_e - vxt_tself0_c, TF_STUB, 4
            .byte       vxt_tself0_p0 - vxt_tself0_c, TPK_M, 1
            .byte       vxt_tself0_p1 - vxt_tself0_c, TPK_M, 1
            .byte       vxt_tself0_p2 - vxt_tself0_c, TPK_M, 1
            .byte       vxt_tself0_p3 - vxt_tself0_c, TPK_C, 0
vxt_tself0_c:
            lda         sp
            sec
            sbc         #0
vxt_tself0_p0 = * - 1
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            cmp         (vm_s),y
            bne         vxt_tself0_s
            lda         (ht)
            cmp         (vm_s)
            bne         vxt_tself0_s
            bit         intr
            bmi         vxt_tself0_s
            bvs         vxt_tself0_s
            bit         vm_mat
            bpl         vxt_tself0_kept
            ldy         #0
vxt_tself0_p1 = * - 1
            iny
            iny
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            stz         vm_mat
vxt_tself0_kept:
            lda         #0
vxt_tself0_p2 = * - 1
            clc
            adc         #4
            adc         vm_s
            sta         sp
            lda         vm_s + 1
            adc         #0
            sta         sp + 1
            cmp         stk_basep
            bne         vxt_tself0_go
            lda         spilled
            beq         vxt_tself0_go
            jsr         unspill
vxt_tself0_go:
            jmp         $0000
vxt_tself0_p3 = * - 2
vxt_tself0_s:
vxt_tself0_e:

.assert vxt_tself0_e - vxt_tself0_c + VX_STUB + 48 < 256, error, "vxt_tself0: an op's code is at most 255 bytes"

vxt_tself1: .byte       vxt_tself1_e - vxt_tself1_c, TF_STUB, 4
            .byte       vxt_tself1_p0 - vxt_tself1_c, TPK_M, 1
            .byte       vxt_tself1_p1 - vxt_tself1_c, TPK_M, 1
            .byte       vxt_tself1_p2 - vxt_tself1_c, TPK_M, 1
            .byte       vxt_tself1_p3 - vxt_tself1_c, TPK_C, 0
vxt_tself1_c:
            lda         sp
            sec
            sbc         #0
vxt_tself1_p0 = * - 1
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            cmp         (vm_s),y
            bne         vxt_tself1_s
            lda         (ht)
            cmp         (vm_s)
            bne         vxt_tself1_s
            bit         intr
            bmi         vxt_tself1_s
            bvs         vxt_tself1_s
            ldy         #2
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            bit         vm_mat
            bpl         vxt_tself1_kept
            ldy         #0
vxt_tself1_p1 = * - 1
            iny
            iny
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            stz         vm_mat
vxt_tself1_kept:
            lda         #0
vxt_tself1_p2 = * - 1
            clc
            adc         #4
            adc         vm_s
            sta         sp
            lda         vm_s + 1
            adc         #0
            sta         sp + 1
            cmp         stk_basep
            bne         vxt_tself1_go
            lda         spilled
            beq         vxt_tself1_go
            jsr         unspill
vxt_tself1_go:
            jmp         $0000
vxt_tself1_p3 = * - 2
vxt_tself1_s:
vxt_tself1_e:

.assert vxt_tself1_e - vxt_tself1_c + VX_STUB + 48 < 256, error, "vxt_tself1: an op's code is at most 255 bytes"

vxt_tself2: .byte       vxt_tself2_e - vxt_tself2_c, TF_STUB, 4
            .byte       vxt_tself2_p0 - vxt_tself2_c, TPK_M, 1
            .byte       vxt_tself2_p1 - vxt_tself2_c, TPK_M, 1
            .byte       vxt_tself2_p2 - vxt_tself2_c, TPK_M, 1
            .byte       vxt_tself2_p3 - vxt_tself2_c, TPK_C, 0
vxt_tself2_c:
            lda         sp
            sec
            sbc         #0
vxt_tself2_p0 = * - 1
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            cmp         (vm_s),y
            bne         vxt_tself2_s
            lda         (ht)
            cmp         (vm_s)
            bne         vxt_tself2_s
            bit         intr
            bmi         vxt_tself2_s
            bvs         vxt_tself2_s
            ldy         #2
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            bit         vm_mat
            bpl         vxt_tself2_kept
            ldy         #0
vxt_tself2_p1 = * - 1
            iny
            iny
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            stz         vm_mat
vxt_tself2_kept:
            lda         #0
vxt_tself2_p2 = * - 1
            clc
            adc         #4
            adc         vm_s
            sta         sp
            lda         vm_s + 1
            adc         #0
            sta         sp + 1
            cmp         stk_basep
            bne         vxt_tself2_go
            lda         spilled
            beq         vxt_tself2_go
            jsr         unspill
vxt_tself2_go:
            jmp         $0000
vxt_tself2_p3 = * - 2
vxt_tself2_s:
vxt_tself2_e:

.assert vxt_tself2_e - vxt_tself2_c + VX_STUB + 48 < 256, error, "vxt_tself2: an op's code is at most 255 bytes"

vxt_tself3: .byte       vxt_tself3_e - vxt_tself3_c, TF_STUB, 4
            .byte       vxt_tself3_p0 - vxt_tself3_c, TPK_M, 1
            .byte       vxt_tself3_p1 - vxt_tself3_c, TPK_M, 1
            .byte       vxt_tself3_p2 - vxt_tself3_c, TPK_M, 1
            .byte       vxt_tself3_p3 - vxt_tself3_c, TPK_C, 0
vxt_tself3_c:
            lda         sp
            sec
            sbc         #0
vxt_tself3_p0 = * - 1
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            cmp         (vm_s),y
            bne         vxt_tself3_s
            lda         (ht)
            cmp         (vm_s)
            bne         vxt_tself3_s
            bit         intr
            bmi         vxt_tself3_s
            bvs         vxt_tself3_s
            ldy         #2
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            bit         vm_mat
            bpl         vxt_tself3_kept
            ldy         #0
vxt_tself3_p1 = * - 1
            iny
            iny
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            stz         vm_mat
vxt_tself3_kept:
            lda         #0
vxt_tself3_p2 = * - 1
            clc
            adc         #4
            adc         vm_s
            sta         sp
            lda         vm_s + 1
            adc         #0
            sta         sp + 1
            cmp         stk_basep
            bne         vxt_tself3_go
            lda         spilled
            beq         vxt_tself3_go
            jsr         unspill
vxt_tself3_go:
            jmp         $0000
vxt_tself3_p3 = * - 2
vxt_tself3_s:
vxt_tself3_e:

.assert vxt_tself3_e - vxt_tself3_c + VX_STUB + 48 < 256, error, "vxt_tself3: an op's code is at most 255 bytes"

vxt_tself4: .byte       vxt_tself4_e - vxt_tself4_c, TF_STUB, 4
            .byte       vxt_tself4_p0 - vxt_tself4_c, TPK_M, 1
            .byte       vxt_tself4_p1 - vxt_tself4_c, TPK_M, 1
            .byte       vxt_tself4_p2 - vxt_tself4_c, TPK_M, 1
            .byte       vxt_tself4_p3 - vxt_tself4_c, TPK_C, 0
vxt_tself4_c:
            lda         sp
            sec
            sbc         #0
vxt_tself4_p0 = * - 1
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            cmp         (vm_s),y
            bne         vxt_tself4_s
            lda         (ht)
            cmp         (vm_s)
            bne         vxt_tself4_s
            bit         intr
            bmi         vxt_tself4_s
            bvs         vxt_tself4_s
            ldy         #2
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            bit         vm_mat
            bpl         vxt_tself4_kept
            ldy         #0
vxt_tself4_p1 = * - 1
            iny
            iny
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            stz         vm_mat
vxt_tself4_kept:
            lda         #0
vxt_tself4_p2 = * - 1
            clc
            adc         #4
            adc         vm_s
            sta         sp
            lda         vm_s + 1
            adc         #0
            sta         sp + 1
            cmp         stk_basep
            bne         vxt_tself4_go
            lda         spilled
            beq         vxt_tself4_go
            jsr         unspill
vxt_tself4_go:
            jmp         $0000
vxt_tself4_p3 = * - 2
vxt_tself4_s:
vxt_tself4_e:

.assert vxt_tself4_e - vxt_tself4_c + VX_STUB + 48 < 256, error, "vxt_tself4: an op's code is at most 255 bytes"

vxt_ret:    .byte       vxt_ret_e - vxt_ret_c, TF_STUB, 0
vxt_ret_c:
            ldy         vm_rb
            lda         (vm_s),y
            sta         ht
            beq         vxt_ret_s
            iny
            lda         (vm_s),y
            lsr
            tax
            lda         ht
            ror
            sta         vm_ip
            txa
            and         #$60
            cmp         vm_idx
            bne         vxt_ret_s
            txa
            and         #$1F
            ora         #$80
            sta         vm_ip + 1
            lda         depth
            bne         :+
            dec         depth + 1
:
            dec         depth
            lda         vm_s
            sta         sp
            ldx         vm_s + 1
            stx         sp + 1
            cpx         stk_basep
            bne         :+
            lda         spilled
            beq         :+
            jsr         unspill
:
            lda         ex
            lsr
            bcs         vxt_ret_go
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         vxt_ret_go
            lda         pk,x
            cmp         #PK_ERROR
            bne         vxt_ret_go
            jmp         vm_reterr
vxt_ret_go:
            jmp         (vm_ip)
vxt_ret_s:
vxt_ret_e:

.assert vxt_ret_e - vxt_ret_c + VX_STUB + 48 < 256, error, "vxt_ret: an op's code is at most 255 bytes"

vxt_pcall:  .byte       vxt_pcall_e - vxt_pcall_c, 0, 2
            .byte       vxt_pcall_p0 - vxt_pcall_c, TPK_W, 12
            .byte       vxt_pcall_p1 - vxt_pcall_c, TPK_U, 13
vxt_pcall_c:
            lda         sp
            sec
            sbc         #0
vxt_pcall_p0 = * - 1
            sta         vm_s
            lda         sp + 1
            sbc         #0
            sta         vm_s + 1
            ldy         #0
vxt_pcall_p1 = * - 1
            sty         vm_rb
            iny
            iny
            lda         (vm_s),y
            lsr
            bcc         :+
            iny
            iny
:
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            lda         #0
            bcc         :+
            lda         #$80
:
            sta         vm_mat
vxt_pcall_e:

vxt_pself:  .byte       vxt_pself_e - vxt_pself_c, 0, 2
            .byte       vxt_pself_p0 - vxt_pself_c, TPK_W, 6
            .byte       vxt_pself_p1 - vxt_pself_c, TPK_U, 7
vxt_pself_c:
            lda         sp
            sec
            sbc         #0
vxt_pself_p0 = * - 1
            sta         vm_s
            lda         sp + 1
            sbc         #0
            sta         vm_s + 1
            ldy         #0
vxt_pself_p1 = * - 1
            sty         vm_rb
            iny
            iny
            lda         (vm_s),y
            lsr
            bcc         :+
            iny
            iny
:
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            lda         #0
            bcc         :+
            lda         #$80
:
            sta         vm_mat
vxt_pself_e:

vxt_locals: .byte       vxt_locals_e - vxt_locals_c, TF_STUB, 4
            .byte       vxt_locals_p0 - vxt_locals_c, TPK_DL, 0
            .byte       vxt_locals_p1 - vxt_locals_c, TPK_DH, 0
            .byte       vxt_locals_p2 - vxt_locals_c, TPK_B, 1
            .byte       vxt_locals_p3 - vxt_locals_c, TPK_N, 0
vxt_locals_c:
            ldy         vm_rb
            iny
            iny
            lda         #0
vxt_locals_p0 = * - 1
            sta         (vm_s),y
            iny
            lda         #0
vxt_locals_p1 = * - 1
            sta         (vm_s),y
            lda         #0
            tax
            sta         (sp)
            ldy         #1
            txa
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            ldx         #0
vxt_locals_p2 = * - 1
vxt_locals_h:
            lda         #<UNBOUND
            sta         (sp)
            ldy         #1
            lda         #>UNBOUND
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         :+
            jsr         vm_page
:
            dex
            bne         vxt_locals_h
            jmp         $0000
vxt_locals_p3 = * - 2
vxt_locals_s:
vxt_locals_e:

.assert vxt_locals_e - vxt_locals_c + VX_STUB + 48 < 256, error, "vxt_locals: an op's code is at most 255 bytes"

vxt_block:  .byte       vxt_block_e - vxt_block_c, TF_STUB, 4
            .byte       vxt_block_p0 - vxt_block_c, TPK_B, 1
            .byte       vxt_block_p1 - vxt_block_c, TPK_B, 2
            .byte       vxt_block_p2 - vxt_block_c, TPK_B, 3
            .byte       vxt_block_p3 - vxt_block_c, TPK_N, 0
vxt_block_c:
            ldy         #0
vxt_block_p0 = * - 1
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            ldx         #0
vxt_block_p1 = * - 1
            beq         vxt_block_d
            ldy         #0
vxt_block_p2 = * - 1
vxt_block_h:
            lda         #<UNBOUND
            sta         (vm_s),y
            iny
            lda         #>UNBOUND
            sta         (vm_s),y
            iny
            dex
            bne         vxt_block_h
vxt_block_d:
            jmp         $0000
vxt_block_p3 = * - 2
vxt_block_s:
vxt_block_e:

.assert vxt_block_e - vxt_block_c + VX_STUB + 48 < 256, error, "vxt_block: an op's code is at most 255 bytes"

vxt_localb: .byte       vxt_localb_e - vxt_localb_c, TF_STUB, 3
            .byte       vxt_localb_p0 - vxt_localb_c, TPK_B, 2
            .byte       vxt_localb_p1 - vxt_localb_c, TPK_B, 1
            .byte       vxt_localb_p2 - vxt_localb_c, TPK_N, 0
vxt_localb_c:
            ldy         #0
vxt_localb_p0 = * - 1
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            bcs         vxt_localb_s
            ldy         #0
vxt_localb_p1 = * - 1
            lda         (vm_s),y
            sta         ex
            iny
            lda         (vm_s),y
            sta         ex + 1
            bne         vxt_localb_ok
            lda         ex
            cmp         #<UNBOUND
            beq         vxt_localb_s
vxt_localb_ok:
            jmp         $0000
vxt_localb_p2 = * - 2
vxt_localb_s:
vxt_localb_e:

.assert vxt_localb_e - vxt_localb_c + VX_STUB + 48 < 256, error, "vxt_localb: an op's code is at most 255 bytes"

vxt_setlb:  .byte       vxt_setlb_e - vxt_setlb_c, TF_STUB, 3
            .byte       vxt_setlb_p0 - vxt_setlb_c, TPK_B, 2
            .byte       vxt_setlb_p1 - vxt_setlb_c, TPK_B, 1
            .byte       vxt_setlb_p2 - vxt_setlb_c, TPK_N, 0
vxt_setlb_c:
            ldy         #0
vxt_setlb_p0 = * - 1
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            bcs         vxt_setlb_s
            ldy         #0
vxt_setlb_p1 = * - 1
            lda         ex
            sta         (vm_s),y
            iny
            lda         ex + 1
            sta         (vm_s),y
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_setlb_p2 = * - 2
vxt_setlb_s:
vxt_setlb_e:

.assert vxt_setlb_e - vxt_setlb_c + VX_STUB + 48 < 256, error, "vxt_setlb: an op's code is at most 255 bytes"

vxt_setblb: .byte       vxt_setblb_e - vxt_setblb_c, TF_STUB, 4
            .byte       vxt_setblb_p0 - vxt_setblb_c, TPK_B, 2
            .byte       vxt_setblb_p1 - vxt_setblb_c, TPK_B, 1
            .byte       vxt_setblb_p2 - vxt_setblb_c, TPK_B, 1
            .byte       vxt_setblb_p3 - vxt_setblb_c, TPK_N, 0
vxt_setblb_c:
            ldy         #0
vxt_setblb_p0 = * - 1
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            bcs         vxt_setblb_s
            ldy         #0
vxt_setblb_p1 = * - 1
            iny
            lda         (vm_s),y
            bne         vxt_setblb_set
            dey
            lda         (vm_s),y
            cmp         #<UNBOUND
            beq         vxt_setblb_s
vxt_setblb_set:
            ldy         #0
vxt_setblb_p2 = * - 1
            lda         ex
            sta         (vm_s),y
            iny
            lda         ex + 1
            sta         (vm_s),y
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_setblb_p3 = * - 2
vxt_setblb_s:
vxt_setblb_e:

.assert vxt_setblb_e - vxt_setblb_c + VX_STUB + 48 < 256, error, "vxt_setblb: an op's code is at most 255 bytes"

vxt_localh: .byte       vxt_localh_e - vxt_localh_c, TF_STUB, 2
            .byte       vxt_localh_p0 - vxt_localh_c, TPK_B, 1
            .byte       vxt_localh_p1 - vxt_localh_c, TPK_N, 0
vxt_localh_c:
            bit         vm_mat
            bmi         vxt_localh_s
            ldy         #0
vxt_localh_p0 = * - 1
            lda         (vm_s),y
            sta         ex
            iny
            lda         (vm_s),y
            sta         ex + 1
            bne         vxt_localh_ok
            lda         ex
            cmp         #<UNBOUND
            beq         vxt_localh_s
vxt_localh_ok:
            jmp         $0000
vxt_localh_p1 = * - 2
vxt_localh_s:
vxt_localh_e:

.assert vxt_localh_e - vxt_localh_c + VX_STUB + 48 < 256, error, "vxt_localh: an op's code is at most 255 bytes"

vxt_setbl:  .byte       vxt_setbl_e - vxt_setbl_c, TF_STUB, 3
            .byte       vxt_setbl_p0 - vxt_setbl_c, TPK_B, 1
            .byte       vxt_setbl_p1 - vxt_setbl_c, TPK_B, 1
            .byte       vxt_setbl_p2 - vxt_setbl_c, TPK_N, 0
vxt_setbl_c:
            bit         vm_mat
            bmi         vxt_setbl_s
            ldy         #0
vxt_setbl_p0 = * - 1
            iny
            lda         (vm_s),y
            bne         vxt_setbl_set
            dey
            lda         (vm_s),y
            cmp         #<UNBOUND
            beq         vxt_setbl_s
vxt_setbl_set:
            ldy         #0
vxt_setbl_p1 = * - 1
            lda         ex
            sta         (vm_s),y
            iny
            lda         ex + 1
            sta         (vm_s),y
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_setbl_p2 = * - 2
vxt_setbl_s:
vxt_setbl_e:

.assert vxt_setbl_e - vxt_setbl_c + VX_STUB + 48 < 256, error, "vxt_setbl: an op's code is at most 255 bytes"

vxt_trye:   .byte       vxt_trye_e - vxt_trye_c, 0, 1
            .byte       vxt_trye_p0 - vxt_trye_c, TPK_T, 1
vxt_trye_c:
            jsr         vm_iserr
            bcs         vxt_trye_on
            jmp         $0000
vxt_trye_p0 = * - 2
vxt_trye_on:
vxt_trye_e:

.assert vxt_trye_e - vxt_trye_c + VX_STUB + 48 < 256, error, "vxt_trye: an op's code is at most 255 bytes"

vxt_add:    .byte       vxt_add_e - vxt_add_c, TF_STUB, 1
            .byte       vxt_add_p0 - vxt_add_c, TPK_N, 0
vxt_add_c:
            lda         sp
            beq         vxt_add_s
            sec
            sbc         #2
            sta         sp
            lda         (sp)
            sta         ht
            and         ex
            lsr
            bcc         vxt_add_u
            ldy         #1
            lda         (sp),y
            sta         ht + 1
            lda         ex
            and         #$FE
            clc
            adc         ht
            tax
            lda         ex + 1
            adc         ht + 1
            bvs         vxt_add_u
            sta         ex + 1
            stx         ex
            jmp         $0000
vxt_add_p0 = * - 2
vxt_add_u:
            lda         sp
            clc
            adc         #2
            sta         sp
vxt_add_s:
vxt_add_e:

.assert vxt_add_e - vxt_add_c + VX_STUB + 48 < 256, error, "vxt_add: an op's code is at most 255 bytes"

vxt_sub:    .byte       vxt_sub_e - vxt_sub_c, TF_STUB, 1
            .byte       vxt_sub_p0 - vxt_sub_c, TPK_N, 0
vxt_sub_c:
            lda         sp
            beq         vxt_sub_s
            sec
            sbc         #2
            sta         sp
            lda         (sp)
            sta         ht
            and         ex
            lsr
            bcc         vxt_sub_u
            ldy         #1
            lda         (sp),y
            sta         ht + 1
            lda         ex
            and         #$FE
            sta         hn
            lda         ht
            sec
            sbc         hn
            tax
            lda         ht + 1
            sbc         ex + 1
            bvs         vxt_sub_u
            sta         ex + 1
            stx         ex
            jmp         $0000
vxt_sub_p0 = * - 2
vxt_sub_u:
            lda         sp
            clc
            adc         #2
            sta         sp
vxt_sub_s:
vxt_sub_e:

.assert vxt_sub_e - vxt_sub_c + VX_STUB + 48 < 256, error, "vxt_sub: an op's code is at most 255 bytes"

vxt_q_lt:   .byte       vxt_q_lt_e - vxt_q_lt_c, TF_STUB, 2
            .byte       vxt_q_lt_p0 - vxt_q_lt_c, TPK_N, 0
            .byte       vxt_q_lt_p1 - vxt_q_lt_c, TPK_N, 0
vxt_q_lt_c:
            lda         sp
            beq         vxt_q_lt_s
            sec
            sbc         #2
            sta         sp
            lda         (sp)
            sta         ht
            and         ex
            lsr
            bcc         vxt_q_lt_u
            ldy         #1
            lda         (sp),y
            sta         ht + 1
            lda         ht
            cmp         ex
            lda         ht + 1
            sbc         ex + 1
            bvc         :+
            eor         #$80
:
            bpl         vxt_q_lt_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_q_lt_p0 = * - 2
vxt_q_lt_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_q_lt_p1 = * - 2
vxt_q_lt_u:
            lda         sp
            clc
            adc         #2
            sta         sp
vxt_q_lt_s:
vxt_q_lt_e:

.assert vxt_q_lt_e - vxt_q_lt_c + VX_STUB + 48 < 256, error, "vxt_q_lt: an op's code is at most 255 bytes"

vxt_q_gt:   .byte       vxt_q_gt_e - vxt_q_gt_c, TF_STUB, 2
            .byte       vxt_q_gt_p0 - vxt_q_gt_c, TPK_N, 0
            .byte       vxt_q_gt_p1 - vxt_q_gt_c, TPK_N, 0
vxt_q_gt_c:
            lda         sp
            beq         vxt_q_gt_s
            sec
            sbc         #2
            sta         sp
            lda         (sp)
            sta         ht
            and         ex
            lsr
            bcc         vxt_q_gt_u
            ldy         #1
            lda         (sp),y
            sta         ht + 1
            lda         ex
            cmp         ht
            lda         ex + 1
            sbc         ht + 1
            bvc         :+
            eor         #$80
:
            bpl         vxt_q_gt_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_q_gt_p0 = * - 2
vxt_q_gt_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_q_gt_p1 = * - 2
vxt_q_gt_u:
            lda         sp
            clc
            adc         #2
            sta         sp
vxt_q_gt_s:
vxt_q_gt_e:

.assert vxt_q_gt_e - vxt_q_gt_c + VX_STUB + 48 < 256, error, "vxt_q_gt: an op's code is at most 255 bytes"

vxt_q_le:   .byte       vxt_q_le_e - vxt_q_le_c, TF_STUB, 2
            .byte       vxt_q_le_p0 - vxt_q_le_c, TPK_N, 0
            .byte       vxt_q_le_p1 - vxt_q_le_c, TPK_N, 0
vxt_q_le_c:
            lda         sp
            beq         vxt_q_le_s
            sec
            sbc         #2
            sta         sp
            lda         (sp)
            sta         ht
            and         ex
            lsr
            bcc         vxt_q_le_u
            ldy         #1
            lda         (sp),y
            sta         ht + 1
            lda         ex
            cmp         ht
            lda         ex + 1
            sbc         ht + 1
            bvc         :+
            eor         #$80
:
            bmi         vxt_q_le_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_q_le_p0 = * - 2
vxt_q_le_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_q_le_p1 = * - 2
vxt_q_le_u:
            lda         sp
            clc
            adc         #2
            sta         sp
vxt_q_le_s:
vxt_q_le_e:

.assert vxt_q_le_e - vxt_q_le_c + VX_STUB + 48 < 256, error, "vxt_q_le: an op's code is at most 255 bytes"

vxt_q_ge:   .byte       vxt_q_ge_e - vxt_q_ge_c, TF_STUB, 2
            .byte       vxt_q_ge_p0 - vxt_q_ge_c, TPK_N, 0
            .byte       vxt_q_ge_p1 - vxt_q_ge_c, TPK_N, 0
vxt_q_ge_c:
            lda         sp
            beq         vxt_q_ge_s
            sec
            sbc         #2
            sta         sp
            lda         (sp)
            sta         ht
            and         ex
            lsr
            bcc         vxt_q_ge_u
            ldy         #1
            lda         (sp),y
            sta         ht + 1
            lda         ht
            cmp         ex
            lda         ht + 1
            sbc         ex + 1
            bvc         :+
            eor         #$80
:
            bmi         vxt_q_ge_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_q_ge_p0 = * - 2
vxt_q_ge_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_q_ge_p1 = * - 2
vxt_q_ge_u:
            lda         sp
            clc
            adc         #2
            sta         sp
vxt_q_ge_s:
vxt_q_ge_e:

.assert vxt_q_ge_e - vxt_q_ge_c + VX_STUB + 48 < 256, error, "vxt_q_ge: an op's code is at most 255 bytes"

vxt_q_eq:   .byte       vxt_q_eq_e - vxt_q_eq_c, TF_STUB, 2
            .byte       vxt_q_eq_p0 - vxt_q_eq_c, TPK_N, 0
            .byte       vxt_q_eq_p1 - vxt_q_eq_c, TPK_N, 0
vxt_q_eq_c:
            lda         sp
            beq         vxt_q_eq_s
            sec
            sbc         #2
            sta         sp
            lda         (sp)
            sta         ht
            and         ex
            lsr
            bcc         vxt_q_eq_u
            ldy         #1
            lda         (sp),y
            sta         ht + 1
            lda         ht
            cmp         ex
            bne         vxt_q_eq_f
            lda         ht + 1
            cmp         ex + 1
            bne         vxt_q_eq_f
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         $0000
vxt_q_eq_p0 = * - 2
vxt_q_eq_f:
            stz         ex
            stz         ex + 1
            jmp         $0000
vxt_q_eq_p1 = * - 2
vxt_q_eq_u:
            lda         sp
            clc
            adc         #2
            sta         sp
vxt_q_eq_s:
vxt_q_eq_e:

.assert vxt_q_eq_e - vxt_q_eq_c + VX_STUB + 48 < 256, error, "vxt_q_eq: an op's code is at most 255 bytes"

; A return pad's length (CALL's and CSELF's the same: VXK_Q's)
VXT_PADL        = vxt_pcall_e - vxt_pcall_c
.assert vxt_pself_e - vxt_pself_c = VXT_PADL, error, "the return pads are of a length"
.assert vxt_call_e - vxt_call_c + VX_STUB + 14 + VXT_PADL < 256, error, "CALL's code is at most 255 bytes"
.assert vxt_cself_e - vxt_cself_c + VX_STUB + 8 + VXT_PADL < 256, error, "CSELF's code is at most 255 bytes"

; Each op's template (0: its stub alone; the fused ones: by s, below; TSELF's by its count, vx_ttself)
vx_tmain:   .word       vxt_ret, vxt_const, vxt_local, 0, 0, vxt_push, vxt_jf, vxt_jt
            .word       vxt_jmp, 0, vxt_head, vxt_call, 0, 0, 0, 0
            .word       0, vxt_add, vxt_sub, vxt_q_lt, vxt_q_gt, vxt_q_le, vxt_q_ge, vxt_q_eq
            .word       0, 0, 0, 0, 0, 0, 0, 0
            .word       0, 0, 0, vxt_lpush, vxt_cpush, vxt_shead, vxt_cself, vxt_tself
            .word       vxt_locals, vxt_localh, vxt_setl, vxt_setbl, 0, 0, vxt_stt, vxt_popx
            .word       vxt_loop, vxt_block, vxt_localb, vxt_setlb, vxt_setblb, 0, vxt_doti, vxt_dotinc
            .word       0, 0, vxt_drop, vxt_trye, 0, 0, 0, 0

; The fused ops' templates, by s (VO_ADD's order: (s - VO_ADD) / 2)
vx_tlq:     .word       0, 0, 0, 0, 0, 0, 0, vxt_lq_inc, vxt_lq_dec
            .word       0, 0, vxt_lq_addc, vxt_lq_subc, 0, 0, 0, 0, 0
vx_tlqp:    .word       0, 0, 0, 0, 0, 0, 0, vxt_lqp_inc, vxt_lqp_dec
            .word       0, 0, vxt_lqp_addc, vxt_lqp_subc, 0, 0, 0, 0, 0
vx_tjlq:    .word       0, 0, 0, 0, 0, 0, 0, 0, 0
            .word       vxt_jlq_zerop, vxt_jlq_onep, 0, 0, vxt_jlq_ltc, vxt_jlq_gtc, vxt_jlq_lec, vxt_jlq_gec, vxt_jlq_eqc
vx_tjq:     .word       0, 0, 0, 0, 0, 0, 0, 0, 0
            .word       vxt_jq_zerop, vxt_jq_onep, 0, 0, vxt_jq_ltc, vxt_jq_gtc, vxt_jq_lec, vxt_jq_gec, vxt_jq_eqc
vx_tll:     .word       vxt_ll_add, vxt_ll_sub, 0, 0, 0, 0, 0, 0, 0
            .word       0, 0, 0, 0, 0, 0, 0, 0, 0
vx_tllp:    .word       vxt_llp_add, vxt_llp_sub, 0, 0, 0, 0, 0, 0, 0
            .word       0, 0, 0, 0, 0, 0, 0, 0, 0
vx_tjll:    .word       0, 0, vxt_jll_lt, vxt_jll_gt, vxt_jll_le, vxt_jll_ge, vxt_jll_eq, 0, 0
            .word       0, 0, 0, 0, 0, 0, 0, 0, 0

; TSELF's, of 0 to 4 arguments (more: vx_tmain's)
vx_ttself:  .word       vxt_tself0, vxt_tself1, vxt_tself2, vxt_tself3, vxt_tself4

; BLOCK's template's length (0: none): its data is past it and its stub (the blocks' table's, a block's parent's)
VXT_BLOCK_D     = vxt_block_e - vxt_block_c
