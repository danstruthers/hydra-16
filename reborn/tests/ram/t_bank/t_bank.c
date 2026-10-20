/*
** t_bank.c - bank programs (bankprog.h) tried, the test bankprog's: a child loaded and called, called again (still
** loaded: its count goes on), a call's cost; words it puts in the caller's RAM, three arguments; BANK_MAX more
** children loaded (the first given up for them: called again, loaded again, its count from 1); and refused, each
** with its reason: its zero page more than given, what it uses not the caller's, no such entry, no such file.  Its
** files in /ram (the test copies them there from /pc): the test children (tests/bank's .bp files: count's under six other names too) and its own exports,
** t_bank.exp.  An "ok" or "not ok" line each, then "t_bank: PASS" (or FAIL).
*/

#include <stdio.h>
#include <string.h>
#include <hydra.h>
#include <bankprog.h>

static unsigned char fails;
static char buf[32];
static const char* const more[BANK_MAX] = {
    "/ram/c1.bp", "/ram/c2.bp", "/ram/c3.bp", "/ram/c4.bp", "/ram/c5.bp", "/ram/c6.bp"
};

static void __fastcall__ ok (unsigned char c, const char* what)
{
    printf ("%s - %s\n", c ? "ok" : "not ok", what);
    fails += !c;
}

int main (void)
{
    struct bank_fn* h;
    struct bank_fn* f;
    unsigned char i;
    unsigned long t;
    int r;

    ok (bank_setup ("/ram/t_bank.exp", 0x60, 16) == 0, "bank_setup");
    h = bank_fn ("/ram/count.bp", "run");
    ok (h != 0, "a child loaded (count.bp's run)");
    r = h ? bank_call (h, int (*)(int), 10) : 0;
    ok (r == 11, "called: its first call");
    r = h ? bank_call (h, int (*)(int), 10) : 0;
    ok (r == 12, "called again: still loaded, its count on");
    t = hy_ticks ();
    for (i = 0; h && i < 200; ++i) {
        bank_call (h, int (*)(int), 0);
    }
    printf ("  200 calls: %lu ticks\n", hy_ticks () - t);
    strcpy (buf, "not yet");                    /* (And strcpy linked: the child's, from this program) */
    f = bank_fn ("/ram/fill.bp", "fill");
    if (f) {
        bank_call (f, void (*)(char*), buf);
    }
    ok (f && !strcmp (buf, "from the child"), "its words in the caller's RAM");
    f = bank_fn ("/ram/fill.bp", "sum3");
    ok (f && bank_call (f, int (*)(int, int, int), 1, 20, 300) == 321, "three arguments");
    for (i = 0; i < BANK_MAX; ++i) {            /* (The first, count.bp, the least lately called: given up) */
        ok (bank_fn (more[i], "run") != 0, more[i]);
    }
    r = h ? bank_call (h, int (*)(int), 0) : 0;
    ok (r == 1, "given up, then called: loaded again, its count from 1");
    f = bank_fn ("/ram/bigzp.bp", "run");
    printf ("  bank_err %u\n", bank_err);
    ok (!f && bank_err == BE_ZP, "too much zero page: refused");
    ok (!bank_fn ("/ram/nosuch.bp", "run") && bank_err == BE_IMPORT && !strcmp (bank_missing, "_nosuch"),
        "what the caller hasn't: refused, and named");
    ok (!bank_fn ("/ram/count.bp", "none") && bank_err == BE_ENTRY, "no such entry");
    ok (!bank_fn ("/ram/gone.bp", "run") && bank_err == BE_FILE, "no such file");
    bank_flush ();
    printf ("t_bank: %s\n", fails ? "FAIL" : "PASS");
    return fails;
}
