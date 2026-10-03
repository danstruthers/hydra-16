; ****************************************************************************
; kdev - the kernel's own devices (docs/reimplementation-from-scratch.md, §14.1), served by a driver of their own
; on srvlib (a boot driver), not by the kernel task: it keeps only tables.  One tree each:
;   #/      the root: an empty directory for each mount point of the default namespace (bin dev env lib mnt pc
;           proc ram rom sd sram tmp, and in dev: gpio i2c mod sd spi), so ls / and ls /dev show them
;   #n      null (reads as nothing, takes every write), zero (reads as zeros, takes every write)
;   #t      ticks: the tick count (its low 16 bits, TICK_HZ a second), in decimal
;   #m      the modules in the paged ROM, a file each: its type and bank
;   #p      the tasks, a directory each (its number): status (its name, state, parent, CPU time in ticks and note
;           group) and ctl (kill, interrupt, note N)
;   #|      pipes: opening pipe makes a new one (its read end; for O_WRITE, its write end), and R_DUP its other end
;           (PIPE does both); 512 bytes each, 8 of them
; To come: #e (the environment), and /proc's other files (args, cwd, fd, ns ...).

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"
.include "srvlib.inc"

            HYX2_DRIVER "kdev", init, srv_serve, 0, 0, HF_BOOT

PIPE_N          = 8
PIPE_SIZE       = 512

.zeropage
pp:         .res        1                                   ; A pipe ...
pend:       .res        1                                   ;   and its end ($80: the write end)
n:          .res        2                                   ; A count
m:          .res        2                                   ; Another
pt:         .res        2                                   ; A pointer
tk:         .res        1                                   ; A task
cnt:        .res        1

.bss
me:         .res        ME_SIZE                             ; A module (MODINFO)
info:       .res        TI_SIZE                             ; A task (TASKINFO)
p_used:     .res        PIPE_N                              ; Each pipe: in use ...
p_rdl:      .res        PIPE_N                              ;   where the next read is ...
p_rdh:      .res        PIPE_N
p_wrl:      .res        PIPE_N                              ;   the next write ...
p_wrh:      .res        PIPE_N
p_cntl:     .res        PIPE_N                              ;   the bytes in it ...
p_cnth:     .res        PIPE_N
p_readers:  .res        PIPE_N                              ;   its ends' fids
p_writers:  .res        PIPE_N
p_buf:      .res        PIPE_N * PIPE_SIZE

.code
; ****************************************************************************
; Init: every device's letter
init:
            ldx         #PIPE_N - 1
:
            stz         p_used,X
            dex
            bpl         :-
            ldx         #0
@letter:
            lda         SRV_TREES,X
            beq         @done
            phx
            jsr         SRV_REGISTER
            plx
            bcs         @failed
            inx
            inx
            inx
            bra         @letter

@done:
            clc
@failed:
            rts

; ****************************************************************************
; #n

h_null:
            cmp         #R_READ
            bne         h_take
            stz         TASK_INBOX + RQ_DONE                ; (Nothing: the end)
            stz         TASK_INBOX + RQ_DONE + 1
            clc
            rts

; A write: all of it taken.  Opens and clunks: nothing to do
h_take:
            cmp         #R_WRITE
            bne         :+
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
:
            clc
            rts

h_zero:
            cmp         #R_READ
            bne         h_take
            MOVR        n, TASK_INBOX + RQ_COUNT            ; Zeros, 64 at a time
            MOVR        r1, TASK_INBOX + RQ_BUF
@part:
            lda         n
            ora         n + 1
            beq         @done
            lda         n + 1
            bne         @full
            lda         n
            cmp         #64
            bcc         :+
@full:
            lda         #64
:
            sta         r2
            stz         r2 + 1
            LDR         r0, zeros
            jsr         CLIENT_WRITE
            sec
            lda         n
            sbc         r2
            sta         n
            bcs         :+
            dec         n + 1
:
            clc
            lda         r1
            adc         r2
            sta         r1
            bcc         @part
            inc         r1 + 1
            bra         @part

@done:
            MOVR        TASK_INBOX + RQ_DONE, TASK_INBOX + RQ_COUNT
            clc
            rts

; ****************************************************************************
; #t

gen_ticks:
            jsr         TICKS
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

; ****************************************************************************
; #m: the modules, a file each (its id: its entry in the module directory)

h_mods:
            cmp         #DYN_FIND
            beq         @find
            cmp         #DYN_IDNAME
            beq         @idname
            lda         z:srv_k                             ; DYN_NAME: the srv_k-th
            jsr         mod_name
            bcs         @done
            lda         z:srv_k
            clc
@done:
            rts

@idname:
            txa
            jmp         mod_name

@find:                                                      ; The one named at srv_p
            stz         cnt
@try:
            lda         cnt
            jsr         mod_name
            bcs         @noent
            LDR         r3, srv_dname
            jsr         srv_same
            beq         @found
            inc         cnt
            bra         @try

@found:
            lda         cnt
            clc
            rts

@noent:
            lda         #E_NOENT
            sec
            rts

; Module .A: its entry in me, its name in srv_dname.  OUT: C = 0; or C = 1: no such module
mod_name:
            pha
            LDR         r0, me
            pla
            jsr         MODINFO
            bcs         @done
            ldx         #0
:
            lda         me + ME_NAME,X
            sta         srv_dname,X
            beq         :+
            inx
            cpx         #12
            bne         :-
            stz         srv_dname,X
:
            clc
@done:
            rts

; A module's file: "program bank 3", or a driver's, or a library's
gen_mod:
            lda         z:srv_id
            jsr         mod_name
            bcs         @done
            lda         me + ME_TYPE
            and         #3
            asl
            tax
            lda         type_words,X
            pha
            lda         type_words + 1,X
            tax
            pla
            jsr         srv_tputs
            lda         #<s_bank
            ldx         #>s_bank
            jsr         srv_tputs
            lda         me + ME_BANK
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
@done:
            rts

; ****************************************************************************
; #p: the tasks, a directory each (its id: the task)

h_procs:
            cmp         #DYN_FIND
            beq         @find
            cmp         #DYN_IDNAME
            beq         @idname
            stz         tk                                  ; DYN_NAME: the srv_k-th task in use
            ldx         z:srv_k
            stx         cnt
@task:
            lda         tk
            cmp         #16
            bcs         @none
            jsr         in_use
            bcs         @next
            lda         cnt
            beq         @this
            dec         cnt
@next:
            inc         tk
            bra         @task

@this:
            ldx         tk
            jsr         task_name
            lda         tk
            clc
            rts

@none:
            sec
            rts

@idname:
            jsr         task_name
            clc
            rts

@find:                                                      ; The number at srv_p, a task in use
            lda         z:srv_p
            sta         pt
            lda         z:srv_p + 1
            sta         pt + 1
            stz         tk
            ldy         #0
@digit:
            lda         (pt),Y
            beq         @end
            cmp         #'/'
            beq         @end
            sec
            sbc         #'0'
            cmp         #10
            bcs         @noent
            sta         cnt
            lda         tk                                  ; * 10, + the digit
            asl
            asl
            clc
            adc         tk
            asl
            clc
            adc         cnt
            sta         tk
            iny
            cpy         #3
            bcc         @digit
            bra         @noent

@end:
            tya
            beq         @noent
            lda         tk
            cmp         #16
            bcs         @noent
            jsr         in_use
            bcs         @noent
            lda         tk
            clc
            rts

@noent:
            lda         #E_NOENT
            sec
            rts

; C = 0 if task .A is in use (its TASKINFO in info).  Keeps tk
in_use:
            pha
            LDR         r0, info
            pla
            jsr         TASKINFO
            bcs         @done
            lda         info + TI_STATE                     ; (0: free)
            beq         @free
            clc
@done:
            rts

@free:
            sec
            rts

; Task .X's name in /proc: its number, in decimal
task_name:
            ldy         #0
            txa
            cmp         #10
            bcc         :+
            pha
            lda         #'1'
            sta         srv_dname
            pla
            sbc         #10                                 ; (C = 1)
            iny
:
            ora         #'0'
            sta         srv_dname,Y
            lda         #0
            sta         srv_dname + 1,Y
            rts

; status: "NAME STATE PARENT CPU GROUP" (the parent - for none; the CPU time in ticks, its low 16 bits)
gen_status:
            lda         z:srv_id
            jsr         in_use
            bcs         @gone
            lda         #<(info + TI_NAME)
            ldx         #>(info + TI_NAME)
            jsr         srv_tputs
            jsr         space
            lda         info + TI_STATE
            cmp         #STATES
            bcc         :+
            lda         #0
:
            asl
            tax
            lda         state_words,X
            pha
            lda         state_words + 1,X
            tax
            pla
            jsr         srv_tputs
            jsr         space
            lda         info + TI_PARENT
            bpl         :+
            lda         #'-'
            jsr         srv_tputc
            bra         :++
:
            ldx         #0
            jsr         srv_tputdec
:
            jsr         space
            lda         info + TI_CPU
            ldx         info + TI_CPU + 1
            jsr         srv_tputdec
            jsr         space
            lda         info + TI_GROUP
            ldx         #0
            jsr         srv_tputdec
            lda         #LF
            jsr         srv_tputc
            clc
            rts

@gone:
            lda         #E_SRCH
            sec
            rts

space:
            lda         #' '
            jmp         srv_tputc

; ctl: kill, interrupt, note N
c_kill:
            ldx         #NOTE_KILL
            bra         c_post

c_intr:
            ldx         #NOTE_INTERRUPT
            bra         c_post

c_note:
            lda         z:srv_argn
            beq         @inval
            lda         srv_arg + 1
            bne         @inval
            ldx         srv_arg
            bra         c_post

@inval:
            lda         #E_INVAL
            sec
            rts

c_post:
            lda         z:srv_id
            jmp         NOTE_POST

; ****************************************************************************
; #|: pipes (a fid's aux: its pipe, and $80 for the write end)

h_pipe:
            cmp         #R_OPEN
            beq         p_open
            cmp         #R_DUP
            beq         p_dup
            pha
            lda         srv_fid_aux,X
            and         #$80
            sta         pend
            lda         srv_fid_aux,X
            and         #$7F
            sta         pp
            pla
            cmp         #R_READ
            bne         :+
            jmp         p_read
:
            cmp         #R_WRITE
            bne         :+
            jmp         p_write
:
            cmp         #R_CLUNK
            beq         p_clunk
            clc
            rts

; A new pipe: its read end, or (O_WRITE) its write end
p_open:
            ldy         #PIPE_N - 1
:
            lda         p_used,Y
            beq         :+
            dey
            bpl         :-
            lda         #E_NOMEM
            sec
            rts
:
            sty         pp
            lda         #1
            sta         p_used,Y
            lda         #0
            sta         p_rdl,Y
            sta         p_rdh,Y
            sta         p_wrl,Y
            sta         p_wrh,Y
            sta         p_cntl,Y
            sta         p_cnth,Y
            sta         p_readers,Y
            sta         p_writers,Y
            lda         TASK_INBOX + RQ_MODE                ; Its end
            and         #O_RW_MASK
            cmp         #O_WRITE
            bne         :+
            lda         #$80
:
            and         #$80
            bra         p_end

; R_DUP: the other end of fid .Y's pipe
p_dup:
            lda         srv_fid_aux,Y
            and         #$7F
            sta         pp
            lda         srv_fid_aux,Y
            eor         #$80
            and         #$80
; Fid .X: pipe pp's end .A (counted)
p_end:
            sta         pend
            ora         pp
            sta         srv_fid_aux,X
            ldy         pp
            lda         pend
            bne         :+
            lda         p_readers,Y
            inc         a
            sta         p_readers,Y
            clc
            rts
:
            lda         p_writers,Y
            inc         a
            sta         p_writers,Y
            clc
            rts

; An end closed: the pipe's free once both are; the other end's waiters look again
p_clunk:
            ldy         pp
            lda         pend
            bne         :+
            lda         p_readers,Y
            beq         @done
            dec         a
            sta         p_readers,Y
            bra         @done
:
            lda         p_writers,Y
            beq         @done
            dec         a
            sta         p_writers,Y
@done:
            lda         p_readers,Y
            ora         p_writers,Y
            bne         :+
            lda         #0
            sta         p_used,Y
:
            inc         TASK_EVENT
            clc
            rts

; The pipes' failures
p_broken:
            lda         #E_PIPE
            sec
            rts

p_again:
            lda         #E_AGAIN
            sec
            rts

p_badf:
            lda         #E_BADF
            sec
            rts

; A read: what's there, as far as the buffer's end; nothing and no writer left: the end; nothing: E_AGAIN
p_read:
            lda         pend
            bne         p_badf
            ldy         pp
            lda         p_cntl,Y
            ora         p_cnth,Y
            bne         @some
            stz         TASK_INBOX + RQ_DONE
            stz         TASK_INBOX + RQ_DONE + 1
            lda         p_writers,Y
            bne         p_again
            clc                                             ; (No writer: the end)
            rts

@some:
            lda         p_cntl,Y                            ; m: what's there ...
            sta         m
            lda         p_cnth,Y
            sta         m + 1
            sec                                             ; n: as far as the buffer's end
            lda         #<PIPE_SIZE
            sbc         p_rdl,Y
            sta         n
            lda         #>PIPE_SIZE
            sbc         p_rdh,Y
            sta         n + 1
            jsr         least                               ; n: the least of them and the count
            lda         p_rdl,Y                             ; From the read place
            ldx         p_rdh,Y
            jsr         p_at
            MOVR        r1, TASK_INBOX + RQ_BUF
            MOVR        r2, n
            jsr         CLIENT_WRITE
            ldy         pp
            clc                                             ; The read place on (round), the count down
            lda         p_rdl,Y
            adc         n
            sta         p_rdl,Y
            lda         p_rdh,Y
            adc         n + 1
            and         #>(PIPE_SIZE - 1)
            sta         p_rdh,Y
            sec
            lda         p_cntl,Y
            sbc         n
            sta         p_cntl,Y
            lda         p_cnth,Y
            sbc         n + 1
            sta         p_cnth,Y
            bra         p_done

; A write: as much as there's room for, as far as the buffer's end; no reader left: E_PIPE; no room: E_AGAIN
p_write:
            lda         pend
            beq         @badf
            ldy         pp
            lda         p_readers,Y
            beq         @broken
            sec                                             ; m: the room ...
            lda         #<PIPE_SIZE
            sbc         p_cntl,Y
            sta         m
            lda         #>PIPE_SIZE
            sbc         p_cnth,Y
            sta         m + 1
            ora         m
            bne         @room
            jmp         p_again

@badf:
            jmp         p_badf

@broken:
            jmp         p_broken

@room:
            sec                                             ; n: as far as the buffer's end
            lda         #<PIPE_SIZE
            sbc         p_wrl,Y
            sta         n
            lda         #>PIPE_SIZE
            sbc         p_wrh,Y
            sta         n + 1
            jsr         least
            lda         p_wrl,Y                             ; To the write place
            ldx         p_wrh,Y
            jsr         p_at
            MOVR        r1, TASK_INBOX + RQ_BUF
            MOVR        r2, n
            jsr         CLIENT_READ
            ldy         pp
            clc                                             ; The write place on (round), the count up
            lda         p_wrl,Y
            adc         n
            sta         p_wrl,Y
            lda         p_wrh,Y
            adc         n + 1
            and         #>(PIPE_SIZE - 1)
            sta         p_wrh,Y
            clc
            lda         p_cntl,Y
            adc         n
            sta         p_cntl,Y
            lda         p_cnth,Y
            adc         n + 1
            sta         p_cnth,Y
p_done:
            MOVR        TASK_INBOX + RQ_DONE, n
            inc         TASK_EVENT                          ; (The other end's waiters look again)
            clc
            rts

; n = the least of n, m and the request's count.  Keeps .Y
least:
            lda         m
            cmp         n
            lda         m + 1
            sbc         n + 1
            bcs         :+
            MOVR        n, m
:
            lda         TASK_INBOX + RQ_COUNT
            cmp         n
            lda         TASK_INBOX + RQ_COUNT + 1
            sbc         n + 1
            bcs         :+
            MOVR        n, TASK_INBOX + RQ_COUNT
:
            rts

; r0 = pipe pp's buffer + .A/.X
p_at:
            clc
            adc         #<p_buf
            sta         r0
            txa
            adc         #>p_buf
            sta         r0 + 1
            lda         pp                                  ; + pp * 512
            asl
            clc
            adc         r0 + 1
            sta         r0 + 1
            rts

.assert     PIPE_SIZE = 512, error, "p_at: a pipe's buffer is 512 bytes"

.rodata
zeros:      .res        64, 0
s_bank:     .byte       " bank ", 0
type_words: .word       s_unknown, s_program, s_driver, s_library
s_unknown:  .byte       "module", 0
s_program:  .byte       "program", 0
s_driver:   .byte       "driver", 0
s_library:  .byte       "library", 0
STATES      = 9
state_words: .word      s_free, s_ready, s_wait, s_call, s_idle, s_new, s_sleep, s_blocked, s_event
s_free:     .byte       "free", 0
s_ready:    .byte       "ready", 0
s_wait:     .byte       "wait", 0
s_call:     .byte       "call", 0
s_idle:     .byte       "idle", 0
s_new:      .byte       "new", 0
s_sleep:    .byte       "sleep", 0
s_blocked:  .byte       "blocked", 0
s_event:    .byte       "event", 0

; ****************************************************************************
; The devices
SRV_TREES:
            .byte       '/'
            .word       tree_root
            .byte       'n'
            .word       tree_null
            .byte       't'
            .word       tree_time
            .byte       'm'
            .word       tree_mods
            .byte       'p'
            .word       tree_procs
            .byte       '|'
            .word       tree_pipe
            .byte       0

tree_root:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_bin,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_dev,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_env,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_lib,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_mnt,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_pc,      0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_proc,    0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_ram,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_rom,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_sd,      0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_sram,    0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_tmp,     0,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_gpio,    2,   SK_DIR,  0,         SM_READ,            0     ; (In dev: its mount points)
            SRV_ENTRY   s_i2c,     2,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_mod,     2,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_sd,      2,   SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_spi,     2,   SK_DIR,  0,         SM_READ,            0
            .word       0
tree_null:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_null,    0,   SK_DATA, h_null,    SM_READ | SM_WRITE, 0
            SRV_ENTRY   s_zero,    0,   SK_DATA, h_zero,    SM_READ | SM_WRITE, 0
            .word       0
tree_time:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_ticks,   0,   SK_TEXT, gen_ticks, SM_READ,            0
            .word       0
tree_mods:
            SRV_ENTRY   s_slash,   $FF, SK_DYN,  h_mods,    SM_READ,            1
            SRV_ENTRY   s_slash,   SE_TEMPLATE, SK_TEXT, gen_mod, SM_READ,      0     ; (Each module's file)
            .word       0
tree_procs:
            SRV_ENTRY   s_slash,   $FF, SK_DYN,  h_procs,   SM_READ,            1
            SRV_ENTRY   s_slash,   SE_TEMPLATE, SK_DIR, 0,  SM_READ,            0     ; (Each task's directory)
            SRV_ENTRY   s_status,  1,   SK_TEXT, gen_status, SM_READ,           0
            SRV_ENTRY   s_ctl,     1,   SK_CTL,  proc_cmds, SM_WRITE,           0
            .word       0
tree_pipe:
            SRV_ENTRY   s_slash,   $FF, SK_DIR,  0,         SM_READ,            0
            SRV_ENTRY   s_pipe,    0,   SK_DATA, h_pipe,    SM_READ | SM_WRITE, 0
            .word       0
proc_cmds:
            .word       s_kill, c_kill
            .word       s_interrupt, c_intr
            .word       s_note, c_note
            .word       0
s_slash:    .byte       "/", 0
s_bin:      .byte       "bin", 0
s_dev:      .byte       "dev", 0
s_env:      .byte       "env", 0
s_lib:      .byte       "lib", 0
s_mnt:      .byte       "mnt", 0
s_pc:       .byte       "pc", 0
s_proc:     .byte       "proc", 0
s_ram:      .byte       "ram", 0
s_rom:      .byte       "rom", 0
s_sd:       .byte       "sd", 0
s_sram:     .byte       "sram", 0
s_tmp:      .byte       "tmp", 0
s_gpio:     .byte       "gpio", 0
s_i2c:      .byte       "i2c", 0
s_mod:      .byte       "mod", 0
s_spi:      .byte       "spi", 0
s_null:     .byte       "null", 0
s_zero:     .byte       "zero", 0
s_ticks:    .byte       "ticks", 0
s_status:   .byte       "status", 0
s_ctl:      .byte       "ctl", 0
s_pipe:     .byte       "pipe", 0
s_kill:     .byte       "kill", 0
s_interrupt: .byte      "interrupt", 0
s_note:     .byte       "note", 0

.include "srvlib.s"
