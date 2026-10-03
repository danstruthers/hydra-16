; ****************************************************************************
; diskctl.s - what the disk tools (mkfs, fsck, label) have in common, included at the end of each (after
; toollib.s): a disk's ctl file, #d/NAME/ctl (the storage driver's), a command made and written to it, and its text
; read.  The work is the driver's.
;   dc_path     dc_ctl = the ctl of the disk named at tl_arg.  OUT: C = 0; or C = 1, .A = E_NAMETOOLONG
;   dc_word     the string at r0 onto the command (dc_cmd, dc_len), a space before it unless it's the first
;   dc_send     the command written to the ctl.  OUT: C = 0; or C = 1, .A = the error
;   dc_read     the ctl's text (255 bytes at most) into dc_text, zero-terminated.  OUT: C = 0; or C = 1, .A

DC_CMD_MAX      = 63            ; A command's length at most (srvlib's SRV_CTL_MAX)

.pushseg

.bss
dc_ctl:     .res        24                                  ; "#d/NAME/ctl"
dc_cmd:     .res        DC_CMD_MAX + 1                      ; The command ...
dc_len:     .res        1                                   ;   and its length
dc_text:    .res        256                                 ; The ctl's text
dc_fd:      .res        1

.code

; dc_ctl = the ctl of the disk named at tl_arg (8 characters at most).  OUT: C = 0; or C = 1, .A = E_NAMETOOLONG
dc_path:
            ldx         #0
:
            lda         dc_s_disks,X
            sta         dc_ctl,X
            inx
            cpx         #3
            bne         :-
            ldy         #0
:
            lda         (tl_arg),Y
            beq         :+
            sta         dc_ctl,X
            inx
            iny
            cpy         #9
            bcc         :-
            lda         #E_NAMETOOLONG
            rts

:
            ldy         #0
:
            lda         dc_s_ctl,Y
            sta         dc_ctl,X
            inx
            iny
            cmp         #0
            bne         :-
            clc
            rts

; The string at r0 onto the command, a space before it unless it's the first (DC_CMD_MAX at most: the rest dropped)
dc_word:
            ldx         dc_len
            beq         :+
            lda         #' '
            jsr         @put
:
            ldy         #0
:
            lda         (r0),Y
            beq         :+
            jsr         @put
            iny
            bne         :-
:
            stx         dc_len
            rts

@put:
            cpx         #DC_CMD_MAX
            bcs         :+
            sta         dc_cmd,X
            inx
:
            rts

; The command written to the ctl.  OUT: C = 0; or C = 1, .A = the error
dc_send:
            LDR         r0, dc_ctl
            lda         #O_WRITE
            jsr         OPEN
            bcs         @done
            sta         dc_fd
            LDR         r0, dc_cmd
            lda         dc_len
            sta         r1
            stz         r1 + 1
            lda         dc_fd
            jsr         WRITE
            php
            pha
            lda         dc_fd
            jsr         CLOSE
            pla
            plp
@done:
            rts

; The ctl's text into dc_text (255 bytes at most), zero-terminated.  OUT: C = 0; or C = 1, .A = the error
dc_read:
            LDR         r0, dc_ctl
            lda         #O_READ
            jsr         OPEN
            bcs         @done
            sta         dc_fd
            LDR         r0, dc_text
            LDR         r1, 255                             ; (Its count: .A alone)
            lda         dc_fd
            jsr         READ
            bcc         :+
            pha
            lda         dc_fd
            jsr         CLOSE
            pla
            sec
            rts

:
            tax                                             ; (Its end: 0)
            stz         dc_text,X
            lda         dc_fd
            jsr         CLOSE
            clc
@done:
            rts

.rodata
dc_s_disks: .byte       "#d/"
dc_s_ctl:   .byte       "/ctl", 0

.popseg
