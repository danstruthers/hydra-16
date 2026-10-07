; ****************************************************************************
; var.s - rc's variables and functions, in the heap: a record each (rc.inc: VF_*), the variables' values lists,
; a function's its body's text.  Every variable goes to the environment (Plan 9's way): a changed one (VF_DIRTY)
; is written before a program starts (var_export), each word with a 0 after it; a function as fn#name, its text.
; rc reads the environment as it starts (var_import).  A name is given as .A bytes at p1.

.include "rc.inc"

LS_MAX          = 16                                        ; Values saved for a moment, at most

.zeropage
vr:         .res        2                                   ; A record
vp:         .res        2                                   ; A pointer

.bss
vk:         .res        1                                   ; (var_find's: the kind, 0 or VF_FN)
vn:         .res        1                                   ;   the name's length
vi:         .res        1                                   ; (var_import's: which variable)
vars:       .res        2                                   ; The first record
vname:      .res        ENV_NAME_MAX + 4                    ; A name for the environment (fn#name)

.code

; The variable (or function: var_fn; not one removed) named .A bytes at p1.  OUT: C = 0, vr = its record; or
; C = 1
var_find:
            stz         vk
            bra         find

var_fn:
            ldx         #VF_FN
            stx         vk
            jsr         find
            bcs         :+
            ldy         #2
            lda         (vr),Y
            and         #VF_GONE
            cmp         #1                                  ; (C = 1: removed)
:
            rts

; The record of kind vk (0, or VF_FN: a removed function's too) named .A bytes at p1.  OUT: C = 0, vr; or C = 1
find:
            sta         vn
            MOVR        vr, vars
@record:
            lda         vr
            ora         vr + 1
            beq         @none
            ldy         #2                                  ; Its kind, and name
            lda         (vr),Y
            and         #VF_FN
            cmp         vk
            bne         @next
            ldy         #5
            lda         (vr),Y
            cmp         vn
            bne         @next
            tax
            beq         @found
            ldy         #0
:
            lda         (p1),Y
            iny
            iny
            iny
            iny
            iny
            iny
            cmp         (vr),Y
            bne         @next
            dey
            dey
            dey
            dey
            dey
            dex
            bne         :-
@found:
            clc
            rts

@next:
            ldy         #0
            lda         (vr),Y
            tax
            iny
            lda         (vr),Y
            sta         vr + 1
            stx         vr
            bra         @record

@none:
            sec
            rts

; The value of the variable named .A bytes at p1: .A/.X = its list (the empty one if it isn't set)
var_get:
            jsr         var_find
            bcs         @empty
            ldy         #3
            lda         (vr),Y
            tax
            iny
            lda         (vr),Y
            bne         :+
            cpx         #0
            beq         @empty
:
            pha
            txa
            plx
            rts

@empty:
            lda         #<empty_list
            ldx         #>empty_list
            rts

; A record for the name (.A bytes at p1) of kind vk, made if there isn't one (its value none).  OUT: vr
record:
            jsr         find
            bcc         @done
            lda         vn                                  ; A new one: at the front
            clc
            adc         #6
            ldx         #0
            jsr         heap_alloc
            sta         vr
            stx         vr + 1
            ldy         #0
            lda         vars
            sta         (vr),Y
            iny
            lda         vars + 1
            sta         (vr),Y
            iny
            lda         vk
            sta         (vr),Y
            iny
            lda         #0
            sta         (vr),Y
            iny
            sta         (vr),Y
            iny
            lda         vn
            sta         (vr),Y
            tax
            beq         :++
            ldy         #0
:
            lda         (p1),Y
            iny
            iny
            iny
            iny
            iny
            iny
            sta         (vr),Y
            dey
            dey
            dey
            dey
            dey
            dex
            bne         :-
:
            MOVR        vars, vr
@done:
            rts

; The variable named .A bytes at p1 set to the list at p2 (copied into the heap; its old value freed).  Keeps vr
; (its record)
var_set:
            stz         vk
            jsr         record
            jsr         free_value
            lda         p2
            ldx         p2 + 1
            jsr         list_copy_heap
; Variable vr's value: the list .A/.X (in the heap), marked changed (for the environment)
set_value:
            ldy         #3
            sta         (vr),Y
            iny
            txa
            sta         (vr),Y
            ldy         #2                                  ; Changed: for the environment
            lda         (vr),Y
            ora         #VF_DIRTY
            and         #<~VF_GONE
            sta         (vr),Y
            rts

; The variable named .A bytes at p1 set to one word: .X bytes at p2
var_set_word:
            pha
            MOVR        p0, p1                              ; (The name, a moment)
            MOVR        p1, p2
            txa
            jsr         list_alloc_word
            sta         p2
            stx         p2 + 1
            MOVR        p1, p0
            pla
            jmp         var_set

; Record vr's value freed (none now)
free_value:
            ldy         #3
            lda         (vr),Y
            tax
            iny
            lda         (vr),Y
            bne         :+
            cpx         #0
            beq         @done
:
            pha
            txa
            plx
            jsr         heap_free
            ldy         #3
            lda         #0
            sta         (vr),Y
            iny
            sta         (vr),Y
@done:
            rts

; The function named .A bytes at p1: its body's text, .X/.Y bytes at p2 (copied into the heap: its length, then it)
fn_set:
            phy
            phx
            ldx         #VF_FN
            stx         vk
            jsr         record
            jsr         free_value
            plx                                             ; (Its length)
            ply
            stx         vp
            sty         vp + 1
            clc
            txa
            adc         #2
            pha
            tya
            adc         #0
            tax
            pla
            jsr         heap_alloc
            sta         p0
            stx         p0 + 1
            lda         vp
            sta         (p0)
            ldy         #1
            lda         vp + 1
            sta         (p0),Y
            clc                                             ; Its text, after its length
            lda         p0
            adc         #2
            sta         p3
            lda         p0 + 1
            adc         #0
            sta         p3 + 1
@copy:
            lda         vp
            ora         vp + 1
            beq         @copied
            lda         (p2)
            sta         (p3)
            inc         p2
            bne         :+
            inc         p2 + 1
:
            inc         p3
            bne         :+
            inc         p3 + 1
:
            lda         vp
            bne         :+
            dec         vp + 1
:
            dec         vp
            bra         @copy

@copied:
            lda         p0
            ldx         p0 + 1
            jmp         set_value

; The function named .A bytes at p1 removed (gone: from the environment too, next time)
fn_del:
            jsr         var_fn
            bcs         @done
            jsr         free_value
            ldy         #2
            lda         (vr),Y
            ora         #VF_GONE | VF_DIRTY
            sta         (vr),Y
@done:
            rts

; ****************************************************************************
; Values for a moment (x=v cmd's, a function's $*): saved on a stack, put back by var_local_restore

; .A = the stack's depth now (a mark)
var_local_mark:
            lda         ls_depth
            rts

; The variable named .A bytes at p1: its value and flags saved (its value none now: var_local_set gives it one)
var_local_save:
            stz         vk
            jsr         record
            ldx         ls_depth
            cpx         #LS_MAX
            bcc         :+
            LDR         r0, s_deep
            jmp         rc_error
:
            lda         vr
            sta         ls_recl,X
            lda         vr + 1
            sta         ls_rech,X
            ldy         #2
            lda         (vr),Y
            sta         ls_flags,X
            iny
            lda         (vr),Y
            sta         ls_vall,X
            iny
            lda         (vr),Y
            sta         ls_valh,X
            lda         #0                                  ; (Its value: kept here, not freed)
            sta         (vr),Y
            dey
            sta         (vr),Y
            inc         ls_depth
            rts

; The variable last saved set to the list at p2
var_local_set:
            ldx         ls_depth
            lda         ls_recl - 1,X
            sta         vr
            lda         ls_rech - 1,X
            sta         vr + 1
            lda         p2
            ldx         p2 + 1
            jsr         list_copy_heap
            jmp         set_value

; The values saved since mark .A put back (each variable's value meanwhile freed; changed: for the environment)
var_local_restore:
            sta         vn
@one:
            lda         ls_depth
            cmp         vn
            beq         @done
            bcc         @done
            dec         ls_depth
            ldx         ls_depth
            lda         ls_recl,X
            sta         vr
            lda         ls_rech,X
            sta         vr + 1
            jsr         free_value
            ldx         ls_depth
            ldy         #3
            lda         ls_vall,X
            sta         (vr),Y
            iny
            lda         ls_valh,X
            sta         (vr),Y
            ldy         #2
            lda         ls_flags,X
            ora         #VF_DIRTY
            sta         (vr),Y
            bra         @one

@done:
            rts

; ****************************************************************************
; The environment

; Every changed variable and function to the environment (ENV_PUT, or ENV_DEL for an empty list or a function
; gone).  Errors (a full environment): said, and the rest go on
var_export:
            MOVR        vr, vars
@record:
            lda         vr
            ora         vr + 1
            bne         :+
            rts
:
            ldy         #2
            lda         (vr),Y
            and         #VF_DIRTY
            beq         @next
            lda         (vr),Y
            and         #<~VF_DIRTY
            sta         (vr),Y
            jsr         env_name                            ; vname: its name in the environment
            jsr         arena_mark
            pha
            phx
            jsr         env_value                           ; p2: its value's bytes, .A/.X of them (C = 1: none)
            bcs         @del
            sta         r2
            stx         r2 + 1
            LDR         r0, vname
            MOVR        r1, p2
            stz         r3
            stz         r3 + 1
            lda         #$FF
            jsr         ENV_PUT
            bcc         @done
            pha                                             ; "rc: env name: why"
            LDR         r0, s_env
            jsr         out2s
            LDR         r0, vname
            jsr         out2s
            pla
            jsr         say_error
            bra         @done

@del:
            LDR         r0, vname
            lda         #$FF
            jsr         ENV_DEL
@done:
            plx
            pla
            jsr         arena_release
@next:
            ldy         #0
            lda         (vr),Y
            tax
            iny
            lda         (vr),Y
            sta         vr + 1
            stx         vr
            jmp         @record

; vname: record vr's name in the environment (a function's: fn#name), zero-terminated
env_name:
            ldx         #0
            ldy         #2
            lda         (vr),Y
            and         #VF_FN
            beq         :+
            lda         #'f'
            sta         vname
            lda         #'n'
            sta         vname + 1
            lda         #'#'
            sta         vname + 2
            ldx         #3
:
            ldy         #5
            lda         (vr),Y
            sta         vn
            ldy         #6
@char:
            lda         vn
            beq         @end
            cpx         #ENV_NAME_MAX
            bcs         @end
            lda         (vr),Y
            sta         vname,X
            inx
            iny
            dec         vn
            bra         @char

@end:
            stz         vname,X
            rts

; Record vr's value as the environment has it: a variable's words each with a 0 after it (in the arena); a
; function's text.  OUT: p2 = it, .A/.X its length; or C = 1: nothing (an empty list, or a function gone)
env_value:
            ldy         #3
            lda         (vr),Y
            sta         vp
            iny
            lda         (vr),Y
            sta         vp + 1
            ora         vp
            bne         :+
            jmp         @none
:
            ldy         #2
            lda         (vr),Y
            and         #VF_FN
            beq         @var
            clc                                             ; A function: its text, after its length
            lda         vp
            adc         #2
            sta         p2
            lda         vp + 1
            adc         #0
            sta         p2 + 1
            ldy         #1
            lda         (vp),Y
            tax
            lda         (vp)
            clc
            rts

@var:
            lda         (vp)
            cmp         #LIST_END
            beq         @none
            jsr         list_start                          ; Its words, a 0 after each
            sta         p2
            stx         p2 + 1
            stz         num
            stz         num + 1
@word:
            lda         (vp)
            cmp         #LIST_END
            beq         @end
            pha
            sec
            adc         num                                 ; (num += its length + 1)
            sta         num
            bcc         :+
            inc         num + 1
:
            pla
            pha
            clc
            adc         #1
            ldx         #0
            bcc         :+
            inx
:
            jsr         arena_alloc
            sta         p0
            stx         p0 + 1
            pla
            tax
            ldy         #0
            cpx         #0
            beq         @zero
:
            iny
            lda         (vp),Y
            dey
            sta         (p0),Y
            iny
            dex
            bne         :-
@zero:
            lda         #0
            sta         (p0),Y
            lda         (vp)                                ; Past the word
            sec
            adc         vp
            sta         vp
            bcc         @word
            inc         vp + 1
            bra         @word

@end:
            lda         num
            ldx         num + 1
            clc
            rts

@none:
            sec
            rts

; The environment, read in: each variable a list (its 0s ending its words), each fn#name a function
var_import:
            stz         vi
@next:
            jsr         arena_mark
            pha
            phx
            LDR         r0, vname
            lda         #$FF
            ldx         vi
            jsr         ENV_NAME                            ; .A/.X: its length
            bcc         :+
            jmp         @done
:
            sta         num
            stx         num + 1
            jsr         arena_alloc                         ; Its value, into the arena
            sta         p2
            stx         p2 + 1
            LDR         r0, vname
            MOVR        r1, p2
            MOVR        r2, num
            stz         r3
            stz         r3 + 1
            lda         #$FF
            jsr         ENV_GET
            LDR         p0, vname
            jsr         str_len
            sta         vn
            lda         vname                               ; fn#: a function
            cmp         #'f'
            bne         @var
            lda         vname + 1
            cmp         #'n'
            bne         @var
            lda         vname + 2
            cmp         #'#'
            bne         @var
            LDR         p1, vname + 3
            lda         vn
            sec
            sbc         #3
            ldx         num
            ldy         num + 1
            jsr         fn_set
            bra         @clean

@var:
            jsr         split0                              ; p2: its words, a list
            LDR         p1, vname
            lda         vn
            jsr         var_set
@clean:
            ldy         #2                                  ; (As the environment has it)
            lda         (vr),Y
            and         #<~VF_DIRTY
            sta         (vr),Y
            plx
            pla
            jsr         arena_release
            inc         vi
            jmp         @next

@done:
            plx
            pla
            jmp         arena_release

; The num bytes at p2 (words, each with a 0 after it; the last may lack its 0) as a list in the arena.  OUT: p2 =
; it
split0:
            MOVR        p3, p2                              ; p3: the next word
            jsr         list_start
            pha
            phx
@word:
            lda         num
            ora         num + 1
            beq         @end
            ldy         #0                                  ; Its length: to its 0, the end, or WORD_MAX
@len:
            cpy         #WORD_MAX
            beq         @got
            lda         num + 1
            bne         :+
            cpy         num
            beq         @got
:
            lda         (p3),Y
            beq         @got
            iny
            bra         @len

@got:
            sty         t1
            MOVR        p1, p3
            tya
            jsr         list_append_word
            clc                                             ; Past it
            lda         p3
            adc         t1
            sta         p3
            bcc         :+
            inc         p3 + 1
:
            sec
            lda         num
            sbc         t1
            sta         num
            bcs         :+
            dec         num + 1
:
            lda         num
            ora         num + 1
            beq         @end
            lda         (p3)                                ; And its 0 (none: WORD_MAX's, the rest the next)
            bne         @word
            inc         p3
            bne         :+
            inc         p3 + 1
:
            lda         num
            bne         :+
            dec         num + 1
:
            dec         num
            bra         @word

@end:
            jsr         list_end
            plx
            pla
            sta         p2
            stx         p2 + 1
            rts

.bss
ls_recl:    .res        LS_MAX                              ; The saved values: each one's record ...
ls_rech:    .res        LS_MAX
ls_vall:    .res        LS_MAX                              ;   its value ...
ls_valh:    .res        LS_MAX
ls_flags:   .res        LS_MAX                              ;   and its flags
ls_depth:   .res        1

.rodata
s_deep:     .byte       "too deep", 0
empty_list: .byte       LIST_END
s_env:      .byte       "rc: env ", 0
