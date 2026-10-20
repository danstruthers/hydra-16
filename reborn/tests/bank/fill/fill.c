/*
** fill.c - a test child (bankprog.h; t_bank's): words put in its caller's RAM (with its caller's strcpy), and a sum
** of three arguments (two on the C stack, one in .A/.X)
*/

#include <string.h>

void __fastcall__ fill (char* b)
{
    strcpy (b, "from the child");
}

int __fastcall__ sum3 (int a, int b, int c)
{
    return a + b + c;
}
