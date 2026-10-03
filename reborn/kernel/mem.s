; ****************************************************************************
; mem.s - memory (BIOS ROM page 1: far calls; docs/reimplementation-from-scratch.md, §10.5).  The kernel manages
; pages and banks; finer allocation is the language runtimes' (malloc, ALLOT ...), which know their own needs.
;   Task RAM: BREAK moves the end of the program's data (from its module's HX_TOP up); PAGES_ALLOC and PAGES_FREE
;     give runs of 256-byte pages above it, from the top of its RAM down (a bitmap in its OS area: TA_PAGEMAP).  The top
;     is $7FFF ($7EFF in task F: the DS1747's registers are its $7FF8-$7FFF).
;   Banks: a task's banks are its own (16 a RAM module); BANKS_ALLOC and BANKS_FREE keep a bitmap in its OS area
;     (TA_BANKMAP) only so that libraries in one program don't collide.  A program selects a bank by writing $00.
;     The modules POST found bad are never given out.
;   Shared segments: runs of shared banks (IDs $80-$FF: U = ID >> 4, $00 = $F0 | ID & $0F), counted by the tasks
;     attached; the banks go back when the last one detaches or ends.  In the kernel task's tables (KCALLs).  The
;     shared chips POST found bad are never given out.
; The bitmaps: bit n is byte n >> 3's bit n & 7, in whichever task's memory is running the code (m_bit).

.include "kdefs.inc"

.segment "KCODE_P1"

; ****************************************************************************
; A task's start (from K_TASK_START and K_DRIVER_MAIN, in the new task): its break at its module's top (a RAM
; program's: set as it loaded, load.s), nothing given out.  Modifies .A, .X
K_MEM_START:
            lda         TA_MODBANK
            inc         a
            beq         :+
            lda         PROM_WINDOW + HX_TOP                ; (Its module's header, at $A000)
            sta         TA_BRK
            sta         TA_BRKMIN
            lda         PROM_WINDOW + HX_TOP + 1
            sta         TA_BRK + 1
            sta         TA_BRKMIN + 1
:
            ldx         #15
:
            stz         TA_PAGEMAP,X
            dex
            bpl         :-
            ldx         #29
:
            stz         TA_BANKMAP,X
            dex
            bpl         :-
            rts

; ****************************************************************************
; BREAK: move the end of the program's data.  IN: r0 = the new end (the first byte past it), or 0 to ask.
; OUT: r0 = the end; or C = 1, .A = E_INVAL (below the program's own data), E_NOMEM (on pages given out, or past
; the top of its RAM)
K_BRK:
            lda         r0
            ora         r0 + 1
            beq         @ask
            lda         r0                                  ; Not below its own data
            cmp         TA_BRKMIN
            lda         r0 + 1
            sbc         TA_BRKMIN + 1
            bcc         @inval
            lda         r0                                  ; K_TMP2: the first page past the new end
            cmp         #1
            lda         r0 + 1
            adc         #0
            sta         K_TMP2
            jsr         m_top                               ; Not past the top
            inc         a
            cmp         K_TMP2
            bcc         @nomem
            jsr         m_pagemap                           ; None of the pages it grows over given out
            jsr         m_brkpage
            sta         K_TASK
@page:
            lda         K_TASK
            cmp         K_TMP2
            bcs         @set
            jsr         m_bit
            bne         @nomem
            inc         K_TASK
            bra         @page

@set:
            lda         r0
            sta         TA_BRK
            lda         r0 + 1
            sta         TA_BRK + 1
@ask:
            lda         TA_BRK
            sta         r0
            lda         TA_BRK + 1
            sta         r0 + 1
            clc
            rts

@inval:
            FAIL        E_INVAL

@nomem:
            FAIL        E_NOMEM

; PAGES_ALLOC: a run of 256-byte pages, above the break, from the top down.  IN: .A = pages (1-127).
; OUT: r0 = the first one's address (.A = its page); or C = 1, .A = E_INVAL (0), E_NOMEM
K_PAGES_ALLOC:
            cmp         #0
            beq         @inval
            sta         K_CNT                               ; The pages wanted
            jsr         m_pagemap
            jsr         m_brkpage
            sta         K_TMP2                              ; The lowest it may give
            jsr         m_top
            sta         K_PTR                               ; K_PTR: the page looked at; K_PTR + 1: the run
            stz         K_PTR + 1                           ;   of free ones so far, down to it
@page:
            lda         K_PTR
            cmp         K_TMP2
            bcc         @nomem
            jsr         m_bit
            bne         @used
            inc         K_PTR + 1
            lda         K_PTR + 1
            cmp         K_CNT
            beq         @found
            bra         @down

@used:
            stz         K_PTR + 1
@down:
            dec         K_PTR
            bra         @page

@found:                                                     ; Pages K_PTR on: given out
            lda         K_PTR
            sta         K_TASK
:
            lda         K_TASK
            jsr         m_set
            inc         K_TASK
            dec         K_CNT
            bne         :-
            stz         r0
            lda         K_PTR
            sta         r0 + 1
            clc
            rts

@inval:
            FAIL        E_INVAL

@nomem:
            FAIL        E_NOMEM

; PAGES_FREE: give pages back.  IN: .A = pages; r0 = the first one's address.  OUT: C = 0; or C = 1, .A = E_INVAL
; (not pages PAGES_ALLOC gave this task)
K_PAGES_FREE:
            cmp         #0
            beq         @inval
            sta         K_CNT
            lda         r0
            bne         @inval
            jsr         m_pagemap
            lda         r0 + 1
            ldx         K_CNT
            jsr         m_check                             ; (Every one given out?)
            bcs         @inval
            lda         r0 + 1
            sta         K_TASK
:
            lda         K_TASK
            jsr         m_clr
            inc         K_TASK
            dec         K_CNT
            bne         :-
            clc
            rts

@inval:
            FAIL        E_INVAL

; ****************************************************************************
; BANKS: the banks this task has.  OUT: .A = how many (16 for each good RAM module); r0 = the good modules (bit =
; module: its banks are $m0-$mF)
K_BANKS:
            jsr         m_goodmods
            stz         K_TMP
            lda         r0
            jsr         @count
            lda         r0 + 1
            jsr         @count
            lda         K_TMP                               ; (15 at most: 240 banks)
            asl
            asl
            asl
            asl
            clc
            rts

@count:                                                     ; K_TMP + the bits set in .A
            lsr
            pha
            bcc         :+
            inc         K_TMP
:
            pla
            bne         @count
            rts

; BANKS_ALLOC: a run of this task's banks, from the lowest.  IN: .A = banks.  OUT: .A = the first (for $00); or
; C = 1, .A = E_INVAL (0), E_NOMEM
K_BANKS_ALLOC:
            cmp         #0
            beq         @inval
            sta         K_CNT
            jsr         m_goodmods
            jsr         m_bankmap
            stz         K_PTR                               ; K_PTR: the bank looked at; K_PTR + 1: the run of free
            stz         K_PTR + 1                           ;   ones so far, up to it
@bank:
            lda         K_PTR
            cmp         #SHARED_BANK
            bcs         @nomem
            jsr         m_modgood
            beq         @used
            lda         K_PTR
            jsr         m_bit
            bne         @used
            inc         K_PTR + 1
            lda         K_PTR + 1
            cmp         K_CNT
            beq         @found
            bra         @up

@used:
            stz         K_PTR + 1
@up:
            inc         K_PTR
            bra         @bank

@found:
            lda         K_PTR                               ; The first: K_PTR - K_CNT + 1
            sec
            sbc         K_CNT
            inc         a
            sta         K_PTR
            sta         K_TASK
:
            lda         K_TASK
            jsr         m_set
            inc         K_TASK
            dec         K_CNT
            bne         :-
            lda         K_PTR
            clc
            rts

@inval:
            FAIL        E_INVAL

@nomem:
            FAIL        E_NOMEM

; BANKS_FREE: give banks back.  IN: .A = the first; .X = how many.  OUT: C = 0; or C = 1, .A = E_INVAL (not banks
; BANKS_ALLOC gave this task)
K_BANKS_FREE:
            cpx         #0
            beq         @inval
            stx         K_CNT
            sta         K_TASK
            clc                                             ; (The run must end below the shared banks)
            adc         K_CNT
            bcs         @inval
            cmp         #SHARED_BANK + 1
            bcs         @inval
            jsr         m_bankmap
            lda         K_TASK
            jsr         m_check
            bcs         @inval
:
            lda         K_TASK
            jsr         m_clr
            inc         K_TASK
            dec         K_CNT
            bne         :-
            clc
            rts

@inval:
            FAIL        E_INVAL

; ****************************************************************************
; Shared segments: the calls (in the calling task), each a KCALL

; SEG_CREATE: a shared segment of .A banks (1-128), attached to this task.  OUT: .A = the segment; or C = 1,
; .A = E_INVAL (0), E_NOMEM
K_SEG_CREATE:
            KCALL_FAR   K_SEG_CREATE_K
            rts

; SEG_ATTACH: attach this task to segment .A (it stays while a task is attached).  OUT: C = 0; or C = 1,
; .A = E_INVAL (no such segment)
K_SEG_ATTACH:
            KCALL_FAR   K_SEG_ATTACH_K
            rts

; SEG_DETACH: detach this task from segment .A (the last to go frees it).  OUT: C = 0; or C = 1, .A = E_INVAL
; (not attached)
K_SEG_DETACH:
            KCALL_FAR   K_SEG_DETACH_K
            rts

; SEG_MAP: how to reach bank .X (0 on) of segment .A.  OUT: .A = the U value, .X = the $00 value; or C = 1,
; .A = E_INVAL (not attached), E_RANGE (no such bank in it)
K_SEG_MAP:
            KCALL_FAR   K_SEG_MAP_K
            rts

; ---- In the kernel task (KCALLs): .Y = the calling task

K_SEG_CREATE_K:
            cmp         #0
            beq         @inval
            cmp         #$100 - SEG_IDS + 1
            bcs         @nomem
            sta         K_CNT                               ; The banks wanted
            sty         K0_TMP2                             ; The caller
            ldx         #SEG_MAX - 1                        ; A free segment
:
            lda         K_SEG_COUNT,X
            beq         :+
            dex
            bpl         :-
            bra         @nomem
:
            stx         K0_TMP3
            jsr         m_shmap
            lda         #SEG_IDS                            ; A run of free IDs on good chips, from the lowest
            sta         K_PTR                               ; K_PTR: the ID looked at; K_PTR + 1: the run so far
            stz         K_PTR + 1
@id:
            lda         K_PTR
            beq         @nomem                              ; (Past $FF)
            lsr                                             ; Its chip: ID bits 2-3 (good?)
            lsr
            and         #3
            tax
            lda         M_BIT8,X
            and         K0_BADSHARED
            bne         @used
            lda         K_PTR
            sec
            sbc         #SEG_IDS
            jsr         m_bit
            bne         @used
            inc         K_PTR + 1
            lda         K_PTR + 1
            cmp         K_CNT
            beq         @found
            bra         @next

@used:
            stz         K_PTR + 1
@next:
            inc         K_PTR
            bra         @id

@found:
            lda         K_PTR                               ; The first: K_PTR - K_CNT + 1
            sec
            sbc         K_CNT
            inc         a
            ldx         K0_TMP3
            sta         K_SEG_FIRST,X
            sec
            sbc         #SEG_IDS
            sta         K_TASK
            lda         K_CNT
            sta         K_SEG_COUNT,X
            stz         K_SEG_REFS,X
:
            lda         K_TASK                              ; Its IDs: taken
            jsr         m_set
            inc         K_TASK
            dec         K_CNT
            bne         :-
            jsr         m_attach                            ; The caller attached
            lda         K0_TMP3
            clc
            rts

@inval:
            FAIL        E_INVAL

@nomem:
            FAIL        E_NOMEM

K_SEG_ATTACH_K:
            sty         K0_TMP2
            jsr         m_segment
            bcs         @inval
            jsr         m_attached
            bne         @done                               ; (Already)
            jsr         m_attach
@done:
            clc
            rts

@inval:
            FAIL        E_INVAL

K_SEG_DETACH_K:
            sty         K0_TMP2
            jsr         m_segment
            bcs         @inval
            jsr         m_attached
            beq         @inval
            jsr         m_detach
            clc
            rts

@inval:
            FAIL        E_INVAL

K_SEG_MAP_K:
            sty         K0_TMP2
            stx         K0_TMP
            jsr         m_segment
            bcs         @inval
            jsr         m_attached
            beq         @inval
            ldx         K0_TMP3
            lda         K0_TMP
            cmp         K_SEG_COUNT,X
            bcs         @range
            clc
            adc         K_SEG_FIRST,X                       ; Its ID
            pha
            and         #$0F
            ora         #SHARED_BANK
            tax                                             ; .X: for $00
            pla
            lsr
            lsr
            lsr
            lsr                                             ; .A: for U
            clc
            rts

@inval:
            FAIL        E_INVAL

@range:
            FAIL        E_RANGE

; A task's end (K_EXIT_K, FARCALL): task .Y detached from every segment it had
K_SEG_EXIT:
            sty         K0_TMP2
            lda         #SEG_MAX - 1
            sta         K0_TMP3
@segment:
            jsr         m_attached
            beq         :+
            jsr         m_detach
:
            dec         K0_TMP3
            bpl         @segment
            rts

; ---- The segments' helpers (in the kernel task): K0_TMP2 = the task, K0_TMP3 = the segment

; Segment .A in use?  OUT: C = 0, K0_TMP3 = it; or C = 1
m_segment:
            cmp         #SEG_MAX
            bcs         @no
            sta         K0_TMP3
            tax
            lda         K_SEG_COUNT,X
            beq         @no
            clc
            rts

@no:
            sec
            rts

; Z = 0 if the task is attached to the segment.  OUT: .X = the task, .A = the segment's bit, .Y = 0 (bits 0-7:
; K_SEGATT_LO) or 1 (_HI)
m_attached:
            lda         K0_TMP3
            ldy         #0
            cmp         #8
            bcc         :+
            iny
:
            and         #7
            tax
            lda         M_BIT8,X
            ldx         K0_TMP2
            cpy         #0
            bne         :+
            and         K_SEGATT_LO,X
            rts
:
            and         K_SEGATT_HI,X
            rts

; The task attached to the segment (it wasn't): a reference more
m_attach:
            jsr         m_attached
            lda         K0_TMP3
            and         #7
            tay
            lda         M_BIT8,Y
            ldy         K0_TMP3
            cpy         #8
            bcs         :+
            ora         K_SEGATT_LO,X
            sta         K_SEGATT_LO,X
            bra         @ref
:
            ora         K_SEGATT_HI,X
            sta         K_SEGATT_HI,X
@ref:
            ldx         K0_TMP3
            inc         K_SEG_REFS,X
            rts

; The task detached from the segment (it was): a reference less, and the last one frees it
m_detach:
            lda         K0_TMP3
            and         #7
            tay
            lda         M_BIT8,Y
            eor         #$FF
            ldx         K0_TMP2
            ldy         K0_TMP3
            cpy         #8
            bcs         :+
            and         K_SEGATT_LO,X
            sta         K_SEGATT_LO,X
            bra         @ref
:
            and         K_SEGATT_HI,X
            sta         K_SEGATT_HI,X
@ref:
            ldx         K0_TMP3
            dec         K_SEG_REFS,X
            bne         @done
            jsr         m_shmap                             ; The last: its IDs go back
            lda         K_SEG_FIRST,X
            sec
            sbc         #SEG_IDS
            sta         K_TASK
            lda         K_SEG_COUNT,X
            sta         K_CNT
            stz         K_SEG_COUNT,X
:
            lda         K_TASK
            jsr         m_clr
            inc         K_TASK
            dec         K_CNT
            bne         :-
@done:
            rts

; ****************************************************************************
; The helpers

; .A = the top page of this task's RAM
m_top:
            lda         T_REGISTER
            and         #TASKS - 1
            cmp         #RTC_TASK
            beq         :+
            lda         #$7F
            rts
:
            lda         #$7E
            rts

; .A = the first page past the break
m_brkpage:
            lda         TA_BRK
            cmp         #1                                  ; (C = 1: a part page)
            lda         TA_BRK + 1
            adc         #0
            rts

; K_PTR2 = a bitmap: this task's pages, its banks, or (in the kernel task) the shared IDs.  Keeps .A, .X, .Y
m_pagemap:
            pha
            lda         #<TA_PAGEMAP
            sta         K_PTR2
            lda         #>TA_PAGEMAP
            bra         m_map

m_bankmap:
            pha
            lda         #<TA_BANKMAP
            sta         K_PTR2
            lda         #>TA_BANKMAP
            bra         m_map

m_shmap:
            pha
            lda         #<K_SHMAP
            sta         K_PTR2
            lda         #>K_SHMAP
m_map:
            sta         K_PTR2 + 1
            pla
            rts

; Bit .A of the bitmap at (K_PTR2): Z = 1 if it's clear.  OUT: .Y = its byte, K_TMP = its mask.  Modifies .X
m_bit:
            pha
            lsr
            lsr
            lsr
            tay
            pla
            and         #7
            tax
            lda         M_BIT8,X
            sta         K_TMP
            and         (K_PTR2),Y
            rts

; Set bit .A (m_set), or clear it (m_clr).  Modifies .A, .X, .Y
m_set:
            jsr         m_bit
            lda         K_TMP
            ora         (K_PTR2),Y
            sta         (K_PTR2),Y
            rts

m_clr:
            jsr         m_bit
            lda         K_TMP
            eor         #$FF
            and         (K_PTR2),Y
            sta         (K_PTR2),Y
            rts

; .X bits from bit .A all set?  OUT: C = 0 if so, else C = 1.  Modifies .A, .X, .Y, K_PTR
m_check:
            sta         K_PTR
            stx         K_PTR + 1
:
            lda         K_PTR
            jsr         m_bit
            beq         @no
            inc         K_PTR
            beq         @no                                 ; (Past bit 255)
            dec         K_PTR + 1
            bne         :-
            clc
            rts

@no:
            sec
            rts

; r0 = the RAM modules installed and good (bit = module).  Modifies .A, .Y
m_goodmods:
            ldy         T_REGISTER
            php
            sei
            K0_GET      K0_BADMODS
            eor         #$FF
            sta         r0
            K0_GET      K0_BADMODS + 1
            eor         #$FF
            sta         r0 + 1
            K0_GET      K0_MODMASK
            and         r0
            sta         r0
            K0_GET      K0_MODMASK + 1
            and         r0 + 1
            sta         r0 + 1
            plp
            rts

; Bank .A's module good?  (Z = 1: no.)  IN: r0 = the good modules (m_goodmods).  Modifies .X
m_modgood:
            lsr
            lsr
            lsr
            lsr
            cmp         #8
            bcs         :+
            tax
            lda         M_BIT8,X
            and         r0
            rts
:
            and         #7
            tax
            lda         M_BIT8,X
            and         r0 + 1
            rts

.segment "KRODATA_P1"
M_BIT8:     .byte       $01, $02, $04, $08, $10, $20, $40, $80
