; ****************************************************************************
; counter - a sample server (the assembly SDK's: sdk/asm/README.md): a driver, a module that runs in place in the
; paged ROM, built on srvlib.  It serves the device #k:
;   /count     a text file: the count, in decimal
;   /ctl       a ctl file: "add N" (the count, N more), "reset" (0); it reads as count
; A driver starts at boot (HF_BOOT: the boot waits for its init, which registers its device letter), in a task of
; its own, and runs only for its clients' requests: srvlib's srv_serve takes each, walks the tree below (srv_tree),
; and calls the handlers (a text file's maker, a ctl command's).  To try it, it goes into the ROM (modules/rom.txt,
; or a test's modules); then:
;   % cat '#k/count'
;   0
;   % echo add 5 >'#k/ctl'; cat '#k/count'
;   5

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"                                       ; (srvlib.s at the end)

            HYX2_DRIVER "counter", init, srv_serve, 0, 0, HF_BOOT

.zeropage
count:      .res        2

.code

; Its init: the count 0, and its device letter registered (C = 1, .A = an error: it ends)
init:
            stz         count
            stz         count + 1
            lda         #'k'
            jmp         SRV_REGISTER

; count: the count, a line (srv_tputdec writes into the text a read gets)
gen_count:
            lda         count
            ldx         count + 1
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; ctl's "add N": srv_arg the number after the word
c_add:
            lda         z:srv_argn                          ; (No number: E_INVAL)
            beq         @inval
            clc
            lda         count
            adc         srv_arg
            sta         count
            lda         count + 1
            adc         srv_arg + 1
            sta         count + 1
            clc
            rts

@inval:
            lda         #E_INVAL
            sec
            rts

; ctl's "reset"
c_reset:
            stz         count
            stz         count + 1
            clc
            rts

.rodata
; The tree: name, parent (entry), kind, handler, mode, aux (a ctl file's: the entry it reads as)
srv_tree:
            SRV_ENTRY   s_root,  $FF, SK_DIR,  0,         SM_READ,            0     ; 0
            SRV_ENTRY   s_count, 0,   SK_TEXT, gen_count, SM_READ,            0     ; 1
            SRV_ENTRY   s_ctl,   0,   SK_CTL,  ctl_cmds,  SM_READ | SM_WRITE, 1     ; 2 (reads as count)
            .word       0
ctl_cmds:                                                   ; Its commands: the word, the handler
            .word       s_add, c_add
            .word       s_reset, c_reset
            .word       0
s_root:     .byte       "/", 0
s_count:    .byte       "count", 0
s_ctl:      .byte       "ctl", 0
s_add:      .byte       "add", 0
s_reset:    .byte       "reset", 0

.include "srvlib.s"
