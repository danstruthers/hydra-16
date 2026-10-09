; ****************************************************************************
; t_srv - a test server on srvlib (a boot driver: task F), for t_file.  The device #T:
;   /hello      a text file: "hello, world\n"
;   /count      a text file: the count, in decimal
;   /ctl        a ctl file: "add N" (the count + N), "kick" (wakes wait's readers), "reset" (unkicked), "fail"
;               (its handler's error, E_BUSY); it reads as count
;   /data       64 bytes, read and written at offsets
;   /sub        a directory, of inner (a text file: "inner\n")
;   /wait       a read waits (E_AGAIN, the client in a wait mask) till a "kick"; then "ok\n"
;   /ro         a text file that can't be opened for writing
;   /out        what's written goes to the serial port (this task's PUTC: a driver has no fd 1); it reads as nothing.
;               A test's fds 0-2, as the console's are init's

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"

            HYX2_DRIVER "t_srv", init, srv_serve, 0, 0, HF_BOOT

DATA_SIZE       = 64
OUT_CHUNK       = 32

.zeropage
count:      .res        2
kicked:     .res        1
left:       .res        2                                   ; (/out: what's left to print ...
from:       .res        2                                   ;   where it is in the client ...
chunk:      .res        1                                   ;   and this part of it)

.bss
data:       .res        DATA_SIZE
waiters:    .res        2                                   ; (The clients waiting to read wait)
obuf:       .res        OUT_CHUNK

.code
init:
            stz         count
            stz         count + 1
            stz         kicked
            stz         waiters
            stz         waiters + 1
            lda         #'T'
            jmp         SRV_REGISTER                        ; (Its error is init's)

; ---- The text files' makers
gen_hello:
            lda         #<s_hello_text
            ldx         #>s_hello_text
            jsr         srv_tputs
            clc
            rts

gen_count:
            lda         count
            ldx         count + 1
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

gen_inner:
            lda         #<s_inner_text
            ldx         #>s_inner_text
            jsr         srv_tputs
            clc
            rts

; ---- /data: 64 bytes at offsets
h_data:
            cmp         #R_READ
            beq         @move
            cmp         #R_WRITE
            beq         @move
            clc                                             ; (Opens and clunks: nothing to do)
            rts

@move:
            pha
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET + 1          ; Past the end: nothing
            ora         TASK_INBOX + RQ_OFFSET + 2
            ora         TASK_INBOX + RQ_OFFSET + 3
            bne         @end
            lda         TASK_INBOX + RQ_OFFSET
            cmp         #DATA_SIZE
            bcs         @end
            sec                                             ; r2: what's left, or what's asked if less
            lda         #DATA_SIZE
            sbc         TASK_INBOX + RQ_OFFSET
            sta         r2
            stz         r2 + 1
            lda         TASK_INBOX + RQ_COUNT + 1
            bne         :+
            lda         TASK_INBOX + RQ_COUNT
            cmp         r2
            bcs         :+
            sta         r2
:
            lda         r2
            sta         TASK_INBOX + RQ_DONE
            clc
            lda         #<data
            adc         TASK_INBOX + RQ_OFFSET
            sta         r0
            lda         #>data
            adc         #0
            sta         r0 + 1
            MOVR        r1, TASK_INBOX + RQ_BUF
            pla
            cmp         #R_READ
            beq         :+
            jsr         CLIENT_READ
            clc
            rts
:
            jsr         CLIENT_WRITE
            clc
            rts

@end:
            pla
            clc
            rts

; ---- /wait: a read waits for a kick
h_wait:
            cmp         #R_READ
            beq         :+
            clc
            rts
:
            lda         kicked
            bne         @read
            lda         #<waiters                           ; Not yet: the client waits
            ldx         #>waiters
            jsr         srv_wait_add
            lda         #E_AGAIN
            sec
            rts

@read:
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         TASK_INBOX + RQ_OFFSET              ; ("ok\n" at offset 0; then the end)
            ora         TASK_INBOX + RQ_OFFSET + 1
            bne         :+
            LDR         r0, s_ok
            MOVR        r1, TASK_INBOX + RQ_BUF
            LDR         r2, 3
            lda         #3
            sta         TASK_INBOX + RQ_DONE
            jsr         CLIENT_WRITE
:
            clc
            rts

; ---- /out: printed, a chunk at a time
h_out:
            cmp         #R_WRITE
            beq         @write
            stz         TASK_INBOX + RQ_DONE                ; (A read: the end; opens and clunks: nothing to do)
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            rts

@write:
            MOVR        left, TASK_INBOX + RQ_COUNT
            MOVR        from, TASK_INBOX + RQ_BUF
@chunk:
            lda         left
            ora         left + 1
            beq         @done
            lda         left + 1                            ; This part: what's left, OUT_CHUNK at most
            bne         @full
            lda         left
            cmp         #OUT_CHUNK
            bcc         :+
@full:
            lda         #OUT_CHUNK
:
            sta         chunk
            sta         r2
            stz         r2 + 1
            LDR         r0, obuf
            MOVR        r1, from
            jsr         CLIENT_READ
            ldx         #0
:
            lda         obuf,X
            jsr         PUTC
            inx
            cpx         chunk
            bne         :-
            sec                                             ; The rest
            lda         left
            sbc         chunk
            sta         left
            bcs         :+
            dec         left + 1
:
            clc
            lda         from
            adc         chunk
            sta         from
            bcc         @chunk
            inc         from + 1
            bra         @chunk

@done:
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
            clc
            rts

; ---- /ctl's commands
c_add:
            clc
            lda         count
            adc         srv_arg
            sta         count
            lda         count + 1
            adc         srv_arg + 1
            sta         count + 1
            clc
            rts

c_kick:
            lda         #1
            sta         kicked
            lda         #<waiters
            ldx         #>waiters
            jsr         srv_wake_all
            clc
            rts

c_reset:
            stz         kicked
            clc
            rts

c_fail:
            lda         #E_BUSY
            sec
            rts

.rodata
srv_tree:
            SRV_ENTRY   s_root,  $FF, SK_DIR,  0,         SM_READ,            0     ; 0
            SRV_ENTRY   s_hello, 0,   SK_TEXT, gen_hello, SM_READ,            0     ; 1
            SRV_ENTRY   s_count, 0,   SK_TEXT, gen_count, SM_READ,            0     ; 2
            SRV_ENTRY   s_ctl,   0,   SK_CTL,  ctl_cmds,  SM_READ | SM_WRITE, 2     ; 3 (reads as count)
            SRV_ENTRY   s_data,  0,   SK_DATA, h_data,    SM_READ | SM_WRITE, 0     ; 4
            SRV_ENTRY   s_sub,   0,   SK_DIR,  0,         SM_READ,            0     ; 5
            SRV_ENTRY   s_inner, 5,   SK_TEXT, gen_inner, SM_READ,            0     ; 6
            SRV_ENTRY   s_wait,  0,   SK_DATA, h_wait,    SM_READ,            0     ; 7
            SRV_ENTRY   s_ro,    0,   SK_TEXT, gen_hello, SM_READ,            0     ; 8
            SRV_ENTRY   s_out,   0,   SK_DATA, h_out,     SM_READ | SM_WRITE, 0     ; 9
            .word       0
ctl_cmds:
            .word       s_add, c_add
            .word       s_kick, c_kick
            .word       s_reset, c_reset
            .word       s_fail, c_fail
            .word       0
s_root:     .byte       "/", 0
s_hello:    .byte       "hello", 0
s_count:    .byte       "count", 0
s_ctl:      .byte       "ctl", 0
s_data:     .byte       "data", 0
s_sub:      .byte       "sub", 0
s_inner:    .byte       "inner", 0
s_wait:     .byte       "wait", 0
s_ro:       .byte       "ro", 0
s_out:      .byte       "out", 0
s_add:      .byte       "add", 0
s_kick:     .byte       "kick", 0
s_reset:    .byte       "reset", 0
s_fail:     .byte       "fail", 0
s_hello_text: .byte     "hello, world", LF, 0
s_inner_text: .byte     "inner", LF, 0
s_ok:       .byte       "ok", LF

.include "srvlib.s"
