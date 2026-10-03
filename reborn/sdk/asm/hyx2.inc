; ****************************************************************************
; hyx2.inc - a module's HYX2 header (docs/conventions.md, "Modules"), for programs and drivers in assembly.  Use
; it with hydra.inc (the calls and constants: made by tools/apigen.js) and modules/module.cfg (the link: code at
; $A000 in the module's own paged ROM bank, run in place; data and BSS in its task's RAM, from $0400).  Or, for a
; RAM program (a program in a file, which SPAWN's loader reads into its task's RAM), assemble with -D HYX2_RAM and
; link with sdk/asm/hyx2.cfg: its header, code and data from $0800, its BSS after them.
;
;   HYX2_PROGRAM "name", main [, banks]                 a program: main gets r0 = its arguments; returning is
;                                                       EXITS with code 0
;   HYX2_DRIVER  "name", init, serve, irq, stop, flags [, banks]
;                                                       a driver: init (C = 1 and .A = an error: it ends), then
;                                                       serve for its calls (.Y = the caller) and irq for its lines
;                                                       (0 for none); flags HF_BOOT: started at boot
;
; The header is the first 48 bytes of the module (the HEADER segment).  Its DATA is copied into the task's RAM
; and its BSS cleared before it starts.
;
; A module of two banks (banks = 2, linked by modules/module2.cfg) has its second bank's code and read-only data
; in the segments CODE2 and RODATA2, at the same addresses as the first's ($A000-$DFFF), so neither bank calls the
; other directly: FAR2 routine calls a routine in the second bank from the first, FAR1 routine one in the first
; from the second (.A, .X, .Y and C go in and come back; N and Z are .A's).  The trampolines are in the module's
; RAM (its DATA), where both banks see them, and switch the task's own bank register ($01); HYX2_BANKS_INIT,
; first thing in its init or main, notes the module's banks.  Such a module owns no IRQ line: its irq entry runs
; in its own task, and might find the other bank at $A000.

.import     __DATA_LOAD__, __DATA_RUN__, __DATA_SIZE__, __BSS_RUN__, __BSS_SIZE__, __RAM_LAST__

.ifdef HYX2_RAM
HYX2_LOAD       = $0800                                     ; (A RAM program: read there by SPAWN)
HYX2_INPLACE    = 0
.else
.import     __ROM_LAST__
HYX2_LOAD       = $A000                                     ; (A module of the paged ROM: run in place)
HYX2_INPLACE    = HF_INPLACE
.endif
HYX2_NAME_LEN   = 12

.macro HYX2_HEADER  name, type, flags, main, serve, irq, stop, device, banks
.assert     .strlen(name) < HYX2_NAME_LEN, error, "A module's name is 11 characters at most"
.ifblank banks
HYX2_BANKS      = 1
.else
HYX2_BANKS      = banks
.endif
.assert     HYX2_BANKS = 1 .or HYX2_BANKS = 2, error, "A module is one bank or two"
.ifdef HYX2_RAM
.assert     HYX2_BANKS = 1, error, "A RAM program is one image"
.endif
.pushseg
.segment "HEADER"
            .byte       "HYX2"                              ; HX_MAGIC
            .byte       48                                  ; HX_HSIZE
            .byte       type                                ; HX_TYPE
            .byte       flags | HYX2_INPLACE                ; HX_FLAGS
            .byte       ABI_VERSION                         ; HX_ABI
            .word       HYX2_LOAD                           ; HX_LOAD
.ifdef HYX2_RAM
            .word       __DATA_LOAD__ + __DATA_SIZE__ - HYX2_LOAD ; HX_LENGTH (to its data's end)
.elseif HYX2_BANKS = 1
            .word       __ROM_LAST__ - HYX2_LOAD            ; HX_LENGTH (in its last bank)
.else
.import     __ROM2_LAST__
            .word       __ROM2_LAST__ - HYX2_LOAD
.endif
            .word       __DATA_LOAD__                       ; HX_DATA_LOAD
            .word       __DATA_RUN__                        ; HX_DATA_RUN
            .word       __DATA_SIZE__                       ; HX_DATA_LEN
            .word       __BSS_RUN__                         ; HX_BSS
            .word       __BSS_SIZE__                        ; HX_BSS_LEN
            .word       __RAM_LAST__                        ; HX_TOP
            .word       main                                ; HX_MAIN
            .word       serve                               ; HX_SERVE
            .word       irq                                 ; HX_IRQ
            .word       stop                                ; HX_STOP
            .byte       device                              ; HX_DEVICE
            .byte       HYX2_BANKS                          ; HX_BANKS
            .word       1                                   ; HX_VERSION
            .byte       name                                ; HX_NAME
            .res        HYX2_NAME_LEN - .strlen(name), 0
.segment "DATA"                                             ; (So both segments exist, if empty)
.segment "BSS"
.if HYX2_BANKS = 2
            HYX2_TRAMPOLINES
.endif
.popseg
.endmacro

.macro HYX2_PROGRAM name, main, banks
            HYX2_HEADER name, HT_PROGRAM, 0, main, 0, 0, 0, 0, banks
.endmacro

.macro HYX2_DRIVER  name, init, serve, irq, stop, flags, banks
            HYX2_HEADER name, HT_DRIVER, flags, init, serve, irq, stop, 0, banks
.endmacro

; A two-bank module's trampolines, in its RAM, and their zero page
.macro HYX2_TRAMPOLINES
.zeropage
hyx2_to:    .res        2                                   ; The routine called
hyx2_a:     .res        1                                   ; (.A, while it's set)
.segment "DATA"
hyx2_far2:                                                  ; The second bank's routine, from the first
            pha
            lda         hyx2_bank2
            sta         $01                                 ; (This task's paged ROM bank)
            pla
            jsr         hyx2_go
            pha
            lda         hyx2_bank1
            sta         $01
            pla
            rts

hyx2_far1:                                                  ; The first bank's routine, from the second
            pha
            lda         hyx2_bank1
            sta         $01
            pla
            jsr         hyx2_go
            pha
            lda         hyx2_bank2
            sta         $01
            pla
            rts

hyx2_go:
            jmp         (hyx2_to)

hyx2_bank1: .byte       0                                   ; The module's banks (HYX2_BANKS_INIT)
hyx2_bank2: .byte       0
.endmacro

; The module's banks noted: the first (this one, as it starts) and the next.  Modifies: .A
.macro HYX2_BANKS_INIT
            lda         $01
            sta         hyx2_bank1
            inc
            sta         hyx2_bank2
.endmacro

; Call routine in the second bank, from the first (FAR2), or in the first from the second (FAR1)
.macro FAR2 routine
            sta         hyx2_a
            lda         #<routine
            sta         hyx2_to
            lda         #>routine
            sta         hyx2_to + 1
            lda         hyx2_a
            jsr         hyx2_far2
.endmacro

.macro FAR1 routine
            sta         hyx2_a
            lda         #<routine
            sta         hyx2_to
            lda         #>routine
            sta         hyx2_to + 1
            lda         hyx2_a
            jsr         hyx2_far1
.endmacro
