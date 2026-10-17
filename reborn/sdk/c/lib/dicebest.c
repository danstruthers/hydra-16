/*
** dicebest.c - dice.h's roll (dice.dl's roll, globals.dl's repeat), best-of-rolls, worst-of-rolls and the D&D
** attribute rolls: danlang's lib/dice.dl's
*/

#include <dice.h>

void __fastcall__ roll (int* out, unsigned char n, int (*f) (void))
{
    while (n--) {
        *out++ = f ();
    }
}

/* b of f's rolls, sorted (best first, or worst first), the first a of them into out: their sum */
static int __fastcall__ keep (int* out, unsigned char a, unsigned char b, int (*f) (void), unsigned char best)
{
    int r[16], t;
    unsigned char i, j;
    int sum = 0;

    if (b > 16) {
        b = 16;
    }
    roll (r, b, f);
    for (i = 1; i < b; ++i) {                   /* (An insertion sort: b is small) */
        t = r[i];
        for (j = i; j && (best ? r[j - 1] < t : r[j - 1] > t); --j) {
            r[j] = r[j - 1];
        }
        r[j] = t;
    }
    for (i = 0; i < a && i < b; ++i) {
        sum += out[i] = r[i];
    }
    return sum;
}

int __fastcall__ best_of_rolls (int* out, unsigned char a, unsigned char b, int (*f) (void))
{
    return keep (out, a, b, f, 1);
}

int __fastcall__ worst_of_rolls (int* out, unsigned char a, unsigned char b, int (*f) (void))
{
    return keep (out, a, b, f, 0);
}

static int d6f (void)
{
    return d6 ();
}

/* One attribute, dnd-attr-rolls-detailed's: four d6 (the best first, into four) and the best three's sum */
static unsigned char __fastcall__ attr (unsigned char* four)
{
    int r[4];
    unsigned char k, sum = 0;

    best_of_rolls (r, 4, 4, d6f);
    for (k = 0; k < 4; ++k) {
        four[k] = r[k];
        if (k < 3) {
            sum += r[k];
        }
    }
    return sum;
}

void __fastcall__ dnd_attr_rolls_detailed (unsigned char* out, unsigned char (*rolled)[4])
{
    unsigned char v[8], d[8][4], i, j, k, t, td[4];

    for (i = 0; i < 8; ++i) {                   /* Eight rolls ... */
        v[i] = attr (d[i]);
    }
    for (i = 1; i < 8; ++i) {                   /*   sorted, the best first ... */
        t = v[i];
        for (k = 0; k < 4; ++k) {
            td[k] = d[i][k];
        }
        for (j = i; j && v[j - 1] < t; --j) {
            v[j] = v[j - 1];
            for (k = 0; k < 4; ++k) {
                d[j][k] = d[j - 1][k];
            }
        }
        v[j] = t;
        for (k = 0; k < 4; ++k) {
            d[j][k] = td[k];
        }
    }
    for (i = 0; i < 6; ++i) {                   /*   and the first six */
        out[i] = v[i];
        if (rolled) {
            for (k = 0; k < 4; ++k) {
                rolled[i][k] = d[i][k];
            }
        }
    }
}

void __fastcall__ dnd_attr_rolls (unsigned char* out)
{
    dnd_attr_rolls_detailed (out, 0);
}
