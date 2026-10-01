.debuginfo

; ****************************************************************************
; Macros: strings (HString), IO ports, register pushes, moves, counters, printing, addresses

.macro HString Str
    .byte       .strlen(Str), Str
.endmacro

; A client-task server's name for size bytes of the scratch they share (ZP_CS, zero.s): checked to be in it
.macro CS_FITS      name, size
.assert     name >= ZP_CS && name + size <= ZP_CS_END, error, .sprintf("%s isn't in ZP_CS (the client-task servers' scratch)", .string(name))
.endmacro

.macro NamedHString Name, Str
Name:
    HString Str
.endmacro

; FORTH Defines

F_CELL          := 2

; read a byte into A from the IO PORT/Byte (PORT_N | BYTE_M)
.macro IO_PORT_READ     port, byte
.ifblank        byte
                lda     port
.else
                lda     IO_PORT_BYTE port, byte
.endif
.endmacro

.macro PUSH_XY
                phx
                phy
.endmacro

.macro  PULL_YX
                ply
                plx
.endmacro

.macro PUSH_AXY
                pha
                phx
                phy
.endmacro

.macro  PULL_YXA
                ply
                plx
                pla
.endmacro

; convenience macros

; MOV
; Modifies: A
.macro MOV              addr1, addr2
                lda     addr1
                sta     addr2
.endmacro

.macro MOV16_HL         addr1, addr2
                MOV     {addr1 + 1}, {addr2 + 1}
                MOV     {addr1}, {addr2}
.endmacro

.macro MOVAY            addr1, addr2
                lda     addr1, Y
                sta     addr2, Y
.endmacro

.macro MOVAY16          addr1, addr2
                MOVAY   {addr1}, {addr2}
                iny
                MOVAY   {addr1}, {addr2}
.endmacro

.macro  INC16_BARE      addr
                inc     addr
                bne     :+
                inc     addr + 1
:
.endmacro

.macro BNE16    addr1, addr2, dest
                lda     addr1
                cmp     addr2
                bne     dest
                lda     addr1 + 1
                cmp     addr2 + 1
                bne     dest
.endmacro

; PRINT HELPERS
.define LOADA(arg)      lda arg

.macro  LDA_CORA    CharOrAddr
.ifnblank   CharOrAddr
                LOADA           CharOrAddr
.endif
.endmacro

.macro META_CALL    call, C1, C2, C3, C4, C5, C6, C7, C8, C9
    .ifblank C2
                LDA_CORA        {C1}
                jsr             call
                .exitmacro
    .else
                lda             C1
                jsr             call
    .endif
                META_CALL       {call}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
.endmacro

.macro META_JMP     call, C1, C2, C3, C4, C5, C6, C7, C8, C9
    .ifblank C2
                LDA_CORA        {C1}
                jmp             call
                .exitmacro
    .else
                lda             C1
                jsr             call
    .endif
                META_JMP        {call}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
.endmacro

.macro  PRINT_CHAR      C1, C2, C3, C4, C5, C6, C7, C8, C9
                META_CALL       WRITE_CHAR, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
.endmacro

.macro  PRINT_CHAR_JMP  C1, C2, C3, C4, C5, C6, C7, C8, C9
                META_JMP        WRITE_CHAR, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
.endmacro

.macro  PRINT_SPACE
                PRINT_CHAR      #ASCII_SPACE
.endmacro

.macro  PRINT_ESC_SEQ   C1, C2, C3, C4, C5, C6, C7, C8
                PRINT_CHAR      #ASCII_ESC, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}
.endmacro

.macro  PRINT_ESC_SEQ_JMP   C1, C2, C3, C4, C5, C6, C7, C8
                PRINT_CHAR_JMP  #ASCII_ESC, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}
.endmacro

.macro  PRINT_ANSI_ESC_SEQ   C1, C2, C3, C4, C5, C6, C7
                PRINT_CHAR      #ASCII_ESC, #ASCII_LBRACKET, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}
.endmacro

.macro  PRINT_BYTE      C1, C2, C3, C4, C5, C6, C7, C8, C9
                META_CALL       WRITE_BYTE, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
.endmacro

.macro  PRINT_BYTE_JMP  C1, C2, C3, C4, C5, C6, C7, C8, C9
                META_JMP        WRITE_BYTE, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
.endmacro

.macro  PRINT_HEX  C1, C2, C3, C4, C5, C6, C7, C8, C9
                META_CALL       WRITE_HEX, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
.endmacro

.macro  PRINT_HEX_MASK  C1, C2, C3, C4, C5, C6, C7, C8, C9
                META_CALL       WRITE_HEX_MASK, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
.endmacro

.macro  PRINT_CRLF
                jsr             WRITE_CRLF
.endmacro

.macro  PRINT_CRLF_JMP
                jmp             WRITE_CRLF
.endmacro

.macro  PRINT_ADDR  addr_lob
                META_CALL       WRITE_BYTE, {addr_lob+1}, {addr_lob}
.endmacro

.macro  STORE_LABEL_H   label, addr
                lda     #>label
                sta     addr
.endmacro

.macro  STORE_LABEL_L   label, addr
                lda     #<label
                sta     addr
.endmacro

.macro  STORE_LABEL  label, addr
                STORE_LABEL_L {label}, {addr}
                STORE_LABEL_H {label}, {addr+1}
.endmacro

.macro  _M_WRITE_HSTRING        addr
                lda             #<addr
                ldy             #>addr
                jsr             WRITE_HSTRING
.endmacro

; JSR using JMP
.macro  _M_JSRR                 addrTo, addrFrom
                lda             #>(addrFrom-1)
                pha
                lda             #<(addrFrom-1)
                pha
                jmp             (addrTo)
.endmacro

.macro SR_N     n
    .if     n > 0
                lsr
                SR_N    n-1
    .endif
.endmacro

; save a byte on branching, when small in necessary
.macro SKIPNEXT
                .byte   $22     ; Undocumented 2-byte NOP, 2 cycles; uses 1 byte to skip the next byte
.endmacro

.macro BYTECOPY fm, to
                lda         fm
                sta         to
.endmacro

.macro LOAD_ADDR    addr, to
                BYTECOPY    {#>addr}, {to + 1}
                BYTECOPY    {#<addr}, {to}
.endmacro
