; ****************************************************************************
; ls [-ld] [name ...] - each name: a directory's entries (its stat records, read whole), or a file's own record;
; none: the current directory (.).  A line an entry: its name, and a / after a directory's.  -l: its mode, device
; and instance, length, time and name ("d-rwxrwxrwx f r     512 2000-01-01 00:00 lib"); -d: a directory itself,
; not its entries.  A name that isn't there is said ("ls: name: why"), and ls ends with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "ls", main

F_L             = $01           ; -l
F_D             = $02           ; -d

.zeropage
rec:        .res        2                                   ; An entry's stat record
name:       .res        2                                   ;   and the name shown for it

.bss
st:         .res        SR_SIZE
left:       .res        2                                   ; A directory's entries still to show
bits:       .res        1                                   ; (A mode's permissions, shifting out)

.code
main:
            jsr         tl_start
            lda         (tl_arg)
            bne         @arg
            LDR         r0, s_dot                           ; None: .
            jsr         one
            jmp         tl_end

@arg:
            MOVR        r0, tl_arg
            jsr         one
            jsr         tl_next
            bne         @arg
            jmp         tl_end

; The name at r0: a directory's entries, or its own
one:
            MOVR        name, r0
            LDR         r1, st
            jsr         STAT
            bcs         @failed
            LDR         rec, st
            lda         st + SR_QTYPE
            and         #QT_DIR
            beq         line                                ; (A file: its record, by the name given)
            lda         tl_flags
            and         #F_D
            bne         line
            MOVR        r0, name                            ; A directory: its entries
            jsr         tl_readdir
            bcs         @failed
            sta         left
            stx         left + 1
            MOVR        rec, r0
            lda         r0                                  ; (Its records' memory, for after)
            pha
            lda         r0 + 1
            pha
@entry:
            lda         left
            ora         left + 1
            beq         @done
            MOVR        name, rec                           ; (Its name: the record's own)
            jsr         line
            clc
            lda         rec
            adc         #SR_SIZE
            sta         rec
            bcc         :+
            inc         rec + 1
:
            lda         left
            bne         :+
            dec         left + 1
:
            dec         left
            bra         @entry

@done:
            pla
            sta         r0 + 1
            pla
            sta         r0
            jmp         tl_free

@failed:
            pha
            MOVR        r0, name
            pla
            jmp         tl_err

; An entry's line: record rec, by name
line:
            lda         tl_flags
            and         #F_L
            beq         @name
            ldy         #SR_QTYPE                           ; -l: its mode: d (a directory), a (append-only), or -
            lda         (rec),Y
            and         #QT_DIR
            beq         :+
            lda         #'d'
            bra         @type

:
            ldy         #SR_MODE + 1
            lda         (rec),Y
            and         #DM_APPEND
            beq         :+
            lda         #'a'
            bra         @type

:
            lda         #'-'
@type:
            jsr         tl_putc
            lda         #'-'
            jsr         tl_putc
            ldy         #SR_MODE                            ; ... rwxrwxrwx (its low 9 bits)
            lda         (rec),Y
            sta         bits
            iny
            lda         (rec),Y
            lsr         a                                   ; (C: bit 8)
            ldx         #0
@bit:
            lda         #'-'
            bcc         :+
            lda         s_rwx,X
:
            jsr         tl_putc
            inx
            cpx         #9
            beq         :+
            asl         bits                                ; (C: the next bit)
            bra         @bit

:
            jsr         tl_space
            ldy         #SR_DEV                             ; Its device and instance
            lda         (rec),Y
            jsr         tl_putc
            iny
            lda         (rec),Y
            bne         :+
            lda         #'-'
:
            jsr         tl_putc
            ldy         #SR_LENGTH + 3                      ; Its length
            ldx         #3
:
            lda         (rec),Y
            sta         tl_num,X
            dey
            dex
            bpl         :-
            lda         #9
            jsr         tl_dec
            jsr         tl_space
            clc                                             ; Its time
            lda         rec
            adc         #SR_MTIME
            sta         r0
            lda         rec + 1
            adc         #0
            sta         r0 + 1
            jsr         tl_date
            jsr         tl_space
@name:
            MOVR        r0, name                            ; Its name (a / after a directory's, but with -l)
            jsr         tl_puts
            lda         tl_flags
            and         #F_L
            bne         @nl
            ldy         #SR_QTYPE
            lda         (rec),Y
            and         #QT_DIR
            beq         @nl
            lda         #'/'
            jsr         tl_putc
@nl:
            jmp         tl_nl

.rodata
s_dot:      .byte       ".", 0
s_rwx:      .byte       "rwxrwxrwx"
tl_name:    .byte       "ls", 0
tl_flagset: .byte       "ld", 0
tl_usage:   .byte       "ls [-ld] [name ...]", 0

.include "toollib.s"
