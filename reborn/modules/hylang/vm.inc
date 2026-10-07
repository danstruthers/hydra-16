; ****************************************************************************
; vm.inc - hylang's bytecode machine (docs/hylang.md, "The bytecode machine"), in its seventh bank: a function's body
; compiled at its second call (vm_compile) to the code of a small machine, which runs it with its arguments where
; they're pushed, in the evaluation stack, not bound in a scope on the heap.  The evaluator's own way is the rest:
; what the compiler doesn't do (most special forms, an fexpr's call) is handed to it, and it calls the functions
; that aren't compiled; a value comes back through a K_VM frame.
;   A function's code is in the arena, RAM banks of its own (up to eight), never moved; its place is the function's
; word 4 (a fixnum: the bank's index, bits 15-13, and the address in the window, bits 12-1: a code's place is even),
; till something the code counts on changes (a built-in's name bound again: vm_rebind), when every function's is
; dropped (vm_flush) and each is compiled again at its next call.  A full arena compiles nothing more till the
; evaluator's next start (ev_run: vm_reset), or a script's next item (k_load's, nothing under its load).  Its word 5
; counts its calls till it's compiled (fun's counts one: compiled at its next if not now); FN_NOVM (word 3) says it
; can't be.
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
VO_HEAD         = $14           ; m c t: ex, the function of call c (m arguments): a function, an ordinary built-in,
                                ;   a partially applied one, a buffer given an index, pushed; an error, or not a
                                ;   function and c has no arguments: to t (past the call); else c (from its
                                ;   arguments) the evaluator's, its value for t
VO_CALL         = $16           ; m ret fn ep code bank idx h r: the function under the m arguments on top called (its
                                ;   return ret); fn, the one it called last, at epoch ep (vm_ep: its cache), its code
                                ;   at code (in bank, idx)
VO_TCALL        = $18           ; m h 0: the same, in tail position
VO_BCALL        = $1A           ; b m h r: built-in b called, its m arguments on top
VO_EXT          = $1C           ; x ...: the rare op x after it, from the second table (vm_xops: VX_), as if here
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
VO_LPUSH        = $46           ; y: LOCAL y, PUSH
VO_CPUSH        = $48           ; v: CONST v, PUSH
VO_SHEAD        = $4A           ; m c t sym ep v: HEAD of sym's global value; v, its value at epoch ep (vm_ep: its
                                ;   (m's bit 7: a tail call of the function by its own name whose arguments call
                                ;   nothing, vc_shflag's: its cache only this frame's function; vm_shf)
                                ;   cache), pushed at once
VO_CSELF        = $4C           ; m code ret h r: CALL of this frame's function (else CALL's way), its code at code
VO_TSELF        = $4E           ; m code 0: TCALL, the same
VO_LOCALS       = $50           ; kt kn tbl names: the frame's scope's slot (NIL: none made), then kt slots, holes
                                ;   (UNBOUND) till they're set: the function's kn locals (names theirs), its blocks'
                                ;   slots; tbl its blocks' table (0: none); the record's scope word this op's place
VO_LOCALH       = $52           ; y sym: a local's value (a hole: sym's past this frame)
VO_SETL         = $54           ; y: the argument or local at y = ex; ex = NIL
VO_SETBL        = $56           ; y sym: set! of a local (a hole: SETBG's)
VO_LQ           = $58           ; s y c: ex = quick op s (VO_ADD ... VO_EQC) on the argument at y
                                ;   (and the fixnum c): LOCAL y, then s (the compiler's, joined)
VO_JLQ          = $5A           ; s y c t: LQ's, then JF's to t
VO_STT          = $5C           ; the word on top = ex
VO_POPX         = $5E           ; ex = the word popped
VO_LOOP         = $60           ; t k e: to t (Ctrl-C: the error interrupted; k words dropped, then to e)
VO_BLOCK        = $62           ; fb k vb parent names: a block's scope begun (vm_blocks, vm_mkblk)
VO_LOCALB       = $64           ; y fb p sym: a block's variable's value (a hole: sym's past it)
VO_SETLB        = $66           ; y fb p: a block's variable = ex; ex = NIL
VO_SETBLB       = $68           ; y fb p sym: set! of a block's variable (a hole: SETBG's)
VO_DOTN         = $6A           ; dotimes' count, ex: pushed, and 0 (else the error)
VO_DOTI         = $6C           ; t: [n i]: i < n, ex = i; else to t
VO_DOTINC       = $6E           ; i one more
VO_EACHL        = $70           ; each's list, ex: its items pushed (a string's characters) (else the error)
VO_EACHI        = $72           ; t: the items: ex = the next; none, to t
VO_DROP         = $74           ; k: k words dropped; ex = NIL
VO_TRYE         = $76           ; t: ex not an error: to t
VO_JQ           = $78           ; s c t: quick op s (VO_INC ... VO_EQC) on ex (and c), then JF's to t
VO_LL           = $7A           ; s y1 y2: ex = quick op s (VO_ADD ... VO_NEQ; bit 0: pushed) on the arguments at y1
                                ;   and y2: LPUSH y1, LOCAL y2, s
VO_JLL          = $7C           ; s y1 y2 t: LL's, then JF's to t
VO_LQP          = $7E           ; s y c: LQ's, its value pushed (LQ, then PUSH)
VO_COUNT        = $80

; The rare ops (EXT's: after it, in the same form; VM_R the same)
VX_EVAL         = $00           ; e h r: e, a call, the evaluator's (ev_call), in this frame's scope
VX_SETBG        = $02           ; sym: set! of sym where it's bound nearest, else its global value
VX_DEFG         = $04           ; sym: sym's global value = ex; ex = NIL
VX_TRYV         = $06           ; ye yc: ex an error: &err's slot its message, &code's its code; the message pushed
VX_TRYH         = $08           ; t: ex a function: on; else the message dropped, to t
VX_TRYF         = $0A           ; the function (ex) pushed under the message: a call, next
VX_COUNT        = $0C
VM_R            = $80           ; (An op's error returned)

VF_SCOPE        = $01           ; (A built-in that looks at its caller's scope: vb_vf's)

VC_LV           = 16            ; The compiler's: its levels (an expression's nesting: 8 bytes of the 65C02's stack) ...
VC_LABS         = 80            ;   labels in a function's code ...
VC_HMAX         = 120           ;   its frame's words (h is under 128) ...
VC_FORMALS      = 15            ;   its formals (a scope's frame holds them all in its first cell) ...
VC_ARGS         = 100           ;   a call's arguments ...
VC_BLK          = 8             ;   blocks one in another (let, each, dotimes, try's handler) ...
VC_BTBL         = 24            ;   blocks in a function ...
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
ar_bank:    .res        8                                   ; The arena's banks (taken as they're wanted) ...
ar_n:       .res        1                                   ;   how many ...
ar_cur:     .res        1                                   ;   the one being filled ...
ar_top:     .res        2                                   ;   its next byte ...
vm_full:    .res        1                                   ;   <> 0: full (nothing compiled till ev_run's vm_reset)
vm_lk:      .res        1                                   ; (vm_scope's: the locals, their names' place ...
vm_ln:      .res        2
vm_opa:     .res        2                                   ;   the op whose scope it is ...
vm_bd:      .res        2                                   ;   a block's BLOCK op, its frame slot, variables, their
vm_bf:      .res        1                                   ;   slots, one, the blocks made)
vm_bk:      .res        1
vm_bv:      .res        1
vm_bj:      .res        1
vm_bc:      .res        1
vm_ep:      .res        2                                   ; The globals' epoch: one more each time one's bound again
                                                            ;   (or a name's first bound in a frame): SHEAD's cache
vm_shc:     .res        1                                   ; ($80: SHEAD's cache filled as its value's pushed)
vm_sb:      .res        1                                   ; The compiler's scratch banks: its bytecode, the translator's
vm_mb:      .res        1                                   ;   map (vmx.inc; 0: not taken yet)
vm_r:       .res        2                                   ; A call's return (its place, compiled: CALL's, CSELF's)
vm_ccf:     .res        1                                   ; ($80: CALL's cache filled as its function's code is run ...
vm_cb:      .res        1                                   ;   from its code's bank)
vm_qs:      .res        1                                   ; (A fused op's quick op)
vm_sjv:     .res        2                                   ; (vm_sj's: a stub's op's code)
vm_wop:     .res        1                                   ; (A walk's: its W_ ...
vm_wf:      .res        2                                   ;   its function, its items, its value so far, an item)
vm_wl:      .res        2
vm_wa:      .res        2
vm_wi:      .res        2
vm_wc:      .res        2
vm_shf:     .res        1                                   ; ($80: a flagged SHEAD's head pushed, the machine's way;
                                                            ;   0: not, native code's shself: tselfh's)
vm_msg:     .res        1                                   ; (vm_fmul's: the product's sign ...
vm_m0:      .res        2                                   ;   the multiplicand ...
vm_mm:      .res        1                                   ;   the multiplier (under 256) ...
vm_p0:      .res        2                                   ;   the product ...
vm_q:       .res        2                                   ;   vm_mul8's)

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
vc_lop          = vc_sd + 1                                 ;   (a LOCAL's or a CONST's place, and past it: vc_push's)
vc_lend         = vc_lop + 2
L_q             = vc_lend + 2                               ; Each level: <> 0, a Q-expression's code (while's) ...
L_nlo           = L_q + VC_LV                               ;   set's names left
L_nhi           = L_nlo + VC_LV
vc_ls_lo        = L_nhi + VC_LV                             ; The function's locals: their names ...
vc_ls_hi        = vc_ls_lo + VC_FORMALS
vc_k            = vc_ls_hi + VC_FORMALS                     ;   how many
vc_isl          = vc_k + 1                                  ;   (vc_formal's: <> 0, a local)
vc_cq           = vc_isl + 1                                ;   (vc_child's: L_q)
L_x             = vc_cq + 1                                 ; Each level: vc_body's, its last its own way ...
L_l3            = L_x + VC_LV                               ;   a label more (a loop's end)
vb_fb           = L_l3 + VC_LV                              ; Each block open: its frame slot ...
vb_k            = vb_fb + VC_BLK                            ;   its variables, their slots ...
vb_vb           = vb_k + VC_BLK
vb_dlo          = vb_vb + VC_BLK                            ;   its BLOCK op ...
vb_dhi          = vb_dlo + VC_BLK
vb_ti           = vb_dhi + VC_BLK                           ;   its table entry ...
vb_nlo          = vb_ti + VC_BLK                            ;   its variables' names (16 each)
vb_nhi          = vb_nlo + VC_BLK * 16
vt_slo          = vb_nhi + VC_BLK * 16                      ; The blocks' table: each one's start, end ...
vt_shi          = vt_slo + VC_BTBL
vt_elo          = vt_shi + VC_BTBL
vt_ehi          = vt_elo + VC_BTBL
vc_cnl          = vt_ehi + VC_BTBL                          ; A block's names, collected (vc_cadd) ...
vc_cnh          = vc_cnl + VC_FORMALS
vc_cn           = vc_cnh + VC_FORMALS                       ;   how many
vc_tgt          = vc_cn + 1                                 ; (vc_addl's: <> 0, vc_cadd's)
vc_bfb          = vc_tgt + 1                                ; (vc_formal's: a block's variable's frame slot, pair)
vc_bp           = vc_bfb + 1
vc_bi           = vc_bp + 1                                 ; (Scratch)
vc_nblk         = vc_bi + 1                                 ; Blocks open ...
vc_ntbl         = vc_nblk + 1                               ;   in the table ...
vc_bused        = vc_ntbl + 1                               ;   their slots used, at most, kept (the frame's) ...
vc_bneed        = vc_bused + 1
vc_btot         = vc_bneed + 1
vc_lop2         = vc_btot + 1                               ; (The op before vc_lop's, if it ends there: vc_l2ok)
vc_l2ok         = vc_lop2 + 2
vc_qo           = vc_l2ok + 1                               ; (Ops joined: vc_push's, vc_qop's, vc_qcop's, vc_jf's)
vc_qy           = vc_qo + 1
vc_qy2          = vc_qy + 1
vc_qt           = vc_qy2 + 1
vc_qr           = vc_qt + 1
vc_qc           = vc_qr + 1
L_st            = vc_qc + 2                                 ; Each level: <> 0, a statement of the function's body
                                                            ;   (its do's, in order, errors returned) ...
vc_cst          = L_st + VC_LV                              ;   (vc_child's: L_st)
vc_da           = vc_cst + 1                                ; The locals set by such a statement so far (a bit each)
L_shl           = vc_da + 2                                 ; Each level: its SHEAD's place (vc_shflag's)
L_shh           = L_shl + VC_LV
vc_wend         = L_shh + VC_LV
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
            jsr         vm_epnext
            ldy         #6
            lda         (hp),y
            and         #SF_INLINED
            beq         @no
            FARN        7, vm_flush
@no:
            rts

; The globals' epoch moved on (a global bound again, a name bound in a frame for the first time, a collection: the
; machine's caches of global values and functions dropped).  Run out ($FFFF), it stays: no cache is filled, and the
; arena's emptied at the next line (vm_full), the epoch 1 again.  Keeps .X, .Y
vm_epnext:
            inc         vm_ep
            bne         @rts
            inc         vm_ep + 1
            bne         @rts
            dec         vm_ep
            dec         vm_ep + 1
            lda         #1
            sta         vm_full
@rts:
            rts

.segment "CODE7"

; ---- The arena

; The machine made, no code yet: as hylang starts (a snapshot's functions may have code from its build, not here)
vm_init:
            stz         vm_on
            stz         vm_sb
            stz         ar_n
            lda         #$FF                                ; (No code's bank yet)
            sta         vm_idx
; The arena emptied, every function's code dropped (ev_run's, when it's full: no frame of the machine's is left)
vm_reset:
            jsr         vm_flush
            lda         #1                                  ; (The epoch: none of the code's caches yet)
            sta         vm_ep
            stz         vm_ep + 1
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

; .A, .X pushed (push's, quicker: .Y not kept)
.macro VPUSH
            sta         (sp)
            ldy         #1
            txa
            sta         (sp),y
            lda         sp
            clc
            adc         #2
            sta         sp
            bne         *+5                                 ; (Past the jsr)
            jsr         vm_page
.endmacro

; IN: .A, .X = a code's place (a fixnum: vm_encode's: its index in the arena times 32, the bits 12-1 of its
; address, which is even, and bit 0 set).  vm_ip, vm_bank, vm_idx = it
vm_decode:
            and         #$FE
            sta         vm_ip
            txa
            and         #$1F
            ora         #$80
            sta         vm_ip + 1
            txa
            and         #$E0
            cmp         vm_idx                              ; (Its bank: the same, as a rule)
            bne         vm_newidx
            rts

; .A = a code's index in the arena (times 32), not vm_idx's: vm_idx, vm_bank its
vm_newidx:
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

; IN: .A, .X = an even address in the window, in the arena's bank vm_idx.  OUT: .A, .X = its place (a fixnum)
vm_encode:
            ora         #1
            pha
            txa
            and         #$1F
            ora         vm_idx
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
            jmp         (vm_ip)

; The next op, the code's bank in the window still (an op that didn't leave it): ip past this one first (vm_nN: N
; bytes; vm_na: .A bytes)
vm_na:
            clc
            adc         vm_ip
            sta         vm_ip
            bcc         vm_nextf
            inc         vm_ip + 1
            bra         vm_nextf
vm_n6:
            lda         vm_ip
            clc
            adc         #6
            sta         vm_ip
            bcc         vm_nextf
            inc         vm_ip + 1
            bra         vm_nextf
vm_n5:
            lda         vm_ip
            clc
            adc         #5
            sta         vm_ip
            bcc         vm_nextf
            inc         vm_ip + 1
            bra         vm_nextf
vm_n4:
            lda         vm_ip
            clc
            adc         #4
            sta         vm_ip
            bcc         vm_nextf
            inc         vm_ip + 1
            bra         vm_nextf
vm_n3:
            lda         vm_ip
            clc
            adc         #3
            sta         vm_ip
            bcc         vm_nextf
            inc         vm_ip + 1
            bra         vm_nextf
vm_n2:
            lda         vm_ip
            clc
            adc         #2
            sta         vm_ip
            bcc         vm_nextf
            inc         vm_ip + 1
            bra         vm_nextf
vm_n1:
            inc         vm_ip
            bne         vm_nextf
            inc         vm_ip + 1
vm_nextf:
            jmp         (vm_ip)

; ip = the address at ip + .Y (an op's target)
vm_jumpy:
            lda         (vm_ip),y
            tax
            iny
            lda         (vm_ip),y
            sta         vm_ip + 1
            stx         vm_ip
            jmp         vm_nextf

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

; ---- The walks: map, filter, foldl, any?, all?, find, count, sum and product (forms.inc's, W_*), here for BCALL

; The walk's frame, on the stack where the BCALL's arguments were: its words' places
WF_F            = 0             ; The function (sum's and product's: + and *, as foldl's)
WF_ITEMS        = 2             ; The items left (conses)
WF_ACC          = 4             ; The value so far (map's and filter's list: its first cons)
WF_LAST         = 6             ;   (map's and filter's: its last cons)
WF_ITEM         = 8             ; The item given
WF_OP           = 10            ; The walk (W_*, a fixnum)
WF_CODE         = 12            ; The code past the BCALL (its place)
WF_RB           = 14            ; Its frame's vm_rb (bit 0 set: a fixnum's), vm_mat (a tail BCALL's r is 0)
WF_FC           = 16            ; The function's code's place, as it's known (0: not yet; 1: none, vm_call's way)
WF_S            = 18            ; Its frame's vm_s: bytes under the walk's (bit 0 set), as a spill moves both
WF_SIZE         = 20
VM_WRET         = 3             ; The arena's trampoline's place (vmx.inc's: at its first bank's start, after h 0
                                ;   and r 2, jmp vm_wret), each call's return

; BCALL of a walk (.X its built-in, vm_m its arguments on top, vm_ip past it): its frame made of them (a list that
; isn't a Q-expression's: the evaluator's way, its error), then its items
vm_walk:
            txa
            sec
            sbc         #BIN_MAP
            sta         vm_wop
            lda         vm_m                                ; (ht: the arguments)
            asl
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         vm_k                                ; (Its list: the last)
            dey
            lda         (ht),y
            tax
            dey
            lda         (ht),y
            jsr         q_conses
            bcc         :+
            ldx         bi_n
            jmp         vm_bout
:
            sta         vm_wl
            stx         vm_wl + 1
            stz         vm_wa                               ; (The value so far: none)
            stz         vm_wa + 1
            lda         vm_wop
            cmp         #W_SUM
            bcc         @fn
            ldx         #<FIX(0)                            ; (sum: + from 0; product: * from 1, foldl's)
            ldy         #<(BUILTIN0 + 2 * BIN_ADD)
            cmp         #W_SUM
            beq         :+
            ldx         #<FIX(1)
            ldy         #<(BUILTIN0 + 2 * BIN_MUL)
:
            stx         vm_wa
            sty         vm_wf
            lda         #>(BUILTIN0 + 2 * BIN_ADD)
            sta         vm_wf + 1
            lda         #W_FOLDL
            sta         vm_wop
            bra         @frame
@fn:
            lda         (ht)                                ; (Its function)
            sta         vm_wf
            ldy         #1
            lda         (ht),y
            sta         vm_wf + 1
            ldx         vm_wop
            cpx         #W_COUNT
            bne         :+
            lda         #<FIX(0)
            sta         vm_wa
            bra         @frame
:
            cpx         #W_FOLDL
            bne         @frame
            iny                                             ; (foldl's: from its value)
            lda         (ht),y
            sta         vm_wa
            iny
            lda         (ht),y
            sta         vm_wa + 1
@frame:
            jsr         vm_wfok                             ; (A function the machine calls as the evaluator would)
            bcc         :+
            ldx         bi_n
            jmp         vm_bout
:
            lda         ht                                  ; (Its frame's vm_s, under the walk's)
            sec
            sbc         vm_s
            ora         #1
            sta         vm_wc
            lda         ht + 1
            sbc         vm_s + 1
            sta         vm_wc + 1
            lda         ht                                  ; (The arguments dropped, the frame pushed)
            sta         sp
            ldx         ht + 1
            stx         sp + 1
            cpx         stk_basep
            bne         :+
            lda         spilled
            beq         :+
            jsr         unspill
:
            lda         vm_wf
            ldx         vm_wf + 1
            VPUSH
            lda         vm_wl
            ldx         vm_wl + 1
            VPUSH
            lda         vm_wa
            ldx         vm_wa + 1
            VPUSH
            lda         #0
            tax
            VPUSH
            VPUSH
            lda         vm_wop
            asl
            ora         #1
            ldx         #0
            VPUSH
            lda         vm_ip
            ldx         vm_ip + 1
            jsr         vm_encode
            VPUSH
            lda         vm_rb
            ora         #1
            ldx         vm_mat
            VPUSH
            lda         #0
            tax
            VPUSH
            lda         vm_wc
            ldx         vm_wc + 1
            VPUSH
; The walk's frame on top: its next item given to its function (its return the trampoline's, vm_wret), or none
; left.  foldl's + and * of fixnums here; a compiled function's frame made at once (its code's place known: WF_FC);
; else the machine's call (vm_call)
vm_wnext:
            jsr         vm_wat
            ldy         #WF_ITEMS + 1
            lda         (ht),y
            tax
            dey
            lda         (ht),y
            bne         :+
            cpx         #0
            bne         :+
            jmp         vm_wend
:
            jsr         deref                               ; (The item; the items after it)
            ldy         #3
            lda         (hp),y
            pha
            dey
            lda         (hp),y
            pha
            dey
            lda         (hp),y
            sta         vm_wi + 1
            lda         (hp)
            sta         vm_wi
            ldy         #WF_ITEMS
            pla
            sta         (ht),y
            iny
            pla
            sta         (ht),y
            ldy         #WF_ITEM
            lda         vm_wi
            sta         (ht),y
            iny
            lda         vm_wi + 1
            sta         (ht),y
            ldy         #WF_F                               ; (Its function, the value so far, its code, as the
            lda         (ht),y                              ;   pushes may move the stack)
            sta         vm_wf
            iny
            lda         (ht),y
            sta         vm_wf + 1
            ldy         #WF_ACC
            lda         (ht),y
            sta         vm_wa
            iny
            lda         (ht),y
            sta         vm_wa + 1
            ldy         #WF_FC
            lda         (ht),y
            sta         vm_wc
            iny
            lda         (ht),y
            sta         vm_wc + 1
            ldy         #WF_OP
            lda         (ht),y
            lsr
            sta         vm_wop
            cmp         #W_FOLDL
            beq         :+
            jmp         @one
:
            lda         vm_wf + 1                           ; (foldl's + or * of two fixnums: here)
            cmp         #>(BUILTIN0 + 2 * BIN_ADD)
            bne         @two
            lda         vm_wf
            cmp         #<(BUILTIN0 + 2 * BIN_ADD)
            beq         @add
            cmp         #<(BUILTIN0 + 2 * BIN_MUL)
            bne         @two
            MOVW        vm_a, vm_wa
            MOVW        vm_b, vm_wi
            jsr         vm_fmul
            bcs         @two
            bra         @acc
@add:
            lda         vm_wa
            and         vm_wi
            lsr
            bcc         @two
            lda         vm_wi
            and         #$FE
            clc
            adc         vm_wa
            sta         ex
            lda         vm_wi + 1
            adc         vm_wa + 1
            bvs         @two
            sta         ex + 1
@acc:
            ldy         #WF_ACC
            lda         ex
            sta         (ht),y
            iny
            lda         ex + 1
            sta         (ht),y
            jmp         vm_wnext
@two:
            jsr         vm_wframe
            lda         vm_wf                               ; (foldl's: the function, the value so far, the item)
            ldx         vm_wf + 1
            VPUSH
            lda         vm_wa
            ldx         vm_wa + 1
            VPUSH
            lda         vm_wi
            ldx         vm_wi + 1
            VPUSH
            lda         #2
            jmp         @call
@one:
            lda         vm_wc                               ; (Its code known, no Ctrl-C nor a note, not too deep, the
            beq         @one2                               ;   stack's page room for 8 bytes: the function, the item
            cmp         #1                                  ;   and its frame's record in one store)
            bne         :+
            lda         vm_wc + 1
            beq         @one2
:
            bit         intr
            bmi         @one2
            bvs         @one2
            lda         depth + 1
            cmp         #>MAX_DEPTH
            bcc         :+
            bne         @one2
            lda         depth
            cmp         #<MAX_DEPTH
            bcs         @one2
:
            lda         sp
            cmp         #$F8
            bcs         @one2
            lda         vm_wf
            sta         (sp)
            ldy         #1
            lda         vm_wf + 1
            sta         (sp),y
            iny
            lda         vm_wi
            sta         (sp),y
            iny
            lda         vm_wi + 1
            sta         (sp),y
            iny
            lda         #<VM_WRET
            sta         (sp),y
            iny
            lda         #>VM_WRET
            sta         (sp),y
            iny
            lda         #0
            sta         (sp),y
            iny
            sta         (sp),y
            lda         sp
            sta         vm_s
            clc
            adc         #8
            sta         sp
            lda         sp + 1
            sta         vm_s + 1
            lda         #4
            sta         vm_rb
            stz         vm_mat
            inc         depth
            bne         :+
            inc         depth + 1
:
            lda         vm_wc
            ldx         vm_wc + 1
            jsr         vm_decode
            jmp         vm_next
@one2:
            jsr         vm_wframe
            lda         vm_wf
            ldx         vm_wf + 1
            VPUSH
            lda         vm_wi
            ldx         vm_wi + 1
            VPUSH
            lda         #1
@call:
            sta         vm_m
            lda         vm_wc                               ; (Its code's place: not known yet, learnt; 0, not compiled
            bne         :+                                  ;   yet, and 1, none: vm_call's way)
            jsr         vm_wlearn
            lda         vm_wc
            beq         @gen
:
            cmp         #1
            bne         @fast
            lda         vm_wc + 1
            beq         @gen
@fast:
            bit         intr                                ; (Ctrl-C, a note, too deep: vm_call's way)
            bmi         @gen
            bvs         @gen
            lda         depth + 1
            cmp         #>MAX_DEPTH
            bcc         :+
            bne         @gen
            lda         depth
            cmp         #<MAX_DEPTH
            bcs         @gen
:
            lda         #<VM_WRET
            sta         vm_t
            lda         #>VM_WRET
            sta         vm_t + 1
            jsr         vm_frame0
            lda         vm_wc
            ldx         vm_wc + 1
            jsr         vm_decode
            jmp         vm_next
@gen:
            stz         vm_tl
            stz         vm_ccf
            lda         #<VM_WRET                           ; (Its return: the trampoline, in the arena's first bank)
            sta         vm_r
            lda         #>VM_WRET
            sta         vm_r + 1
            lda         #2
            sta         vm_ip
            lda         #$80
            sta         vm_ip + 1
            stz         vm_idx
            lda         ar_bank
            sta         vm_bank
            jmp         vm_call

; The walk's function's code's place, vm_wc, learnt (and kept in its frame, WF_FC, under the function and its vm_m
; arguments on top): a function's cell, compiled, of as many formals, not an fexpr nor one that takes extras; 1 if
; it isn't one (vm_call's way each time); 0 if it isn't compiled yet (vm_call's way, which may compile it)
vm_wlearn:
            lda         #1
            sta         vm_wc
            stz         vm_wc + 1
            lda         vm_wf
            lsr
            bcs         @keep
            ldx         vm_wf + 1
            cpx         #IMM_PAGES
            bcc         @keep
            lda         pk,x
            cmp         #PK_FUNC
            bne         @keep
            lda         vm_wf
            ldy         #3
            jsr         cell_get                            ; (Its flags, its formals)
            and         #FN_FEXPR | FN_EXTRAS | FN_NOVM
            bne         @keep
            cpx         vm_m
            bne         @keep
            lda         vm_wf
            ldx         vm_wf + 1
            ldy         #4
            jsr         cell_get                            ; (Its code)
            sta         vm_wc
            stx         vm_wc + 1
            ora         vm_wc + 1
            bne         @keep
            rts                                             ; (Not yet: 0, not kept)
@keep:
            lda         vm_m                                ; (Its frame: under the function and its arguments)
            asl
            clc
            adc         #2 + WF_SIZE
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #WF_FC
            lda         vm_wc
            sta         (ht),y
            iny
            lda         vm_wc + 1
            sta         (ht),y
            rts

; The function's value, ex, for the walk on top (the trampoline's, its call returned): an error, the walk's value
vm_wret:
            jsr         vm_iserr
            bcc         :+
            jmp         vm_wdone
:
            ldy         #0                                  ; (vm_k = 1 if it isn't NIL)
            lda         ex
            ora         ex + 1
            beq         :+
            iny
:
            sty         vm_k
            jsr         vm_wat
            ldy         #WF_OP
            lda         (ht),y
            lsr
            cmp         #W_FOLDL
            bcs         @fold
            cmp         #W_FILTER
            bne         @add                                ; (map's: the value put in its list)
            lda         vm_k                                ; (filter's: the item, if the value isn't NIL)
            bne         :+
            jmp         vm_wnext
:
            ldy         #WF_ITEM
            lda         (ht),y
            sta         ex
            iny
            lda         (ht),y
            sta         ex + 1
@add:
            MOVW        et, ex                              ; (In a cons, at the list's end)
            stz         eu
            stz         eu + 1
            lda         #PK_QCONS
            jsr         make2
            bcc         :+
            MOVW        ex, nomem_err
            jmp         vm_wdone
:
            MOVW        ex, hv
            jsr         vm_wat
            ldy         #WF_LAST
            lda         (ht),y
            sta         hv
            iny
            lda         (ht),y
            sta         hv + 1
            ora         hv
            bne         @link
            ldy         #WF_ACC                             ; (The first)
            lda         ex
            sta         (ht),y
            iny
            lda         ex + 1
            sta         (ht),y
            bra         @last
@link:
            lda         ex
            ldx         ex + 1
            ldy         #1
            jsr         cell_set
            jsr         vm_wat
@last:
            ldy         #WF_LAST
            lda         ex
            sta         (ht),y
            iny
            lda         ex + 1
            sta         (ht),y
            jmp         vm_wnext
@fold:
            cmp         #W_ANY
            bcs         @test
            ldy         #WF_ACC                             ; (foldl's: the value so far)
            lda         ex
            sta         (ht),y
            iny
            lda         ex + 1
            sta         (ht),y
            jmp         vm_wnext
@test:
            tax
            ldy         vm_k
            cpx         #W_ALL
            bne         :+
            tya                                             ; (all?: NIL, at the first NIL)
            beq         @done
            jmp         vm_wnext
:
            tya
            bne         :+
            jmp         vm_wnext
:
            cpx         #W_COUNT
            bne         :+
            ldy         #WF_ACC                             ; (count: one more)
            lda         (ht),y
            clc
            adc         #2
            sta         (ht),y
            iny
            lda         (ht),y
            adc         #0
            sta         (ht),y
            jmp         vm_wnext
:
            cpx         #W_FIND
            bne         :+
            ldy         #WF_ITEM                            ; (find: the item)
            lda         (ht),y
            sta         ex
            iny
            lda         (ht),y
            sta         ex + 1
            jmp         vm_wdone
:
            lda         #<T_VAL                             ; (any?: T)
            sta         ex
            stz         ex + 1
            jmp         vm_wdone
@done:
            stz         ex
            stz         ex + 1
            jmp         vm_wdone

; No items left: the walk's value
vm_wend:
            ldy         #WF_OP
            lda         (ht),y
            lsr
            tax
            ldy         #WF_ACC                             ; (map's, filter's, foldl's, count's: the value so far)
            lda         (ht),y
            sta         ex
            iny
            lda         (ht),y
            sta         ex + 1
            cpx         #W_ANY
            bcc         vm_wdone
            cpx         #W_COUNT
            beq         vm_wdone
            stz         ex                                  ; (any?'s and find's: NIL; all?'s: T)
            stz         ex + 1
            cpx         #W_ALL
            bne         vm_wdone
            lda         #<T_VAL
            sta         ex
; The walk's value, ex: its frame dropped, the code past the BCALL on, its frame its caller's again (vm_s h words
; under the BCALL's place, its h's; vm_rb and vm_mat as they were), its error returned if its r says
vm_wdone:
            jsr         vm_wat
            ldy         #WF_RB
            lda         (ht),y
            and         #$FE
            sta         vm_rb
            iny
            lda         (ht),y
            sta         vm_mat
            ldy         #WF_CODE + 1
            lda         (ht),y
            pha
            dey
            lda         (ht),y
            pha
            lda         #WF_SIZE / 2
            jsr         drop
            pla
            plx
            jsr         vm_decode
            lda         vm_bank
            sta         $00
            lda         vm_ip
            sec
            sbc         #2
            sta         ht
            lda         vm_ip + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)                                ; (h)
            asl
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         vm_s
            lda         sp + 1
            sbc         #0
            sta         vm_s + 1
            ldy         #1                                  ; (r: R)
            lda         (ht),y
            lsr
            bcc         :+
            jsr         vm_iserr
            bcc         :+
            jmp         vm_ret
:
            jmp         vm_next

; C = 0 if the walk's function (vm_wf) is one the machine calls as forms.inc's walk would (ev_apply_values): a
; function's cell (not an fexpr's), one partially applied, or a built-in, not a special form nor one that runs the
; machine, that takes as many as it's given (foldl's two, else one); else C = 1 (the evaluator's walk).  Keeps ht
vm_wfok:
            lda         vm_wf
            lsr
            bcs         @no
            ldx         vm_wf + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcc         @no
            txa
            lsr
            lda         vm_wf
            ror
            tax
            lda         vb_flags,x
            and         #BF_MACHINE | BF_SPECIAL
            bne         @no
            ldy         #1
            lda         vm_wop
            cmp         #W_FOLDL
            bne         :+
            iny
:
            tya
            cmp         vb_min,x
            bcc         @no
            cmp         vb_max,x
            beq         @yes
            bcs         @no
@yes:
            clc
            rts
@cell:
            lda         pk,x
            cmp         #PK_PARTIAL
            beq         @yes
            cmp         #PK_FUNC
            bne         @no
            lda         ht
            pha
            lda         ht + 1
            pha
            lda         vm_wf
            ldy         #3
            jsr         cell_get
            tay
            pla
            sta         ht + 1
            pla
            sta         ht
            tya
            and         #FN_FEXPR
            beq         @yes
@no:
            sec
            rts

; vm_s, vm_rb, vm_mat: the walk's caller's frame's (its frame on top: ht, vm_wat's), as its function's call may
; have left them another's
vm_wframe:
            ldy         #WF_S
            lda         (ht),y
            and         #$FE
            sta         vm_k
            iny
            lda         (ht),y
            sta         vm_l
            lda         ht
            sec
            sbc         vm_k
            sta         vm_s
            lda         ht + 1
            sbc         vm_l
            sta         vm_s + 1
            ldy         #WF_RB
            lda         (ht),y
            and         #$FE
            sta         vm_rb
            iny
            lda         (ht),y
            sta         vm_mat
            rts

; ht = the walk's frame (on top)
vm_wat:
            lda         sp
            sec
            sbc         #WF_SIZE
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            rts

; Native code's TSELF past a flagged SHEAD that pushed nothing (tselfh's: Ctrl-C, a note): this frame's function
; pushed under the .A arguments on top, as SHEAD would have (vm_shf $80), for TSELF's way (op_tself)
vm_tsh:
            sta         vm_k
            lda         #0                                  ; (A word more: VPUSH's, which may spill and move the
            tax                                             ;   stack down)
            VPUSH
            lda         vm_k
            asl
            sta         vm_l
            clc
            adc         #2
            sta         vm_t
            lda         sp                                  ; (ht: the first argument's place, hp past it)
            sec
            sbc         vm_t
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         ht
            clc
            adc         #2
            sta         hp
            lda         ht + 1
            adc         #0
            sta         hp + 1
            ldy         vm_l                                ; (The arguments a word up, the last first)
            beq         @fn
@up:
            dey
            lda         (ht),y
            sta         (hp),y
            tya
            bne         @up
@fn:
            lda         (vm_s)
            sta         (ht)
            ldy         #1
            lda         (vm_s),y
            sta         (ht),y
            lda         #$80
            sta         vm_shf
            rts

; A native op's stub (vmx.inc's: jsr vm_sj, then the word of the op's code in the machine, then the op's data, the op
; as it was): vm_ip its data, its code run.  (The op's slow way: a template's, or the op's alone)
vm_sj:
            pla
            sta         vm_ip
            pla
            sta         vm_ip + 1
            ldy         #1
            lda         (vm_ip),y
            sta         vm_sjv
            iny
            lda         (vm_ip),y
            sta         vm_sjv + 1
            lda         vm_ip
            clc
            adc         #3
            sta         vm_ip
            bcc         :+
            inc         vm_ip + 1
:
            jmp         (vm_sjv)

; RET in native code (its op: jmp vm_nret): to its caller's code (in another bank of the arena, vm_retx's; the
; evaluator's call, NIL: the machine's RET), the frame dropped: its return pad (past the call's data) finds the
; caller's frame again; an error, vm_reterr's (returned by the caller too if its call's r says)
vm_nret:
            ldy         vm_rb
            lda         (vm_s),y
            bne         :+
            jmp         op_ret
:
            and         #$FE
            sta         vm_ip
            iny
            lda         (vm_s),y
            tax
            and         #$1F
            ora         #$80
            sta         vm_ip + 1
            txa
            and         #$E0
            cmp         vm_idx
            beq         :+
            jmp         vm_retx
:
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
            bcs         @go
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcc         @go
            lda         pk,x
            cmp         #PK_ERROR
            bne         @go
            jmp         vm_reterr
@go:
            jmp         (vm_ip)

; RET's in native code, to a caller in another bank of the arena (.A its index, times 32; vm_ip its return pad): its
; bank's, then RET's way here (the frame dropped, an error looked at), on to the pad
vm_retx:
            jsr         vm_newidx
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
            jsr         vm_iserr
            bcc         :+
            jmp         vm_reterr
:
            jmp         vm_next

; RET's in native code, ex an error, vm_ip its caller's return pad (not come to): the caller's frame (vm_callsite's),
; and the error returned from it too if its call's r says; else on, to the pad (its frame again: the same)
vm_reterr:
            jsr         vm_callsite
            bcc         :+
            jmp         vm_ret
:
            jmp         vm_nextf

; A frame's record pushed near a page's end (native code's CALL and CSELF; VPUSH's way, which may spill and move the
; stack down): .A, .X its return, no scope; vm_s its function's word, .Y (M) bytes below the record
vm_rec:
            sty         vm_k
            VPUSH
            lda         #0
            tax
            VPUSH
            lda         vm_k
            clc
            adc         #4
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         vm_s
            lda         sp + 1
            sbc         #0
            sta         vm_s + 1
            rts

; sp's page crossed by VPUSH: the next (the stack's oldest block spilled if it's full)
vm_page:
            inc         sp + 1
            lda         sp + 1
            cmp         stk_endp
            bcc         :+
            jmp         spill
:
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
            adc         #2
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            rts

; vm_mat from the record's scope (made: $80)
vm_matget:
            jsr         vm_fslot                            ; (A scope made: a frame there)
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            lda         #0
            bcc         :+
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
            VPUSH
            lda         #0
            tax
            VPUSH
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
            VPUSH
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
            lda         vm_s                                ; (The stack cut back to the frame)
            sta         sp
            ldx         vm_s + 1
            stx         sp + 1
            cpx         stk_basep
            bne         :+
            lda         spilled
            beq         :+
            jsr         unspill
:
            lda         vm_t                                ; (NIL: the evaluator's call, the value for its frame on
            bne         :+                                  ;   top)
            stz         vm_on
            LDAX        ev_return
            jmp         vm_go1
:
            lda         vm_t                                ; (Its caller's code: vm_decode's)
            and         #$FE
            sta         vm_ip
            lda         vm_t + 1
            tax
            and         #$1F
            ora         #$80
            sta         vm_ip + 1
            txa
            and         #$E0
            cmp         vm_idx
            beq         :+
            jsr         vm_newidx
:
            lda         vm_bank                             ; (Its caller's frame: vm_callsite's)
            sta         $00
            lda         vm_ip
            sec
            sbc         #2
            sta         ht
            lda         vm_ip + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)
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
            lda         (ht),y
            tax
            and         #$FE
            sta         vm_rb
            tay                                             ; (vm_mat: vm_matget's)
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
            txa                                             ; (R: its error returned)
            lsr
            bcc         :+
            jsr         vm_iserr
            bcc         :+
            jmp         vm_ret
:
            jmp         vm_nextf

; ee = the scope where the code at vm_opa runs: this frame's (made if it isn't yet: its parent the function's own
; scope, its formals and locals bound in order, holes too; where they're read and set from then on), or the
; innermost block's the code is in (vm_blocks: each made as it's wanted, outermost first).  It may collect
vm_scope:
            bit         vm_mat
            bmi         :+
            jsr         vm_mkfn
:
            jsr         vm_fslot                            ; (ee: the frame's scope)
            lda         (vm_s),y
            sta         ee
            iny
            lda         (vm_s),y
            sta         ee + 1
            jsr         vm_lplace                           ; (Its blocks: its LOCALS op's table)
            bcs         @rts
            ldy         #3
            lda         (ht),y
            sta         vm_u
            iny
            lda         (ht),y
            sta         vm_u + 1
            ora         vm_u
            beq         @rts
            jmp         vm_blocks
@rts:
            rts

; OUT: .Y = the place of this frame's scope: the record's scope word (rb + 2); past it (rb + 4) if a LOCALS op's
; place is there (a fixnum)
vm_fslot:
            ldy         vm_rb
            iny
            iny
            lda         (vm_s),y
            lsr
            bcc         :+
            iny
            iny
:
            rts

; OUT: ht = this frame's LOCALS op (in its code: the window's set), C = 0; C = 1 if it has none
vm_lplace:
            ldy         vm_rb
            iny
            iny
            lda         (vm_s),y
            lsr
            bcs         :+
            sec
            rts
:
            iny
            lda         (vm_s),y
            and         #$1F
            ora         #$80
            sta         ht + 1
            dey
            lda         (vm_s),y
            and         #$FE
            sta         ht
            lda         vm_bank
            sta         $00
            clc
            rts

; This frame's scope made: its parent the function's scope; its formals bound to its arguments, then its locals to
; theirs (holes too: LOCALS k kn tbl names); vm_mat set
vm_mkfn:
            stz         vm_lk                               ; (Its locals: kn of them, their names at its LOCALS op)
            jsr         vm_lplace
            bcs         @nolocals
            ldy         #2
            lda         (ht),y
            sta         vm_lk
            lda         ht
            clc
            adc         #5
            sta         vm_ln
            lda         ht + 1
            adc         #0
            sta         vm_ln + 1
@nolocals:
            jsr         vm_self
            ldy         #2
            jsr         cell_get
            sta         eu
            stx         eu + 1
            lda         vm_rb
            lsr
            dec         a
            clc
            adc         vm_lk
            FARN        1, frame_new
            MOVW        ee, hv
            jsr         vm_fslot
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
            beq         @locals
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
@locals:
            lda         vm_rb                               ; (Its locals, from rb + 6, each bound to its slot)
            clc
            adc         #6
            sta         vm_k
@local:
            lda         vm_lk
            beq         @made
            ldy         vm_k
            lda         (vm_s),y
            sta         eu
            iny
            lda         (vm_s),y
            sta         eu + 1
            lda         vm_bank
            sta         $00
            lda         vm_ln
            sta         ht
            lda         vm_ln + 1
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            tax
            lda         (ht)
            FARN        1, frame_add
            lda         vm_ln
            clc
            adc         #2
            sta         vm_ln
            bcc         :+
            inc         vm_ln + 1
:
            inc         vm_k
            inc         vm_k
            dec         vm_lk
            bra         @local
@made:
            lda         #$80
            sta         vm_mat
            rts

; vm_u = this frame's block table ([n] [start end]...: each block's code, start its BLOCK op): ee = the scope of the
; innermost block the code at vm_opa is in (each of its blocks made as it's wanted, outermost first); ee as it was
; (the frame's) if it's in none
vm_blocks:
            lda         vm_bank
            sta         $00
            lda         vm_u
            sta         hq
            lda         vm_u + 1
            sta         hq + 1
            lda         (hq)
            sta         vm_k
            stz         vm_t
            stz         vm_t + 1
            ldy         #1
@entry:
            lda         vm_k
            beq         @found
            lda         vm_opa                              ; (start <= vm_opa < end: this one, or one in it later)
            cmp         (hq),y
            iny
            lda         vm_opa + 1
            sbc         (hq),y
            iny
            bcc         @skip
            lda         vm_opa
            cmp         (hq),y
            iny
            lda         vm_opa + 1
            sbc         (hq),y
            iny
            bcs         @next
            phy
            tya
            sec
            sbc         #4
            tay
            lda         (hq),y
            sta         vm_t
            iny
            lda         (hq),y
            sta         vm_t + 1
            ply
            bra         @next
@skip:
            iny
            iny
@next:
            dec         vm_k
            bra         @entry
@found:
            lda         vm_t + 1
            beq         @rts
            ldx         #0                                  ; (Its chain to the outermost, on the 65C02's stack)
@chain:
            lda         vm_t
            pha
            lda         vm_t + 1
            pha
            inx
            lda         vm_t
            sta         ht
            lda         vm_t + 1
            sta         ht + 1
            ldy         #4
            lda         (ht),y
            sta         vm_t
            iny
            lda         (ht),y
            sta         vm_t + 1
            ora         vm_t
            bne         @chain
            stx         vm_bc
@make:
            pla
            sta         ht + 1
            pla
            sta         ht
            jsr         vm_mkblk
            dec         vm_bc
            bne         @make
@rts:
            rts

; ht = a block's BLOCK op (fb k vb parent names), ee the scope it's in: ee = the block's scope (made, if its frame
; slot has none yet: its variables bound, holes too, from its slots)
vm_mkblk:
            lda         ht
            sta         vm_bd
            lda         ht + 1
            sta         vm_bd + 1
            lda         vm_bank
            sta         $00
            ldy         #1
            lda         (ht),y
            sta         vm_bf
            tay
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            bcc         @make
            sta         ee + 1
            dey
            lda         (vm_s),y
            sta         ee
            rts
@make:
            MOVW        eu, ee
            ldy         #2
            lda         (ht),y
            sta         vm_bk
            iny
            lda         (ht),y
            sta         vm_bv
            lda         vm_bk
            FARN        1, frame_new
            MOVW        ee, hv
            ldy         vm_bf
            lda         ee
            sta         (vm_s),y
            iny
            lda         ee + 1
            sta         (vm_s),y
            stz         vm_bj
@var:
            lda         vm_bj
            cmp         vm_bk
            bcs         @done
            asl
            clc
            adc         vm_bv
            tay
            lda         (vm_s),y
            sta         eu
            iny
            lda         (vm_s),y
            sta         eu + 1
            lda         vm_bank
            sta         $00
            lda         vm_bd
            sta         ht
            lda         vm_bd + 1
            sta         ht + 1
            lda         vm_bj
            asl
            clc
            adc         #6
            tay
            lda         (ht),y
            pha
            iny
            lda         (ht),y
            tax
            pla
            FARN        1, frame_add
            inc         vm_bj
            bra         @var
@done:
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

; OUT: .A, .X = the arguments, as written, of the call after the op (HEAD's, SHEAD's)
vm_cargs:
            lda         vm_bank
            sta         $00
            ldy         #3
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            ldy         #1
            jmp         cell_get

; ---- The ops

; CSELF's and TSELF's: vm_m = its arguments; ht = the function's word under them; to no unless it's this frame's
; function, as many arguments as its formals, and no Ctrl-C nor a note
.macro SELFQ no
            ldy         #1
            lda         (vm_ip),y
            sta         vm_m
            asl
            adc         #2
            cmp         vm_rb
            bne         no
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            cmp         (vm_s),y
            bne         no
            lda         (ht)
            cmp         (vm_s)
            bne         no
            bit         intr
            bmi         no
            bvs         no
.endmacro

op_const:
            ldy         #1
            lda         (vm_ip),y
            sta         ex
            iny
            lda         (vm_ip),y
            sta         ex + 1
            jmp         vm_n3

op_cpush:
            ldy         #2
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            VPUSH
            jmp         vm_n3

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
            jmp         vm_n2
@scope:
            jsr         vm_slot
            jmp         vm_adv2

op_lpush:
            ldy         #1
            lda         (vm_ip),y
            bit         vm_mat
            bmi         @scope
            tay
            iny
            lda         (vm_s),y
            tax
            dey
            lda         (vm_s),y
            VPUSH
            jmp         vm_n2
@scope:
            jsr         vm_slot
            lda         ex
            ldx         ex + 1
            VPUSH
            jmp         vm_adv2

; .A = an argument's or a local's place: ex = its value in the frame's scope (vm_slot); hp = the scope's cell, .Y =
; its pair's value there (vm_pairp: argument k's pair k, at 2y + 2; local j's pair n + j, at 2y - 6)
vm_slot:
            jsr         vm_pairp
            lda         (hp),y
            sta         ex
            iny
            lda         (hp),y
            sta         ex + 1
            rts
vm_pairp:
            cmp         vm_rb
            bcc         @arg
            asl                                             ; (Local j, at rb + 6 + 2j: its pair n + j's, 2y - 10)
            sec
            sbc         #10
            bra         @at
@arg:
            asl
            adc         #2
@at:
            pha
            jsr         vm_fslot
            lda         (vm_s),y
            pha
            iny
            lda         (vm_s),y
            tax
            pla
            jsr         deref
            ply
            rts

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
            ldy         #1
            jsr         vm_glook
            lda         #3
            jmp         vm_valerr

; ex = the value of the symbol at .Y in the op (lookup's: the scope made's, if there's one, else its global's)
vm_glook:
            MOVW        vm_opa, vm_ip
            stz         ee
            stz         ee + 1
            bit         vm_mat
            bpl         :+
            phy
            jsr         vm_scope
            ply
:
            lda         vm_bank
            sta         $00
            lda         (vm_ip),y
            sta         ex
            iny
            lda         (vm_ip),y
            sta         ex + 1
            FARN        1, lookup
            rts

op_lookup:
            MOVW        vm_opa, vm_ip
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
            VPUSH
            jmp         vm_n1

op_jf:
            lda         ex + 1
            bne         @no
            lda         ex
            beq         @to
            cmp         #<SNIL
            beq         @to
@no:
            jmp         vm_n3
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
            jmp         vm_n3
@to:
            ldy         #1
            jmp         vm_jumpy

op_jmp:
            ldy         #1
            jmp         vm_jumpy

op_je:
            jsr         vm_iserr
            bcs         :+
            jmp         vm_n4
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

; SHEAD: the global value of the symbol at 6 (a function's own name, as it was compiled): this frame's function,
; pushed at once; anything else, HEAD's way
op_shead:
            ldy         #1                                  ; (Flagged: its head pushed, vm_shf says)
            lda         (vm_ip),y
            bpl         :+
            lda         #$80
            sta         vm_shf
:
            bit         vm_mat                              ; (Its cache: the global's value, as it was at this
            bmi         @slow                               ;   epoch; no scope made)
            ldy         #8
            lda         (vm_ip),y
            cmp         vm_ep
            bne         @slow
            iny
            lda         (vm_ip),y
            cmp         vm_ep + 1
            bne         @slow
            ldy         #11
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            VPUSH
            lda         #12
            jmp         vm_na
@slow:
            lda         #12
            sta         vm_fl
            stz         vm_shc
            bit         vm_mat
            bmi         @look
            dec         vm_shc                              ; (Read where it is: its cache filled)
            ldy         #7
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
            ldy         #1                                  ; (This frame's function?)
            lda         (vm_s),y
            cmp         ex + 1
            bne         @other
            lda         (vm_s)
            cmp         ex
            bne         @other
            ldx         ex + 1
            VPUSH
            jsr         vm_shfill
            lda         #12
            jmp         vm_adv
@other:
            lda         vm_bank                             ; (Flagged: its cache this frame's function alone.  The
            sta         $00                                 ;   code's bank in the window again: deref's took it)
            ldy         #1
            lda         (vm_ip),y
            bpl         :+
            stz         vm_shc
:
            lda         ex
            cmp         #<UNBOUND
            bne         vm_head
            lda         ex + 1
            cmp         #>UNBOUND
            bne         vm_head
@look:
            stz         vm_shc
            ldy         #6
            jsr         vm_glook
            bra         vm_head

; SHEAD's cache: ex, at this epoch (none if it's run out)
vm_shfill:
            lda         vm_bank
            sta         $00
            lda         vm_ep
            and         vm_ep + 1
            cmp         #$FF
            beq         @rts
            ldy         #8
            lda         vm_ep
            sta         (vm_ip),y
            iny
            lda         vm_ep + 1
            sta         (vm_ip),y
            iny
            lda         ex
            sta         (vm_ip),y
            iny
            lda         ex + 1
            sta         (vm_ip),y
@rts:
            rts

; HEAD: ex, the function of a call of m arguments (c the call, t past it); vm_fl, the op's length
op_head:
            lda         #6
            sta         vm_fl
            stz         vm_shc
vm_head:
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
            cmp         #PK_PARTIAL                         ; (A function partially applied: CALL's, its values
            beq         @push                               ;   first)
            cmp         #PK_BUFFER                          ; (A buffer given an index: CALL's)
            bne         @hash
            lda         vm_bank
            sta         $00
            ldy         #1
            lda         (vm_ip),y
            and         #$7F
            cmp         #1
            beq         @push
            bra         @eval
@hash:
            cmp         #PK_HASH
            beq         @eval
@notfn:
            lda         vm_bank                             ; (Not a function: with no arguments, itself; else the
            sta         $00                                 ;   evaluator's, its error or exit's)
            ldy         #1
            lda         (vm_ip),y
            and         #$7F
            bne         @eval
@to:
            lda         vm_bank
            sta         $00
            ldy         #4
            jmp         vm_jumpy
@error:
            lda         vm_bank
            sta         $00
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
            VPUSH
            bit         vm_shc
            bpl         :+
            jsr         vm_shfill
:
            lda         vm_fl
            jmp         vm_adv
@eval:
            MOVW        vm_opa, vm_ip
            jsr         vm_scope                            ; (The evaluator's, from the function: ev_head's, with
            jsr         vm_cargs                            ;   the arguments as written, in this frame's scope)
            sta         cr
            stx         cr + 1
            lda         vm_bank
            sta         $00
            ldy         #4
            lda         (vm_ip),y
            tax
            iny
            lda         (vm_ip),y
            sta         vm_ip + 1
            stx         vm_ip
            LDAX        ev_head
            jmp         vm_out0

; CSELF m code ret h r, TSELF m code 0: a call of this frame's function (as compiled: else CALL's and TCALL's way),
; its code at code (CSELF's return, ret)
op_tself:
            SELFQ       @call
            ldy         #2                                  ; (Its arguments to this frame's; its return kept, no
            ldx         vm_m                                ;   scope)
            beq         @args
@arg:
            lda         (ht),y
            sta         (vm_s),y
            iny
            lda         (ht),y
            sta         (vm_s),y
            iny
            dex
            bne         @arg
@args:
            bit         vm_mat                              ; (A scope made: the record's none again)
            bpl         :+
            iny
            iny
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            stz         vm_mat
:
            lda         vm_rb                               ; (The stack cut back to the record's end: its locals
            clc                                             ;   made again by the code's LOCALS)
            adc         #4
            adc         vm_s
            pha
            lda         vm_s + 1
            adc         #0
            tax
            pla
            sta         sp
            stx         sp + 1
            cpx         stk_basep
            bne         :+
            lda         spilled
            beq         :+
            jsr         unspill
:
            jmp         vm_scode
@call:
            MOVW        vm_opa, vm_ip
            stz         vm_ccf
            lda         #$80
            ldx         #5
            jmp         vm_scall

op_cself:
            SELFQ       @call
            bra         @self
@call:
            MOVW        vm_opa, vm_ip
            ldy         #4                                  ; (CALL's way: its return, compiled)
            lda         (vm_ip),y
            sta         vm_r
            iny
            lda         (vm_ip),y
            sta         vm_r + 1
            stz         vm_ccf
            lda         #0
            ldx         #8
            jmp         vm_scall
@self:
            lda         depth + 1                           ; (Too deep: CALL's way, its error)
            cmp         #>MAX_DEPTH
            bcc         :+
            bne         @call
            lda         depth
            cmp         #<MAX_DEPTH
            bcs         @call
:
            ldy         #5                                  ; (Its frame: its return, past the op; no scope)
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            VPUSH
            lda         #0
            tax
            VPUSH
            lda         vm_rb
            clc
            adc         #4
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         vm_s
            lda         sp + 1
            sbc         #0
            sta         vm_s + 1
            stz         vm_mat
            inc         depth
            bne         vm_scode
            inc         depth + 1
            bra         vm_scode

; The code at the op's bytes 2 and 3 (this function's) run
vm_scode:
            lda         vm_bank
            sta         $00
            ldy         #3
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            sta         vm_ip
            stx         vm_ip + 1
            jmp         vm_nextf

; Not this frame's function (or Ctrl-C, too deep ...): CALL's or TCALL's way (.A vm_tl, .X the op's length)
vm_scall:
            sta         vm_tl
            txa
            clc
            adc         vm_ip
            sta         vm_ip
            bcc         :+
            inc         vm_ip + 1
:
            jmp         vm_call

op_tcall:
            lda         #$80
            sta         vm_tl
            stz         vm_ccf
            lda         #4
            jmp         vm_callop
op_call:
            ldy         #6                                  ; (Its cache: at this epoch ...
            lda         (vm_ip),y
            cmp         vm_ep
            bne         @miss
            iny
            lda         (vm_ip),y
            cmp         vm_ep + 1
            bne         @miss
            ldy         #1                                  ;   the function it called last, under the arguments)
            lda         (vm_ip),y
            sta         vm_m
            asl
            adc         #2
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #4
            lda         (vm_ip),y
            cmp         (ht)
            bne         @miss
            iny
            lda         (vm_ip),y
            ldy         #1
            cmp         (ht),y
            bne         @miss
            bit         intr                                ; (Ctrl-C, a note, too deep: CALL's way)
            bmi         @miss
            bvs         @miss
            lda         depth + 1
            cmp         #>MAX_DEPTH
            bcc         @hit
            bne         @miss
            lda         depth
            cmp         #<MAX_DEPTH
            bcc         @hit
@miss:
            jmp         op_callm
@hit:
            ldy         #3                                  ; (Its frame: its return, no scope)
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            VPUSH
            lda         #0
            tax
            VPUSH
            lda         vm_k
            sta         vm_rb
            clc
            adc         #4
            sta         vm_k
            lda         sp
            sec
            sbc         vm_k
            sta         vm_s
            lda         sp + 1
            sbc         #0
            sta         vm_s + 1
            stz         vm_mat
            inc         depth
            bne         :+
            inc         depth + 1
:
            ldy         #11                                 ; (Its code)
            lda         (vm_ip),y
            cmp         vm_idx
            beq         :+
            sta         vm_idx
            dey
            lda         (vm_ip),y
            sta         vm_bank
:
            ldy         #9
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            sta         vm_ip
            stx         vm_ip + 1
            jmp         vm_next

; CALL, its cache missed (native code's way in, its template having looked): a buffer given a fixnum index in it,
; its byte here (vm_bget's; Ctrl-C or a note, CALL's way), and on past its return pad; else the call's way (vm_callg)
op_callm:
            ldy         #1
            lda         (vm_ip),y
            dec         a
            bne         vm_callg
            lda         sp                                  ; (ht: the buffer, under the index)
            sec
            sbc         #4
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)
            lsr
            bcs         vm_callg
            ldy         #1
            lda         (ht),y
            tax
            cpx         #IMM_PAGES
            bcc         vm_callg
            lda         pk,x
            cmp         #PK_BUFFER
            bne         vm_callg
            bit         intr
            bmi         vm_callg
            bvs         vm_callg
            ldy         #3                                  ; (Its index)
            lda         (ht),y
            tax
            dey
            lda         (ht),y
            jsr         vm_bidx
            bcs         vm_callg
            ldy         #1
            lda         (ht),y
            tax
            lda         (ht)
            jsr         vm_bat
            bcs         @no
            lda         (hp)
            jsr         vm_bfix
            lda         ht                                  ; (The two words dropped)
            sta         sp
            ldx         ht + 1
            stx         sp + 1
            cpx         stk_basep
            bne         :+
            lda         spilled
            beq         :+
            jsr         unspill
:
            lda         #14 + VXT_PADL                      ; (Past its return pad: the frame's the same)
            jmp         vm_adv
@no:
            lda         vm_bank
            sta         $00
vm_callg:
            stz         vm_tl
            lda         #$80                                ; (Its cache filled, if it's a compiled function's)
            sta         vm_ccf
            ldy         #2                                  ; (Its return: past it, compiled)
            lda         (vm_ip),y
            sta         vm_r
            iny
            lda         (vm_ip),y
            sta         vm_r + 1
            lda         #14
; CALL's and TCALL's: .A, the op's length
vm_callop:
            pha
            MOVW        vm_opa, vm_ip
            ldy         #1
            lda         (vm_ip),y
            sta         vm_m
            pla                                             ; (ip: past it, where its value goes)
            clc
            adc         vm_ip
            sta         vm_ip
            bcc         vm_call
            inc         vm_ip + 1
; A call: vm_m arguments on top, the function under them; vm_tl $80 in tail position (else vm_r its return); ip
; past the op
vm_call:
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
            beq         :+
            jmp         vm_cother
:
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
            lda         (hp),y                              ;   be; its first, the evaluator's, counted there)
            bmi         vm_cout
            ldy         #10
            lda         (hp),y
            beq         vm_cout
            FARN        6, vm_compile
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
            lda         vm_r                                ; (Its return: this code past the call)
            sta         vm_t
            lda         vm_r + 1
            sta         vm_t + 1
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
            lda         vm_bank                             ; (The caller's: its CALL's cache filled after)
            sta         vm_cb
            lda         vm_u
            ldx         vm_u + 1
            jsr         vm_decode
            bit         vm_ccf
            bpl         :+
            jsr         vm_cfill
:
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

; CALL's cache (the op at vm_opa, in the caller's bank vm_cb): cf, at this epoch (none if it's run out), its code
; at vm_ip (vm_bank, vm_idx)
vm_cfill:
            stz         vm_ccf
            lda         vm_ep
            and         vm_ep + 1
            cmp         #$FF
            beq         @rts
            lda         vm_cb
            sta         $00
            MOVW        ht, vm_opa
            ldy         #4
            lda         cf
            sta         (ht),y
            iny
            lda         cf + 1
            sta         (ht),y
            iny
            lda         vm_ep
            sta         (ht),y
            iny
            lda         vm_ep + 1
            sta         (ht),y
            iny
            lda         vm_ip
            sta         (ht),y
            iny
            lda         vm_ip + 1
            sta         (ht),y
            iny
            lda         vm_bank
            sta         (ht),y
            iny
            lda         vm_idx
            sta         (ht),y
@rts:
            rts

; A buffer's byte, here (buffers.inc's buf_at and buffer-put, for a fixnum index in the buffer): vm_bget, (b i): cf
; the buffer, ex the index: ex = its byte, C = 0; C = 1 (ex kept) if the index isn't a fixnum in it.  vm_bput,
; (buffer-put b i byte) with its arguments on top: ex = the old byte, the new one put, C = 0; C = 1 if they aren't
; a buffer, a fixnum index in it and a fixnum 0-255 (buffer-put's way, then)
vm_bget:
            lda         ex
            ldx         ex + 1
            jsr         vm_bidx
            bcs         vm_bno
            lda         cf
            ldx         cf + 1
            jsr         vm_bat
            bcs         vm_bno
            lda         (hp)
vm_bfix:
            asl
            sta         ex
            lda         #0
            rol
            sta         ex + 1
            inc         ex
            clc
vm_bno:
            rts
vm_bput:
            lda         sp
            sec
            sbc         #6
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #4                                  ; (Its byte: 0-255)
            lda         (ht),y
            lsr
            bcc         @no
            sta         vm_k
            iny
            lda         (ht),y
            cmp         #2
            bcs         @no
            lsr
            bcc         :+
            lda         vm_k
            ora         #$80
            sta         vm_k
:
            ldy         #3                                  ; (Its index)
            lda         (ht),y
            tax
            dey
            lda         (ht),y
            jsr         vm_bidx
            bcs         @no
            ldy         #1                                  ; (Its buffer)
            lda         (ht),y
            tax
            lda         (ht)
            lsr
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_BUFFER
            bne         @no
            lda         (ht)
            jsr         vm_bat
            bcs         @no
            lda         (hp)
            pha
            lda         vm_k
            sta         (hp)
            pla
            bra         vm_bfix
@no:
            sec
            rts

; .A, .X = an index: vm_t = it, C = 0, if it's a fixnum 0 on; else C = 1
vm_bidx:
            sta         vm_t
            lsr
            bcc         @no
            txa
            bmi         @no
            lsr
            sta         vm_t + 1
            lda         vm_t
            ror
            sta         vm_t
            clc
            rts
@no:
            sec
            rts

; .A, .X = a buffer: hp = its byte vm_t's place (its blob's bank in the window), C = 0; C = 1 if vm_t isn't in it
vm_bat:
            asl                                             ; (Its cell: deref's)
            sta         hp
            lda         bank_of,x
            sta         $00
            lda         ptr_hi,x
            adc         #0
            sta         hp + 1
            lda         vm_t                                ; (Below its length)
            cmp         (hp)
            ldy         #1
            lda         vm_t + 1
            sbc         (hp),y
            bcs         @rts
            iny                                             ; (Its blob: blob_deref's, its bytes past its header)
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            tax
            and         #$0F
            sta         hp + 1
            txa
            lsr
            lsr
            lsr
            lsr
            tax
            lda         blob_bank,x
            sta         $00
            pla
            asl
            rol         hp + 1
            adc         vm_t
            sta         hp
            lda         hp + 1
            adc         vm_t + 1
            sta         hp + 1
            lda         hp
            clc
            adc         #4
            sta         hp
            lda         hp + 1
            adc         #$80
            sta         hp + 1
            clc
@rts:
            rts

; BCALL's bit-and, bit-or, bit-xor of two (native code's): two fixnums on top: ex = their bits' (a fixnum's tag bit
; is theirs, and or or; xor's set again), the two dropped, C = 0; else C = 1, nothing changed (BCALL's way)
vm_band:
            jsr         vm_b2
            bcs         vm_brts
            lda         (ht)
            and         (ht),y
            sta         vm_a
            dey
            lda         (ht),y
            ldy         #3
            and         (ht),y
            bra         vm_bset
vm_bor:
            jsr         vm_b2
            bcs         vm_brts
            lda         (ht)
            ora         (ht),y
            sta         vm_a
            dey
            lda         (ht),y
            ldy         #3
            ora         (ht),y
            bra         vm_bset
vm_bxor:
            jsr         vm_b2
            bcs         vm_brts
            lda         (ht)
            eor         (ht),y
            ora         #1
            sta         vm_a
            dey
            lda         (ht),y
            ldy         #3
            eor         (ht),y
vm_bset:
            sta         ex + 1
            lda         vm_a
            sta         ex
; The words from ht dropped (sp = ht: back from the bank they were spilled to, if they were), C = 0
vm_bdrop:
            lda         ht
            sta         sp
            ldx         ht + 1
            stx         sp + 1
            cpx         stk_basep
            bne         :+
            lda         spilled
            beq         :+
            jsr         unspill
:
            clc
vm_brts:
            rts

; ht = the two words on top (sp - 4), .Y = 2.  OUT: C = 1 if they aren't both fixnums
vm_b2:
            lda         sp
            sec
            sbc         #4
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #2
            lda         (ht)
            and         (ht),y
            eor         #1
            lsr
            rts

; BCALL's shl and shr of two (native code's): n and k fixnums, k 0 or more (shl's below 15) and n shifted to a fixnum
; (shl: its sign kept at each step; shr: down, toward minus infinity, k past 15 as 15): ex = it, the two dropped,
; C = 0; else C = 1, nothing changed (BCALL's way)
vm_bshl:
            jsr         vm_b2
            bcs         vm_brts
            iny
            lda         (ht),y                              ; (k: 0 to 14)
            bne         @no
            dey
            lda         (ht),y
            lsr
            cmp         #15
            bcs         @no
            tax
            ldy         #1
            lda         (ht),y
            sta         vm_a + 1
            lda         (ht)                                ; (2n, shifted)
            and         #$FE
            sta         vm_a
            txa
            beq         @done
@step:
            lda         vm_a + 1                            ; (Bits 15 and 14 the same, or it isn't a fixnum)
            asl
            eor         vm_a + 1
            bmi         @no
            asl         vm_a
            rol         vm_a + 1
            dex
            bne         @step
@done:
            lda         vm_a                                ; (Its tag bit again)
            ora         #1
            sta         vm_a
            lda         vm_a + 1
            jmp         vm_bset
@no:
            sec
            rts
vm_bshr:
            jsr         vm_b2
            bcs         vm_brts
            iny
            lda         (ht),y                              ; (k: 0 or more)
            bmi         @no
            bne         @far
            dey
            lda         (ht),y
            lsr
            cmp         #15
            bcc         :+
@far:
            lda         #15
:
            tax
            ldy         #1
            lda         (ht),y
            sta         vm_a + 1
            lda         (ht)
            sta         vm_a
            txa
            beq         @done
@step:
            lda         vm_a + 1                            ; (Its sign kept: (2n + 1) >> k, with its tag bit set
            cmp         #$80                                ;   again, is 2 (n >> k) + 1)
            ror         vm_a + 1
            ror         vm_a
            dex
            bne         @step
@done:
            lda         vm_a
            ora         #1
            sta         vm_a
            lda         vm_a + 1
            jmp         vm_bset
@no:
            sec
            rts

; BCALL's char-at of two (native code's): a string and a fixnum index in it: ex = its byte's character, the two
; dropped, C = 0; else C = 1, nothing changed (BCALL's way).  The window kept
vm_bchat:
            lda         sp
            sec
            sbc         #4
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #2                                  ; (The index: a fixnum, 0 or more)
            lda         (ht),y
            lsr
            bcc         @no
            iny
            lda         (ht),y
            bmi         @no
            lsr
            sta         vm_t + 1
            dey
            lda         (ht),y
            ror
            sta         vm_t
            lda         (ht)                                ; (The string)
            lsr
            bcs         @no
            ldy         #1
            lda         (ht),y
            tax
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_STRING
            bne         @no
            lda         (ht)
            jsr         vm_bat
            bcs         @out
            lda         (hp)
            asl
            sta         ex
            lda         #0
            rol
            ora         #>CHAR0
            sta         ex + 1
            lda         vm_bank
            sta         $00
            jmp         vm_bdrop
@out:
            lda         vm_bank
            sta         $00
@no:
            sec
            rts

; BCALL's char-code of one (native code's): a character on top: ex = its code, it dropped, C = 0; else C = 1,
; nothing changed (BCALL's way)
vm_bccode:
            lda         sp
            sec
            sbc         #2
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)
            lsr
            bcs         @no
            ldy         #1
            lda         (ht),y
            and         #$FE
            cmp         #>CHAR0
            bne         @no
            lda         (ht)
            ora         #1
            sta         vm_a
            lda         (ht),y
            and         #1
            jmp         vm_bset
@no:
            sec
            rts

; BCALL's * of two (op_bcall's, native code's): two fixnums on top whose product is one: ex = it, the two dropped,
; C = 0; else C = 1, nothing changed (BCALL's way).  The window kept.  Both 0 to 127 (their high bytes 0): their
; product at once (under 16384)
vm_bmul:
            lda         sp
            sec
            sbc         #4
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            ldy         #3
            ora         (ht),y
            bne         @gen
            dey
            lda         (ht)
            and         (ht),y
            lsr
            bcc         @no
            lda         (ht)
            lsr
            tax
            lda         (ht),y
            lsr
            jsr         vm_mul8
            asl
            ora         #1
            sta         ex
            txa
            rol
            sta         ex + 1
            bra         @drop
@no:
            sec
            rts
@gen:
            ldy         #3
            lda         (ht),y
            sta         vm_b + 1
            dey
            lda         (ht),y
            sta         vm_b
            dey
            lda         (ht),y
            sta         vm_a + 1
            lda         (ht)
            sta         vm_a
            jsr         vm_fmul
            bcs         @rts
@drop:
            lda         ht                                  ; (The two dropped)
            sta         sp
            ldx         ht + 1
            stx         sp + 1
            cpx         stk_basep
            bne         @ok
            lda         spilled
            beq         @ok
            jsr         unspill
@ok:
            clc
@rts:
            rts

; ex = vm_a * vm_b, fixnums whose product is one (but -16384, the slow way's): C = 0; else C = 1, ex kept.  One of
; the two under 256 (the multiplier), the other in two bytes, each byte's product by vm_mul8
vm_fmul:
            lda         vm_a
            and         vm_b
            lsr
            bcs         :+
            sec
            rts
:
            lda         vm_a + 1                            ; (Its sign)
            eor         vm_b + 1
            sta         vm_msg
            lda         vm_a                                ; (|a|, |b|: one under 256)
            ldx         vm_a + 1
            jsr         vm_fabs
            sta         vm_m0
            stx         vm_m0 + 1
            lda         vm_b
            ldx         vm_b + 1
            jsr         vm_fabs
            cpx         #0
            beq         @m
            ldy         vm_m0 + 1
            bne         @no
            ldy         vm_m0
            sta         vm_m0
            stx         vm_m0 + 1
            tya
@m:
            sta         vm_mm
            ldx         vm_m0
            jsr         vm_mul8
            sta         vm_p0
            stx         vm_p0 + 1
            lda         vm_m0 + 1
            beq         @fit
            ldx         vm_mm
            jsr         vm_mul8
            cpx         #0
            bne         @no
            clc
            adc         vm_p0 + 1
            bcs         @no
            sta         vm_p0 + 1
@fit:
            lda         vm_p0 + 1                           ; (Under 16384: a fixnum, 2p + 1 or -2p + 1)
            cmp         #$40
            bcs         @no
            asl         vm_p0
            rol
            bit         vm_msg
            bmi         @neg
            sta         ex + 1
            lda         vm_p0
            ora         #1
            sta         ex
            clc
            rts
@neg:
            sta         vm_p0 + 1
            lda         #0
            sec
            sbc         vm_p0
            tay
            lda         #0
            sbc         vm_p0 + 1
            sta         ex + 1
            tya
            ora         #1
            sta         ex
            clc
            rts
@no:
            sec
            rts

; .A, .X = a fixnum: its number's magnitude (.A low, .X high)
vm_fabs:
            cpx         #$80
            pha
            txa
            ror
            tax
            pla
            ror
            cpx         #$80
            bcc         @rts
            eor         #$FF
            clc
            adc         #1
            pha
            txa
            eor         #$FF
            adc         #0
            tax
            pla
@rts:
            rts

; .A, .X = bytes a, b: .A, .X = a * b (low, high), quarter squares': q(a + b) - q(|a - b|), q(x) = x * x / 4
vm_mul8:
            sta         vm_q
            stx         vm_q + 1
            sec
            sbc         vm_q + 1
            bcs         :+
            eor         #$FF
            adc         #1
:
            tay
            lda         vm_q
            clc
            adc         vm_q + 1
            tax
            bcs         @hi
            lda         vm_sqlo,x
            sec
            sbc         vm_sqlo,y
            pha
            lda         vm_sqhi,x
            sbc         vm_sqhi,y
            tax
            pla
            rts
@hi:
            lda         vm_sqlo + 256,x
            sec
            sbc         vm_sqlo,y
            pha
            lda         vm_sqhi + 256,x
            sbc         vm_sqhi,y
            tax
            pla
            rts

; Not a function's cell: a buffer given an index (its byte, buffers.inc's buf_at); a function partially applied (the
; evaluator's ev_apply_values: its values first); else the evaluator's
vm_cother:
            cmp         #PK_BUFFER
            bne         @partial
            jsr         pop                                 ; (Its index)
            sta         ex
            stx         ex + 1
            jsr         vm_bget                             ; (Here, a fixnum in it; else buf_at's way)
            bcc         :+
            FARN        6, buf_at
            bcc         :+
            MOVW        ex, nomem_err
:
            lda         #1                                  ; (Its word)
            jsr         drop
            jmp         vm_cvalue
@partial:
            cmp         #PK_PARTIAL
            beq         @lb846
            jmp         vm_cout
@lb846:
            LDAX        ev_apply_values
            jmp         vm_cout2

; A built-in called (cf, as HEAD let it through: not a special form, nor one that takes errors): here, unless it
; runs the machine or isn't given as many arguments as it takes (the evaluator's: partially applied, or an error)
vm_cbuiltin:
            cpx         #>BUILTIN0
            bcs         @lb845
            jmp         vm_cout
@lb845:
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
            jmp         vm_cvalue
@out:
            lda         vb_flags,x                          ; (A special form: ev_apply_values', its error)
            bpl         :+
            LDAX        ev_apply_values
            jmp         vm_cout2
:
            lda         vb_vf,x
            beq         :+
            jsr         vm_scope
:
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
            ldy         #1                                  ; (* of two: here, if it's fixnums' and their product one)
            lda         (vm_ip),y
            cmp         #BIN_MUL
            bne         @bp
            iny
            lda         (vm_ip),y
            cmp         #2
            bne         @bc
            jsr         vm_bmul
            bcs         @bc
            lda         #5
            jmp         vm_adv
@bp:
            cmp         #BIN_BUFPUT                         ; (buffer-put, of three: here, if it can be)
            bne         @bc
            ldy         #1
            iny
            lda         (vm_ip),y
            cmp         #3
            bne         @bc
            jsr         vm_bput
            bcs         @bno
            lda         #3
            jsr         drop
            lda         #5
            jmp         vm_adv
@bno:
            lda         vm_bank
            sta         $00
@bc:
            MOVW        vm_opa, vm_ip
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
            bne         @mach
            lda         vm_m
            cmp         vb_min,x
            bcc         @out
            cmp         vb_max,x
            beq         :+
            bcs         @out
:
            jsr         vm_bcode
            jmp         vm_cvalue
@mach:
            cpx         #BIN_MAP                            ; (A walk but foldr's, of as many as it takes: here)
            bcc         vm_bout
            cpx         #BIN_MAP + W_PRODUCT + 1
            bcs         vm_bout
            cpx         #BIN_MAP + W_FOLDR
            beq         vm_bout
            lda         vm_m
            cmp         vb_min,x
            bne         vm_bout
            jmp         vm_walk
@out:
; BCALL's built-in (bi_n, .X) the evaluator's (one that runs the machine, or not given as many as it takes)
vm_bout:
            lda         vb_vf,x
            beq         :+
            jsr         vm_scope
:
            LDAX        ev_apply
            ldy         #0
            jmp         vm_outa

op_eval:
            MOVW        vm_opa, vm_ip
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
            MOVW        vm_opa, vm_ip
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

; ---- Locals, set, set!, def, while

; LOCALS k names: k locals (names a body binds with set, =(...)), each a hole (UNBOUND: not bound yet) after the
; record; the record's scope this op's place (a fixnum: the names' place, for vm_scope, till a scope's made)
op_locals:
            lda         vm_ip
            ldx         vm_ip + 1
            jsr         vm_encode
            ldy         vm_rb
            iny
            iny
            sta         (vm_s),y
            iny
            txa
            sta         (vm_s),y
            lda         #0                                  ; (The scope's slot: none yet)
            tax
            VPUSH
            ldy         #1
            lda         (vm_ip),y
            sta         vm_k
@hole:
            lda         #<UNBOUND
            ldx         #>UNBOUND
            VPUSH
            dec         vm_k
            bne         @hole
            lda         vm_bank
            sta         $00
            ldy         #2
            lda         (vm_ip),y
            asl
            adc         #5
            jmp         vm_adv

; LOCALH y sym: a local's value; a hole, sym's past this frame (the function's scope's: lookup's)
op_localh:
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
            bra         @hole
@scope:
            jsr         vm_slot
@hole:
            lda         ex + 1
            bne         @ok
            lda         ex
            cmp         #<UNBOUND
            bne         @ok
            jsr         vm_outer
            ldy         #2
            jsr         vm_symy
            FARN        1, lookup
            lda         #4
            jmp         vm_valerr
@ok:
            jmp         vm_adv4

; SETL y: the argument or local at y = ex (its pair, once there's a scope); ex = NIL
op_setl:
            ldy         #1
            lda         (vm_ip),y
            bit         vm_mat
            bmi         @scope
            tay
            lda         ex
            sta         (vm_s),y
            iny
            lda         ex + 1
            sta         (vm_s),y
            stz         ex
            stz         ex + 1
            jmp         vm_n2
@scope:
            jsr         vm_setl
            jmp         vm_adv2

; .A = an argument's or a local's place: it = ex (its pair, once there's a scope); ex = NIL
vm_setl:
            bit         vm_mat
            bmi         @scope
            tay
            lda         ex
            sta         (vm_s),y
            iny
            lda         ex + 1
            sta         (vm_s),y
            bra         @nil
@scope:
            jsr         vm_pairp
            lda         ex
            sta         (hp),y
            iny
            lda         ex + 1
            sta         (hp),y
@nil:
            stz         ex
            stz         ex + 1
            rts

; SETBL y sym: set! of a local: SETL's, unless it's a hole (not bound here yet): SETBG's, past this frame
op_setbl:
            ldy         #1
            lda         (vm_ip),y
            pha
            bit         vm_mat
            bmi         @scope
            tay
            iny
            lda         (vm_s),y
            bne         @set
            dey
            lda         (vm_s),y
            cmp         #<UNBOUND
            bne         @set
            bra         @hole
@scope:
            jsr         vm_pairp
            iny
            lda         (hp),y
            bne         @set
            dey
            lda         (hp),y
            cmp         #<UNBOUND
            bne         @set
@hole:
            pla
            ldy         #2
            lda         #4
            bra         vm_setbx
@set:
            pla
            jsr         vm_setl
            jmp         vm_adv4

; SETBG sym: set! of sym where it's bound nearest (the scope's, the function's scope's), else its global value (the
; error Unbound Symbol, if it has none); ex = NIL or the error
op_setbg:
            ldy         #1
            lda         #3
; set! of the symbol at .Y in the op (an op of .A bytes) to ex
vm_setbx:
            pha
            phy
            MOVW        vm_opa, vm_ip
            MOVW        eu, ex
            bit         vm_mat
            bpl         :+
            jsr         vm_scope
            bra         @set
:
            jsr         vm_outer
@set:
            ply
            lda         vm_bank
            sta         $00
            lda         (vm_ip),y
            sta         lk_s
            iny
            lda         (vm_ip),y
            sta         lk_s + 1
            FARN        1, set_bang
            bcc         :+
            MOVW        ex, nomem_err
:
            pla
            jmp         vm_valerr

; DEFG sym: sym's global value = ex (vm_rebind's first); ex = NIL
op_defg:
            ldy         #2
            lda         (vm_ip),y
            tax
            dey
            lda         (vm_ip),y
            jsr         deref
            jsr         vm_rebind
            ldy         #2
            lda         ex
            sta         (hp),y
            iny
            lda         ex + 1
            sta         (hp),y
            stz         ex
            stz         ex + 1
            jmp         vm_adv3

; STT: the word on top = ex
op_stt:
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
            jmp         vm_n1

; POPX: ex = the word popped
op_popx:
            jsr         pop
            sta         ex
            stx         ex + 1
            jmp         vm_n1

; LOOP t k e: to t; Ctrl-C, the error interrupted (VM_R: returned; else k words dropped, then to e)
op_loop:
            bit         intr
            bmi         :+
            ldy         #1
            jmp         vm_jumpy
:
            FARN        1, intr_err
            bcc         :+
            MOVW        ex, nomem_err
:
            lda         vm_bank
            sta         $00
            lda         (vm_ip)
            bpl         :+
            jmp         vm_ret
:
            ldy         #3
            lda         (vm_ip),y
            beq         :+
            jsr         drop
:
            lda         vm_bank
            sta         $00
            ldy         #4
            jmp         vm_jumpy

; ee = the function's own scope (its word 2: past this frame)
vm_outer:
            jsr         vm_self
            ldy         #2
            jsr         cell_get
            sta         ee
            stx         ee + 1
            rts

; ex = the symbol at .Y in the op
vm_symy:
            lda         vm_bank
            sta         $00
            lda         (vm_ip),y
            sta         ex
            iny
            lda         (vm_ip),y
            sta         ex + 1
            rts

; ---- Blocks (let, each, dotimes, try's handler): their scopes

; BLOCK fb k vb parent names: a block's scope begun (again, a loop's next step): its frame slot fb none, its k
; variables (slots from vb) holes; parent, the BLOCK of the block it's in (0: the function's); names, its variables'
; (vm_mkblk's)
op_block:
            ldy         #1
            lda         (vm_ip),y
            tay
            lda         #0
            sta         (vm_s),y
            iny
            sta         (vm_s),y
            ldy         #2
            lda         (vm_ip),y
            sta         vm_k
            beq         @done
            iny
            lda         (vm_ip),y
            tay
@hole:
            lda         #<UNBOUND
            sta         (vm_s),y
            iny
            lda         #>UNBOUND
            sta         (vm_s),y
            iny
            dec         vm_k
            bne         @hole
@done:
            ldy         #2
            lda         (vm_ip),y
            asl
            adc         #6
            jmp         vm_na

; .Y = a block variable's op's place of fb (y fb p): C = 1, hp, .Y = its pair in the block's frame (there's one); C = 0,
; .Y = its slot (there isn't)
vm_bvar:
            lda         (vm_ip),y                           ; (fb)
            tax
            iny
            lda         (vm_ip),y                           ; (p: its pair's value at 4p + 6)
            asl
            asl
            adc         #6
            sta         vm_k
            dey
            dey
            lda         (vm_ip),y                           ; (y)
            sta         vm_bj
            txa
            tay
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            bcc         @slot
            tax
            dey
            lda         (vm_s),y
            jsr         deref
            ldy         vm_k
            sec
            rts
@slot:
            ldy         vm_bj
            clc
            rts

; LOCALB y fb p sym: a block variable's value; a hole, sym's where the block is (lookup's: past the hole)
op_localb:
            ldy         #2                                  ; (The block's scope: none, its slot)
            lda         (vm_ip),y
            tay
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            bcs         :+
            ldy         #1
            lda         (vm_ip),y
            tay
            bra         @slot
:
            ldy         #2
            jsr         vm_bvar
            bcs         @pair
@slot:
            lda         (vm_s),y
            sta         ex
            iny
            lda         (vm_s),y
            sta         ex + 1
            bne         :+
            lda         ex
            cmp         #<UNBOUND
            beq         @hole
:
            jmp         vm_n6
@pair:
            lda         (hp),y
            sta         ex
            iny
            lda         (hp),y
            sta         ex + 1
@hole:
            lda         ex + 1
            bne         @ok
            lda         ex
            cmp         #<UNBOUND
            bne         @ok
            MOVW        vm_opa, vm_ip
            jsr         vm_scope
            ldy         #4
            jsr         vm_symy
            FARN        1, lookup
            lda         #6
            jmp         vm_valerr
@ok:
            jmp         vm_adv6

; SETLB y fb p: a block variable = ex (its pair, if the block has a frame); ex = NIL
op_setlb:
            ldy         #2                                  ; (The block's scope: none, its slot)
            lda         (vm_ip),y
            tay
            iny
            lda         (vm_s),y
            cmp         #IMM_PAGES
            bcs         :+
            ldy         #1
            lda         (vm_ip),y
            tay
            lda         ex
            sta         (vm_s),y
            iny
            lda         ex + 1
            sta         (vm_s),y
            stz         ex
            stz         ex + 1
            jmp         vm_n4
:
            ldy         #2
            jsr         vm_setlb
            bcs         :+                                  ; (A slot: the bank as it was)
            jmp         vm_n4
:
            jmp         vm_adv4
vm_setlb:
            jsr         vm_bvar
            bcs         @pair
            lda         ex
            sta         (vm_s),y
            iny
            lda         ex + 1
            sta         (vm_s),y
            bra         @nil
@pair:
            lda         ex
            sta         (hp),y
            iny
            lda         ex + 1
            sta         (hp),y
@nil:
            stz         ex
            stz         ex + 1
            rts

; SETBLB y fb p sym: set! of a block variable: SETLB's, unless it's a hole (SETBG's, past it)
op_setblb:
            ldy         #2
            jsr         vm_bvar
            bcs         @pair
            iny
            lda         (vm_s),y
            bne         @set
            dey
            lda         (vm_s),y
            cmp         #<UNBOUND
            bne         @set
            bra         @hole
@pair:
            iny
            lda         (hp),y
            bne         @set
            dey
            lda         (hp),y
            cmp         #<UNBOUND
            bne         @set
@hole:
            ldy         #4
            lda         #6
            jmp         vm_setbx
@set:
            lda         vm_bank
            sta         $00
            ldy         #2
            jsr         vm_setlb
            bcs         :+
            jmp         vm_n6
:
            jmp         vm_adv6

vm_adv6:
            lda         #6
            jmp         vm_adv

; DOTN: dotimes' count, ex: a fixnum, pushed, then 0 (its i); else the error ('dotimes': ... not ...), VM_R: returned
op_dotn:
            lda         ex
            lsr
            bcc         @bad
            lda         ex
            ldx         ex + 1
            VPUSH
            lda         #<FIX(0)
            ldx         #>FIX(0)
            VPUSH
            jmp         vm_adv1
@bad:
            FARN        1, dot_err
            bcc         :+
            MOVW        ex, nomem_err
:
            lda         #1
            jmp         vm_valerr

; DOTI t: [n i] on top: i < n, ex = i; else to t
op_doti:
            lda         sp
            sec
            sbc         #4
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #2                                  ; (i, less n: signed)
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
            bmi         :+
            ldy         #1
            jmp         vm_jumpy
:
            jmp         vm_n3

; DOTINC: the i on top one more
op_dotinc:
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
            jmp         vm_n1

; EACHL: each's list, ex: its items pushed (a list's; a string's characters, a list made); else the error ('each'
; ...), VM_R: returned
op_eachl:
            lda         ex
            ldx         ex + 1
            jsr         q_conses
            bcc         @push
            lda         ex
            ldx         ex + 1
            jsr         kind_of
            cmp         #PK_STRING
            bne         @bad
            FARN        2, str_chars
            bcs         @nomem
            lda         hv
            ldx         hv + 1
@push:
            VPUSH
            jmp         vm_adv1
@bad:
            FARN        1, each_err
            bcc         :+
@nomem:
            MOVW        ex, nomem_err
:
            lda         #1
            jmp         vm_valerr

; EACHI t: the items on top: none left, to t; else ex = the next, the rest on top
op_eachi:
            lda         sp
            sec
            sbc         #2
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            lda         (ht)
            ldy         #1
            ora         (ht),y
            bne         :+
            jmp         vm_jumpy
:
            lda         (ht),y
            tax
            lda         (ht)
            jsr         deref
            ldy         #2
            lda         (hp),y
            pha
            iny
            lda         (hp),y
            pha
            lda         (hp)
            sta         ex
            ldy         #1
            lda         (hp),y
            sta         ex + 1
            ldy         #1
            pla
            sta         (ht),y
            pla
            sta         (ht)
            jmp         vm_adv3

; EACHI's in native code (eachi's template): the items on top: C = 1 if none's left; else ex = the next, the rest
; on top, C = 0.  The code's bank in the window again
vm_eachn:
            lda         sp
            sec
            sbc         #2
            sta         ht
            lda         sp + 1
            sbc         #0
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            tax
            ora         (ht)
            bne         :+
            sec
            rts
:
            lda         (ht)
            jsr         deref
            ldy         #3
            lda         (hp),y
            pha
            dey
            lda         (hp),y
            pha
            dey
            lda         (hp),y
            sta         ex + 1
            lda         (hp)
            sta         ex
            pla
            sta         (ht)
            pla
            ldy         #1
            sta         (ht),y
            lda         vm_bank
            sta         $00
            clc
            rts

; DROP k: k words dropped; ex = NIL
op_drop:
            ldy         #1
            lda         (vm_ip),y
            jsr         drop
            stz         ex
            stz         ex + 1
            jmp         vm_adv2

; TRYE t: ex not an error: to t
op_trye:
            jsr         vm_iserr
            bcs         :+
            lda         vm_bank
            sta         $00
            ldy         #1
            jmp         vm_jumpy
:
            jmp         vm_adv3

; TRYV ye yc: ex, an error: slot ye = its message, slot yc = its code (try's handler's &err and &code, its block
; just begun); its message pushed
op_tryv:
            lda         ex
            ldx         ex + 1
            ldy         #1
            jsr         cell_get
            sta         vm_u
            stx         vm_u + 1
            lda         ex
            ldx         ex + 1
            ldy         #0
            jsr         cell_get
            sta         vm_t
            stx         vm_t + 1
            lda         vm_bank
            sta         $00
            ldy         #1
            lda         (vm_ip),y
            tay
            lda         vm_t
            sta         (vm_s),y
            iny
            lda         vm_t + 1
            sta         (vm_s),y
            ldy         #2
            lda         (vm_ip),y
            tay
            lda         vm_u
            sta         (vm_s),y
            iny
            lda         vm_u + 1
            sta         (vm_s),y
            lda         vm_t
            ldx         vm_t + 1
            VPUSH
            jmp         vm_adv3

; TRYH t: the handler's value, ex: a function, on (TRYF, CALL: given the message); else the message dropped, to t
op_tryh:
            lda         ex
            lsr
            bcs         @not
            ldx         ex + 1
            cpx         #IMM_PAGES
            bcs         @cell
            cpx         #>BUILTIN0
            bcs         @fn
            bra         @not
@cell:
            lda         pk,x
            cmp         #PK_FUNC
            beq         @fn
            cmp         #PK_PARTIAL
            beq         @fn
@not:
            lda         #1
            jsr         drop
            lda         vm_bank
            sta         $00
            ldy         #1
            jmp         vm_jumpy
@fn:
            jmp         vm_adv3

; TRYF: the message popped, the function (ex) pushed, then the message: a call of it, next
op_tryf:
            jsr         pop
            pha
            phx
            lda         ex
            ldx         ex + 1
            VPUSH
            plx
            pla
            VPUSH
            jmp         vm_adv1

; ---- The quick ops: fixnums here, else their built-in's code

; vm_a = the value popped, vm_b = ex (an op of 1 byte)
vm_pop2:
            lda         sp
            beq         @page
            sec
            sbc         #2
            sta         sp
            ldy         #1
            lda         (sp),y
            sta         vm_a + 1
            lda         (sp)
            sta         vm_a
            bra         :+
@page:
            jsr         pop
            sta         vm_a
            stx         vm_a + 1
:
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

; Not fixnums (or past one): the op's built-in on its values (vm_a, vm_b), the usual way; its error returned if the
; op's VM_R; then past the op (vm_fl bytes)
vm_fslow:
            lda         vm_bank
            sta         $00
            lda         (vm_ip)
            and         #$7F
            jsr         vm_qcall
            lda         (vm_ip)
            bpl         vm_fdone
            jsr         vm_iserr
            bcc         vm_fdone
            jmp         vm_ret
vm_fdone:
            lda         vm_fl
            jmp         vm_adv

; ex = quick op .A's built-in (VO_ADD ...: vo_bin, vo_n) on vm_a (and vm_b), the usual way (an error given: it, as
; the evaluator has it, errors stopping what they reach); the code's bank after
vm_qcall:
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
            MOVW        ex, vm_a                            ; (An error given: the value)
            jsr         vm_iserr
            bcs         @bank
            lda         vm_m
            cmp         #2
            bcc         :+
            MOVW        ex, vm_b
            jsr         vm_iserr
            bcs         @bank
:
            lda         vm_a
            ldx         vm_a + 1
            VPUSH
            lda         vm_m
            cmp         #2
            bcc         :+
            lda         vm_b
            ldx         vm_b + 1
            VPUSH
:
            jsr         vm_bcode
@bank:
            lda         vm_bank
            sta         $00
            rts

; The binary ones: (op a b), a pushed, b in ex
op_add:
            jsr         vm_pop2
            lda         vm_a
            and         vm_b
            lsr
            bcs         @lb844
            jmp         vm_fslow
@lb844:
            lda         vm_b
            and         #$FE
            clc
            adc         vm_a
            tax
            lda         vm_b + 1
            adc         vm_a + 1
            bvc         @lb843
            jmp         vm_fslow
@lb843:
vm_fix1:
            sta         ex + 1
            stx         ex
            jmp         vm_n1

op_sub:
            jsr         vm_pop2
            lda         vm_a
            and         vm_b
            lsr
            bcs         @lb842
            jmp         vm_fslow
@lb842:
            lda         vm_b
            and         #$FE
            sta         vm_k
            lda         vm_a
            sec
            sbc         vm_k
            tax
            lda         vm_a + 1
            sbc         vm_b + 1
            bvc         @lb841
            jmp         vm_fslow
@lb841:
            bra         vm_fix1

op_lt:
            jsr         vm_pop2
            jsr         vm_fx2
            jsr         vm_less
            bcs         vm_t1
            bra         vm_f1
op_gt:
            jsr         vm_pop2
            jsr         vm_fx2
            jsr         vm_more
            bcs         vm_t1
            bra         vm_f1
op_le:
            jsr         vm_pop2
            jsr         vm_fx2
            jsr         vm_more
            bcc         vm_t1
            bra         vm_f1
op_ge:
            jsr         vm_pop2
            jsr         vm_fx2
            jsr         vm_less
            bcc         vm_t1
            bra         vm_f1
op_neq:
            jsr         vm_pop2
            jsr         vm_fx2
            lda         vm_a
            cmp         vm_b
            bne         vm_f1
            lda         vm_a + 1
            cmp         vm_b + 1
            bne         vm_f1
vm_t1:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         vm_n1
vm_f1:
            stz         ex
            stz         ex + 1
            jmp         vm_n1

; vm_a and vm_b fixnums: back; else (its return dropped) the op's built-in's way
vm_fx2:
            lda         vm_a
            and         vm_b
            lsr
            bcc         :+
            rts
:
            pla
            pla
            jmp         vm_fslow

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

; The unary ones: ex
op_inc:
            lda         ex
            lsr
            bcc         vm_slow1
            lda         ex
            clc
            adc         #2
            tax
            lda         ex + 1
            adc         #0
            bvs         vm_slow1
            jmp         vm_fix1
op_dec:
            lda         ex
            lsr
            bcc         vm_slow1
            lda         ex
            sec
            sbc         #2
            tax
            lda         ex + 1
            sbc         #0
            bvs         vm_slow1
            jmp         vm_fix1
op_zerop:
            lda         ex
            lsr
            bcc         vm_slow1
            lda         ex
            cmp         #<FIX(0)
            bne         vm_f1
            lda         ex + 1
            bne         vm_f1
            bra         vm_t1
op_onep:
            lda         ex
            lsr
            bcc         vm_slow1
            lda         ex
            cmp         #<FIX(1)
            bne         vm_f1
            lda         ex + 1
            beq         @lb852
            jmp         vm_f1
@lb852:
            jmp         vm_t1
vm_slow1:
            jsr         vm_one
            jmp         vm_fslow

; With a constant: ex and the fixnum after the op
op_addc:
            lda         ex
            lsr
            bcc         vm_slow3
            ldy         #1
            lda         (vm_ip),y
            and         #$FE
            clc
            adc         ex
            tax
            iny
            lda         (vm_ip),y
            adc         ex + 1
            bvs         vm_slow3
vm_fix3:
            sta         ex + 1
            stx         ex
            jmp         vm_n3
op_subc:
            lda         ex
            lsr
            bcc         vm_slow3
            ldy         #1
            lda         (vm_ip),y
            and         #$FE
            sta         vm_k
            lda         ex
            sec
            sbc         vm_k
            tax
            iny
            lda         ex + 1
            sbc         (vm_ip),y
            bvs         vm_slow3
            bra         vm_fix3
vm_slow3:
            jsr         vm_cst2
            jmp         vm_fslow
op_ltc:
            lda         ex
            lsr
            bcc         vm_slow3
            jsr         vm_xltc
            bcs         vm_t3
            bra         vm_f3
op_gec:
            lda         ex
            lsr
            bcc         vm_slow3
            jsr         vm_xltc
            bcc         vm_t3
            bra         vm_f3
op_gtc:
            lda         ex
            lsr
            bcc         vm_slow3
            jsr         vm_cltx
            bcs         vm_t3
            bra         vm_f3
op_lec:
            lda         ex
            lsr
            bcc         vm_slow3
            jsr         vm_cltx
            bcc         vm_t3
            bra         vm_f3
op_eqc:
            lda         ex
            lsr
            bcc         vm_slow3
            ldy         #1
            lda         ex
            cmp         (vm_ip),y
            bne         vm_f3
            iny
            lda         ex + 1
            cmp         (vm_ip),y
            bne         vm_f3
vm_t3:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         vm_n3
vm_f3:
            stz         ex
            stz         ex + 1
            jmp         vm_n3

; OUT: C = 1 if ex < the constant (vm_xltc), the constant < ex (vm_cltx): fixnums, signed
vm_xltc:
            ldy         #1
            lda         ex
            cmp         (vm_ip),y
            iny
            lda         ex + 1
            sbc         (vm_ip),y
            bvc         :+
            eor         #$80
:
            asl
            rts
vm_cltx:
            ldy         #1
            lda         (vm_ip),y
            cmp         ex
            iny
            lda         (vm_ip),y
            sbc         ex + 1
            bvc         :+
            eor         #$80
:
            asl
            rts

; ---- EXT and the fused ops

; EXT x: the rare op x after it (vm_xops: VX_), as if it were here
op_ext:
            inc         vm_ip
            bne         :+
            inc         vm_ip + 1
:
            lda         (vm_ip)
            tax
            jmp         (vm_xops,x)

; The fused ops (the compiler's: a LOCAL and the quick op after it; an LPUSH, a LOCAL and a quick op of two; and a
; JF after them): s, the quick op (VO_ADD ... VO_EQC; bit 0, its value pushed), worked on fixnums here (vm_qf), else
; its built-in's way (vm_qcall); VM_R, as the quick op's was

; LQ s y c: ex = s of the argument at y (and c)
vm_lqg:
            jsr         vm_lqv
            jsr         vm_qdo
            bcs         vm_qret
            lda         #5
vm_qpush:
            pha
            lda         (vm_ip)                             ; (LQP, or s's bit 0: pushed)
            and         #$7F
            cmp         #VO_LQP
            beq         @push
            ldy         #1
            lda         (vm_ip),y
            lsr
            bcc         :+
@push:
            lda         ex
            ldx         ex + 1
            VPUSH
:
            pla
            jmp         vm_na
vm_qret:
            jmp         vm_ret

; LL s y1 y2: ex = s of the arguments at y1 and y2
vm_llg:
            jsr         vm_llv
            jsr         vm_qdo
            bcs         vm_qret
            lda         #4
            bra         vm_qpush

; JLQ s y c t: LQ's, then JF's (to t if ex is NIL or ())
vm_jlqg:
            jsr         vm_lqv
            jsr         vm_qdo
            bcs         vm_qret
            ldy         #5
; JF's: ex NIL or (), to the target at .Y; else on, past it
vm_qjf:
            lda         ex + 1
            bne         @no
            lda         ex
            beq         @to
            cmp         #<SNIL
            beq         @to
@no:
            iny
            iny
            tya
            jmp         vm_na
@to:
            jmp         vm_jumpy

; JLL s y1 y2 t: LL's, then JF's
vm_jllg:
            jsr         vm_llv
            jsr         vm_qdo
            bcs         vm_qret
            ldy         #4
            bra         vm_qjf

; JQ s c t: s of ex (and c), then JF's
vm_jqg:
            MOVW        vm_a, ex
            ldy         #2
            lda         (vm_ip),y
            sta         vm_b
            iny
            lda         (vm_ip),y
            sta         vm_b + 1
            ldy         #1
            lda         (vm_ip),y
            jsr         vm_qdo
            bcs         vm_qret
            ldy         #4
            bra         vm_qjf

; The fused ops, dispatched again on s (its bit 0 aside: lq_tab ...): code of their own for the usual ones (fixnums;
; else the generic code, from the start), the generic code (vm_lqg ...) for the rest
op_lq:
            ldy         #1
            lda         (vm_ip),y
            tax
            jmp         (lq_tab - VO_ADD,x)
op_lqp:
            ldy         #1
            lda         (vm_ip),y
            tax
            jmp         (lqp_tab - VO_ADD,x)
op_jlq:
            ldy         #1
            lda         (vm_ip),y
            tax
            jmp         (jlq_tab - VO_ADD,x)
op_jq:
            ldy         #1
            lda         (vm_ip),y
            tax
            jmp         (jq_tab - VO_ADD,x)
op_ll:
            ldy         #1
            lda         (vm_ip),y
            and         #$FE
            tax
            jmp         (ll_tab - VO_ADD,x)
op_jll:
            ldy         #1
            lda         (vm_ip),y
            tax
            jmp         (jll_tab - VO_ADD,x)

; vm_a = the argument at the op's byte 2, a fixnum (no scope made); else to slow
.macro LQLOAD slow
            ldy         #2
            lda         (vm_ip),y
            bit         vm_mat
            bmi         slow
            tay
            lda         (vm_s),y
            sta         vm_a
            lsr
            bcc         slow
            iny
            lda         (vm_s),y
            sta         vm_a + 1
.endmacro

; vm_a, vm_b = the arguments at the op's bytes 2 and 3, fixnums (no scope made); else to slow
.macro LLLOAD slow
            bit         vm_mat
            bmi         slow
            ldy         #3
            lda         (vm_ip),y
            tay
            lda         (vm_s),y
            sta         vm_b
            lsr
            bcc         slow
            iny
            lda         (vm_s),y
            sta         vm_b + 1
            ldy         #2
            lda         (vm_ip),y
            tay
            lda         (vm_s),y
            sta         vm_a
            lsr
            bcc         slow
            iny
            lda         (vm_s),y
            sta         vm_a + 1
.endmacro

; vm_a = ex, a fixnum; else to slow
.macro JQLOAD slow
            lda         ex
            sta         vm_a
            lsr
            bcc         slow
            lda         ex + 1
            sta         vm_a + 1
.endmacro

; N = 1 if vm_a < the fixnum at the op's byte k (CLT), the fixnum < vm_a (CGT); vm_a < vm_b (VLT), vm_b < vm_a (VGT)
.macro CLT k
            ldy         #k
            lda         vm_a
            cmp         (vm_ip),y
            iny
            lda         vm_a + 1
            sbc         (vm_ip),y
            bvc         :+
            eor         #$80
:
.endmacro
.macro CGT k
            ldy         #k
            lda         (vm_ip),y
            cmp         vm_a
            iny
            lda         (vm_ip),y
            sbc         vm_a + 1
            bvc         :+
            eor         #$80
:
.endmacro
.macro VLT
            lda         vm_a
            cmp         vm_b
            lda         vm_a + 1
            sbc         vm_b + 1
            bvc         :+
            eor         #$80
:
.endmacro
.macro VGT
            lda         vm_b
            cmp         vm_a
            lda         vm_b + 1
            sbc         vm_a + 1
            bvc         :+
            eor         #$80
:
.endmacro

; The JF's after a test: true, on past the op (vm_j7t: 7 bytes, vm_j6t: 6), ex = T; false, ex = NIL, to the target
; at byte 5 (vm_j7f) or 4 (vm_j6f)
vm_j7t:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            lda         #7
            jmp         vm_na
vm_j6t:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            jmp         vm_n6
vm_j7f:
            stz         ex
            stz         ex + 1
            ldy         #5
            jmp         vm_jumpy
vm_j6f:
            stz         ex
            stz         ex + 1
            ldy         #4
            jmp         vm_jumpy

; LQ's and LL's value, ex: pushed if s's bit 0 says; then on, past the op (.A bytes)
vm_lqend:
            pha
            ldy         #1
            lda         (vm_ip),y
            lsr
            bcc         :+
            lda         ex
            ldx         ex + 1
            VPUSH
:
            pla
            jmp         vm_na

lq_addc:
            LQLOAD       @g
            ldy         #3
            lda         (vm_ip),y
            and         #$FE
            clc
            adc         vm_a
            sta         ex
            iny
            lda         (vm_ip),y
            adc         vm_a + 1
            bvs         @g
            sta         ex + 1
            jmp         vm_n5
@g:
            jmp         vm_lqg

lq_subc:
            LQLOAD       @g
            ldy         #3
            lda         (vm_ip),y
            and         #$FE
            sta         vm_k
            lda         vm_a
            sec
            sbc         vm_k
            sta         ex
            iny
            lda         vm_a + 1
            sbc         (vm_ip),y
            bvs         @g
            sta         ex + 1
            jmp         vm_n5
@g:
            jmp         vm_lqg

lq_inc:
            LQLOAD       @g
            lda         vm_a
            clc
            adc         #2
            sta         ex
            lda         vm_a + 1
            adc         #0
            bvs         @g
            sta         ex + 1
            jmp         vm_n5
@g:
            jmp         vm_lqg

lq_dec:
            LQLOAD       @g
            lda         vm_a
            sec
            sbc         #2
            sta         ex
            lda         vm_a + 1
            sbc         #0
            bvs         @g
            sta         ex + 1
            jmp         vm_n5
@g:
            jmp         vm_lqg

lqp_addc:
            LQLOAD       @g
            ldy         #3
            lda         (vm_ip),y
            and         #$FE
            clc
            adc         vm_a
            sta         ex
            iny
            lda         (vm_ip),y
            adc         vm_a + 1
            bvs         @g
            sta         ex + 1
            lda         ex
            ldx         ex + 1
            VPUSH
            jmp         vm_n5
@g:
            jmp         vm_lqg

lqp_subc:
            LQLOAD       @g
            ldy         #3
            lda         (vm_ip),y
            and         #$FE
            sta         vm_k
            lda         vm_a
            sec
            sbc         vm_k
            sta         ex
            iny
            lda         vm_a + 1
            sbc         (vm_ip),y
            bvs         @g
            sta         ex + 1
            lda         ex
            ldx         ex + 1
            VPUSH
            jmp         vm_n5
@g:
            jmp         vm_lqg

lqp_inc:
            LQLOAD       @g
            lda         vm_a
            clc
            adc         #2
            sta         ex
            lda         vm_a + 1
            adc         #0
            bvs         @g
            sta         ex + 1
            lda         ex
            ldx         ex + 1
            VPUSH
            jmp         vm_n5
@g:
            jmp         vm_lqg

lqp_dec:
            LQLOAD       @g
            lda         vm_a
            sec
            sbc         #2
            sta         ex
            lda         vm_a + 1
            sbc         #0
            bvs         @g
            sta         ex + 1
            lda         ex
            ldx         ex + 1
            VPUSH
            jmp         vm_n5
@g:
            jmp         vm_lqg

ll_add:
            LLLOAD       @g
            lda         vm_b
            and         #$FE
            clc
            adc         vm_a
            sta         ex
            lda         vm_b + 1
            adc         vm_a + 1
            bvs         @g
            sta         ex + 1
            lda         #4
            jmp         vm_lqend
@g:
            jmp         vm_llg

ll_sub:
            LLLOAD       @g
            lda         vm_b
            and         #$FE
            sta         vm_k
            lda         vm_a
            sec
            sbc         vm_k
            sta         ex
            lda         vm_a + 1
            sbc         vm_b + 1
            bvs         @g
            sta         ex + 1
            lda         #4
            jmp         vm_lqend
@g:
            jmp         vm_llg

jlq_zerop:
            LQLOAD       @g
            lda         vm_a
            cmp         #<FIX(0)
            bne         :+
            lda         vm_a + 1
            bne         :+
            jmp         vm_j7t
:
            jmp         vm_j7f
@g:
            jmp         vm_jlqg

jq_zerop:
            JQLOAD       @g
            lda         vm_a
            cmp         #<FIX(0)
            bne         :+
            lda         vm_a + 1
            bne         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jqg

jlq_onep:
            LQLOAD       @g
            lda         vm_a
            cmp         #<FIX(1)
            bne         :+
            lda         vm_a + 1
            bne         :+
            jmp         vm_j7t
:
            jmp         vm_j7f
@g:
            jmp         vm_jlqg

jq_onep:
            JQLOAD       @g
            lda         vm_a
            cmp         #<FIX(1)
            bne         :+
            lda         vm_a + 1
            bne         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jqg

jlq_ltc:
            LQLOAD       @g
            CLT         3
            bpl         :+
            jmp         vm_j7t
:
            jmp         vm_j7f
@g:
            jmp         vm_jlqg

jq_ltc:
            JQLOAD       @g
            CLT         2
            bpl         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jqg

jlq_gtc:
            LQLOAD       @g
            CGT         3
            bpl         :+
            jmp         vm_j7t
:
            jmp         vm_j7f
@g:
            jmp         vm_jlqg

jq_gtc:
            JQLOAD       @g
            CGT         2
            bpl         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jqg

jlq_lec:
            LQLOAD       @g
            CGT         3
            bmi         :+
            jmp         vm_j7t
:
            jmp         vm_j7f
@g:
            jmp         vm_jlqg

jq_lec:
            JQLOAD       @g
            CGT         2
            bmi         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jqg

jlq_gec:
            LQLOAD       @g
            CLT         3
            bmi         :+
            jmp         vm_j7t
:
            jmp         vm_j7f
@g:
            jmp         vm_jlqg

jq_gec:
            JQLOAD       @g
            CLT         2
            bmi         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jqg

jlq_eqc:
            LQLOAD       @g
            ldy         #3
            lda         vm_a
            cmp         (vm_ip),y
            bne         :+
            iny
            lda         vm_a + 1
            cmp         (vm_ip),y
            bne         :+
            jmp         vm_j7t
:
            jmp         vm_j7f
@g:
            jmp         vm_jlqg

jq_eqc:
            JQLOAD       @g
            ldy         #2
            lda         vm_a
            cmp         (vm_ip),y
            bne         :+
            iny
            lda         vm_a + 1
            cmp         (vm_ip),y
            bne         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jqg

jll_lt:
            LLLOAD       @g
            VLT
            bpl         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jllg

jll_gt:
            LLLOAD       @g
            VGT
            bpl         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jllg

jll_le:
            LLLOAD       @g
            VGT
            bmi         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jllg

jll_ge:
            LLLOAD       @g
            VLT
            bmi         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jllg

jll_eq:
            LLLOAD       @g
            lda         vm_a
            cmp         vm_b
            bne         :+
            lda         vm_a + 1
            cmp         vm_b + 1
            bne         :+
            jmp         vm_j6t
:
            jmp         vm_j6f
@g:
            jmp         vm_jllg

; vm_a = the argument at the op's byte 2 (vm_lval), vm_b = the fixnum at bytes 3 and 4; .A = s
vm_lqv:
            ldy         #2
            jsr         vm_lval
            ldy         #3
            lda         (vm_ip),y
            sta         vm_b
            iny
            lda         (vm_ip),y
            sta         vm_b + 1
            ldy         #1
            lda         (vm_ip),y
            rts

; vm_a, vm_b = the arguments at the op's bytes 2 and 3; .A = s
vm_llv:
            ldy         #3
            jsr         vm_lval
            MOVW        vm_b, vm_a
            ldy         #2
            jsr         vm_lval
            ldy         #1
            lda         (vm_ip),y
            rts

; vm_a = the argument at the op's byte .Y (its slot; the scope's pair once there's one: vm_slot's, through ex); the
; code's bank in the window after
vm_lval:
            lda         (vm_ip),y
            bit         vm_mat
            bmi         @scope
            tay
            lda         (vm_s),y
            sta         vm_a
            iny
            lda         (vm_s),y
            sta         vm_a + 1
            rts
@scope:
            jsr         vm_slot
            MOVW        vm_a, ex
            lda         vm_bank
            sta         $00
            rts

; .A = s (bit 0 aside), vm_a (and vm_b) its values: ex = its value, fixnums here (vm_qf's), else its built-in's
; (vm_qcall).  The code's bank in the window after.  OUT: C = 1 if it's an error the op returns (its VM_R)
vm_qdo:
            and         #$FE
            sta         vm_qs
            sec
            sbc         #VO_ADD
            tax
            jsr         @go
            bcs         :+
            rts
:
            lda         vm_qs
            jsr         vm_qcall
            lda         (vm_ip)
            bpl         :+
            jmp         vm_iserr
:
            clc
            rts
@go:
            jmp         (vm_qf,x)

; The fixnum work (vm_qf's): ex its value.  OUT: C = 1 if it isn't fixnums, or it's past one
vf_add:
            lda         vm_a
            and         vm_b
            lsr
            bcc         vf_no
            lda         vm_b
            and         #$FE
            clc
            adc         vm_a
            tax
            lda         vm_b + 1
            adc         vm_a + 1
            bvs         vf_no
vf_val:
            sta         ex + 1
            stx         ex
            clc
            rts
vf_sub:
            lda         vm_a
            and         vm_b
            lsr
            bcc         vf_no
            lda         vm_b
            and         #$FE
            sta         vm_k
            lda         vm_a
            sec
            sbc         vm_k
            tax
            lda         vm_a + 1
            sbc         vm_b + 1
            bvs         vf_no
            bra         vf_val
vf_inc:
            lda         vm_a
            lsr
            bcc         vf_no
            lda         vm_a
            clc
            adc         #2
            tax
            lda         vm_a + 1
            adc         #0
            bvs         vf_no
            bra         vf_val
vf_dec:
            lda         vm_a
            lsr
            bcc         vf_no
            lda         vm_a
            sec
            sbc         #2
            tax
            lda         vm_a + 1
            sbc         #0
            bvs         vf_no
            bra         vf_val
vf_no:
            sec
            rts
vf_t:
            lda         #<T_VAL
            sta         ex
            stz         ex + 1
            clc
            rts
vf_f:
            stz         ex
            stz         ex + 1
            clc
            rts
vf_lt:
            jsr         vf_ab
            bcs         vf_no
            bmi         vf_t
            bra         vf_f
vf_ge:
            jsr         vf_ab
            bcs         vf_no
            bmi         vf_f
            bra         vf_t
vf_gt:
            jsr         vf_ba
            bcs         vf_no
            bmi         vf_t
            bra         vf_f
vf_le:
            jsr         vf_ba
            bcs         vf_no
            bmi         vf_f
            bra         vf_t
vf_eq:
            lda         vm_a
            and         vm_b
            lsr
            bcc         vf_no
            lda         vm_a
            cmp         vm_b
            bne         vf_f
            lda         vm_a + 1
            cmp         vm_b + 1
            bne         vf_f
            bra         vf_t
vf_zerop:
            lda         vm_a
            lsr
            bcc         vf_no
            lda         vm_a
            cmp         #<FIX(0)
            bne         vf_f
            lda         vm_a + 1
            bne         vf_f
            bra         vf_t
vf_onep:
            lda         vm_a
            lsr
            bcc         vf_no
            lda         vm_a
            cmp         #<FIX(1)
            bne         vf_f
            lda         vm_a + 1
            bne         vf_f
            bra         vf_t

; vm_a < vm_b (vf_ab), vm_b < vm_a (vf_ba), fixnums, signed.  OUT: C = 1 if they aren't fixnums; else N = 1 if it holds
vf_ab:
            lda         vm_a
            and         vm_b
            lsr
            bcc         :+
            lda         vm_a
            cmp         vm_b
            lda         vm_a + 1
            sbc         vm_b + 1
            bvc         vf_sign
            eor         #$80
vf_sign:
            clc                                             ; (N kept)
            rts
:
            sec
            rts
vf_ba:
            lda         vm_a
            and         vm_b
            lsr
            bcc         :+
            lda         vm_b
            cmp         vm_a
            lda         vm_b + 1
            sbc         vm_a + 1
            bvc         vf_sign
            eor         #$80
            bra         vf_sign
:
            sec
            rts

; ---- The compiler (in the sixth bank, the collector's: FARN 6, vm_compile)

.segment "CODE6"

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
            stz         vc_k
            stz         vc_btot
            stz         vc_tgt
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
            jsr         vc_locals                           ; (Its locals: set's names)
            jsr         vc_scratch                          ; (Its scratch banks)
            bcc         :+
            jmp         vc_full
:
            lda         ar_n                                ; (The arena: its first bank)
            bne         vc_again
            jsr         vc_bank
            bcc         vc_again
            jmp         vc_full
vc_again:
            lda         vm_sb                               ; (In the scratch bank, from its start: vmx.inc makes it
            sta         vc_bk                               ;   native code, in the arena)
            stz         vc_idx
            stz         hq
            stz         vc_start
            lda         #$80
            sta         hq + 1
            sta         vc_start + 1
            stz         vc_nlab
            stz         vc_lv
            stz         vc_lend + 1
            stz         vc_l2ok
            stz         vc_cq
            stz         vc_nblk
            stz         vc_ntbl
            stz         vc_bused
            stz         vc_bneed
            lda         vc_n                                ; (Its frame: 3 + n words, then its scope's slot and
            clc                                             ;   kt, if it has locals or blocks)
            adc         #3
            sta         vc_h
            lda         vc_k
            clc
            adc         vc_btot
            beq         @lnamed
            sec
            adc         vc_h
            sta         vc_h
            cmp         #VC_HMAX
            bcc         :+
            jmp         vc_fail
:
            lda         #VO_LOCALS                          ; (LOCALS kt kn tbl names)
            jsr         vc_byte
            lda         vc_k
            clc
            adc         vc_btot
            jsr         vc_byte
            lda         vc_k
            jsr         vc_byte
            lda         #0
            tax
            jsr         vc_word
            stz         vc_x
@lname:
            ldx         vc_x
            cpx         vc_k
            bcs         @lnamed
            lda         vc_ls_lo,x
            pha
            lda         vc_ls_hi,x
            tax
            pla
            jsr         vc_word
            inc         vc_x
            bra         @lname
@lnamed:
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
            sta         L_st
            stz         vc_da
            stz         vc_da + 1
            lda         #M_R
            sta         L_mode
            jsr         vc_call
@ret:
            lda         #VO_RET
            jsr         vc_byte
            lda         vc_btot                             ; (Its blocks' slots more than kept: again, keeping them)
            cmp         vc_bneed
            bcs         :+
            lda         vc_bneed
            sta         vc_btot
            jmp         vc_again
:
            lda         hq                                  ; (Its code's end: the translator's)
            sta         vx_end
            lda         hq + 1
            sta         vx_end + 1
            lda         vc_ntbl
            sta         vx_ntbl
            lda         vc_ntbl                             ; (Its blocks' table: after its code, LOCALS' tbl)
            beq         @notbl
            lda         vc_bk
            sta         $00
            lda         vc_start
            sta         ht
            lda         vc_start + 1
            sta         ht + 1
            ldy         #3
            lda         hq
            sta         (ht),y
            iny
            lda         hq + 1
            sta         (ht),y
            lda         vc_ntbl
            jsr         vc_byte
            stz         vc_x
@entry:
            ldx         vc_x
            cpx         vc_ntbl
            bcs         @notbl
            lda         vt_slo,x
            pha
            lda         vt_shi,x
            tax
            pla
            jsr         vc_word
            ldx         vc_x
            lda         vt_elo,x
            pha
            lda         vt_ehi,x
            tax
            pla
            jsr         vc_word
            inc         vc_x
            bra         @entry
@notbl:
            FARN        8, vm_xlate                         ; (Native code, in the arena: its place the function's
            bcc         :+                                  ;   word 4)
            cmp         #0
            beq         @full
            jmp         vc_novm
@full:
            sec
            rts
:
            lda         cf
            ldx         cf + 1
            jsr         deref
            ldy         #8
            lda         vx_place
            sta         (hp),y
            iny
            lda         vx_place + 1
            sta         (hp),y
            clc
            rts

; The scratch banks (vm_sb, vm_mb), taken at the first function compiled.  OUT: C = 1 if there's no room
vc_scratch:
            lda         vm_sb
            bne         @ok
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @rts
            sta         vm_mb
            lda         #1
            jsr         BANKS_ALLOC
            bcs         @rts
            sta         vm_sb
@ok:
            clc
@rts:
            rts

; The scratch filled (4K: the translator's map's room): the function's always evaluated
vc_over:
            ldx         vc_s
            txs
            jmp         vc_novm

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
            cpy         #$90
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
            stz         vc_lend + 1                         ; (A label here: nothing joins the op before it)
            stz         vc_l2ok
            rts

; PUSH emitted: the frame a word higher (joined to the op just before it, if it can be: a LOCAL, a CONST: LPUSH,
; CPUSH; LQ: LQP; LL: s's bit 0)
vc_push:
            jsr         vc_last
            bcc         @push
            cmp         #VO_LOCAL
            bne         :+
            lda         #VO_LPUSH
            sta         (ht)
            bra         vc_hinc
:
            cmp         #VO_CONST
            bne         :+
            lda         #VO_CPUSH
            sta         (ht)
            bra         vc_hinc
:
            cmp         #VO_LQ
            bne         :+
            lda         (ht)                                ; (LQ: LQP)
            and         #VM_R
            ora         #VO_LQP
            sta         (ht)
            stz         vc_lend + 1
            bra         vc_hinc
:
            cmp         #VO_LL
            bne         @push
            ldy         #1                                  ; (LL: s's bit 0)
            lda         (ht),y
            ora         #1
            sta         (ht),y
            stz         vc_lend + 1                         ; (Nothing more joins it)
            bra         vc_hinc
@push:
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
            stz         vc_isl
            ldx         vc_nblk                             ; (The blocks open: the innermost first)
@blk:
            dex
            bmi         @fn
            txa
            asl
            asl
            asl
            asl
            sta         vc_bi
            ldy         #0
@bj:
            tya
            cmp         vb_k,x
            bcs         @blk
            phy
            clc
            adc         vc_bi
            tay
            lda         vb_nlo,y
            cmp         vc_t
            bne         @bno
            lda         vb_nhi,y
            cmp         vc_t + 1
            bne         @bno
            ply                                             ; (Block x's variable y: its slot vb + 2y, pair y)
            lda         #2
            sta         vc_isl
            lda         vb_fb,x
            sta         vc_bfb
            sty         vc_bp
            tya
            asl
            clc
            adc         vb_vb,x
            clc
            rts
@bno:
            ply
            iny
            bra         @bj
@fn:
            ldy         #0
@k:
            cpy         vc_n
            bcs         @locals
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
@locals:
            ldy         #0
@j:
            cpy         vc_k
            bcs         @no
            lda         vc_ls_lo,y
            cmp         vc_t
            bne         @jnext
            lda         vc_ls_hi,y
            cmp         vc_t + 1
            beq         @local
@jnext:
            iny
            bra         @j
@local:
            inc         vc_isl                              ; (A local j: at rb + 6 + 2j)
            tya
            asl
            adc         vc_rb
            adc         #6
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
            lda         vc_cq
            sta         L_q,x
            stz         vc_cq
            lda         vc_cst
            sta         L_st,x
            stz         vc_cst
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
            jsr         vc_mark                             ; (Anything else: itself)
            lda         #VO_CONST
            jsr         vc_byte
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            jsr         vc_marked
            jmp         @done
@imm:
            cmp         #0                                  ; (() is NIL)
            bne         @const
            lda         L_elo,x
            cmp         #<SNIL
            bne         @const
            jsr         vc_mark
            lda         #VO_CONST
            jsr         vc_byte
            lda         #0
            tax
            jsr         vc_word
            jsr         vc_marked
            jmp         @done
@call:
            jsr         vc_call
            jmp         @done
@quote:
            lda         L_q,x                               ; (A Q-expression that's code: while's)
            beq         :+
            jsr         vc_call
            jmp         @done
:
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
            jmp         @done
@sym:
            lda         L_elo,x                             ; (A name: a formal's argument; else its value, a
            pha                                             ;   global's, or looked up in the function's scope)
            lda         L_ehi,x
            tax
            pla
            jsr         vc_formal
            bcs         @free
            ldy         vc_isl
            bne         @local
            pha
            jsr         vc_mark
            lda         #VO_LOCAL
            jsr         vc_byte
            pla
            jsr         vc_byte
            jsr         vc_marked
            jmp         @done
@local:
            cpy         #2
            beq         @blkvar
            jsr         vc_dtest                            ; (Set by a statement before: LOCAL, then LOCALH's JE)
            beq         :+
            pha
            jsr         vc_mark
            lda         #VO_LOCAL
            jsr         vc_byte
            pla
            jsr         vc_byte
            jsr         vc_marked
            jsr         vc_je
            jmp         @done
:
            pha                                             ; (A local: LOCALH, its name for a hole)
            lda         #VO_LOCALH
            jsr         vc_rop
            pla
            jsr         vc_byte
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            jsr         vc_je
            bra         @done
@blkvar:
            pha                                             ; (A block's variable: LOCALB y fb p sym)
            lda         #VO_LOCALB
            jsr         vc_rop
            pla
            jsr         vc_byte
            lda         vc_bfb
            jsr         vc_byte
            lda         vc_bp
            jsr         vc_byte
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            jsr         vc_je
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

; vc_lop = here, an op's place (vc_mark: the op before it, if it ends here, kept as vc_lop2, vc_l2ok); vc_lend =
; here, past it (vc_marked): the next may join it (vc_push, vc_qop, vc_qcop, vc_jf)
vc_mark:
            stz         vc_l2ok
            lda         vc_lend + 1
            beq         @set
            cmp         hq + 1
            bne         @set
            lda         vc_lend
            cmp         hq
            bne         @set
            lda         vc_lop
            sta         vc_lop2
            lda         vc_lop + 1
            sta         vc_lop2 + 1
            inc         vc_l2ok
@set:
            lda         hq
            sta         vc_lop
            lda         hq + 1
            sta         vc_lop + 1
            stz         vc_lend + 1
            rts
vc_marked:
            lda         hq
            sta         vc_lend
            lda         hq + 1
            sta         vc_lend + 1
            rts

; OUT: C = 1 if the op at vc_lop ends here (nothing since, no label): .A its op (VM_R aside), ht its place (its bank
; in the window)
vc_last:
            lda         vc_lend + 1
            beq         @no
            cmp         hq + 1
            bne         @no
            lda         vc_lend
            cmp         hq
            bne         @no
            lda         vc_lop
            sta         ht
            lda         vc_lop + 1
            sta         ht + 1
            lda         vc_bk
            sta         $00
            lda         (ht)
            and         #$7F
            sec
            rts
@no:
            clc
            rts

; hq = vc_lop (the op there written again, as another)
vc_rewind:
            lda         vc_lop
            sta         hq
            lda         vc_lop + 1
            sta         hq + 1
            rts

; A rare op .A emitted after EXT (vc_xbyte; vc_xrop: VM_R as this level wants)
vc_xbyte:
            pha
            lda         #VO_EXT
            jsr         vc_byte
            pla
            jmp         vc_byte
vc_xrop:
            pha
            lda         #VO_EXT
            jsr         vc_byte
            pla
            jmp         vc_rop

; Quick op .A (VO_ADDC ... VO_EQC) emitted with its fixnum vc_v (VM_R as this level wants), joined to a LOCAL just
; before it: LQ s y c
vc_qcop:
            sta         vc_qo
            jsr         vc_last
            bcc         @plain
            cmp         #VO_LOCAL
            bne         @plain
            ldy         #1
            lda         (ht),y
            sta         vc_qy
            jsr         vc_rewind
            lda         #VO_LQ
            jsr         vc_rop
            lda         vc_qo
            jsr         vc_byte
            lda         vc_qy
            jsr         vc_byte
            bra         @c
@plain:
            jsr         vc_mark
            lda         vc_qo
            jsr         vc_rop
@c:
            lda         vc_v
            ldx         vc_v + 1
            jsr         vc_word
            jmp         vc_marked

; Quick op .A (VO_ADD ... VO_ONEP) emitted (VM_R as this level wants), joined to a LOCAL just before it (LQ s y 0),
; or, of two values, to an LPUSH and a LOCAL (LL s y1 y2)
vc_qop:
            sta         vc_qo
            jsr         vc_last
            bcc         @plain
            cmp         #VO_LOCAL
            bne         @plain
            ldy         #1
            lda         (ht),y
            sta         vc_qy
            lda         vc_qo
            cmp         #VO_INC                             ; (One value: LQ)
            bcs         @lq
            lda         vc_l2ok                             ; (Two: an LPUSH just before)
            beq         @plain
            lda         vc_lop2
            sta         ht
            lda         vc_lop2 + 1
            sta         ht + 1
            lda         (ht)
            cmp         #VO_LPUSH
            bne         @plain
            ldy         #1
            lda         (ht),y
            sta         vc_qy2
            MOVW        vc_lop, vc_lop2
            stz         vc_l2ok
            jsr         vc_rewind
            lda         #VO_LL
            jsr         vc_rop
            lda         vc_qo
            jsr         vc_byte
            lda         vc_qy2
            jsr         vc_byte
            lda         vc_qy
            jsr         vc_byte
            jmp         vc_marked
@lq:
            jsr         vc_rewind
            lda         #VO_LQ
            jsr         vc_rop
            lda         vc_qo
            jsr         vc_byte
            lda         vc_qy
            jsr         vc_byte
            lda         #0
            tax
            jsr         vc_word
            jmp         vc_marked
@plain:
            jsr         vc_mark
            lda         vc_qo
            jsr         vc_rop
            jmp         vc_marked

; JF to label .A emitted, joined to the op just before it if it can be (LQ, LL: JLQ, JLL; a quick op on ex alone,
; or with a constant: JQ s c)
vc_jf:
            sta         vc_qt
            jsr         vc_last
            bcc         @plain
            cmp         #VO_LQ
            beq         @lq
            cmp         #VO_LL
            beq         @ll
            cmp         #VO_INC                             ; (INC ... ONEP, ADDC ... EQC: JQ)
            bcc         @plain
            cmp         #VO_LPUSH
            bcs         @plain
            sta         vc_qo
            lda         (ht)
            and         #VM_R
            sta         vc_qr
            stz         vc_qc
            stz         vc_qc + 1
            lda         vc_qo
            cmp         #VO_ADDC
            bcc         :+
            ldy         #1
            lda         (ht),y
            sta         vc_qc
            iny
            lda         (ht),y
            sta         vc_qc + 1
:
            jsr         vc_rewind
            lda         #VO_JQ
            ora         vc_qr
            jsr         vc_byte
            lda         vc_qo
            jsr         vc_byte
            lda         vc_qc
            ldx         vc_qc + 1
            jsr         vc_word
            bra         @t
@lq:
            lda         #VO_JLQ
            bra         :+
@ll:
            lda         #VO_JLL
:
            sta         vc_qo
            ldy         #1                                  ; (Not pushed)
            lda         (ht),y
            lsr
            bcs         @plain
            lda         (ht)
            and         #VM_R
            ora         vc_qo
            sta         (ht)
            bra         @t
@plain:
            lda         #VO_JF
            jsr         vc_byte
@t:
            stz         vc_lend + 1
            lda         vc_qt
            jmp         vc_ref

; Label .A defined here, a forward one (nothing refers to it once it's defined): if nothing refers to it yet, the
; ops on either side of it may be joined
vc_deff:
            tax
            lda         lb_lo,x
            ora         lb_hi,x
            bne         :+
            lda         hq
            sta         lb_lo,x
            lda         hq + 1
            sta         lb_hi,x
            lda         #1
            sta         lb_def,x
            rts
:
            txa
            jmp         vc_def

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
            lda         vc_bflags,y
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
            cpy         #BIN_LET                            ; (let, each, dotimes, try: blocks)
            bne         :+
            jsr         vc_inl
            jmp         vc_let
:
            cpy         #BIN_EACH
            bne         :+
            jsr         vc_inl
            jmp         vc_each
:
            cpy         #BIN_DOTIMES
            bne         :+
            jsr         vc_inl
            jmp         vc_dotimes
:
            cpy         #BIN_TRY
            bne         :+
            jsr         vc_inl
            jmp         vc_try
:
            cpy         #BIN_SET
            beq         @set
            cpy         #BIN_SETB
            beq         @set
            cpy         #BIN_DEF
            beq         @set
            cpy         #BIN_WHILE
            bne         :+
            cmp         #1
            bcc         @eval
            jsr         vc_inl
            jmp         vc_while
@set:
            cmp         #1
            bcc         @eval
            jsr         vc_inl
            jmp         vc_set
:
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
            stz         L_b,x
            jsr         vc_ghead                            ; (A global's name: SHEAD; the function's own: CSELF)
            bcc         @head
            jsr         vc_self
            ldx         vc_lv
            rol         L_b,x
            lda         hq                                  ; (Its place: vc_shflag's)
            sta         L_shl,x
            lda         hq + 1
            sta         L_shh,x
            lda         #VO_SHEAD
            jsr         vc_rop
            jsr         vc_mct
            lda         vc_hd
            ldx         vc_hd + 1
            jsr         vc_word
            lda         #0                                  ; (Its cache: none yet, epoch 0)
            tax
            jsr         vc_word
            jsr         vc_word
            jmp         @args
@head:
            lda         #M_OK                               ; (Its function: an error's for HEAD)
            sta         vc_cm
            ldx         vc_lv
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
            jsr         vc_mct
@args:
            jsr         vc_hinc
            ldx         vc_lv                               ; (Its arguments: an error's the call's value)
            lda         L_t,x
            jsr         vc_amode
            jsr         vc_args
            ldx         vc_lv
            lda         L_b,x
            beq         @call
            lda         L_tail,x                            ; (CSELF m code h r; TSELF m code 0)
            beq         :+
            jsr         vc_shflag
            lda         #VO_TSELF
            jsr         vc_byte
            jsr         vc_mcode
            lda         #0
            jsr         vc_byte
            bra         @t
:
            lda         #VO_CSELF
            jsr         vc_byte
            jsr         vc_mcode
            lda         #4
            jsr         vc_retw
            bra         @hr
@call:
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
            lda         L_tail,x
            bne         @hr
            lda         #12
            jsr         vc_retw
            jsr         vc_ccache
@hr:
            ldx         vc_lv
            lda         L_h0,x
            jsr         vc_byte
            jsr         vc_rbyte
@t:
            ldx         vc_lv
            lda         L_h0,x
            sta         vc_h
            lda         L_t,x
            jsr         vc_def
            jmp         vc_je

; A tail call of the function by its own name (this level's): its SHEAD flagged (m's bit 7) if its arguments' code,
; from past it to hq, calls nothing (vc_pure's ops alone, and BCALL of *): its head need not be pushed (vmx.inc's
; shself and tselfh), as nothing can bind its name again before TSELF
vc_shflag:
            ldx         vc_lv
            lda         L_shl,x
            clc
            adc         #12
            sta         ht
            lda         L_shh,x
            adc         #0
            sta         ht + 1
            lda         vc_bk
            sta         $00
@op:
            lda         ht
            cmp         hq
            lda         ht + 1
            sbc         hq + 1
            bcs         @yes
            lda         (ht)
            and         #$7F
            cmp         #VO_BCALL
            bne         :+
            ldy         #1
            lda         (ht),y
            cmp         #BIN_MUL
            bne         @no
            lda         #5
            bra         @next
:
            lsr
            tax
            lda         vc_pure,x
            beq         @no
@next:
            clc
            adc         ht
            sta         ht
            bcc         @op
            inc         ht + 1
            bra         @op
@yes:
            ldx         vc_lv
            lda         L_shl,x
            sta         ht
            lda         L_shh,x
            sta         ht + 1
            ldy         #1
            lda         (ht),y
            ora         #$80
            sta         (ht),y
@no:
            rts

; CALL's cache emitted: none yet (fn, epoch 0, code, bank, idx)
vc_ccache:
            ldx         #8
:
            lda         #0
            jsr         vc_byte
            dex
            bne         :-
            rts

; A call's return emitted: the place (a fixnum, vm_encode's) of the code .A bytes on (the call's end: its return,
; CALL's cache, h and r)
vc_retw:
            clc
            adc         hq
            sta         vc_t
            lda         hq + 1
            adc         #0
            and         #$1F
            ora         vc_idx
            tax
            lda         vc_t
            ora         #1
            jmp         vc_word

; HEAD's operands: m (the call's arguments), c (the call), t (its label)
vc_mct:
            ldx         vc_lv
            lda         L_m,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_elo,x
            pha
            lda         L_ehi,x
            tax
            pla
            jsr         vc_word
            ldx         vc_lv
            lda         L_t,x
            jmp         vc_ref

; CSELF's and TSELF's: m, the function's code's place
vc_mcode:
            ldx         vc_lv
            lda         L_m,x
            jsr         vc_byte
            lda         vc_start
            ldx         vc_start + 1
            jmp         vc_word

; OUT: C = 1 if this level's call's function (vc_hd) is a global's name: the function has no scope, and it's a
; symbol, not a formal (SHEAD's)
vc_ghead:
            lda         vc_snil
            bne         @no
            lda         vc_hd
            lsr
            bcs         @no
            ldx         vc_hd + 1
            cpx         #IMM_PAGES
            bcc         @no
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @no
            lda         vc_hd
            jmp         vc_formal                           ; (Not a formal: C = 1)
@no:
            clc
            rts

; OUT: C = 1 if this level's call is of the function compiled by its own name (vc_ghead's, its global value cf), as
; many arguments as its formals
vc_self:
            ldx         vc_lv
            lda         L_m,x
            cmp         vc_n
            bne         @no
            lda         vc_hd
            ldx         vc_hd + 1
            jsr         deref
            ldy         #2
            lda         (hp),y
            cmp         cf
            bne         @no
            iny
            lda         (hp),y
            cmp         cf + 1
            bne         @no
            sec
            rts
@no:
            clc
            rts

; A built-in's call (L_b): its arguments pushed (taken as errors too, if it takes them), BCALL
vc_bcall:
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_t,x
            ldy         L_b,x
            lda         vc_bflags,y
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
            jsr         vc_qok
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
            lda         vc_cm                               ; (Its second: its error the op's value, as a JE has it)
            cmp         #M_J
            bne         :+
            lda         #M_OK
            sta         vc_cm
:
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            dec         vc_h
@op:
            ldx         vc_lv
            lda         L_b,x
            jsr         vc_qop
            bra         @end
@const:
            ldx         vc_lv
            lda         L_b,x
            clc
            adc         #VO_ADDC - VO_ADD
            jsr         vc_qcop
@end:
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_deff
            jmp         vc_je

; A quick op's first argument: M_J's jump left out (M_OK) if the second is a name or a fixnum, or there's none (the
; op given its error has it as its value, vm_qcall's, the second evaluated or not: the same)
vc_qok:
            lda         vc_cm
            cmp         #M_J
            bne         @rts
            ldx         vc_lv
            lda         L_m,x
            cmp         #2
            bcc         @ok
            lda         L_alo,x                             ; (The second: its rest's first)
            pha
            lda         L_ahi,x
            tax
            pla
            jsr         vc_split
            lda         vc_q
            ldx         vc_q + 1
            jsr         vc_split
            pha
            lsr
            pla
            bcs         @ok
            cpx         #IMM_PAGES
            bcc         @rts
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @rts
@ok:
            lda         #M_OK
            sta         vc_cm
@rts:
            rts

; The evaluator's (EVAL): the expression evaluated in this frame's scope
vc_eval:
            lda         #VX_EVAL
            jsr         vc_xbyte
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
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_jf
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
            jsr         vc_stmt
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            bra         @next
@last:
            jsr         vc_same
            jsr         vc_stmt
            jsr         vc_nextt
            ldx         vc_lv
            lda         L_l2,x
            jmp         vc_def

; The next part a statement (vc_cst), if this level's a statement and returns its errors (a do's parts)
vc_stmt:
            ldx         vc_lv
            lda         L_st,x
            beq         :+
            lda         L_mode,x
            cmp         #M_R
            bne         :+
            lda         #1
            sta         vc_cst
:
            rts

; .A = a local's place (rb + 6 + 2j): its bit in vc_da set (vc_dset), tested (vc_dtest: Z = 0 if it's set).  Keep .A
vc_dset:
            pha
            jsr         vc_dbit
            ora         vc_da,x
            sta         vc_da,x
            pla
            rts
vc_dtest:
            pha
            jsr         vc_dbit
            and         vc_da,x
            tax
            pla
            cpx         #0
            rts
vc_dbit:
            sec
            sbc         vc_rb
            sec
            sbc         #6
            lsr
            tax
            and         #7
            tay
            txa
            lsr
            lsr
            lsr
            tax
            lda         vc_bits,y
            rts

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
            cmp         #VO_JF                              ; (and's JF: joined to the test, vc_jf)
            bne         :+
            lda         L_l2,x
            jsr         vc_jf
            bra         @next
:
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

; ---- Locals, set, set!, def, while (the compiler's)

; The function's locals: each name its body binds with set, =(...) (a symbol, or {symbols}, after set), not a formal,
; VC_FORMALS less its formals at most (more are bound by the evaluator: EVAL): vc_ls, vc_k
vc_locals:
            stz         vc_sd
            lda         cf
            ldx         cf + 1
            ldy         #1
            jsr         cell_get
; IN: .A, .X = a value: a list's items each scanned in turn (VC_SYMS deep), and a set's names added
vc_scan:
            pha
            lsr
            pla
            bcs         @rts
            cpx         #IMM_PAGES
            bcc         @rts
            ldy         pk,x
            cpy         #PK_SCONS
            beq         @list
            cpy         #PK_QCONS
            bne         @rts
@list:
            inc         vc_sd
            ldy         vc_sd
            cpy         #VC_SYMS
            bcs         @out
            pha
            phx
            jsr         vc_isset
            plx
            pla
@item:
            pha
            phx
            ldy         #0
            jsr         cell_get
            jsr         vc_scan
            plx
            pla
            ldy         #1
            jsr         cell_get
            sta         vc_t
            stx         vc_t + 1
            ora         vc_t + 1
            beq         @out
            lda         vc_t
            bra         @item
@out:
            dec         vc_sd
@rts:
            rts

; IN: .A, .X = a list: (set names ...)?  Its names added (vc_addl)
vc_isset:
            jsr         vc_split
            pha
            lsr
            pla
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            ldy         pk,x
            cpy         #PK_SYMBOL
            bne         @no
            jsr         deref                               ; (Its value set's built-in)
            ldy         #2
            lda         (hp),y
            cmp         #<(BUILTIN0 + 2 * BIN_SET)
            bne         @no
            iny
            lda         (hp),y
            cmp         #>(BUILTIN0 + 2 * BIN_SET)
            bne         @no
            lda         vc_q
            ora         vc_q + 1
            beq         @no
            lda         vc_q
            ldx         vc_q + 1
            jsr         vc_split                            ; (Its names: a symbol, or {symbols})
            pha
            lsr
            pla
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            ldy         pk,x
            cpy         #PK_SYMBOL
            beq         vc_addl
            cpy         #PK_QCONS
            bne         @no
@each:
            jsr         vc_split
            jsr         vc_addl
            lda         vc_q
            ldx         vc_q + 1
            bne         @each
            cmp         #0
            bne         @each
@no:
            rts

; IN: .A, .X = a value: a symbol, not a formal or a local yet, a local more (if there's room).  Keeps vc_q
vc_addl:
            ldy         vc_tgt                              ; (A block's names being collected: vc_cadd's)
            beq         :+
            jmp         vc_cadd
:
            pha
            lsr
            pla
            bcs         @no
            cpx         #IMM_PAGES
            bcc         @no
            ldy         pk,x
            cpy         #PK_SYMBOL
            bne         @no
            pha
            phx
            jsr         vc_formal
            plx
            pla
            bcc         @no
            pha
            lda         vc_n
            clc
            adc         vc_k
            cmp         #VC_FORMALS
            pla
            bcs         @no
            ldy         vc_k
            sta         vc_ls_lo,y
            txa
            sta         vc_ls_hi,y
            inc         vc_k
@no:
            rts

; (set names v...), (set! names v...), (def names v...): names a symbol or {symbols}, as many values; each value (an
; error too) bound in turn: set's, a formal's or a local's (SETL); set!'s where it's bound nearest (a formal's: SETL; a
; local's: SETBL; else SETBG); def's globally (DEFG).  Anything else (and set! taking errors) the evaluator's
vc_set:
            ldx         vc_lv
            lda         L_b,x
            cmp         #BIN_SETB
            bne         :+
            lda         L_mode,x
            cmp         #M_OK
            beq         @eval
:
            lda         L_alo,x
            pha
            lda         L_ahi,x
            tax
            pla
            jsr         vc_split                            ; (Its names; the values after)
            ldy         vc_lv
            sta         L_nlo,y
            txa
            sta         L_nhi,y
            lda         vc_q
            sta         L_alo,y
            lda         vc_q + 1
            sta         L_ahi,y
            jsr         vc_ncheck
            bcs         @eval
            sta         vc_t
            ldx         vc_lv
            lda         L_m,x
            dec         a
            cmp         vc_t
            bne         @eval
            lda         vc_t
            bne         @each
            jmp         vc_nil
@eval:
            jmp         vc_eval
@each:
            ldx         vc_lv
            lda         L_alo,x
            ora         L_ahi,x
            beq         @done
            lda         #M_OK                               ; (Its value, an error too)
            sta         vc_cm
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            jsr         vc_nname
            jsr         vc_bind
            bra         @each
@done:
            rts

; OUT: C = 0, .A = this level's names counted (L_nlo/hi: a symbol, or a list of them; set's each a formal or a
; local); C = 1 if they aren't
vc_ncheck:
            ldx         vc_lv
            lda         L_nlo,x
            sta         vc_v
            lda         L_nhi,x
            sta         vc_v + 1
            ora         vc_v
            beq         @none
            ldx         vc_v + 1
            cpx         #IMM_PAGES
            bcc         @bad
            lda         vc_v
            lsr
            bcs         @bad
            lda         pk,x
            cmp         #PK_SYMBOL
            bne         @list
            lda         vc_v
            jsr         vc_nok
            bcs         @bad
            lda         #1
            rts
@list:
            cmp         #PK_QCONS
            bne         @bad
            stz         vc_x
@item:
            lda         vc_v
            ldx         vc_v + 1
            jsr         vc_split
            jsr         vc_nok
            bcs         @bad
            inc         vc_x
            lda         vc_q
            sta         vc_v
            lda         vc_q + 1
            sta         vc_v + 1
            ora         vc_v
            bne         @item
            lda         vc_x
            clc
            rts
@none:
            lda         #0
            clc
            rts
@bad:
            sec
            rts

; IN: .A, .X = a name.  OUT: C = 0 if it's a symbol (for set, a formal or a local)
vc_nok:
            pha
            lsr
            pla
            bcs         @bad
            cpx         #IMM_PAGES
            bcc         @bad
            ldy         pk,x
            cpy         #PK_SYMBOL
            bne         @bad
            phx
            ldx         vc_lv
            ldy         L_b,x
            plx
            cpy         #BIN_SET
            bne         @ok
            jmp         vc_formal
@ok:
            clc
            rts
@bad:
            sec
            rts

; OUT: .A, .X = this level's next name (L_nlo/hi: the rest; a symbol, then NIL)
vc_nname:
            ldx         vc_lv
            lda         L_nlo,x
            pha
            lda         L_nhi,x
            tax
            pla
            ldy         pk,x
            cpy         #PK_SYMBOL
            bne         @list
            ldy         vc_lv
            pha
            lda         #0
            sta         L_nlo,y
            sta         L_nhi,y
            pla
            rts
@list:
            jsr         vc_split
            pha
            phx
            ldx         vc_lv
            lda         vc_q
            sta         L_nlo,x
            lda         vc_q + 1
            sta         L_nhi,x
            plx
            pla
            rts

; IN: .A, .X = a name, ex its value: bound as this level's form binds it
vc_bind:
            sta         vc_v
            stx         vc_v + 1
            ldy         vc_lv
            lda         L_b,y
            cmp         #BIN_DEF
            beq         @def
            lda         vc_v
            jsr         vc_formal
            bcs         @setbg
            ldy         vc_isl
            bne         @local
@setl:
            pha                                             ; (A formal: SETL)
            lda         #VO_SETL
            jsr         vc_byte
            pla
            jmp         vc_byte
@local:
            cpy         #2
            beq         @blk
            ldy         vc_lv
            ldx         L_b,y
            cpx         #BIN_SET
            bne         :+
            ldx         L_st,y                              ; (set, a statement: the local set from here)
            beq         @setl
            jsr         vc_dset
            bra         @setl
:
            jsr         vc_dtest                            ; (set! of one set: SETL)
            bne         @setl
            pha                                             ; (set! of a local: SETBL)
            lda         #VO_SETBL
            jsr         vc_rop
            pla
            jsr         vc_byte
            lda         vc_v
            ldx         vc_v + 1
            jsr         vc_word
            jmp         vc_je
@setbg:
            lda         #VX_SETBG
            jsr         vc_xrop
            lda         vc_v
            ldx         vc_v + 1
            jsr         vc_word
            jmp         vc_je
@def:
            lda         #VX_DEFG
            jsr         vc_xbyte
            lda         vc_v
            ldx         vc_v + 1
            jmp         vc_word
@blk:
            pha                                             ; (A block's variable: set, SETLB; set!, SETBLB)
            ldy         vc_lv
            ldx         L_b,y
            cpx         #BIN_SET
            bne         @setblb
            lda         #VO_SETLB
            jsr         vc_byte
            pla
            jsr         vc_byte
            lda         vc_bfb
            jsr         vc_byte
            lda         vc_bp
            jmp         vc_byte
@setblb:
            lda         #VO_SETBLB
            jsr         vc_rop
            pla
            jsr         vc_byte
            lda         vc_bfb
            jsr         vc_byte
            lda         vc_bp
            jsr         vc_byte
            lda         vc_v
            ldx         vc_v + 1
            jsr         vc_word
            jmp         vc_je

; (while test body...): a loop, its test and each body expression a Q-expression run as code (or any expression); its
; value the body's last, NIL if it never ran (a word kept for it, STT, POPX); an error the while's value
vc_while:
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab                              ; (L_t: the test; L_l2: past the loop; L_b: the end)
            ldx         vc_lv
            sta         L_t,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_l2,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_b,x
            jsr         vc_amode
            lda         #VO_CPUSH                           ; (Its value so far: NIL)
            jsr         vc_byte
            lda         #0
            tax
            jsr         vc_word
            jsr         vc_hinc
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_def
            jsr         vc_amset                            ; (The test)
            lda         #1
            sta         vc_cq
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            ldx         vc_lv
            lda         L_l2,x
            jsr         vc_jf
            ldx         vc_lv
            lda         L_alo,x
            ora         L_ahi,x
            beq         @loop
@body:
            jsr         vc_amset                            ; (The body, each in turn: the last's value kept)
            lda         #1
            sta         vc_cq
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            ldx         vc_lv
            lda         L_alo,x
            ora         L_ahi,x
            bne         @body
            lda         #VO_STT
            jsr         vc_byte
@loop:
            ldx         vc_lv                               ; (Back to the test; Ctrl-C, the error: VM_R if the
            lda         L_am,x                              ;   parts return theirs, else to their label)
            cmp         #M_R
            bne         @j
            lda         #VO_LOOP | VM_R
            jsr         vc_byte
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_ref
            lda         #0
            jsr         vc_byte
            lda         #0
            tax
            jsr         vc_word
            bra         @done
@j:
            lda         #VO_LOOP
            jsr         vc_byte
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_ref
            ldx         vc_lv
            lda         vc_h
            sec
            sbc         L_ah,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_aj,x
            jsr         vc_ref
@done:
            ldx         vc_lv
            lda         L_l2,x
            jsr         vc_def
            lda         #VO_POPX
            jsr         vc_byte
            dec         vc_h
            ldx         vc_lv
            lda         L_b,x
            jmp         vc_def

; ---- Blocks (the compiler's): let, each, dotimes, try's handler, each a scope of its own

; IN: .A, .X = a name: a symbol, collected once for the block being made (vc_cnl/h: vc_cn of them; past VC_FORMALS,
; counted, not kept: the block isn't compiled).  Keeps vc_q
vc_cadd:
            pha
            lsr
            pla
            bcs         @rts
            cpx         #IMM_PAGES
            bcc         @rts
            ldy         pk,x
            cpy         #PK_SYMBOL
            bne         @rts
            sta         vc_t
            stx         vc_t + 1
            ldy         #0
@k:
            cpy         vc_cn
            bcs         @add
            cpy         #VC_FORMALS
            bcs         @add
            lda         vc_cnl,y
            cmp         vc_t
            bne         @next
            lda         vc_cnh,y
            cmp         vc_t + 1
            beq         @rts
@next:
            iny
            bra         @k
@add:
            ldy         vc_cn
            cpy         #VC_FORMALS
            bcs         @over
            lda         vc_t
            sta         vc_cnl,y
            lda         vc_t + 1
            sta         vc_cnh,y
@over:
            inc         vc_cn
@rts:
            rts

; IN: .A, .X = a list of expressions: the names set binds in each collected (vc_cadd, through vc_scan's vc_addl)
vc_collect:
            ldy         #1
            sty         vc_tgt
@item:
            cmp         #0
            bne         :+
            cpx         #0
            beq         @done
:
            pha
            phx
            ldy         #0
            jsr         cell_get
            stz         vc_sd
            jsr         vc_scan
            plx
            pla
            ldy         #1
            jsr         cell_get
            bra         @item
@done:
            stz         vc_tgt
            rts

; OUT: C = 0 if a block of the vc_cn names collected can be compiled (VC_FORMALS of them at most, a level and a
; table entry free)
vc_bcan:
            lda         vc_cn
            cmp         #VC_FORMALS + 1
            bcs         @no
            lda         vc_nblk
            cmp         #VC_BLK
            bcs         @no
            lda         vc_ntbl
            cmp         #VC_BTBL
@no:
            rts

; A block begun: BLOCK fb k vb parent names (its slots after the function's locals and the blocks' before it), a
; scope more for the names it binds (vc_formal's), its table entry's start
vc_bopen:
            lda         vc_k
            clc
            adc         vc_bused
            asl
            bcs         @full
            adc         vc_rb
            bcs         @full
            adc         #6
            bcs         @full
            sta         vc_t                                ; (fb)
            lda         vc_cn
            asl
            adc         vc_t
            bcs         @full
            cmp         #240
            bcc         :+
@full:
            jmp         vc_fail
:
            ldx         vc_nblk
            lda         vc_t
            sta         vb_fb,x
            clc
            adc         #2
            sta         vb_vb,x
            lda         vc_cn
            sta         vb_k,x
            lda         hq
            sta         vb_dlo,x
            lda         hq + 1
            sta         vb_dhi,x
            txa                                             ; (Its names)
            asl
            asl
            asl
            asl
            sta         vc_bi
            ldy         #0
@name:
            cpy         vc_cn
            bcs         @named
            phy
            lda         vc_cnl,y
            pha
            lda         vc_cnh,y
            pha
            tya
            clc
            adc         vc_bi
            tay
            pla
            sta         vb_nhi,y
            pla
            sta         vb_nlo,y
            ply
            iny
            bra         @name
@named:
            lda         #VO_BLOCK
            jsr         vc_byte
            ldx         vc_nblk
            lda         vb_fb,x
            jsr         vc_byte
            ldx         vc_nblk
            lda         vb_k,x
            jsr         vc_byte
            ldx         vc_nblk
            lda         vb_vb,x
            jsr         vc_byte
            ldx         vc_nblk                             ; (Its parent: the block it's in, or none)
            beq         @top
            dex
            lda         vb_dlo,x
            pha
            lda         vb_dhi,x
            tax
            pla
            bra         @par
@top:
            lda         #0
            tax
@par:
            jsr         vc_word
            stz         vc_x
@nameo:
            ldx         vc_x
            cpx         vc_cn
            bcs         @table
            lda         vc_cnl,x
            pha
            lda         vc_cnh,x
            tax
            pla
            jsr         vc_word
            inc         vc_x
            bra         @nameo
@table:
            ldx         vc_nblk                             ; (Its table entry: its start)
            ldy         vc_ntbl
            tya
            sta         vb_ti,x
            lda         vb_dlo,x
            sta         vt_slo,y
            lda         vb_dhi,x
            sta         vt_shi,y
            inc         vc_ntbl
            lda         vc_bused                            ; (Its slots used)
            sec
            adc         vc_cn
            sta         vc_bused
            cmp         vc_bneed
            bcc         :+
            sta         vc_bneed
:
            inc         vc_nblk
            rts

; The block ended: its table entry's end here; its scope gone
vc_bclose:
            dec         vc_nblk
            ldx         vc_nblk
            ldy         vb_ti,x
            lda         hq
            sta         vt_elo,y
            lda         hq + 1
            sta         vt_ehi,y
            rts

; This level's arguments left, each run as code (a Q-expression's too): in turn, in the parts' way; the last, if .A
; <> 0, in the level's own way and tail position
vc_body:
            ldx         vc_lv
            sta         L_x,x
@next:
            ldx         vc_lv
            lda         L_alo,x
            ora         L_ahi,x
            beq         @done
            lda         L_alo,x
            pha
            lda         L_ahi,x
            tax
            pla
            ldy         #1
            jsr         cell_get
            sta         vc_t
            txa
            ora         vc_t
            bne         @part
            ldx         vc_lv
            lda         L_x,x
            beq         @part
            jsr         vc_same
            lda         #1
            sta         vc_cq
            jsr         vc_nextt
            bra         @next
@part:
            jsr         vc_amset
            lda         #1
            sta         vc_cq
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            bra         @next
@done:
            rts

; .A, an op that may give an error: VM_R if the parts' way is M_R
vc_ropam:
            ldx         vc_lv
            ldy         L_am,x
            bne         :+
            ora         #VM_R
:
            jmp         vc_byte

; After an op that may give an error: JE to the parts' label if their way is M_J (the words over its height dropped)
vc_jeam:
            ldx         vc_lv
            lda         L_am,x
            cmp         #M_J
            bne         @no
            lda         #VO_JE
            jsr         vc_byte
            ldx         vc_lv
            lda         vc_h
            sec
            sbc         L_ah,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_aj,x
            jmp         vc_ref
@no:
            rts

; LOOP back to label .A (Ctrl-C: the error; the parts' way: VM_R, or a jump)
vc_loop:
            pha
            ldx         vc_lv
            lda         L_am,x
            cmp         #M_R
            bne         @j
            lda         #VO_LOOP | VM_R
            jsr         vc_byte
            pla
            jsr         vc_ref
            lda         #0
            jsr         vc_byte
            lda         #0
            tax
            jmp         vc_word
@j:
            lda         #VO_LOOP
            jsr         vc_byte
            pla
            jsr         vc_ref
            ldx         vc_lv
            lda         vc_h
            sec
            sbc         L_ah,x
            jsr         vc_byte
            ldx         vc_lv
            lda         L_aj,x
            jmp         vc_ref

; SETLB of the innermost block's variable .A (its index)
vc_setb0:
            pha
            lda         #VO_SETLB
            jsr         vc_byte
            ldx         vc_nblk
            dex
            pla
            pha
            asl
            clc
            adc         vb_vb,x
            jsr         vc_byte
            ldx         vc_nblk
            dex
            lda         vb_fb,x
            jsr         vc_byte
            pla
            jmp         vc_byte

; IN: .A, .X = a loop's {name x}.  OUT: C = 0, vc_hd = name, vc_v = x; C = 1 if it isn't one
vc_spec:
            pha
            lsr
            pla
            bcs         @bad
            cpx         #IMM_PAGES
            bcc         @bad
            ldy         pk,x
            cpy         #PK_QCONS
            bne         @bad
            sta         vc_v
            stx         vc_v + 1
            jsr         count_list
            cmp         #2
            bne         @bad
            lda         vc_v
            ldx         vc_v + 1
            jsr         vc_split
            sta         vc_hd
            stx         vc_hd + 1
            pha
            lsr
            pla
            bcs         @bad
            cpx         #IMM_PAGES
            bcc         @bad
            ldy         pk,x
            cpy         #PK_SYMBOL
            bne         @bad
            lda         vc_q
            ldx         vc_q + 1
            ldy         #0
            jsr         cell_get
            sta         vc_v
            stx         vc_v + 1
            clc
            rts
@bad:
            sec
            rts

; IN: .A, .X = a binding of let's ({name} or {name value}).  OUT: C = 0, its name collected; C = 1 if it isn't one
vc_bone:
            pha
            lsr
            pla
            bcs         @bad
            cpx         #IMM_PAGES
            bcc         @bad
            ldy         pk,x
            cpy         #PK_QCONS
            bne         @bad
            pha
            phx
            jsr         count_list
            plx
            tay
            pla
            cpy         #1
            bcc         @bad
            cpy         #3
            bcs         @bad
            ldy         #0
            jsr         cell_get
            pha
            lsr
            pla
            bcs         @bad
            cpx         #IMM_PAGES
            bcc         @bad
            ldy         pk,x
            cpy         #PK_SYMBOL
            bne         @bad
            jsr         vc_cadd
            clc
            rts
@bad:
            sec
            rts

; (let body) or (let {{name value} ...} body...): a block, its names bound in turn, each value run in it (an error,
; the let's value); its body run as code, the last in the let's way
vc_let:
            stz         vc_cn
            ldx         vc_lv
            stz         L_nlo,x
            stz         L_nhi,x
            lda         L_m,x
            bne         :+
@eval:
            jmp         vc_eval
:
            cmp         #1
            beq         @body
            jsr         vc_next                             ; (Its bindings: NIL, or {binding...})
            ldy         vc_lv
            sta         L_nlo,y
            txa
            sta         L_nhi,y
            sta         vc_v + 1
            lda         L_nlo,y
            sta         vc_v
            ora         vc_v + 1
            beq         @body
            ldx         vc_v + 1
            cpx         #IMM_PAGES
            bcc         @eval
            lda         vc_v
            lsr
            bcs         @eval
            lda         pk,x
            cmp         #PK_QCONS
            bne         @eval
@bind:
            lda         vc_v
            ora         vc_v + 1
            beq         @body
            lda         vc_v
            ldx         vc_v + 1
            jsr         vc_split
            ldy         vc_q
            sty         vc_v
            ldy         vc_q + 1
            sty         vc_v + 1
            jsr         vc_bone
            bcs         @eval
            bra         @bind
@body:
            ldx         vc_lv                               ; (The names its body sets)
            lda         L_alo,x
            pha
            lda         L_ahi,x
            tax
            pla
            jsr         vc_collect
            jsr         vc_bcan
            bcs         @eval
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_t,x
            jsr         vc_amode
            jsr         vc_bopen
@value:
            ldx         vc_lv                               ; (Each binding's value (NIL), bound)
            lda         L_nlo,x
            ora         L_nhi,x
            beq         @run
            lda         L_nlo,x
            pha
            lda         L_nhi,x
            tax
            pla
            jsr         vc_split                            ; (A binding; the rest kept)
            ldy         vc_lv
            pha
            lda         vc_q
            sta         L_nlo,y
            lda         vc_q + 1
            sta         L_nhi,y
            pla
            jsr         vc_split                            ; (Its name; vc_q its value's list)
            pha
            phx
            lda         vc_q
            ora         vc_q + 1
            beq         @nil
            lda         vc_q
            ldx         vc_q + 1
            ldy         #0
            jsr         cell_get
            pha
            phx
            jsr         vc_amset
            plx
            pla
            ldy         #0
            jsr         vc_child
            bra         @set
@nil:
            jsr         vc_nil
@set:
            plx
            pla
            jsr         vc_formal
            pha
            lda         #VO_SETLB
            jsr         vc_byte
            pla
            jsr         vc_byte
            lda         vc_bfb
            jsr         vc_byte
            lda         vc_bp
            jsr         vc_byte
            bra         @value
@run:
            lda         #1                                  ; (Its body)
            jsr         vc_body
            jsr         vc_bclose
            ldx         vc_lv
            lda         L_t,x
            jmp         vc_def

; (dotimes {i n} body...), (each {x list} body...): a loop, its count or list run where the loop is; for each i
; (0 to n - 1) or item, a block of the name bound, its body run as code; NIL (an error, the loop's value)
vc_dotimes:
            lda         #VO_DOTN
            bra         vc_loopf
vc_each:
            lda         #VO_EACHL
vc_loopf:
            ldx         vc_lv
            sta         L_b,x                               ; (The op: DOTN, EACHL)
            lda         L_m,x
            cmp         #2
            bcs         :+
@eval:
            jmp         vc_eval
:
            jsr         vc_next                             ; ({name x}; the body after)
            jsr         vc_spec
            bcs         @eval
            stz         vc_cn
            lda         vc_hd
            ldx         vc_hd + 1
            jsr         vc_cadd
            ldx         vc_lv
            lda         vc_v
            sta         L_nlo,x
            lda         vc_v + 1
            sta         L_nhi,x
            lda         L_alo,x
            pha
            lda         L_ahi,x
            tax
            pla
            jsr         vc_collect
            jsr         vc_bcan
            bcs         @eval
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab                              ; (L_t: the next; L_l2: past the loop; L_l3: the end)
            ldx         vc_lv
            sta         L_t,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_l2,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_l3,x
            jsr         vc_amode
            jsr         vc_amset                            ; (Its count, its list)
            ldx         vc_lv
            lda         L_nlo,x
            pha
            lda         L_nhi,x
            tax
            pla
            ldy         #0
            jsr         vc_child
            ldx         vc_lv
            lda         L_b,x
            jsr         vc_ropam
            jsr         vc_jeam
            jsr         vc_hinc                             ; (Pushed: n and i; the items)
            ldx         vc_lv
            lda         L_b,x
            cmp         #VO_DOTN
            bne         :+
            jsr         vc_hinc
:
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_def
            ldx         vc_lv                               ; (DOTI, EACHI: past the loop, or ex the next)
            lda         L_b,x
            clc
            adc         #VO_DOTI - VO_DOTN
            jsr         vc_byte
            ldx         vc_lv
            lda         L_l2,x
            jsr         vc_ref
            jsr         vc_bopen
            lda         #0
            jsr         vc_setb0
            lda         #0                                  ; (Its body: each in turn)
            jsr         vc_body
            jsr         vc_bclose
            ldx         vc_lv
            lda         L_b,x
            cmp         #VO_DOTN
            bne         :+
            lda         #VO_DOTINC
            jsr         vc_byte
:
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_loop
            ldx         vc_lv
            lda         L_l2,x
            jsr         vc_def
            lda         #VO_DROP                            ; (Its words dropped: NIL)
            jsr         vc_byte
            ldx         vc_lv
            lda         L_b,x
            ldy         #1
            cmp         #VO_DOTN
            bne         :+
            iny
:
            tya
            jsr         vc_byte
            ldx         vc_lv
            lda         L_h0,x
            sta         vc_h
            lda         L_l3,x
            jmp         vc_def

; (try x [handler]): x run as code, its error caught: NIL, or the handler's value in a block of &err and &code (a
; function's: called with the message)
vc_try:
            ldx         vc_lv
            lda         L_m,x
            beq         @eval
            cmp         #3
            bcs         @eval
            stz         vc_cn                               ; (Its block's names: &err, &code, its handler's sets)
            lda         sym_err
            ldx         sym_err + 1
            jsr         vc_cadd
            lda         sym_code
            ldx         sym_code + 1
            jsr         vc_cadd
            ldx         vc_lv
            lda         L_alo,x
            pha
            lda         L_ahi,x
            tax
            pla
            ldy         #1
            jsr         cell_get
            jsr         vc_collect
            jsr         vc_bcan
            bcc         :+
@eval:
            jmp         vc_eval
:
            ldx         vc_lv
            lda         vc_h
            sta         L_h0,x
            jsr         vc_lab                              ; (L_t: a handler that isn't a function; L_l2: the end)
            ldx         vc_lv
            sta         L_t,x
            jsr         vc_lab
            ldx         vc_lv
            sta         L_l2,x
            lda         #M_OK                               ; (x: its error kept)
            sta         vc_cm
            lda         #1
            sta         vc_cq
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            lda         #VO_TRYE
            jsr         vc_byte
            ldx         vc_lv
            lda         L_l2,x
            jsr         vc_ref
            ldx         vc_lv
            lda         L_m,x
            cmp         #2
            beq         @handler
            jsr         vc_nil                              ; (No handler: NIL)
            jmp         @end
@handler:
            jsr         vc_bopen
            lda         #VX_TRYV                            ; (&err and &code: its variables 0 and 1)
            jsr         vc_xbyte
            ldx         vc_nblk
            dex
            lda         vb_vb,x
            jsr         vc_byte
            ldx         vc_nblk
            dex
            lda         vb_vb,x
            clc
            adc         #2
            jsr         vc_byte
            jsr         vc_hinc
            jsr         vc_same                             ; (The handler, run as code)
            lda         #1
            sta         vc_cq
            jsr         vc_next
            ldy         #0
            jsr         vc_child
            lda         #VX_TRYH
            jsr         vc_xbyte
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_ref
            lda         #VX_TRYF
            jsr         vc_xbyte
            jsr         vc_hinc
            ldx         vc_lv                               ; (A function: called with the message)
            lda         L_tail,x
            beq         :+
            lda         #VO_TCALL
            bra         :++
:
            lda         #VO_CALL
:
            jsr         vc_byte
            lda         #1
            jsr         vc_byte
            ldx         vc_lv
            lda         L_tail,x
            bne         :+
            lda         #12
            jsr         vc_retw
            jsr         vc_ccache
:
            ldx         vc_lv
            lda         L_h0,x
            jsr         vc_byte
            jsr         vc_rbyte
            ldx         vc_lv
            lda         L_h0,x
            sta         vc_h
            jsr         vc_je
            jsr         vc_bclose
            ldx         vc_lv
            lda         L_t,x
            jsr         vc_def
@end:
            ldx         vc_lv
            lda         L_l2,x
            jmp         vc_def

.segment "RODATA7"

; The ops' code, by number (VM_R set: the same)
.macro VM_OPS
            .word       op_ret, op_const, op_local, op_global, op_lookup, op_push, op_jf, op_jt
            .word       op_jmp, op_je, op_head, op_call, op_tcall, op_bcall, op_ext, op_quote
            .word       op_quote0, op_add, op_sub, op_lt, op_gt, op_le, op_ge, op_neq
            .word       op_inc, op_dec, op_zerop, op_onep, op_addc, op_subc, op_ltc, op_gtc
            .word       op_lec, op_gec, op_eqc, op_lpush, op_cpush, op_shead, op_cself, op_tself
            .word       op_locals, op_localh, op_setl, op_setbl, op_lq, op_jlq, op_stt, op_popx
            .word       op_loop, op_block, op_localb, op_setlb, op_setblb, op_dotn, op_doti, op_dotinc
            .word       op_eachl, op_eachi, op_drop, op_trye, op_jq, op_ll, op_jll, op_lqp
.endmacro
.assert     VO_COUNT = 64 * 2, error, "vm.inc: vm_ops has an op too few or too many"
vm_ops:
            VM_OPS
            VM_OPS

; The rare ops' code (EXT's), by number (VM_R set: the same)
.macro VM_XOPS
            .word       op_eval, op_setbg, op_defg, op_tryv, op_tryh, op_tryf
            .res        128 - VX_COUNT, $FF
.endmacro
vm_xops:
            VM_XOPS
            VM_XOPS

; Quarter squares (vm_mul8's): x * x / 4, x 0 to 511, its low bytes, its high
vm_sqlo:
.repeat 512, I
            .byte       <((I * I) / 4)
.endrep
vm_sqhi:
.repeat 512, I
            .byte       >((I * I) / 4)
.endrep

; The fused ops' fixnum work, by quick op (VO_ADD's order: s - VO_ADD).  OUT: C = 1 if it isn't fixnums (or past one)
vm_qf:      .word       vf_add, vf_sub, vf_lt, vf_gt, vf_le, vf_ge, vf_eq, vf_inc, vf_dec, vf_zerop, vf_onep
            .word       vf_add, vf_sub, vf_lt, vf_gt, vf_le, vf_ge, vf_eq

; The fused ops' code by s (VO_ADD's order: the tables' place less VO_ADD, s's bit 0 aside)
lq_tab:     .word       vm_lqg, vm_lqg, vm_lqg, vm_lqg, vm_lqg, vm_lqg
            .word       vm_lqg, lq_inc, lq_dec, vm_lqg, vm_lqg, lq_addc
            .word       lq_subc, vm_lqg, vm_lqg, vm_lqg, vm_lqg, vm_lqg
lqp_tab:    .word       vm_lqg, vm_lqg, vm_lqg, vm_lqg, vm_lqg, vm_lqg
            .word       vm_lqg, lqp_inc, lqp_dec, vm_lqg, vm_lqg, lqp_addc
            .word       lqp_subc, vm_lqg, vm_lqg, vm_lqg, vm_lqg, vm_lqg
jlq_tab:    .word       vm_jlqg, vm_jlqg, vm_jlqg, vm_jlqg, vm_jlqg, vm_jlqg
            .word       vm_jlqg, vm_jlqg, vm_jlqg, jlq_zerop, jlq_onep, vm_jlqg
            .word       vm_jlqg, jlq_ltc, jlq_gtc, jlq_lec, jlq_gec, jlq_eqc
jq_tab:     .word       vm_jqg, vm_jqg, vm_jqg, vm_jqg, vm_jqg, vm_jqg
            .word       vm_jqg, vm_jqg, vm_jqg, jq_zerop, jq_onep, vm_jqg
            .word       vm_jqg, jq_ltc, jq_gtc, jq_lec, jq_gec, jq_eqc
ll_tab:     .word       ll_add, ll_sub, vm_llg, vm_llg, vm_llg, vm_llg
            .word       vm_llg, vm_llg, vm_llg, vm_llg, vm_llg, vm_llg
            .word       vm_llg, vm_llg, vm_llg, vm_llg, vm_llg, vm_llg
jll_tab:    .word       vm_jllg, vm_jllg, jll_lt, jll_gt, jll_le, jll_ge
            .word       jll_eq, vm_jllg, vm_jllg, vm_jllg, vm_jllg, vm_jllg
            .word       vm_jllg, vm_jllg, vm_jllg, vm_jllg, vm_jllg, vm_jllg

; The quick ops' built-ins and their values counted (VO_ADD's order: (op - VO_ADD) / 2)
vo_bin:     .byte       BIN_ADD, BIN_SUB, BIN_LT, BIN_GT, BIN_LE, BIN_GE, BIN_EQ, BIN_INC, BIN_DEC, BIN_ZEROP
            .byte       BIN_ONEP, BIN_ADD, BIN_SUB, BIN_LT, BIN_GT, BIN_LE, BIN_GE, BIN_EQ
vo_n:       .byte       2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2

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

.segment "RODATA6"

; The ops that call nothing (vc_shflag's), by number: their length (0: one that may)
vc_pure:    .byte       0, 3, 2, 3, 0, 1, 0, 0                          ; (RET CONST LOCAL GLOBAL LOOKUP PUSH JF JT)
            .byte       0, 0, 0, 0, 0, 0, 0, 0                          ; (JMP JE HEAD CALL TCALL BCALL EXT QUOTE)
            .byte       3, 1, 1, 1, 1, 1, 1, 1                          ; (QUOTE0 ADD SUB LT GT LE GE NEQ)
            .byte       1, 1, 1, 1, 3, 3, 3, 3                          ; (INC DEC ZEROP ONEP ADDC SUBC LTC GTC)
            .byte       3, 3, 3, 2, 3, 0, 0, 0                          ; (LEC GEC EQC LPUSH CPUSH SHEAD CSELF TSELF)
            .byte       0, 0, 0, 0, 5, 0, 0, 0                          ; (LOCALS LOCALH SETL SETBL LQ JLQ STT POPX)
            .byte       0, 0, 0, 0, 0, 0, 0, 0                          ; (LOOP BLOCK LOCALB SETLB SETBLB DOTN DOTI DOTINC)
            .byte       0, 0, 0, 0, 0, 4, 0, 5                          ; (EACHL EACHI DROP TRYE JQ LL JLL LQP)

; The compiler's: the built-ins that are quick ops, the values they're given, the op
QUICKS          = 11
vq_bin:     .byte       BIN_ADD, BIN_SUB, BIN_LT, BIN_GT, BIN_LE, BIN_GE, BIN_EQ, BIN_INC, BIN_DEC, BIN_ZEROP
            .byte       BIN_ONEP
vq_n:       .byte       2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1
vq_op:      .byte       VO_ADD, VO_SUB, VO_LT, VO_GT, VO_LE, VO_GE, VO_NEQ, VO_INC, VO_DEC, VO_ZEROP, VO_ONEP

; The built-ins' flags again (vb_flags'), for this bank
; A bit's mask, by its number
vc_bits:    .byte       1, 2, 4, 8, 16, 32, 64, 128

vc_bflags:
.repeat BI_COUNT, I
            .byte       .ident(.sprintf("bi_fl_%d", I))
.endrep
