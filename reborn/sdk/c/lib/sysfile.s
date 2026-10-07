; ****************************************************************************
; sysfile.s - the calls behind remove, rmdir, mkdir and rename (cc65's code sets errno from what they return: 0,
; or the kernel's error code).  Names are relative to the task's current directory unless they start with / (or
; are # names).

            .export     __sysremove, __sysrmdir, __sysmkdir, __sysrename
            .import     popax, addysp

            .include    "zeropage.inc"
            .include    "hydra.inc"

            .code

; unsigned char __fastcall__ _sysremove (const char* name): a file, or an empty directory (REMOVE)
__sysremove:
__sysrmdir:
            sta         r0
            stx         r0 + 1
            jsr         REMOVE
            bcs         done
ok:
            lda         #0
done:
            rts

; unsigned char _sysmkdir (const char* name, ...): a new directory (CREATE, DM_DIR), its mode left out
__sysmkdir:
            dey                                             ; (The name alone: the rest off the C stack)
            dey
            jsr         addysp
            jsr         popax
            sta         r0
            stx         r0 + 1
            lda         #O_READ
            ldx         #DM_DIR
            jsr         CREATE
            bcs         done
            jsr         CLOSE
            bcs         done
            bra         ok

; unsigned char __fastcall__ _sysrename (const char* oldname, const char* newname): a new name in its own directory
; (WSTAT: a record with the new name, the rest $FF: changed no further).  The new name's last part is the name (mv
; moves a file to another directory)
__sysrename:
            sta         ptr2                                ; The new name: its last part (past its last /)
            stx         ptr2 + 1
            ldy         #0
@find:
            lda         (ptr2),Y
            beq         @found
            iny
            cmp         #'/'
            bne         @find
            tya
            clc
            adc         ptr2
            sta         ptr2
            bcc         :+
            inc         ptr2 + 1
:
            ldy         #0
            bra         @find

@found:
            ldy         #SR_SIZE - 1
            lda         #$FF
:
            sta         stat,Y
            dey
            bpl         :-
            ldy         #0
@copy:
            lda         (ptr2),Y
            sta         stat + SR_NAME,Y
            beq         @named
            iny
            cpy         #31
            bne         @copy
            lda         #0
            sta         stat + SR_NAME,Y
@named:
            jsr         popax                               ; The old name
            sta         r0
            stx         r0 + 1
            lda         #<stat
            sta         r1
            lda         #>stat
            sta         r1 + 1
            jsr         WSTAT
            bcs         done
            bra         ok

            .bss
stat:       .res        SR_SIZE
