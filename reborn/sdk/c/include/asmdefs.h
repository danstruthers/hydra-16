/*
** asmdefs.h - the asm library's constants, for C (cc65).  Made by tools/apigen.js from spec/asm.def:
** don't edit.  asm.h includes it.
*/

#ifndef _ASMDEFS_H
#define _ASMDEFS_H

#define ASM_RAM                 0x5000    /* The assembler's state: the caller's RAM it takes while it runs, from here ... */
#define ASM_RAM_END             0x7E00    /*   to here (not this) */
#define AF_RAW                  0x01      /* FILE's flags: the bytes alone from .org's address (as -b), not a RAM program */
#define AF_LABELS               0x02      /*   a labels file too, the output's name and .lbl (as -l: ld65's -Ln form) */
#define AF_QUIET                0x04      /* BEGIN's flags: messages kept for ERROR, not said */
#define DIS_MAX                 64        /* The room for DIS's text: an instruction as as writes it, its 0 too */
#define DF_PAD                  0x01      /* DIS's flags: its name padded to 12 columns (the SDK's sources' way), else one space */
#define DK_ADDR                 0x01      /* DIS's kinds: its operand is an address (r4: the zero page's or not), read or written */
#define DK_GO                   0x02      /*   it goes to the address r4 (a jump, jsr, a branch: bbr's and bbs's too) */
#define DK_CALL                 0x04      /*   a jsr (DK_GO too) */
#define DK_END                  0x08      /*   nothing after it is reached by going on from it (brk, rti, rts, stp, jmp, bra) */
#define DK_UNDEF                0x10      /*   an opcode the W65C02S doesn't define (the NOP it runs: .byte and its bytes) */

#endif
