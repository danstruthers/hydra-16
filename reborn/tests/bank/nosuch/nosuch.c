/*
** nosuch.c - a test child (bankprog.h; t_bank's) that uses what its caller hasn't (nosuch): refused as it's loaded
** (BE_IMPORT, bank_missing its name).  It names no parents: the build would refuse it
*/

extern int nosuch;

int __fastcall__ run (int x)
{
    return nosuch + x;
}
