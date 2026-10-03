; ****************************************************************************
; du [-a] [name ...] - the kilobytes each name's tree takes (its files' lengths, each rounded up to a whole KB), a
; line for each directory in it, the deepest first ("12	/ram/u"), the name's own last; -a: a line for each file
; too.  None: the current directory.  A name that isn't there is said ("du: name: why"), and du ends with code 1.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "du", main

F_A             = $01           ; -a

.bss
path:       .res        PATH_MAX + 1                        ; The name, then each entry's in turn
st:         .res        SR_SIZE
sums:       .res        4 * (TL_DEPTH + 2)                  ; Each directory open's total so far (tl_depth's)

.code
main:
            jsr         tl_start
            LDR         tl_pp, path
            LDR         tl_vdir, enter
            LDR         tl_vpost, leave
            LDR         tl_vfile, file
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

; The name at r0: a file's kilobytes, or a directory's tree's
one:
            ldy         #$FF                                ; Its path
:
            iny
            cpy         #PATH_MAX + 1
            bcs         @long
            lda         (r0),Y
            sta         path,Y
            bne         :-
            LDR         r0, path
            LDR         r1, st
            jsr         STAT
            bcs         @failed
            lda         st + SR_QTYPE
            and         #QT_DIR
            bne         @dir
            LDR         r0, st                              ; A file: its own
            jsr         kb
            jmp         say

@dir:
            ldx         #1                                  ; A directory: its tree, at depth 1
            jsr         zero
            jsr         tl_walk
            ldx         #1
            jsr         load
            jmp         say

@long:
            lda         #E_NAMETOOLONG
@failed:
            pha
            LDR         r0, path
            pla
            jmp         tl_err

; A directory in the tree: its total from 0
enter:
            ldx         tl_depth
            inx
            jsr         zero
            clc
            rts

; After what's in it: its total said, and added to the one it's in
leave:
            ldx         tl_depth
            inx
            jsr         load
            jsr         add
            jmp         say

; A file in the tree (its record at r0): its kilobytes added to its directory's, and said with -a
file:
            jsr         kb
            jsr         add
            lda         tl_flags
            and         #F_A
            beq         :+
            jmp         say

:
            rts

; tl_num = the kilobytes of the file whose record is at r0: its length, rounded up
kb:
            clc
            ldy         #SR_LENGTH
            lda         (r0),Y
            adc         #<1023
            iny
            lda         (r0),Y
            adc         #>1023
            sta         tl_num
            iny
            lda         (r0),Y
            adc         #0
            sta         tl_num + 1
            iny
            lda         (r0),Y
            adc         #0
            sta         tl_num + 2
            stz         tl_num + 3
            ldx         #2                                  ; (/ 1024: the 1024s byte up, then / 4)
:
            lsr         tl_num + 2
            ror         tl_num + 1
            ror         tl_num
            dex
            bne         :-
            rts

; tl_num added to the total of the directory at tl_depth.  Keeps tl_num
add:
            lda         tl_depth
            asl
            asl
            tax
            clc
            lda         sums,X
            adc         tl_num
            sta         sums,X
            lda         sums + 1,X
            adc         tl_num + 1
            sta         sums + 1,X
            lda         sums + 2,X
            adc         tl_num + 2
            sta         sums + 2,X
            lda         sums + 3,X
            adc         tl_num + 3
            sta         sums + 3,X
            rts

; Total .X set to 0
zero:
            txa
            asl
            asl
            tax
            stz         sums,X
            stz         sums + 1,X
            stz         sums + 2,X
            stz         sums + 3,X
            rts

; tl_num = total .X
load:
            txa
            asl
            asl
            tax
            ldy         #0
:
            lda         sums,X
            sta         tl_num,Y
            inx
            iny
            cpy         #4
            bne         :-
            rts

; tl_num and the path, a line: "kilobytes<tab>path"
say:
            lda         tl_num                              ; (tl_dec changes tl_num: its value kept)
            pha
            lda         tl_num + 1
            pha
            lda         tl_num + 2
            pha
            lda         tl_num + 3
            pha
            lda         #1
            jsr         tl_dec
            lda         #TAB
            jsr         tl_putc
            LDR         r0, path
            jsr         tl_puts
            jsr         tl_nl
            pla
            sta         tl_num + 3
            pla
            sta         tl_num + 2
            pla
            sta         tl_num + 1
            pla
            sta         tl_num
            rts

.rodata
s_dot:      .byte       ".", 0
tl_name:    .byte       "du", 0
tl_flagset: .byte       "a", 0
tl_usage:   .byte       "du [-a] [name ...]", 0

.include "toollib.s"
