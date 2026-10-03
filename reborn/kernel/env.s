; ****************************************************************************
; env.s - environments (BIOS ROM page 2: far calls; docs/reimplementation-from-scratch.md, §14.1).
;
; Each task has an environment of its own: ENV_SIZE bytes in the kernel task's RAM (K_ENV + the task * ENV_SIZE), its
; variables one after another, each its name's length (1-ENV_NAME_MAX), its name, its value's length (2) and its
; value (any bytes: rc's lists are their words with a 0 after each); a 0 ends them.  SPAWN gives a child a copy of
; its parent's (SPAWN_NOENV: an empty one); the boot's tasks start with empty ones.  The calls (ENV_GET, ENV_PUT,
; ENV_DEL, ENV_NAME) are KCALLs: the caller's side copies the name into its TA_SCRATCH, and the kernel task takes it,
; and the caller's r-registers, from there by kcopy and quick looks; values move by kcopy.  #e (kdev) serves them as
; files: /env/NAME.

.include "kdefs.inc"

.segment "KCODE_P2"

; ****************************************************************************
; The caller's side

; ENV_GET: a variable's value.  IN: .A = a task ($FF: this one); r0 = its name; r1 = a buffer; r2 = its size; r3 =
; an offset in the value.  OUT: .A/.X = the value's length from the offset (0: at its end or past it), as much of
; it as fits in the buffer; or C = 1, .A = E_NOENT, E_SRCH, E_INVAL, E_NAMETOOLONG
K_ENV_GET:
            jsr         e_arg
            bcs         :+
            KCALL_FAR   K_ENV_GET_K
:
            rts

; ENV_PUT: a variable set, or made.  IN: .A = a task ($FF: this one); r0 = its name; r1 = bytes; r2 = their count;
; r3 = where in its value they go (the value cut there first: 0 for a new value; its length at most).  OUT: C = 0;
; or C = 1, .A = E_NOMEM (no room: ENV_SIZE bytes an environment), E_INVAL (past the value's end; a bad name),
; E_NOENT (an offset, and no such variable), E_SRCH, E_NAMETOOLONG
K_ENV_PUT:
            jsr         e_arg
            bcs         :+
            KCALL_FAR   K_ENV_PUT_K
:
            rts

; ENV_DEL: a variable removed.  IN: .A = a task ($FF: this one); r0 = its name.  OUT: C = 0; or C = 1, .A =
; E_NOENT, E_SRCH, E_INVAL, E_NAMETOOLONG
K_ENV_DEL:
            jsr         e_arg
            bcs         :+
            KCALL_FAR   K_ENV_DEL_K
:
            rts

; ENV_NAME: a variable's name, by its place.  IN: .A = a task ($FF: this one); .X = which (0 on); r0 = a buffer
; (ENV_NAME_MAX + 1 bytes).  OUT: its name in the buffer, zero-terminated; .A/.X = its value's length; or C = 1, .A
; = E_NOENT (past the last), E_SRCH
K_ENV_NAME:
            KCALL_FAR   K_ENV_NAME_K
            rts

; r0's name into TA_SCRATCH: 1-ENV_NAME_MAX characters, no /.  OUT: C = 0; or C = 1, .A = E_INVAL, E_NAMETOOLONG.
; Keeps .A (C = 0)
e_arg:
            pha
            ldy         #0
@char:
            lda         (r0),Y
            sta         TA_SCRATCH,Y
            beq         @end
            cmp         #'/'
            beq         @inval
            iny
            cpy         #ENV_NAME_MAX + 1
            bne         @char
            pla
            FAIL        E_NAMETOOLONG

@end:
            pla
            cpy         #1                                  ; (C = 1: not empty)
            bcc         @empty
            clc
            rts

@inval:
            pla
@empty:
            FAIL        E_INVAL

; ****************************************************************************
; The kernel task's side (KCALLs: .Y = the caller)

; ENV_GET's: the value from the caller's r3 on, into its r1 buffer (r2 bytes at most)
K_ENV_GET_K:
            jsr         e_prep
            bcs         @done
            jsr         e_find
            bcs         @noent
            jsr         e_args
            sec                                             ; K_ENV_N: the value from the offset (none past its
            lda         K_ENV_EL                            ;   end) ...
            sbc         K_ENV_OFF
            sta         K_ENV_N
            lda         K_ENV_EL + 1
            sbc         K_ENV_OFF + 1
            sta         K_ENV_N + 1
            bcs         :+
            stz         K_ENV_N
            stz         K_ENV_N + 1
:
            lda         K_ENV_N                             ; K_CNT: that, or the buffer's size if less
            cmp         K_CNT
            lda         K_ENV_N + 1
            sbc         K_CNT + 1
            bcs         :+
            lda         K_ENV_N
            sta         K_CNT
            lda         K_ENV_N + 1
            sta         K_CNT + 1
:
            jsr         e_value                             ; K_PTR: the value, at the offset
            clc
            lda         K0_PTR2
            adc         K_ENV_OFF
            sta         K_PTR
            lda         K0_PTR2 + 1
            adc         K_ENV_OFF + 1
            sta         K_PTR + 1
            lda         K_ENV_SRC                           ; K_PTR2: the caller's buffer
            sta         K_PTR2
            lda         K_ENV_SRC + 1
            sta         K_PTR2 + 1
            lda         K_CNT
            ora         K_CNT + 1
            beq         :+
            lda         K0_EC
            clc
            FARCALL     K_KCOPY
:
            lda         K_ENV_N
            ldx         K_ENV_N + 1
            clc
@done:
            rts

@noent:
            FAIL        E_NOENT

; ENV_PUT's: the caller's r1 bytes (r2 of them) into the value at its r3 (the value cut there first)
K_ENV_PUT_K:
            jsr         e_prep
            bcs         @done
            jsr         e_args
            lda         K_CNT                               ; (K_ENV_N: the count)
            sta         K_ENV_N
            lda         K_CNT + 1
            sta         K_ENV_N + 1
            jsr         e_used
            jsr         e_find
            bcc         @set
            lda         K_ENV_OFF                           ; A new variable: from its start
            ora         K_ENV_OFF + 1
            beq         :+
            FAIL        E_NOENT
:
            jmp         e_new

@set:
            lda         K_ENV_EL                            ; Not past the value's end
            cmp         K_ENV_OFF
            lda         K_ENV_EL + 1
            sbc         K_ENV_OFF + 1
            bcs         :+
            FAIL        E_INVAL
:
            jmp         e_set

@done:
            rts

; ENV_DEL's: the variable removed (the ones after it moved down over it)
K_ENV_DEL_K:
            jsr         e_prep
            bcs         @done
            jsr         e_used
            jsr         e_find
            bcs         @noent
            jsr         e_value                             ; K0_PTR2: its end (its value's)
            clc
            lda         K0_PTR2
            adc         K_ENV_EL
            sta         K0_PTR2
            lda         K0_PTR2 + 1
            adc         K_ENV_EL + 1
            sta         K0_PTR2 + 1
            lda         K0_PTR                              ; To its start
            sta         K0_EP
            lda         K0_PTR + 1
            sta         K0_EP + 1
            jsr         e_tail
            jsr         e_move
            clc
@done:
            rts

@noent:
            FAIL        E_NOENT

; ENV_NAME's: variable .X's name, into the caller's r0 buffer; .A/.X its value's length
K_ENV_NAME_K:
            stx         K0_ENL                              ; (Which, for now)
            jsr         e_task
            bcs         @done
            lda         K_ENV_EB
            sta         K0_PTR
            lda         K_ENV_EB + 1
            sta         K0_PTR + 1
@entry:
            lda         (K0_PTR)
            beq         @noent
            jsr         e_vlen
            lda         K0_ENL
            beq         @this
            dec         K0_ENL
            jsr         e_skip
            bra         @entry

@this:
            lda         (K0_PTR)                            ; Its name, zero-terminated, into K_ENVNAME
            tax
            tay
            lda         #0
            sta         K_ENVNAME,X
:
            lda         (K0_PTR),Y
            sta         K_ENVNAME - 1,Y
            dey
            bne         :-
            inx                                             ; (And its 0)
            stx         K_CNT
            stz         K_CNT + 1
            lda         #<K_ENVNAME
            sta         K_PTR
            lda         #>K_ENVNAME
            sta         K_PTR + 1
            ldx         K0_EC
            ldy         T_REGISTER
            php
            sei
            QL_GET      r0
            sta         K_PTR2
            QL_GET      r0 + 1
            sta         K_PTR2 + 1
            plp
            lda         K0_EC
            clc
            FARCALL     K_KCOPY
            lda         K_ENV_EL
            ldx         K_ENV_EL + 1
            clc
@done:
            rts

@noent:
            FAIL        E_NOENT

; ****************************************************************************
; At SPAWN, and the boot

; A child's environment: a copy of its parent's, or empty (FARCALL from K_SPAWN_K: .X = the child, K0_TMP3 = its
; parent, K0_SPAWNF = SPAWN's flags).  Keeps .X
K_ENV_INHERIT:
            phx
            txa
            jsr         e_block
            sta         K0_EP + 1                           ; (To the child's)
            stz         K0_EP
            lda         K0_SPAWNF
            and         #SPAWN_NOENV
            bne         @empty
            lda         K0_TMP3                             ; From the parent's: its bytes in use
            jsr         e_block
            jsr         e_used
            lda         K_ENV_EB
            sta         K0_PTR2
            lda         K_ENV_EB + 1
            sta         K0_PTR2 + 1
            lda         K_ENV_U
            sta         K_ENV_MN
            lda         K_ENV_U + 1
            sta         K_ENV_MN + 1
            jsr         e_move
            plx
            rts

@empty:
            lda         #0
            sta         (K0_EP)
            plx
            rts

; Task .X's environment emptied (a task started at boot).  Keeps .X
K_ENV_CLEAR:
            txa
            jsr         e_block
            sta         K0_EP + 1
            stz         K0_EP
            lda         #0
            sta         (K0_EP)
            rts

; ****************************************************************************
; The pieces

; The KCALL's task (.A: $FF the caller, .Y) and the name in the caller's TA_SCRATCH: K0_EC = the caller, K_ENV_EB =
; the task's environment, K_ENVNAME = the name (K0_ENL its length).  OUT: C = 0; or C = 1, .A = E_SRCH
e_prep:
            jsr         e_task
            bcs         @done
            lda         #<K_ENVNAME                         ; The name
            sta         K_PTR
            lda         #>K_ENVNAME
            sta         K_PTR + 1
            lda         #<TA_SCRATCH
            sta         K_PTR2
            lda         #>TA_SCRATCH
            sta         K_PTR2 + 1
            lda         #ENV_NAME_MAX + 1
            sta         K_CNT
            stz         K_CNT + 1
            lda         K0_EC
            sec
            FARCALL     K_KCOPY
            ldx         #0
:
            lda         K_ENVNAME,X
            beq         :+
            inx
            bra         :-
:
            stx         K0_ENL
            clc
@done:
            rts

; The KCALL's task: .A ($FF: the caller, .Y), one in use.  OUT: K0_EC = the caller; K_ENV_EB = the task's
; environment; C = 0; or C = 1, .A = E_SRCH
e_task:
            sty         K0_EC
            cmp         #$FF
            bne         :+
            tya
:
            cmp         #TASKS
            bcs         @srch
            tax
            ldy         T_REGISTER
            php
            sei
            QL_GET      TK_STATE
            plp
            cmp         #ST_FREE
            beq         @srch
            txa
            jsr         e_block
            clc
            rts

@srch:
            FAIL        E_SRCH

; Task .A's environment: K_ENV_EB, and .A = its high byte
e_block:
            asl
            asl
            .assert     ENV_SIZE = 1024, error, "e_block: an environment is 4 pages"
            clc
            adc         #>K_ENV
            stz         K_ENV_EB
            sta         K_ENV_EB + 1
            rts

; The caller's r1 (K_ENV_SRC), r2 (K_CNT) and r3 (K_ENV_OFF), by quick looks
e_args:
            ldx         K0_EC
            ldy         T_REGISTER
            php
            sei
            QL_GET      r1
            sta         K_ENV_SRC
            QL_GET      r1 + 1
            sta         K_ENV_SRC + 1
            QL_GET      r2
            sta         K_CNT
            QL_GET      r2 + 1
            sta         K_CNT + 1
            QL_GET      r3
            sta         K_ENV_OFF
            QL_GET      r3 + 1
            sta         K_ENV_OFF + 1
            plp
            rts

; The variable named K_ENVNAME in the environment at K_ENV_EB.  OUT: C = 0, K0_PTR = its entry, K_ENV_EL = its
; value's length; or C = 1, K0_PTR = the environment's end (its 0)
e_find:
            lda         K_ENV_EB
            sta         K0_PTR
            lda         K_ENV_EB + 1
            sta         K0_PTR + 1
@entry:
            lda         (K0_PTR)
            beq         @none
            jsr         e_vlen
            lda         (K0_PTR)
            cmp         K0_ENL
            bne         @next
            tay
:
            lda         (K0_PTR),Y                          ; (Its name's byte y - 1)
            cmp         K_ENVNAME - 1,Y
            bne         @next
            dey
            bne         :-
            clc
            rts

@next:
            jsr         e_skip
            bra         @entry

@none:
            sec
            rts

; K_ENV_EL = the value's length of the entry at K0_PTR
e_vlen:
            lda         (K0_PTR)
            tay
            iny
            lda         (K0_PTR),Y
            sta         K_ENV_EL
            iny
            lda         (K0_PTR),Y
            sta         K_ENV_EL + 1
            rts

; K0_PTR past its entry (K_ENV_EL its value's length)
e_skip:
            jsr         e_value
            clc
            lda         K0_PTR2
            adc         K_ENV_EL
            sta         K0_PTR
            lda         K0_PTR2 + 1
            adc         K_ENV_EL + 1
            sta         K0_PTR + 1
            rts

; K0_PTR2 = the value of the entry at K0_PTR: past its name's length, its name and its value's length
e_value:
            lda         (K0_PTR)
            clc
            adc         #3
            adc         K0_PTR
            sta         K0_PTR2
            lda         K0_PTR + 1
            adc         #0
            sta         K0_PTR2 + 1
            rts

; K_ENV_U = the environment's bytes in use (at K_ENV_EB: its 0 too).  Modifies: K0_PTR, K_ENV_EL
e_used:
            lda         K_ENV_EB
            sta         K0_PTR
            lda         K_ENV_EB + 1
            sta         K0_PTR + 1
@entry:
            lda         (K0_PTR)
            beq         @end
            jsr         e_vlen
            jsr         e_skip
            bra         @entry

@end:
            sec                                             ; (Its 0: + 1)
            lda         K0_PTR
            sbc         K_ENV_EB
            sta         K_ENV_U
            lda         K0_PTR + 1
            sbc         K_ENV_EB + 1
            sta         K_ENV_U + 1
            inc         K_ENV_U
            bne         :+
            inc         K_ENV_U + 1
:
            rts

; K_ENV_MN = the bytes from K0_PTR2 to the environment's end (its 0 too: K_ENV_EB + K_ENV_U)
e_tail:
            clc
            lda         K_ENV_EB
            adc         K_ENV_U
            sta         K_ENV_MN
            lda         K_ENV_EB + 1
            adc         K_ENV_U + 1
            sta         K_ENV_MN + 1
            sec
            lda         K_ENV_MN
            sbc         K0_PTR2
            sta         K_ENV_MN
            lda         K_ENV_MN + 1
            sbc         K0_PTR2 + 1
            sta         K_ENV_MN + 1
            rts

; A new variable, at the environment's end (K0_PTR: its 0): its name (K_ENVNAME), K_ENV_N bytes of the caller's
; (K_ENV_SRC), and the 0 after it.  IN: K_ENV_U.  OUT: C = 0; or C = 1, .A = E_NOMEM
e_new:
            clc                                             ; Room?  U + its name's length + 3 + N
            lda         K0_ENL
            adc         #3
            adc         K_ENV_U
            sta         K_ENV_MN
            lda         K_ENV_U + 1
            adc         #0
            sta         K_ENV_MN + 1
            clc
            lda         K_ENV_MN
            adc         K_ENV_N
            sta         K_ENV_MN
            lda         K_ENV_MN + 1
            adc         K_ENV_N + 1
            sta         K_ENV_MN + 1
            bcs         e_nomem
            lda         #<ENV_SIZE
            cmp         K_ENV_MN
            lda         #>ENV_SIZE
            sbc         K_ENV_MN + 1
            bcc         e_nomem
            lda         K0_ENL                              ; Its name's length, its name, its value's length
            sta         (K0_PTR)
            tay
:
            lda         K_ENVNAME - 1,Y
            sta         (K0_PTR),Y
            dey
            bne         :-
            lda         K_ENV_N
            sta         K_ENV_EL
            ldy         K0_ENL
            iny
            sta         (K0_PTR),Y
            lda         K_ENV_N + 1
            sta         K_ENV_EL + 1
            iny
            sta         (K0_PTR),Y
            jsr         e_value                             ; Its value, then the 0
            clc
            lda         K0_PTR2
            adc         K_ENV_N
            sta         K0_EP
            lda         K0_PTR2 + 1
            adc         K_ENV_N + 1
            sta         K0_EP + 1
            lda         #0
            sta         (K0_EP)
            stz         K_ENV_OFF
            stz         K_ENV_OFF + 1
            jmp         e_in

; No room: the call fails, E_NOMEM
e_nomem:
            FAIL        E_NOMEM

; The variable at K0_PTR (its value K_ENV_EL long): cut at K_ENV_OFF, then K_ENV_N bytes of the caller's
; (K_ENV_SRC) there, the variables after it moved to fit.  IN: K_ENV_U.  OUT: C = 0; or C = 1, .A = E_NOMEM
e_set:
            clc                                             ; Room?  U - EL + OFF + N
            lda         K_ENV_U
            adc         K_ENV_OFF
            sta         K_ENV_MN
            lda         K_ENV_U + 1
            adc         K_ENV_OFF + 1
            sta         K_ENV_MN + 1
            clc
            lda         K_ENV_MN
            adc         K_ENV_N
            sta         K_ENV_MN
            lda         K_ENV_MN + 1
            adc         K_ENV_N + 1
            sta         K_ENV_MN + 1
            bcs         e_nomem
            sec
            lda         K_ENV_MN
            sbc         K_ENV_EL
            sta         K_ENV_MN
            lda         K_ENV_MN + 1
            sbc         K_ENV_EL + 1
            sta         K_ENV_MN + 1
            lda         #<ENV_SIZE
            cmp         K_ENV_MN
            lda         #>ENV_SIZE
            sbc         K_ENV_MN + 1
            bcc         e_nomem
            jsr         e_value                             ; The variables after it: from its value's end ...
            clc
            lda         K0_PTR2
            adc         K_ENV_OFF                           ;   (K0_EP: to the new value's end)
            sta         K0_EP
            lda         K0_PTR2 + 1
            adc         K_ENV_OFF + 1
            sta         K0_EP + 1
            clc
            lda         K0_EP
            adc         K_ENV_N
            sta         K0_EP
            lda         K0_EP + 1
            adc         K_ENV_N + 1
            sta         K0_EP + 1
            clc
            lda         K0_PTR2
            adc         K_ENV_EL
            sta         K0_PTR2
            lda         K0_PTR2 + 1
            adc         K_ENV_EL + 1
            sta         K0_PTR2 + 1
            jsr         e_tail
            jsr         e_move
            clc                                             ; Its value's length: OFF + N
            lda         K_ENV_OFF
            adc         K_ENV_N
            pha
            lda         K_ENV_OFF + 1
            adc         K_ENV_N + 1
            pha
            lda         (K0_PTR)
            tay
            iny
            iny
            pla
            sta         (K0_PTR),Y
            dey
            pla
            sta         (K0_PTR),Y
            ; (Falls into e_in)

; K_ENV_N bytes of the caller's (K_ENV_SRC) into the value of the entry at K0_PTR, at K_ENV_OFF.  OUT: C = 0
e_in:
            lda         K_ENV_N
            ora         K_ENV_N + 1
            beq         @done
            jsr         e_value
            clc
            lda         K0_PTR2
            adc         K_ENV_OFF
            sta         K_PTR
            lda         K0_PTR2 + 1
            adc         K_ENV_OFF + 1
            sta         K_PTR + 1
            lda         K_ENV_SRC
            sta         K_PTR2
            lda         K_ENV_SRC + 1
            sta         K_PTR2 + 1
            lda         K_ENV_N
            sta         K_CNT
            lda         K_ENV_N + 1
            sta         K_CNT + 1
            lda         K0_EC
            sec
            FARCALL     K_KCOPY
@done:
            clc
            rts

; K_ENV_MN bytes from K0_PTR2 to K0_EP, in the kernel task's RAM (they may overlap: when they move up over their
; own bytes, a byte at a time from the end; else a page at a time).  Modifies: .A, .X, .Y, K0_PTR2, K0_EP, K_ENV_MN
e_move:
            lda         K0_EP                               ; To higher, and the source's end past where they go?
            cmp         K0_PTR2
            lda         K0_EP + 1
            sbc         K0_PTR2 + 1
            bcc         @up
            clc
            lda         K0_PTR2
            adc         K_ENV_MN
            tax
            lda         K0_PTR2 + 1
            adc         K_ENV_MN + 1                        ; (.X/.A: the source's end)
            cmp         K0_EP + 1
            bne         :+
            cpx         K0_EP
:
            beq         @up
            bcs         @back
@up:
            ldy         #0
            ldx         K_ENV_MN + 1
            beq         @part
@page:
            lda         (K0_PTR2),Y
            sta         (K0_EP),Y
            iny
            bne         @page
            inc         K0_PTR2 + 1
            inc         K0_EP + 1
            dex
            bne         @page
@part:
            ldx         K_ENV_MN
            beq         @done
:
            lda         (K0_PTR2),Y
            sta         (K0_EP),Y
            iny
            dex
            bne         :-
@done:
            rts

@back:
            clc                                             ; Both ends past their last byte
            lda         K0_PTR2
            adc         K_ENV_MN
            sta         K0_PTR2
            lda         K0_PTR2 + 1
            adc         K_ENV_MN + 1
            sta         K0_PTR2 + 1
            clc
            lda         K0_EP
            adc         K_ENV_MN
            sta         K0_EP
            lda         K0_EP + 1
            adc         K_ENV_MN + 1
            sta         K0_EP + 1
@byte:
            lda         K_ENV_MN
            ora         K_ENV_MN + 1
            beq         @done
            lda         K0_PTR2
            bne         :+
            dec         K0_PTR2 + 1
:
            dec         K0_PTR2
            lda         K0_EP
            bne         :+
            dec         K0_EP + 1
:
            dec         K0_EP
            lda         (K0_PTR2)
            sta         (K0_EP)
            lda         K_ENV_MN
            bne         :+
            dec         K_ENV_MN + 1
:
            dec         K_ENV_MN
            bra         @byte
