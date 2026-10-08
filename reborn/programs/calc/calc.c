/*
** calc.c - calc [-b base] [-d digits] [expression ...]: the Hydra's numbers at rc (num.h; the number libraries):
** an expression worked out exactly, and its value written in decimal, or in the base -b names (hylang's base
** strings: x, #x, b, o, c, 16r ...).  The expression is its arguments (rc's words, put together), or, with none,
** each line of its input in turn.
**   % calc 2/3 + 0.5
**   7/6
**   % calc sqrt 2
**   1.41421356237
**   % calc -b x 255
**   FF
** Numbers are read in decimal (42, 0.5, 2i, 1_000), and in any base by their own prefix (#xFF, #b0.1, #c+-0).
** Operators: + - * / (exactly: 1/3 is a third), % (an integer's remainder), ^ (a power: exact when it's whole),
** unary - and +, parentheses.  Functions, a word with its argument after it (sqrt 2, sqrt(2); sqrt 2^2 is sqrt 4,
** sqrt(4)^2 is 4): sqrt exp log sin cos tan atan (the math library's, to -d's digits: 12 without it), abs floor
** round truncate fib numerator denominator re im rational random; with two arguments in parentheses, gcd(a, b)
** pow(a, b) complex(a, b) fixed(a, places).  Constants: pi, e, i.  rc's own characters need quotes: calc '2^10',
** calc '(1+2)*3' ('#' starts a comment).  Its status: none; or the last error's text ("division by zero"), "usage".
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <hydra.h>
#include <num.h>

#define POOL        12000       /* The numbers an expression makes, each kept till it's done */
#define LINE        256         /* An expression: 255 characters at most */
#define NAME_MAX    12

static num_t pool[POOL];
static unsigned used;
static num_t quot[NUM_MAX];     /* (%'s quotient) */
static const char* p;           /* The expression, as it's read */
static const char* base;        /* -b's (NULL: decimal) */
static jmp_buf fail;
static const char* status;      /* The last error's text */
static char why[48];

/* The expression's error: said, and on to the next */
static void error (const char* what)
{
    fprintf (stderr, "calc: %s\n", what);
    status = what;
    longjmp (fail, 1);
}

static void usage (void)
{
    fputs ("usage: calc [-b base] [-d digits] [expression ...]\n", stderr);
    hy_exits ("usage");
}

/* Where a function's result goes, and its room: the pool's end.  (A function's operands are worked out before it's
** called, never in its arguments: they move the pool's end) */
#define TOP         (pool + used), (POOL - used)

/* The number a function made at the pool's end (its length n, or -1): kept there */
static num_t* keep (int n)
{
    num_t* r = pool + used;

    if (n < 0) {
        error (num_error == NE_ROOM ? "too long" : num_strerror (num_error));
    }
    used += n;
    return r;
}

/* A number from text, read in decimal (a # base of its own as it says): the whole text; or, end, its longest start
** that's one, *end after it */
static num_t* read (const char* text, char** end)
{
    int n = num_parse (TOP, text, "d", end);

    if (n < 0 && num_error == NE_NOTNUM) {
        sprintf (why, "%.24s: not a number", text);
        error (why);
    }
    return keep (n);
}

static void space (void)
{
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
}

static void expect (char c)
{
    space ();
    if (*p != c) {
        sprintf (why, "%c expected", c);
        error (why);
    }
    ++p;
}

static num_t* expr (void);
static num_t* factor (void);

/* A number: a word of letters, digits, . and _ (in decimal); or one with a # base of its own, as far as it goes */
static num_t* number (void)
{
    static char word[LINE];
    const char* q = p;
    char* end;
    unsigned char n = 0;
    num_t* a;

    if (*p == '#') {
        while (*q && *q != ' ' && *q != '\t' && strchr ("(),*^%", *q) == NULL) {
            word[n++] = *q++;
        }
        word[n] = '\0';
        a = read (word, &end);
        p += end - word;
        return a;
    }
    while (isalnum (*q) || *q == '.' || *q == '_') {
        word[n++] = *q++;
    }
    word[n] = '\0';
    p = q;
    return read (word, NULL);
}

/* An integer (or a number equal to one) as an unsigned */
static unsigned whole (const num_t* a)
{
    long v;

    if (num_to_int (a, &v) != 0 || v < 0 || v > 65535L) {
        error ("a whole number is needed");
    }
    return (unsigned) v;
}

/* (a, b): a function's two arguments (the second, second) */
static num_t* second;
static num_t* two (void)
{
    num_t* a;

    expect ('(');
    a = expr ();
    expect (',');
    second = expr ();
    expect (')');
    return a;
}

/* A name's value: a constant, or a function of what's after it */
static const char* const funcs[] = {
    "sqrt", "exp", "log", "sin", "cos", "tan", "atan", "abs", "floor", "round", "truncate", "fib",
    "numerator", "denominator", "re", "im", "rational", "random",
    "gcd", "pow", "complex", "fixed", "pi", "e", "i", NULL
};
enum {
    F_SQRT, F_EXP, F_LOG, F_SIN, F_COS, F_TAN, F_ATAN, F_ABS, F_FLOOR, F_ROUND, F_TRUNCATE, F_FIB,
    F_NUMERATOR, F_DENOMINATOR, F_RE, F_IM, F_RATIONAL, F_RANDOM,
    F_GCD, F_POW, F_COMPLEX, F_FIXED, F_PI, F_E, F_I
};

static num_t* call (const char* name)
{
    unsigned char f;
    unsigned places;
    num_t* a;

    for (f = 0; funcs[f] && strcmp (funcs[f], name); ++f) {
    }
    switch (f) {
        case F_PI:
            return keep (num_pi (TOP));
        case F_E:
            a = read ("1", NULL);
            return keep (num_exp (TOP, a));
        case F_I:
            return read ("1i", NULL);
        case F_GCD:
            a = two ();
            return keep (num_gcd (TOP, a, second));
        case F_POW:
            a = two ();
            return keep (num_pow (TOP, a, second));
        case F_COMPLEX:
            a = two ();
            return keep (num_complex (TOP, a, second));
        case F_FIXED:
            a = two ();
            places = whole (second);
            return keep (num_to_fixed (TOP, a, places));
    }
    if (funcs[f] == NULL) {
        sprintf (why, "%s: unknown", name);
        error (why);
    }
    /* One argument: in parentheses (a value then, as a number is), or a factor (sqrt 2^2 is sqrt 4) */
    space ();
    if (*p == '(') {
        ++p;
        a = expr ();
        expect (')');
    } else {
        a = factor ();
    }
    switch (f) {
        case F_SQRT:        return keep (num_sqrt (TOP, a));
        case F_EXP:         return keep (num_exp (TOP, a));
        case F_LOG:         return keep (num_log (TOP, a));
        case F_SIN:         return keep (num_sin (TOP, a));
        case F_COS:         return keep (num_cos (TOP, a));
        case F_TAN:         return keep (num_tan (TOP, a));
        case F_ATAN:        return keep (num_atan (TOP, a));
        case F_ABS:         return keep (num_abs (TOP, a));
        case F_FLOOR:       return keep (num_floor (TOP, a));
        case F_ROUND:       return keep (num_round (TOP, a));
        case F_TRUNCATE:    return keep (num_truncate (TOP, a));
        case F_FIB:         return keep (num_fib (TOP, a));
        case F_NUMERATOR:   return keep (num_numerator (TOP, a));
        case F_DENOMINATOR: return keep (num_denominator (TOP, a));
        case F_RE:          return keep (num_part (TOP, a, 0));
        case F_IM:          return keep (num_part (TOP, a, 1));
        case F_RATIONAL:    return keep (num_to_rational (TOP, a));
    }
    return keep (num_random (TOP, a));
}

/* A number, a name's value, or an expression in parentheses */
static num_t* primary (void)
{
    num_t* a;
    char name[NAME_MAX + 1];
    unsigned char n = 0;

    space ();
    if (*p == '(') {
        ++p;
        a = expr ();
        expect (')');
        return a;
    }
    if (isdigit (*p) || *p == '.' || *p == '#') {
        return number ();
    }
    if (islower (*p)) {
        while (isalnum (*p)) {
            if (n == NAME_MAX) {
                error ("a name too long");
            }
            name[n++] = *p++;
        }
        name[n] = '\0';
        return call (name);
    }
    if (*p == '\0') {
        error ("an operand is missing");
    }
    sprintf (why, "%c: unexpected", *p);
    error (why);
    return 0;
}

/* a ^ b (b a factor: 2^-1, and 2^3^2 is 2^9) */
static num_t* power (void)
{
    num_t* a = primary ();
    num_t* b;

    space ();
    if (*p == '^') {
        ++p;
        b = factor ();
        return keep (num_pow (TOP, a, b));
    }
    return a;
}

/* - and + before one: -2^2 is -4 */
static num_t* factor (void)
{
    num_t* a;

    space ();
    if (*p == '-') {
        ++p;
        a = factor ();
        return keep (num_neg (TOP, a));
    }
    if (*p == '+') {
        ++p;
        return factor ();
    }
    return power ();
}

static num_t* term (void)
{
    num_t* a = factor ();
    num_t* b;
    char c;

    for (;;) {
        space ();
        c = *p;
        if (c != '*' && c != '/' && c != '%') {
            return a;
        }
        ++p;
        b = factor ();
        if (c == '*') {
            a = keep (num_mul (TOP, a, b));
        } else if (c == '/') {
            a = keep (num_div (TOP, a, b));
        } else if (num_idiv (quot, sizeof quot, TOP, a, b) < 0) {
            keep (-1);
        } else {
            a = keep (num_size (pool + used));
        }
    }
}

static num_t* expr (void)
{
    num_t* a = term ();
    num_t* b;
    char c;

    for (;;) {
        space ();
        c = *p;
        if (c != '+' && c != '-') {
            return a;
        }
        ++p;
        b = term ();
        a = keep (c == '+' ? num_add (TOP, a, b) : num_sub (TOP, a, b));
    }
}

/* An expression worked out and its value written (an empty one: nothing) */
static void run (const char* text)
{
    num_t* v;

    used = 0;
    p = text;
    if (setjmp (fail)) {
        return;
    }
    space ();
    if (*p == '\0') {
        return;
    }
    v = expr ();
    space ();
    if (*p) {
        sprintf (why, "%.24s: unexpected", p);
        error (why);
    }
    printf ("%{*}N\n", base, v);
}

int main (int argc, char* argv[])
{
    static char line[LINE];
    static const num_t zero[1] = { 0 };
    int k = 1, d;
    unsigned n;

    while (k < argc && argv[k][0] == '-' && argv[k][1] && !argv[k][2]) {
        if (argv[k][1] == '-') {
            ++k;
            break;
        }
        if (argv[k][1] != 'b' && argv[k][1] != 'd') {
            break;                                          /* (-5: the expression's) */
        }
        if (k + 1 == argc) {
            usage ();
        }
        if (argv[k][1] == 'b') {
            base = argv[k + 1];
        } else {
            d = atoi (argv[k + 1]);
            if (d < 1 || d > 100) {
                fputs ("calc: -d: 1 to 100 digits\n", stderr);
                hy_exits ("usage");
            }
            if (num_digits (d) < 0) {
                fprintf (stderr, "calc: %s\n", num_strerror (num_error));
                hy_exits (num_strerror (num_error));
            }
        }
        k += 2;
    }
    if (num_init () < 0) {
        fputs ("calc: no number libraries\n", stderr);
        hy_exits ("no number libraries");
    }
    if (base && num_display (line, sizeof line, zero, base) < 0) {
        fprintf (stderr, "calc: %s: not a base\n", base);
        hy_exits ("not a base");
    }

    if (k < argc) {
        for (n = 0; k < argc; ++k) {
            if (n + strlen (argv[k]) + 2 > LINE) {
                fputs ("calc: too long\n", stderr);
                hy_exits ("too long");
            }
            if (n) {
                line[n++] = ' ';
            }
            strcpy (line + n, argv[k]);
            n += strlen (argv[k]);
        }
        run (line);
    } else {
        while (fgets (line, sizeof line, stdin)) {
            n = strlen (line);
            if (n && line[n - 1] == '\n') {
                line[n - 1] = '\0';
            }
            run (line);
        }
    }
    if (status) {
        hy_exits (status);
    }
    return 0;
}
