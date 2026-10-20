/*
** hello.c - a bank program (sdk/c/include/bankprog.h), to try one from the shell with bankrun: its arguments said
** back ("bankrun /rom/lib/bank/hello.bp one two"); its value how many there were.  Its printf bankrun's.
*/

#include <stdio.h>

int __fastcall__ run (int argc, char** argv)
{
    int i;

    printf ("hello from a bank program (%s)", argv[0]);
    for (i = 1; i < argc; ++i) {
        printf (" %s", argv[i]);
    }
    putchar ('\n');
    return argc - 1;
}
