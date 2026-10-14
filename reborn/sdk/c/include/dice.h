/*
** dice.h - dice for games: danlang's lib/dice.dl in C (the same functions, by the same names where C allows: a +
** is _plus, a - is _), and the random numbers they're rolled with.
**
** The numbers are hylang's and HyForth's (lib random): xorshift32 (x ^= x << 13, x ^= x >> 17, x ^= x << 5: every
** 32-bit value but 0, in turn), seeded at the first use from the clock and the ticks, unless rseed has seeded it.
** random (n) is from 0 to n - 1, as hylang's (random n) and HyForth's random: the high 16 bits of rand16 () * n.
**
** A die of sides sides is 1 to sides; die_plus adds a modifier (die+); d3 ... d100 (and d3_plus ...) are dice.dl's
** named dice.  dice (n, sides) is n of them summed (dice_plus: and a modifier, added once).  sumrolls sums a list of
** rolls, each a die, dice, or dice and a modifier ({ n, sides, mod }: dice.dl's 6, {6}, {3 6} and {3 6 1}).
** special_die rolls a die and gives the value its face has in a table (dice.dl's special-die: case).  The open-ended
** dice roll again on the highest face, and add (oe_die: a 6 on a d6 is 6 more, and so on).  roll calls a function
** n times, its values into an array (dice.dl's roll, globals.dl's repeat); best_of_rolls and worst_of_rolls keep the
** best (or worst) a of b such rolls; dnd_attr_rolls is a set of six D&D attributes: eight rolls of 4d6 less the
** lowest, the best six (dnd_attr_rolls_detailed: each with its four dice too).
*/

#ifndef _DICE_H
#define _DICE_H

/* ---- The random numbers */

unsigned rand16 (void);                         /* The next, 16 bits (HyForth's rand) */
unsigned long rand32 (void);                    /* The next, 32 bits (rand32) */
void __fastcall__ rseed (unsigned long seed);   /* The generator started from seed (0: 1) (HyForth's rseed) */
unsigned __fastcall__ random (unsigned n);      /* From 0 to n - 1 (n 0: 0) */

/* ---- Dice */

int __fastcall__ die_plus (unsigned sides, int mod);    /* 1 to sides, and mod */
int __fastcall__ die (unsigned sides);                  /* 1 to sides */

#define d3()            die (3)
#define d3_plus(m)      die_plus (3, (m))
#define d4()            die (4)
#define d4_plus(m)      die_plus (4, (m))
#define d6()            die (6)
#define d6_plus(m)      die_plus (6, (m))
#define d8()            die (8)
#define d8_plus(m)      die_plus (8, (m))
#define d10()           die (10)
#define d10_plus(m)     die_plus (10, (m))
#define d12()           die (12)
#define d12_plus(m)     die_plus (12, (m))
#define d20()           die (20)
#define d20_plus(m)     die_plus (20, (m))
#define d24()           die (24)
#define d24_plus(m)     die_plus (24, (m))
#define d30()           die (30)
#define d30_plus(m)     die_plus (30, (m))
#define d60()           die (60)
#define d60_plus(m)     die_plus (60, (m))
#define d100()          die (100)
#define d100_plus(m)    die_plus (100, (m))

int __fastcall__ dice_plus (unsigned char n, unsigned sides, int mod);  /* n dice summed, and mod */
int __fastcall__ dice (unsigned char n, unsigned sides);                /* n dice summed */

struct dice_spec {                              /* One of sumrolls's rolls: n dice of sides sides, and mod */
    unsigned char   n;
    unsigned        sides;
    int             mod;
};
int __fastcall__ sumrolls (const struct dice_spec* d, unsigned char count);

struct dice_case {                              /* A face of a special die, and its value */
    unsigned        face;
    int             value;
};
int __fastcall__ special_die (unsigned sides, const struct dice_case* cases, unsigned char count);
                                                /* The value of the face rolled (none in cases: 0) */

int __fastcall__ oe_die_plus (unsigned sides, int mod);         /* Open-ended: the highest face rolls again */
int __fastcall__ oe_die (unsigned sides);
int __fastcall__ oe_dice_plus (unsigned char n, unsigned sides, int mod);
int __fastcall__ oe_dice (unsigned char n, unsigned sides);

void __fastcall__ roll (int* out, unsigned char n, int (*f) (void));    /* n of f's values, into out */
int __fastcall__ best_of_rolls (int* out, unsigned char a, unsigned char b, int (*f) (void));
                                                /* The best a of b rolls of f, into out (the best first): their sum.
                                                **   b at most 16 */
int __fastcall__ worst_of_rolls (int* out, unsigned char a, unsigned char b, int (*f) (void));
                                                /* The worst a, the worst first */
void __fastcall__ dnd_attr_rolls (unsigned char* out);          /* Six attributes, the best first */
void __fastcall__ dnd_attr_rolls_detailed (unsigned char* out, unsigned char (*rolled)[4]);
                                                /* And each one's four dice, the best first (rolled[6][4]) */

#endif
