/*
** hyerrstr.c - hydra.h's error texts (apart from hydra.c's, so a program that never names an error doesn't take in
** the texts)
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hydra.h>

const char* __fastcall__ hy_errstr (unsigned char code)
{
    return _stroserror (code);
}
