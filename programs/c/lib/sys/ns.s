; ****************************************************************************
; ns.s - the namespace calls for C (include/hydra.h): hy_bind, hy_mount, hy_unmount, hy_hide, hy_newns (IO_BIND,
; IO_MOUNT, IO_UNMOUNT: os_rom/io/ns.s).  A failed call sets _oserror and errno, and returns -1.

        .export     _hy_bind, _hy_mount, _hy_unmount, _hy_hide, _hy_newns
        .import     ___mappederrno, popax

        .include    "zeropage.inc"
        .include    "hydra.inc"

        .assert     NS_AFTER = 1 .and NS_BEFORE = 2 .and NS_CREATE = 4, error, "hydra.h: HY_MAFTER, HY_MBEFORE, HY_MCREATE"

        .code

; int __fastcall__ hy_bind (const char* new, const char* old, int flags)
_hy_bind:
        ldy         #0
        bra         bindmount

; int __fastcall__ hy_mount (const char* dev, const char* old, int flags, const char* spec): spec NULL: none
_hy_mount:
        sta         ZP_IO_CNT                           ; The spec (NS_SPEC)
        stx         ZP_IO_CNT + 1
        jsr         popax                               ; The flags
        ldy         ZP_IO_CNT + 1
        beq         :+
        ora         #NS_SPEC
:
        ldy         #1

bindmount:
        sty         tmp2                                ; (Which)
        and         #NS_AFTER | NS_BEFORE | NS_CREATE | NS_SPEC
        sta         tmp1                                ; The flags
        jsr         popax                               ; old
        sta         ptr1
        stx         ptr1 + 1
        jsr         popax                               ; new (dev)
        sta         ZP_IO_BUF
        stx         ZP_IO_BUF + 1
        lda         ptr1
        ldy         ptr1 + 1
        ldx         tmp1
        lsr         tmp2
        bcs         @mount
        jsr         IO_BIND
        bra         done

@mount:
        jsr         IO_MOUNT
        bra         done

; int __fastcall__ hy_unmount (const char* new, const char* old): old's member new; NULL: all of old's entries
_hy_unmount:
        sta         ptr1                                ; old
        stx         ptr1 + 1
        jsr         popax                               ; new
        sta         ZP_IO_BUF
        stx         ZP_IO_BUF + 1
        ora         ZP_IO_BUF + 1                       ; .X = 0: all of them; 1: the member
        beq         :+
        lda         #1
:
        tax
        lda         ptr1
        ldy         ptr1 + 1
        jsr         IO_UNMOUNT
        bra         done

; int __fastcall__ hy_hide (const char* path): nothing under path is found (this task, and the ones it starts)
_hy_hide:
        pha
        txa
        tay
        pla
        ldx         #NS_HIDDEN
        jsr         IO_BIND
        bra         done

; int hy_newns (void): a fresh namespace, as Plan 9's newns: this task's own entries go, but its /ram, so it sees
; the system namespace (IO_UNMOUNT with NS_FRESH)
_hy_newns:
        lda         #<root
        ldy         #>root
        ldx         #NS_FRESH
        jsr         IO_UNMOUNT

done:
        bcs         error
        lda         #0
        tax
        rts

error:
        jmp         ___mappederrno

        .rodata

root:   .byte       "/", 0                              ; (hy_newns's path: unused)
