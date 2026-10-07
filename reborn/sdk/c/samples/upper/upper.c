/*
** upper.c - a filter: its input (stdin: the console, a file with <, or a pipe) to its output in capitals, a line
** at a time, to the end of its input (Ctrl-D at the console).
**   % echo hello | upper
**   HELLO
*/

#include <stdio.h>
#include <ctype.h>

int main (void)
{
    char line[128];
    char* p;

    while (fgets (line, sizeof line, stdin) != 0) {
        for (p = line; *p; ++p) {
            *p = toupper (*p);
        }
        fputs (line, stdout);
    }
    return 0;
}
