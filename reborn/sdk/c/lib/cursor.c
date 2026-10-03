/*
** cursor.c - cursor(): the terminal's cursor shown (1) or hidden (0), with ANSI's ESC [ ? 25 h / l.  The old
** setting comes back.  (It takes the place of cc65's cursor, which sets a flag for a target's own cgetc.)
*/

#include <conio.h>

void __fastcall__ _hy_putc (char c);                    /* conglue.s */

static unsigned char shown = 1;

unsigned char __fastcall__ cursor (unsigned char onoff)
{
    unsigned char old = shown;

    shown = onoff != 0;
    _hy_putc (27);
    _hy_putc ('[');
    _hy_putc ('?');
    _hy_putc ('2');
    _hy_putc ('5');
    _hy_putc (shown ? 'h' : 'l');
    return old;
}
