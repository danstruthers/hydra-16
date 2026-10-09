; ****************************************************************************
; w65c02.inc - the W65C02S's instructions, the one table: each opcode's name and addressing mode, each mode's length.
; The asm library's disassembler reads it as it is (dis.inc); the assembler makes its encoding table from it
; (modules/as/stmt.s's op_init: each name's opcode in each mode).  A name's index is its place in names (3 letters
; each); RMB, SMB, BBR and BBS have their bit after them, from the opcode (its bits 4-6).  The opcodes the W65C02S
; doesn't define are NOPs of 1, 2 or 3 bytes (M_N1, M_N2, M_N3: written as .byte).

; The modes (op_mode)
M_IMP           = 0             ; (none)
M_ACC           = 1             ; a
M_IMM           = 2             ; #$nn
M_ZP            = 3             ; $nn
M_ZPX           = 4             ; $nn,x
M_ZPY           = 5             ; $nn,y
M_IZX           = 6             ; ($nn,x)
M_IZY           = 7             ; ($nn),y
M_IZP           = 8             ; ($nn)
M_ABS           = 9             ; $nnnn
M_ABX           = 10            ; $nnnn,x
M_ABY           = 11            ; $nnnn,y
M_IND           = 12            ; ($nnnn)
M_IAX           = 13            ; ($nnnn,x)
M_REL           = 14            ; a branch: where it goes
M_ZPR           = 15            ; $nn, where it goes (BBR, BBS)
MODES           = 16            ; (The instructions' modes; from here, the undefined opcodes' NOPs ...)
M_N1            = 16            ;   of 1 byte ...
M_N2            = 17            ;   2 ...
M_N3            = 18            ;   and 3
NAMES           = 71            ; The names (names: 3 letters each)

N_RMB           = 5             ; (The names' indexes of the instructions with a bit in their names)
N_BBR           = 7
N_SMB           = 40
N_BBS           = 43
N_NONE          = 2             ; (???: an undefined opcode's)

.pushseg
.rodata

; Each mode's length (the opcode's and its operand's)
modelen:    .byte       1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 2, 3, 1, 2, 3

; The names, 3 letters each
names:      .byte       "brkora???tsbaslrmbphpbbrbpltrbclcincjsrandbitrol"
            .byte       "plpbmisecdecrtieorlsrphajmpbvccliphyrtsadcstzror"
            .byte       "plabvsseiplybrastastystxsmbdeytxabbsbcctyatxsldy"
            .byte       "ldaldxtaytaxbcsclvtsxcpycmpinydexwaibnecldphxstp"
            .byte       "cpxsbcinxnopbeqsedplx"
.assert     * - names = NAMES * 3, error, "NAMES: the names' count"

; Each opcode's name (its index in names) and mode (M_*)
op_name:
            .byte        0,  1,  2,  2,  3,  1,  4,  5,  6,  1,  4,  2,  3,  1,  4,  7
            .byte        8,  1,  1,  2,  9,  1,  4,  5, 10,  1, 11,  2,  9,  1,  4,  7
            .byte       12, 13,  2,  2, 14, 13, 15,  5, 16, 13, 15,  2, 14, 13, 15,  7
            .byte       17, 13, 13,  2, 14, 13, 15,  5, 18, 13, 19,  2, 14, 13, 15,  7
            .byte       20, 21,  2,  2,  2, 21, 22,  5, 23, 21, 22,  2, 24, 21, 22,  7
            .byte       25, 21, 21,  2,  2, 21, 22,  5, 26, 21, 27,  2,  2, 21, 22,  7
            .byte       28, 29,  2,  2, 30, 29, 31,  5, 32, 29, 31,  2, 24, 29, 31,  7
            .byte       33, 29, 29,  2, 30, 29, 31,  5, 34, 29, 35,  2, 24, 29, 31,  7
            .byte       36, 37,  2,  2, 38, 37, 39, 40, 41, 14, 42,  2, 38, 37, 39, 43
            .byte       44, 37, 37,  2, 38, 37, 39, 40, 45, 37, 46,  2, 30, 37, 30, 43
            .byte       47, 48, 49,  2, 47, 48, 49, 40, 50, 48, 51,  2, 47, 48, 49, 43
            .byte       52, 48, 48,  2, 47, 48, 49, 40, 53, 48, 54,  2, 47, 48, 49, 43
            .byte       55, 56,  2,  2, 55, 56, 19, 40, 57, 56, 58, 59, 55, 56, 19, 43
            .byte       60, 56, 56,  2,  2, 56, 19, 40, 61, 56, 62, 63,  2, 56, 19, 43
            .byte       64, 65,  2,  2, 64, 65, 11, 40, 66, 65, 67,  2, 64, 65, 11, 43
            .byte       68, 65, 65,  2,  2, 65, 11, 40, 69, 65, 70,  2,  2, 65, 11, 43
op_mode:
            .byte        0,  6, 17, 16,  3,  3,  3,  3,  0,  2,  1, 16,  9,  9,  9, 15
            .byte       14,  7,  8, 16,  3,  4,  4,  3,  0, 11,  1, 16,  9, 10, 10, 15
            .byte        9,  6, 17, 16,  3,  3,  3,  3,  0,  2,  1, 16,  9,  9,  9, 15
            .byte       14,  7,  8, 16,  4,  4,  4,  3,  0, 11,  1, 16, 10, 10, 10, 15
            .byte        0,  6, 17, 16, 17,  3,  3,  3,  0,  2,  1, 16,  9,  9,  9, 15
            .byte       14,  7,  8, 16, 17,  4,  4,  3,  0, 11,  0, 16, 18, 10, 10, 15
            .byte        0,  6, 17, 16,  3,  3,  3,  3,  0,  2,  1, 16, 12,  9,  9, 15
            .byte       14,  7,  8, 16,  4,  4,  4,  3,  0, 11,  0, 16, 13, 10, 10, 15
            .byte       14,  6, 17, 16,  3,  3,  3,  3,  0,  2,  0, 16,  9,  9,  9, 15
            .byte       14,  7,  8, 16,  4,  4,  5,  3,  0, 11,  0, 16,  9, 10, 10, 15
            .byte        2,  6,  2, 16,  3,  3,  3,  3,  0,  2,  0, 16,  9,  9,  9, 15
            .byte       14,  7,  8, 16,  4,  4,  5,  3,  0, 11,  0, 16, 10, 10, 11, 15
            .byte        2,  6, 17, 16,  3,  3,  3,  3,  0,  2,  0,  0,  9,  9,  9, 15
            .byte       14,  7,  8, 16, 17,  4,  4,  3,  0, 11,  0,  0, 18, 10, 10, 15
            .byte        2,  6, 17, 16,  3,  3,  3,  3,  0,  2,  0, 16,  9,  9,  9, 15
            .byte       14,  7,  8, 16, 17,  4,  4,  3,  0, 11,  0, 16, 18, 10, 10, 15

.popseg
