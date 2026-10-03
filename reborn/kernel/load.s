; ****************************************************************************
; load.s - SPAWN and the loader (BIOS ROM page 3: far calls; docs/reimplementation-from-scratch.md, §10.6, §11).
;
; SPAWN runs in the calling task: it opens the program's file through the caller's namespace, reads its HYX2 header
; (into TA_PATH) and checks it, then has the kernel task set a task up for it (task.s: K_SPAWN_K), its fds the
; map's.  A module of the paged ROM (HF_INPLACE: #m's files) runs in place: the kernel task finds it in the module
; directory by its name and sets it up as at boot.  Any other program is a RAM program: its task gets the file as its
; fd LOAD_FD and starts at K_TASK_LOAD (task.s), loading itself (K_LOAD): its image read to its load address, the
; file closed, its BSS cleared, its entry and break set; then it starts as every program does.  So SPAWN doesn't
; wait for the image, and the file's server copies it straight into the child's RAM.

.include "kdefs.inc"

.segment "KCODE_P3"

; SPAWN: start a program.  IN: r0 = its path; r1 = its arguments (zero-terminated, up to 175 characters), or 0;
; .A = flags (SPAWN_*); with SPAWN_FDMAP, r2 = an fd map (a count, SPAWN_FDS at most, then the caller's fd for each
; of the child's from 0, $FF for none; without it, fds 0, 1 and 2).  OUT: C = 0, .A = the task; or C = 1, .A =
; E_TOOBIG, E_INVAL, E_NOEXEC, E_NOTASK, or OPEN's and READ's errors
K_SPAWN:
            and         #<~SPAWN_LOAD                       ; (The kernel's own)
            sta         L_FLAGS
            lda         r1
            sta         L_ARGS
            lda         r1 + 1
            sta         L_ARGS + 1
            stz         L_ARGLEN                            ; Its arguments' length with the 0 (checked first)
            ora         r1
            beq         @map
            ldy         #0
:
            lda         (r1),Y
            beq         :+
            iny
            cpy         #TA_ARGS_MAX + 1
            bne         :-
            FAIL        E_TOOBIG

:
            iny
            sty         L_ARGLEN
@map:                                                       ; The fd map, into TA_SCRATCH + SP_MAP
            ldx         #FD_MAX - 1
            lda         #$FF
:
            sta         TA_SCRATCH + SP_MAP,X
            dex
            bpl         :-
            lda         L_FLAGS
            and         #SPAWN_FDMAP
            bne         @given
            stz         TA_SCRATCH + SP_MAP                 ; (None: 0, 1 and 2)
            lda         #1
            sta         TA_SCRATCH + SP_MAP + 1
            inc         a
            sta         TA_SCRATCH + SP_MAP + 2
            bra         @open

@given:
            lda         (r2)
            cmp         #SPAWN_FDS + 1
            bcc         :+
            FAIL        E_INVAL

:
            tay
            beq         @open
:
            lda         (r2),Y
            sta         TA_SCRATCH + SP_MAP - 1,Y
            dey
            bne         :-
@open:
            jsr         l_header                            ; The file (L_FD), its header in TA_PATH
            bcc         :+
            rts
:
            ldx         #HX_NAME_MAX                        ; Its name, for the kernel task (and ended)
            stz         TA_SCRATCH + SP_NAME,X
:
            dex
            bmi         :+
            lda         TA_PATH + HX_NAME,X
            sta         TA_SCRATCH + SP_NAME,X
            bra         :-
:
            lda         TA_PATH + HX_FLAGS
            and         #HF_INPLACE
            bne         @start
            lda         L_FD                                ; A RAM program: the file is its LOAD_FD too
            sta         TA_SCRATCH + SP_MAP + LOAD_FD
            lda         #SPAWN_LOAD
            tsb         L_FLAGS
@start:
            lda         L_FLAGS
            KCALL_FAR   K_SPAWN_K                           ; .A = the task, set up (task.s)
            jsr         l_close                             ; (Ours: a RAM program's LOAD_FD is its own)
            bcc         :+
            rts
:
            sta         L_TASK
            lda         L_ARGLEN                            ; Its arguments, into its TA_ARGS
            beq         @noargs
            sta         K_CNT
            stz         K_CNT + 1
            lda         L_ARGS
            sta         K_PTR
            lda         L_ARGS + 1
            sta         K_PTR + 1
            lda         #<TA_ARGS
            sta         K_PTR2
            lda         #>TA_ARGS
            sta         K_PTR2 + 1
            lda         L_TASK
            clc
            FARCALL     K_KCOPY
            bra         @go

@noargs:
            ldx         L_TASK
            ldy         T_REGISTER
            php
            sei
            lda         #0
            QL_PUT      TA_ARGS
            plp
@go:
            lda         L_TASK
            FARCALL     K_TASK_GO
            clc
            rts

; The file at r0 opened for reading (L_FD), its header read into TA_PATH and checked (l_check).  OUT: C = 0; or
; C = 1, .A = the error (the file closed)
l_header:
            lda         #O_READ
            FARCALL     K_OPEN
            bcs         l_done
            sta         L_FD
            jsr         l_rdhead
            bcs         l_close
            rts

; L_FD closed.  Keeps .A and C
l_close:
            php
            pha
            lda         L_FD
            FARCALL     K_CLOSE
            pla
            plp
l_done:
            rts

; Fd L_FD's next HX_SIZE bytes into TA_PATH: a header, checked (l_check).  OUT: C = 0; or C = 1, .A = the error
l_rdhead:
            lda         #<TA_PATH
            sta         r0
            lda         #>TA_PATH
            sta         r0 + 1
            lda         #HX_SIZE
            sta         r1
            stz         r1 + 1
            jsr         l_read
            bcs         @done
            cmp         #HX_SIZE
            bne         @noexec
            txa
            bne         @noexec
            jmp         l_check

@noexec:
            lda         #E_NOEXEC
            sec
@done:
            rts

; READ from fd L_FD.  IN, OUT: READ's
l_read:
            lda         L_FD
            FARCALL     K_READ
            rts

; In a RAM program's new task (K_TASK_LOAD, its first instructions: task.s), the program loaded from its file (its
; fd LOAD_FD, SPAWN's): its header read into TA_PATH and checked again (the file may have changed), its image read
; to its load address (the header copied there first: its image's start), the file closed, its BSS cleared, its
; entry and its break set (K_MEM_START leaves a RAM program's).  OUT: C = 0; or C = 1, .A = the error (E_NOEXEC:
; not a program, or it ended too soon)
K_LOAD:
            lda         #LOAD_FD
            sta         L_FD
            stz         r0                                  ; From the start
            stz         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #0
            FARCALL     K_SEEK
            bcs         @done
            jsr         l_rdhead                            ; Its header
            bcs         @done
            jsr         l_image
            bcs         @done
            jsr         l_close
            jsr         l_bss
            lda         TA_PATH + HX_MAIN                   ; Its entry, its break
            sta         TA_ENTRY
            lda         TA_PATH + HX_MAIN + 1
            sta         TA_ENTRY + 1
            lda         TA_PATH + HX_TOP
            sta         TA_BRK
            sta         TA_BRKMIN
            lda         TA_PATH + HX_TOP + 1
            sta         TA_BRK + 1
            sta         TA_BRKMIN + 1
            clc
@done:
            rts

; The image (its header in TA_PATH): the header to its load address, then the rest read from L_FD after it.
; OUT: C = 0; or C = 1, .A = the error (E_NOEXEC: the file ended too soon)
l_image:
            lda         TA_PATH + HX_LOAD
            sta         L_AT
            lda         TA_PATH + HX_LOAD + 1
            sta         L_AT + 1
            ldy         #HX_SIZE - 1
:
            lda         TA_PATH,Y
            sta         (L_AT),Y
            dey
            bpl         :-
            clc
            lda         L_AT
            adc         #HX_SIZE
            sta         L_AT
            bcc         :+
            inc         L_AT + 1
:
            sec
            lda         TA_PATH + HX_LENGTH
            sbc         #HX_SIZE
            sta         L_LEFT
            lda         TA_PATH + HX_LENGTH + 1
            sbc         #0
            sta         L_LEFT + 1
@read:
            lda         L_LEFT
            ora         L_LEFT + 1
            beq         @done                               ; (C = 1 from the sbc ... )
            lda         L_AT
            sta         r0
            lda         L_AT + 1
            sta         r0 + 1
            lda         L_LEFT
            sta         r1
            lda         L_LEFT + 1
            sta         r1 + 1
            jsr         l_read
            bcs         @failed
            sta         r2                                  ; (The count read: none is the file's end, too soon)
            stx         r2 + 1
            ora         r2 + 1
            beq         @short
            clc
            lda         L_AT
            adc         r2
            sta         L_AT
            lda         L_AT + 1
            adc         r2 + 1
            sta         L_AT + 1
            sec
            lda         L_LEFT
            sbc         r2
            sta         L_LEFT
            lda         L_LEFT + 1
            sbc         r2 + 1
            sta         L_LEFT + 1
            bra         @read

@done:
            clc
            rts

@short:
            lda         #E_NOEXEC
            sec
@failed:
            rts

; The BSS (its header in TA_PATH) cleared: whole pages, then the rest
l_bss:
            lda         TA_PATH + HX_BSS
            sta         L_AT
            lda         TA_PATH + HX_BSS + 1
            sta         L_AT + 1
            ldy         #0
            lda         #0
            ldx         TA_PATH + HX_BSS_LEN + 1
            beq         @part
@page:
            sta         (L_AT),Y
            iny
            bne         @page
            inc         L_AT + 1
            dex
            bne         @page
@part:
            ldx         TA_PATH + HX_BSS_LEN
            beq         @done
:
            sta         (L_AT),Y
            iny
            dex
            bne         :-
@done:
            rts

; The HYX2 header at TA_PATH: a program this kernel runs?  One in place (the kernel task finds it by its name), or a
; RAM program all of whose RAM is this task's program RAM: its image from PROG_LOAD up, its data where it loads
; (the image is read, not copied), its BSS from PROG_RAM up, each no higher than its top, and its top no higher than
; this task's RAM's (task F's stops below the DS1747's registers).  OUT: C = 0; or C = 1, .A = E_NOEXEC.
; Modifies .A, .X, .Y, L_AT
l_check:
            ldx         #3
:
            lda         TA_PATH + HX_MAGIC,X
            cmp         l_s_hyx2,X
            bne         @no
            dex
            bpl         :-
            lda         TA_PATH + HX_ABI                    ; (Not built for a later kernel)
            beq         @no
            cmp         #ABI_VERSION + 1
            bcs         @no
            lda         TA_PATH + HX_TYPE
            cmp         #HT_PROGRAM
            bne         @no
            lda         TA_PATH + HX_FLAGS
            and         #HF_INPLACE
            bne         @yes
            lda         TA_PATH + HX_LOAD + 1               ; A RAM program
            cmp         #>PROG_LOAD
            bcc         @no
            lda         TA_PATH + HX_LENGTH + 1             ; (Its header, at least)
            bne         :+
            lda         TA_PATH + HX_LENGTH
            cmp         #HX_SIZE
            bcc         @no
:
            lda         TA_PATH + HX_DATA_LEN               ; Its data: where it loads
            ora         TA_PATH + HX_DATA_LEN + 1
            beq         :+
            lda         TA_PATH + HX_DATA_LOAD
            cmp         TA_PATH + HX_DATA_RUN
            bne         @no
            lda         TA_PATH + HX_DATA_LOAD + 1
            cmp         TA_PATH + HX_DATA_RUN + 1
            bne         @no
:
            lda         TA_PATH + HX_BSS + 1
            cmp         #>PROG_RAM
            bcc         @no
            ldx         #HX_LOAD                            ; Its image and its BSS, below its top
            ldy         #HX_LENGTH
            jsr         l_below
            bcc         @no
            ldx         #HX_BSS
            ldy         #HX_BSS_LEN
            jsr         l_below
            bcc         @no
            lda         T_REGISTER                          ; Its top, at this task's RAM's at most
            and         #TASKS - 1
            cmp         #RTC_TASK
            lda         #>$8000
            bcc         :+
            lda         #>RTC_REGS
:
            cmp         TA_PATH + HX_TOP + 1
            bcc         @no
            bne         @yes
            lda         TA_PATH + HX_TOP
            bne         @no
@yes:
            clc
            rts

@no:
            FAIL        E_NOEXEC

; C = 1 if the area at TA_PATH + .X (2), TA_PATH + .Y (2) bytes long, ends at the header's top or below
l_below:
            clc
            lda         TA_PATH,X
            adc         TA_PATH,Y
            sta         L_AT
            lda         TA_PATH + 1,X
            adc         TA_PATH + 1,Y
            sta         L_AT + 1
            bcs         @past                               ; (Past $FFFF)
            lda         TA_PATH + HX_TOP
            cmp         L_AT
            lda         TA_PATH + HX_TOP + 1
            sbc         L_AT + 1
            rts

@past:
            clc
            rts

.segment "KRODATA_P3"
l_s_hyx2:   .byte       "HYX2"
