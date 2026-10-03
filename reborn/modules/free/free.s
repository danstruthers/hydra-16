; ****************************************************************************
; free - the memory: each task's own RAM banks (16 a RAM module, 8 KB each: BANKS, SYSINFO), and the shared
; RAM's (SEGINFO): what segments can have, what they have, and what's free, in KB.
;   ram     256 KB a task (2 modules)
;   shared  1024 KB, 512 KB in segments (2), 512 KB free

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "toollib.inc"

            HYX2_PROGRAM "free", main

.bss
total:      .res        1                                   ; The shared banks ...
given:      .res        1                                   ;   those in segments ...
segs:       .res        1                                   ;   and the segments

.code
main:
            jsr         tl_start
            LDR         r0, s_ram                           ; A task's banks
            jsr         tl_puts
            jsr         BANKS
            jsr         kb
            LDR         r0, s_atask
            jsr         tl_puts
            jsr         SYSINFO
            txa
            jsr         tl_setnum
            lda         #1
            jsr         tl_dec
            LDR         r0, s_modules
            jsr         tl_puts
            jsr         SEGINFO                             ; The shared RAM
            sta         total
            stx         given
            stz         segs
            ldx         #15
:
            lsr         r0 + 1
            ror         r0
            bcc         :+
            inc         segs
:
            dex
            bpl         :--
            LDR         r0, s_shared
            jsr         tl_puts
            lda         total
            jsr         kb
            LDR         r0, s_in
            jsr         tl_puts
            lda         given
            jsr         kb
            LDR         r0, s_segs
            jsr         tl_puts
            lda         segs
            jsr         tl_setnum
            lda         #1
            jsr         tl_dec
            LDR         r0, s_free
            jsr         tl_puts
            sec
            lda         total
            sbc         given
            jsr         kb
            LDR         r0, s_free2
            jsr         tl_puts
            jmp         tl_end

; .A banks, in KB (8 each): "N KB"
kb:
            sta         tl_num
            stz         tl_num + 1
            stz         tl_num + 2
            stz         tl_num + 3
            ldx         #3
:
            asl         tl_num
            rol         tl_num + 1
            dex
            bne         :-
            lda         #1
            jsr         tl_dec
            LDR         r0, s_kb
            jmp         tl_puts

.rodata
s_ram:      .byte       "ram     ", 0
s_atask:    .byte       " a task (", 0
s_modules:  .byte       " modules)", LF, 0
s_shared:   .byte       "shared  ", 0
s_in:       .byte       ", ", 0
s_segs:     .byte       " in segments (", 0
s_free:     .byte       "), ", 0
s_free2:    .byte       " free", LF, 0
s_kb:       .byte       " KB", 0
tl_name:    .byte       "free", 0
tl_flagset: .byte       0
tl_usage:   .byte       "free", 0

.include "toollib.s"
