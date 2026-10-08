; ****************************************************************************
; machine.inc - the Hydra's (the eighth bank): SOUND (the sound driver's commands on /dev/snd, its text ones on
; /dev/sndctl, as C's snd.h has them), PLAY (play, the program: a line of the score language, or a file), SYS and
; CALL ABSOLUTE (machine code, or a system call by its name: basicsys.inc's table), RREG (the registers after).

.bss
snd_fd:     .res        1                                   ; /dev/snd's fd + 1 (0: not open yet) ...
snd_cfd:    .res        1                                   ;   and /dev/sndctl's
snd_v:      .res        4                                   ; (SOUND's channel, note, patch, volume: low bytes, high)
snd_h:      .res        4
snd_len:    .res        1                                   ; (Its commands' bytes)
snd_buf:    .res        12
pl_ch:      .res        1                                   ; PLAY's channel ($FF: none)
sys_addr:   .res        2                                   ; SYS's address, its .A, .X, .Y in ...
sys_a:      .res        1
sys_x:      .res        1
sys_y:      .res        1
sys_regs:   .res        4                                   ;   and .A, .X, .Y and the flags after (RREG's)
sys_r:      .res        4                                   ; (r0 and r1 kept over the output's WRITE)

.segment "CODE8"

; ---- The eighth bank's statements (fn.inc: ST_B8 on)
st_disp8:
            tax
            jmp         (st_jt8,x)
st_jt8:
            .word       st_sound, st_sounds, st_playch, st_play, st_sys, st_rregv, st_screen, st_pset, st_preset
            .word       st_line, st_gmove, st_glx, st_gly, st_circle, st_gprint, st_palette, st_paint, st_draw
            .word       st_sprite, st_spritei, st_spriteoff, st_window, st_view
.assert     * - st_jt8 = (ST_END - ST_B8) * 2, error, "st_jt8 and fn.inc's ST_B8 group differ"

; ---- SOUND

; ST_SOUND: channel, note, patch, volume taken off (each below 0: not given; note: off): the driver's commands, one
; write (no other program's between): the patch and volume first if they're given, then the note on (or off)
st_sound:
            ldx         #3                                  ; (The volume first: the last pushed)
:
            phx
            jsr         pop_int16
            plx
            lda         t0
            sta         snd_v,x
            lda         t0 + 1
            sta         snd_h,x
            dex
            bpl         :-
            lda         snd_h                               ; (A channel: 0-23)
            beq         @lb900
            jmp         @bad
@lb900:
            lda         snd_v
            cmp         #SND_CHANNELS
            bcs         @bad
            stz         snd_len
            lda         snd_h + 1                           ; (No note: off)
            bpl         :+
            lda         #SND_R_OFF
            ldx         #0
            jsr         snd_pair
            bra         @write
:
            bne         @bad
            lda         snd_v + 1
            bmi         @bad
            lda         snd_h + 2                           ; (Its patch)
            bmi         :+
            bne         @bad
            lda         snd_v + 2
            cmp         #SND_PATCHES
            bcs         @bad
            tax
            lda         #SND_R_PATCH
            jsr         snd_pair
:
            lda         snd_h + 3                           ; (Its volume)
            bmi         :+
            bne         @bad
            ldx         snd_v + 3
            bmi         @bad
            lda         #SND_R_VOL
            jsr         snd_pair
:
            lda         #SND_R_NOTE
            ldx         snd_v + 1
            jsr         snd_pair
@write:
            lda         snd_fd                              ; (/dev/snd: opened the first time)
            bne         :+
            LDR         r0, s_snd
            jsr         ram_path
            lda         #O_RDWR
            jsr         OPEN
            bcs         @err
            inc         a
            sta         snd_fd
:
            LDR         r0, snd_buf
            lda         snd_len
            sta         r1
            stz         r1 + 1
            lda         snd_fd
            dec         a
            jsr         WRITE
            bcs         @err
            rts
@bad:
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error
@err:
            FARN        7, sys_err

; snd_buf on: the channel's command, then command .A with .X
snd_pair:
            ldy         snd_len
            pha
            lda         #SND_R_CH
            sta         snd_buf,y
            lda         snd_v
            sta         snd_buf + 1,y
            pla
            sta         snd_buf + 2,y
            txa
            sta         snd_buf + 3,y
            tya
            clc
            adc         #4
            sta         snd_len
            rts

; ST_SOUNDS: SOUND "text": a line for /dev/sndctl (the string taken off: "pan 0 left", "volume 150" ...)
st_sounds:
            FARN        7, str_lbuf
            pha
            lda         snd_cfd
            bne         :+
            LDR         r0, s_sndctl
            jsr         ram_path
            lda         #O_WRITE
            jsr         OPEN
            bcs         @err
            inc         a
            sta         snd_cfd
:
            pla
            sta         r1
            stz         r1 + 1
            LDR         r0, lbuf
            lda         snd_cfd
            dec         a
            jsr         WRITE
            bcs         @err
            rts
@err:
            FARN        7, sys_err

s_snd:      .byte       "/dev/snd", 0
s_sndctl:   .byte       "/dev/sndctl", 0

; ---- PLAY

; ST_PLAYCH: PLAY's channel (taken off: below 0, none)
st_playch:
            jsr         pop_int16
            lda         t0 + 1
            beq         :+
            lda         #$FF
            sta         pl_ch
            rts
:
            lda         t0
            cmp         #SND_CHANNELS
            bcc         :+
            lda         #ER_FUNC
            ldx         #0
            jmp         b_error
:
            sta         pl_ch
            rts

; ST_PLAY: the string taken off played by play, the program, and waited for: a file's name (no channel, .zsm or .mml
; at its end: play name), else a line of the score language (play -m ch line; channel 0 if none was given).  Its
; failure: its message, BASIC's error
st_play:
            FARN        7, str_lbuf
            cmp         #SH_ARGS - 8
            bcc         :+
            lda         #ER_STRING
            ldx         #0
            jmp         b_error
:
            sta         snd_len
            lda         pl_ch                               ; (A file: no channel, its name .zsm or .mml)
            cmp         #$FF
            bne         @mml
            lda         snd_len
            cmp         #5
            bcc         @mml
            ldx         #3
            ldy         snd_len
:
            dey
            lda         lbuf,y
            ora         #$20
            cmp         s_zsm,x
            bne         @notzsm
            dex
            bpl         :-
            bra         @file
@notzsm:
            ldx         #3
            ldy         snd_len
:
            dey
            lda         lbuf,y
            ora         #$20
            cmp         s_mmlx,x
            bne         @mml
            dex
            bpl         :-
@file:
            ldx         #0                                  ; (sh_argb: the name, 0, 0)
            bra         @text
@mml:
            lda         #'-'                                ; (sh_argb: -m, the channel, the line, 0, 0)
            sta         sh_argb
            lda         #'m'
            sta         sh_argb + 1
            stz         sh_argb + 2
            ldx         #3
            lda         pl_ch                               ; (The channel: 0 if none was given; 1 or 2 digits)
            cmp         #$FF
            bne         :+
            lda         #0
:
            cmp         #10
            bcc         @one
            ldy         #'0'
:
            cmp         #10
            bcc         :+
            sbc         #10
            iny
            bra         :-
:
            pha
            tya
            sta         sh_argb,x
            inx
            pla
@one:
            ora         #'0'
            sta         sh_argb,x
            inx
            stz         sh_argb,x
            inx
@text:
            ldy         #0
:
            cpy         snd_len
            beq         :+
            lda         lbuf,y
            sta         sh_argb,x
            inx
            iny
            bra         :-
:
            stz         sh_argb,x
            stz         sh_argb + 1,x
            lda         #$FF                                ; (The next PLAY's channel: none, as it begins)
            sta         pl_ch
            jsr         out_flush
            FARN        6, con_cooked
            ldx         #9                                  ; (play: /bin/play, else the ROM's)
:
            lda         s_binplay,x
            sta         sh_cwd,x
            dex
            bpl         :-
            jsr         pl_spawn
            bcc         @started
            cmp         #E_NOENT
            bne         @err
            ldx         #7
:
            lda         s_mplay,x
            sta         sh_cwd,x
            dex
            bpl         :-
            jsr         pl_spawn
            bcc         @started
@err:
            FARN        7, sys_err
@started:
            stz         esbuf
            FARN        7, rc_wait
            lda         sh_status
            ora         sh_status + 1
            bne         :+
            rts
:
            lda         #$FF                                ; (Its message: the error's text)
            ldx         #1
            jmp         b_error

; play SPAWNed: sh_cwd its path, sh_argb its arguments.  OUT: as SPAWN's
pl_spawn:
            LDR         r0, sh_cwd
            LDR         r1, sh_argb
            lda         #0
            jmp         SPAWN

s_zsm:      .byte       ".zsm"
s_mmlx:     .byte       ".mml"
s_binplay:  .byte       "/bin/play", 0
s_mplay:    .byte       "#m/play", 0

; ---- SYS, CALL ABSOLUTE, RREG

; ST_SYS: an address or a call's name (a string: basicsys.inc's names, either case), then .A, .X and .Y, taken off:
; the code called with them (jsr), the output written first (r0 and r1 kept over it: a program's POKEs); .A, .X, .Y
; and the flags after RREG's.  A name not there: the system's E_NOSYS
st_sys:
            ldx         #2                                  ; (.Y, .X, .A: the last pushed first)
:
            phx
            jsr         pop_int16
            plx
            lda         t0
            sta         sys_a,x
            dex
            bpl         :-
            jsr         top_vt
            lda         (vt)
            cmp         #VT_STR
            beq         @name
            jsr         pop_int16
            MOVW        sys_addr, t0
            bra         @call
@name:
            FARN        7, str_lbuf                         ; (Its name in capitals, looked for)
            ldy         #0
:
            lda         lbuf,y
            beq         :++
            cmp         #'a'
            bcc         :+
            cmp         #'z' + 1
            bcs         :+
            and         #$DF
            sta         lbuf,y
:
            iny
            bra         :--
:
            jsr         sys_lookup
@call:
            ldx         #3                                  ; (The output out first, r0 and r1 kept)
:
            lda         r0,x
            sta         sys_r,x
            dex
            bpl         :-
            jsr         out_flush
            ldx         #3
:
            lda         sys_r,x
            sta         r0,x
            dex
            bpl         :-
            lda         sys_addr + 1                        ; (At $8000-$9FFF: BANK's bank there)
            cmp         #$80
            bcc         :+
            cmp         #$A0
            bcs         :+
            lda         ubank
            sta         RAM_BANK
:
            lda         sys_a
            ldx         sys_x
            ldy         sys_y
            jsr         @go
            php
            sta         sys_regs
            stx         sys_regs + 1
            sty         sys_regs + 2
            pla
            sta         sys_regs + 3
            jmp         code_sel
@go:
            jmp         (sys_addr)

; sys_addr = the address of the call lbuf names (in capitals), from basicsys.inc's table; not there: E_NOSYS
sys_lookup:
            LDW         t4, SYS_NAMES
@name:
            lda         (t4)
            bne         :+
            lda         #E_NOSYS
            FARN        7, sys_err
:
            ldy         #0
@char:
            lda         (t4),y                              ; (Its last character: bit 7 set)
            and         #$7F
            cmp         lbuf,y
            bne         @next
            lda         (t4),y
            iny
            asl         a
            bcc         @char
            lda         lbuf,y                              ; (All of it, and no more)
            bne         @next
            lda         (t4),y
            sta         sys_addr
            iny
            lda         (t4),y
            sta         sys_addr + 1
            rts
@next:
            ldy         #0                                  ; (Past this one's name and address)
:
            lda         (t4),y
            iny
            asl         a
            bcc         :-
            iny
            iny
            tya
            clc
            adc         t4
            sta         t4
            bcc         @name
            inc         t4 + 1
            bra         @name

; ST_RREGV: register n (taken off: 0 .A, 1 .X, 2 .Y, 3 the flags) after the last SYS, pushed
st_rregv:
            jsr         pop_int16
            lda         t0
            and         #3
            tax
            lda         sys_regs,x
            ldx         #0
            FARN        6, push_ax
            rts

; (basicsys.inc's names: their last character's bit 7 set)
.macro htasc str
            .repeat     .strlen(str) - 1, I
            .byte       .strat(str, I)
            .endrepeat
            .byte       .strat(str, .strlen(str) - 1) | $80
.endmacro
.include "basicsys.inc"
