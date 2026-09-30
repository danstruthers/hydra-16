/*
** code.c - ends with the exit status its argument gives: a number (the code), or anything else (hy_exits: that
** message, and 1); none: 0 (success).  The shell keeps it: its status word, and $status.
**     0:/> code 3
**     0:/> status .
**      0003
*/

#include <stdlib.h>
#include <ctype.h>
#include <hydra.h>

int main (int argc, char* argv[])
{
    if (argc < 2) {
        return 0;
    }
    if (isdigit (argv[1][0])) {
        return atoi (argv[1]);
    }
    hy_exits (argv[1]);
    return 0;
}
