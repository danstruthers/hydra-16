/*
** shapes.c - cc65's TGI (tgi.h) on the Vera X (vera.h's hydra_tgi): a line, a bar, a circle, an ellipse and text on
** the bitmap under the console, then some of their pixels read back (tgi_getpixel) and said.  shapes [-w] (-w: a
** key waited for before the bitmap goes)
**   % shapes -w
*/

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <tgi.h>
#include <hydra.h>
#include <vera.h>

int main (int argc, char* argv[])
{
    unsigned x, y, dots = 0;

    tgi_install (hydra_tgi);
    tgi_init ();
    if (tgi_geterror () != TGI_ERR_OK) {
        puts ("shapes: no screen (a Vera X?)");
        return 1;
    }
    tgi_clear ();
    tgi_setcolor (4);
    tgi_line (0, 0, tgi_getmaxx (), tgi_getmaxy ());
    tgi_setcolor (2);
    tgi_bar (10, 10, 59, 39);
    tgi_setcolor (14);
    tgi_circle (160, 120, 50);
    tgi_setcolor (11);
    tgi_ellipse (160, 120, 80, 30);
    tgi_setcolor (15);
    tgi_outtextxy (100, 200, "Hydra-16");
    for (y = 200; y < 208; ++y) {                   /* The text's dots */
        for (x = 100; x < 164; ++x) {
            dots += tgi_getpixel (x, y) == 15;
        }
    }
    printf ("%ux%u, %u colours: %u %u %u %u %u, text %u dots\n", tgi_getxres (), tgi_getyres (),
        tgi_getcolorcount () ? tgi_getcolorcount () : 256,         /* (TGI's count is a byte: 256 is 0) */
        tgi_getpixel (0, 0), tgi_getpixel (160, 120), tgi_getpixel (20, 20), tgi_getpixel (210, 120), tgi_getpixel (240, 120), dots);
    if (argc > 1 && strcmp (argv[1], "-w") == 0) {
        cgetc ();
    }
    tgi_done ();
    tgi_uninstall ();
    return 0;
}
