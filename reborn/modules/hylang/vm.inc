; ****************************************************************************
; vm.inc - hylang's bytecode machine (docs/hylang.md, "The bytecode machine"), in its seventh bank: a function's body
; compiled at its second call (vm_compile) to the code of a small machine, which runs it with its arguments where
; they're pushed, in the evaluation stack, not bound in a scope on the heap.  The evaluator's own way is the rest:
; what the compiler doesn't do (most special forms, an fexpr's call) is handed to it, and it calls the functions
; that aren't compiled; a value comes back through a K_VM frame.
;   A function's code is in the arena, RAM banks of its own (up to four), never moved; its place is the function's
; word 4 (a fixnum: the bank's index, bits 14-13, and the address in the window, bits 12-0), till something the
; code counts on changes (a built-in's name bound again: vm_rebind), when every function's is dropped (vm_flush)
; and each is compiled again at its next call.  A full arena compiles nothing more till the evaluator's next start
; (ev_run: vm_reset).  Its word 5 counts its calls till it's compiled; FN_NOVM (word 3) says it can't be.
;   The machine: ex its value register (an accumulator), vm_ip its next op, vm_s its frame: the function's word,
; its arguments (as many as its formals, 15 at most), then its record (vm_rb from vm_s): the return (a fixnum, its
; caller's code past the call; NIL: the evaluator's, which called it) and its scope (NIL till one's made: vm_scope),
; then what its code pushes.  So every word on the stack is a value or a fixnum, and the collector marks it all.  A
; call's function and arguments are pushed, then CALL makes the callee's frame of them, in place (TCALL, in tail
; position, in the caller's place: a tail loop runs in constant space).  The scope is made when the code needs one
; (a Q-expression with names in it, the evaluator called, a built-in that looks at its caller's scope); from then
; its arguments are read there, where the evaluator may have changed them.
;   An error is a value, as the evaluator's: an op that may give one returns it from the function (its R variant,
; VM_R set: in a call's r byte, bit 0), unless something takes it (an argument of a built-in that does, the
; condition of a JE).  Each call and op that may go to the evaluator ends with two bytes, h (its place in its
; frame, in words) and r (its frame's record, and bit 0 R; 0 in tail position): a return or a K_VM frame finds the
; frame from them.  A call's place, h, is under 128 words, so its frame is in RAM whenever its words above are.

; The ops (an op's number is even; VM_R set: its value's error returned).  Operands follow it
VO_RET          = $00           ; ex returned
VO_CONST        = $02           ; v: ex = v
VO_LOCAL        = $04           ; y: ex = the argument at vm_s + y (its scope's, once there's one)
VO_GLOBAL       = $06           ; sym: ex = its global value (the function has no scope)
VO_LOOKUP       = $08           ; sym: ex = its value (in the function's scope: lookup's)
VO_PUSH         = $0A           ; ex pushed
VO_JF           = $0C           ; t: to t if ex is NIL or ()
VO_JT           = $0E           ; t: to t if it isn't
VO_JMP          = $10           ; t
VO_JE           = $12           ; k t: an error: k words dropped, then to t
VO_HEAD         = $14           ; c t: ex, call c's function: a function, an ordinary built-in, pushed; an error, or
                                ;   not a function and c has no arguments: to t (past the call); else c (from its
                                ;   arguments) the evaluator's, its value for t
VO_CALL         = $16           ; m h r: the function under the m arguments on top called
VO_TCALL        = $18           ; m h 0: the same, in tail position
VO_BCALL        = $1A           ; b m h r: built-in b called, its m arguments on top
VO_EVAL         = $1C           ; e h r: e, a call, the evaluator's (ev_call), in this frame's scope
VO_QUOTE        = $1E           ; q: ex = q with this frame's scope (a Q-expression's value)
VO_QUOTE0       = $20           ; q: ex = q with none (it has no names)
VO_ADD          = $22           ; The quick ones (fixnums worked here, else their built-in): the stack's top and ex
VO_SUB          = $24
VO_LT           = $26
VO_GT           = $28
VO_LE           = $2A
VO_GE           = $2C
VO_NEQ          = $2E           ;   (==)
VO_INC          = $30           ; ... ex alone
VO_DEC          = $32
VO_ZEROP        = $34
VO_ONEP         = $36
VO_ADDC         = $38           ; ... ex and the fixnum after the op (as VO_ADD's to VO_NEQ's)
VO_SUBC         = $3A
VO_LTC          = $3C
VO_GTC          = $3E
VO_LEC          = $40
VO_GEC          = $42
VO_EQC          = $44
VO_COUNT        = $46
VM_R            = $80           ; (An op's error returned)

VF_SCOPE        = $01           ; (A built-in that looks at its caller's scope: vb_vf's)

VC_LV           = 16            ; The compiler's: its levels (an expression's nesting: 8 bytes of the 65C02's stack) ...
VC_LABS         = 80            ;   labels in a function's code ...
VC_HMAX         = 120           ;   its frame's words (h is under 128) ...
VC_FORMALS      = 15            ;   its formals (a scope's frame holds them all in its first cell) ...
VC_ARGS         = 100           ;   a call's arguments ...
VC_SYMS         = 8             ;   and a Q-expression's nesting looked in for a name (deeper: it's taken to have one)
M_R             = 0             ; (An expression's way with an error: returned ...
M_OK            = 1             ;   it's its value ...
M_J             = 2             ;   or a jump: JE to a label, its height)

.bss
vm_rb:      .res        1                                   ; The frame's record: its place from vm_s
vm_bank:    .res        1                                   ; The code's bank ...
vm_idx:     .res        1                                   ;   and its index in the arena, times 32
vm_on:      .res        1                                   ; <> 0: the machine's running (its frame on top)
vm_mat:     .res        1                                   ; $80: the frame's scope made (its arguments there)
vm_jv:      .res        2                                   ; (A jump to another bank: vm_go1, vm_go7)
vm_t:       .res        2                                   ; (Scratch)
vm_u:       .res        2
vm_a:       .res        2                                   ; (A quick op's values, its length)
vm_b:       .res        2
vm_fl:      .res        1
vm_m:       .res        1                                   ; (A call's arguments, the words under them, how it's made)
vm_l:       .res        1
vm_k:       .res        1
vm_tl:      .res        1
ar_bank:    .res        4                                   ; The arena's banks (taken as they're wanted) ...
ar_n:       .res        1                                   ;   how many ...
ar_cur:     .res        1                                   ;   the one being filled ...
ar_top:     .res        2                                   ;   its next byte ...
vm_full:    .res        1                                   ;   <> 0: full (nothing compiled till ev_run's vm_reset)

; The compiler's: in the reader's scratch, past its number registers (hysys.inc's room: idle as a function's compiled)
vc_ws           = sy_area
vc_fs_lo        = vc_ws                                     ; The formals
vc_fs_hi        = vc_fs_lo + 16
lb_lo           = vc_fs_hi + 16                             ; Each label: its address, or its references' chain ...
lb_hi           = lb_lo + VC_LABS
lb_def          = lb_hi + VC_LABS                           ;   <> 0: defined
L_elo           = lb_def + VC_LABS                          ; Each level: the expression ...
L_ehi           = L_elo + VC_LV
L_alo           = L_ehi + VC_LV                             ;   a call's arguments not compiled yet ...
L_ahi           = L_alo + VC_LV
L_tail          = L_ahi + VC_LV                             ;   <> 0: in tail position ...
L_mode          = L_tail + VC_LV                            ;   its way with an error (and a jump's label, height) ...
L_jlab          = L_mode + VC_LV
L_jh            = L_jlab + VC_LV
L_am            = L_jh + VC_LV                              ;   its parts' way (those that don't take errors) ...
L_aj            = L_am + VC_LV
L_ah            = L_aj + VC_LV
L_h0            = L_ah + VC_LV                              ;   the frame's height where it began ...
L_m             = L_h0 + VC_LV                              ;   its arguments (parts) counted ...
L_t             = L_m + VC_LV                               ;   its labels ...
L_l2            = L_t + VC_LV
L_b             = L_l2 + VC_LV                              ;   a built-in's number, an op
vc_n            = L_b + VC_LV                               ; The function's formals counted ...
vc_rb           = vc_n + 1                                  ;   its record's place ...
vc_snil         = vc_rb + 1                                 ;   0: it has no scope ...
vc_h            = vc_snil + 1                               ;   the frame's height (in words) ...
vc_lv           = vc_h + 1                                  ;   the level ...
vc_nlab         = vc_lv + 1                                 ;   labels made ...
vc_bk           = vc_nlab + 1                               ;   the arena's bank, and its index times 32 ...
vc_idx          = vc_bk + 1
vc_s            = vc_idx + 1                                ;   the 65C02's stack as it began ...
vc_start        = vc_s + 1                                  ;   where its code begins ...
vc_q            = vc_start + 2                              ;   (scratch)
vc_hd           = vc_q + 2
vc_v            = vc_hd + 2
vc_t            = vc_v + 2
vc_x            = vc_t + 2
vc_cm           = vc_x + 1                                  ;   (a part's way with an error: vc_child's)
vc_cj           = vc_cm + 1
vc_ch           = vc_cj + 1
vc_sd           = vc_ch + 1
vc_wend         = vc_sd + 1
.assert     vc_wend <= sy_end, error, "vm.inc: the compiler's room runs past the reader's scratch"

.segment "DATA4"                                            ; (In the task's RAM: every bank's)

; A jump to .A, .X in the seventh bank (the machine's), or in the first (the evaluator's), from either
vm_go7:
            sta         vm_jv
            stx         vm_jv + 1
            lda         hyx2_bank7
            sta         $01
            jmp         (vm_jv)
vm_go1:
            sta         vm_jv
            stx         vm_jv + 1
            lda         hyx2_bank1
            sta         $01
            jmp         (vm_jv)

; hp: a symbol's cell (deref's), its global value about to change, or its name bound in a frame for the first
; time: if code compiled counts on its value (SF_INLINED: a special form inlined, a quick op, a built-in called),
; it's all dropped (vm_flush).  Keeps hp and the window
vm_rebind:
            ldy         #6
            lda         (hp),y
            and         #SF_INLINED
            beq         @no
            FARN        7, vm_flush
@no:
            rts

.segment "CODE7"

; ---- The arena

; The machine made, no code yet: as hylang starts (a snapshot's functions may have code from its build, not here)
vm_init:
            stz         vm_on
            stz         ar_n
; The arena emptied, every function's code dropped (ev_run's, when it's full: no frame of the machine's is left)
vm_reset:
            jsr         vm_flush
            stz         ar_cur
            stz         ar_top
            lda         #$80
            sta         ar_top + 1
            stz         vm_full
            rts

; Every function's code dropped (its word 4 NIL: compiled again at its next call), every symbol's SF_INLINED
; cleared.  Keeps .A, .X, .Y, hp and the window
vm_flush:
            pha
            phx
            phy
            lda         $00
            pha
            lda         hq
            pha
            lda         hq + 1
            pha
            ldx         #IMM_PAGES
@page:
            lda         bank_of,x
            sta         $00
            stz         hq
            lda         ptr_hi,x
            sta         hq + 1
            lda         pk,x                                ; (Each page of functions: each cell's word 4; of
            cmp         #PK_SYMBOL                          ;   symbols, each one's flags)
            beq         @syms
            cmp         #PK_FUNC
            bne         @next
            jsr         @half
            inc         hq + 1
            jsr         @half
            bra         @next
@syms:
            jsr         @sym
            inc         hq + 1
            jsr         @sym
@next:
            inx
            bne         @page
            pla
            sta         hq + 1
            pla
            sta         hq
            pla
            sta         $00
            ply
            plx
            pla
            rts
@half:
            ldy         #8                                  ; (16 cells of 16 bytes)
@cell:
            lda         #0
            sta         (hq),y
            iny
            sta         (hq),y
            tya
            clc
            adc         #15
            tay
            bcc         @cell
            rts
@sym:
            ldy         #6                                  ; (64 cells of 8 bytes)
@flags:
            lda         (hq),y
            and         #<~SF_INLINED
            sta         (hq),y
            tya
            clc
            adc         #8
            tay
            bcc         @flags
            rts

; ---- The machine

; IN: .A, .X = a code's place (a fixnum: vm_encode's).  vm_ip, vm_bank, vm_idx = it
vm_decode:
            sta         vm_t
            txa
            lsr
            sta         vm_t + 1                            ; (Its index, times 32, and the address's bits 12-8)
            lda         vm_t
            ror
            sta         vm_ip
            lda         vm_t + 1
            and         #$1F
            ora         #$80
            sta         vm_ip + 1
            lda         vm_t + 1
            and         #$60
            sta         vm_idx
            lsr
            lsr
            lsr
            lsr
            lsr
            tax
            lda         ar_bank,x
            sta         vm_bank
            rts

; IN: .A, .X = an address in the window, in the arena's bank vm_idx.  OUT: .A, .X = its place (a fixnum)
vm_encode:
            asl
            ora         #1
            pha
            txa
            and         #$1F
            ora         vm_idx
            rol
            tax
            pla
            rts

; The next op: ip advanced past this one first (vm_adv: .A bytes)
vm_adv5:
            lda         #5
            bra         vm_adv
vm_adv4:
            lda         #4
            bra         vm_adv
vm_adv3:
            lda         #3
            bra         vm_adv
vm_adv2:
            lda         #2
vm_adv:
            clc
            adc         vm_ip
            sta         vm_ip
            bcc         vm_next
            inc         vm_ip + 1
            bra         vm_next
vm_adv1:
            inc         vm_ip
            bne         vm_next
            inc         vm_ip + 1
vm_next:
            lda         vm_bank
            sta         $00
            lda         (vm_ip)
            tax
            jmp         (vm_ops,x)

; ip = the address at ip + .Y (an op's target)
vm_jumpy:
            lda         (vm_ip),y
            tax
            iny
            lda         (vm_ip),y
            sta         vm_ip + 1
            stx         vm_ip
            jmp         vm_next

; OUT: C = 1 if ex is an error.  Keeps .Y
vm_iserr:
            lda         ex
            lsr
            bcs         @no
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_ERROR
            beq         @yes
@no:
            clc
            rts
@yes:
            sec
            rts

; ex, the value of an op of .A bytes: an error returned if the op's VM_R; else the next op
vm_valerr:
            pha
            jsr         vm_iserr
            bcs         :+
            pla
            jmp         vm_adv
:
            lda         vm_bank
            sta         $00
            lda         (vm_ip)
            bmi         :+
            pla
            jmp         vm_adv
:
            pla
            jmp         vm_ret

; OUT: .A = the byte before ip (the r of the call or op it's past)
vm_rbyte:
            lda         vm_bank
            sta         $00
            lda         vm_ip
            sec
            sbc         #1
            sta         ht
            lda         vm_ip + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)
            rts

; .A, .X = sp: the stack cut back to there (a block spilled brought back, if it's down to its first page)
vm_setsp:
            sta         sp
            stx         sp + 1
            cpx         stk_basep
            bne         :+
            lda         spilled
            beq         :+
            jmp         unspill
:
            rts

; ht = the word under the vm_m words on top (a call's function)
vm_below:
            lda         vm_m
            asl
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         ht
            sec
            sbc         #2
            sta         ht
            bcs         :+
            dec         ht + 1
:
            rts

; vm_mat from the record's scope (made: $80)
vm_matget:
            ldy         vm_rb
            iny
            iny
            lda         (vm_s),y
            iny
            ora         (vm_s),y
            beq         :+
            lda         #$80
:
            sta         vm_mat
            rts

; vm_s = a call's place in its caller's frame (its function's word there, or a K_VM frame's), vm_ip the caller's
; code past the call (h and r its last two bytes).  OUT: vm_s, vm_rb, vm_mat the caller's frame's; C = 1 if the
; call's error is returned
vm_callsite:
            lda         vm_bank
            sta         $00
            lda         vm_ip
            sec
            sbc         #2
            sta         ht
            lda         vm_ip + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)                                ; (h: words under the call's place)
            asl
            sta         vm_k
            lda         vm_s
            sec
            sbc         vm_k
            sta         vm_s
            bcs         :+
            dec         vm_s + 1
:
            ldy         #1
            lda         (ht),y                              ; (r: the record's place, and R)
            lsr
            php
            asl
            sta         vm_rb
            jsr         vm_matget
            plp
            rts

; A frame made of the function's word and the vm_m arguments on top: its record pushed (its return vm_t, no scope
; yet); vm_s, vm_rb, vm_mat its; a frame more (depth)
vm_frame0:
            lda         vm_t
            ldx         vm_t + 1
            jsr         push
            lda         #0
            tax
            jsr         push
            lda         vm_m
            asl
            clc
            adc         #6
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         vm_s
            lda         sp + 1
            sbc         #0
            sta         vm_s + 1
            lda         vm_m
            asl
            clc
            adc         #2
            sta         vm_rb
            stz         vm_mat
            inc         depth
            bne         :+
            inc         depth + 1
:
            rts

; vm_l words (1 or 2) put under the .A words on top (vm_m), which move up.  Keeps vm_l
vm_up:
            sta         vm_m
            stx         vm_l
            stx         vm_k
@push:
            lda         #0
            tax
            jsr         push
            dec         vm_k
            bne         @push
            lda         vm_m
            beq         @done
            lda         vm_l                                ; (Each, from the last, 2 * vm_l bytes up)
            asl
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         ht
            sec
            sbc         #2
            sta         ht
            bcs         :+
            dec         ht + 1
:
            ldx         vm_m
@move:
            ldy         #0
            lda         (ht),y
            ldy         vm_k
            sta         (ht),y
            ldy         #1
            lda         (ht),y
            ldy         vm_k
            iny
            sta         (ht),y
            lda         ht
            sec
            sbc         #2
            sta         ht
            bcs         :+
            dec         ht + 1
:
            dex
            bne         @move
@done:
            rts

; A K_VM frame under the .A words on top (vm_m: a call's arguments, going to the evaluator), vm_l words (1, a call's
; function's word there; 2, none) put under them for it: its resume vm_ip (this code past the call)
vm_kframe:
            jsr         vm_up
            lda         vm_m
            asl
            clc
            adc         #4
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         vm_ip
            ldx         vm_ip + 1
            jsr         vm_encode
            sta         (ht)
            txa
            ldy         #1
            sta         (ht),y
            lda         #<FIX(K_VM)
            iny
            sta         (ht),y
            lda         #0
            iny
            sta         (ht),y
            inc         depth
            bne         :+
            inc         depth + 1
:
            rts

; In tail position, going to the evaluator: this frame dropped, the .A words on top (a call's arguments) moved down
; to its place; under them, if its caller's the machine, a K_VM frame for it (resume its return)
vm_tailx:
            sta         vm_m
            ldy         vm_rb
            lda         (vm_s),y
            sta         vm_t
            iny
            lda         (vm_s),y
            sta         vm_t + 1
            lda         depth
            bne         :+
            dec         depth + 1
:
            dec         depth
            lda         vm_s                                ; (Where they go: the frame's place, or past a K_VM
            sta         hq                                  ;   frame's two words)
            lda         vm_s + 1
            sta         hq + 1
            lda         vm_t
            ora         vm_t + 1
            beq         :+
            lda         hq
            clc
            adc         #4
            sta         hq
            bcc         :+
            inc         hq + 1
:
            lda         vm_m                                ; (From sp - 2m, up)
            asl
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #0
            cpy         vm_k
            beq         @set
@copy:
            lda         (ht),y
            sta         (hq),y
            iny
            cpy         vm_k
            bne         @copy
@set:
            lda         hq
            clc
            adc         vm_k
            pha
            lda         hq + 1
            adc         #0
            tax
            pla
            jsr         vm_setsp
            lda         vm_t
            ora         vm_t + 1
            beq         @done
            lda         vm_t                                ; (vm_s: moved with the rest, if a block came back)
            sta         (vm_s)
            ldy         #1
            lda         vm_t + 1
            sta         (vm_s),y
            lda         #<FIX(K_VM)
            iny
            sta         (vm_s),y
            lda         #0
            iny
            sta         (vm_s),y
            inc         depth
            bne         @done
            inc         depth + 1
@done:
            rts

; Out to the evaluator at .A, .X (ev_head, ev_eval: nothing on the stack for it), its value for the code at vm_ip
; (whose last byte, r, is 0 in tail position: then this frame's done first)
vm_out0:
            sta         vm_u
            stx         vm_u + 1
            jsr         vm_rbyte
            bne         :+
            lda         #0
            jsr         vm_tailx
            bra         vm_go
:
            lda         #0
            ldx         #2
            jsr         vm_kframe
vm_go:
            stz         vm_on
            lda         vm_u
            ldx         vm_u + 1
            jmp         vm_go1

; Out to the evaluator at .A, .X (ev_apply, ev_vmnote), cf applied to the vm_m arguments on top, .Y words under
; them (1: cf's word; 0: none): as vm_out0
vm_outa:
            sta         vm_u
            stx         vm_u + 1
            sty         vm_l
            jsr         vm_rbyte
            bne         :+
            lda         vm_m
            jsr         vm_tailx
            bra         @go
:
            lda         #2
            sec
            sbc         vm_l
            tax
            lda         vm_m
            jsr         vm_kframe
@go:
            lda         vm_m
            sta         cn
            stz         cn + 1
            lda         #1
            sta         ev_inargs
            bra         vm_go

; ---- In and out

; cf, a compiled function, applied to its cn values on top of the stack (ev_fapply's, as many as its formals): its
; frame made of them, under them its word; its code run
vm_enter:
            stz         ev_inargs
            lda         cn
            ldx         #1
            jsr         vm_up
            jsr         vm_below
            lda         cf
            sta         (ht)
            ldy         #1
            lda         cf + 1
            sta         (ht),y
            stz         vm_t                                ; (Its return: the evaluator's)
            stz         vm_t + 1
            jsr         vm_frame0
            lda         cf
            ldx         cf + 1
            ldy         #4
            jsr         cell_get
            jsr         vm_decode
            lda         #1
            sta         vm_on
            jmp         vm_next

; A value from the evaluator (k_vm's: its K_VM frame popped, its resume on top), ex: the code goes on
vm_resume:
            jsr         pop
            jsr         vm_decode
            lda         sp
            sta         vm_s
            lda         sp + 1
            sta         vm_s + 1
            jsr         vm_callsite
            lda         #1
            sta         vm_on
            bcc         :+
            jsr         vm_iserr
            bcc         :+
            jmp         vm_ret
:
            jmp         vm_next

; An abort while the machine runs (ev_abort's: no room, too deep ...), ex its error: this frame returns it
vm_abort:
            ldx         ev_s
            txs
; ex returned: this frame dropped; its caller's code goes on (a return of the machine's), or the evaluator's
vm_ret:
op_ret:
            ldy         vm_rb
            lda         (vm_s),y
            sta         vm_t
            iny
            lda         (vm_s),y
            sta         vm_t + 1
            lda         depth
            bne         :+
            dec         depth + 1
:
            dec         depth
            lda         vm_s
            ldx         vm_s + 1
            jsr         vm_setsp
            lda         vm_t
            ora         vm_t + 1
            bne         :+
            stz         vm_on                               ; (The evaluator's call: the value for its frame on top)
            LDAX        ev_return
            jmp         vm_go1
:
            lda         vm_t
            ldx         vm_t + 1
            jsr         vm_decode
            jsr         vm_callsite
            bcc         :+
            jsr         vm_iserr
            bcs         vm_ret
:
            jmp         vm_next

; ee = this frame's scope, made if it isn't yet (its parent the function's; its formals bound to its arguments, which
; are read there from now on).  It may collect
vm_scope:
            bit         vm_mat
            bpl         @make
            ldy         vm_rb
            iny
            iny
            lda         (vm_s),y
            sta         ee
            iny
            lda         (vm_s),y
            sta         ee + 1
            rts
@make:
            jsr         vm_self
            ldy         #2
            jsr         cell_get
            sta         eu
            stx         eu + 1
            lda         vm_rb
            lsr
            dec         a
            FARN        1, frame_new
            MOVW        ee, hv
            ldy         vm_rb
            iny
            iny
            lda         ee
            sta         (vm_s),y
            iny
            lda         ee + 1
            sta         (vm_s),y
            jsr         vm_self                             ; (Its formals, each bound to its argument)
            ldy         #0
            jsr         cell_get
            sta         vm_u
            stx         vm_u + 1
            lda         #2
            sta         vm_k
@formal:
            lda         vm_u
            ora         vm_u + 1
            beq         @done
            ldy         vm_k
            lda         (vm_s),y
            sta         eu
            iny
            lda         (vm_s),y
            sta         eu + 1
            lda         vm_u
            ldx         vm_u + 1
            jsr         deref
            ldy         #2
            lda         (hp),y
            sta         vm_t
            iny
            lda         (hp),y
            sta         vm_t + 1
            ldy         #1
            lda         (hp),y
            tax
            lda         (hp)
            FARN        1, frame_add
            MOVW        vm_u, vm_t
            inc         vm_k
            inc         vm_k
            bra         @formal
@done:
            lda         #$80
            sta         vm_mat
            rts

; OUT: .A, .X = this frame's function
vm_self:
            ldy         #1
            lda         (vm_s),y
            tax
            lda         (vm_s)
            rts

; ex = the symbol after the op
vm_symex:
            lda         vm_bank
            sta         $00
            ldy         #1
            lda         (vm_ip),y
            sta         ex
            iny
            lda         (vm_ip),y
            sta         ex + 1
            rts

; OUT: .A, .X = the arguments, as written, of the call after the op (HEAD's)
vm_cargs:
            lda         vm_bank
            sta         $00
            ldy         #2
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            ldy         #1
            jmp         cell_get

; ---- The ops

op_const:
            ldy         #1
            lda         (vm_ip),y
            sta         ex
            iny
            lda         (vm_ip),y
            sta         ex + 1
            jmp         vm_adv3

op_local:
            ldy         #1
            lda         (vm_ip),y
            bit         vm_mat
            bmi         @scope
            tay
            lda         (vm_s),y
            sta         ex
            iny
            lda         (vm_s),y
            sta         ex + 1
            jmp         vm_adv2
@scope:
            asl                                             ; (In the scope: its pair's value, at 2y + 2)
            inc         a
            inc         a
            pha
            ldy         vm_rb
            iny
            iny
            lda         (vm_s),y
            pha
            iny
            lda         (vm_s),y
            tax
            pla
            jsr         deref
            ply
            lda         (hp),y
            sta         ex
            iny
            lda         (hp),y
            sta         ex + 1
            jmp         vm_adv2

op_global:
            bit         vm_mat
            bmi         @look
            ldy         #2
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            jsr         deref
            ldy         #3
            lda         (hp),y
            sta         ex + 1
            dey
            lda         (hp),y
            sta         ex
            cmp         #<UNBOUND
            bne         :+
            lda         ex + 1
            cmp         #>UNBOUND
            beq         @look
:
            jmp         vm_adv3
@look:
            stz         ee                                  ; (Unbound, or in a scope made: lookup's way)
            stz         ee + 1
            bit         vm_mat
            bpl         :+
            jsr         vm_scope
:
            jsr         vm_symex
            FARN        1, lookup
            lda         #3
            jmp         vm_valerr

op_lookup:
            bit         vm_mat
            bpl         :+
            jsr         vm_scope
            bra         @look
:
            jsr         vm_self
            ldy         #2
            jsr         cell_get
            sta         ee
            stx         ee + 1
@look:
            jsr         vm_symex
            FARN        1, lookup
            lda         #3
            jmp         vm_valerr

op_push:
            lda         ex
            ldx         ex + 1
            jsr         push
            jmp         vm_adv1

op_jf:
            lda         ex + 1
            bne         @no
            lda         ex
            beq         @to
            cmp         #<SNIL
            beq         @to
@no:
            jmp         vm_adv3
@to:
            ldy         #1
            jmp         vm_jumpy

op_jt:
            lda         ex + 1
            bne         @to
            lda         ex
            beq         @no
            cmp         #<SNIL
            bne         @to
@no:
            jmp         vm_adv3
@to:
            ldy         #1
            jmp         vm_jumpy

op_jmp:
            ldy         #1
            jmp         vm_jumpy

op_je:
            jsr         vm_iserr
            bcs         :+
            jmp         vm_adv4
:
            ldy         #1
            lda         (vm_ip),y
            beq         :+
            jsr         drop
:
            lda         vm_bank
            sta         $00
            ldy         #2
            jmp         vm_jumpy

op_head:
            lda         ex
            lsr
            bcs         @notfn
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcc         @notfn
            txa                                             ; (A built-in: not a special form, nor one that takes
            lsr                                             ;   errors, whose arguments are compiled as a function's)
            lda         ex
            ror
            tax
            lda         vb_flags,x
            and         #BF_SPECIAL | BF_ERRORS
            bne         @eval
            bra         @push
@cell:
            lda         pk,x
            cmp         #PK_FUNC
            beq         @func
            cmp         #PK_ERROR
            beq         @error
            cmp         #PK_PARTIAL
            beq         @eval
            cmp         #PK_HASH
            beq         @eval
            cmp         #PK_BUFFER
            beq         @eval
@notfn:
            jsr         vm_cargs                            ; (Not a function: with no arguments, itself; else the
            stx         vm_k                                ;   evaluator's, its error or exit's)
            ora         vm_k
            bne         @eval
@to:
            lda         vm_bank
            sta         $00
            ldy         #3
            jmp         vm_jumpy
@error:
            lda         (vm_ip)
            bpl         @to
            jmp         vm_ret
@func:
            lda         ex
            ldy         #3
            jsr         cell_get
            and         #FN_FEXPR
            bne         @eval
@push:
            lda         ex
            ldx         ex + 1
            jsr         push
            jmp         vm_adv5
@eval:
            jsr         vm_scope                            ; (The evaluator's, from the function: ev_head's, with
            jsr         vm_cargs                            ;   the arguments as written, in this frame's scope)
            sta         cr
            stx         cr + 1
            lda         vm_bank
            sta         $00
            ldy         #3
            lda         (vm_ip),y
            tax
            iny
            lda         (vm_ip),y
            sta         vm_ip + 1
            stx         vm_ip
            LDAX        ev_head
            jmp         vm_out0

op_tcall:
            lda         #$80
            bra         :+
op_call:
            lda         #0
:
            sta         vm_tl
            ldy         #1
            lda         (vm_ip),y
            sta         vm_m
            lda         vm_ip                               ; (ip: past it, where its value goes)
            clc
            adc         #4
            sta         vm_ip
            bcc         :+
            inc         vm_ip + 1
:
            jsr         vm_below                            ; (cf: the function, under its arguments)
            lda         (ht)
            sta         cf
            ldy         #1
            lda         (ht),y
            sta         cf + 1
            bit         intr                                ; (Ctrl-C: the error interrupted; a note for on-note's
            bpl         :+                                  ;   function: it's called first, the evaluator's way)
            jmp         vm_cintr
:
            bvc         :+
            jmp         vm_cnote
:
            ldx         cf + 1
            cpx         #IMM_PAGES
            bcs         :+
            jmp         vm_cbuiltin
:
            lda         pk,x
            cmp         #PK_FUNC
            bne         vm_cout
            lda         cf
            jsr         deref
            ldy         #6
            lda         (hp),y                              ; (An fexpr, or one that takes extras: the evaluator's)
            and         #FN_FEXPR | FN_EXTRAS
            bne         vm_cout
            iny
            lda         (hp),y                              ; (As many arguments as its formals)
            cmp         vm_m
            bne         vm_cout
            iny
            lda         (hp),y                              ; (Its code)
            sta         vm_u
            iny
            lda         (hp),y
            sta         vm_u + 1
            ora         vm_u
            bne         vm_cfunc
            ldy         #6                                  ; (None yet: compiled at its second call, unless it can't
            lda         (hp),y                              ;   be)
            bmi         vm_cout
            ldy         #10
            lda         (hp),y
            bne         :+
            lda         #<FIX(1)
            sta         (hp),y
            bra         vm_cout
:
            jsr         vm_compile
            bcs         vm_cout
            lda         cf
            ldx         cf + 1
            ldy         #4
            jsr         cell_get
            sta         vm_u
            stx         vm_u + 1
            bra         vm_cfunc

; The call made through the evaluator (ev_apply): a function not compiled, a built-in that runs the machine ...
vm_cout:
            LDAX        ev_apply
vm_cout2:
            ldy         #1
            jmp         vm_outa

; A note pending: the call made through the evaluator, on-note's function given it first (ev_vmnote)
vm_cnote:
            LDAX        ev_vmnote
            bra         vm_cout2

; A compiled function called: its frame made (in tail position, in this one's place), its code run
vm_cfunc:
            bit         vm_tl
            bmi         @tail
            lda         depth + 1                           ; (Too deep: the error)
            cmp         #>MAX_DEPTH
            bcc         :+
            bne         vm_cdeep
            lda         depth
            cmp         #<MAX_DEPTH
            bcs         vm_cdeep
:
            lda         vm_ip                               ; (Its return: this code past the call)
            ldx         vm_ip + 1
            jsr         vm_encode
            sta         vm_t
            stx         vm_t + 1
            jsr         vm_frame0
            bra         @code
@tail:
            ldy         vm_rb                               ; (This frame's return kept; the function and its
            lda         (vm_s),y                            ;   arguments moved down to its place)
            sta         vm_t
            iny
            lda         (vm_s),y
            sta         vm_t + 1
            jsr         vm_below
            lda         vm_m
            asl
            adc         #2
            sta         vm_k
            ldy         #0
:
            lda         (ht),y
            sta         (vm_s),y
            iny
            cpy         vm_k
            bne         :-
            lda         vm_s
            clc
            adc         vm_k
            pha
            lda         vm_s + 1
            adc         #0
            tax
            pla
            jsr         vm_setsp
            lda         depth
            bne         :+
            dec         depth + 1
:
            dec         depth
            jsr         vm_frame0
@code:
            lda         vm_u
            ldx         vm_u + 1
            jsr         vm_decode
            jmp         vm_next

; Too deep: the error, the call's value
vm_cdeep:
            LDAX        s_toodeep
            FARN        1, err_text
            bcc         vm_cdrop
            MOVW        ex, nomem_err
; The call's function and arguments dropped, ex its value
vm_cdrop:
            lda         vm_m
            inc         a
            jsr         drop
; ex, a call's value, for the code at vm_ip (in tail position, returned; else, an error returned if the call's r says)
vm_cvalue:
            bit         vm_tl
            bmi         @ret
            jsr         vm_rbyte
            lsr
            bcc         @next
            jsr         vm_iserr
            bcs         @ret
@next:
            jmp         vm_next
@ret:
            jmp         vm_ret

; Ctrl-C: the error interrupted, the call's value
vm_cintr:
            FARN        1, intr_err
            bcc         vm_cdrop
            MOVW        ex, nomem_err
            bra         vm_cdrop

; A built-in called (cf, as HEAD let it through: not a special form, nor one that takes errors): here, unless it
; runs the machine or isn't given as many arguments as it takes (the evaluator's: partially applied, or an error)
vm_cbuiltin:
            cpx         #>BUILTIN0
            bcs         @lb724
            jmp         vm_cout
@lb724:
            txa
            lsr
            lda         cf
            ror
            sta         bi_n
            tax
            lda         vb_flags,x
            and         #BF_MACHINE | BF_SPECIAL
            bne         @out
            lda         vm_m
            cmp         vb_min,x
            bcc         @out
            cmp         vb_max,x
            beq         :+
            bcs         @out
:
            jsr         vm_bcode
            lda         #1                                  ; (Its word too)
            jsr         drop
            bra         vm_cvalue
@out:
            lda         vb_vf,x
            bne         @lb723
            jmp         vm_cout
@lb723:
            jsr         vm_scope
            jmp         vm_cout

; Built-in bi_n's code given the vm_m values on top (cf the built-in): ex its value (no room: the error); the values
; dropped
vm_bcode:
            ldx         bi_n
            lda         vb_vf,x                             ; (One that looks at its caller's scope: made)
            beq         :+
            jsr         vm_scope
            ldx         bi_n
:
            lda         vm_m
            sta         cn
            stz         cn + 1
            asl
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         argp
            lda         sp + 1
            sbc         #0
            sta         argp + 1
            lda         vb_lo,x
            sta         hyx2_to
            lda         vb_hi,x
            sta         hyx2_to + 1
            lda         vb_flags,x
            and         #BF_BANK
            tax
            lda         hyx2_bank1,x
            sta         hyx2_n
            jsr         hyx2_farn
            bcc         :+
            MOVW        ex, nomem_err
:
            lda         vm_m
            jmp         drop

op_bcall:
            ldy         #1
            lda         (vm_ip),y
            sta         bi_n
            iny
            lda         (vm_ip),y
            sta         vm_m
            lda         vm_ip                               ; (ip: past it)
            clc
            adc         #5
            sta         vm_ip
            bcc         :+
            inc         vm_ip + 1
:
            stz         vm_tl
            lda         bi_n                                ; (cf: the built-in)
            asl
            sta         cf
            lda         #>BUILTIN0
            adc         #0
            sta         cf + 1
            ldx         bi_n
            lda         vb_flags,x
            and         #BF_MACHINE
            bne         @out
            lda         vm_m
            cmp         vb_min,x
            bcc         @out
            cmp         vb_max,x
            beq         :+
            bcs         @out
:
            jsr         vm_bcode
            jmp         vm_cvalue
@out:
            lda         vb_vf,x
            beq         :+
            jsr         vm_scope
:
            LDAX        ev_apply
            ldy         #0
            jmp         vm_outa

op_eval:
            jsr         vm_scope
            lda         vm_bank
            sta         $00
            ldy         #1
            lda         (vm_ip),y
            sta         ex
            iny
            lda         (vm_ip),y
            sta         ex + 1
            lda         vm_ip
            clc
            adc         #5
            sta         vm_ip
            bcc         :+
            inc         vm_ip + 1
:
            LDAX        ev_call                             ; (A call: the function's body too, a Q-expression)
            jmp         vm_out0

op_quote:
            jsr         vm_scope
            MOVW        eu, ee
            bra         vm_quote
op_quote0:
            stz         eu
            stz         eu + 1
vm_quote:
            lda         vm_bank
            sta         $00
            ldy         #1
            lda         (vm_ip),y
            sta         et
            iny
            lda         (vm_ip),y
            sta         et + 1
            lda         #PK_SCOPED
            jsr         make2
            bcs         :+
            MOVW        ex, hv
            jmp         vm_adv3
:
            MOVW        ex, nomem_err
            jmp         vm_ret

; ---- The quick ops: fixnums here, else their built-in's code

; vm_a = the value popped, vm_b = ex (an op of 1 byte)
vm_pop2:
            jsr         pop
            sta         vm_a
            stx         vm_a + 1
            MOVW        vm_b, ex
            lda         #1
            sta         vm_fl
            rts

; vm_a = ex, vm_b = the fixnum after the op (3 bytes)
vm_cst2:
            MOVW        vm_a, ex
            ldy         #1
            lda         (vm_ip),y
            sta         vm_b
            iny
            lda         (vm_ip),y
            sta         vm_b + 1
            lda         #3
            sta         vm_fl
            rts

; vm_a = ex (1 byte)
vm_one:
            MOVW        vm_a, ex
            lda         #1
            sta         vm_fl
            rts

op_add:
            jsr         vm_pop2
            bra         vm_add
op_addc:
            jsr         vm_cst2
vm_add:
            lda         vm_a
            and         vm_b
            lsr
            bcc         vm_fslow
            lda         vm_b
            and         #$FE
            clc
            adc         vm_a
            tax
            lda         vm_b + 1
            adc         vm_a + 1
            bvs         vm_fslow
vm_fix:
            sta         ex + 1
            stx         ex
vm_fdone:
            lda         vm_fl
            jmp         vm_adv

op_sub:
            jsr         vm_pop2
            bra         vm_sub
op_subc:
            jsr         vm_cst2
vm_sub:
            lda         vm_a
            and         vm_b
            lsr
            bcc         vm_fslow
            lda         vm_b
            and         #$FE
            sta         vm_k
            lda         vm_a
            sec
            sbc         vm_k
            tax
            lda         vm_a + 1
            sbc         vm_b + 1
            bvs         vm_fslow
            bra         vm_fix

; Not fixnums (or past one): the op's built-in on its values, the usual way; its error returned if the op's VM_R
vm_fslow:
            lda         vm_bank
            sta         $00
            lda         (vm_ip)
            and         #$7F
            sec
            sbc         #VO_ADD
            lsr
            tax
            lda         vo_n,x
            sta         vm_m
            lda         vo_bin,x
            sta         bi_n
            asl
            sta         cf
            lda         #>BUILTIN0
            adc         #0
            sta         cf + 1
            lda         vm_a
            ldx         vm_a + 1
            jsr         push
            lda         vm_m
            cmp         #2
            bcc         :+
            lda         vm_b
            ldx         vm_b + 1
            jsr         push
:
            jsr         vm_bcode
            lda         vm_bank
            sta         $00
            lda         (vm_ip)
            bpl         vm_fdone
            jsr         vm_iserr
            bcc         vm_fdone
            jmp         vm_ret

vm_true:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         vm_fdone
vm_false:
            stz         ex
            stz         ex + 1
            jmp         vm_fdone

; OUT: C = 1 if vm_a < vm_b (vm_less), vm_b < vm_a (vm_more): fixnums, signed
vm_less:
            lda         vm_a
            cmp         vm_b
            lda         vm_a + 1
            sbc         vm_b + 1
            bvc         :+
            eor         #$80
:
            asl
            rts
vm_more:
            lda         vm_b
            cmp         vm_a
            lda         vm_b + 1
            sbc         vm_a + 1
            bvc         :+
            eor         #$80
:
            asl
            rts

op_lt:
            jsr         vm_pop2
            bra         vm_lt
op_ltc:
            jsr         vm_cst2
vm_lt:
            lda         vm_a
            and         vm_b
            lsr
            bcs         @lb722
            jmp         vm_fslow
@lb722:
            jsr         vm_less
            bcs         vm_true
            bra         vm_false

op_gt:
            jsr         vm_pop2
            bra         vm_gt
op_gtc:
            jsr         vm_cst2
vm_gt:
            lda         vm_a
            and         vm_b
            lsr
            bcs         @lb721
            jmp         vm_fslow
@lb721:
            jsr         vm_more
            bcs         vm_true
            bra         vm_false

op_le:
            jsr         vm_pop2
            bra         vm_le
op_lec:
            jsr         vm_cst2
vm_le:
            lda         vm_a
            and         vm_b
            lsr
            bcs         @lb720
            jmp         vm_fslow
@lb720:
            jsr         vm_more
            bcs         vm_false
            jmp         vm_true

op_ge:
            jsr         vm_pop2
            bra         vm_ge
op_gec:
            jsr         vm_cst2
vm_ge:
            lda         vm_a
            and         vm_b
            lsr
            bcs         @lb719
            jmp         vm_fslow
@lb719:
            jsr         vm_less
            bcc         @lb718
            jmp         vm_false
@lb718:
            jmp         vm_true

op_neq:
            jsr         vm_pop2
            bra         vm_neq
op_eqc:
            jsr         vm_cst2
vm_neq:
            lda         vm_a
            and         vm_b
            lsr
            bcs         @lb717
            jmp         vm_fslow
@lb717:
            lda         vm_a
            cmp         vm_b
            beq         @lb716
            jmp         vm_false
@lb716:
            lda         vm_a + 1
            cmp         vm_b + 1
            beq         @lb715
            jmp         vm_false
@lb715:
            jmp         vm_true

op_inc:
            jsr         vm_one
            lda         vm_a
            lsr
            bcs         @lb714
            jmp         vm_fslow
@lb714:
            lda         vm_a
            clc
            adc         #2
            tax
            lda         vm_a + 1
            adc         #0
            bvc         @lb713
            jmp         vm_fslow
@lb713:
            jmp         vm_fix

op_dec:
            jsr         vm_one
            lda         vm_a
            lsr
            bcs         @lb712
            jmp         vm_fslow
@lb712:
            lda         vm_a
            sec
            sbc         #2
            tax
            lda         vm_a + 1
            sbc         #0
            bvc         @lb711
            jmp         vm_fslow
@lb711:
            jmp         vm_fix

op_zerop:
            jsr         vm_one
            lda         vm_a
            lsr
            bcs         @lb710
            jmp         vm_fslow
@lb710:
            lda         vm_a
            cmp         #<FIX(0)
            beq         @lb709
            jmp         vm_false
@lb709:
            lda         vm_a + 1
            beq         @lb708
            jmp         vm_false
@lb708:
            jmp         vm_true

op_onep:
            jsr         vm_one
            lda         vm_a
            lsr
            bcs         @lb707
            jmp         vm_fslow
@lb707:
            lda         vm_a
            cmp         #<FIX(1)
            beq         @lb706
            jmp         vm_false
@lb706:
            lda         vm_a + 1
            beq         @lb705
            jmp         vm_false
@lb705:
            jmp         vm_true

; ---- The compiler

; cf, a function: its body compiled, its word 4 its code's place (fun's, as it's defined; a call's, at its second).
; OUT: C = 0 if it's compiled; C = 1 if not (FN_NOVM set if it can't be: its formals, its body too big ...; or the
; arena's full, or it takes extras).  It makes nothing on the heap (so it doesn't collect)
vm_compile:
            lda         vm_full
            bne         @no
            lda         cf
            ldx         cf + 1
            ldy         #3
            jsr         cell_get                            ; (Its flags; its formals counted)
            and         #FN_EXTRAS | FN_NOVM
            beq         :+
@no:
            sec
            rts
:
            stx         vc_v
            tsx
            stx         vc_s
            ldx         vc_v
            cpx         #VC_FORMALS + 1
            bcc         :+
            jmp         vc_novm
:
            stx         vc_v
            txa
            asl
            adc         #2
            sta         vc_rb
            lda         cf
            ldx         cf + 1
            ldy         #2
            jsr         cell_get                            ; (Its scope: none, its free names globals)
            stx         vc_snil
            ora         vc_snil
            sta         vc_snil
            lda         cf
            ldx         cf + 1
            ldy         #0
            jsr         cell_get                            ; (Its formals: symbols, none twice)
            sta         vc_q
            stx         vc_q + 1
            stz         vc_n
@formal:
            lda         vc_q
            ora         vc_q + 1
            beq         @formals
            lda         vc_q
            ldx         vc_q + 1
            jsr         vc_split
            sta         vc_hd
            stx         vc_hd + 1
            lsr
            bcs         @bad
            cpx         #IMM_PAGES
            bcc         @bad
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @bad
            lda         vc_hd
            jsr         vc_formal
            bcc         @bad
            ldx         vc_n
            lda         vc_hd
            sta         vc_fs_lo,x
            lda         vc_hd + 1
            sta         vc_fs_hi,x
            inc         vc_n
            bra         @formal
@bad:
            jmp         vc_novm
@formals:
            lda         vc_n
            cmp         vc_v
            bne         @bad
            lda         ar_n                                ; (The arena: its first bank)
            bne         vc_again
            jsr         vc_bank
            bcc         vc_again
            jmp         vc_full
vc_again:
            ldx         ar_cur                              ; (At the arena's top)
            lda         ar_bank,x
            sta         vc_bk
            txa
            asl
            asl
            asl
            asl
            asl
            sta         vc_idx
            lda         ar_top
            sta         hq
            sta         vc_start
            lda         ar_top + 1
            sta         hq + 1
            sta         vc_start + 1
            stz         vc_nlab
            stz         vc_lv
            lda         vc_n
            clc
            adc         #3
            sta         vc_h
            lda         cf
            ldx         cf + 1
            ldy         #1
            jsr         cell_get                            ; (Its body: a call, in tail position)
            sta         L_elo
            stx         L_ehi
            ora         L_ehi
            bne         :+
            lda         #VO_CONST                           ; (None: NIL)
            jsr         vc_byte
            lda         #0
            tax
            jsr         vc_word
            bra         @ret
:
            lda         #1
            sta         L_tail
            lda         #M_R
            sta         L_mode
            jsr         vc_call
@ret:
            lda         #VO_RET
            jsr         vc_byte
            lda         hq                                  ; (Its code's place: the function's word 4)
            sta         ar_top
            lda         hq + 1
            sta         ar_top + 1
            lda         vc_start
            asl
            ora         #1
            sta         vc_t
            lda         vc_start + 1
            and         #$1F
            ora         vc_idx
            rol
            sta         vc_t + 1
            lda         cf
            ldx         cf + 1
            jsr         deref
            ldy         #8
            lda         vc_t
            sta         (hp),y
            iny
            lda         vc_t + 1
            sta         (hp),y
            clc
            rts

; The bank filled: the code begun again in the next (taken, if it isn't yet), unless it began at a bank's start
vc_over:
            ldx         vc_s
            txs
            lda         vc_start
            bne         :+
            lda         vc_start + 1
            cmp         #$80
            beq         vc_novm
:
            ldx         ar_cur
            inx
            cpx         ar_n
            bcc         :+
            cpx         #4
            bcs         vc_full
            jsr         vc_bank
            bcs         vc_full
            ldx         ar_cur
            inx
:
            stx         ar_cur
            stz         ar_top
            lda         #$80
            sta         ar_top + 1
            jmp         vc_again

; The arena's full: nothing compiled till it's emptied
vc_full:
            lda         #1
            sta         vm_full
            sec
            rts

; Something the compiler doesn't do (nested too deep, a frame too big ...): the function's always evaluated
vc_fail:
            ldx         vc_s
            txs
vc_novm:
            lda         cf
            ldx         cf + 1
            jsr         deref
            ldy         #6
            lda         (hp),y
            ora         #FN_NOVM
            sta         (hp),y
            sec
            rts

; A bank more for the arena.  OUT: C = 1 if there's none
vc_bank:
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @rts
            ldx         ar_n
            sta         ar_bank,x
            inc         ar_n
            clc
@rts:
            rts

; .A emitted (hq, in the arena's bank vc_bk; past it, vc_over).  Keeps .X
vc_byte:
            ldy         hq + 1
            cpy         #$A0
            bcc         :+
            jmp         vc_over
:
            ldy         vc_bk
            sty         $00
            sta         (hq)
            inc         hq
            bne         :+
            inc         hq + 1
:
            rts

; .A, .X emitted
vc_word:
            jsr         vc_byte
            txa
            jmp         vc_byte

; OUT: .A = a new label (not defined, nothing refers to it yet)
vc_lab:
            lda         vc_nlab
            cmp         #VC_LABS
            bcc         :+
            jmp         vc_fail
:
            tax
            stz         lb_lo,x
            stz         lb_hi,x
            stz         lb_def,x
            inc         vc_nlab
            txa
            rts

; Label .A's address emitted (not defined yet: this place on its chain, to be patched)
vc_ref:
            sta         vc_x
            tax
            lda         lb_def,x
            bne         @def
            lda         hq
            sta         vc_t
            lda         hq + 1
            sta         vc_t + 1
            lda         lb_lo,x
            pha
            lda         lb_hi,x
            tax
            pla
            jsr         vc_word
            ldx         vc_x
            lda         vc_t
            sta         lb_lo,x
            lda         vc_t + 1
            sta         lb_hi,x
            rts
@def:
            lda         lb_lo,x
            pha
            lda         lb_hi,x
            tax
            pla
            jmp         vc_word

; Label .A defined here: each place on its chain patched
vc_def:
            tax
@ref:
            lda         lb_lo,x
            sta         ht
            lda         lb_hi,x
            sta         ht + 1
            ora         ht
            beq         @done
            lda         vc_bk
            sta         $00
            ldy         #0
            lda         (ht),y
            sta         lb_lo,x
            lda         hq
            sta         (ht),y
            iny
            lda         (ht),y
            sta         lb_hi,x
            lda         hq + 1
            sta         (ht),y
            bra         @ref
@done:
            lda         hq
            sta         lb_lo,x
            lda         hq + 1
            sta         lb_hi,x
            lda         #1
            sta         lb_def,x
            rts

; PUSH emitted: the frame a word higher
vc_push:
            lda         #VO_PUSH
            jsr         vc_byte
vc_hinc:
            inc         vc_h
            lda         vc_h
            cmp         #VC_HMAX
            bcc         :+
            jmp         vc_fail
:
            rts

; IN: .A, .X = a cons.  OUT: .A, .X = its first; vc_q = the rest
vc_split:
            jsr         deref
            ldy         #2
            lda         (hp),y
            sta         vc_q
            iny
            lda         (hp),y
            sta         vc_q + 1
            ldy         #1
            lda         (hp),y
            tax
            lda         (hp)
            rts

; IN: .A, .X = a symbol.  OUT: C = 0 and .A = its argument's place in the frame (2 + 2k) if it's formal k
vc_formal:
            sta         vc_t
            stx         vc_t + 1
            ldy         #0
@k:
            cpy         vc_n
            bcs         @no
            lda         vc_fs_lo,y
            cmp         vc_t
            bne         @next
            lda         vc_fs_hi,y
            cmp         vc_t + 1
            beq         @yes
@next:
            iny
            bra         @k
@yes:
            tya
            asl
            adc         #2
            clc
            rts
@no:
            sec
            rts

; IN: .A, .X = a value (an expression's part).  OUT: C = 1 if it's a name, or has one in it (a list's), or might
vc_syms:
            pha
            lsr
            pla
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            ldy         pk,x
            cpy         #PK_SYMBOL
            beq         @yes
            cpy         #PK_SCOPED
            beq         @yes
            cpy         #PK_SCONS
            beq         @list
            cpy         #PK_QCONS
            beq         @list
@no:
            clc
            rts
@yes:
            sec
            rts
@list:
            inc         vc_sd
            ldy         vc_sd
            cpy         #VC_SYMS
            bcs         @found
@item:
            pha
            phx
            ldy         #0
            jsr         cell_get
            jsr         vc_syms
            plx
            pla
            bcs         @found
            ldy         #1
            jsr         cell_get
            sta         vc_t
            stx         vc_t + 1
            ora         vc_t + 1
            beq         @none
            lda         vc_t
            bra         @item
@none:
            dec         vc_sd
            clc
            rts
@found:
            dec         vc_sd
            sec
            rts

; A part compiled, a level down: .A, .X its expression, .Y <> 0 in tail position (its way M_R), vc_cm, vc_cj, vc_ch
; its way with an error
vc_child:
            pha
            lda         vc_lv
            cmp         #VC_LV - 1
            bcc         :+
            jmp         vc_fail
:
            inc         vc_lv
            phx
            ldx         vc_lv
            pla
            sta         L_ehi,x
            pla
            sta         L_elo,x
            tya
            sta         L_tail,x
            beq         :+
            lda         #M_R
            sta         vc_cm
:
            lda         vc_cm
            sta         L_mode,x
            lda         vc_cj
            sta         L_jlab,x
            lda         vc_ch
            sta         L_jh,x
            jsr         vc_expr
            dec         vc_lv
            rts

; This level's parts that don't take errors: their way (L_am ...), as its own if it's M_R or M_J; if it's M_OK, a
; jump to label .A (its end), the frame's height as it began
vc_amode:
            ldx         vc_lv
            ldy         L_mode,x
            cpy         #M_OK
            beq         @ok
            tya
            sta         L_am,x
            lda         L_jlab,x
            sta         L_aj,x
            lda         L_jh,x
            sta         L_ah,x
            rts
@ok:
            sta         L_aj,x
            lda         #M_J
            sta         L_am,x
            lda         L_h0,x
            sta         L_ah,x
            rts

; vc_cm, vc_cj, vc_ch = this level's parts' way (vc_amset), or its own (vc_same)
vc_amset:
            ldx         vc_lv
            lda         L_am,x
            sta         vc_cm
            lda         L_aj,x
            sta         vc_cj
            lda         L_ah,x
            sta         vc_ch
            rts
vc_same:
            ldx         vc_lv
            lda         L_mode,x
            sta         vc_cm
            lda         L_jlab,x
            sta         vc_cj
            lda         L_jh,x
            sta         vc_ch
            rts

; OUT: .A, .X = this level's next argument (L_alo/hi: the rest)
vc_next:
            ldx         vc_lv
            lda         L_alo,x
            pha
            lda         L_ahi,x
            tax
            pla
            jsr         vc_split
            pha
            phx
            ldx         vc_lv
            lda         vc_q
            sta         L_alo,x
            lda         vc_q + 1
            sta         L_ahi,x
            plx
            pla
            rts

; The next argument compiled, in tail position if this level is (its way vc_cm ...)
vc_nextt:
            jsr         vc_next
            pha
            phx
            ldx         vc_lv
            ldy         L_tail,x
            plx
            pla
            jmp         vc_child

; Each argument left compiled in turn, its way L_am ..., and pushed
vc_args:
            ldx         vc_lv
            lda         L_alo,x
            ora         L_ahi,x
            beq         @done
            jsr         vc_amset
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            jsr         vc_push
            bra         vc_args
@done:
            rts

; An op .A that may give an error, emitted as this level's way wants: M_R, its VM_R variant; else as it is (M_J: a JE
; after it, vc_je)
vc_rop:
            ldx         vc_lv
            ldy         L_mode,x
            bne         :+
            ora         #VM_R
:
            jmp         vc_byte

; After an op whose value may be an error: M_J, a JE to the level's label (the words over its height dropped)
vc_je:
            ldx         vc_lv
            lda         L_mode,x
            cmp         #M_J
            bne         @no
            lda         #VO_JE
            jsr         vc_byte
            ldx         vc_lv
            lda         vc_h
            sec
            sbc         L_jh,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_jlab,x
            jmp         vc_ref
@no:
            rts

; The r byte of a call, EVAL ...: 0 in tail position; else the record's place, and R (bit 0) if the level's M_R
vc_rbyte:
            ldx         vc_lv
            lda         L_tail,x
            beq         :+
            lda         #0
            jmp         vc_byte
:
            lda         L_mode,x
            bne         :+
            lda         vc_rb
            ora         #1
            jmp         vc_byte
:
            lda         vc_rb
            jmp         vc_byte

; An expression compiled (this level's): its value in ex; in tail position, returned
vc_expr:
            ldx         vc_lv
            lda         L_elo,x
            lsr
            bcs         @const
            lda         L_ehi,x
            cmp         #IMM_PAGES
            bcc         @imm
            tay
            lda         pk,y
            cmp         #PK_SYMBOL
            beq         @sym
            cmp         #PK_SCONS
            beq         @call
            cmp         #PK_QCONS
            beq         @quote
@const:
            lda         #VO_CONST                           ; (Anything else: itself)
            jsr         vc_byte
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            bra         @done
@imm:
            cmp         #0                                  ; (() is NIL)
            bne         @const
            lda         L_elo,x
            cmp         #<SNIL
            bne         @const
            lda         #VO_CONST
            jsr         vc_byte
            lda         #0
            tax
            jsr         vc_word
            bra         @done
@call:
            jsr         vc_call
            bra         @done
@quote:
            lda         L_elo,x                             ; (A Q-expression: with its scope, if it has names)
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_syms
            lda         #VO_QUOTE0
            bcc         :+
            lda         #VO_QUOTE
:
            jsr         vc_byte
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            bra         @done
@sym:
            lda         L_elo,x                             ; (A name: a formal's argument; else its value, a
            pha                                             ;   global's, or looked up in the function's scope)
            lda         L_ehi,x
            tax
            pla
            jsr         vc_formal
            bcs         @free
            pha
            lda         #VO_LOCAL
            jsr         vc_byte
            pla
            jsr         vc_byte
            bra         @done
@free:
            lda         #VO_LOOKUP
            ldx         vc_snil
            bne         :+
            lda         #VO_GLOBAL
:
            jsr         vc_rop
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            jsr         vc_je
@done:
            ldx         vc_lv
            lda         L_tail,x
            beq         :+
            lda         #VO_RET
            jmp         vc_byte
:
            rts

; A call compiled (this level's expression, an S-expression): a special form inlined (if, do, and, or) or the
; evaluator's; a quick op; a built-in; else any function (HEAD, CALL)
vc_call:
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_split
            sta         vc_hd
            stx         vc_hd + 1
            ldx         vc_lv
            lda         vc_q
            sta         L_alo,x
            lda         vc_q + 1
            sta         L_ahi,x
            lda         vc_q
            ldx         vc_q + 1
            jsr         count_list
            cmp         #VC_ARGS
            bcc         :+
            jmp         vc_fail
:
            ldx         vc_lv
            sta         L_m,x
            lda         vc_hd                               ; (Its function a name, not a formal's ...
            lsr
            bcc         @lb801
            jmp         @any
@lb801:
            ldx         vc_hd + 1
            cpx         #IMM_PAGES
            bcc         @any
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @any
            lda         vc_hd
            jsr         vc_formal
            bcc         @any
            lda         vc_hd                               ;   its value now, a global's, if no frame has ever bound
            ldx         vc_hd + 1                           ;   the name)
            jsr         deref
            ldy         #6
            lda         (hp),y
            and         #SF_LOCAL
            bne         @any
            ldy         #2
            lda         (hp),y
            sta         vc_v
            iny
            lda         (hp),y
            sta         vc_v + 1
            lda         vc_v
            lsr
            bcs         @any
            ldx         vc_v + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcc         @any
            txa                                             ; (A built-in)
            lsr
            lda         vc_v
            ror
            ldx         vc_lv
            sta         L_b,x
            tay
            lda         vb_flags,y
            bmi         @special
            jsr         vc_inl
            ldx         vc_lv
            ldy         L_b,x
            jsr         vc_quick
            bcs         :+
            jmp         vc_fast
:
            jmp         vc_bcall
@cell:
            lda         pk,x
            cmp         #PK_FUNC
            bne         @any
            lda         vc_v
            ldy         #3
            jsr         cell_get
            and         #FN_FEXPR
            beq         @any
            jmp         vc_eval                             ; (An fexpr's call: the evaluator's)
@any:
            jmp         vc_generic
@special:
            ldx         vc_lv
            lda         L_m,x
            cpy         #BIN_IF
            bne         :+
            cmp         #2
            bcc         @eval
            cmp         #4
            bcs         @eval
            jsr         vc_inl
            jmp         vc_if
:
            cpy         #BIN_DO
            bne         :+
            jsr         vc_inl
            jmp         vc_do
:
            cpy         #BIN_AND
            bne         :+
            jsr         vc_inl
            jmp         vc_and
:
            cpy         #BIN_OR
            bne         @eval
            jsr         vc_inl
            jmp         vc_or
@eval:
            jmp         vc_eval

; The call's function's name (vc_hd) marked: the code counts on its global value (vm_rebind's)
vc_inl:
            lda         vc_hd
            ldx         vc_hd + 1
            jsr         deref
            ldy         #6
            lda         (hp),y
            ora         #SF_INLINED
            sta         (hp),y
            rts

; Built-in .Y with this level's L_m arguments: a quick op?  OUT: C = 0, L_b = the op (plain), if it is
vc_quick:
            ldx         #QUICKS - 1
@q:
            tya
            cmp         vq_bin,x
            bne         @next
            ldy         vc_lv
            lda         L_m,y
            cmp         vq_n,x
            bne         @no
            lda         vq_op,x
            ldy         vc_lv
            sta         L_b,y
            clc
            rts
@next:
            dex
            bpl         @q
@no:
            sec
            rts

; The call: its function evaluated (its error the call's value), HEAD, its arguments pushed, CALL (TCALL)
vc_generic:
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_t,x
            lda         #M_OK                               ; (Its function: an error's for HEAD)
            sta         vc_cm
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            ldy         #0
            jsr         deref
            lda         (hp)
            pha
            ldy         #1
            lda         (hp),y
            tax
            pla
            ldy         #0
            jsr         vc_child
            lda         #VO_HEAD
            jsr         vc_rop
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_ref
            jsr         vc_hinc
            ldx         vc_lv                               ; (Its arguments: an error's the call's value)
            lda         L_t,x
            jsr         vc_amode
            jsr         vc_args
            ldx         vc_lv
            lda         L_tail,x
            beq         :+
            lda         #VO_TCALL
            bra         :++
:
            lda         #VO_CALL
:
            jsr         vc_byte
            ldx         vc_lv
            lda         L_m,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_h0,x
            jsr         vc_byte
            jsr         vc_rbyte
            ldx         vc_lv
            lda         L_h0,x
            sta         vc_h
            lda         L_t,x
            jsr         vc_def
            jmp         vc_je

; A built-in's call (L_b): its arguments pushed (taken as errors too, if it takes them), BCALL
vc_bcall:
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_t,x
            ldy         L_b,x
            lda         vb_flags,y
            and         #BF_ERRORS
            beq         :+
            lda         #M_OK
            sta         L_am,x
            bra         :++
:
            lda         L_t,x
            jsr         vc_amode
:
            jsr         vc_args
            lda         #VO_BCALL
            jsr         vc_byte
            ldx         vc_lv
            lda         L_b,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_m,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_h0,x
            jsr         vc_byte
            jsr         vc_rbyte
            ldx         vc_lv
            lda         L_h0,x
            sta         vc_h
            lda         L_t,x
            jsr         vc_def
            jmp         vc_je

; A quick op's call (L_b the op): (op x), (op x y): x pushed, y in ex; or y a fixnum: the op's C variant
vc_fast:
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_t,x
            jsr         vc_amode
            jsr         vc_amset
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            ldx         vc_lv
            lda         L_m,x
            cmp         #2
            bne         @op
            lda         L_alo,x                             ; (y a fixnum?)
            pha
            lda         L_ahi,x
            tax
            pla
            jsr         vc_split
            sta         vc_v
            stx         vc_v + 1
            lsr
            bcs         @const
            jsr         vc_push
            jsr         vc_amset
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            dec         vc_h
@op:
            ldx         vc_lv
            lda         L_b,x
            jsr         vc_rop
            bra         @end
@const:
            ldx         vc_lv
            lda         L_b,x
            clc
            adc         #VO_ADDC - VO_ADD
            jsr         vc_rop
            lda         vc_v
            ldx         vc_v + 1
            jsr         vc_word
@end:
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_def
            jmp         vc_je

; The evaluator's (EVAL): the expression evaluated in this frame's scope
vc_eval:
            lda         #VO_EVAL
            jsr         vc_byte
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            lda         vc_h
            jsr         vc_byte
            jsr         vc_rbyte
            jmp         vc_je

; (if c a [b])
vc_if:
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab                              ; (L_t: else; L_l2: the end)
            ldx         vc_lv
            sta         L_t,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_l2,x
            jsr         vc_amode                            ; (c: its error, the if's value)
            jsr         vc_amset
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            lda         #VO_JF
            jsr         vc_byte
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_ref
            jsr         vc_same                             ; (a: the if's way)
            jsr         vc_nextt
            ldx         vc_lv
            lda         L_tail,x
            bne         :+
            lda         #VO_JMP
            jsr         vc_byte
            ldx         vc_lv
            lda         L_l2,x
            jsr         vc_ref
:
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_def
            ldx         vc_lv
            lda         L_m,x
            cmp         #3
            bne         @nil
            jsr         vc_same
            jsr         vc_nextt
            bra         @end
@nil:
            lda         #VO_CONST
            jsr         vc_byte
            lda         #0
            tax
            jsr         vc_word
@end:
            ldx         vc_lv
            lda         L_l2,x
            jmp         vc_def

; (do e...): each in turn (an error the do's value), the last's value
vc_do:
            ldx         vc_lv
            lda         L_m,x
            bne         :+
            jmp         vc_nil
:
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_l2,x
            jsr         vc_amode
@next:
            ldx         vc_lv
            lda         L_m,x
            cmp         #1
            beq         @last
            dec         L_m,x
            jsr         vc_amset
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            bra         @next
@last:
            jsr         vc_same
            jsr         vc_nextt
            ldx         vc_lv
            lda         L_l2,x
            jmp         vc_def

; (and e...): the first NIL, or the last's value (none: T); (or e...): the first that isn't NIL, or the last's (none:
; NIL)
vc_and:
            lda         #VO_JF
            ldy         #<T_VAL
            bra         vc_andor
vc_or:
            lda         #VO_JT
            ldy         #<NIL
vc_andor:
            ldx         vc_lv
            sta         L_b,x
            lda         L_m,x
            bne         :+
            tya
            ldx         #0
            jmp         vc_const
:
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_l2,x
            jsr         vc_amode
@next:
            ldx         vc_lv
            lda         L_m,x
            cmp         #1
            beq         @last
            dec         L_m,x
            jsr         vc_amset
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            ldx         vc_lv
            lda         L_b,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_l2,x
            jsr         vc_ref
            bra         @next
@last:
            jsr         vc_same
            jsr         vc_nextt
            ldx         vc_lv
            lda         L_l2,x
            jmp         vc_def

; CONST NIL (vc_nil), CONST .A, .X (vc_const)
vc_nil:
            lda         #0
            tax
vc_const:
            pha
            lda         #VO_CONST
            jsr         vc_byte
            pla
            jmp         vc_word

.segment "RODATA7"

; The ops' code, by number (VM_R set: the same)
.macro VM_OPS
            .word       op_ret, op_const, op_local, op_global, op_lookup, op_push, op_jf, op_jt
            .word       op_jmp, op_je, op_head, op_call, op_tcall, op_bcall, op_eval, op_quote
            .word       op_quote0, op_add, op_sub, op_lt, op_gt, op_le, op_ge, op_neq
            .word       op_inc, op_dec, op_zerop, op_onep, op_addc, op_subc, op_ltc, op_gtc
            .word       op_lec, op_gec, op_eqc
            .res        128 - VO_COUNT, $FF
.endmacro
.assert     VO_COUNT = 35 * 2, error, "vm.inc: vm_ops has an op too few or too many"
vm_ops:
            VM_OPS
            VM_OPS

; The quick ops' built-ins and their values counted (VO_ADD's order: (op - VO_ADD) / 2)
vo_bin:     .byte       BIN_ADD, BIN_SUB, BIN_LT, BIN_GT, BIN_LE, BIN_GE, BIN_EQ, BIN_INC, BIN_DEC, BIN_ZEROP
            .byte       BIN_ONEP, BIN_ADD, BIN_SUB, BIN_LT, BIN_GT, BIN_LE, BIN_GE, BIN_EQ
vo_n:       .byte       2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2

; The compiler's: the built-ins that are quick ops, the values they're given, the op
QUICKS          = 11
vq_bin:     .byte       BIN_ADD, BIN_SUB, BIN_LT, BIN_GT, BIN_LE, BIN_GE, BIN_EQ, BIN_INC, BIN_DEC, BIN_ZEROP
            .byte       BIN_ONEP
vq_n:       .byte       2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1
vq_op:      .byte       VO_ADD, VO_SUB, VO_LT, VO_GT, VO_LE, VO_GE, VO_NEQ, VO_INC, VO_DEC, VO_ZEROP, VO_ONEP

; The built-ins' table again (builtins.inc's), for this bank: each one's least and most arguments, flags, code; and
; whether it looks at its caller's scope (vb_vf: VF_SCOPE)
vb_min:
.repeat BI_COUNT, I
            .byte       .ident(.sprintf("bi_min_%d", I))
.endrep
vb_max:
.repeat BI_COUNT, I
            .byte       .ident(.sprintf("bi_max_%d", I))
.endrep
vb_flags:
.repeat BI_COUNT, I
            .byte       .ident(.sprintf("bi_fl_%d", I))
.endrep
vb_lo:
.repeat BI_COUNT, I
            .byte       <.ident(.sprintf("bi_cd_%d", I))
.endrep
vb_hi:
.repeat BI_COUNT, I
            .byte       >.ident(.sprintf("bi_cd_%d", I))
.endrep
vb_vf:
.repeat BI_COUNT, I
.if I = BIN_LIST .or I = BIN_FN .or I = BIN_FUN .or I = BIN_FEXPR .or I = BIN_HASH_CREATE .or I = BIN_HASH_PUT .or I = BIN_TO_HASH
            .byte       VF_SCOPE
.else
            .byte       0
.endif
.endrep
