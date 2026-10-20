/*
** count.c - a test child (bankprog.h; t_bank's): how many times it's been called since it was loaded (its BSS's, kept
** while it stays loaded; loaded again, from 1), and x more
*/

static int n;

int __fastcall__ run (int x)
{
    return ++n + x;
}
