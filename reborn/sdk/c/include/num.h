/*
** num.h - the Hydra's numbers in C: the number libraries (modules of the paged ROM: numbers, the number system, and
** math, its functions; spec/numbers.def), which hylang, HyForth and BASIC use too.  A number is exact: an integer of
** any size (to 255 bytes), a fixed decimal (1.25), a rational (2/3) or a complex number (1+2i), kept in an array of
** bytes in the stored format (num_t: NUM_MAX bytes at most; a small integer is one byte, 1/3 three).
**   A function that makes a number takes its place and its room (dst, room: bytes) first, then its operands, and
** gives back the result's length; or -1 if it fails, num_error saying why (NE_BIG, NE_DIV0 ...: num_strerror's
** text).  A result may go over an operand (num_add (a, sizeof a, a, b)): they're read first.  Numbers are read and
** written in the base (num_set_base: decimal, "d", at the start), or one named: hylang's base strings, "x" (FF),
** "#x" (#xFF: the # says its prefix is written), "b", "o", "c" (balanced ternary), "16r", "<x" (least digit
** first), "[01]" (digits of its own) ...
**   The first call readies the libraries (num_init): it takes two RAM banks of the program's (the libraries' and
** printf's).  printf and scanf take numbers too: %N, and a base in braces before a conversion (%{x}N, %{#b}d, %{*}N:
** the base from the arguments); the C SDK's README says how.
*/

#ifndef _NUM_H
#define _NUM_H

#include <stdarg.h>
#include <numdefs.h>                    /* NUM_MAX, the errors (NE_), the kinds (NK_), BITS's operations (NBIT_) ... */

typedef unsigned char num_t;            /* A number: its bytes, in the stored format */

extern unsigned char num_error;         /* Why the last call that failed did (NE_*) */

int num_init (void);                                        /* The libraries readied (the first call does it):
                                                            **   0, or -1: NE_INIT (none, or no bank for them) */
const char* __fastcall__ num_strerror (unsigned char e);    /* An error's text: "division by zero" */
unsigned __fastcall__ num_size (const num_t* a);            /* The bytes a number takes (by its tags) */

/* ---- The base, and text.  A base string NULL: the base */

int __fastcall__ num_set_base (const char* base);           /* The base from now on.  0-3 (bit 0: decimal; bit 1:
                                                            **   its prefix written), or -1: NE_BASE */
int __fastcall__ num_get_base (char* dst, unsigned room);   /* The base's string (a 0 after it): its length */
int __fastcall__ num_parse (num_t* dst, unsigned room, const char* text, const char* base, char** end);
                                                            /* A number read from text: the longest start of it
                                                            **   that's one (spaces round it passed over), *end
                                                            **   after it; end NULL: the whole text must be one */
int __fastcall__ num_display (char* dst, unsigned room, const num_t* a, const char* base);
                                                            /* A number as text, exactly (1/3, 0.5, #b0.1, 1+2i),
                                                            **   a 0 after it: its length (room: the 0's too) */
int num_format (char* dst, unsigned room, const char* fmt, ...);
int __fastcall__ num_vformat (char* dst, unsigned room, const char* fmt, va_list ap);
                                                            /* hylang's format: "{} and {x}", a number (num_t*)
                                                            **   for each placeholder ({} in the base, {x} in x;
                                                            **   {{ and }} a brace), 16 at most */

/* ---- Arithmetic, by the tower's rules (complex if either is, else rational, else fixed, else an integer) */

int __fastcall__ num_add (num_t* dst, unsigned room, const num_t* a, const num_t* b);
int __fastcall__ num_sub (num_t* dst, unsigned room, const num_t* a, const num_t* b);
int __fastcall__ num_mul (num_t* dst, unsigned room, const num_t* a, const num_t* b);
int __fastcall__ num_div (num_t* dst, unsigned room, const num_t* a, const num_t* b);   /* Exactly: 2/3 */
int __fastcall__ num_neg (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_abs (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_pow (num_t* dst, unsigned room, const num_t* a, const num_t* b);
                                                            /* a to the b: exactly when b is whole, else (the math
                                                            **   library's) exactly if it can be, else to digits */
int __fastcall__ num_idiv (num_t* q, unsigned qroom, num_t* r, unsigned rroom, const num_t* a, const num_t* b);
                                                            /* Integers: the quotient (toward 0) at q, its length;
                                                            **   the remainder at r (num_size) */
int __fastcall__ num_gcd (num_t* dst, unsigned room, const num_t* a, const num_t* b);
int __fastcall__ num_cmp (const num_t* a, const num_t* b);  /* -1, 0 or 1 (a below, equal to, above b); 2 if it
                                                            **   fails */
int __fastcall__ num_kind (const num_t* a);                 /* NK_INT, NK_FIXED, NK_RATIONAL, NK_COMPLEX; or -1 */
int __fastcall__ num_sign (const num_t* a);                 /* -1, 0 or 1 (a complex number's real part's); 2 */

/* ---- Conversions */

int __fastcall__ num_truncate (num_t* dst, unsigned room, const num_t* a);      /* Toward 0 */
int __fastcall__ num_floor (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_round (num_t* dst, unsigned room, const num_t* a);         /* A half to the even one */
int __fastcall__ num_to_fixed (num_t* dst, unsigned room, const num_t* a, unsigned places);   /* Cut, not rounded */
int __fastcall__ num_to_rational (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_numerator (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_denominator (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_complex (num_t* dst, unsigned room, const num_t* re, const num_t* im);
int __fastcall__ num_part (num_t* dst, unsigned room, const num_t* a, unsigned char im);  /* im 0: its real part */
int __fastcall__ num_from_int (num_t* dst, unsigned room, long v);
int __fastcall__ num_from_uint (num_t* dst, unsigned room, unsigned long v);
int __fastcall__ num_to_int (const num_t* a, long* v);      /* An integer: *v its low 32 bits; 0 if it fits a long,
                                                            **   1 only an unsigned long, 2 neither; or -1 */
int __fastcall__ num_check (num_t* dst, unsigned room, const unsigned char* bytes, unsigned count);
                                                            /* Bytes that are a number in the stored format (its one
                                                            **   form), copied: its length; or -1, NE_NOTNUM */

/* ---- Bits (two's complement, any size), random numbers, Fibonacci */

int __fastcall__ num_bits (num_t* dst, unsigned room, const num_t* a, const num_t* b, unsigned char op);
                                                            /* op NBIT_AND, _OR, _XOR, _NOT (a alone), _SHL, _SHR
                                                            **   (by b); NBIT_TEST: bit b of a, 0 or 1 */
int __fastcall__ num_random (num_t* dst, unsigned room, const num_t* n);        /* 0 to n - 1; n 0: 0 up to 1 */
void __fastcall__ num_seed (unsigned seed);                 /* 0: from the clock */
int __fastcall__ num_fib (num_t* dst, unsigned room, const num_t* n);

/* ---- The math library's functions: exact when they can be (sqrt 9/4 is 3/2), else a fixed decimal of the
** precision's significant digits (12 at the start), correctly rounded */

int __fastcall__ num_digits (unsigned char digits);         /* The precision (1-100) from now on (0: as it is): the
                                                            **   one before */
int __fastcall__ num_sqrt (num_t* dst, unsigned room, const num_t* a);          /* Of a negative number, imaginary */
int __fastcall__ num_exp (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_log (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_sin (num_t* dst, unsigned room, const num_t* a);           /* In radians */
int __fastcall__ num_cos (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_tan (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_atan (num_t* dst, unsigned room, const num_t* a);
int __fastcall__ num_pi (num_t* dst, unsigned room);

#endif
