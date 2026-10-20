/*
** bankrun.c - a bank program (a child: sdk/c/include/bankprog.h, sdk/c/child.cfg) run from the shell, to try one:
**   bankrun [-n N] CHILD [ARG ...]
** CHILD (its .bp file) loaded into a RAM bank of bankrun's own and its run (int argc, char** argv) called, its
** arguments CHILD's (argv[0] its file); its value the status.  -n N: called N times (still loaded: the calls' time
** told, in ticks).  A child uses bankrun's own (its exports, /rom/lib/bank/bankrun.exp, the build's): cc65's runtime, every
** helper (lent.s), printf and the C library bankrun links (lent's table below), and 32 bytes of zero page from $60.
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <bankprog.h>

/* What a child may use of the C library: linked whether bankrun uses it or not (so in its exports) */
const void* const lent[] = {
    printf, sprintf, puts, putchar, fputs, fopen, fclose, fgets, fread, fwrite, open, read, write, close,
    strlen, strcpy, strncpy, strcat, strcmp, strncmp, strchr, strrchr, strstr, memcpy, memmove, memset, memcmp,
    atoi, itoa, utoa, malloc, free, hy_ticks, hy_time
};

static const char* const why[] = {
    "", "not set up", "no such file, or not read whole", "not a bank program", "its zero page too much",
    "no exports' file", "uses what bankrun hasn't: ", "no bank free", "no run in it"
};

int main (int argc, char* argv[])
{
    struct bank_fn* h;
    unsigned n = 1, k;
    unsigned long t;
    int i = 1, r = 0;

    if (argc > 2 && !strcmp (argv[1], "-n")) {
        n = atoi (argv[2]);
        i = 3;
    }
    if (i >= argc) {
        fputs ("usage: bankrun [-n N] CHILD [ARG ...]\n", stderr);
        return 1;
    }
    bank_setup ("/rom/lib/bank/bankrun.exp", 0x60, 32);
    if (!(h = bank_fn (argv[i], "run"))) {
        printf ("bankrun: %s: %s%s\n", argv[i], why[bank_err < sizeof why / sizeof why[0] ? bank_err : 0],
            bank_err == BE_IMPORT ? bank_missing : "");
        return 1;
    }
    t = hy_ticks ();
    for (k = 0; k < n; ++k) {
        r = bank_call (h, int (*)(int, char**), argc - i, argv + i);
    }
    if (n > 1) {
        printf ("%u calls: %lu ticks\n", n, hy_ticks () - t);
    }
    return r;
}
