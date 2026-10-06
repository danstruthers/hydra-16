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

vxt_jmp:    .byte       vxt_jmp_e - vxt_jmp_c, 0, 1
            .byte       vxt_jmp_p0 - vxt_jmp_c, TPK_T, 1
vxt_jmp_c:
            jmp         $0000
vxt_jmp_p0 = * - 2
vxt_jmp_e:

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

vxt_loop:   .byte       vxt_loop_e - vxt_loop_c, TF_STUB, 1
            .byte       vxt_loop_p0 - vxt_loop_c, TPK_T, 1
vxt_loop_c:
            bit         intr
            bmi         vxt_loop_s
            jmp         $0000
vxt_loop_p0 = * - 2
vxt_loop_s:
vxt_loop_e:

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

vxt_popx:   .byte       vxt_popx_e - vxt_popx_c, 0, 0
vxt_popx_c:
            jsr         pop
            sta         ex
            stx         ex + 1
vxt_popx_e:

vxt_drop:   .byte       vxt_drop_e - vxt_drop_c, 0, 1
            .byte       vxt_drop_p0 - vxt_drop_c, TPK_B, 1
vxt_drop_c:
            lda         #0
vxt_drop_p0 = * - 1
            jsr         drop
            stz         ex
            stz         ex + 1
vxt_drop_e:

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
            sta         ht
            lsr
            bcc         vxt_jlq_zerop_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            cmp         #<FIX(0)
            bne         vxt_jlq_zerop_f
            lda         ht + 1
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

vxt_jq_zerop:.byte       vxt_jq_zerop_e - vxt_jq_zerop_c, TF_STUB, 2
            .byte       vxt_jq_zerop_p0 - vxt_jq_zerop_c, TPK_N, 0
            .byte       vxt_jq_zerop_p1 - vxt_jq_zerop_c, TPK_T, 4
vxt_jq_zerop_c:
            lda         ex
            sta         ht
            lsr
            bcc         vxt_jq_zerop_s
            lda         ex + 1
            sta         ht + 1
            lda         ht
            cmp         #<FIX(0)
            bne         vxt_jq_zerop_f
            lda         ht + 1
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
            sta         ht
            lsr
            bcc         vxt_jlq_onep_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            cmp         #<FIX(1)
            bne         vxt_jlq_onep_f
            lda         ht + 1
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

vxt_jq_onep:.byte       vxt_jq_onep_e - vxt_jq_onep_c, TF_STUB, 2
            .byte       vxt_jq_onep_p0 - vxt_jq_onep_c, TPK_N, 0
            .byte       vxt_jq_onep_p1 - vxt_jq_onep_c, TPK_T, 4
vxt_jq_onep_c:
            lda         ex
            sta         ht
            lsr
            bcc         vxt_jq_onep_s
            lda         ex + 1
            sta         ht + 1
            lda         ht
            cmp         #<FIX(1)
            bne         vxt_jq_onep_f
            lda         ht + 1
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
            sta         ht
            lsr
            bcc         vxt_jlq_ltc_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            cmp         #0
vxt_jlq_ltc_p1 = * - 1
            lda         ht + 1
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

vxt_jq_ltc: .byte       vxt_jq_ltc_e - vxt_jq_ltc_c, TF_STUB, 4
            .byte       vxt_jq_ltc_p0 - vxt_jq_ltc_c, TPK_B, 2
            .byte       vxt_jq_ltc_p1 - vxt_jq_ltc_c, TPK_B, 3
            .byte       vxt_jq_ltc_p2 - vxt_jq_ltc_c, TPK_N, 0
            .byte       vxt_jq_ltc_p3 - vxt_jq_ltc_c, TPK_T, 4
vxt_jq_ltc_c:
            lda         ex
            sta         ht
            lsr
            bcc         vxt_jq_ltc_s
            lda         ex + 1
            sta         ht + 1
            lda         ht
            cmp         #0
vxt_jq_ltc_p0 = * - 1
            lda         ht + 1
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
            sta         ht
            lsr
            bcc         vxt_jlq_gtc_s
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

vxt_jq_gtc: .byte       vxt_jq_gtc_e - vxt_jq_gtc_c, TF_STUB, 4
            .byte       vxt_jq_gtc_p0 - vxt_jq_gtc_c, TPK_B, 2
            .byte       vxt_jq_gtc_p1 - vxt_jq_gtc_c, TPK_B, 3
            .byte       vxt_jq_gtc_p2 - vxt_jq_gtc_c, TPK_N, 0
            .byte       vxt_jq_gtc_p3 - vxt_jq_gtc_c, TPK_T, 4
vxt_jq_gtc_c:
            lda         ex
            sta         ht
            lsr
            bcc         vxt_jq_gtc_s
            lda         ex + 1
            sta         ht + 1
            lda         #0
vxt_jq_gtc_p0 = * - 1
            cmp         ht
            lda         #0
vxt_jq_gtc_p1 = * - 1
            sbc         ht + 1
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
            sta         ht
            lsr
            bcc         vxt_jlq_lec_s
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

vxt_jq_lec: .byte       vxt_jq_lec_e - vxt_jq_lec_c, TF_STUB, 4
            .byte       vxt_jq_lec_p0 - vxt_jq_lec_c, TPK_B, 2
            .byte       vxt_jq_lec_p1 - vxt_jq_lec_c, TPK_B, 3
            .byte       vxt_jq_lec_p2 - vxt_jq_lec_c, TPK_N, 0
            .byte       vxt_jq_lec_p3 - vxt_jq_lec_c, TPK_T, 4
vxt_jq_lec_c:
            lda         ex
            sta         ht
            lsr
            bcc         vxt_jq_lec_s
            lda         ex + 1
            sta         ht + 1
            lda         #0
vxt_jq_lec_p0 = * - 1
            cmp         ht
            lda         #0
vxt_jq_lec_p1 = * - 1
            sbc         ht + 1
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
            sta         ht
            lsr
            bcc         vxt_jlq_gec_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            cmp         #0
vxt_jlq_gec_p1 = * - 1
            lda         ht + 1
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

vxt_jq_gec: .byte       vxt_jq_gec_e - vxt_jq_gec_c, TF_STUB, 4
            .byte       vxt_jq_gec_p0 - vxt_jq_gec_c, TPK_B, 2
            .byte       vxt_jq_gec_p1 - vxt_jq_gec_c, TPK_B, 3
            .byte       vxt_jq_gec_p2 - vxt_jq_gec_c, TPK_N, 0
            .byte       vxt_jq_gec_p3 - vxt_jq_gec_c, TPK_T, 4
vxt_jq_gec_c:
            lda         ex
            sta         ht
            lsr
            bcc         vxt_jq_gec_s
            lda         ex + 1
            sta         ht + 1
            lda         ht
            cmp         #0
vxt_jq_gec_p0 = * - 1
            lda         ht + 1
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
            sta         ht
            lsr
            bcc         vxt_jlq_eqc_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            cmp         #0
vxt_jlq_eqc_p1 = * - 1
            bne         vxt_jlq_eqc_f
            lda         ht + 1
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

vxt_jq_eqc: .byte       vxt_jq_eqc_e - vxt_jq_eqc_c, TF_STUB, 4
            .byte       vxt_jq_eqc_p0 - vxt_jq_eqc_c, TPK_B, 2
            .byte       vxt_jq_eqc_p1 - vxt_jq_eqc_c, TPK_B, 3
            .byte       vxt_jq_eqc_p2 - vxt_jq_eqc_c, TPK_N, 0
            .byte       vxt_jq_eqc_p3 - vxt_jq_eqc_c, TPK_T, 4
vxt_jq_eqc_c:
            lda         ex
            sta         ht
            lsr
            bcc         vxt_jq_eqc_s
            lda         ex + 1
            sta         ht + 1
            lda         ht
            cmp         #0
vxt_jq_eqc_p0 = * - 1
            bne         vxt_jq_eqc_f
            lda         ht + 1
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
            sta         ht
            lsr
            bcc         vxt_lq_addc_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            clc
            adc         #0
vxt_lq_addc_p1 = * - 1
            sta         ex
            lda         ht + 1
            adc         #0
vxt_lq_addc_p2 = * - 1
            bvs         vxt_lq_addc_s
            sta         ex + 1
            jmp         $0000
vxt_lq_addc_p3 = * - 2
vxt_lq_addc_s:
vxt_lq_addc_e:

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
            sta         ht
            lsr
            bcc         vxt_lqp_addc_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            clc
            adc         #0
vxt_lqp_addc_p1 = * - 1
            sta         ex
            lda         ht + 1
            adc         #0
vxt_lqp_addc_p2 = * - 1
            bvs         vxt_lqp_addc_s
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
vxt_lqp_addc_p3 = * - 2
vxt_lqp_addc_s:
vxt_lqp_addc_e:

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
            sta         ht
            lsr
            bcc         vxt_lq_subc_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            sec
            sbc         #0
vxt_lq_subc_p1 = * - 1
            sta         ex
            lda         ht + 1
            sbc         #0
vxt_lq_subc_p2 = * - 1
            bvs         vxt_lq_subc_s
            sta         ex + 1
            jmp         $0000
vxt_lq_subc_p3 = * - 2
vxt_lq_subc_s:
vxt_lq_subc_e:

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
            sta         ht
            lsr
            bcc         vxt_lqp_subc_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            sec
            sbc         #0
vxt_lqp_subc_p1 = * - 1
            sta         ex
            lda         ht + 1
            sbc         #0
vxt_lqp_subc_p2 = * - 1
            bvs         vxt_lqp_subc_s
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
vxt_lqp_subc_p3 = * - 2
vxt_lqp_subc_s:
vxt_lqp_subc_e:

vxt_lq_inc: .byte       vxt_lq_inc_e - vxt_lq_inc_c, TF_STUB, 2
            .byte       vxt_lq_inc_p0 - vxt_lq_inc_c, TPK_B, 2
            .byte       vxt_lq_inc_p1 - vxt_lq_inc_c, TPK_N, 0
vxt_lq_inc_c:
            bit         vm_mat
            bmi         vxt_lq_inc_s
            ldy         #0
vxt_lq_inc_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            lsr
            bcc         vxt_lq_inc_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            clc
            adc         #2
            sta         ex
            lda         ht + 1
            adc         #0
            bvs         vxt_lq_inc_s
            sta         ex + 1
            jmp         $0000
vxt_lq_inc_p1 = * - 2
vxt_lq_inc_s:
vxt_lq_inc_e:

vxt_lqp_inc:.byte       vxt_lqp_inc_e - vxt_lqp_inc_c, TF_STUB, 2
            .byte       vxt_lqp_inc_p0 - vxt_lqp_inc_c, TPK_B, 2
            .byte       vxt_lqp_inc_p1 - vxt_lqp_inc_c, TPK_N, 0
vxt_lqp_inc_c:
            bit         vm_mat
            bmi         vxt_lqp_inc_s
            ldy         #0
vxt_lqp_inc_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            lsr
            bcc         vxt_lqp_inc_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            clc
            adc         #2
            sta         ex
            lda         ht + 1
            adc         #0
            bvs         vxt_lqp_inc_s
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
vxt_lqp_inc_p1 = * - 2
vxt_lqp_inc_s:
vxt_lqp_inc_e:

vxt_lq_dec: .byte       vxt_lq_dec_e - vxt_lq_dec_c, TF_STUB, 2
            .byte       vxt_lq_dec_p0 - vxt_lq_dec_c, TPK_B, 2
            .byte       vxt_lq_dec_p1 - vxt_lq_dec_c, TPK_N, 0
vxt_lq_dec_c:
            bit         vm_mat
            bmi         vxt_lq_dec_s
            ldy         #0
vxt_lq_dec_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            lsr
            bcc         vxt_lq_dec_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            sec
            sbc         #2
            sta         ex
            lda         ht + 1
            sbc         #0
            bvs         vxt_lq_dec_s
            sta         ex + 1
            jmp         $0000
vxt_lq_dec_p1 = * - 2
vxt_lq_dec_s:
vxt_lq_dec_e:

vxt_lqp_dec:.byte       vxt_lqp_dec_e - vxt_lqp_dec_c, TF_STUB, 2
            .byte       vxt_lqp_dec_p0 - vxt_lqp_dec_c, TPK_B, 2
            .byte       vxt_lqp_dec_p1 - vxt_lqp_dec_c, TPK_N, 0
vxt_lqp_dec_c:
            bit         vm_mat
            bmi         vxt_lqp_dec_s
            ldy         #0
vxt_lqp_dec_p0 = * - 1
            lda         (vm_s),y
            sta         ht
            lsr
            bcc         vxt_lqp_dec_s
            iny
            lda         (vm_s),y
            sta         ht + 1
            lda         ht
            sec
            sbc         #2
            sta         ex
            lda         ht + 1
            sbc         #0
            bvs         vxt_lqp_dec_s
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
vxt_lqp_dec_p1 = * - 2
vxt_lqp_dec_s:
vxt_lqp_dec_e:

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
            sta         hp
            and         ht
            lsr
            bcc         vxt_ll_add_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         hp
            and         #$FE
            clc
            adc         ht
            sta         ex
            lda         hp + 1
            adc         ht + 1
            bvs         vxt_ll_add_s
            sta         ex + 1
            jmp         $0000
vxt_ll_add_p2 = * - 2
vxt_ll_add_s:
vxt_ll_add_e:

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
            sta         hp
            and         ht
            lsr
            bcc         vxt_llp_add_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         hp
            and         #$FE
            clc
            adc         ht
            sta         ex
            lda         hp + 1
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
            sta         hp
            and         ht
            lsr
            bcc         vxt_ll_sub_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         hp
            and         #$FE
            sta         hn
            lda         ht
            sec
            sbc         hn
            sta         ex
            lda         ht + 1
            sbc         hp + 1
            bvs         vxt_ll_sub_s
            sta         ex + 1
            jmp         $0000
vxt_ll_sub_p2 = * - 2
vxt_ll_sub_s:
vxt_ll_sub_e:

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
            sta         hp
            and         ht
            lsr
            bcc         vxt_llp_sub_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         hp
            and         #$FE
            sta         hn
            lda         ht
            sec
            sbc         hn
            sta         ex
            lda         ht + 1
            sbc         hp + 1
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
            sta         hp
            and         ht
            lsr
            bcc         vxt_jll_lt_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         ht
            cmp         hp
            lda         ht + 1
            sbc         hp + 1
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
            sta         hp
            and         ht
            lsr
            bcc         vxt_jll_ge_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         ht
            cmp         hp
            lda         ht + 1
            sbc         hp + 1
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
            sta         hp
            and         ht
            lsr
            bcc         vxt_jll_gt_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         hp
            cmp         ht
            lda         hp + 1
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
            sta         hp
            and         ht
            lsr
            bcc         vxt_jll_le_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         hp
            cmp         ht
            lda         hp + 1
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
            sta         hp
            and         ht
            lsr
            bcc         vxt_jll_eq_s
            iny
            lda         (vm_s),y
            sta         hp + 1
            lda         ht
            cmp         hp
            bne         vxt_jll_eq_f
            lda         ht + 1
            cmp         hp + 1
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

; Each op's template (0: its stub alone; the fused ones: by s, below)
vx_tmain:   .word       0, vxt_const, vxt_local, 0, 0, vxt_push, vxt_jf, vxt_jt
            .word       vxt_jmp, 0, 0, 0, 0, 0, 0, 0
            .word       0, 0, 0, 0, 0, 0, 0, 0
            .word       0, 0, 0, 0, 0, 0, 0, 0
            .word       0, 0, 0, vxt_lpush, vxt_cpush, 0, 0, 0
            .word       0, 0, vxt_setl, 0, 0, 0, vxt_stt, vxt_popx
            .word       vxt_loop, 0, 0, 0, 0, 0, vxt_doti, vxt_dotinc
            .word       0, 0, vxt_drop, 0, 0, 0, 0, 0

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
