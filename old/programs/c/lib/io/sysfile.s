; ****************************************************************************
; sysfile.s - the calls behind remove, rmdir, mkdir and rename (cc65's common code sets errno from what they
; return: 0, or the Hydra's error).  Names are relative to the task's current directory unless they start
; with /.

        .export     __sysremove, __sysrmdir, __sysmkdir, __sysrename
        .import     popax, addysp

        .include    "zeropage.inc"
        .include    "hydra.inc"

        .code

; unsigned char __fastcall__ _sysremove (const char* name): a file, or an empty directory (IO_REMOVE)
__sysremove:
__sysrmdir:
        jsr         nameay
        jsr         IO_REMOVE
        bcs         done
ok:
        lda         #0
done:
        rts

; unsigned char _sysmkdir (const char* name, ...): a new directory (IO_CREATE, HFS_M_DIR)
__sysmkdir:
        dey                                             ; (Only the name: the mode's left out)
        dey
        jsr         addysp
        jsr         popax
        jsr         nameay
        ldx         #HFS_M_DIR
        stx         ZP_IO_BUF
        ldx         #IO_MODE_READ
        jsr         IO_CREATE
        bcs         done
        jsr         IO_CLOSE
        bcs         done
        bra         ok

; unsigned char __fastcall__ _sysrename (const char* oldname, const char* newname): a new name in the same
; directory (IO_WSTAT: the shell's mv does the same).  The new name can't have a '/' (ERR_IO_NAME).
__sysrename:
        sta         ptr2                                ; The new name, into the stat record
        stx         ptr2 + 1
        ldy         #0
@copy:
        lda         (ptr2),y
        sta         stat + IO_ST_NAME,y
        beq         @named
        cmp         #'/'
        beq         @bad
        iny
        cpy         #32
        bne         @copy

@bad:
        jsr         popax                               ; (The old name: off the C stack)
        lda         #ERR_IO_NAME
        rts

@named:
        lda         #$FF                                ; (Its mode as it is)
        sta         stat + IO_ST_MODE
        jsr         popax                               ; The old name
        jsr         nameay
        ldx         #IO_MODE_READ
        jsr         IO_OPEN
        bcs         done
        sta         tmp1                                ; (The fd)
        lda         #<stat
        sta         ZP_IO_BUF
        lda         #>stat
        sta         ZP_IO_BUF + 1
        lda         tmp1
        jsr         IO_WSTAT
        php
        pha
        lda         tmp1
        jsr         IO_CLOSE
        pla
        plp
        bcs         done
        bra         ok

; .A.Y = the name at .A.X (IO calls take names in .A.Y).  Preserves .A
nameay:
        pha
        txa
        tay
        pla
        rts

        .bss
stat:       .res    IO_STAT_SIZE
