/*
** hello.c - a first C program for the Hydra-16: its arguments, some arithmetic, the heap and the clock.
**     0:/> hello one "two three"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <hydra.h>

int main (int argc, char* argv[])
{
    int i;
    long sum = 0;
    char* copy;
    time_t now;
    char when[32];

    printf ("Hello from C on the Hydra-16!\n");
    printf ("%d arguments:", argc - 1);
    for (i = 1; i < argc; ++i) {
        printf (" [%s]", argv[i]);
    }
    printf ("\n");

    for (i = 1; i <= 1000; ++i) {
        sum += (long) i * i;
    }
    printf ("1^2 + ... + 1000^2 = %ld\n", sum);

    copy = strdup ("a copy on the heap");
    printf ("%s (%u bytes free)\n", copy, _heapmemavail ());
    free (copy);

    now = time (0);
    strftime (when, sizeof when, "%Y-%m-%d %H:%M:%S", localtime (&now));
    printf ("The clock says %s\n", when);
    return 0;
}
