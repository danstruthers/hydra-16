; ****************************************************************************
; bank.s - (dq0harn) the referee's RAM banks, as the UI's (games/dq0/bank.s): the window at $8000 set and read;
; bytes copied between a bank and the main RAM; and the overlays' calls.  An overlay is code linked at $8000
; (dq0harn.cfg: OV1 ...), read into a bank of its own as the referee starts (ovbank: overlay n's bank); a call of an
; overlay's function goes through its stub here (OVFN): the bank there now kept on a little stack of its own, the
; overlay's put in, its function called, the bank put back.  So an overlay can call the main code and other overlays
; alike, and the main code can call them.  The rule that keeps it right: a function here that changes the window
; (bread, bwrite) never reads or writes the caller's memory in it, only the main RAM ($0800-$7EFF).

            .export     _setbank, _getbank, _bread, _bwrite, _ovbank
            .import     popa, popax

            .include    "zeropage.inc"

BANKREG     = $00                   ; The task's RAM bank register (written: the bank at $8000; read: that bank)
OVMAX       = 3                     ; dq0harn.cfg's overlays
DEPTH       = 16                    ; Overlay calls in calls, at most

            .bss
_ovbank:    .res        OVMAX       ; Overlay n's bank: _ovbank[n - 1] (dq0harn.c's ovl_load sets them)
ovstk:      .res        DEPTH       ; The banks the overlay calls found, to put back
ovsp:       .res        1
ova:        .res        1           ; (.A and .X kept through a stub)
ovx:        .res        1

            .code

; void __fastcall__ setbank (unsigned char b): the bank at $8000.  (hydra.h's hy_bank, but a call: cc65's optimizer
; can lose an index in hy_bank (a[i]))
_setbank:
            sta         BANKREG
            rts

; unsigned char getbank (void): the bank at $8000
_getbank:
            lda         BANKREG
            ldx         #0
            rts

; void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n): n bytes from bank's from
; (in the window) to to (in the main RAM), the window put back as it was
_bread:
            jsr         args                ; ptr1 from, ptr2 to, ptr3 n, tmp1 the bank
            ldy         BANKREG
            lda         tmp1
            sta         BANKREG
            jsr         copy
            sty         BANKREG
            rts

; void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n): n bytes from from (in the
; main RAM) to bank's to (in the window), the window put back as it was
_bwrite:
            jsr         args                ; ptr1 to, ptr2 from, ptr3 n, tmp1 the bank
            lda         ptr1                ; (copy goes from ptr1 to ptr2: swapped)
            ldx         ptr2
            sta         ptr2
            stx         ptr1
            lda         ptr1 + 1
            ldx         ptr2 + 1
            sta         ptr2 + 1
            stx         ptr1 + 1
            ldy         BANKREG
            lda         tmp1
            sta         BANKREG
            jsr         copy
            sty         BANKREG
            rts

; bread's and bwrite's arguments: n (.A, .X) in ptr3, then the two addresses in ptr2 and ptr1, the bank in tmp1
args:
            sta         ptr3
            stx         ptr3 + 1
            jsr         popax
            sta         ptr2
            stx         ptr2 + 1
            jsr         popax
            sta         ptr1
            stx         ptr1 + 1
            jsr         popa
            sta         tmp1
            rts

; ptr3 bytes from ptr1 to ptr2 (upward).  Keeps .Y
copy:
            phy
            ldy         #0
            ldx         ptr3 + 1            ; Whole pages first
            beq         @part
@page:      lda         (ptr1),y
            sta         (ptr2),y
            iny
            bne         @page
            inc         ptr1 + 1
            inc         ptr2 + 1
            dex
            bne         @page
@part:      ldx         ptr3                ; Then what's left
            beq         @done
@byte:      lda         (ptr1),y
            sta         (ptr2),y
            iny
            dex
            bne         @byte
@done:      ply
            rts

; ---- The overlays' calls

; Overlay .Y's bank in the window, the one there kept.  Keeps .A, .X
ovl_in:
            sta         ova
            stx         ovx
            ldx         ovsp
            lda         BANKREG
            sta         ovstk,x
            inc         ovsp
            lda         _ovbank - 1,y
            sta         BANKREG
            lda         ova
            ldx         ovx
            rts

; The bank the last ovl_in found, put back.  Keeps .A, .X (a function's value)
ovl_out:
            sta         ova
            stx         ovx
            dec         ovsp
            ldx         ovsp
            lda         ovstk,x
            sta         BANKREG
            lda         ova
            ldx         ovx
            rts

; A stub: name() calls overlay ov's o_name(), its arguments and its value as they are (.Y isn't an argument of a
; function that isn't variadic)
.macro      OVFN        name, ov
            .export     .ident(.concat("_", .string(name)))
            .import     .ident(.concat("_o_", .string(name)))
.ident(.concat("_", .string(name))):
            ldy         #ov
            jsr         ovl_in
            jsr         .ident(.concat("_o_", .string(name)))
            jmp         ovl_out
.endmacro

; The overlays' functions (dq0harn.h has them with their overlays)
            OVFN        sheet, 1
            OVFN        brief, 1
            OVFN        harm, 1
            OVFN        fight, 2
            OVFN        pc_armour, 3
            OVFN        pc_side, 3
