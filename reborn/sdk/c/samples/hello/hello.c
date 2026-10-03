/*
** hello.c - the C SDK's first sample: hello, its arguments, and an exit status.
**   % hello world
**   hello from C, world
*/

#include <stdio.h>

int main (int argc, char* argv[])
{
    int i;

    printf ("hello from C");
    for (i = 1; i < argc; ++i) {
        printf ("%s %s", i == 1 ? "," : "", argv[i]);
    }
    printf ("\n");
    return 0;
}
