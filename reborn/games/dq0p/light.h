/*
** light.h - (dq0p) the light layer: light.c
*/

#define L_ROOM          6                       /* A lit room's own light (R_LIT, its floor and walls) */
#define NLS             8                       /* The lit things lying about the light layer keeps (LAMP_FAR off) */
#define LAMP_FAR        64
struct lsrc { unsigned x, y; unsigned char r; };
extern struct lsrc ls[NLS];                     /* (As the last look found them: their squares, their reach) */
extern unsigned char nls;

void light_init (void);                         /* Its bank (the senses' own), its place in the world's events */
void light_events (void);                       /* The world's E_LIGHT since: the grids near them out of date */
void light_lamps_begin (void);                  /* The lamps found again (things.c's, each look: light_lamp each), */
void __fastcall__ light_lamp (unsigned x, unsigned y, unsigned char r);
void __fastcall__ light_lamps_end (unsigned char sn);   /*   those come or gone: the grids near them out of date */
unsigned char __fastcall__ cached (unsigned char sn, unsigned x, unsigned y);  /* Square x, y's chunk made already */
unsigned char* __fastcall__ light_grid (unsigned char sn, unsigned x, unsigned y, unsigned char mk);  /* Its chunk's
                                                **   grid, in the senses' bank (mk: worked out if it isn't), or 0 */
unsigned char __fastcall__ light_at (unsigned char sn, unsigned x, unsigned y);    /* How bright it is: 0-15 */
