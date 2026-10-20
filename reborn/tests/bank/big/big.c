/*
** big.c - a test child (bankprog.h; t_bank's) of 5,000 bytes and more: two of it (b1.bp, b2.bp) don't fit in one
** bank with the rest, so loading the second gives up what was used least lately.  Its run: x and its table's first
*/

static const unsigned char table[5000] = { 1, 2, 3 };

int __fastcall__ run (int x)
{
    return x + table[0];
}
