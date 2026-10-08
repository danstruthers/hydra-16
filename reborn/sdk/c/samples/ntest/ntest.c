/*
** ntest.c - num.h's test (tests/tests.js's c runs it at rc): the number libraries from C (arithmetic, conversions,
** text in bases, bits, the math library's functions, errors), num_format, and printf's and scanf's numbers (%N,
** %{base}).  Each check prints "ok - " or "not ok - " and its name; the last line is "ntest: N failed", and its exit
** status is the count.
*/

#include <stdio.h>
#include <string.h>
#include <num.h>

#define ROOM    200

static int failed;
static num_t a[ROOM], b[ROOM], c[ROOM], d[ROOM];
static char s[400];

static void check (int good, const char* what)
{
    printf ("%s - %s", good ? "ok" : "not ok", what);
    if (!good) {
        printf (" (num_error %d, s \"%s\")", num_error, s);
        ++failed;
    }
    printf ("\n");
}

/* n made from its text, in the base */
static void make (num_t* n, const char* text)
{
    if (num_parse (n, ROOM, text, NULL, NULL) < 0) {
        printf ("ntest: %s: %s\n", text, num_strerror (num_error));
    }
}

/* Is the number at n (a result's length len) written so in the base? */
static int is (int len, const num_t* n, const char* text)
{
    s[0] = '\0';
    return len > 0 && num_display (s, sizeof s, n, NULL) >= 0 && strcmp (s, text) == 0;
}

int main (void)
{
    long v;
    int i, j;

    check (num_init () == 0, "num_init: the libraries");

    /* ---- Arithmetic, exactly */
    make (a, "2/3");
    make (b, "0.5");
    check (is (num_add (c, ROOM, a, b), c, "7/6"), "num_add: 2/3 + 0.5 is 7/6");
    check (is (num_sub (c, ROOM, a, b), c, "1/6"), "num_sub");
    check (is (num_mul (c, ROOM, a, b), c, "1/3"), "num_mul");
    check (is (num_div (c, ROOM, a, b), c, "4/3"), "num_div");
    check (is (num_neg (c, ROOM, a), c, "-2/3") && is (num_abs (d, ROOM, c), d, "2/3"), "num_neg, num_abs");
    check (is (num_add (a, ROOM, a, a), a, "4/3"), "a result over an operand");
    make (a, "0.1");
    make (b, "0.2");
    check (is (num_add (c, ROOM, a, b), c, "0.3"), "0.1 + 0.2 is 0.3");
    make (a, "2");
    make (b, "100");
    check (is (num_pow (c, ROOM, a, b), c, "1267650600228229401496703205376"), "num_pow: 2^100");
    make (a, "4");
    make (b, "1/2");
    check (is (num_pow (c, ROOM, a, b), c, "2"), "num_pow: 4^1/2 (the math library's)");
    make (a, "-17");
    make (b, "5");
    check (is (num_idiv (c, ROOM, d, ROOM, a, b), c, "-3") && is (1, d, "-2"), "num_idiv: -17 by 5");
    make (a, "12");
    make (b, "18");
    check (is (num_gcd (c, ROOM, a, b), c, "6"), "num_gcd");
    make (a, "1");
    make (b, "1.0");
    check (num_cmp (a, b) == 0, "num_cmp: 1 and 1.0 equal");
    make (a, "1/3");
    make (b, "0.333");
    check (num_cmp (a, b) == 1 && num_cmp (b, a) == -1, "num_cmp: 1/3 above 0.333");
    check (num_kind (a) == NK_RATIONAL && num_kind (b) == NK_FIXED && num_sign (a) == 1, "num_kind, num_sign");
    check (num_size (a) == 3 && num_size (b) == 4, "num_size");

    /* ---- Conversions */
    make (a, "-2.5");
    check (is (num_truncate (c, ROOM, a), c, "-2") && is (num_floor (c, ROOM, a), c, "-3")
           && is (num_round (c, ROOM, a), c, "-2"), "num_truncate, num_floor, num_round");
    make (a, "1/3");
    check (is (num_to_fixed (c, ROOM, a, 5), c, "0.33333"), "num_to_fixed");
    make (a, "0.25");
    check (is (num_to_rational (c, ROOM, a), c, "1/4"), "num_to_rational");
    make (a, "6/4");
    check (is (num_numerator (c, ROOM, a), c, "3") && is (num_denominator (c, ROOM, a), c, "2"),
           "num_numerator, num_denominator");
    make (a, "1");
    make (b, "-2");
    check (is (num_complex (c, ROOM, a, b), c, "1-2i") && is (num_part (d, ROOM, c, 1), d, "-2"),
           "num_complex, num_part");
    check (is (num_from_int (c, ROOM, -100000L), c, "-100000")
           && is (num_from_uint (d, ROOM, 4000000000UL), d, "4000000000"), "num_from_int, num_from_uint");
    check (num_to_int (c, &v) == 0 && v == -100000L && num_to_int (d, &v) == 1
           && (unsigned long) v == 4000000000UL, "num_to_int");
    check (is (num_check (c, ROOM, (const unsigned char*) "\x80\x64", 2), c, "100")
           && num_check (c, ROOM, (const unsigned char*) "\x80\x05", 2) < 0 && num_error == NE_NOTNUM,
           "num_check: a number's one form");

    /* ---- Bits, random numbers, Fibonacci */
    make (a, "12");
    make (b, "10");
    check (is (num_bits (c, ROOM, a, b, NBIT_AND), c, "8") && is (num_bits (c, ROOM, a, b, NBIT_XOR), c, "6"),
           "num_bits: and, xor");
    make (a, "1");
    make (b, "100");
    check (is (num_bits (c, ROOM, a, b, NBIT_SHL), c, "1267650600228229401496703205376"), "num_bits: shift");
    make (a, "8");
    make (b, "3");
    check (num_bits (c, ROOM, a, b, NBIT_TEST) == 1, "num_bits: a bit");
    num_seed (1);
    make (a, "10");
    check (num_random (c, ROOM, a) > 0 && num_kind (c) == NK_INT && num_to_int (c, &v) == 0 && v >= 0 && v < 10,
           "num_random");
    make (a, "100");
    check (is (num_fib (c, ROOM, a), c, "354224848179261915075"), "num_fib");

    /* ---- The math library's functions */
    check (num_digits (0) == 12, "num_digits: 12 at the start");
    make (a, "2");
    check (is (num_sqrt (c, ROOM, a), c, "1.41421356237"), "num_sqrt 2");
    make (a, "9/4");
    check (is (num_sqrt (c, ROOM, a), c, "3/2"), "num_sqrt 9/4, exactly");
    make (a, "-1");
    check (is (num_sqrt (c, ROOM, a), c, "1i"), "num_sqrt -1");
    check (is (num_pi (c, ROOM), c, "3.14159265359"), "num_pi");
    make (a, "0");
    check (is (num_exp (c, ROOM, a), c, "1") && is (num_sin (c, ROOM, a), c, "0") && is (num_cos (c, ROOM, a), c, "1"),
           "num_exp, num_sin, num_cos");
    make (a, "1");
    check (is (num_log (c, ROOM, a), c, "0") && is (num_atan (c, ROOM, a), c, "0.785398163397"), "num_log, num_atan");
    check (num_digits (20) == 12, "num_digits 20");
    make (a, "2");
    check (is (num_sqrt (c, ROOM, a), c, "1.4142135623730950488"), "num_sqrt 2 at 20 digits");
    num_digits (12);

    /* ---- Text: bases, reading */
    make (a, "255");
    check (num_display (s, sizeof s, a, "#b") == 10 && strcmp (s, "#b11111111") == 0, "num_display in #b");
    make (b, "1/3");
    check (num_display (s, sizeof s, b, "b") > 0 && strcmp (s, "1/11") == 0, "a fraction in b");
    make (b, "0.5");
    check (num_display (s, sizeof s, b, "b") > 0 && strcmp (s, "0.1") == 0, "a radix point in b");
    check (num_set_base ("x") >= 0 && is (1, a, "FF") && num_get_base (s, sizeof s) == 1 && strcmp (s, "x") == 0,
           "num_set_base, num_get_base");
    make (c, "1F");
    check (num_set_base ("d") == 1 && is (1, c, "31"), "a number read in the base");
    check (num_set_base ("w") < 0 && num_error == NE_BASE, "not a base");
    {
        char* end;
        check (is (num_parse (c, ROOM, "12abc", NULL, &end), c, "12") && strcmp (end, "abc") == 0, "num_parse: a start");
        check (is (num_parse (c, ROOM, " 1+2i ", NULL, NULL), c, "1+2i") && is (num_parse (c, ROOM, "#xFF", NULL, NULL), c, "255"),
               "num_parse: 1+2i, #xFF");
    }
    check (num_parse (c, ROOM, "12abc", NULL, NULL) < 0 && num_error == NE_NOTNUM, "num_parse: the whole text");
    check (num_parse (c, ROOM, "1/0", NULL, NULL) < 0 && num_error == NE_DIV0
           && strcmp (num_strerror (num_error), "division by zero") == 0, "num_parse 1/0, num_strerror");
    make (a, "0");
    make (b, "1");
    check (num_div (c, ROOM, b, a) < 0 && num_error == NE_DIV0, "num_div by 0");
    make (a, "1267650600228229401496703205376");
    check (num_mul (c, 4, a, a) < 0 && num_error == NE_ROOM, "no room");
    check (num_display (s, 4, a, NULL) < 0 && num_error == NE_ROOM && s[0] == '\0', "num_display: no room");
    make (a, "255");
    make (b, "1/3");
    check (num_format (s, sizeof s, "{} is {x}, {{{#b}}}; {}", a, a, a, b) > 0
           && strcmp (s, "255 is FF, {#b11111111}; 1/3") == 0, "num_format");

    /* ---- printf: %N, %{base} */
    make (a, "2/3");
    make (b, "255");
    sprintf (s, "%N|%8N|%-8N|%+N|%{x}N|%{#x}N", a, a, a, a, b, b);
    check (strcmp (s, "2/3|     2/3|2/3     |+2/3|FF|#xFF") == 0, "printf: %N, its width, -, +, %{x}N");
    sprintf (s, "%{x}d %{#b}d %{c}d %{x}lu %{*}N %{}N", 255, 5, 5, 4000000000UL, "#o", b, b);
    check (strcmp (s, "FF #b101 --+ EE6B2800 #o377 255") == 0, "printf: %{base}d, ld, lu, %{*}N");
    sprintf (s, "%d %x %5s %05d %{x}05d", -5, 255, "ab", 42, 255);
    check (strcmp (s, "-5 ff    ab 00042 000FF") == 0, "printf: C's own conversions");
    sprintf (s, "[%{w}d]", 5);
    check (strcmp (s, "[]") == 0, "printf: not a base");
    make (c, "2");
    make (d, "1000");
    num_pow (a, ROOM, c, d);
    check (sprintf (s, "%N", a) == 302 && strlen (s) == 302 && memcmp (s, "10715086071862673209", 20) == 0
           && strcmp (s + 290, "205668069376") == 0, "printf: 2^1000 (302 digits)");

    /* ---- scanf: %N, %{base} */
    check (sscanf ("2/3 #xFF", "%N %N", a, ROOM, b, ROOM) == 2 && is (1, a, "2/3") && is (1, b, "255"), "scanf: %N");
    check (sscanf ("FF,101", "%{x}d,%{b}d", &i, &j) == 2 && i == 255 && j == 5, "scanf: %{x}d, a word to the comma");
    check (sscanf ("1/2 3", "%*N %N", a, ROOM) == 1 && is (1, a, "3"), "scanf: %*N");
    check (sscanf ("x", "%N", a, ROOM) == 0 && sscanf ("1.5", "%{d}d", &i) == 0, "scanf: not a number, not an integer");
    check (sscanf ("12 34", "%d %{*}d", &i, "o", &j) == 2 && i == 12 && j == 28, "scanf: %{*}d");

    printf ("ntest: %d failed\n", failed);
    return failed;
}
