; ****************************************************************************
; stmt.s - as's statements (as.s has the whole story): a line's label (name:, @name:, or : an unnamed one), then a
; constant (name = expr, name := expr), an instruction, a directive or a macro's use.
;   Instructions: the W65C02S's, each addressing mode written as ca65 writes it (a: or z: before an address forces
; its absolute or zero page form); an address takes the zero page form when it's known by then in this pass and
; under $100.
;   Directives: .byte .byt .word .addr .dword .res .asciiz; .include .incbin; .if .ifdef .ifndef .ifblank .ifnblank
; .elseif .else .endif; .macro (.mac) .endmacro (.endmac) .exitmacro; .segment, .code .rodata .data .bss .zeropage,
; .pushseg .popseg; .assert .error .warning; .org (with -b); and .import .export .global (and their zp kinds),
; .setcpu .pc02 .p02 .feature .macpack .debuginfo .list .listbytes .case .smart .autoimport, taken and left.
;   A macro is defined in pass 1, its body kept in the heap a line at a time: its record (MD_*), then its lines, each
; a ref to the next, its length and its bytes.  Used, its lines are read in turn (mline: each name that's one of
; its parameters made its argument) from a frame of as.s's input, its arguments in the frame's buffer.

.include "as.inc"

BANKREG         = $00

MD_FIRST        = 0             ; A macro's record: its first line's ref ...
MD_LAST         = 2             ;   its last's ...
MD_NP           = 4             ;   its parameters: how many ...
MD_NAMES        = 5             ;   then each one's length and bytes
ML_NEXT         = 0             ; A macro's line: the next one's ref ...
ML_LEN          = 2             ;   its length ...
ML_TEXT         = 3             ;   and its bytes

; The W65C02S's instructions: their names, each opcode's name and mode, each mode's length (M_*, NAMES, N_*;
; names, op_name, op_mode, modelen): the one table, the asm library's (its disassembler reads it as it is)
.include "w65c02.inc"

.zeropage
opp:        .res        2                                   ; A row of optab

.bss
optab:      .res        MODES * NAMES                       ; Each mode's opcode for each name ($FF: none; op_init)
nfirst:     .res        26                                  ; The names by their first letter: the first's index ...
nnext:      .res        NAMES                               ;   and each one's next ($FF: none)
c1:         .res        1                                   ; (find3's: a name's second letter ...
c2:         .res        1                                   ;   and third ...
n3:         .res        1                                   ;   and the index tried)
cdepth:     .res        1                                   ; .if levels ...
cstate:     .res        CONDS                               ;   each one's state (CS_*)
mcol:       .res        1                                   ; A macro's body coming: 1 kept (pass 1), 2 passed by
mdef:       .res        2                                   ; The macro being defined: its record's ref
mprev:      .res        2                                   ;   its last line so far
cseg:       .res        1                                   ; The segment assembled into ...
soffl:      .res        SEGS                                ;   each one's offset (its size so far) ...
soffh:      .res        SEGS
sbasel:     .res        SEGS                                ;   its base ...
sbaseh:     .res        SEGS
ssizel:     .res        SEGS                                ;   and its size in pass 1
ssizeh:     .res        SEGS
segstk:     .res        SEGSTACK                            ; .pushseg's
segsp:      .res        1
nidx:       .res        1                                   ; An instruction: its name's index ...
nbit:       .res        1                                   ;   the bit in its name (RMB, SMB, BBR, BBS) ...
mode:       .res        1                                   ;   its mode ...
force:      .res        1                                   ;   a: or z: ($80: absolute, $40: zero page) ...
addr:       .res        4                                   ;   its operand ...
addrz:      .res        1                                   ;   which may be on the zero page (0: yes)
target:     .res        4                                   ; (BBR, BBS: their target)
li0:        .res        1
here:       .res        2                                   ; Where the statement starts
mtext:      .res        LINE_MAX + 1                        ; A macro's line, before its parameters are made its
mfr:        .res        1                                   ;   arguments, and its frame
npar:       .res        1                                   ; (.macro: its parameters so far)
pnames:     .res        128                                 ;   (their names: each one's length and bytes)
pnlen:      .res        1
aoff:       .res        1                                   ; (expand's: the arguments' bytes so far)
mq:         .res        1                                   ; (mline's: in a string)
isfd:       .res        1                                   ; (.incbin's file)
istat:      .res        SR_SIZE
ileft:      .res        2
ipart:      .res        1                                   ; (A part of it)

.code

; ---- Each pass

; The state at a pass's start: CODE, every segment empty, no .if, no macro coming
stmt_init:
            lda         pass
            cmp         #1
            bne         :+
            jsr         op_init
:
            lda         #SEG_CODE
            sta         cseg
            ldx         #SEGS - 1
:
            stz         soffl,X
            stz         soffh,X
            dex
            bpl         :-
            stz         segsp
            stz         cdepth
            stz         mcol
            rts

; A pass's end: an .if or .macro not ended said; pass 1's sizes kept, the others' checked against them
seg_end:
            lda         cdepth
            beq         :+
            LDR         r0, s_noendif
            jsr         err
:
            lda         mcol
            beq         :+
            LDR         r0, s_noendm
            jsr         err
:
            ldx         #SEGS - 1
@seg:
            lda         pass
            cmp         #1
            bne         @check
            lda         soffl,X
            sta         ssizel,X
            lda         soffh,X
            sta         ssizeh,X
            bra         @next
@check:
            lda         nerrors                             ; (After errors, a size may differ: not said)
            bne         @next
            lda         soffl,X
            cmp         ssizel,X
            bne         @phase
            lda         soffh,X
            cmp         ssizeh,X
            bne         @phase
@next:
            dex
            bpl         @seg
            rts
@phase:
            LDR         r0, s_phase
            jmp         err

; ---- A line

; The line (line) assembled
statement:
            stz         li
            lda         mcol                                ; A macro's body: kept, or passed by
            beq         :+
            jmp         collect
:
            jsr         active
            bcc         @on
            jmp         skipping
@on:
            jsr         here_set
            jsr         atend
            bne         :+
            rts
:
            cmp         #':'                                ; An unnamed label
            bne         @name
            jsr         eat
            jsr         un_def
            bcc         @rest
            rts
@name:
            lda         li
            sta         li0
            jsr         getid
            bcs         @stmt
            jsr         peek                                ; name: (the : right after it; but not name :=)
            cmp         #':'
            bne         @const
            ldy         li
            lda         line + 1,Y
            cmp         #'='
            beq         @const
            jsr         eat
            jsr         label
            bcs         @done
@rest:
            jsr         atend
            bne         @stmt0
@done:
            rts
@const:
            jsr         constant                            ; name = expr, name := expr?
            bcc         @done
            lda         li0                                 ; (No: a statement)
            sta         li
            bra         @stmt
@stmt0:
            lda         li
            sta         li0
@stmt:
            lda         li0
            sta         li
            jsr         skipws
            jsr         peek
            cmp         #'.'
            bne         :+
            jmp         directive
:
            jsr         getid
            bcc         :+
            LDR         r0, s_what
            jmp         err
:
            jsr         mnemonic
            bcs         :+
            jmp         instruction
:
            jsr         sym_find                            ; A macro's use?
            bcs         @unknown
            ldy         #SY_FLAGS
            lda         (hp),Y
            and         #SF_MACRO
            beq         @unknown
            jmp         expand
@unknown:
            LDR         r0, s_unknown
            jmp         errtok

; here = the place the statement starts at
here_set:
            ldx         cseg
            clc
            lda         sbasel,X
            adc         soffl,X
            sta         here
            lda         sbaseh,X
            adc         soffh,X
            sta         here + 1
            rts

; The segment's offset + .A (-128 to 127)
soffadd:
            ldx         cseg
            ldy         #0
            cmp         #0
            bpl         :+
            dey
:
            clc
            adc         soffl,X
            sta         soffl,X
            tya
            adc         soffh,X
            sta         soffh,X
            rts

; A label (tok) here.  A normal one starts a new scope for cheap locals.  C = 1: said
label:
            lda         tok
            cmp         #'@'
            beq         :+
            inc         scope
            bne         :+
            inc         scope + 1
:
            jsr         sym_def
            bcs         @done
            jsr         redef                               ; (Not twice in a pass)
            bcs         @done
            lda         here
            sta         val
            lda         here + 1
            sta         val + 1
            stz         val + 2
            stz         val + 3
            lda         #SF_LABEL
            ldx         cseg
            jsr         sym_set
            clc
@done:
            rts

; C = 1 (said): the symbol at hp is defined already in this pass, or is a macro.  One of as's own or its caller's
; (SF_LINK: DEFINE's, a BASIC program's names) is the source's from now on
redef:
            ldy         #SY_FLAGS
            lda         (hp),Y
            and         #SF_MACRO
            bne         @twice
            lda         (hp),Y
            bit         #SF_LINK
            beq         :+
            and         #$FF ^ SF_LINK
            sta         (hp),Y
            clc
            rts
:
            and         #SF_PASS
            cmp         pass
            beq         @twice
            clc
            rts
@twice:
            LDR         r0, s_twice
            jsr         errtok
            sec
            rts

; name = expr or name := expr (tok the name, li after it).  C = 0: done (or said); C = 1: not one
constant:
            jsr         skipws
            jsr         peek
            cmp         #'='
            beq         @eq
            cmp         #':'
            bne         @no
            ldy         li
            lda         line + 1,Y
            cmp         #'='
            bne         @no
            jsr         eat
@eq:
            jsr         eat
            ldx         tlen                                ; (The name kept: expr's names use tok)
:
            lda         tok,X
            sta         mtext,X
            dex
            bpl         :-
            lda         tlen
            pha
            jsr         expr
            pla
            bcs         @done
            sta         tlen
            tax
:
            lda         mtext,X
            sta         tok,X
            dex
            bpl         :-
            jsr         sym_def
            bcs         @done
            ldy         #SY_FLAGS                           ; (A constant may be set again: = in a macro)
            lda         (hp),Y
            and         #SF_MACRO
            beq         :+
            LDR         r0, s_twice
            jsr         errtok
            clc
            rts
:
            lda         #0
            ldx         #$FF
            jsr         sym_set
            jsr         trailing
@done:
            clc
            rts
@no:
            sec
            rts

; Anything left on the line: said
trailing:
            jsr         atend
            beq         :+
            LDR         r0, s_trail
            jsr         err
:
            rts

; ---- .if and its kin

; C = 0: lines are assembled (no .if, or the innermost on)
active:
            ldx         cdepth
            beq         @yes
            lda         cstate - 1,X
            and         #CS_ON
            beq         @no
@yes:
            clc
            rts
@no:
            sec
            rts

; A line while lines aren't assembled: only .if (a level, all of it off), .elseif, .else, .endif matter
skipping:
            jsr         skipws
            jsr         peek
            cmp         #'.'
            bne         @done
            jsr         getid
            LDR         p2, cdirs
            jsr         tfind
            bcc         @done
            jmp         (p2)
@done:
            rts

; A level, off, inside one that's off (or done)
d_ifoff:
            lda         #CS_DONE
            bra         cpush

; .if expr
d_if:
            jsr         active
            bcs         d_ifoff
            jsr         expr
            bcs         d_ifoff
            jsr         number_ok
            bcs         d_ifoff
            jsr         valbool
            bra         ifon

; .ifdef name, .ifndef name
d_ifdef:
            jsr         active
            bcs         d_ifoff
            jsr         getid
            bcs         d_ifbad
            jsr         defined
            bra         ifon
d_ifndef:
            jsr         active
            bcs         d_ifoff
            jsr         getid
            bcs         d_ifbad
            jsr         defined
            eor         #1
            bra         ifon
d_ifbad:
            LDR         r0, s_what
            jsr         err
            bra         d_ifoff

; .ifblank (nothing after it), .ifnblank
d_ifblank:
            jsr         active
            bcs         d_ifoff
            jsr         atend
            beq         :+
            lda         #0
            bra         ifon
:
            lda         #1
            bra         ifon
d_ifnblank:
            jsr         active
            bcs         d_ifoff
            jsr         atend
            beq         :+
            lda         #1
            bra         ifon
:
            lda         #0
            ; (on into ifon)

; A level, on if .A <> 0 (in one that's on)
ifon:
            cmp         #0
            beq         :+
            lda         #CS_ON | CS_DONE | CS_OUTER
            bra         cpush
:
            lda         #CS_OUTER
            ; (on into cpush)

; Push a level, its state .A
cpush:
            ldx         cdepth
            cpx         #CONDS
            bcs         @deep
            sta         cstate,X
            inc         cdepth
            rts
@deep:
            LDR         r0, s_deep
            jmp         err

; .else
d_else:
            ldx         cdepth
            beq         d_nomatch
            lda         cstate - 1,X
            and         #CS_OUTER | CS_DONE
            cmp         #CS_OUTER
            bne         :+
            lda         #CS_OUTER | CS_ON | CS_DONE
            sta         cstate - 1,X
            rts
:
            lda         cstate - 1,X
            and         #$FF ^ CS_ON
            sta         cstate - 1,X
            rts

; .elseif expr (evaluated only if no branch is taken yet, in a level that's assembled)
d_elseif:
            ldx         cdepth
            beq         d_nomatch
            lda         cstate - 1,X
            and         #CS_OUTER | CS_DONE
            cmp         #CS_OUTER
            bne         :+
            phx
            jsr         expr
            plx
            bcs         :+
            jsr         valbool
            beq         :+
            lda         #CS_OUTER | CS_ON | CS_DONE
            sta         cstate - 1,X
            rts
:
            lda         cstate - 1,X
            and         #$FF ^ CS_ON
            sta         cstate - 1,X
            rts

; .endif
d_endif:
            lda         cdepth
            beq         d_nomatch
            dec         cdepth
            rts

d_nomatch:
            LDR         r0, s_nomatch
            jmp         err

; .A (Z) = val <> 0.  Keeps .X
valbool:
            lda         val
            ora         val + 1
            ora         val + 2
            ora         val + 3
            beq         :+
            lda         #1
:
            rts

; ---- Directives

directive:
            jsr         getid
            LDR         p2, dirs
            jsr         tfind
            bcc         @unknown
            jmp         (p2)
@unknown:
            LDR         r0, s_dir
            jmp         errtok

; Directives taken and left (the rest of the line too)
d_ignore:
            rts

; .byte, .byt, .asciiz: expressions (a byte each) and strings; .asciiz a 0 after
d_byte:
            stz         p1
            bra         dbytes
d_asciiz:
            lda         #1
            sta         p1
dbytes:
@item:
            jsr         skipws
            jsr         peek
            cmp         #'"'
            bne         @expr
            jsr         getstr
            bcs         @done
            ldx         #0
:
            cpx         slen
            beq         @next
            lda         sbuf,X
            phx
            jsr         emit
            plx
            inx
            bra         :-
@expr:
            jsr         expr
            bcs         @done
            jsr         byte_ok
            lda         val
            jsr         emit
@next:
            lda         #','
            jsr         expect
            bcc         @item
            lda         p1
            beq         :+
            lda         #0
            jsr         emit
:
            jsr         trailing
@done:
            rts

; .word, .addr: expressions, two bytes each
d_word:
            lda         #2
            bra         :+
d_dword:
            lda         #4
:
            sta         p1
@item:
            jsr         expr
            bcs         @done
            ldx         #0
:
            lda         val,X
            phx
            jsr         emit
            plx
            inx
            cpx         p1
            bne         :-
            lda         #','
            jsr         expect
            bcc         @item
            jsr         trailing
@done:
            rts

; .res count [, fill]
d_res:
            jsr         expr
            bcs         @done
            jsr         number_ok
            bcs         @done
            lda         val
            sta         p1
            lda         val + 1
            sta         p1 + 1
            stz         p2                                  ; (The fill: 0)
            lda         #','
            jsr         expect
            bcs         @fill
            jsr         expr
            bcs         @done
            lda         val
            sta         p2
@fill:
            lda         p1
            ora         p1 + 1
            beq         @end
            lda         p2
            jsr         emit_res
            lda         p1
            bne         :+
            dec         p1 + 1
:
            dec         p1
            bra         @fill
@end:
            jsr         trailing
@done:
            rts

; .include "file"
d_include:
            jsr         getstr
            bcs         @done
            jsr         trailing
            LDR         r0, sbuf
            jsr         push_file
@done:
            rts

; .incbin "file" [, start [, count]]: its bytes (from start, count of them)
d_incbin:
            jsr         getstr
            bcs         @done
            LDR         r0, sbuf
            jsr         find_include
            bcs         @done
            sta         isfd
            jsr         ib_range
            bcs         @close
            jsr         ib_copy
@close:
            lda         isfd
            jmp         CLOSE
@done:
            rts

; ileft = .incbin's file's length (64K at most) less start, or count.  C = 1: an error (said)
ib_range:
            LDR         r0, istat
            lda         isfd
            jsr         FSTAT
            bcs         ib_bad
            lda         istat + SR_LENGTH
            sta         ileft
            lda         istat + SR_LENGTH + 1
            sta         ileft + 1
            lda         #','
            jsr         expect
            bcs         @all
            jsr         expr                                ; start
            bcs         @rts
            sec
            lda         ileft
            sbc         val
            sta         ileft
            lda         ileft + 1
            sbc         val + 1
            sta         ileft + 1
            bcs         :+
            stz         ileft
            stz         ileft + 1
:
            lda         val
            sta         r0
            lda         val + 1
            sta         r0 + 1
            stz         r1
            stz         r1 + 1
            ldx         #0
            lda         isfd
            jsr         SEEK
            lda         #','
            jsr         expect
            bcs         @all
            jsr         expr                                ; count
            bcs         @rts
            lda         val
            sta         ileft
            lda         val + 1
            sta         ileft + 1
@all:
            clc
@rts:
            rts
ib_bad:
            LDR         r0, s_ioerr
            jsr         err
            sec
            rts

; ileft bytes of .incbin's file: their room (passes 1, 2), or the bytes (pass 3), 255 at a time
ib_copy:
            lda         ileft
            ora         ileft + 1
            beq         @rts
            lda         #255                                ; (This part's)
            ldx         ileft + 1
            bne         :+
            lda         ileft
:
            sta         ipart
            sec
            lda         ileft
            sbc         ipart
            sta         ileft
            bcs         :+
            dec         ileft + 1
:
            lda         pass
            cmp         #3
            beq         @read
            ldx         cseg                                ; Passes 1, 2: its room alone
            clc
            lda         soffl,X
            adc         ipart
            sta         soffl,X
            bcc         ib_copy
            inc         soffh,X
            bra         ib_copy
@read:
            LDR         r0, sbuf
            lda         ipart
            sta         r1
            stz         r1 + 1
            lda         isfd
            jsr         READ
            bcs         ib_bad
            cmp         ipart
            bne         @short
            ldy         #0
:
            lda         sbuf,Y
            phy
            jsr         emit
            ply
            iny
            cpy         ipart
            bne         :-
            bra         ib_copy
@short:
            LDR         r0, s_short
            jmp         err
@rts:
            rts

; .segment "NAME"; .code .rodata .data .bss .zeropage
d_segment:
            jsr         getstr
            bcs         @done
            ldx         #0
@find:
            lda         segnames,X
            sta         r0
            lda         segnames + 1,X
            beq         @unknown
            sta         r0 + 1
            ldy         #0
:
            lda         (r0),Y
            cmp         sbuf,Y
            bne         @next
            iny
            cmp         #0
            bne         :-
            txa
            lsr         a
            sta         cseg
            jmp         trailing
@next:
            inx
            inx
            bra         @find
@unknown:
            LDR         r0, s_seg
            jsr         err
@done:
            rts
d_code:
            lda         #SEG_CODE
            bra         setseg
d_rodata:
            lda         #SEG_RODATA
            bra         setseg
d_data:
            lda         #SEG_DATA
            bra         setseg
d_bss:
            lda         #SEG_BSS
            bra         setseg
d_zeropage:
            lda         #SEG_ZP
setseg:
            sta         cseg
            jmp         trailing

; .pushseg, .popseg
d_pushseg:
            ldx         segsp
            cpx         #SEGSTACK
            bcs         :+
            lda         cseg
            sta         segstk,X
            inc         segsp
            rts
:
            LDR         r0, s_deep
            jmp         err
d_popseg:
            ldx         segsp
            beq         :+
            dec         segsp
            lda         segstk - 1,X
            sta         cseg
            rts
:
            LDR         r0, s_nomatch
            jmp         err

; .assert expr, error|warning|ldwarning|lderror [, "message"]: in pass 3 (every value known by then)
d_assert:
            lda         pass
            cmp         #3
            bne         @done
            jsr         expr
            bcs         @done
            jsr         valbool
            bne         @done
            stz         p1                                  ; (p1: a warning, not an error)
            lda         #','
            jsr         expect
            bcs         @nomsg
            jsr         getid
            bcs         @nomsg
            LDR         r0, s_warning
            jsr         tokis
            bcs         :+
            LDR         r0, s_ldwarning
            jsr         tokis
            bcc         :++
:
            inc         p1
:
            lda         #','                                ; Its message
            jsr         expect
            bcs         @nomsg
            jsr         getstr
            bcs         @nomsg
            LDR         r0, sbuf
            bra         @say
@nomsg:
            LDR         r0, s_assert
@say:
            lda         p1
            bne         :+
            jmp         err
:
            jmp         warn
@done:
            rts

; .error "message", .warning "message"
d_error:
            jsr         getstr
            bcs         :+
            LDR         r0, sbuf
            jmp         err
:
            rts
d_warning:
            jsr         getstr
            bcs         :+
            LDR         r0, sbuf
            jmp         warn
:
            rts

; .org address (with -b: the image's start, before anything's in it)
d_org:
            lda         fraw
            beq         @no
            ldx         #SEGS - 1
:
            lda         soffl,X
            ora         soffh,X
            bne         @no
            dex
            bpl         :-
            jsr         expr
            bcs         @done
            jsr         number_ok
            bcs         @done
            lda         val
            sta         imgbase
            lda         val + 1
            sta         imgbase + 1
            jmp         trailing
@no:
            LDR         r0, s_org
            jmp         err
@done:
            rts

; ---- Macros

; .macro name [param [, param ...]]
d_macro:
            jsr         getid
            bcc         :+
            LDR         r0, s_what
            jmp         err
:
            lda         pass
            cmp         #1
            beq         :+
            lda         #2                                  ; (Passes 2, 3: defined already; its body passed by)
            sta         mcol
            rts
:
            ldx         tlen                                ; (Its name kept)
:
            lda         tok,X
            sta         mtext,X
            dex
            bpl         :-
            lda         tlen
            sta         mfr
            stz         npar
            stz         pnlen
@param:
            jsr         getid
            bcs         @params
            ldx         pnlen
            lda         tlen
            clc
            adc         pnlen
            cmp         #127
            bcs         @many
            lda         tlen
            sta         pnames,X
            inx
            ldy         #0
:
            lda         tok,Y
            sta         pnames,X
            inx
            iny
            cpy         tlen
            bne         :-
            stx         pnlen
            inc         npar
            lda         npar
            cmp         #MAX_PARAMS + 1
            bcs         @many
            lda         #','
            jsr         expect
            bcc         @param
            bra         @params
@many:
            LDR         r0, s_params
            jmp         err
@params:
            jsr         trailing
            lda         pnlen                               ; Its record
            clc
            adc         #MD_NAMES
            jsr         halloc
            bcs         @done
            lda         sref
            sta         mdef
            lda         sref + 1
            sta         mdef + 1
            lda         #0
            ldy         #MD_FIRST
:
            sta         (hp),Y
            iny
            cpy         #MD_NP
            bne         :-
            lda         npar
            sta         (hp),Y
            ldx         #0
            ldy         #MD_NAMES
:
            cpx         pnlen
            beq         :+
            lda         pnames,X
            sta         (hp),Y
            iny
            inx
            bra         :-
:
            ldx         mfr                                 ; Its name, a symbol
            stx         tlen
:
            lda         mtext,X
            sta         tok,X
            dex
            bpl         :-
            jsr         sym_def
            bcs         @done
            ldy         #SY_FLAGS
            lda         (hp),Y
            and         #SF_PASS | SF_MACRO
            beq         :+
            LDR         r0, s_twice
            jsr         errtok
            bra         @done
:
            lda         mdef
            sta         val
            lda         mdef + 1
            sta         val + 1
            stz         val + 2
            stz         val + 3
            lda         #SF_MACRO
            ldx         #$FF
            jsr         sym_set
            stz         mprev
            stz         mprev + 1
            lda         #1
            sta         mcol
@done:
            rts

; A line of a macro's body: kept (pass 1), or passed by; .endmacro its end
collect:
            jsr         skipws
            jsr         peek
            cmp         #'.'
            bne         @keep
            lda         li
            pha
            jsr         getid
            pla
            sta         li
            LDR         r0, s_d_endmacro
            jsr         tokis
            bcs         @end
            LDR         r0, s_d_endmac
            jsr         tokis
            bcc         @keep
@end:
            stz         mcol
            rts
@keep:
            lda         mcol
            cmp         #1
            bne         @done
            ldx         #0                                  ; Its length
:
            lda         line,X
            beq         :+
            inx
            bne         :-
:
            stx         p1
            txa
            clc
            adc         #ML_TEXT
            bcc         :+
            LDR         r0, s_toolong
            jmp         err
:
            jsr         halloc
            bcs         @done
            lda         #0
            ldy         #ML_NEXT
            sta         (hp),Y
            iny
            sta         (hp),Y
            iny
            lda         p1
            sta         (hp),Y
            ldx         #0
            ldy         #ML_TEXT
:
            cpx         p1
            beq         :+
            lda         line,X
            sta         (hp),Y
            iny
            inx
            bra         :-
:
            lda         mprev                               ; Linked from the last line, or the record
            ora         mprev + 1
            bne         @link
            lda         mdef
            ldx         mdef + 1
            jsr         hsel
            ldy         #MD_FIRST
            bra         @put
@link:
            lda         mprev
            ldx         mprev + 1
            jsr         hsel
            ldy         #ML_NEXT
@put:
            lda         sref
            sta         (hp),Y
            iny
            lda         sref + 1
            sta         (hp),Y
            lda         sref
            sta         mprev
            lda         sref + 1
            sta         mprev + 1
@done:
            rts

; .endmacro with no .macro
d_endmacro:
            jmp         d_nomatch

; .exitmacro: the innermost macro's use ended
d_exitmacro:
            ldx         frames
@frame:
            dex
            bmi         @none
            lda         ftype,X
            cmp         #FR_MACRO
            bne         @frame
            stz         fnextl,X                            ; (Its next line: none)
            stz         fnexth,X
            rts
@none:
            LDR         r0, s_nomatch
            jmp         err

; A macro's use (tok its name, hp at its symbol; li after it): its arguments (to commas at the top, outside "",
; ( ) and { }; a { } around one taken away) into a new frame's buffer, and the frame on the input
expand:
            jsr         sym_get
            sta         mdef
            stx         mdef + 1
            jsr         new_frame                           ; (.X: the frame; p1: its buffer)
            bcc         :+
            rts
:
            stx         mfr
            stz         aoff
            lda         #FR_MACRO
            sta         ftype,X
            lda         mdef
            ldx         mdef + 1
            jsr         hsel
            ldy         #MD_NP
            lda         (hp),Y
            sta         npar
            ldy         #MD_FIRST
            lda         (hp),Y
            ldx         mfr
            sta         fnextl,X
            iny
            lda         (hp),Y
            sta         fnexth,X
            lda         mdef
            sta         fdefl,X
            lda         mdef + 1
            sta         fdefh,X
            ldy         #0                                  ; (.Y: in the buffer)
            stz         p2                                  ; (p2: the arguments so far)
@arg:
            jsr         skipws
            jsr         atend
            bne         :+
            jmp         @end
:
            ldx         li                                  ; One: to a , at the top
            stz         p2 + 1                              ; (Depth in ( ) and { })
            stz         p3                                  ; (In "")
            lda         line,X
            cmp         #'{'
            bne         @chars
            inx                                             ; ({ }: taken away)
            stx         li
            lda         #1
            sta         p3 + 1                              ; (p3 + 1: in braces)
            bra         @chars0
@chars:
            stz         p3 + 1
@chars0:
@char:
            lda         line,X
            beq         @argend
            ldy         p3
            bne         @instr
            cmp         #';'
            beq         @argend
            cmp         #'"'
            beq         @quote
            cmp         #'('
            beq         @in
            cmp         #'{'
            beq         @in
            cmp         #')'
            beq         @out
            cmp         #'}'
            bne         :+
            lda         p2 + 1
            bne         @out
            lda         p3 + 1                              ; (The } that ends a { } argument)
            beq         @put0
            bra         @argend
:
            cmp         #','
            bne         @put0
            lda         p2 + 1                              ; (Not inside ( ) or { })
            ora         p3 + 1
            beq         @argend
            bra         @put0
@quote:
            lda         p3
            eor         #1
            sta         p3
            bra         @put0
@instr:
            cmp         #'"'
            beq         @quote
            bra         @put0
@in:
            inc         p2 + 1
            bra         @put0
@out:
            dec         p2 + 1
@put0:
            inx
            bra         @char
@argend:
            lda         line,X                              ; (.X: its end; li its start)
            pha
            stz         line,X
            phx
            jsr         argcopy
            plx
            pla
            sta         line,X
            lda         p3 + 1                              ; (Past a })
            beq         :+
            lda         line,X
            cmp         #'}'
            bne         :+
            inx
:
            stx         li
            inc         p2
            lda         #','
            jsr         expect
            bcs         @end
            jmp         @arg
@end:
            lda         p2                                  ; Too many?
            cmp         npar
            beq         :+
            bcc         :+
            LDR         r0, s_args
            jmp         err
:
            lda         npar                                ; The ones not given: blank
            sec
            sbc         p2
            beq         @done
            tax
:
            lda         #0
            phx
            jsr         argput
            plx
            dex
            bne         :-
@done:
            rts

; The argument (li to its 0, blanks at its ends taken away) on the end of the frame's buffer, and a 0
argcopy:
            jsr         skipws
            ldx         li                                  ; (Its end, less blanks)
:
            lda         line,X
            beq         :+
            inx
            bra         :-
:
            dex
            cpx         li
            bcc         :+
            lda         line,X
            cmp         #' '
            beq         :-
            cmp         #TAB
            beq         :-
:
            inx
            stx         p3
            ldx         li
@byte:
            cpx         p3
            beq         @end
            lda         line,X
            phx
            ldx         mfr
            jsr         argput
            plx
            inx
            bra         @byte
@end:
            lda         #0
            ldx         mfr
            ; (on into argput)

; .A on the end of the frame's buffer (p1 at it; aoff its bytes so far).  Keeps .X
argput:
            ldy         aoff
            cpy         #255
            bcs         :+
            sta         (p1),Y
            inc         aoff
            rts
:
            lda         #0
            sta         (p1),Y
            rts

; A line of the macro in the innermost frame (.X) into line, each of its parameters made its argument.  C = 1:
; none left
mline:
            stx         mfr
            lda         fnextl,X
            ora         fnexth,X
            bne         :+
            sec
            rts
:
            lda         fnextl,X                            ; Its text into mtext
            pha
            lda         fnexth,X
            tax
            pla
            jsr         hsel
            ldy         #ML_LEN
            lda         (hp),Y
            sta         p2
            ldx         #0
            ldy         #ML_TEXT
:
            cpx         p2
            beq         :+
            lda         (hp),Y
            sta         mtext,X
            iny
            inx
            bra         :-
:
            stz         mtext,X
            ldy         #ML_NEXT                            ; Its next line
            ldx         mfr
            lda         (hp),Y
            sta         fnextl,X
            iny
            lda         (hp),Y
            sta         fnexth,X
            jsr         frame_buf                           ; (p1: the arguments)
            lda         fdefl,X                             ; (p3: the record, for its parameters)
            sta         p3
            lda         fdefh,X
            sta         p3 + 1
            ldx         #0                                  ; .X: in mtext; .Y: in line
            ldy         #0
            stz         mq
@byte:
            lda         mtext,X
            beq         @end
            cmp         #'"'                                ; (In a string: as it is)
            bne         :+
            lda         mq
            eor         #1
            sta         mq
            lda         #'"'
            bra         @plain
:
            pha
            lda         mq
            bne         @inq
            pla
            jsr         isstart
            bcs         @name
            bra         @plain
@inq:
            pla
@plain:
            sta         line,Y
            inx
            iny
            beq         @long
            bra         @byte
@name:
            stx         p2                                  ; A name: one of its parameters?
            sty         p2 + 1
            jsr         param
            bcs         @copy
            ldy         p2 + 1                              ; (Yes: its argument, .A)
            jsr         argsub
            bcs         @long
            bra         @byte
@copy:
            ldx         p2                                  ; (No: as it is)
            ldy         p2 + 1
:
            lda         mtext,X
            sta         line,Y
            inx
            iny
            beq         @long
            lda         mtext,X
            jsr         isnamec
            bcs         :-
            bra         @byte
@end:
            lda         #0
            sta         line,Y
            clc
            rts
@long:
            stz         line + LINE_MAX
            LDR         r0, s_toolong
            jsr         err
            clc
            rts

; C = 1: .A can start a name (a letter, _, @, .).  Keeps .A, .X, .Y
isstart:
            cmp         #'_'
            beq         @yes
            cmp         #'@'
            beq         @yes
            cmp         #'.'
            beq         @yes
            pha
            jsr         lower
            cmp         #'a'
            bcc         @no
            cmp         #'z' + 1
            bcs         @no
            pla
@yes:
            sec
            rts
@no:
            pla
            clc
            rts

; C = 1: .A can be in a name after its first byte.  Keeps .A, .X, .Y
isnamec:
            cmp         #'_'
            beq         @yes
            cmp         #'0'
            bcc         @no
            cmp         #'9' + 1
            bcc         @yes
            pha
            jsr         lower
            cmp         #'a'
            bcc         @no1
            cmp         #'z' + 1
            bcs         @no1
            pla
@yes:
            sec
            rts
@no1:
            pla
@no:
            clc
            rts

; The name at mtext + p2: one of the macro's parameters (p3: its record)?  C = 0: its index in .A, .X past the
; name; C = 1: not one
param:
            lda         p3
            ldx         p3 + 1
            jsr         hsel
            ldy         #MD_NP
            lda         (hp),Y
            sta         npar
            iny
            stz         nbit                                ; (The parameter's index)
@par:
            lda         nbit
            cmp         npar
            bcs         @no
            lda         (hp),Y                              ; (Its length)
            sta         force
            iny
            ldx         p2
@cmp:
            lda         (hp),Y
            cmp         mtext,X
            bne         @skip
            iny
            inx
            dec         force
            bne         @cmp
            lda         mtext,X                             ; All of it, and the name ends there?
            jsr         isnamec
            bcs         @skip1
            lda         nbit
            clc
            rts
@skip:
            iny                                             ; (Past the rest of it)
            dec         force
            bne         @skip
            inc         nbit
            bra         @par
@skip1:
            inc         nbit
            bra         @par
@no:
            sec
            rts

; Argument .A (of the frame's buffer: p1) into line at .Y on.  OUT: .Y past it; C = 1: too long
argsub:
            phx
            tax                                             ; (Its start: past .A zeros)
            sty         p2 + 1
            ldy         #0
:
            cpx         #0
            beq         :++
:
            lda         (p1),Y
            iny
            cmp         #0
            bne         :-
            dex
            bra         :--
:
            ldx         p2 + 1
:
            lda         (p1),Y
            beq         @end
            sta         line,X
            iny
            inx
            beq         @long
            bra         :-
@end:
            txa
            tay
            plx
            clc
            rts
@long:
            plx
            sec
            rts

; ---- Instructions

; The name in tok an instruction's?  C = 0: nidx its index (nbit: RMB, SMB, BBR, BBS's bit); C = 1: no
mnemonic:
            lda         tlen
            cmp         #3
            beq         @three
            cmp         #4
            bne         @no
            lda         tok + 3                             ; (rmb0 ... bbs7)
            sec
            sbc         #'0'
            cmp         #8
            bcs         @no
            sta         nbit
            jsr         find3
            bcs         @no
            lda         nidx
            cmp         #N_RMB
            beq         @yes
            cmp         #N_SMB
            beq         @yes
            cmp         #N_BBR
            beq         @yes
            cmp         #N_BBS
            beq         @yes
@no:
            sec
            rts
@three:
            jsr         find3
            bcs         @no
            lda         nidx
            cmp         #N_RMB
            beq         @no
            cmp         #N_SMB
            beq         @no
            cmp         #N_BBR
            beq         @no
            cmp         #N_BBS
            beq         @no
@yes:
            clc
            rts

; nidx = the index of tok's first three letters among the names (its first letter's chain).  C = 1: none
find3:
            lda         tok + 1
            jsr         lower
            sta         c1
            lda         tok + 2
            jsr         lower
            sta         c2
            lda         tok
            jsr         lower
            sec
            sbc         #'a'
            cmp         #26
            bcs         @no
            tax
            lda         nfirst,X
@name:
            cmp         #$FF
            beq         @no
            tax
            sta         n3                                  ; (Its letters: names + 3 * it)
            asl         a
            adc         n3
            tay
            lda         names + 1,Y
            cmp         c1
            bne         @next
            lda         names + 2,Y
            cmp         c2
            bne         @next
            stx         nidx
            clc
            rts
@next:
            lda         nnext,X
            bra         @name
@no:
            sec
            rts

; The opcode of instruction nidx in mode .A (optab's).  C = 0: .A; C = 1: none
opcode:
            sta         mode
            cmp         #MODES
            bcs         @none
            tax
            lda         rowl,X
            sta         opp
            lda         rowh,X
            sta         opp + 1
            ldy         nidx
            lda         (opp),Y
            cmp         #$FF
            beq         @none
            cpy         #N_RMB                              ; (RMB, SMB, BBR, BBS: their bit, in bits 4-6)
            beq         @bit
            cpy         #N_SMB
            beq         @bit
            cpy         #N_BBR
            beq         @bit
            cpy         #N_BBS
            beq         @bit
            clc
            rts
@bit:
            sta         opp
            lda         nbit
            asl         a
            asl         a
            asl         a
            asl         a
            ora         opp
            clc
            rts
@none:
            sec
            rts

; The tables, made at pass 1's start: optab (each mode's row: each name's opcode in that mode; RMB, SMB, BBR and
; BBS their bit 0's), and the names' chains by their first letter
op_init:
            ldx         #MODES - 1                          ; optab all $FF
@row:
            lda         rowl,X
            sta         opp
            lda         rowh,X
            sta         opp + 1
            ldy         #NAMES - 1
            lda         #$FF
:
            sta         (opp),Y
            dey
            bpl         :-
            dex
            bpl         @row
            ldx         #0                                  ; Each opcode in its mode's row
@op:
            ldy         op_name,X
            cpy         #N_NONE
            beq         @next
            txa                                             ; (RMB ... BBS: bit 0's alone)
            and         #$70
            beq         @put
            cpy         #N_RMB
            beq         @next
            cpy         #N_SMB
            beq         @next
            cpy         #N_BBR
            beq         @next
            cpy         #N_BBS
            beq         @next
@put:
            lda         op_mode,X
            cmp         #MODES
            bcs         @next                               ; (16 and up: none)
            phy
            tay
            lda         rowl,Y
            sta         opp
            lda         rowh,Y
            sta         opp + 1
            ply
            txa
            sta         (opp),Y
@next:
            inx
            bne         @op
            ldx         #25                                 ; The names' chains
            lda         #$FF
:
            sta         nfirst,X
            dex
            bpl         :-
            ldx         #NAMES - 1
            ldy         #(NAMES - 1) * 3
@name:
            cpx         #N_NONE
            beq         @skip
            phy
            lda         names,Y
            sec
            sbc         #'a'
            tay
            lda         nfirst,Y
            sta         nnext,X
            txa
            sta         nfirst,Y
            ply
@skip:
            dey
            dey
            dey
            dex
            bpl         @name
            rts

; C = 0: instruction nidx has mode .A.  Keeps .X
hasmode:
            phx
            jsr         opcode
            plx
            rts

; An instruction (nidx; li after its name): its operand's mode, then its bytes
instruction:
            stz         force
            jsr         skipws
            jsr         peek
            bne         :+
            jmp         @none
:
            cmp         #'#'
            bne         :+
            jsr         eat
            jsr         expr
            bcs         @rts0
            jsr         keep
            jsr         trailing
            lda         #M_IMM
            jmp         @emit
@rts0:
            rts
@bad0:
            jmp         @bad
:
            cmp         #'('
            bne         @plain
            lda         li                                  ; ( ... : indirect, if it's ( expr , x ), ( expr ) , y
            sta         li0                                 ;   or ( expr ) alone
            jsr         eat
            jsr         expr
            bcs         @rts0
            jsr         keep
            lda         #','
            jsr         expect
            bcs         @ind
            jsr         xreg
            bcs         @bad0
            lda         #')'
            jsr         expect
            bcs         @bad0
            jsr         trailing
            lda         #M_IZX
            ldx         #M_IAX
            jmp         @zpabs
@ind:
            lda         #')'
            jsr         expect
            bcs         @bad0
            lda         #','
            jsr         expect
            bcs         @ind1
            jsr         yreg
            bcs         @bad0
            jsr         trailing
            lda         #M_IZY
            jmp         @emit
@ind1:
            jsr         atend
            bne         @reparse
            lda         #M_IZP
            ldx         #M_IND
            jmp         @zpabs
@reparse:
            lda         li0                                 ; (An expression that starts with a (: no
            sta         li                                  ;   indirect after all)
            bra         @expr
@plain:
            jsr         aonly                               ; a alone: the accumulator
            bcs         :+
            lda         #M_ACC
            jmp         @emit
:
            ldy         li                                  ; a: or z:
            lda         line + 1,Y
            cmp         #':'
            bne         @expr
            lda         line,Y
            jsr         lower
            cmp         #'a'
            bne         :+
            lda         #$80
            bra         :++
:
            cmp         #'z'
            bne         @expr
            lda         #$40
:
            sta         force
            inc         li
            inc         li
@expr:
            lda         nidx                                ; BBR, BBS: zp, target
            cmp         #N_BBR
            beq         @zpr
            cmp         #N_BBS
            beq         @zpr
            jsr         expr
            bcc         :+
            rts
:
            jsr         keep
            lda         #M_REL                              ; A branch?
            jsr         hasmode
            bcs         :+
            jsr         trailing
            lda         #M_REL
            jmp         @emit
:
            lda         #','
            jsr         expect
            bcs         @abs
            jsr         xreg
            bcs         :+
            jsr         trailing
            lda         #M_ZPX
            ldx         #M_ABX
            bra         @zpabs
:
            jsr         yreg
            bcs         @bad
            jsr         trailing
            lda         #M_ZPY
            ldx         #M_ABY
            bra         @zpabs
@abs:
            jsr         trailing
            lda         #M_ZP
            ldx         #M_ABS
            bra         @zpabs
@zpr:
            jsr         expr
            bcs         @rts
            jsr         keep
            lda         #','
            jsr         expect
            bcs         @bad
            lda         #2                                  ; (The zp in addr; the target, * in it 2 on, past the
            jsr         soffadd                             ;   opcode and the zp, as ca65 has it)
            jsr         expr
            php
            lda         #$FE
            jsr         soffadd
            plp
            bcs         @rts
            ldx         #3
:
            lda         val,X
            sta         target,X
            dex
            bpl         :-
            jsr         trailing
            lda         #M_ZPR
            jmp         @emit
@none:
            lda         #M_IMP                              ; Nothing after it: implied, or the accumulator
            jsr         hasmode
            lda         #M_IMP
            bcc         :+
            lda         #M_ACC
:
            jmp         @emit
@bad:
            LDR         r0, s_operand
            jmp         err
@rts:
            rts

; Zero page form .A or absolute .X: the zero page one if it has it and the address may be (or z:), else the
; absolute if it has it, else the zero page
@zpabs:
            sta         p1
            stx         p1 + 1
            bit         force
            bmi         @useabs
            bvs         @usezp
            lda         addrz
            bne         @useabs
@usezp:
            lda         p1
            jsr         hasmode
            bcc         @zp
@useabs:
            lda         p1 + 1
            jsr         hasmode
            bcc         @a
            lda         p1
            bra         @emit
@zp:
            lda         p1
            bra         @emit
@a:
            lda         p1 + 1
            ; (on into @emit)

; Its bytes in mode .A
@emit:
            jsr         opcode
            bcc         :+
            LDR         r0, s_mode
            jmp         err
:
            jsr         emit
            ldx         mode
            lda         modelen,X
            cmp         #2
            bcc         @done
            bne         @two
            cpx         #M_REL                              ; One byte: the address's, or the branch's offset
            beq         @rel
            cpx         #M_IMM
            beq         @imm
            jsr         zp_ok
            lda         addr
            jmp         emit
@imm:
            jsr         byte_ok2
            lda         addr
            jmp         emit
@rel:
            lda         #2
            jsr         reloff
            jmp         emit
@two:
            cpx         #M_ZPR
            beq         @zprb
            lda         addr                                ; Two: the address
            jsr         emit
            lda         addr + 1
            jmp         emit
@zprb:
            jsr         zp_ok                               ; (BBR, BBS: the zp, then the offset to the target)
            lda         addr
            jsr         emit
            ldx         #3
:
            lda         target,X
            sta         addr,X
            dex
            bpl         :-
            lda         #3
            jsr         reloff
            jmp         emit
@done:
            rts

; The operand (val) kept: addr, and addrz (0: it may be the zero page's)
keep:
            ldx         #3
:
            lda         val,X
            sta         addr,X
            dex
            bpl         :-
            jsr         val_zp
            lda         #0
            rol         a
            sta         addrz
            rts

; ,x and ,y: C = 0, the register's name eaten
xreg:
            lda         #'x'
            bra         :+
yreg:
            lda         #'y'
:
            sta         p3 + 1
            jsr         skipws
            jsr         peek
            jsr         lower
            cmp         p3 + 1
            bne         @no
            ldy         li                                  ; (Not the start of a longer name)
            lda         line + 1,Y
            jsr         isnamec
            bcs         @no
            jsr         eat
            clc
            rts
@no:
            sec
            rts

; a alone (then the line's end): C = 0, eaten
aonly:
            jsr         peek
            jsr         lower
            cmp         #'a'
            bne         @no
            ldy         li
            lda         line + 1,Y
            jsr         isnamec
            bcs         @no
            lda         li
            pha
            jsr         eat
            jsr         atend
            beq         :+
            pla                                         ; (a, then more: a name, say)
            sta         li
            bra         @no
:
            pla
            clc
            rts
@no:
            sec
            rts

; .A = a branch's offset from here + .A (its length) to addr (pass 3: said if it's too far)
reloff:
            clc
            adc         here
            sta         p2
            lda         here + 1
            adc         #0
            sta         p2 + 1
            sec
            lda         addr
            sbc         p2
            sta         p2
            lda         addr + 1
            sbc         p2 + 1
            sta         p2 + 1
            lda         pass
            cmp         #3
            bne         @ok
            lda         p2                                  ; -128 to 127: the high byte the low's sign
            asl         a
            lda         p2 + 1
            adc         #0
            beq         @ok
            LDR         r0, s_far
            jsr         err
@ok:
            lda         p2
            rts

; Pass 3: addr on the zero page (said if not)
zp_ok:
            lda         pass
            cmp         #3
            bne         :+
            lda         addr + 1
            ora         addr + 2
            ora         addr + 3
            beq         :+
            LDR         r0, s_zp
            jsr         err
:
            rts

; Pass 3: val, or addr, a byte (-128 to 255; said if not)
byte_ok:
            ldx         #3
:
            lda         val,X
            sta         addr,X
            dex
            bpl         :-
byte_ok2:
            lda         pass
            cmp         #3
            bne         @ok
            lda         addr + 1
            and         addr + 2
            and         addr + 3
            cmp         #$FF
            beq         @ok
            lda         addr + 1
            ora         addr + 2
            ora         addr + 3
            beq         @ok
            LDR         r0, s_byte
            jsr         err
@ok:
            rts

; ---- Bytes

; .A at the place assembled at (pass 3: into the image, if the segment's loaded), the segment's offset on one.
; In BSS or the zero page: data said (pass 1) but for .res's
emit:
            pha
            lda         cseg
            cmp         #SEG_BSS
            beq         @bss
            cmp         #SEG_ZP
            bne         emit_go
@bss:
            lda         pass
            cmp         #1
            bne         emit_go
            LDR         r0, s_inbss
            jsr         err
            bra         emit_go

; .res's: no data said in BSS, the zero page
emit_res:
            pha
emit_go:
            lda         pass
            cmp         #3
            bne         @on
            lda         cseg
            cmp         #SEG_BSS
            beq         @on
            cmp         #SEG_ZP
            beq         @on
            ldx         cseg                                ; The image's byte
            clc
            lda         sbasel,X
            adc         soffl,X
            sta         p3
            lda         sbaseh,X
            adc         soffh,X
            sta         p3 + 1
            pla
            pha
            jsr         emit_at
@on:
            pla
            ldx         cseg
            inc         soffl,X
            bne         :+
            inc         soffh,X
:
            rts

.rodata
s_noendif:  .byte       "an .if with no .endif", 0
s_noendm:   .byte       "a .macro with no .endmacro", 0
s_phase:    .byte       "a segment's size changed between passes (a size that depends on a value after it)", 0
s_what:     .byte       "a name expected", 0
s_unknown:  .byte       "not an instruction, directive or macro", 0
s_twice:    .byte       "defined twice", 0
s_trail:    .byte       "more on the line than it takes", 0
s_deep:     .byte       "nested too deep", 0
s_nomatch:  .byte       "with nothing to match it", 0
s_dir:      .byte       "a directive as doesn't know", 0
s_short:    .byte       ".incbin: the file's shorter", 0
s_ioerr:    .byte       ".incbin: the file can't be read", 0
s_seg:      .byte       "a segment as doesn't know (ZEROPAGE, HEADER, CODE, RODATA, DATA, BSS)", 0
s_assert:   .byte       "an assertion failed", 0
s_org:      .byte       ".org: only with -b, before any bytes", 0
s_params:   .byte       "too many parameters", 0
s_args:     .byte       "too many arguments", 0
s_toolong:  .byte       "a line of more than 255 bytes", 0
s_operand:  .byte       "a bad operand", 0
s_mode:     .byte       "an addressing mode the instruction doesn't have", 0
s_far:      .byte       "a branch too far", 0
s_zp:       .byte       "not a zero page address", 0
s_byte:     .byte       "not a byte", 0
s_inbss:    .byte       "data in BSS or the zero page", 0
s_warning:  .byte       "warning", 0
s_ldwarning: .byte      "ldwarning", 0

; The directives (lower case) and their routines
dirs:
            .word       s_d_byte, d_byte
            .word       s_d_byt, d_byte
            .word       s_d_word, d_word
            .word       s_d_addr, d_word
            .word       s_d_dword, d_dword
            .word       s_d_res, d_res
            .word       s_d_asciiz, d_asciiz
            .word       s_d_include, d_include
            .word       s_d_incbin, d_incbin
            .word       s_d_macro, d_macro
            .word       s_d_mac, d_macro
            .word       s_d_endmacro, d_endmacro
            .word       s_d_endmac, d_endmacro
            .word       s_d_exitmacro, d_exitmacro
            .word       s_d_exitmac, d_exitmacro
            .word       s_d_segment, d_segment
            .word       s_d_code, d_code
            .word       s_d_rodata, d_rodata
            .word       s_d_data, d_data
            .word       s_d_bss, d_bss
            .word       s_d_zeropage, d_zeropage
            .word       s_d_pushseg, d_pushseg
            .word       s_d_popseg, d_popseg
            .word       s_d_assert, d_assert
            .word       s_d_error, d_error
            .word       s_d_warning, d_warning
            .word       s_d_org, d_org
            .word       s_d_import, d_ignore
            .word       s_d_importzp, d_ignore
            .word       s_d_export, d_ignore
            .word       s_d_exportzp, d_ignore
            .word       s_d_global, d_ignore
            .word       s_d_globalzp, d_ignore
            .word       s_d_setcpu, d_ignore
            .word       s_d_pc02, d_ignore
            .word       s_d_p02, d_ignore
            .word       s_d_feature, d_ignore
            .word       s_d_macpack, d_ignore
            .word       s_d_debuginfo, d_ignore
            .word       s_d_list, d_ignore
            .word       s_d_listbytes, d_ignore
            .word       s_d_case, d_ignore
            .word       s_d_smart, d_ignore
            .word       s_d_autoimport, d_ignore
cdirs:
            .word       s_d_if, d_if
            .word       s_d_ifdef, d_ifdef
            .word       s_d_ifndef, d_ifndef
            .word       s_d_ifblank, d_ifblank
            .word       s_d_ifnblank, d_ifnblank
            .word       s_d_elseif, d_elseif
            .word       s_d_else, d_else
            .word       s_d_endif, d_endif
            .word       0, 0                                ; (The end of both)

s_d_byte:   .byte       ".byte", 0
s_d_byt:    .byte       ".byt", 0
s_d_word:   .byte       ".word", 0
s_d_addr:   .byte       ".addr", 0
s_d_dword:  .byte       ".dword", 0
s_d_res:    .byte       ".res", 0
s_d_asciiz: .byte       ".asciiz", 0
s_d_include: .byte      ".include", 0
s_d_incbin: .byte       ".incbin", 0
s_d_if:     .byte       ".if", 0
s_d_ifdef:  .byte       ".ifdef", 0
s_d_ifndef: .byte       ".ifndef", 0
s_d_ifblank: .byte      ".ifblank", 0
s_d_ifnblank: .byte     ".ifnblank", 0
s_d_elseif: .byte       ".elseif", 0
s_d_else:   .byte       ".else", 0
s_d_endif:  .byte       ".endif", 0
s_d_macro:  .byte       ".macro", 0
s_d_mac:    .byte       ".mac", 0
s_d_endmacro: .byte     ".endmacro", 0
s_d_endmac: .byte       ".endmac", 0
s_d_exitmacro: .byte    ".exitmacro", 0
s_d_exitmac: .byte      ".exitmac", 0
s_d_segment: .byte      ".segment", 0
s_d_code:   .byte       ".code", 0
s_d_rodata: .byte       ".rodata", 0
s_d_data:   .byte       ".data", 0
s_d_bss:    .byte       ".bss", 0
s_d_zeropage: .byte     ".zeropage", 0
s_d_pushseg: .byte      ".pushseg", 0
s_d_popseg: .byte       ".popseg", 0
s_d_assert: .byte       ".assert", 0
s_d_error:  .byte       ".error", 0
s_d_warning: .byte      ".warning", 0
s_d_org:    .byte       ".org", 0
s_d_import: .byte       ".import", 0
s_d_importzp: .byte     ".importzp", 0
s_d_export: .byte       ".export", 0
s_d_exportzp: .byte     ".exportzp", 0
s_d_global: .byte       ".global", 0
s_d_globalzp: .byte     ".globalzp", 0
s_d_setcpu: .byte       ".setcpu", 0
s_d_pc02:   .byte       ".pc02", 0
s_d_p02:    .byte       ".p02", 0
s_d_feature: .byte      ".feature", 0
s_d_macpack: .byte      ".macpack", 0
s_d_debuginfo: .byte    ".debuginfo", 0
s_d_list:   .byte       ".list", 0
s_d_listbytes: .byte    ".listbytes", 0
s_d_case:   .byte       ".case", 0
s_d_smart:  .byte       ".smart", 0
s_d_autoimport: .byte   ".autoimport", 0

; The segments' names (a segment's number: its place here)
segnames:
            .word       s_zeropage, s_header, s_code, s_rodata, s_data, s_bss, 0
s_zeropage: .byte       "ZEROPAGE", 0
s_header:   .byte       "HEADER", 0
s_code:     .byte       "CODE", 0
s_rodata:   .byte       "RODATA", 0
s_data:     .byte       "DATA", 0
s_bss:      .byte       "BSS", 0

; optab's rows
rowl:
.repeat MODES, I
            .byte       <(optab + I * NAMES)
.endrepeat
rowh:
.repeat MODES, I
            .byte       >(optab + I * NAMES)
.endrepeat

