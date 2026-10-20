; ****************************************************************************
; bankprog.s - bank programs (bankprog.h): children, o65 modules linked at $8000 (sdk/c/child.cfg, build.js's
; children), kept by the kernel in this task's code cache (BANK_LOAD: RAM banks of the task's own, several children to
; a bank, the one used least lately given up when there's no room) and called here like any function, on the caller's
; C stack.
;
; A handle (bank_fn's) names its child's file and entry: the kernel's directory entry it was found in (its bank, its
; entry there, its load's number) and the entry's address, from the child's exports' table (in its bank).  bank_jsr,
; the call: its bank in the window (the bank register, $00; the shared banks' U left as it is), its directory entry
; still the same load (else BANK_LOAD again: given up meanwhile, loaded again), its use stamped, the entry called
; with .A/.X and the C stack as they were, the bank register put back with .A/.X as the entry left them.  No child may
; call one: it's in the window itself.
; ****************************************************************************

            .setcpu     "65C02"
            .export     _bank_setup, _bank_fn_, _bank_jsr, _bank_flush, _bank_limit
            .export     _bank_err, _bank_missing, _bank_go
            .import     popa, popax

            .include    "zeropage.inc"
            .include    "hydra.inc"

NFN         = 12                    ; bankprog.h's BANK_FNS
FN_CHILD    = 0                     ; struct bank_fn
FN_ENTRY    = 2
FN_BANK     = 4
FN_ENT      = 5                     ; (Its directory entry's offset in RB_DIR's page)
FN_GEN      = 6
FN_AT       = 7
FN_SIZE     = 9
BANKREG     = $00                   ; The bank register: the window's bank

BE_SETUP    = 1                     ; bankprog.h's BE_* (BANK_LOAD's .X, BL_*, the rest)
BE_IMPORT   = 6
BE_ENTRY    = 8

            .bss
_bank_err:  .res        1
_bank_missing: .res     16
_bank_go:   .res        2
expath:     .res        2           ; The caller's exports' file, and its children's zero page
zpbase:     .res        1
zpn:        .res        1
fns:        .res        NFN * FN_SIZE   ; The handles
nfn:        .res        1
sava:       .res        1           ; (bank_jsr's .A, .X round a reload, and the entry)
savx:       .res        1
jvec:       .res        2

            .code

; ---- int __fastcall__ bank_setup (const char* exports, unsigned char zp, unsigned char n)
_bank_setup:
            sta         zpn
            jsr         popa
            sta         zpbase
            jsr         popax
            sta         expath
            stx         expath + 1
            lda         #0
            tax
            rts

; ---- struct bank_fn* __fastcall__ bank_fn_ (const char* child, const char* entry): a handle (the one already made
; for the same names, or a new one), its child loaded and its entry found; or NULL
_bank_fn_:
            sta         ptr2                ; ptr2 the entry's name, ptr1 the child's
            stx         ptr2 + 1
            jsr         popax
            sta         ptr1
            stx         ptr1 + 1
            stz         _bank_err
            lda         #<fns               ; One with these names already?
            sta         ptr3
            lda         #>fns
            sta         ptr3 + 1
            ldx         nfn
@find:      beq         @new
            ldy         #FN_CHILD
            lda         (ptr3),y
            cmp         ptr1
            bne         @next
            iny
            lda         (ptr3),y
            cmp         ptr1 + 1
            bne         @next
            iny
            lda         (ptr3),y
            cmp         ptr2
            bne         @next
            iny
            lda         (ptr3),y
            cmp         ptr2 + 1
            beq         @bind
@next:      clc
            lda         ptr3
            adc         #FN_SIZE
            sta         ptr3
            bcc         :+
            inc         ptr3 + 1
:           dex
            bra         @find
@new:       lda         nfn                 ; A new one (ptr3 at the next free)
            cmp         #NFN
            bcs         @full
            lda         expath + 1
            beq         @full
            inc         nfn
            ldy         #FN_CHILD
            lda         ptr1
            sta         (ptr3),y
            iny
            lda         ptr1 + 1
            sta         (ptr3),y
            iny
            lda         ptr2
            sta         (ptr3),y
            iny
            lda         ptr2 + 1
            sta         (ptr3),y
            jsr         bind
            bcc         @good
            dec         nfn                 ; (Its child not loaded: the new handle given back)
            bra         @none
@bind:      jsr         bind
            bcs         @none
@good:      lda         ptr3
            ldx         ptr3 + 1
            rts
@full:      lda         #BE_SETUP
            sta         _bank_err
@none:      lda         #0
            tax
            rts

; ---- bind: handle ptr3 made good: its child in the cache (BANK_LOAD), its directory entry and load kept in it, its
; entry's address from the child's table.  C = 1 if it can't be (bank_err)
bind:
            ldy         #FN_CHILD
            lda         (ptr3),y
            sta         r0
            iny
            lda         (ptr3),y
            sta         r0 + 1
            lda         expath
            sta         r1
            lda         expath + 1
            sta         r1 + 1
            lda         zpbase
            ldx         zpn
            jsr         BANK_LOAD
            bcc         @have
            stx         _bank_err           ; (Why: BL_*, bankprog.h's BE_*)
            cpx         #BE_IMPORT
            bne         @no
            ldy         #0                  ; (Its name)
:           lda         TA_SCRATCH,y
            sta         _bank_missing,y
            beq         @no
            iny
            cpy         #15
            bne         :-
            lda         #0
            sta         _bank_missing,y
@no:        sec
            rts
@have:      ldy         #FN_BANK
            sta         (ptr3),y
            iny
            txa
            asl
            asl
            asl
            asl
            sta         (ptr3),y
            lda         BANKREG             ; Its entry, in its table in its bank
            pha
            ldy         #FN_BANK
            lda         (ptr3),y
            sta         BANKREG
            jsr         dirent              ; (ptr4: its directory entry)
            ldy         #RE_GEN
            lda         (ptr4),y
            ldy         #FN_GEN
            sta         (ptr3),y
            ldy         #RE_TABLE
            lda         (ptr4),y
            tax
            iny
            lda         (ptr4),y
            sta         ptr4 + 1
            stx         ptr4
            ldy         #FN_ENTRY
            lda         (ptr3),y
            sta         ptr2
            iny
            lda         (ptr3),y
            sta         ptr2 + 1
@entry:     lda         (ptr4)              ; (The table's end: an empty name)
            beq         @noentry
            ldy         #0
@cmp:       lda         (ptr4),y
            cmp         (ptr2),y
            bne         @skip
            iny
            cmp         #0
            bne         @cmp
            lda         (ptr4),y            ; Found: its address after the 0
            pha
            iny
            lda         (ptr4),y
            ldy         #FN_AT + 1
            sta         (ptr3),y
            dey
            pla
            sta         (ptr3),y
            pla
            sta         BANKREG
            clc
            rts
@skip:      ldy         #0                  ; (Past this one: its name, its 0, its address)
:           lda         (ptr4),y
            iny
            cmp         #0
            bne         :-
            iny
            iny
            tya
            clc
            adc         ptr4
            sta         ptr4
            bcc         @entry
            inc         ptr4 + 1
            bra         @entry
@noentry:   pla
            sta         BANKREG
            lda         #BE_ENTRY
            sta         _bank_err
            sec
            rts

; ptr4 = handle ptr3's directory entry (in RB_DIR's page), in the bank in the window
dirent:
            ldy         #FN_ENT
            lda         (ptr3),y
            sta         ptr4
            lda         #>RB_DIR
            sta         ptr4 + 1
            rts

; ---- bank_jsr: bank_call's call through the handle bank_go, .A/.X and the C stack its arguments
_bank_jsr:
            sta         sava
            lda         _bank_go
            sta         ptr3
            lda         _bank_go + 1
            sta         ptr3 + 1
            lda         BANKREG
            pha
@try:       ldy         #FN_GEN             ; (Its load none: bank_flush's)
            lda         (ptr3),y
            beq         @again
            ldy         #FN_ENT             ; Its directory entry ...
            lda         (ptr3),y
            sta         ptr4
            lda         #>RB_DIR
            sta         ptr4 + 1
            ldy         #FN_BANK            ; ... its bank in the window: still the same load?
            lda         (ptr3),y
            sta         BANKREG
            ldy         #RE_GEN
            lda         (ptr4),y
            ldy         #FN_GEN
            cmp         (ptr3),y
            bne         @again
            inc         TK_RCLOCK           ; (Used now)
            bne         :+
            inc         TK_RCLOCK + 1
:           ldy         #RE_STAMP
            lda         TK_RCLOCK
            sta         (ptr4),y
            iny
            lda         TK_RCLOCK + 1
            sta         (ptr4),y
            ldy         #FN_AT
            lda         (ptr3),y
            sta         jvec
            iny
            lda         (ptr3),y
            sta         jvec + 1
            lda         sava
            jsr         @jump
            tay                             ; (.Y: cc65's calls don't keep it)
            pla
            sta         BANKREG
            tya
            rts
@jump:      jmp         (jvec)
@again:     stx         savx                ; (Given up meanwhile: loaded again, the handle made good)
            pla
            sta         BANKREG
            pha
            jsr         bind
            ldx         savx
            bcc         @try
            pla
            sta         BANKREG
            lda         #$FF
            tax
            rts

; ---- void bank_flush (void): every child given up (their banks freed)
_bank_flush:
            lda         #0
flush:
            jsr         BANK_FLUSH
            ldx         nfn                 ; (Every handle's load none: loaded again if it's called)
            beq         @done
            lda         #<fns
            sta         ptr3
            lda         #>fns
            sta         ptr3 + 1
@one:       ldy         #FN_GEN
            lda         #0
            sta         (ptr3),y
            clc
            lda         ptr3
            adc         #FN_SIZE
            sta         ptr3
            bcc         :+
            inc         ptr3 + 1
:           dex
            bne         @one
@done:      rts

; ---- void __fastcall__ bank_limit (unsigned char banks): bank_flush, and the code cache's banks at most from now on
_bank_limit:
            bra         flush
