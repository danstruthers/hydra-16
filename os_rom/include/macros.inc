.debuginfo

; ****************************************************************************
; Macros: strings (HString), IO ports, register pushes, moves, counters, printing, addresses

.macro HString Str
    .byte       .strlen(Str), Str
.endmacro

.macro NamedHString Name, Str
Name:
    HString Str
.endmacro

; FORTH Defines

F_CELL          := 2

; write a byte in A to the IO PORT
.macro IO_PORT_WRITE    port, byte, imm
.ifnblank       imm
                lda     #imm
.endif
.ifblank        byte
                sta     port
.else
                sta     IO_PORT_BYTE port, byte
.endif
.endmacro

; read a byte into A from the IO PORT/Byte (PORT_N | BYTE_M)
.macro IO_PORT_READ     port, byte
.ifblank        byte
                lda     port
.else
                lda     IO_PORT_BYTE port, byte
.endif
.endmacro

.macro PUSH_AX
                pha
                phx
.endmacro

.macro  PULL_XA
                plx
                pla
.endmacro

.macro PUSH_AY
                pha
                phy
.endmacro

.macro  PULL_YA
                ply
                pla
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

.macro MOVA             addr1, addr2
                MOV     {addr1}, {addr2}
.endmacro

.macro MOVX             addr1, addr2
                ldx     addr1
                stx     addr2
.endmacro

.macro MOVY             addr1, addr2
                ldy     addr1
                sty     addr2
.endmacro

.macro MOV16            addr1, addr2
                MOV     {addr1}, {addr2}
                MOV     {addr1 + 1}, {addr2 + 1}
.endmacro

.macro MOV16_HL         addr1, addr2
                MOV     {addr1 + 1}, {addr2 + 1}
                MOV     {addr1}, {addr2}
.endmacro

.macro MOVA16           addr1, addr2
                MOV16   {addr1}, {addr2}
.endmacro

.macro MOVA16_HL        addr1, addr2
                MOV16_HL   {addr1}, {addr2}
.endmacro

.macro MOVX16           addr1, addr2
                MOVX    {addr1}, {addr2}
                MOVX    {addr1 + 1}, {addr2 + 1}
.endmacro

.macro MOVX16_HL        addr1, addr2
                MOVX    {addr1 + 1}, {addr2 + 1}
                MOVX    {addr1}, {addr2}
.endmacro

.macro MOVY16           addr1, addr2
                MOVY    {addr1}, {addr2}
                MOVY    {addr1 + 1}, {addr2 + 1}
.endmacro

.macro MOVY16_HL        addr1, addr2
                MOVY    {addr1 + 1}, {addr2 + 1}
                MOVY    {addr1}, {addr2}
.endmacro

.macro MOVAX            addr1, addr2
                lda     addr1, X
                sta     addr2, X
.endmacro

.macro MOVAY            addr1, addr2
                lda     addr1, Y
                sta     addr2, Y
.endmacro

.macro MOVAX16          addr1, addr2
                MOVAX   {addr1}, {addr2}
                inx
                MOVAX   {addr1}, {addr2}
.endmacro

.macro MOVAY16          addr1, addr2
                MOVAY   {addr1}, {addr2}
                iny
                MOVAY   {addr1}, {addr2}
.endmacro

; X: # of bytes to move
; Clobbers A
.macro BLKMOVX          addr1, addr2
                phy
                txa
                tay
:
                dey
                lda     addr1, Y
                sta     addr2, Y
                bne     :-
                ply
.endmacro

; Y: # of bytes to move
; Clobbers A, Y
.macro BLKMOVY          addr1, addr2
:
                dey
                lda     addr1, Y
                sta     addr2, Y
                bne     :-
.endmacro

.macro  _M_ADDTO        addr
                adc     {addr}
                sta     {addr}
.endmacro

; _M_INCC: inc and set C/V if rollover.  Clobbers .A, C
.macro  _M_INCC    addr
                sec
                lda     #0
                _M_ADDTO    {addr}
.endmacro

.macro  _M_INCC16       addr
                inc     addr
                bne     :+
                _M_INCC addr + 1
                bra     :++
:
                lda     addr + 1
                ora     #1
:
.endmacro

.macro  _M_INCC32       addr
                inc     addr
                bne     :+
                inc     addr+1
                bne     :+
                _M_INCC16   addr+2
.endmacro

.macro  INC16_BARE      addr
                inc     addr
                bne     :+
                inc     addr + 1
:
.endmacro

.macro  DEC16_BARE      addr
                lda     addr
                bne     :+
                dec     addr+1
:
                dec     addr
.endmacro

.macro  INC16           addr
                inc     addr
                bne     :+
                inc     addr + 1
                bra     :++
:
                lda     addr + 1
                ora     #1
:
.endmacro

.macro  INC32           addr
                inc     addr
                bne     :+
                inc     addr+1
                bne     :+
                INC16   addr+2
.endmacro

.macro  DEC16           addr
                lda     addr
                bne     :+
                dec     addr+1
:
                dec     addr
                bne     :+
                lda     addr+1      ; LOB is zero, so use Z and N from HOB
                bra     :++
:
                lda     addr+1
                ora     #1          ; reset Z, if set, without affecting N
:
.endmacro

.macro  DEC32           addr
                lda     addr
                bne     :+++
                cmp     addr+1
                bne     :++
                cmp     addr+2
                bne     :+
                dec     addr+3
:
                dec     addr+2
:
                dec     addr+1
:
                dec     addr

.endmacro

.macro BNE16    addr1, addr2, dest
                lda     addr1
                cmp     addr2
                bne     dest
                lda     addr1 + 1
                cmp     addr2 + 1
                bne     dest
.endmacro

; No-clobber (NC) macros to wrap another macro that overwrites one or more registers
.macro  NC_A            MAC, p1, p2
                pha
                MAC     p1, p2
                pla
.endmacro

.macro  NC_X            MAC, p1, p2
                phx
                MAC     p1, p2
                plx
.endmacro

.macro  NC_Y            MAC, p1, p2
                phy
                MAC     p1, p2
                ply
.endmacro

.macro  NC_AX           MAC, p1, p2
                PUSH_AX
                MAC     p1, p2
                PULL_XA
.endmacro

.macro  NC_AY           MAC, p1, p2
                PUSH_AY
                MAC     p1, p2
                PULL_YA
.endmacro

.macro  NC_XY           MAC, p1, p2
                PUSH_XY
                MAC     p1, p2
                PULL_YX
.endmacro

.macro  NC_AXY          MAC, p1, p2
                PUSH_AXY
                MAC     p1, p2
                PULL_YXA
.endmacro

.macro  DEC16_NC_A      addr
                NC_A    DEC16, addr
.endmacro

.macro SWAP_AX
                pha
                txa
                plx
.endmacro

.macro SWAP_AY
                pha
                tya
                ply
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

.macro  PRINT_ANSI_ESC_SEQ_JMP   C1, C2, C3, C4, C5, C6, C7
                PRINT_CHAR_JMP  #ASCII_ESC, #ASCII_LBRACKET, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}
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

.macro  PRINT_HEX_JMP  C1, C2, C3, C4, C5, C6, C7, C8, C9
                META_JMP        WRITE_HEX, {C1}, {C2}, {C3}, {C4}, {C5}, {C6}, {C7}, {C8}, {C9}
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

.macro  STORE_LABEL_HL  label, addr
                STORE_LABEL_H {label}, {addr+1}
                STORE_LABEL_L {label}, {addr}
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

.macro _M_JSRR_NC_A             addrFrom, addrTo
                NC_A            _M_JSRR, {addrFrom}, {addrTo}
.endmacro

.macro SL_N     n
    .if     n > 0
                asl
                SL_N    n-1
    .endif
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

.macro SKIPNEXT2
                .byte   $DC     ; Undocumented 3-byte NOP, 4 cycles, reads absolute address IP+1, IP+2; uses 1 byte to skip two bytes
.endmacro

.macro BYTECOPY fm, to
                lda         fm
                sta         to
.endmacro

.macro WORDCOPY fm, to
                BYTECOPY    {fm + 1}, {to + 1}
                BYTECOPY    {fm}, {to}
.endmacro

.macro LOAD_ADDR    addr, to
                BYTECOPY    {#>addr}, {to + 1}
                BYTECOPY    {#<addr}, {to}
.endmacro

.macro MEMCP    addrFrom, addrTo, size
                PUSH_AY
                LOAD_ADDR   {addrFrom}, ZP_TEMP_VEC
                LOAD_ADDR   {addrTo}, ZP_TEMP_VEC2
                lda         #<size
                ldy         #>size
                jsr         MEM_COPY
                PULL_YA
.endmacro
