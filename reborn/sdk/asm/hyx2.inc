; ****************************************************************************
; hyx2.inc - a module's HYX2 header (docs/conventions.md, "Modules"), for programs and drivers in assembly.  Use
; it with hydra.inc (the calls and constants: made by tools/apigen.js) and modules/module.cfg (the link: code at
; $A000 in the module's own paged ROM bank, run in place; data and BSS in its task's RAM, from $0400).
;
;   HYX2_PROGRAM "name", main                           a program: main gets r0 = its arguments; returning is
;                                                       EXITS with code 0
;   HYX2_DRIVER  "name", init, serve, irq, stop, flags  a driver: init (C = 1 and .A = an error: it ends), then
;                                                       serve for its calls (.Y = the caller) and irq for its lines
;                                                       (0 for none); flags HF_BOOT: started at boot
;
; The header is the first 48 bytes of the module (the HEADER segment).  Its DATA is copied into the task's RAM
; and its BSS cleared before it starts.

.import     __DATA_LOAD__, __DATA_RUN__, __DATA_SIZE__, __BSS_RUN__, __BSS_SIZE__, __ROM_LAST__, __RAM_LAST__

HYX2_LOAD       = $A000
HYX2_NAME_LEN   = 12

.macro HYX2_HEADER  name, type, flags, main, serve, irq, stop, device
.assert     .strlen(name) < HYX2_NAME_LEN, error, "A module's name is 11 characters at most"
.pushseg
.segment "HEADER"
            .byte       "HYX2"                              ; HX_MAGIC
            .byte       48                                  ; HX_HSIZE
            .byte       type                                ; HX_TYPE
            .byte       flags | HF_INPLACE                  ; HX_FLAGS
            .byte       ABI_VERSION                         ; HX_ABI
            .word       HYX2_LOAD                           ; HX_LOAD
            .word       __ROM_LAST__ - HYX2_LOAD            ; HX_LENGTH
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
            .byte       1                                   ; HX_BANKS
            .word       1                                   ; HX_VERSION
            .byte       name                                ; HX_NAME
            .res        HYX2_NAME_LEN - .strlen(name), 0
.segment "DATA"                                             ; (So both segments exist, if empty)
.segment "BSS"
.popseg
.endmacro

.macro HYX2_PROGRAM name, main
            HYX2_HEADER name, HT_PROGRAM, 0, main, 0, 0, 0, 0
.endmacro

.macro HYX2_DRIVER  name, init, serve, irq, stop, flags
            HYX2_HEADER name, HT_DRIVER, flags, init, serve, irq, stop, 0
.endmacro
