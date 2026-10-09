/*
** asm.h - the W65C02S's instructions in C: the asm library (a module of the paged ROM, spec/asm.def), which db, dis
** and HyForth's disasm share, writes an instruction as the assembler as writes it (ca65's language: lower case, a:
** where as would take the zero page form, .byte for an opcode the chip doesn't define), so the text assembled is the
** bytes again.  And symbol files: ld65's label files (ld65 -Ln, as -l: "al 000830 .main") and the system calls'
** names (hydra.inc's "WRITE = $F953" ...), whose symbols name addresses.
*/

#ifndef _ASM_H
#define _ASM_H

#include <asmdefs.h>                    /* DIS_MAX, DF_PAD, the kinds (DK_) */

struct dis {                            /* An instruction, disassembled: */
    unsigned                at;         /*   in: its address */
    const unsigned char*    bytes;      /*   in: its bytes (its opcode, and the two after it) */
    const char*             name;       /*   in: a name for the address its operand names (NULL: the address) */
    unsigned char           flags;      /*   in: DF_PAD (its name padded to 12 columns) */
    unsigned char           len;        /*   out: its length (1-3) */
    unsigned char           kind;       /*   out: DK_ADDR, DK_GO, DK_CALL, DK_END, DK_UNDEF */
    unsigned                addr;       /*   out: the address its operand names (a branch's: where it goes) */
    char                    text[DIS_MAX];  /* out: as as writes it */
};

unsigned char __fastcall__ dis_insn (struct dis* d);        /* d's instruction: its length, or 0 if there's no asm
                                                            **   library (the first call finds it) */

/* ---- Symbols */

int __fastcall__ lbl_load (const char* file);               /* A file's symbols added: an ld65 label file's ("al
                                                            **   000830 .main": but cheap locals and the linker's
                                                            **   __NAME__), or a .inc's system calls ("WRITE =
                                                            **   $F953": $F800 up) and r0-r15.  How many, or -1 */
const char* __fastcall__ lbl_at (unsigned addr);            /* The symbol at addr, or NULL */
const char* __fastcall__ lbl_label (unsigned addr);         /* The symbol at addr whose name no symbol elsewhere has
                                                            **   (a label a source can define), or NULL */
const char* __fastcall__ lbl_near (unsigned addr, unsigned within);
                                                            /* "name" or "name+1C": the nearest at or below addr,
                                                            **   less than within past it, or NULL (a buffer of its
                                                            **   own, till the next call) */
int __fastcall__ lbl_addr (const char* name, unsigned* addr);   /* A symbol's address: 0, or -1 */
unsigned lbl_count (void);                                  /* The symbols */

#endif
