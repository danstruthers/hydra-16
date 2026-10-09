/*
** numdefs.h - the number libraries' constants, for C (cc65).  Made by tools/apigen.js from spec/numbers.def:
** don't edit.  num.h includes it.
*/

#ifndef _NUMDEFS_H
#define _NUMDEFS_H

#define NUM_MAX                 1040      /* The longest number in the stored format (a complex number of two rationals of 255-byte integers: 1039) */
#define NT_POS                  0x80      /* The format's tags: $00-$7F an integer -64 to 63; $80 + n - 1, a positive integer of n bytes (1-16) */
#define NT_NEG                  0x90      /* A negative integer of 1-16 bytes ($90 + n - 1) */
#define NT_LONG_POS             0xA0      /* A positive integer of 17-255 bytes: a length byte, then its magnitude */
#define NT_LONG_NEG             0xA1      /* A negative one */
#define NT_FIXED                0xB0      /* A fixed decimal of 0-15 places ($B0 + places), then its digits: an integer */
#define NT_FIXED_LONG           0xC0      /* A fixed decimal of 16-65535 places: the places (2 bytes), then its digits */
#define NT_RATIONAL             0xC1      /* A rational: its numerator, then its denominator (above 1), in lowest terms */
#define NT_COMPLEX              0xC2      /* A complex number: its real part, then its imaginary part (not 0) */
#define NT_NONE                 0xFF      /* Never a number: a program's own */
#define NE_BIG                  1         /* Too big: an integer past 255 bytes, a fixed decimal past 65535 places */
#define NE_DIV0                 2         /* Division by zero */
#define NE_NOTNUM               3         /* Not a number: text that isn't one, or bytes that aren't a number in the stored format */
#define NE_ROOM                 4         /* No room for the result in the place given */
#define NE_DOMAIN               5         /* Outside the function's domain */
#define NE_BASE                 6         /* Not a base: a base string that names none */
#define NE_INT                  7         /* An integer is needed (a number equal to one will do) */
#define NE_REAL                 8         /* A real number is needed, not a complex one */
#define NE_INIT                 9         /* The bank given (r13) isn't one INIT filled */
#define NE_FORMAT               10        /* A format's placeholders and its arguments don't match */
#define NK_INT                  0         /* A kind: an integer */
#define NK_FIXED                1         /* A fixed decimal */
#define NK_RATIONAL             2         /* A rational */
#define NK_COMPLEX              3         /* A complex number */
#define NPARSE_PROGRAM          1         /* PARSE's .Y: the text is a program's (a bare number starts with a digit 0-9) */
#define NPARSE_WHOLE            2         /* PARSE's .Y: the whole text must be the number */
#define NFMT_NUMBER             0         /* FORMAT's arguments: a number's entry (its address after) */
#define NFMT_STRING             1         /* A string's (zero-terminated) */
#define NFMT_END                0xFF      /* The table's end */
#define NBIT_AND                0         /* BITS's operations (.Y): and */
#define NBIT_OR                 1         /* Or */
#define NBIT_XOR                2         /* Xor */
#define NBIT_NOT                3         /* Not (x alone: -x - 1) */
#define NBIT_SHL                4         /* Shift left by y */
#define NBIT_SHR                5         /* Shift right by y (toward minus infinity) */
#define NBIT_TEST               6         /* Bit y's test (.A = 0 or 1, no result) */

#endif
