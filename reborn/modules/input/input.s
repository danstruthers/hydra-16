; ****************************************************************************
; input - the Vera X's input controller's driver (docs/design/plans/VIDEO.md, step 6): a program, a user-level driver
; as 9front's nusb/kb is (srvlib's servers run only when a request comes, and this polls).  The controller is the
; X16's SMC (an ATtiny861 with the X16 community's firmware, x16-smc; a PS/2 keyboard and mouse on it) on the I2C
; bus at $42, read through gpio's #i/42 (/dev/i2c/42: its files are opened by their devices' names, so any namespace
; has them).  init starts it as the shells start, in a note group of its own, so a Ctrl-C at the keyboard can't reach
; it; with no controller it ends at once ("no input controller", code 1).
;   The controller: a write of a register alone ($30: its version; $22: the mouse's mode) makes it the next read's;
; a register and a value are a command ($40 V: the default request; $20 V: the mouse's mode asked for; $1A $ED V:
; the keyboard's LEDs).  A read that names no register gets the default request's answer: $41, a key code; $43, a
; key code (0: none) and a mouse packet (0: none).  With nothing to give, it doesn't answer its address (E_IO here),
; so a look that finds nothing costs an address byte.  Its key codes are the IBM PC/AT's key numbers (bit 7: a
; release), its packets the PS/2 mouse's.
;   Every POLL ticks (67 times a second) it's read till it has nothing (DRAIN reads at most); after QUIET looks with
; nothing (2 s), every POLL_IDLE (10 times a second), till something comes.  A look that finds nothing costs some
; 5,000 cycles (gpio's request, the bus, the scheduler): 10% of the CPU at 67 a second, 1.8% at 10, so the first key
; after a quiet spell waits a tenth of a second at most.  Its reads and commands want /dev/i2c's subaddress 0, as
; the system starts.  Then:
;   * The keys go to the console's keyboard, #c/kbin, as a PC terminal (xterm) sends them: the characters (the US
;     layout: km_plain, km_shift), with Shift and Caps Lock; Ctrl (a letter's control character; Ctrl-Space and
;     Ctrl-2 NUL, Ctrl-3 to Ctrl-7 ESC to US, Ctrl-8 and Ctrl-? DEL, Ctrl-/ US); Alt, either, an ESC first; Enter CR,
;     Backspace DEL (Ctrl-Backspace BS), Tab, Shift-Tab CSI Z, Ctrl-Tab CSI 9;5u and Ctrl-Shift-Tab CSI 9;6u; the
;     cursor keys, Home, End, Insert, Delete, Page Up and Down and F1-F12 as xterm's (CSI A, CSI 2~, SS3 P, CSI 15~
;     ...; with Shift, Alt or Ctrl CSI 1;M A, CSI 2;M~, CSI 1;M P: M is 1, + 1 Shift, + 2 Alt, + 4 Ctrl); the
;     keypad's digits with Num Lock on (as it starts), its cursor keys with it off; Scroll Lock the console's hold
;     (Ctrl-] h).  The locks' LEDs follow them.  A key's repeat is the keyboard's own.
;   * The mouse (a wheel's asked for, mode 3, as it starts; its answer read MOUSE_POLLS polls on): its moves summed,
;     written to /dev/vid/mousein as m DX DY B (y down; the buttons Plan 9's: 1 left, 2 middle, 4 right), one as
;     the buttons change and one for the rest; the wheel as button 8 (up) or 16 (down), pressed and let go.  With no
;     mouse, or no /dev/vid, the keys alone.

.include "hydra.inc"
.include "hyx2.inc"
.include "macros.inc"

            HYX2_PROGRAM "input", main

POLL            = 3             ; Ticks between looks (67 a second) ...
POLL_IDLE       = 20            ;   and after QUIET of them with nothing (10 a second)
QUIET           = 134           ; (2 s of looks)
DRAIN           = 16            ; Reads at most a look (a key code and a packet each)
MOUSE_POLLS     = 34            ; Looks before the mouse's mode is read (half a second: it resets itself)
KOUT_MAX        = 64            ; The keys' bytes, a write to #c/kbin at most
R_MOUSE_MODE    = $20           ; The controller's registers: the mouse's mode asked for (0, 3 a wheel, 4 five
R_MOUSE_ID      = $22           ;   buttons) ... and given ...
R_VERSION       = $30           ;   its version (major) ...
R_DEFAULT       = $40           ;   the default request ...
R_KEYS          = $41           ;   a key code ...
R_PS2           = $43           ;   a key code and a mouse packet ...
R_KBD_CMD2      = $1A           ;   a command of two bytes to the keyboard
KBD_LEDS        = $ED           ; The keyboard's command: its LEDs
L_SCROLL        = $01           ; The locks (the LEDs' bits): Scroll Lock ...
L_NUM           = $02           ;   Num Lock ...
L_CAPS          = $04           ;   Caps Lock
M_SHIFT         = $03           ; The modifiers (km_mod's bits, left and right): Shift ...
M_CTRL          = $0C           ;   Ctrl ...
M_ALT           = $30           ;   Alt
K_CAPS          = 30            ; Key numbers: Caps Lock ...
K_NUM           = 90            ;   Num Lock ...
K_SCROLL        = 125           ;   Scroll Lock
BS              = $08
ESC             = $1B
DEL             = $7F
CTRL_RB         = $1D           ; Ctrl-]: the console's windows' key

.zeropage
fd_i2c:     .res        1                                   ; /dev/i2c/42 ...
fd_kbd:     .res        1                                   ;   #c/kbin ...
fd_mouse:   .res        1                                   ;   /dev/vid/mousein ($FF: none)
msize:      .res        1                                   ; The mouse's packet: 3 or 4 bytes (0: no mouse)
mid:        .res        1                                   ;   its mode (0, 3, 4)
mwait:      .res        1                                   ;   looks till its mode's read (0: read)
mods:       .res        1                                   ; The modifiers down (M_*)
locks:      .res        1                                   ; The locks on (L_*)
kn:         .res        1                                   ; kout's bytes
drains:     .res        1                                   ; Reads left this look
quiet:      .res        1                                   ; Looks left before the idle rate (0: at it)
mbtn:       .res        1                                   ; The mouse's buttons (Plan 9's) ...
mpend:      .res        1                                   ;   <> 0: a change to send ...
mdx:        .res        2                                   ;   its moves since the last sent ...
mdy:        .res        2
t:          .res        2                                   ; Scratch
u:          .res        2
ln:         .res        1                                   ; line's bytes

.bss
ibuf:       .res        8                                   ; A read's answer
cmd:        .res        4                                   ; A command to the controller
kout:       .res        KOUT_MAX                            ; The keys' bytes, for #c/kbin
line:       .res        32                                  ; A line for /dev/vid/mousein
digits:     .res        6

.code
main:
            ldx         #ln - fd_i2c                        ; (Its zero page: all 0)
:
            stz         fd_i2c,X
            dex
            bpl         :-
            LDR         r0, s_i2c                           ; The controller: there?
            lda         #O_RDWR
            jsr         OPEN
            bcs         @none
            sta         fd_i2c
            lda         #R_VERSION                          ; (Its version: the register alone, then a read)
            sta         cmd
            lda         #1
            jsr         command
            bcs         @none
            jsr         read1
            bcs         @none
            LDR         r0, s_kbin                          ; The console's keyboard
            lda         #O_WRITE
            jsr         OPEN
            bcs         @nocons
            sta         fd_kbd
            lda         #$FF
            sta         fd_mouse
            lda         #R_KEYS                             ; Keys alone, till the mouse answers
            jsr         default
            lda         #R_MOUSE_MODE                       ; The mouse: a wheel's asked for
            sta         cmd
            lda         #3
            sta         cmd + 1
            lda         #2
            jsr         command
            lda         #MOUSE_POLLS
            sta         mwait
            lda         #QUIET                              ; (At POLL as it starts)
            sta         quiet
            lda         #L_NUM                              ; Num Lock on
            sta         locks
            jsr         leds
@loop:
            jsr         look
            lda         #POLL                               ; (Every POLL ticks, or POLL_IDLE when it's been quiet)
            ldx         quiet
            bne         :+
            lda         #POLL_IDLE
:
            ldx         #0
            jsr         SLEEP
            bra         @loop

@none:
            LDR         r0, s_none
            bra         @end

@nocons:
            LDR         r0, s_nocons
@end:
            lda         #1
            jmp         EXITS

; ****************************************************************************
; The controller

; A look: read till there's nothing (DRAIN reads at most), each key and packet acted on; then the keys and the
; mouse's moves sent.  Something read: QUIET looks more at POLL; nothing: one less.  (The mouse's mode read
; MOUSE_POLLS looks on)
look:
            lda         mwait
            beq         :+
            dec         mwait
            bne         :+
            jsr         mouse_mode
:
            lda         quiet
            beq         :+
            dec         quiet
:
            lda         #DRAIN
            sta         drains
@read:
            jsr         fetch
            bcs         @sent
            lda         #QUIET
            sta         quiet
            lda         ibuf
            beq         :+
            jsr         key
:
            lda         msize
            beq         :+
            lda         ibuf + 1
            beq         :+
            jsr         packet
:
            dec         drains
            bne         @read
@sent:
            jsr         m_flush
            jmp         k_flush

; The default request's answer, into ibuf: a key code (0: none), and with a mouse its packet (ibuf + 1: 0, none).
; OUT: C = 0; or C = 1: nothing (the controller didn't answer)
fetch:
            stz         ibuf + 1
            LDR         r0, ibuf
            lda         msize
            inc         a
            sta         r1
            stz         r1 + 1
            lda         fd_i2c
            jmp         READ

; A byte read from the controller (a register's, written alone before), into ibuf.  OUT: C = 0; or C = 1
read1:
            LDR         r0, ibuf
            LDR         r1, 1
            lda         fd_i2c
            jmp         READ

; cmd's first .A bytes written to the controller.  OUT: C = 0; or C = 1
command:
            sta         r1
            stz         r1 + 1
            LDR         r0, cmd
            lda         fd_i2c
            jmp         WRITE

; The default request .A
default:
            sta         cmd + 1
            lda         #R_DEFAULT
            sta         cmd
            lda         #2
            jmp         command

; The keyboard's LEDs, as locks has them
leds:
            lda         #R_KBD_CMD2
            sta         cmd
            lda         #KBD_LEDS
            sta         cmd + 1
            lda         locks
            sta         cmd + 2
            lda         #3
            jmp         command

; The mouse's mode, read: 3 or 4 (a wheel): packets of 4; 0: 3; else no mouse.  With one (and /dev/vid/mousein),
; the default request a key code and a packet
mouse_mode:
            lda         #R_MOUSE_ID
            sta         cmd
            lda         #1
            jsr         command
            bcs         @none
            jsr         read1
            bcs         @none
            lda         ibuf
            sta         mid
            ldx         #3
            cmp         #0
            beq         @mouse
            ldx         #4
            cmp         #3
            beq         @mouse
            cmp         #4
            bne         @none
@mouse:
            phx
            LDR         r0, s_mousein
            lda         #O_WRITE
            jsr         OPEN
            plx
            bcs         @none
            sta         fd_mouse
            stx         msize
            lda         #R_PS2
            jmp         default

@none:
            rts

; ****************************************************************************
; The keys

; Key code .A (bit 7: a release): a modifier's state, a lock's, or its bytes into kout
key:
            tax
            and         #$7F
            tay
            lda         km_mod,Y                            ; A modifier: down or up
            beq         @other
            cpx         #$80
            bcs         :+
            tsb         mods
            rts
:
            trb         mods
            rts

@other:
            cpx         #$80                                ; (A release: nothing)
            bcs         @done
            cpy         #K_CAPS
            beq         @caps
            cpy         #K_NUM
            beq         @num
            cpy         #K_SCROLL
            beq         @scroll
            lda         locks                               ; The keypad, Num Lock off: its cursor keys
            and         #L_NUM
            bne         @plain
            lda         km_pad,Y
            beq         @plain
            cmp         #$FF
            beq         @done
            bra         @special

@plain:
            lda         mods
            and         #M_SHIFT
            beq         :+
            lda         km_shift,Y
            bra         @got
:
            lda         km_plain,Y
@got:
            beq         @done
            cmp         #$80
            bcc         k_char
@special:
            jmp         k_special

@caps:
            lda         #L_CAPS
            bra         @lock

@num:
            lda         #L_NUM
            bra         @lock

@scroll:
            lda         #CTRL_RB                            ; (Hold: the console's Ctrl-] h)
            jsr         k_put
            lda         #'h'
            jsr         k_put
            lda         #L_SCROLL
@lock:
            eor         locks
            sta         locks
            jmp         leds

@done:
            rts

; A character (.A, the layout's): Caps Lock's case, Tab's sequences, Ctrl's control character, Alt's ESC first
k_char:
            sta         t
            lda         locks                               ; Caps Lock: a letter's case turned
            and         #L_CAPS
            beq         @tab
            lda         t
            and         #$DF
            cmp         #'A'
            bcc         @tab
            cmp         #'Z' + 1
            bcs         @tab
            lda         t
            eor         #$20
            sta         t
@tab:
            lda         t
            cmp         #TAB
            bne         @ctrl
            lda         mods
            bit         #M_CTRL
            beq         @shtab
            jsr         k_csi                               ; Ctrl-Tab: CSI 9;5u (Ctrl-Shift-Tab: 6)
            lda         #'9'
            jsr         k_put
            lda         #';'
            jsr         k_put
            lda         mods
            and         #M_SHIFT
            beq         :+
            lda         #1
:
            clc
            adc         #'5'
            jsr         k_put
            lda         #'u'
            jmp         k_put

@shtab:
            and         #M_SHIFT                            ; Shift-Tab: CSI Z
            beq         @ctrl
            jsr         k_csi
            lda         #'Z'
            jmp         k_put

@ctrl:
            lda         mods
            and         #M_CTRL
            beq         @alt
            lda         t
            jsr         ctrl_of
            sta         t
@alt:
            lda         mods
            and         #M_ALT
            beq         :+
            lda         #ESC
            jsr         k_put
:
            lda         t
            jmp         k_put

; Ctrl and the character .A: its control character (xterm's).  OUT: .A
ctrl_of:
            cmp         #DEL                                ; (Ctrl-Backspace: BS)
            beq         @bs
            cmp         #'?'
            beq         @del
            cmp         #'@'
            bcs         @low                                ; @ to ~: its low 5 bits
            cmp         #' '
            beq         @nul
            cmp         #'/'
            beq         @us
            cmp         #'2'
            beq         @nul
            cmp         #'8'
            beq         @del
            cmp         #'3'
            bcc         @same
            cmp         #'8'
            bcs         @same
            adc         #<(ESC - '3')                       ; (3-7: ESC to US; C = 0)
            rts

@low:
            and         #$1F
            rts

@nul:
            lda         #0
            rts

@us:
            lda         #$1F
            rts

@bs:
            lda         #BS
            rts

@del:
            lda         #DEL
@same:
            rts

; A cursor or function key (.A, its KEY_* code) as xterm sends it: with no modifiers ESC, then [ or O, then its
; number if it has one (CSI 2~ ...), and its final; with them, CSI, its number (or 1), ;, M and its final
k_special:
            sec
            sbc         #KEY_UP
            tay
            jsr         k_m                                 ; M: 1, none
            sta         u
            lda         #ESC
            jsr         k_put
            lda         u
            cmp         #1
            bne         @mod
            lda         sp_lead,Y                           ; No modifiers: [ or O, its number, its final
            jsr         k_put
            lda         sp_n,Y
            beq         :+
            jsr         k_dec
:
            lda         sp_final,Y
            jmp         k_put

@mod:
            lda         #'['
            jsr         k_put
            lda         sp_n,Y
            bne         :+
            lda         #1
:
            jsr         k_dec
            lda         #';'
            jsr         k_put
            lda         u
            jsr         k_dec
            lda         sp_final,Y
            jmp         k_put

; xterm's modifier number: 1, + 1 Shift, + 2 Alt, + 4 Ctrl.  OUT: .A.  Keeps .Y
k_m:
            lda         #1
            sta         t
            lda         mods
            and         #M_SHIFT
            beq         :+
            inc         t
:
            lda         mods
            and         #M_ALT
            beq         :+
            inc         t
            inc         t
:
            lda         mods
            and         #M_CTRL
            beq         :+
            lda         t
            clc
            adc         #4
            sta         t
:
            lda         t
            rts

; ESC [ into kout
k_csi:
            lda         #ESC
            jsr         k_put
            lda         #'['
            jmp         k_put

; .A (0-99) in decimal into kout.  Keeps .Y
k_dec:
            ldx         #0
:
            cmp         #10
            bcc         :+
            sbc         #10
            inx
            bra         :-
:
            pha
            txa
            beq         :+
            ora         #'0'
            jsr         k_put
:
            pla
            ora         #'0'
            ; (k_put)

; .A into kout (full: sent first).  Keeps .X, .Y
k_put:
            phx
            ldx         kn
            cpx         #KOUT_MAX
            bcc         :+
            pha
            phy
            jsr         k_flush
            ply
            pla
            ldx         kn
:
            sta         kout,X
            inc         kn
            plx
            rts

; kout's bytes to the console's keyboard (#c/kbin)
k_flush:
            lda         kn
            beq         @done
            sta         r1
            stz         r1 + 1
            stz         kn
            LDR         r0, kout
            lda         fd_kbd
            jmp         WRITE
@done:
            rts

; ****************************************************************************
; The mouse

; A packet (ibuf + 1 on): its buttons (a change: the moves before it sent first), its moves summed (y down), its
; wheel as button 8 or 16 pressed and let go
packet:
            lda         ibuf + 1                            ; Its buttons, Plan 9's: left 1, middle 2, right 4
            and         #1
            sta         t
            lda         ibuf + 1
            and         #2
            asl
            ora         t
            sta         t
            lda         ibuf + 1
            and         #4
            lsr
            ora         t
            cmp         mbtn
            beq         @moves
            pha
            jsr         m_flush
            pla
            sta         mbtn
            lda         #1
            sta         mpend
@moves:
            lda         ibuf + 2                            ; x: 9 bits, its sign in byte 0's bit 4
            ldx         #0
            jsr         @sign4
            clc
            adc         mdx
            sta         mdx
            txa
            adc         mdx + 1
            sta         mdx + 1
            lda         ibuf + 3                            ; y: its sign in bit 5, up (so it's taken away)
            ldx         #0
            sta         t
            lda         ibuf + 1
            and         #$20
            beq         :+
            dex
:
            sec
            lda         mdy
            sbc         t
            sta         mdy
            stx         t
            lda         mdy + 1
            sbc         t
            sta         mdy + 1
            lda         ibuf + 1                            ; (A move: its bytes, or its signs)
            and         #$30
            ora         ibuf + 2
            ora         ibuf + 3
            beq         :+
            lda         #1
            sta         mpend
:
            lda         msize                               ; The wheel
            cmp         #4
            bne         @done
            lda         ibuf + 4
            ldx         mid
            cpx         #4
            bne         :+
            and         #$0F                                ; (Mode 4: 4 bits, signed)
            bit         #$08
            beq         :+
            ora         #$F0
:
            tax
            beq         @done
            bmi         @up
            lda         #16
            bra         @wheel
@up:
            lda         #8
@wheel:
            pha
            jsr         m_flush                             ; (The moves first)
            pla
            ora         mbtn                                ; Pressed ...
            jsr         m_send0
            lda         mbtn                                ;   and let go
            jmp         m_send0

@done:
            rts

@sign4:                                                     ; (x's sign, byte 0's bit 4: .X $FF)
            pha
            lda         ibuf + 1
            and         #$10
            beq         :+
            dex
:
            pla
            rts

; The moves and buttons waiting sent: m DX DY B
m_flush:
            lda         mpend
            beq         @done
            stz         mpend
            lda         mbtn
            jsr         m_line
            stz         mdx
            stz         mdx + 1
            stz         mdy
            stz         mdy + 1
@done:
            rts

; m 0 0 .A sent (the buttons .A, no move: m_flush has just sent the moves)
m_send0:
            ; (m_line)

; m mdx mdy .A, a line, to /dev/vid/mousein
m_line:
            pha
            stz         ln
            lda         #'m'
            jsr         l_put
            lda         #' '
            jsr         l_put
            lda         mdx
            ldx         mdx + 1
            jsr         l_dec
            lda         #' '
            jsr         l_put
            lda         mdy
            ldx         mdy + 1
            jsr         l_dec
            lda         #' '
            jsr         l_put
            pla
            ldx         #0
            jsr         l_dec
            lda         #LF
            jsr         l_put
            LDR         r0, line
            lda         ln
            sta         r1
            stz         r1 + 1
            lda         fd_mouse
            jmp         WRITE

; .A into line
l_put:
            ldy         ln
            sta         line,Y
            inc         ln
            rts

; .A/.X (16 bits, signed) in decimal into line
l_dec:
            sta         t
            stx         t + 1
            txa
            bpl         :+
            lda         #'-'
            jsr         l_put
            sec
            lda         #0
            sbc         t
            sta         t
            lda         #0
            sbc         t + 1
            sta         t + 1
:
            ldy         #0                                  ; Its digits, the last first (t / 10 till 0)
@digit:
            ldx         #16
            lda         #0
@bit:
            asl         t
            rol         t + 1
            rol         a
            cmp         #10
            bcc         :+
            sbc         #10
            inc         t
:
            dex
            bne         @bit
            ora         #'0'
            sta         digits,Y
            iny
            lda         t
            ora         t + 1
            bne         @digit
:
            dey
            lda         digits,Y
            phy
            jsr         l_put
            ply
            tya
            bne         :-
            rts

.rodata
s_i2c:      .byte       "#i/42", 0
s_kbin:     .byte       "#c/kbin", 0
s_mousein:  .byte       "#v/mousein", 0
s_none:     .byte       "no input controller", 0
s_nocons:   .byte       "no console keyboard (#c/kbin)", 0

; The cursor and function keys (KEY_UP to KEY_F12), as xterm sends them: [ or O, a number (0: none), the final
sp_lead:    .byte       "[[[[[[[[[[OOOO[[[[[[[["
sp_n:       .byte       0, 0, 0, 0, 0, 0, 2, 3, 5, 6, 0, 0, 0, 0, 15, 17, 18, 19, 20, 21, 23, 24
sp_final:   .byte       "ABCDHF~~~~PQRS~~~~~~~~"

; Each key's character (US), or its KEY_* code ($80-$95), or 0: none
km_plain:
            .byte       $00, $60, $31, $32, $33, $34, $35, $36, $37, $38, $39, $30, $2D, $3D, $00, $7F  ; 0-15
            .byte       $09, $71, $77, $65, $72, $74, $79, $75, $69, $6F, $70, $5B, $5D, $5C, $00, $61  ; 16-31
            .byte       $73, $64, $66, $67, $68, $6A, $6B, $6C, $3B, $27, $5C, $0D, $00, $5C, $7A, $78  ; 32-47
            .byte       $63, $76, $62, $6E, $6D, $2C, $2E, $2F, $00, $00, $00, $00, $00, $20, $00, $00  ; 48-63
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $86, $87, $00, $00, $83  ; 64-79
            .byte       $84, $85, $00, $80, $81, $88, $89, $00, $00, $82, $00, $37, $34, $31, $00, $2F  ; 80-95
            .byte       $38, $35, $32, $30, $2A, $39, $36, $33, $2E, $2D, $2B, $00, $0D, $00, $1B, $00  ; 96-111
            .byte       $8A, $8B, $8C, $8D, $8E, $8F, $90, $91, $92, $93, $94, $95, $00, $00, $00, $00  ; 112-127

; With Shift
km_shift:
            .byte       $00, $7E, $21, $40, $23, $24, $25, $5E, $26, $2A, $28, $29, $5F, $2B, $00, $7F  ; 0-15
            .byte       $09, $51, $57, $45, $52, $54, $59, $55, $49, $4F, $50, $7B, $7D, $7C, $00, $41  ; 16-31
            .byte       $53, $44, $46, $47, $48, $4A, $4B, $4C, $3A, $22, $7C, $0D, $00, $7C, $5A, $58  ; 32-47
            .byte       $43, $56, $42, $4E, $4D, $3C, $3E, $3F, $00, $00, $00, $00, $00, $20, $00, $00  ; 48-63
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $86, $87, $00, $00, $83  ; 64-79
            .byte       $84, $85, $00, $80, $81, $88, $89, $00, $00, $82, $00, $37, $34, $31, $00, $2F  ; 80-95
            .byte       $38, $35, $32, $30, $2A, $39, $36, $33, $2E, $2D, $2B, $00, $0D, $00, $1B, $00  ; 96-111
            .byte       $8A, $8B, $8C, $8D, $8E, $8F, $90, $91, $92, $93, $94, $95, $00, $00, $00, $00  ; 112-127

; The keypad's keys with Num Lock off: a KEY_* code, $FF none (0: not the keypad's, or as with it on)
km_pad:
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 0-15
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 16-31
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 32-47
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 48-63
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 64-79
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $84, $83, $85, $00, $00  ; 80-95
            .byte       $80, $FF, $81, $86, $00, $88, $82, $89, $87, $00, $00, $00, $00, $00, $00, $00  ; 96-111
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 112-127

; The modifiers' bits (M_*)
km_mod:
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 0-15
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 16-31
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $01, $00, $00, $00  ; 32-47
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $02, $04, $40, $10, $00, $20, $80  ; 48-63
            .byte       $08, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 64-79
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 80-95
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 96-111
            .byte       $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00  ; 112-127

